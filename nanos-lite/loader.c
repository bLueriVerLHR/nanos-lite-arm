#include <elf.h>
#include "fs.h"

#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t get_ramdisk_size();

static uintptr_t loader(const char *filename) {
  int fd = fs_open(filename, O_RDWR, 0);
  Elf_Ehdr ehdr[1];
  fs_lseek(fd, 0, SEEK_SET);
  fs_read(fd, ehdr, sizeof(Elf_Ehdr));
  Log("Ident: %d", *(uint32_t *)ehdr->e_ident);
  assert(*(uint32_t *)ehdr->e_ident == 0x464c457f); // " ELF"
  assert(ehdr->e_machine == EM_ARM);
  Log("ramdisk size: %lx", get_ramdisk_size());
  Elf_Phdr phdr[1];
  for (int i = 0; i < ehdr->e_phnum; ++i) {
    fs_lseek(fd, ehdr->e_phoff + sizeof(Elf_Phdr) * i, SEEK_SET);
    fs_read(fd, phdr, sizeof(Elf_Phdr));
    // Log("Phdr Type:    %lx", phdr->p_type);
    // Log("Phdr Offset:  %lx", phdr->p_offset);

    if (phdr->p_type != PT_LOAD) {
      continue;
    }
    
    char buf[4096] = {0};
    char zbuf[1024] = {0};
    uint32_t filesz = phdr->p_filesz;
    uint32_t memsz = phdr->p_memsz;
    uint32_t alrdy = 0;
    // Log("filesz: %lx", filesz);
    // Log("memsz:  %lx", memsz);

    fs_lseek(fd, phdr->p_offset, SEEK_SET);
    while (filesz > 4096) {
      fs_read(fd, buf, 4096);
      memcpy((void *)(phdr->p_vaddr + alrdy), buf, 4096);
      filesz -= 4096;
      alrdy += 4096;
    }

    if (filesz) {
      fs_read(fd, buf, filesz);
      memcpy((void *)(phdr->p_vaddr + alrdy), buf, filesz);
      alrdy += filesz;
    }

    // Log("alrdy:  %lx", alrdy);
    // Log("nstrt:  %lx", phdr->p_vaddr + alrdy);
    memsz -= alrdy;
    while (memsz > 1024) {
      memcpy((void *)(phdr->p_vaddr + alrdy), zbuf, 1024);
      memsz -= 1024;
      alrdy += 1024;
    }
    if (memsz) {
      memcpy((void *)(phdr->p_vaddr + alrdy), zbuf, memsz);    
      alrdy += memsz;
    }
  }
  return ehdr->e_entry;
}

void naive_uload(const char *filename) {
  Log("Loading %s", filename);
  uintptr_t entry = loader(filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

