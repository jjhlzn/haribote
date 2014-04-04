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


/* ������ */
#define SHT_NULL      0   //��Ч��
#define SHT_PROGBITS  1   //����Ρ�����Ρ����ݶζ�����������
#define SHT_SYMTAB    2	  //��ʾ�öε�����Ϊ���ű�
#define SHT_STRTAB	  3   //��ʾ�öε�����Ϊ�ַ�����
#define SHT_RELA  	  4   //�ض�λ���öΰ������ض�λ��Ϣ
#define SHT_HASH	  5   //���ű�Ĺ�ϣ��
#define SHT_DYNAMIC   6	  //��̬������Ϣ
#define SHT_NOTE      7   //��ʾ����Ϣ
#define SHT_NOBITS    8   //��ʾ�ö����ļ���û�����ݣ�����.bss
#define SHT_REL       9   //�öΰ������ض�λ��Ϣ
#define SHT_SHLIB     10  //����
#define SHT_DNYSYM	  11  //��̬���ӵķ��ű�


/*  �εı�־ */
#define SHF_WRITE     1   //��ʾ�ö��ڽ��̿ռ��п�д
#define SHT_ALLOC	  2   //��ʾ�ö��ڽ��̿ռ�����Ҫ����ռ䡣��Щ����ָʾ�������Ϣ�Ķβ���Ҫ�ڽ��̿ռ��б�����ռ䣬����һ�㲻���������־�������Ρ����ݶκ�.bss�ζ����������־
#define SHT_EXECINSTR 4   //��ʾ�ö��ڽ��̿ռ���Ա�ִ�У�һ��ָ�����



#endif
