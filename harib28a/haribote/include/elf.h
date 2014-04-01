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

#endif
