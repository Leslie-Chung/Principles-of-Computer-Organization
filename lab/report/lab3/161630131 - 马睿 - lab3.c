#include "common.h"

void mem_read(uintptr_t block_num, uint8_t *buf);
void mem_write(uintptr_t block_num, const uint8_t *buf);
// TODO: implement the following functions
typedef struct {
	uint8_t valid : 1;	  // 有效位,1位
	uint8_t dirty : 1; // 脏位，1位
	uint32_t tag;	 // 标记位
	uint8_t data[BLOCK_SIZE];	 // 数据区，512位
} CacheSlot;

CacheSlot **cache;

int row_count;//cache总行数
int rows_of_group;// 每组行数
int group_count;// cache组数
int group_id_width;// cache组号长度
int tag_width;// tag长度

// 从cache中读出`addr`地址处的4字节数据
// 若缺失, 需要先从内存中读入数据
uint32_t cache_read(uintptr_t addr) {
	try_increase(1); 
	uint32_t val;

	addr = addr & ~0x3;//使地址按字节对齐
	uint32_t tag = (addr >> (32 - tag_width)) & ~(~0 << tag_width); //得到标记
	uint32_t group_id = (addr >> 6) & ~(~0 << group_id_width); //得到组号
	uint32_t offset = addr & ~(~0 << BLOCK_WIDTH); //得到块内地址

	int i, j;
	for (i = 0;i < rows_of_group;i++) {//遍历相应组号中的所有行
		if (cache[group_id][i].valid && cache[group_id][i].tag == tag) {// 判断有效位和标记
			hit_increase(1); 
			val = 0;
			for (j = 3; j >= 0; j--) { //小端方式，所以先从大地址开始取数
				val *= 0x100;
				val += cache[group_id][i].data[offset + j];
			}
			return val;
		}
	}

	i = rand() % rows_of_group;// 随机替换
	if (cache[group_id][i].dirty == 1) {// 脏位为1需要先写回到内存中
		mem_write((cache[group_id][i].tag << group_id_width) + group_id, cache[group_id][i].data);
	}
	mem_read(addr >> BLOCK_WIDTH, cache[group_id][i].data);
	cache[group_id][i].tag = addr >> (32 - tag_width);
	cache[group_id][i].valid = 1;
	cache[group_id][i].dirty = 0;
	val = 0;
	for (j = 3; j >= 0; j--) {
		val *= 0x100;
		val += cache[group_id][i].data[offset + j];
	}

	return val;
}

// 往cache中`addr`地址所属的块写入数据`data`, 写掩码为`wmask`
// 例如当`wmask`为`0xff`时, 只写入低8比特
// 若缺失, 需要从先内存中读入数据
void cache_write(uintptr_t addr, uint32_t data, uint32_t wmask) {
	try_increase(1);
	uint32_t * p;

	addr = addr & ~0x3;
	uint32_t tag = (addr >> (6 + group_id_width)) & ~(~0 << tag_width);
	uint32_t group_id = (addr >> 6) & ~(~0 << group_id_width);
	uint32_t offset = addr & ~(~0 << BLOCK_WIDTH);

	int i;
	for (i = 0; i < rows_of_group; i++) {
		if (cache[group_id][i].valid && cache[group_id][i].tag == tag) {
			hit_increase(1);
			cache[group_id][i].dirty = 1;
			p = (uint32_t *)&cache[group_id][i].data[offset];
			*p = (*p & ~wmask) | (data & wmask);
			return;
		}
	}


	i = rand() % rows_of_group;
	if (cache[group_id][i].dirty == 1) {
		mem_write((cache[group_id][i].tag << group_id_width) + group_id, cache[group_id][i].data);
	}
	mem_read(addr >> BLOCK_WIDTH, cache[group_id][i].data);
	cache[group_id][i].tag = addr >> (32 - tag_width);
	cache[group_id][i].valid = 1;
	cache[group_id][i].dirty = 1;
	p = (uint32_t *)&cache[group_id][i].data[offset];
	*p = (*p & ~wmask) | (data & wmask);
	return;
}

// 初始化一个数据大小为 2^total_size_width B, 关联度为 2^associativity_width 的cache
// 例如 init_cache(14, 2) 将初始化一个16KB, 4路组相联的cache
// 将所有valid bit置为无效即可
void init_cache(int total_size_width, int associativity_width) {
	row_count = exp2(total_size_width) / BLOCK_SIZE;
	rows_of_group = exp2(associativity_width);
	group_count = row_count / rows_of_group;

	 /*2 ^ (total_size_width - BLOCK_WIDTH) / 2 ^ (associativity_width) = 2 ^ (group_id_width)
	 --> 行数/关联度 = 每组行数 
	 */
	group_id_width = total_size_width - BLOCK_WIDTH - associativity_width;
	tag_width = 32 - BLOCK_WIDTH - group_id_width;
	cache = (CacheSlot * *)malloc(sizeof(CacheSlot *) * group_count);
	int i;
	for (i = 0; i < group_count; i++) {
		cache[i] = (CacheSlot *)calloc(rows_of_group, sizeof(CacheSlot));
	}
	
	return;
}

