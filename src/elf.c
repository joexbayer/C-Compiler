#include <cc.h>
#include <stdint.h>

#define EI_NIDENT 16
#define PT_LOAD 1
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

static void elf_header(Elf32_Ehdr *ehdr, uint32_t entry, uint32_t phoff) {
    memset(ehdr, 0, sizeof(Elf32_Ehdr));
    ehdr->e_ident[0] = 0x7f;
    ehdr->e_ident[1] = 'E';
    ehdr->e_ident[2] = 'L';
    ehdr->e_ident[3] = 'F';
    ehdr->e_ident[4] = 1;
    ehdr->e_ident[5] = 1;
    ehdr->e_ident[6] = 1;
    ehdr->e_type = 2; 
    ehdr->e_machine = 3;
    ehdr->e_version = 1;
    ehdr->e_entry = entry;
    ehdr->e_phoff = phoff;
    ehdr->e_ehsize = sizeof(Elf32_Ehdr);
    ehdr->e_phentsize = sizeof(Elf32_Phdr);
    ehdr->e_phnum = 1;
}

static void program_header(Elf32_Phdr *phdr, uint32_t offset, uint32_t vaddr, uint32_t filesz) {
    memset(phdr, 0, sizeof(Elf32_Phdr));
    phdr->p_type = PT_LOAD;
    phdr->p_offset = offset;
    phdr->p_vaddr = vaddr;
    phdr->p_paddr = vaddr;
    phdr->p_filesz = filesz;
    phdr->p_memsz = filesz;
    phdr->p_flags = PF_R | PF_X | PF_W;
    phdr->p_align = 0x0;
}

/**
 * @brief Write the ELF header to the file
 * 
 * @param file The file to write to
 * @param entry The entry point of the program
 * @param text_size The size of the text section
 * @param data_size The size of the data section
 * @return int 0 on success, -1 on failure
 */
int write_elf_header(FILE* file, int entry, int text_size, int data_size) {
    Elf32_Ehdr ehdr;
    Elf32_Phdr phdr;

    int org = config.org;
    uint32_t entry_point = org + sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr);
    elf_header(&ehdr, entry_point, sizeof(Elf32_Ehdr));
    program_header(&phdr, 0, org, text_size + sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr));
    fwrite(&ehdr, sizeof(Elf32_Ehdr), 1, file);
    fwrite(&phdr, sizeof(Elf32_Phdr), 1, file);

    return 0;
}

