#include <stdint.h>
#include <stdbool.h>
#include <string.h> // memset
#include <assert.h>

#include <dos.h>
#include <dpmi.h>

#include "geometry.h"

#include <sys/nearptr.h>

#define SCR_W 320
#define SCR_H 200

__dpmi_meminfo fb_meminfo;

typedef struct{
	uint8_t* px;
	const int16_t w,h;
} VGA_PXBUF;

static VGA_PXBUF vga_fb={
	.px=NULL,
	.w=SCR_W,
	.h=SCR_H};

static int32_t* font=NULL;

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
	if(__djgpp_nearptr_enable()){
		vga_fb.px=(uint8_t*)__djgpp_conventional_base+0xA0000;
		font=get_font(0x3);
	}else{
		fb_meminfo.size=16; // pages
		assert(__dpmi_allocate_linear_memory(&fb_meminfo,0)>=0);
		printf("%lx\n",fb_meminfo.address);
		assert(__dpmi_map_device_in_memory_block(&fb_meminfo,0xA0000ul)>=0);
		printf("%lx\n",fb_meminfo.address);
		vga_fb.px=(uint8_t*)fb_meminfo.address;
	}


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

SHL void* vga_plot_getptr(void* ptr,const int x,const int y){
	uint8_t* px=(uint8_t*)ptr;
	//assert(clip_point(x,y,SCR_W,SCR_H));
	return &px[SCR_W*y+x];
}

SHL void vga_plot_unsafe(void* ptr,const int x,const int y,const uint8_t c){
	uint8_t* px=(uint8_t*)vga_plot_getptr(ptr,x,y);
	//assert(clip_point(x,y,SCR_W,SCR_H));
	*px=c;
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
	if(!font) return;
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

SHL void vga_line_o0(
	const VGA_PXBUF* pb,
	int16_t x,
	int16_t y,
	int16_t dx,
	const int16_t dy,
	const int16_t xdir,
	const uint8_t c)
{
	int dy2=dy*2;
	int dy2minusdx2=dy2-dx*2;
	int error=dy2-dx;
	vga_plot(pb->px,x,y,c);
	while(dx--){
		if(error>=0){
			++y;
			error+=dy2minusdx2;
		}else{
			error+=dy2;
		}
		x+=xdir;
		vga_plot(pb->px,x,y,c);
	}
}

SHL void vga_line_o1(
	const VGA_PXBUF* pb,
	int16_t x,
	int16_t y,
	const int16_t dx,
	int16_t dy,
	const int16_t xdir,
	const uint8_t c)
{
	int dx2=dx*2;
	int dx2minusdy2=dx2-dy*2;
	int error=dx2-dy;
	vga_plot(pb->px,x,y,c);
	while(dy--){
		if(error>=0){
			x+=xdir;
			error+=dx2minusdy2;
		}else{
			error+=dx2;
		}
		++y;
		vga_plot(pb->px,x,y,c);
	}
}

SHL void vga_line(
	const VGA_PXBUF* pb,
	int16_t x0,
	int16_t y0,
	int16_t x1,
	int16_t y1,
	const uint8_t c)
{
	if(y0>y1){
		int16_t temp=y0;
		y0=y1;
		y1=temp;
		temp=x0;
		x0=x1;
		x1=temp;
	}
	int16_t dx=x1-x0;
	int16_t dy=y1-y0;
	if(dx>0){
		if(dx>dy){
			vga_line_o0(pb,x0,y0,dx,dy,1,c);
		}else{
			vga_line_o1(pb,x0,y0,dx,dy,1,c);
		}
	}else{
		dx=-dx;
		if(dx>dy){
			vga_line_o0(pb,x0,y0,dx,dy,-1,c);
		}else{
			vga_line_o1(pb,x0,y0,dx,dy,-1,c);
		}
	}
}

SHL void vga_line_fast(
	const VGA_PXBUF* pb,
	int16_t x1,
	int16_t y1,
	int16_t x2,
	int16_t y2,
	const uint8_t c)
{
	if(y1>y2){
		int16_t tx=x1;
		int16_t ty=y1;
		x1=x2;
		y1=y2;
		x2=tx;
		y2=ty;
	}
	uint8_t* px=(uint8_t*)vga_plot_getptr(pb->px,x1,y1);
	int xadv;

	void drawh(int runlength){
		for(int i=0;i<runlength;++i){
			*px=c;
			px+=xadv;
		}
		px+=pb->w;
	}

	void drawv(int runlength){
		for(int i=0;i<runlength;++i){
			*px=c;
			px+=pb->w;
		}
		px+=xadv;
	}

	int16_t xd;
	if((xd=x2-x1)<0){
		xadv=-1;
		xd=-xd;
	}else{
		xadv=1;
	}
	int16_t yd=y2-y1;
	if(yd==0){ // horizontal line
		for(int16_t i=0;i<=xd;++i){
			*px=c;
			px+=xadv;
		}
		return;
	}
	if(xd==0){ // vertical line
		for(int16_t i=0;i<=yd;++i){
			*px=c;
			px+=pb->w;
		}
		return;
	}
	if(xd==yd){ // diagonal line
		for(int16_t i=0;i<=xd;++i){
			*px=c;
			px+=xadv+pb->w;
		}
		return;
	}
	if(xd>=yd){
		int wholestep=xd/yd;
		int adjup=(xd%yd)*2;
		int adjdown=yd*2;
		int error=(xd%yd)-(yd*2);
		int initialpixelcount=(wholestep/2)+1;
		int finalpixelcount=initialpixelcount;
		if((wholestep&0x01)==0){
			if(adjup==0) --initialpixelcount;
		}else{
			error+=yd;
		}
		drawh(initialpixelcount);
		for(int i=0;i<(yd-1);++i){
			int runlength=wholestep;
			if((error+=adjup)>0){
				++runlength;
				error-=adjdown;
			}
			drawh(runlength);
		}
		drawh(finalpixelcount);
	}else{
		int wholestep=yd/xd;
		int adjup=(yd%xd)*2;
		int adjdown=xd*2;
		int error=(yd%xd)-(xd*2);
		int initialpixelcount=(wholestep/2)+1;
		int finalpixelcount=initialpixelcount;
		if((wholestep&0x01)==0){
			if(adjup==0) --initialpixelcount;
		}else{
			error+=xd;
		}
		drawv(initialpixelcount);
		for(int i=0;i<(xd-1);++i){
			int runlength=wholestep;
			if((error+=adjup)>0){
				++runlength;
				error-=adjdown;
			}
			drawv(runlength);
		}
		drawv(finalpixelcount);
	}
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
