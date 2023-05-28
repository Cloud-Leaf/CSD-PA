#include "common.h"
#include "fs.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;
extern void ramdisk_read(void *buf, off_t offset, size_t len);
#define RAMDISK_SIZE ((&ramdisk_end) - (&ramdisk_start))

uintptr_t loader(_Protect *as, const char *filename) {
  //TODO();

  //ramdisk_read(DEFAULT_ENTRY,0,RAMDISK_SIZE);
  int fd=fs_open(filename,0,0);
  Log("filename=%s,fd=%d",filename,fd);
  
  fs_read(fd,DEFAULT_ENTRY,fs_filesz(fd));
  
  fs_close(fd);//关闭文件

  return (uintptr_t)DEFAULT_ENTRY;
}
