#include <stdint.h>
#include <stdbool.h>
#include <string.h> // memset

#include <dos.h>
#include <dpmi.h>

#include "geometry.h"

#include <sys/nearptr.h>

#define SCR_W 320
#define SCR_H 200

//__dpmi_meminfo fb_meminfo;

typedef struct{
	uint8_t* px;
	const int16_t w,h;
} VGA_PXBUF;

static VGA_PXBUF vga_fb={
	.px=NULL,
	.w=SCR_W,
	.h=SCR_H};

static int32_t* font;

SHL void* get_font(uint8_t which){
  __dpmi_regs r;
  r.h.bh=which;
  r.x.ax=0x1130;
  __dpmi_int(0x10,&r);
  return (void*)(__djgpp_conventional_base+((size_t)r.x.es<<4)+r.x.bp);
  //uint16_t height=r.x.cx;
  //uint8_t rows=r.h.dl;
}

SHL bool vga_dj_init(void){
	if(!__djgpp_nearptr_enable()) return false;
	vga_fb.px=(uint8_t*)__djgpp_conventional_base+0xA0000;
	//fb_meminfo.size=16; // pages
	//assert(__dpmi_allocate_linear_memory(&fb_meminfo,0)>=0);
	//printf("%lx\n",fb_meminfo.address);
	//assert(__dpmi_map_device_in_memory_block(&fb_meminfo,0xA0000ul)>=0);
	//printf("%lx\n",fb_meminfo.address);

	font=get_font(0x3);

	return true;
}

SHL void set_mode(int mode){
  __dpmi_regs r;
  r.x.ax=mode;
  __dpmi_int(0x10,&r);
}

SHL void vsync(void){
	while(inp(0x3da)&0x8){__dpmi_yield();};
	while(!(inp(0x3da)&0x8)){};
}

SHL void vga_pal_set(uint8_t c,uint8_t r,uint8_t g,uint8_t b){
	outportb(0x3c8,c);
	outportb(0x3c9,r);
	outportb(0x3c9,g);
	outportb(0x3c9,b);
}

SHL void vga_pal_save(uint8_t* pal){
	outportb(0x3c7,0);
	for(int i=0;i<256*3;++i){
		pal[i]=inportb(0x3c9); 
	}
}

SHL void vga_pal_restore(uint8_t* pal){
	outportb(0x3c8,0);
	for(int i=0;i<256*3;++i){
		pal[i]=inportb(0x3c9);
	}
}

SHL void vga_cls(VGA_PXBUF* pb,uint16_t c){
		memset(pb->px,c,pb->w*pb->h);
}

SHL void vga_swap(void* bb){
	memcpy(vga_fb.px,bb,vga_fb.w*vga_fb.h);
}

SHL void vga_plot_unsafe(void* ptr,const int x,const int y,const uint8_t c){
	uint8_t* px=(uint8_t*)ptr;
	//assert(clip_point(x,y,SCR_W,SCR_H));
	px[SCR_W*y+x]=c;
}

SHL void vga_plot(void* ptr,const int x,const int y,const uint8_t c){
	if(!clip_point(x,y,SCR_W,SCR_H)) return;
	vga_plot_unsafe(ptr,x,y,c);
}

SHL uint8_t vga_getpx_unsafe(const VGA_PXBUF* pb,const int16_t x,const int16_t y){
	return pb->px[pb->w*y+x];
}

SHL uint8_t vga_getpx(const VGA_PXBUF* pb,const int16_t x,const int16_t y){
	if(!clip_point(x,y,pb->w,pb->h)) return 0;
	return vga_getpx_unsafe(pb,x,y);
}

SHL void vga_drawchar(void* ptr,int16_t x,int16_t y,uint8_t color,char c){
	int32_t* fp=(int32_t*)&font[2*c];
	int32_t bitmap=fp[0];
	y+=3;
	int i=0;
	while(bitmap!=0){
		if(bitmap<0){
			vga_plot(ptr,x+(i&0x7),y-(i>>3),color);
		}
		bitmap<<=1;
		++i;
	}
	bitmap=fp[1];
	i=0;
	while(bitmap!=0){
		if(bitmap<0){
			vga_plot(ptr,x+(i&0x7),4+y-(i>>3),color);
		}
		bitmap<<=1;
		++i;
	}
}

SHL void vga_drawstr(void* ptr,const char* str,const int16_t x,const int16_t y,const uint8_t color){
	int16_t xx=x;
	while(*str){
		vga_drawchar(ptr,xx,y,color,*str++);
		xx+=8;
	}
}

SHL void vga_line(VGA_PXBUF* pb,const VRECT* r,const uint8_t c){
	//uint8_t* px=(uint8_t*)ptr;
	vga_plot(pb->px,r->x1,r->y1,c);
	vga_plot(pb->px,r->x2,r->y2,c);
}

SHL void vga_rect_vr(void* ptr,const VRECT* vr,const int c){
	uint8_t* px=(uint8_t*)ptr;
	VRECT r;
	r.x1=min(vr->x1,vr->x2);
	r.x2=max(vr->x1,vr->x2);
	r.y1=min(vr->y1,vr->y2);
	r.y2=max(vr->y1,vr->y2);
	r.x1=max(r.x1,0);
	r.y1=max(r.y1,0);
	r.x2=min(r.x2,SCR_W);
	r.y2=min(r.y2,SCR_H);
	int w=max(0,r.x2-r.x1);
	for(int y=r.y1; y<r.y2;++y)
		memset(&px[SCR_W*y+r.x1],c,w);
}

SHL void vga_rect_xy(void* ptr,int x1,int y1,int x2,int y2,int color){
	VRECT r={.x1=x1,.y1=y1,.x2=x2,.y2=y2};
	vga_rect_vr(ptr,&r,color);
}

SHL void vga_rect_gr(void* ptr,const GRECT* gr,int c){
	GRECT scr={
		.x=0,
		.y=0,
		.w=SCR_W,
		.h=SCR_H};
	GRECT r;
	gr_copy(gr,&r);
	if(!gr_intersect(&scr,&r)) return;
	uint8_t* px=(uint8_t*)ptr;
	for(int y=r.y; y<r.y+r.h;++y){
		memset(&px[SCR_W*y+r.x],c,r.w);
	}
}

SHL void vga_rect_xywh(void* ptr,int x,int y,int w,int h,int color){
	GRECT r={.x=x,.y=y,.w=w,.h=h};
	vga_rect_gr(ptr,&r,color);
}
