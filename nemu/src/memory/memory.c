#include "common.h"
#include <string.h>
#include <stdlib.h>
#include "burst.h"
#include "nemu.h"
#define BLOCK_SIZE 64
#define OFFSETBIT 6

#define CACHESIZE1 64*1024
#define EIGHT_WAY 8
#define GROUPNUMBER 1024/8
#define GROUPBIT 7

#define CACHESIZE2 4*1024*1024
#define SIXTEEN_WAY 16
#define GROUPNUMBER2 4*1024
#define GROUPBIT2 12
uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);
void ddr3_read(hwaddr_t, void*);
void ddr3_write(hwaddr_t, void*,uint8_t*);
/* Memory accessing interfaces */
struct Cache
{
	bool valid;
	int tag;
	uint8_t data[BLOCK_SIZE];
}cache[CACHESIZE1/BLOCK_SIZE];

struct SecondaryCache
{
	bool valid;
	bool dirty;
	int tag;
	uint8_t data[BLOCK_SIZE];
}cache2[CACHESIZE2/BLOCK_SIZE];

void init_cache()
{
	int i;
	for (i = 0;i < CACHESIZE1/BLOCK_SIZE;i ++)
	{
		cache[i].valid = false;
		cache[i].tag = 0;
		memset (cache[i].data,0,BLOCK_SIZE);
	}
	for (i = 0;i < CACHESIZE2/BLOCK_SIZE;i ++)
	{
		cache2[i].valid = false;
		cache2[i].dirty = false;
		cache2[i].tag = 0;
		memset (cache2[i].data,0,BLOCK_SIZE);
	}
}

/* Memory accessing interfaces */
uint32_t cache2_read(hwaddr_t addr) 
{
	uint32_t g = (addr >> (OFFSETBIT)) & (GROUPNUMBER2-1); //group number
	uint32_t block = (addr >> OFFSETBIT)<<OFFSETBIT;
	int i;
	bool v = false;
	for (i = g * SIXTEEN_WAY ; i < (g + 1) * SIXTEEN_WAY ;i ++)
	{
		if (cache2[i].tag == (addr >> (OFFSETBIT+GROUPBIT2))&& cache2[i].valid)
			{
				//count+=20;
				v = true;
				break;
			}
	}
	if (!v)
	{
		//count==200;
		int j;
		for (i = g * SIXTEEN_WAY ; i < (g + 1) * SIXTEEN_WAY ;i ++)
		{
			if (!cache2[i].valid)break;
		}
		if (i == (g + 1) * SIXTEEN_WAY)//ramdom
		{
			srand (0);
			i = g * SIXTEEN_WAY + rand() % SIXTEEN_WAY;
			if (cache2[i].dirty)
			{
				uint8_t mask[BURST_LEN * 2];
				memset(mask, 1, BURST_LEN * 2);
				for (j = 0;j < BLOCK_SIZE/BURST_LEN;j ++)
				ddr3_write(block + j * BURST_LEN, cache2[i].data + j * BURST_LEN, mask);
			}
		}
		cache2[i].valid = true;
		cache2[i].tag = addr >> (OFFSETBIT+GROUPBIT2);
		cache2[i].dirty = false;
		for (j = 0;j < BURST_LEN;j ++)
		ddr3_read(block + j * BURST_LEN , cache2[i].data + j * BURST_LEN);
	}
	return i;
}

uint32_t cache_read(hwaddr_t addr) 
{
	uint32_t g = (addr>>OFFSETBIT) & (GROUPNUMBER-1); //group number
	int i;
	bool v = false;
	for (i = g * EIGHT_WAY ; i < (g + 1) * EIGHT_WAY ;i ++)
	{
		if (cache[i].tag == (addr >> (OFFSETBIT+GROUPBIT))&& cache[i].valid)
		{
		//	timecount+=2;
			v = true;
			break;
		}
	}
	if (!v)
	{
		//timecount+=20;
		int j = cache2_read (addr);
		for (i = g * EIGHT_WAY ; i < (g+1) * EIGHT_WAY ;i ++)
		{
			if (!cache[i].valid)break;
		}
		if (i == (g + 1) * EIGHT_WAY)//ramdom
		{
			srand (0);
			i = g * EIGHT_WAY + rand() % EIGHT_WAY;
		}
		cache[i].valid = true;
		cache[i].tag = addr >> (OFFSETBIT+GROUPBIT);
		memcpy (cache[i].data,cache2[j].data,BLOCK_SIZE);
	}
	return i;
}

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	//bigcount+=20;
	uint32_t offset = addr & (BLOCK_SIZE - 1);//pian yi liang
	uint32_t block = cache_read(addr);//position
	uint8_t temp[4];
	memset (temp,0,sizeof (temp));
	if (offset + len >= BLOCK_SIZE) //in more than one block
	{
		uint32_t _block = cache_read(addr + len);
		memcpy(temp,cache[block].data + offset, BLOCK_SIZE - offset);
		memcpy(temp + BLOCK_SIZE - offset,cache[_block].data, len - (BLOCK_SIZE - offset));
	}
	else
	{
		memcpy(temp,cache[block].data + offset,len);
	}
	int zero = 0;
	uint32_t tmp = unalign_rw(temp + zero, 4) & (~0u >> ((4 - len) << 3)); 
	//Log("%d %d",bigcount,timecount);
	return tmp;
//	return dram_read(addr, len) & (~0u >> ((4 - len) << 3));
}

void cache2_write(hwaddr_t addr, size_t len,uint32_t data) {
	uint32_t g = (addr >> OFFSETBIT) & (GROUPNUMBER2-1);  //group number
	uint32_t offset = addr & (BLOCK_SIZE - 1); // inside addr
	int i;
	bool v = false;
	for (i = g * SIXTEEN_WAY ; i < (g + 1) * SIXTEEN_WAY ;i ++)
	{
		if (cache2[i].tag == (addr >> (OFFSETBIT+GROUPBIT))&& cache2[i].valid)
		{
			v = true;
			break;
		}
	}
	if (!v)
		i = cache2_read (addr);
	cache2[i].dirty = true;
	memcpy (cache2[i].data + offset , &data , len);
}

void cache_write(hwaddr_t addr, size_t len,uint32_t data) {
	uint32_t g = (addr>>OFFSETBIT) & (GROUPNUMBER-1); //group number
	uint32_t offset = addr & (BLOCK_SIZE - 1); // inside addr
	int i;
	bool v = false;
	for (i = g * EIGHT_WAY ; i < (g + 1) * EIGHT_WAY ;i ++)
	{
		if (cache[i].tag == (addr >> 13)&& cache[i].valid)
			{
				v = true;
				break;
			}
	}
	if (v)
	{
		memcpy (cache[i].data + offset , &data , len);
	}
	cache2_write(addr,len,data);
}
void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	cache_write(addr, len, data);
	//dram_write(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	hwaddr_write(addr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	return lnaddr_read(addr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_write(addr, len, data);
}

