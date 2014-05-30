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

////获取log缓冲，该函数需要同步访问，因为同时可能有多个进程获取缓冲
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

////释放log缓冲，该函数需要同步，因为同时可能又多个进程放回缓冲
void 
put_log_buf(struct LogBufferMgr *log_mgr, char *buf)
{
	if(log_mgr->frees == log_mgr->buf_count){
		panic("log buf manager fail: too much put_log_buf");
	}
	log_mgr->frees++;
}
