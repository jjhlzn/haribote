#include "bootpack.h"
#define LOG_BUF_COUNT 1024


void 
init_logmgr(struct LogBufferMgr *log_mgr)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	log_mgr->buf_count = LOG_BUF_COUNT;
	log_mgr->frees = log_mgr->buf_count;
	log_mgr->nr_free_buf = 0;

	char *buf = (char *)memman_alloc_4k(memman, LOG_ENTRY_SIZE * LOG_BUF_COUNT);
	log_mgr->buf = buf;
}

////��ȡlog���壬�ú�����Ҫͬ�����ʣ���Ϊͬʱ�����ж�����̻�ȡ����
char *
get_log_buf(struct LogBufferMgr *log_mgr)
{
	if(log_mgr->frees == 0)
		return NULL;
	char *buf_entry = log_mgr->buf + log_mgr->nr_free_buf * LOG_ENTRY_SIZE;
	log_mgr->frees--;
	log_mgr->nr_free_buf++;
	if(log_mgr->nr_free_buf == log_mgr->buf_count)
		log_mgr->nr_free_buf = 0;
	return buf_entry;
}

////�ͷ�log���壬�ú�����Ҫͬ������Ϊͬʱ�����ֶ�����̷Żػ���
void 
put_log_buf(struct LogBufferMgr *log_mgr, char *buf)
{
	if(log_mgr->frees == log_mgr->buf_count){
		panic("log buf manager fail: too much put_log_buf");
	}
	log_mgr->frees++;
}
