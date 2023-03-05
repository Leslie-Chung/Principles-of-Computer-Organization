#include "common.h"
#include"fs.h"

#define DEFAULT_ENTRY ((void *)0x8048000)
// 从ramdisk中`offset`偏移处的`len`字节读入到`buf`中
void ramdisk_read(void *buf, off_t offset, size_t len);

// 把`buf`中的`len`字节写入到ramdisk中`offset`偏移处
void ramdisk_write(const void *buf, off_t offset, size_t len);

// 返回ramdisk的大小, 单位为字节
size_t get_ramdisk_size();
void* new_page(void);
uintptr_t loader(_Protect *as, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  size_t f_size = fs_filesz(fd);

  void *va = DEFAULT_ENTRY;
  void *pa = NULL;
  int pages = f_size / PGSIZE + 1;
  int i;
  for (i = 0; i < pages;i++){
    pa = new_page();
    // Log("Map va to pa: 0x%08x to 0x%08x", va, pa);
    _map(as, va, pa);
    fs_read(fd, pa, PGSIZE);
    va += PGSIZE;
  }
  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
