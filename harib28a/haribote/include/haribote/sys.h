//extern int sys_setup();
extern int sys_exit(); 
////extern int sys_fork(); 已实现
extern int sys_read();
extern int sys_write();
extern int sys_open();
extern int sys_close();
//extern int sys_waitpid(); 已实现
extern int sys_creat();
extern int sys_link();
extern int sys_unlink();
//extern int sys_execve();
extern int sys_chdir();
//extern int sys_time();
//extern int sys_mknod();
extern int sys_chmod();
//extern int sys_chown();
//extern int sys_break();
//extern int sys_stat();
extern int sys_lseek();
extern int sys_getpid();
//extern int sys_mount();
//extern int sys_umount();
//extern int sys_setuid();
//extern int sys_getuid();
//extern int sys_stime();
//extern int sys_ptrace();
//extern int sys_alarm();
//extern int sys_fstat();
//extern int sys_pause();
//extern int sys_utime();
//extern int sys_stty();
//extern int sys_gtty();
//extern int sys_access();
//extern int sys_nice();
//extern int sys_ftime();
//extern int sys_sync();
//extern int sys_kill();
//extern int sys_rename();
//extern int sys_mkdir();
//extern int sys_rmdir();
//extern int sys_dup();
//extern int sys_pipe();
//extern int sys_times();
//extern int sys_prof();
//extern int sys_brk();
//extern int sys_setgid();
//extern int sys_getgid();
//extern int sys_signal();
//extern int sys_geteuid();
//extern int sys_getegid();
//extern int sys_acct();
//extern int sys_phys();
//extern int sys_lock();
//extern int sys_ioctl();
//extern int sys_fcntl();
//extern int sys_mpx();
//extern int sys_setpgid();
//extern int sys_ulimit();
//extern int sys_uname();
//extern int sys_umask();
//extern int sys_chroot();
//extern int sys_ustat();
//extern int sys_dup2();
//extern int sys_getppid();
//extern int sys_getpgrp();
//extern int sys_setsid();
//extern int sys_sigaction();
//extern int sys_sgetmask();
//extern int sys_ssetmask();
//extern int sys_setreuid();
//extern int sys_setregid();

fn_ptr sys_call_table[] = { 
/*  0  */	0, 
/*  1  */	0, 
/*  2  */	0, 
/*  3  */	sys_read,
/*  4  */   sys_write, 
/*  5  */	sys_open, 
/*  6  */	sys_close,
/*  7  */	0,
/*  8  */	sys_creat,
/*  9  */	sys_link,
/* 10  */	sys_unlink,
/* 11  */	0,
/* 12  */	sys_chdir,
/* 13  */	0,
/* 14  */	0,
/* 15  */	sys_chmod,
/* 16  */	0,
/* 17  */	0,
/* 18  */	0,
/* 19  */	sys_lseek,
/* 20  */	sys_getpid,
/* 21  */	0
};
