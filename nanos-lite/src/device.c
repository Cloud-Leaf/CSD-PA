#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf,dispinfo+offset,len);
}

extern void getScreen(int* width,int* height);
void fb_write(const void *buf, off_t offset, size_t len) {
  assert(offset%4==0 &&len%4==0);
  int index,screen_x1,screen_y1,screen_y2;
  int width=0,height=0;
  getScreen(&width,&height);//获取屏幕信息

  index=offset/4;//数组下标
  screen_y1=index/width;
  screen_x1=index%width;//计算坐标

  index=(offset+len)/4;
  screen_y2=index/width;

  assert(screen_y2>=screen_y1);
  //如果只有一行
  if(screen_y2==screen_y1) {
    _draw_rect(buf,screen_x1,screen_y1,len/4,1);
    return;
  }
  //如果有两行
  int tempx=width-screen_x1;
  if(screen_y2-screen_y1==1) {
    _draw_rect(buf,screen_x1,screen_y1,tempx,1);
    _draw_rect(buf+tempx*4,0,screen_y2,len/4-tempx,1);
    return;
  }

  //三行及以上调用三次_drac_rect
  _draw_rect(buf,screen_x1,screen_y1,tempx,1);
  int tempy=screen_y2-screen_y1-1;
  _draw_rect(buf+tempx*4,0,screen_y1+1,width,tempy);
  _draw_rect(buf+tempx*4+tempy*width*4,0,screen_y2,len/4-tempx-tempy*width,1);
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention

  int width=0,height=0;
  getScreen(&width,&height);//初始化获取屏幕信息
  sprintf(dispinfo,"WIDTH:%d\nHEIGHT:%d\n",width,height);
}
