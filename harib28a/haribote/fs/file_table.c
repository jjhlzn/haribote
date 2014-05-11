/*
 *  linux/fs/file_table.c
 *
 *  (C) 1991  Linus Torvalds
 */
#include "bootpack.h"
#include "sys/types.h"
#include "fs.h"

struct file file_table[NR_FILE];
