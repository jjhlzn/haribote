#ifndef __KERNEL_H__
#define __KERNEL_H__
/* 内核和应用进行传递的数据结构 */

struct MOUSE_INFO {
	int x, y, btn;
	int flag; //标志这个鼠标信息的数据是否有用，0表示有用，-1表示无用
};

#endif
