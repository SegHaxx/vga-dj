// Bubble Galaxy display hack nicked from twitter user yuruyurau
// https://twitter.com/yuruyurau/status/1226846058728177665
// https://twitter.com/yuruyurau/status/1288435907045945344

#include <go32.h>

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "vga_dj.h"
#include "colorspace.h"
#include "timer.h"

uint8_t bb[SCR_W*SCR_H];
static VGA_PXBUF vga_bb={
	.px=bb,
	.w=SCR_W,
	.h=SCR_H};

SHL void showpal(void){
	for(int i=0;i<256;++i){
		int x=i&0xf;
		int y=i>>4;
		vga_plot(bb,x,y,i);
	}
}

SHL void slack(void){
	srandom(rawclock());
	const float r=M_PI*2/235;
	float x=0;
	float v=0;
	float t=random()/((float)INT_MAX/50);
	float u;
	float sz=4;
	float sw=-1.0+SCR_W/sz;
	float sh=-1.0+SCR_H/sz;
	int in=0;
	int jn=0;
	int frame=0;
	char buf[32]="";
	timer_reset();
	uclock_t uticks;
	while(!_go32_was_ctrl_break_hit()){
		vga_cls(&vga_bb,0);
		for(int i=0;i<in;i+=1){
			for(int j=0;j<jn;j+=1){
				float iv=i+v;
				float rix=r*i+x;
				double siniv,cosiv,sinrix,cosrix;
				sincos(iv,&siniv,&cosiv);
				sincos(rix,&sinrix,&cosrix);
				u=siniv+sinrix;
				v=cosiv+cosrix;
				x=u+t;
				//int a=(float)(i&0xF);
				int a=(float)i*(1/((float)in*(1.0/15.0)));
				int b=(float)j*(1/((float)jn*(1.0/16.0)));
				int c=16+16*a+b;
				vga_plot_unsafe(bb,(SCR_W/2.0)+u*sw,(SCR_H/2.0)+v*sh,c);
			}
		}
		t+=.0025;
		//t+=.025;
		//n=t/2;
		in=200;
		//in=6;
		jn=200;
		showpal();
		vga_drawstr(bb,buf,0,SCR_H-8,7);
		vsync();
		vga_swap(&bb);
		++frame;
		uticks=timer_elapsed();
		float time=timer_to_fsec(uticks);
		sprintf(buf,"%.2f fps",(float)frame/time);
		//n=n>200?n:n+1;
	}
}

SHL void rainbow(void){
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
		//vga_pal_set(i+16,x<<2,y<<2,0);
		//vga_pal_set(i+16,x<<2,63,y<<2);
		//vga_pal_set(i+16,y<<2,0,x<<2);
		//vga_pal_set(i+16,63,x<<2,y<<2);
		//vga_pal_set(i+16,0,x<<2,y<<2);
	}
}

int main(int argc, char **argv){
	assert(vga_dj_init());
	set_mode(0x13);
	//uint8_t pal_save[256*3];
	//vga_pal_save(pal_save);
	//rainbow();
	galaxy();

	slack();

	set_mode(0x3);
	return 0;
}
