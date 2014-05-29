#ifndef _MEMORY_H
#define _MEMORY_H

#define PAGE_DIR_ADDR 0x00400000
#define PAGE_BIT_MAP_ADDR  0x00900000
#define NO_FREE_PAGE_ADDR 0
#define MB *(1024 * 1024)
#define KB *(1024)
#define TOTAL_MEM 0x04000000
#define LOW_MEM (0x00b00000 + 0x01000000)  //0~0x00b00000Ϊ�ں˵Ĵ�����������ڿռ䣬
										   //0x00b00000~0x01000000Ϊ�ں˶�̬�ɷ�����ڴ�
										  
#define HIGH_MEMORY  TOTAL_MEM   
unsigned int get_and_map_free_pages(u32 addr, int n);
int free_page_tables(unsigned long from,unsigned long size);

#endif
