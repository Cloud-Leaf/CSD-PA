#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
  extern void getScreen(int* width,int* height);//外部引用声明
  int width=0,height=0;
  getScreen(&width,&height);//获取屏幕信息
  file_table[FD_FB].size=width*height*sizeof(uint32_t);
  Log("Set FD_FB size=%d",file_table[FD_FB].size);//辅助输出
}

size_t fs_filesz(int fd) {
  assert(fd>=0 && fd<NR_FILES);//不能超过给定文件个数
  return file_table[fd].size;
}

off_t disk_offset(int fd) {
  assert(fd>=0 && fd<NR_FILES);//不能超过给定文件个数
  return file_table[fd].disk_offset;
}

off_t get_open_offset(int fd) {
  assert(fd>=0 && fd<NR_FILES);//不能超过给定文件个数
  return file_table[fd].open_offset;
}

//设置open_offset，每次从指定位置读取
void set_open_offset(int fd,off_t n) {
  assert(fd>=0 && fd<NR_FILES);//不能超过给定文件个数
  assert(n>=0);
  if(n>file_table[fd].size) 
    n=file_table[fd].size;
  file_table[fd].open_offset=n;
}

extern void ramdisk_read(void *buf,off_t offset,size_t len);
extern void ramdisk_write(void *buf,off_t offset,size_t len);
extern void dispinfo_read(void *buf, off_t offset, size_t len);
extern void fb_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);

int fs_open(const char* filename,int flags,int mode) {
  for(int i=0;i<NR_FILES;i++) 
    if(strcmp(filename,file_table[i].name)==0) {
      return i;
    }
  
  panic("This file is not exist!");
  return -1;
}

ssize_t fs_read(int fd,void* buf,size_t len) {
  assert(fd>=0 && fd<NR_FILES);//不能超过给定文件个数
  
  if(fd<3||fd==FD_FB) {
    Log("arg invalid:fd<3 or fd==FD_FB");
    return 0;
  }

  if(fd==FD_EVENTS)
    return events_read(buf,len);

  int n=fs_filesz(fd)-get_open_offset(fd);//最多能读取多少字节
  if(n>len) 
    n=len;
  if(fd==FD_DISPINFO)
    dispinfo_read(buf,get_open_offset(fd),n);
  else
    ramdisk_read(buf,disk_offset(fd)+get_open_offset(fd),n);
  set_open_offset(fd,get_open_offset(fd)+n);//设置偏移量
  return n;
}

int fs_close(int fd) {
  assert(fd>=0 && fd<NR_FILES);//不能超过给定文件个数
  return 0;
} 

ssize_t fs_write(int fd,void* buf,size_t len) {
  assert(fd>=0 && fd<NR_FILES);//不能超过给定文件个数
  if(fd<3||fd==FD_DISPINFO) {
    Log("arg invalid:fd<3 or fd==FD_DISPINFO");
    return 0;
  }

  int n=fs_filesz(fd)-get_open_offset(fd);
  if(n>len) //大于文件长度则最多只能写文件长度个字节
    n=len;
  
  if(fd==FD_FB)
    fb_write(buf,get_open_offset(fd),n);
  else
    ramdisk_write(buf,disk_offset(fd)+get_open_offset(fd),n);

  set_open_offset(fd,get_open_offset(fd)+n);//设置偏移量
  return n;
}

off_t fs_lseek(int fd,off_t offset,int whence) {
  switch(whence) {
    case SEEK_SET://文件开始
      set_open_offset(fd,offset);
      return get_open_offset(fd);
    case SEEK_CUR:
      set_open_offset(fd,get_open_offset(fd)+offset);
      return get_open_offset(fd);  
    case SEEK_END://文件末尾
      set_open_offset(fd,fs_filesz(fd)+offset);
      return get_open_offset(fd);
    default:
      panic("Unhandled whence ID = %d",whence);
      return -1;
  }
}