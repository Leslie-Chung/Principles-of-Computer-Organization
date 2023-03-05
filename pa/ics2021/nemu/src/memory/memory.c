#include "nemu.h"
#include "device/mmio.h"
#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

paddr_t page_translate(vaddr_t vaddr, bool _write) {
	PDE pde;
  uint32_t PDE_addr = cpu.cr3.page_directory_base << 12;
  uint32_t DIR = vaddr >> 22;
  PDE_addr = PDE_addr | (DIR << 2);
  pde.val = paddr_read(PDE_addr, 4);
  Assert(pde.present, "PDE: 0x%08x, vaddr: 0x%08x", pde.val, vaddr);

  if(pde.accessed == 0){
    pde.accessed = 1;
    paddr_write(PDE_addr, 4, pde.val);
  }

  PTE pte;
  uint32_t PTE_addr = pde.val & 0xfffff000;
  uint32_t PAGE = (vaddr >> 12) & 0x3ff;
  PTE_addr = PTE_addr | (PAGE << 2);
  pte.val = paddr_read(PTE_addr, 4);
  Assert(pte.present, "PTE: 0x%08x, vaddr: 0x%08x", pte.val, vaddr);

  if(pte.accessed == 0 || (pte.dirty == 0 && _write)){
    pte.accessed = 1;
    pte.dirty = 1;
    paddr_write(PTE_addr, 4, pte.val);
  }

  paddr_t paddr = (pte.val & 0xfffff000) | (vaddr & 0xfff);
  // Log("vaddr: 0x%08x, paddr: 0x%08x", vaddr, paddr);
	return paddr;
}


uint32_t paddr_read(paddr_t addr, int len) {
  int mmio_id = is_mmio(addr);
    if (mmio_id != -1) {
    return mmio_read(addr, len, mmio_id);
  }
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int mmio_id = is_mmio(addr);
  if (mmio_id != -1) {
    mmio_write(addr, len, data, mmio_id);
  }
  else memcpy(guest_to_host(addr), &data, len);
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if(cpu.cr0.paging) {
    if ((addr & 0xfff) + len > 0x1000) {//如果是跨页访问，即从要访问的起始位置+要访问的长度超过了一页的大小
      int low_len = 0x1000 - (addr & 0xfff);
      int high_len = len - low_len;
      uint32_t low_data = paddr_read(page_translate(addr, false), low_len);
      uint32_t high_data = paddr_read(page_translate(addr + low_len, false), high_len);
      return (high_data << (low_len << 3)) | low_data;
    }
    else {
      paddr_t paddr = page_translate(addr, false);
      return paddr_read(paddr, len);
    }
  } 
  else {
    return paddr_read(addr, len);
  }

}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if(cpu.cr0.paging) {
    if ((addr & 0xfff) + len > 0x1000) {
      assert(0);
    }
    else {
      paddr_t paddr = page_translate(addr, true);
      return paddr_write(paddr, len, data);
    }
  } 
  else {
    paddr_write(addr, len, data);
  }
}