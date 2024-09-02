#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup(){
  ehdr = NULL;
  phdr = NULL;
  free(ehdr);
  free(phdr);
}

void error_exit(const char *message){
    perror(message);
    loader_cleanup();
    exit(1);
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char **exe){
  fd = open(*exe, O_RDONLY);
  // 1. Load entire binary content into the memory from the ELF file.
  off_t FDsize = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  char *heap_memory;
  heap_memory = (char *)malloc(FDsize);
  if(!heap_memory){
    error_exit("Error: Memory allocation failed");
  }
  ssize_t file_read = read(fd, heap_memory, FDsize);

  if(file_read < 0 || (size_t)file_read != FDsize){
    error_exit("Error: File read operation failed");
    free(heap_memory);
  }

  ehdr = (Elf32_Ehdr *)heap_memory;

  if(ehdr->e_type != ET_EXEC){
    error_exit("Error: Unsupported ELF file type");
  }

  phdr = (Elf32_Phdr *)(heap_memory + ehdr->e_phoff);
  unsigned int entry = ehdr->e_entry;

  Elf32_Phdr *tmp = phdr;
  int total_phdr = ehdr->e_phnum;
  void *virtual_mem;
  void *entry_addr;
  int i = 0;

  // 2. Iterate through the PHDR table and find the section of PT_LOAD
  //    type that contains the address of the entrypoint method in fib.c
 for(int i = 0; i < ehdr->e_phnum; i++){
  if(phdr[i].p_type == PT_LOAD){
    // 3. Allocate memory of the size "p_memsz" using mmap function
    //    and then copy the segment content
    void *virtual_mem = mmap(NULL, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if(virtual_mem == MAP_FAILED){
      error_exit("Error: Memory mapping failed");
      }
      memcpy(virtual_mem, heap_memory + phdr[i].p_offset, phdr[i].p_filesz);
      if(entry >= phdr[i].p_vaddr && entry < phdr[i].p_vaddr + phdr[i].p_memsz){
        // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
        entry_addr = virtual_mem + (entry - phdr[i].p_vaddr);
        break;
        }
      }
  }

  if(entry_addr != NULL){
    // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
    int (*_start)(void) = (int (*)(void))entry_addr;

    // 6. Call the "_start" method and print the value returned from the "_start"
    int result = _start();
    printf("User _start return value = %d\n", result);
  }

  else{
    printf("Entry Point Address is out of bounds.\n");
    free(heap_memory);
    exit(1);
  }
  close(fd);
}


int main(int argc, char **argv){
  if(argc != 2){
    printf("Usage: %s <ELF Executable> \n", argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  FILE *elfFile = fopen(argv[1], "rb");
  if(!elfFile){
    printf("Error: Unable to open ELF file.\n");
    exit(1);
  }
  fclose(elfFile);

  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(&argv[1]);
  // 3. invoke the cleanup routine inside the loader
  loader_cleanup();
  return 0;
}


