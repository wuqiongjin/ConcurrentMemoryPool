#include <sys/mman.h>
#include <stdio.h>
#include <memory.h>
 
int main(int argc, char *argv[]) {
  char *m;
  size_t s = (8UL * 1024 * 1024);
 
  m = (char*)mmap(NULL, s, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | 0x40000 /*MAP_HUGETLB*/, -1, 0);
  if (m == MAP_FAILED) {
    perror("map mem");
    m = NULL;
    return 1;
  }
 
  memset(m, 0, s);
 
  printf("map_hugetlb ok, press ENTER to quit!\n");
 
  munmap(m, s);
  return 0;
}

