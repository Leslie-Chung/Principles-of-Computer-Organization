#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "memory/memory.h"
#include "cpu/reg.h"
#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(){
	if(free_ == NULL) {
		printf("There is no more memory to set watchpoint. You should delete some watchpoints to free memory.\n");
		return NULL;
	}
	WP* p = free_;
	free_ = free_->next;
	p->next = head;
	head = p;
	return head;
}
void free_wp(WP *wp){
	if(head == NULL){
		printf("There is no watchpoint to free.\n");
		return ;
	}
	if(wp == head){
		head = head->next;
		wp->next = free_;
		free_ = wp;	
		return ;
	}
	WP *p = head;
	while(p->next != wp) p = p->next;
	p->next = wp->next;
	wp->next = free_;
	free_ = wp;
}

int set_watchpoint(char *e){
 	WP* wp = new_wp();		
	if(wp == NULL)	return -1;
	memset(wp->expr, 0, sizeof(wp->expr));
	strcpy(wp->expr, e);
	wp->type = 0;
	bool success = true;
	wp->old_val = expr(e, &success);
	if(!success){
		printf("set watchpoint failed. Please check your exprssion!\n");
		free_wp(wp);
		return -1;
	}
	return wp->NO;
}

bool delete_watchpoint(int NO)
{
	WP *wp = head;
	while (wp && wp->NO != NO){
		wp = wp->next;
	}
	if (wp){
		free_wp(wp);
		return true;
	}
	else  return false;
}

void list_watchpoint(void){
	if(head == NULL) {
		printf("There is no watchpoints!\n");
		return;
	}	
	printf("NO  Expr            Old Value\n");
	WP *p = head;							  
	while (p) {
		if (p->type == 0) {
			printf("%2d  %-16s%#010x\n", p->NO, p->expr, p->old_val);
		}
		p = p->next;
	}
}

WP* scan_watchpoint(){
	WP *p = head;
	bool success;
	uint32_t new_value = 0;
	while(p){
		if(p->type == 0){
			success = true;
			new_value = expr(p->expr, &success);
			if(p->old_val != new_value){
				p->new_val = new_value;
				return p;
			}
		}
		else{
			if(p->new_val == 1){
			//	printf("%08x\n",cpu.eip);
				cpu.eip -= 1;
				p->new_val = 2;
			}
			else if(p->new_val == 2){
				*p->expr = *(char *)guest_to_host(p->old_val);
				*(char *)guest_to_host(p->old_val) = 0xcc;
				p->new_val = 0;
			}
		}
		
		p = p->next;
	}
	return NULL;
}

int set_breakpoint(char *e){
 	WP* wp = new_wp();
	if(wp == NULL) return -1;
	memset(wp->expr, 0, sizeof(wp->expr));
	wp->type = 1; //断点
	bool success = true;
	wp->old_val = expr(e, &success); //old_value保存 设置断点的地址（程序执行到该地址处中断）
	if(!success){
		printf("set breakpoint failed. Please check your exprssion!\n");
		free_wp(wp);
		return -1;	
	}
	wp->new_val = 0;
	*wp->expr = *(char *)guest_to_host(wp->old_val);//expr保存原来的操作码	
	/*
	 *     #define guest_to_host(p) ((void *)(pmem + (unsigned)p))
	 *      convert the guest physical address in the guest program to host virtual address in NEMU   
  */
	*(char *)guest_to_host(wp->old_val) = 0xcc;//从内存中修改断点地址对应的操作码（机器指令中，第一个字节就是操作码）为0xcc

	return wp->NO;
}

void recover_int3() {
	WP *p = head;
	while (p) {
		if (p->type == 1 && p->old_val == cpu.eip){//到了断点位置
			*(char *)guest_to_host(p->old_val) = *p->expr;
			p->new_val = 1;
			break;
		}
		p = p->next;
	}
}
