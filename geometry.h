#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __GNUC__ // shut up gcc
#define SHL static __attribute__((__unused__)) __attribute__((fastcall))
#else
#define SHL static
#endif

typedef struct{
	int16_t x,y;
} PXY;

typedef struct{
   int16_t x1,y1,x2,y2;
} VRECT;

typedef struct{
	int16_t x,y,w,h;
} GRECT;

// handy dandy int16x4 copy, using this saves some code size
#if defined(__GNUC__) && defined(__OPTIMIZE_SIZE__)
__attribute__ ((noinline)) // no gcc don't inline it
#endif
SHL void copy_i16x4(const void* src,void* dst){
	int32_t* s=(int32_t*)src;
	volatile int32_t* d=(int32_t*)dst;
	d[0]=s[0];
	d[1]=s[1];
}
#define vr_copy(s,d) copy_i16x4(s,d) // copy a VRECT
#define gr_copy(s,d) copy_i16x4(s,d) // copy a GRECT

SHL void vr_print(const VRECT* r){
	printf("x1=%i y1=%i x2=%i y2=%i\n",r->x1,r->y1,r->x2,r->y2);}

SHL void gr_print(const GRECT* r){
	printf("x=%i y=%i w=%i h=%i\n",r->x,r->y,r->w,r->h);}

SHL bool clip_point(int x,int y,int w,int h){
	if(x<0) return false;
	if(y<0) return false;
	if(x>w-1) return false;
	if(y>h-1) return false;
	return true;
};

#define min(a,b) ({__typeof__(a)_a=(a);__typeof__(b)_b=(b);_a>_b?_b:_a;})
#define max(a,b) ({__typeof__(a)_a=(a);__typeof__(b)_b=(b);_a>_b?_a:_b;})

SHL bool gr_intersect(const GRECT* a,GRECT* b){
	int16_t x1=max(a->x,b->x);
	int16_t y1=max(a->y,b->y);
	int16_t x2=min(a->x+a->w,b->x+b->w);
	int16_t y2=min(a->y+a->h,b->y+b->h);
	b->x=x1;
	b->y=y1;
	b->w=x2-x1;
	b->h=y2-y1;
	return (x2>x1)&&(y2>y1);
}
