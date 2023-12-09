// Benchmark and test

#include <pc.h>
#include <dpmi.h> //DOS protected mode?
#include <go32.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "vga_dj.h"
#include "colorspace.h"

#include "timer.h"

uint8_t bb[SCR_W*SCR_H];

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

static uclock_t uticks;
static int frame;

static void bench_start(char** log,char* str){
	if(log){
		*log=my_strcpy(*log,str);
		*log=my_strcpy(*log," ");
	}
	frame=0;
	timer_reset();
}

SHL bool frame_end(void){
	++frame;
	uticks=timer_elapsed();
	if(uticks>5*UCLOCKS_PER_SEC) return true;
	return false;
}

static void bench_end(char** log){
	int time=uticks/(UCLOCKS_PER_SEC/100);
	int fps=((uclock_t)frame*UCLOCKS_PER_SEC*100)/uticks;
	*log=itoa_u32(*log,frame);
	*log=my_strcpy(*log," frames in ");
	*log=itoa_u32_d(*log,time);
	*log=my_strcpy(*log," sec ");
	*log=my_strcpy(itoa_u32_d(*log,fps)," fps\n");
}

SHL void bench_cls(char** log){
	bench_start(log,"cls");
	while(1){
		vga_cls(&vga_fb,frame);
		if(frame_end()) break;
	}
	bench_end(log);
}

SHL void bench_plot_x(char** log){
	bench_start(log,"plot x");
	while(1){
		for(int y=-16;y<SCR_H+16;++y){
			uint8_t c=frame-y;
			for(int x=-16;x<SCR_W+16;++x){
				vga_plot(vga_fb.px,x,y,c++);
			}
		}
		if(frame_end()) break;
	}
	bench_end(log);
}

SHL void bench_plot_y(char** log){
	bench_start(log,"plot y");
	while(1){
		for(int x=-16;x<SCR_W+16;++x){ 
			uint8_t c=frame+x;
			for(int y=-16;y<SCR_H+16;++y){
				vga_plot(vga_fb.px,x,y,c++);
			}
		}
		if(frame_end()) break;
	}
	bench_end(log);
}

uint32_t rng_xor32_state=-1;
SHL uint32_t rng_xor32(void){
	uint32_t x=rng_xor32_state;
	x^=x<<13;
	x^=x>>17;
	x^=x<<5;
	return rng_xor32_state=x;
}

SHL void bench_rect_vr(char** log){
	bench_start(log,"rect vr");
	int count=0;
	for(int i=-16;i<SCR_W+16;++i){
		VRECT r;
		r.x1=i;
		r.y1=i;
		r.x2=SCR_W-i;
		r.y2=SCR_H-i;
		vga_rect_vr(vga_fb.px,&r,i);
		++count;
	}
	while(1){
		for(int c=0;c<256;++c){
			VRECT r;
			r.x1=rng_xor32()%(SCR_W*3)-SCR_W;
			r.y1=rng_xor32()%(SCR_H*3)-SCR_H;
			r.x2=rng_xor32()%(SCR_W*3)-SCR_W;
			r.y2=rng_xor32()%(SCR_H*3)-SCR_H;
			vga_rect_vr(vga_fb.px,&r,c);
			++count;
		}
		if(frame_end()) break;
	}
	int time=uticks/(UCLOCKS_PER_SEC/100);
	int rate=((uclock_t)count*UCLOCKS_PER_SEC*100)/uticks;
	*log=itoa_u32(*log,count);
	*log=my_strcpy(*log," rects in ");
	*log=itoa_u32_d(*log,time);
	*log=my_strcpy(*log," sec ");
	*log=my_strcpy(itoa_u32_d(*log,rate)," rects/sec\n");
}

SHL void bench_plot_y_bb(void){
	bench_start(0,0);
	while(1){
		for(int x=-16;x<SCR_W+16;++x){ 
			uint8_t c=frame+x;
			for(int y=-16;y<SCR_H+16;++y){
				vga_plot(bb,x,y,c++);
			}
		}
		if(frame_end()) break;
	}
	float time=timer_to_fsec(uticks);
	printf("plot y bb %i frames in %f sec %f fps\n",
			frame,time,(float)frame/time);
}

SHL void bench_text_bb(void){
	bench_start(0,0);
	while(1){
		//vsync();
		//vga_cls(0);
		for(int y=0;y<SCR_H;y+=8){
			uint8_t c=y/8+frame;
			for(int x=0;x<SCR_W;x+=8){
				vga_drawchar(bb,x,y,c,c);
				++c;
			}
		}
		if(frame_end()) break;
	}
	float time=timer_to_fsec(uticks);
	printf("drawchar %i frames in %f sec %f fps\n",
			frame,time,(float)frame/time);
}

SHL void benchmark(char** log){
	bench_cls(log);
	bench_plot_x(log);
	bench_plot_y(log);
	bench_rect_vr(log);
}

// set the palette to a nice spectrum
SHL void rainbow(void){
	for(int i=0;i<256;++i){
		uint16_t r,g,b;
		HSVtoRGB16(&r,&g,&b,i*(0xFFFF/256),0xFFFF,0xFFFF);
		vga_pal_set(i,r>>10,g>>10,b>>10);
	}
}

int main(int argc, char **argv){
	assert(vga_dj_init());
	set_mode(0x13);
	//uint8_t pal_save[256*3];
	//vga_pal_save(pal_save);
	rainbow();

	// buffer for the benchmark log
	// beware there is no overflow check
	char log[1024];
	char* logptr=log;
	benchmark(&logptr);
	set_mode(0x3);
	// scratch the last carriage return because puts() adds one
	logptr[-1]='\0';
	// output the log
	puts(log);
	printf("Benchmarking backbuffer...\n");
	bench_plot_y_bb();
	bench_text_bb();
	return 0;
}
