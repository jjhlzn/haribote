extern int sys_read();
extern int sys_write();
extern int sys_open();
extern int sys_close();


fn_ptr sys_call_table[] = { 
	0, 
	0, 
	0, 
	sys_read,
    sys_write, 
	sys_open, 
	sys_close
};
