#include <go32.h>
#include <bios.h>

#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "vga_dj.h"
#include "colorspace.h"
#include "fixedpoint.h"
#include "timer.h"

#include "segiconcolor.h"

uint8_t bb[SCR_W*SCR_H];

static const VGA_PXBUF pb_bb={
	.px=bb,
	.w=SCR_W,
	.h=SCR_H};

static bool key_hit(void){
	if(bioskey(1)){
		bioskey(0);
		return true;
	}
	return false;
}

SHL void showpal(void){
	for(int i=0;i<256;++i){
		int x=i&0xf;
		int y=i>>4;
		vga_plot_unsafe(bb,x,y,i);
	}
}

SHL void gray(void){
	for(int i=0;i<256;++i){
		vga_pal_set(i,i,i,i);
	}
}

SHL void rainbow(int n){
	for(int i=0;i<n;++i){
		uint16_t r,g,b;
		HSVtoRGB16(&r,&g,&b,i*(0xffff/n),0xFFFF,0xFFFF);
		vga_pal_set(256-n+i,r>>10,g>>10,b>>10);
	}
	//vga_pal_set(255,0xff,0xff,0xff);
}

SHL void rainbow16(void){
	for(int i=0;i<256-16;++i){
		int x=i&0xf;
		int y=i>>4;
		uint16_t r,g,b;
		HSVtoRGB16(&r,&g,&b,x<<12,0xFFFF-(y<<12),0xFFFF);
		vga_pal_set(i+16,r>>10,g>>10,b>>10);
	}
}

SHL void galaxy(void){
	for(int i=0;i<256-16;++i){
		int x=i&0xf;
		int y=1+(i>>4);
		vga_pal_set(i+16,y<<2,x<<2,63);
		//vga_pal_set(i+1,x<<2,y<<2,0);
		//vga_pal_set(i+1,y<<2,63,x<<2);
		//vga_pal_set(i+1,y<<2,0,x<<2);
		//vga_pal_set(i+1,63,x<<2,y<<2);
		//vga_pal_set(i+1,0,x<<2,y<<2);
	}
}

static char* my_strcpy(char* dst, const char* src){
	while((*dst++=*src++));
	return dst-1;
}

SHL char* itoa_u32(char* dst,uint32_t n){
	char tmp[11];
	char* str=tmp+10;
	*str=0;
	do{
		*--str='0'+(n%10);
		n/=10;
	}while(n>0);
	return my_strcpy(dst,str);
}

SHL char* itoa_u32_d(char* dst,uint32_t n){
	char tmp[12];
	char* str=tmp+11;
	*str=0;
	int c=0;
	do{
		*--str='0'+(n%10);
		n/=10;
		++c;
		if(c==2){
			*--str='.';
		}
	}while(n>0);
	return my_strcpy(dst,str);
}

static int fps;
static int frame;
static char buf[32];

static void screen_start(void){
	fps=0;
	frame=0;
	buf[0]=0;
	timer_reset();
}

static void frame_end(void* bb){
	if(fps>5800) vsync();
	if(bb) vga_swap(bb);
	++frame;
	fps=(UCLOCKS_PER_SEC*100)/(timer_elapsed()/frame);
	my_strcpy(itoa_u32_d(buf,fps)," fps");
}

static void dist(void){
	float x1=0;
	float y1=0;
	while(!key_hit()){
		for(int y=0;y<SCR_H;++y){
			for(int x=0;x<SCR_W;++x){
				float dist=sqrt(pow(x1-x,2)+pow(y1-y,2));
				uint8_t c=(float)0xfe*sin((100.0/SCR_W)*(float)dist);
				//int c=dist;
				vga_plot_unsafe(bb,x,y,1+c);
			}
		}
		showpal();
		vsync();
		vga_swap(bb);
		x1+=10;
		y1+=10;
	}
}
// 486DX33 52.93fps
static void derp(void){
	if((vga_fb.w!=320)&&(vga_fb.h!=200)) return;
	const int w=320;
	const int h=200;
	screen_start();
	while(!key_hit()){
		int t=frame<<6;
		//int cx=sin16(t)>>6;
		//int cy=cos16(t)>>6;
		int cx=isin_S4(t)>>4;
		int cy=isin_S4(t+(1<<13))>>4;
		for(int y=0;y<h;++y){
#if 0
			for(int x=0;x<w;++x){
				vga_plot_unsafe(fb,x,y,frame+((x+frame)^(y+frame)));
			}
#else
			int yy=cy+y;
			for(int x=0;x<w;x+=4){
				int xx=cx+x;
				uint8_t a=frame+((xx  )^yy); uint32_t l=a;
				uint8_t b=frame+((xx+1)^yy); l|=b<<8;
				uint8_t c=frame+((xx+2)^yy); l|=c<<16;
				uint8_t d=frame+((xx+3)^yy); l|=d<<24;
				*(uint32_t*)&vga_fb.px[w*y+x]=l;
			}
#endif
		}
		vga_drawstr(vga_fb.px,buf,0,h-8,7);
		//showpal();
		frame_end(NULL);
	}
}

static void shadebobs(void){
	vga_pal_set(0,0,0,0);
	screen_start();
	double t=0;
	while(!key_hit()){
		for(int i=0;i<16;++i){
			float w=(SCR_W-13.0)/2.0;
			float h=(SCR_H-13.0)/2.0;
			int x=w+sin(2.5*t)*w;
			int y=h+sin(3.1*t)*h;
			for(int y2=0;y2<14;++y2){
				for(int x2=0;x2<14;++x2){
					int c=vga_getpx_unsafe(&pb_bb,x+x2,y+y2);
					c+=1;
					if(c>255) c-=255;
					vga_plot_unsafe(bb,x+x2,y+y2,c);
				}
			}
			t+=0.01;
		}
		if(timer_elapsed()<UCLOCKS_PER_SEC/30) continue;
		//vga_rect_xy(bb,0,SCR_H-8,8*11,SCR_H,0);
		//showpal();
		//if(fps>5800) vsync();
		vga_swap(bb);
		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,0);
		vga_drawstr(vga_fb.px,buf,2,SCR_H-8,0);
		vga_drawstr(vga_fb.px,buf,1,SCR_H-8,128);
		++frame;
		fps=(UCLOCKS_PER_SEC*100)/(timer_elapsed()/frame);
		my_strcpy(itoa_u32_d(buf,fps)," fps");
		timer_reset();
		frame=0;
	}
}

// 486DX33 25.88 fps
static void roto(void){
	uint8_t tex_fetch(fp_t u,fp_t v){
		return fp_to_int16(u)^fp_to_int16(v);
	}
	rainbow(256);
	screen_start();
	while(!key_hit()){
		float t=timer_elapsed_fsec();
		double sin_t;
		double cos_t;
		sincos(t*0.375,&sin_t,&cos_t);
		float scale=t*t*0.0375;
		fp_t fsin_t=float_to_fp(sin_t*scale);
		fp_t fcos_t=float_to_fp(cos_t*scale);
		fp_t xo=float_to_fp(-SCR_W/2+sin_t*SCR_W/2);
		fp_t yo=float_to_fp(-SCR_H/2+cos_t*SCR_H/2);
		for(int py=0;py<SCR_H;++py){
			fp_t y=fp_mul(float_to_fp(1.2),int_to_fp(py)+yo);
			fp_t u=fp_mul(fcos_t,xo)-fp_mul(fsin_t,y);
			fp_t v=fp_mul(fsin_t,xo)+fp_mul(fcos_t,y);
#if 0
			for(int x=0;x<SCR_W;++x){
				vga_plot_unsafe(vga_fb.px,x,y,fp_to_int16(u)^fp_to_int16(v));
				u+=fcos_t;
				v+=fsin_t;
			}
#else
			for(int px=0;px<SCR_W;px+=4){
				uint8_t a=tex_fetch(u,v);
				u+=fcos_t;
				v+=fsin_t;
				uint8_t b=tex_fetch(u,v);
				u+=fcos_t;
				v+=fsin_t;
				uint8_t c=tex_fetch(u,v);
				u+=fcos_t;
				v+=fsin_t;
				uint8_t d=tex_fetch(u,v);
				u+=fcos_t;
				v+=fsin_t;
				uint32_t l=(a)|(b<<8)|(c<<16)|(d<<24);
				*(uint32_t*)&vga_fb.px[SCR_W*py+px]=l;
			}
#endif
		}
		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,128);
		//showpal();
		frame_end(NULL);
	}
}

static void load_pal_seg(void){
	for(int i=0;i<6;++i){
		uint8_t r=seg_pal[i][0]>>2;
		uint8_t g=seg_pal[i][1]>>2;
		uint8_t b=seg_pal[i][2]>>2;
		vga_pal_set(i,r,g,b);
	}
	vga_pal_set(0,0,0,0);
	//vga_pal_set(1,-1,-1,-1);
}

// 486DX33 15.09fps
static void roto_pb(void){
	uint8_t tex_fetch(fp_t u,fp_t v){
		uint8_t c=vga_getpx_unsafe(&seg_pb,fp_to_int(u>>1)&0x1f,fp_to_int(v>>1)&0x1f);
		if(!c) c=((fp_to_int16(u>>1)^fp_to_int16(v>>1))&0x7F)+128;
		return c;
	}
	gray();
	rainbow(128);
	load_pal_seg();
	screen_start();
	while(!key_hit()){
		float t=timer_elapsed_fsec();
		double sin_t;
		double cos_t;
		sincos(t*0.25,&sin_t,&cos_t);
		float scale=-t*0.025;
		fp_t fsin_t=float_to_fp(sin_t*scale);
		fp_t fcos_t=float_to_fp(cos_t*scale);
		fp_t xo=float_to_fp(-SCR_W/2+sin_t*SCR_W/2);
		fp_t yo=float_to_fp(-SCR_H/2+cos_t*SCR_H/2);
		fp_t uo=int_to_fp(32)-fp_mul(fcos_t,xo);
		fp_t vo=int_to_fp(32)-fp_mul(fsin_t,xo);
		for(int py=0;py<SCR_H;++py){
			fp_t y=fp_mul(float_to_fp(1.2),yo+int_to_fp(py));
			fp_t u=uo-fp_mul(fsin_t,y);
			fp_t v=vo+fp_mul(fcos_t,y);
#if 0
			for(int x=0;x<SCR_W;++x){
				uint8_t c=tex_fetch(u,v);
				vga_plot_unsafe(vga_fb.px,x,y,c);
				u+=fcos_t;
				v+=fsin_t;
			}
#else
			for(int px=0;px<SCR_W;px+=4){
				uint8_t a=tex_fetch(u,v);
				u+=fcos_t;
				v+=fsin_t;
				uint32_t l=a;
				uint8_t b=tex_fetch(u,v);
				u+=fcos_t;
				v+=fsin_t;
				l|=b<<8;
				uint8_t c=tex_fetch(u,v);
				u+=fcos_t;
				v+=fsin_t;
				l|=c<<16;
				uint8_t d=tex_fetch(u,v);
				u+=fcos_t;
				v+=fsin_t;
				l|=d<<24;
				*(uint32_t*)&vga_fb.px[SCR_W*py+px]=l;
			}
#endif
		}
		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,128);
		//showpal();
		frame_end(NULL);
	}
}

// 486DX33 32.72fps
static void tunnel(int scale){
	uint8_t tun[2][SCR_W][SCR_H];
	for(int y=0;y<SCR_H;++y){
		for(int x=0;x<SCR_W;++x){
			double x2=x-SCR_W/2;
			double y2=y-SCR_H/2;
			y2*=1.2; // aspect ratio correction
			fp_t u=float_to_fp((128.0/M_PI)*(atan2(y2,x2)+M_PI));
			fp_t v=float_to_fp(4096.0/sqrt(x2*x2+y2*y2));
			tun[0][x][y]=fp_to_int(u<<scale);
			tun[1][x][y]=fp_to_int(v<<scale);
		}
	}

	rainbow(256);

	screen_start();
	while(!key_hit()){
		int tt=timer_elapsed()/(UCLOCKS_PER_SEC/70);
		//float t=frame/256.0;
		//int8_t cx=cos(t)*256.0;
		int cx=tt;
		int cy=tt;
		for(int y=0;y<SCR_H;++y){
#if 0
			for(int x=0;x<SCR_W;++x){
				int8_t u=tun[0][x][y]+cx;
				int8_t v=tun[1][x][y]+cy;
				vga_plot_unsafe(vga_fb.px,x,y,u^v);
			}
#else
			for(int x=0;x<SCR_W;x+=4){
				uint8_t u,v;
				uint8_t a,b,c,d;
				u=tun[0][x][y];
				v=tun[1][x][y];
				a=(cx+u)^(v+cy);
				u=tun[0][x+1][y];
				v=tun[1][x+1][y];
				b=(cx+u)^(v+cy);
				u=tun[0][x+2][y];
				v=tun[1][x+2][y];
				c=(cx+u)^(v+cy);
				u=tun[0][x+3][y];
				v=tun[1][x+3][y];
				d=(cx+u)^(v+cy);
				uint32_t l=(a)|(b<<8)|(c<<16)|(d<<24);
				*(uint32_t*)&vga_fb.px[SCR_W*y+x]=l;
			}
#endif
		}
		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,7);
		//showpal();
		frame_end(NULL);
	}
}

// 486DX33 37.43fps
static void plane(void){
	uint8_t tex_fetch(fp_t u,int v){
		return fp_to_int16(u)^v;
	}
	rainbow(256);
	//vga_pal_set(0,0,0,0);
	//vga_pal_set(1,-1,-1,-1);
	screen_start();
	while(!key_hit()){
		int tt=timer_elapsed()/(UCLOCKS_PER_SEC/36);
		fp_t t=fp_div(int_to_fp(tt),int_to_fp(2));
		fp_t cx=float_to_fp(cos(tt/128.0)*96.0);
		fp_t cy=t+t;
		for(int y=0;y<SCR_H;++y){
			int y1=y-SCR_H/2;
			fp_t z=int_to_fp(abs(y1)+8);
			z=fp_div(int_to_fp(1),fp_mul(float_to_fp(0.05),z));
			int v=fp_to_int(cy+fp_mul(int_to_fp(SCR_H),z));
			fp_t u=cx-fp_mul(int_to_fp(SCR_W/2),z);
#if 0
			for(int x=0;x<SCR_W;++x){
				uint8_t c=tex_fetch(u,v); u+=fz;
				vga_plot_unsafe(fb,x,y,c);
			}
#else
			for(int x=0;x<SCR_W;x+=4){
				uint8_t a=tex_fetch(u,v); u+=z;
				uint8_t b=tex_fetch(u,v); u+=z;
				uint8_t c=tex_fetch(u,v); u+=z;
				uint8_t d=tex_fetch(u,v); u+=z;
				uint32_t l=(a)|(b<<8)|(c<<16)|(d<<24);
				*(uint32_t*)&vga_fb.px[SCR_W*y+x]=l;
			}
#endif
		}
		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,7);
		//showpal();
		frame_end(NULL);
	}
}

// 486DX33 20.27fps
static void plane_pb(void){
	uint8_t tex_fetch(fp_t u,int v){
		uint8_t c=vga_getpx_unsafe(&seg_pb,fp_to_int(u<<1)&0x1f,(v)&0x1f);
		if(!c) c=((fp_to_int16(u)^v)&0x7F)+128;
		return c;
	}
	gray();
	rainbow(128);
	load_pal_seg();
	screen_start();
	while(!key_hit()){
		int tt=timer_elapsed()/(UCLOCKS_PER_SEC/36);
		fp_t t=fp_div(int_to_fp(tt),int_to_fp(2));
		fp_t cx=float_to_fp(cos(tt/128.0)*96.0);
		fp_t cy=-(t+t);
		for(int y=0;y<SCR_H;++y){
			int y1=y-SCR_H/2;
			fp_t z=int_to_fp(abs(y1)+8);
			z=fp_div(int_to_fp(1),fp_mul(float_to_fp(0.05),z));
			int v=fp_to_int(cy-fp_mul(int_to_fp(SCR_H),z));
			fp_t u=cx-fp_mul(int_to_fp(SCR_W/2),z);
#if 0
			for(int x=0;x<SCR_W;++x){
				uint8_t c=tex_fetch(u,v); u+=fz;
				vga_plot_unsafe(fb,x,y,c);
			}
#else
			for(int x=0;x<SCR_W;x+=4){
				uint8_t a=tex_fetch(u,v); u+=z;
				uint8_t b=tex_fetch(u,v); u+=z;
				uint8_t c=tex_fetch(u,v); u+=z;
				uint8_t d=tex_fetch(u,v); u+=z;
				uint32_t l=(a)|(b<<8)|(c<<16)|(d<<24);
				*(uint32_t*)&vga_fb.px[SCR_W*y+x]=l;
			}
#endif
		}
		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,7);
		//showpal();
		frame_end(NULL);
	}
}

static void VectorsUp(
	DRAW_CONTEXT* dc,
	int16_t x,
	int16_t y,
	int16_t w,
	int16_t h)
{
	int wx=x-w;
	int wy=y-h;
	for(;wx<(x+w);++wx){
		vga_line_fast(dc,x,y,wx,wy);++dc->c;}
	wx=x+w-1;
	wy=y-h;
	for(;wy<(y+h);++wy){
		vga_line_fast(dc,x,y,wx,wy);++dc->c;}
	wx=x+w-1;
	wy=y+h-1;
	for(;wx>=(x-w);--wx){
		vga_line_fast(dc,x,y,wx,wy);++dc->c;}
	wx=x-w;
	wy=y+h-1;
	for(;wy>=(y-h);--wy){
		vga_line_fast(dc,x,y,wx,wy);++dc->c;}
}

static void lines(void){
	DRAW_CONTEXT dc={
		.pxb=&vga_fb
	};

	//vga_pal_set(0,0,0,0);
	//vga_pal_set(1,-1,-1,-1);
	screen_start();
	while(!key_hit()){
		//vga_cls(&vga_fb,0);
		//VRECT r;
		//r.x1=0;
		//r.y1=0;
		//r.x2=SCR_W-1;
		//r.y2=SCR_H-1;
		//vga_line(&vga_fb,&r,1);

		dc.c=frame;
		VectorsUp(&dc,SCR_W/2,SCR_H/2,SCR_W/2,SCR_H/2);

		//VectorsUp(SCR_W/4  ,SCR_H/4,  SCR_W/4,SCR_H/4,frame);
		//VectorsUp(SCR_W*3/4,SCR_H/4,  SCR_W/4,SCR_H/4,frame);
		//VectorsUp(SCR_W/4,  SCR_H*3/4,SCR_W/4,SCR_H/4,frame);
		//VectorsUp(SCR_W*3/4,SCR_H*3/4,SCR_W/4,SCR_H/4,frame);

		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,128);
		//showpal();
		frame_end(NULL);
	}
}

static float osc_sin(float freq){
	return sin(M_PI_2*freq);
}

static void plasma1(void){
	rainbow(256);
	screen_start();
	while(!key_hit()){
		float t=timer_elapsed_fsec();
		double sin_t;
		double cos_t;
		sincos(M_PI_2*t*0.5,&sin_t,&cos_t);
		fp_t dx=float_to_fp(sin_t*t*0.1);
		fp_t dy=float_to_fp(cos_t*t*0.1);
		fp_t y=-fp_mul(dy,int_to_fp(vga_fb.h))>>1;
		for(int py=0;py<vga_fb.h;++py){
			fp_t x=float_to_fp(-0.5*fp_to_float(dx)*vga_fb.w);
			float tt=t*t;
			float freq=osc_sin(t*0.0666);
			float amp=1.0/(1.0+freq*0.5);
			freq+=0.0001;
			freq*=0.25;
			amp*=24.0;
			amp*=fp_to_float(dx);
			//freq*=fp_to_float(dy);
			x+=float_to_fp(amp*osc_sin(t+(((float)py-(vga_fb.h*0.5))*freq)));
			fp_t yy=sin8(fp_to_int(y))+frame*2;
#if 0
			for(int px=0;px<vga_fb.w;++px){
				int16_t c=sin8(fp_to_int(x))+yy;
				//int16_t c=fp_to_int(x)^fp_to_int(y);
				x+=dx;
				vga_plot_unsafe(vga_fb.px,px,py,c);
			}
#else
			for(int px=0;px<vga_fb.w;px+=4){
				uint8_t a=sin8(fp_to_int(x))+yy;
				uint32_t l=a;
				x+=dx;
				uint8_t b=sin8(fp_to_int(x))+yy;
				l|=b<<8;
				x+=dx;
				uint8_t c=sin8(fp_to_int(x))+yy;
				l|=c<<16;
				x+=dx;
				uint8_t d=sin8(fp_to_int(x))+yy;
				l|=d<<24;
				x+=dx;
				*(uint32_t*)&vga_fb.px[SCR_W*py+px]=l;
			}
#endif
			y+=dy;
		}
		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,128);
		frame_end(NULL);
	}
}

static void plasma(void){
	rainbow(256);
	screen_start();
	fp_t t=0;
	while(!key_hit()){
		fp_t xo=fp_mul(sinfp(t<<8),float_to_fp(300));
		fp_t yo=fp_mul(cosfp(t<<8),float_to_fp(300));
		//xo=0;
		//yo=0;
		for(int y=0;y<vga_fb.h;++y){
			fp_t dx=t<<8;
			dx=fp_mul(sinfp(t<<8),float_to_fp(2));
			fp_t dy=t<<8;
			dy=fp_mul(sinfp(t<<8),float_to_fp(2));
			fp_t xx=xo+(t<<9);
			fp_t yy=yo+fp_mul(int_to_fp(y),dy);
			//xx+=fp_mul(sinfp(int_to_fp(y+t)>>3),float_to_fp(200));
			int16_t yyy=sin8(fp_to_int(yy));
#if 0
			for(int x=0;x<vga_fb.w;++x){
				//int16_t c=sin8(fp_to_int(xx))+yyy+frame;
				int16_t c=(sin16(fp_to_int(xx<<8))+sin16(fp_to_int(yy<<8)))>>3;
				xx+=dx;
				vga_plot_unsafe(vga_fb.px,x,y,c);
			}
#else
			for(int x=0;x<vga_fb.w;x+=4){
				uint8_t a=sin8(fp_to_int(xx))+yyy+frame;
				uint32_t l=a;
				xx+=dx;
				uint8_t b=sin8(fp_to_int(xx))+yyy+frame;
				l|=b<<8;
				xx+=dx;
				uint8_t c=sin8(fp_to_int(xx))+yyy+frame;
				l|=c<<16;
				xx+=dx;
				uint8_t d=sin8(fp_to_int(xx))+yyy+frame;
				l|=d<<24;
				xx+=dx;
				*(uint32_t*)&vga_fb.px[SCR_W*y+x]=l;
			}
#endif
		}
		vga_drawstr(vga_fb.px,buf,0,SCR_H-8,128);
		//showpal();
		frame_end(NULL);
		t+=1;
	}
}

static void do_demo(void){
	rainbow(256);
	tunnel(0);
	tunnel(1);
	tunnel(2);
	tunnel(3);
	return;
	//gray();
	//galaxy();

	lines();

	derp();
	//plasma();
	plasma1(); //return;

	shadebobs();
	roto();
	roto_pb();
	plane();
	plane_pb();
	//dist();
}

int main(int argc, char **argv){
	sintbl8_init();
	//screen_start();
	sintbl16_init();
	//printf("%.2f sec\n",timer_elapsed_fsec());
	//return;
	if(!vga_dj_init()) return -1;
	set_mode(0x13);
	//uint8_t pal_save[256*3];
	//vga_pal_save(pal_save);
	do_demo();

	set_mode(0x3);
	return 0;
}
