/*
 *  linux/fs/open.c
 *
 *  (C) 1991  Linus Torvalds
 *  
 *  本文件实现了许多与文件操作相关的系统调用。主要有文件的创建、打开和关闭，文件宿主和属性
 *  的修改、文件访问权限的修改、文件操作时间的修改和系统文件系统root的变动等。
 */

#include "bootpack.h"
#include "fs.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>

#include <asm/segment.h>

int sys_dup(unsigned int fildes);

int sys_ustat(int dev, struct ustat * ubuf)
{
	return -ENOSYS;
}

int sys_utime(char * filename, struct utimbuf * times)
{
	struct m_inode * inode;
	long actime,modtime;

	if (!(inode=namei(filename)))
		return -ENOENT;
	if (times) {
		actime = get_fs_long((unsigned long *) &times->actime);
		modtime = get_fs_long((unsigned long *) &times->modtime);
	} else
		actime = modtime = CURRENT_TIME;
	inode->i_atime = actime;
	inode->i_mtime = modtime;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

/*
 * XXX should we use the real or effective uid?  BSD uses the real uid,
 * so as to make this call useful to setuid programs.
 */
////检查文件的访问权限
//参数filename是文件名，mode是检查的访问属性，它有3个有效比特位组成：R_OK（值4）、
//W_OK(2)、X_OK(1)和F_OK(0)组成，分别表示检测文件是否可读、可写、可执行和文件是否
//存在。如果访问允许的话，则返回0，否则返回出错码。
int sys_access(const char * filename,int mode)
{
	struct m_inode * inode;
	int res, i_mode;

	mode &= 0007;
	if (!(inode=namei(filename)))
		return -EACCES;
	i_mode = res = inode->i_mode & 0777;
	iput(inode);
	if (current->uid == inode->i_uid)
		res >>= 6;
	else if (current->gid == inode->i_gid)
		res >>= 6;
	if ((res & 0007 & mode) == mode)
		return 0;
	/*
	 * XXX we are doing this test last because we really should be
	 * swapping the effective with the real user id (temporarily),
	 * and then calling suser() routine.  If we do call the
	 * suser() routine, it needs to be called last. 
	 */
	if ((!current->uid) &&
	    (!(mode & 1) || (i_mode & 0111)))
		return 0;
	return -EACCES;
}

////改变当前工作目录系统调用
//参数filename是目录名
//操作成功则返回0，否则返回出错码
int sys_chdir(const char * filename)
{
	struct m_inode * inode;

	if (!(inode = namei(filename)))
		return -ENOENT;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	iput(current->pwd);
	current->pwd = inode;
	return (0);
}

///根表跟目录系统调用
//把指定的目录名设置为当前进程的根目录'/'
//如果操作成功则返回0，否则返回出错码
int sys_chroot(const char * filename)
{
	struct m_inode * inode;

	if (!(inode=namei(filename)))
		return -ENOENT;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	iput(current->root);
	current->root = inode;
	return (0);
}

////修改文件属性系统调用
//参数filename是文件名，mode是新的文件属性
//如果操作成功则返回0，否则返回出错码
int sys_chmod(const char * filename,int mode)
{
	struct m_inode * inode;

	if (!(inode=namei(filename)))
		return -ENOENT;
	if ((current->euid != inode->i_uid) && !suser()) {
		iput(inode);
		return -EACCES;
	}
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

////修改文件宿主系统调用
int sys_chown(const char * filename,int uid,int gid)
{
	struct m_inode * inode;

	if (!(inode=namei(filename)))
		return -ENOENT;
	if (!suser()) {
		iput(inode);
		return -EACCES;
	}
	inode->i_uid=uid;
	inode->i_gid=gid;
	inode->i_dirt=1;
	iput(inode);
	return 0;
}

////打开（或创建）文件系统调用
int sys_open(const char * filename,int flag,int mode)
{
	struct m_inode * inode;
	struct file * f;
	int i,fd;

	mode &= 0777 & ~current->umask;
	//找到一个空闲的文件结构
	for(fd=0 ; fd<NR_OPEN ; fd++)
		if (!current->filp[fd])
			break;
	if (fd>=NR_OPEN)
		return -EINVAL;
	
	current->close_on_exec &= ~(1<<fd);
	
	f=0+file_table;
	for (i=0 ; i<NR_FILE ; i++,f++)
		if (!f->f_count) break;
	if (i>=NR_FILE)
		return -EINVAL;
	(current->filp[fd]=f)->f_count++;
	if ((i=open_namei(filename,flag,mode,&inode))<0) {
		debug("retval of open_namei = %d",i);
		current->filp[fd]=NULL;
		f->f_count=0;
		return i;
	}
	debug("open file successful");
/* ttys are somewhat special (ttyxx major==4, tty major==5) */
	if (S_ISCHR(inode->i_mode)) {
		if (MAJOR(inode->i_zone[0])==4) {
			if (current->leader && current->tty<0) {
				current->tty = MINOR(inode->i_zone[0]);
				//tty_table[current->tty].pgrp = current->pgrp; //CHANGED, 不支持tty
			}
		} else if (MAJOR(inode->i_zone[0])==5)
			if (current->tty<0) {
				iput(inode);
				current->filp[fd]=NULL;
				f->f_count=0;
				return -EPERM;
			}
	}
/* Likewise with block-devices: check for floppy_change */
	if (S_ISBLK(inode->i_mode))
		check_disk_change(inode->i_zone[0]);
	f->f_mode = inode->i_mode;
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
	return (fd);
}

int sys_creat(const char * pathname, int mode)
{
	return sys_open(pathname, O_CREAT | O_TRUNC, mode);
}

int sys_close(unsigned int fd)
{	
	struct file * filp;

	if (fd >= NR_OPEN)
		return -EINVAL;
	current->close_on_exec &= ~(1<<fd);
	if (!(filp = current->filp[fd]))
		return -EINVAL;
	current->filp[fd] = NULL;
	if (filp->f_count == 0)
		panic("Close: file count is 0");
	if (--filp->f_count)
		return (0);
	iput(filp->f_inode);
	return (0);
}

/* 打开进程的三个标准文件，标准输入、标准输出、标准出错 */
PUBLIC void open_std_files(struct TASK *task)
{
	struct file * f;
	int i,fd;
	
	//open stdin
	for(fd=0 ; fd<NR_OPEN ; fd++)
		if (!task->filp[fd])
			break;
	if (fd>=NR_OPEN){
		panic("there is no file descriptor");
	}
	
	f=0+file_table;
	for (i=0 ; i<NR_FILE ; i++,f++)
		if (!f->f_count) break;
	if (i>=NR_FILE){
		panic("there is no free struct file");
	}
	(task->filp[fd]=f)->f_count++;
	//debug("open fd[%d]",fd);
	
	//open stdout
	for(fd=0 ; fd<NR_OPEN ; fd++)
		if (!task->filp[fd])
			break;
	if (fd>=NR_OPEN){
		panic("there is no file descriptor");
	}
	
	f=0+file_table;
	for (i=0 ; i<NR_FILE ; i++,f++)
		if (!f->f_count) break;
	if (i>=NR_FILE){
		panic("there is no free struct file");
	}
	(task->filp[fd]=f)->f_count++;
	//debug("open fd[%d]",fd);
	
	//open stderr
	for(fd=0 ; fd<NR_OPEN ; fd++)
		if (!task->filp[fd])
			break;
	if (fd>=NR_OPEN){
		panic("there is no file descriptor");
	}
	
	f=0+file_table;
	for (i=0 ; i<NR_FILE ; i++,f++)
		if (!f->f_count) break;
	if (i>=NR_FILE){
		panic("there is no free struct file");
	}
	(task->filp[fd]=f)->f_count++;
	//debug("open fd[%d]",fd);
	
	//struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	//int i;
	//for (i = 0; i < NR_FILE_DESC; i++)
	//	if (f_desc_table[i].fd_inode == 0)
	//		break;
	//if (i >= NR_FILE_DESC)
	//	panic("f_desc_table[] is full (PID:%d)", task->pid);
	//task->filp[STDIN] = &f_desc_table[i];
	
	//task->filp[STDIN]->fd_mode = O_RDONLY;
	//task->filp[STDIN]->fd_pos = 0;
	//task->filp[STDIN]->fd_cnt = 1;
	//struct inode* STDIN_node = (struct inode*)memman_alloc(memman, sizeof(struct inode));
	//STDIN_node->i_mode = 1; //I_CHAR_SPECIAL;
	//STDIN_node->i_size = task->pid;
	//task->filp[STDIN]->fd_inode = STDIN_node;
	
	//for (i = 0; i < NR_FILE_DESC; i++)
	//	if (f_desc_table[i].fd_inode == 0)
	//		break;
	//if (i >= NR_FILE_DESC)
	//	panic("f_desc_table[] is full (PID:%d)", task->pid);
	//task->filp[STDOUT] = &f_desc_table[i];
	
	//task->filp[STDOUT]->fd_mode = O_WRONLY;
	//task->filp[STDOUT]->fd_pos = 2;
	//task->filp[STDOUT]->fd_cnt = 1;
	//struct inode* STDOUT_node = (struct inode*)memman_alloc(memman, sizeof(struct inode));
	//STDOUT_node->i_mode = 1; //I_CHAR_SPECIAL;
	////debug("STDOUT_node->i_mode = %x", STDOUT_node->i_mode);
	//STDOUT_node->i_size = task->pid;
	//task->filp[STDOUT]->fd_inode = STDOUT_node;
	
	//for (i = 0; i < NR_FILE_DESC; i++)
	//	if (f_desc_table[i].fd_inode == 0)
	//		break;
	//if (i >= NR_FILE_DESC)
	//	panic("f_desc_table[] is full (PID:%d)", task->pid);
	//task->filp[STDERR] = &f_desc_table[i];
	
	//task->filp[STDERR]->fd_mode = O_WRONLY;
	//task->filp[STDERR]->fd_pos = 0;
	//task->filp[STDERR]->fd_cnt = 1;
	//struct inode* STDERR_node = (struct inode*)memman_alloc(memman, sizeof(struct inode));
	//STDERR_node->i_mode = 1; //I_CHAR_SPECIAL;
	////STDERR_node->i_size = task->pid;
	//task->filp[STDERR]->fd_inode = STDERR_node;
	
	//panic("open_std_files");
}


/*****************************************************************************/
/**
 * Get all the inodes of files
 *
******************************************************************************/
PUBLIC struct FILEINFO* get_all_files(int dev){
	//int i, j;
	
	//struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	///**
	// * Search the dir for the files.
	// */
	//int dir_blk0_nr = root_inode->i_start_sect;
	//debug("root_inode->i_start_sect = %d",root_inode->i_start_sect);
	//int nr_dir_blks = (root_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	//int nr_dir_entries = root_inode->i_size / DIR_ENTRY_SIZE; /**
	//														   * including unused slots
	//														   * (the file has been deleted
	//														   * but the slot is still there)
	//														   */
	//debug("nr_dir_blks = %d",nr_dir_blks);
	//debug("nr_dir_entries = %d",nr_dir_entries);

	
	//int m = 0;
	//struct dir_entry * pde;
	//struct FILEINFO * p_file = (struct FILEINFO * )memman_alloc(memman, (nr_dir_entries+1) * sizeof(struct FILEINFO));
	//struct FILEINFO * file_list = p_file;
	////获取文件名
	//for (i = 0; i < nr_dir_blks; i++) {
	//	debug("read sect[%d]",dir_blk0_nr + i);
	//	RD_SECT(root_inode->i_dev, dir_blk0_nr + i);
	//	pde = (struct dir_entry *)fsbuf;
	//	for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
	//		if (++m > nr_dir_entries)
	//			break;
	//		strcpy(p_file->name,pde->name);
	//		p_file->size = pde->inode_nr; //暂时用i_size来存储inode_nr
	//		p_file++;
	//		debug(pde->name);
	//		
	//	}
	//	if (m > nr_dir_entries) /* all entries have been iterated */
	//		break;
	//}
	
	////获取文件大小
	//p_file = file_list; //重置p_file到开头
	//m=0;
	//for (i = 0; i < nr_dir_blks; i++) {
	//	for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
	//		if (++m > nr_dir_entries)
	//			break;
	//		struct inode *tmp_inode = get_inode(dev,p_file->size); //这时p_file->inode_nr
	//												//get_inode需要读取扇区，会覆盖fsbuf,因此需要和上面获取文件名的循环独立开
	//		p_file->size = tmp_inode->i_size;
	//		debug("filename = [%s], filesize = [%d]",p_file->name,p_file->size);
	//		p_file++;
	//		
	//		
	//	}
	//	if (m > nr_dir_entries) /* all entries have been iterated */
	//		break;
	//}
	//p_file -> size = -1;
	//return file_list;
	return NULL;
}

