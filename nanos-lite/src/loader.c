#include "common.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;
extern void ramdisk_read(void *buf, off_t offset, size_t len);
#define RAMDISK_SIZE ((&ramdisk_end) - (&ramdisk_start))

uintptr_t loader(_Protect *as, const char *filename) {
  //TODO();

  ramdisk_read(DEFAULT_ENTRY,0,RAMDISK_SIZE);

  return (uintptr_t)DEFAULT_ENTRY;
}
