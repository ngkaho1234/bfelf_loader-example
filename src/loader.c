#define _FILE_OFFSET_BITS 64
#define PAGE_SIZE 4096

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <libelf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <bfelf_loader.h>

char *file_content;
char *load_address;

off_t file_size;
off_t load_size;
struct bfelf_file_t elf_file;
struct bfelf_loader_t elf_loader;
struct e_string_t entry_string = {
	.buf = "entry",
	.len = 5
};

static char *alloc_exec(bfelf64_xword size)
{
	return mmap(0, (size + PAGE_SIZE) / PAGE_SIZE * PAGE_SIZE,
			    PROT_READ|PROT_WRITE|PROT_EXEC,
			    MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

static void free_exec(void *addr, bfelf64_xword size)
{
	munmap(addr, (size + PAGE_SIZE) / PAGE_SIZE * PAGE_SIZE);
}

static char *load_elf_file(struct bfelf_file_t *ef)
{
	int i;
	char *exec;
	bfelf64_xword total = 0;
	bfelf64_xword num_segments = bfelf_file_num_segments(ef);

	for (i = 0;i < num_segments;++i) {
		struct bfelf_phdr *phdr = NULL;
		bfelf_file_get_segment(ef, i, &phdr);

		if (total < phdr->p_vaddr + phdr->p_memsz)
			total = phdr->p_vaddr + phdr->p_memsz;

	}
	exec = alloc_exec(total);
	if (exec)
		memset(exec, 0, total);
	else
		goto err_out;

	for (i = 0;i < num_segments;++i) {
		void *exec_p, *file_p;
		struct bfelf_phdr *phdr = 0;

		bfelf_file_get_segment(ef, i, &phdr);

		exec_p = exec + phdr->p_vaddr;
		file_p = ef->file + phdr->p_offset;

		memcpy(exec_p, file_p, phdr->p_filesz);
	}
	load_size = total;
	return exec;

err_out:
	return NULL;
}

int main(int argc, char** argv, char** envp)
{
	int ret = 1, fd;
	struct stat stat_buf;
	void *addr = NULL;
	int (*entry)(void);
	if (argc != 2)
		return 1;

	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		return 1;
	
	fstat(fd, &stat_buf);
	if (!stat_buf.st_size) {
		ret = 1;
		goto out;
	}
	file_size = stat_buf.st_size;

	file_content = alloc_exec(file_size);
	if (!file_content)
		goto out;

	read(fd, file_content, file_size);

	bfelf_file_init(file_content, file_size, &elf_file);
	load_address = load_elf_file(&elf_file);
	if (!load_address) {
		ret = 1;
		goto out;
	}

	if (bfelf_loader_add(&elf_loader, &elf_file, load_address) != BFELF_SUCCESS) {
		ret = 1;
		goto out;
	}
	if (bfelf_loader_relocate(&elf_loader) != BFELF_SUCCESS) {
		ret = 1;
		goto out;
	}
	if (bfelf_loader_resolve_symbol(&elf_loader, &entry_string, &addr) != BFELF_SUCCESS) {
		ret = 1;
		fprintf(stderr, "Failed to resolve symbol.\n");
		goto out;
	}
	entry = addr;
	printf("Num: %d\n", entry());
out:
	if (file_content)
		free_exec(file_content, file_size);

	if (load_address)
		free_exec(load_address, load_size);

	close(fd);
	return ret;
}
