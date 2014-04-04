#ifndef _ELF_H
#define _ELF_H

#define PT_LOAD 1

#define EI_NIDENT 16

#define Elf32_Addr int*
#define Elf32_Half unsigned short
#define Elf32_Off  unsigned int
#define Elf32_Sword int
#define Elf32_Word unsigned int

typedef struct{
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half  e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr  e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
}Elf32_Phdr;

typedef struct {
	Elf32_Word	sh_name;		/* Section name (string tbl index) */
	Elf32_Word	sh_type;		/* Section type */
	Elf32_Word	sh_flags;		/* Section flags */
	Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
	Elf32_Off	sh_offset;		/* Section file offset */
	Elf32_Word	sh_size;		/* Section size in bytes */
	Elf32_Word	sh_link;		/* Link to another section */
	Elf32_Word	sh_info;		/* Additional section information */
	Elf32_Word	sh_addralign;		/* Section alignment */
	Elf32_Word	sh_entsize;		/* Entry size if section holds table */
} Elf32_Shdr;


/* 段类型 */
#define SHT_NULL      0   //无效段
#define SHT_PROGBITS  1   //程序段。代码段、数据段都是这种类型
#define SHT_SYMTAB    2	  //表示该段的内容为符号表
#define SHT_STRTAB	  3   //表示该段的内容为字符串表
#define SHT_RELA  	  4   //重定位表。该段包含了重定位信息
#define SHT_HASH	  5   //符号表的哈希表
#define SHT_DYNAMIC   6	  //动态链接信息
#define SHT_NOTE      7   //提示性信息
#define SHT_NOBITS    8   //表示该段在文件中没有内容，比如.bss
#define SHT_REL       9   //该段包含了重定位信息
#define SHT_SHLIB     10  //保留
#define SHT_DNYSYM	  11  //动态链接的符号表


/*  段的标志 */
#define SHF_WRITE     1   //表示该段在进程空间中可写
#define SHT_ALLOC	  2   //表示该段在进程空间中需要分配空间。有些包含指示或控制信息的段不需要在进程空间中被分配空间，他们一般不会有这个标志。像代码段、数据段和.bss段都会有这个标志
#define SHT_EXECINSTR 4   //表示该段在进程空间可以被执行，一般指代码段



#endif
