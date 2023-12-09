typedef int32_t fp_t;
#define FP_FRAC_BITS 16

SHL fp_t int_to_fp(int i){
	return i<<FP_FRAC_BITS;
}

SHL int fp_to_int(fp_t fp){
	return fp>>FP_FRAC_BITS;
}

SHL int16_t fp_to_int16(fp_t fp){
	return fp>>FP_FRAC_BITS;
}

SHL fp_t float_to_fp(double f){
	return f*(double)(1<<FP_FRAC_BITS);
}

SHL float fp_to_float(fp_t fp){
	return (float)fp/(float)(1<<FP_FRAC_BITS);
}

SHL fp_t fp_mul(fp_t a,fp_t b){
	return ((int64_t)a*(int64_t)b)>>FP_FRAC_BITS;
}

SHL fp_t fp_div(fp_t a,fp_t b){
	return ((int64_t)a<<FP_FRAC_BITS)/((int64_t)b);
}

// A third-order integer sine approximation
// @param x Angle (with 2^15 units/circle)
// @return  Sine value (Q12)
SHL int32_t isin_S3(int32_t x){
	//const int32_t qA=29; // Q-pos for output
	const int32_t qA=12;
	const int32_t qN=13; // Q-pos for quarter circle
	const int32_t qP=15; // Q-pos for parentheses intermediate
	const int32_t qR=2*qN-qP;
	const int32_t qS=qN+qP+1-qA;

	x=x<<(30-qN);    // shift to full s32 range (Q13->Q30)
	if((x^(x<<1))<0) // test for quadrant 1 or 2
		x=(1<<31)-x;
	x=x>>(30-qN);
	return x*((3<<qP)-(x*x>>qR))>>qS;
}

// fourth-order integer sine approximation
// @param x   angle (with 2^15 units/circle)
// @return     Sine value (Q12)
SHL int32_t isin_S4(int32_t x){
	const int qA=12; // Q-pos for output
	//const int qA=15; // Q-pos for output
	const int qN=13,B=19900,C=3516;

	int c=x<<(30-qN); // Semi-circle info into carry
	x-=1<<qN; // sine -> cosine calc

	x=x<<(31-qN); // Mask with PI
	x=x>>(31-qN); // Note: SIGNED shift! (to qN)
	x=x*x>>(2*qN-14); // x=x^2 To Q14

	int y=B-(x*C>>14); // B - x^2*C
	y=(1<<qA)-(x*y>>16); // A - x^2*(B-x^2*C)

	return c>=0?y:-y;
}

// fifth-order integer sine approximation
// @param i   angle (with 2^15 units/circle)
// @return    16 bit fixed point Sine value (4.12) (ie: +4096 = +1 & -4096 = -1)
SHL int16_t isin_S5(int16_t i){
	// Convert (signed) input to a value between 0 and 8192
	// (8192 is pi/2, which is the region of the curve fit)
	i<<=1;
	uint8_t c=i<0; // set carry for output pos/neg

	// flip input value to corresponding value in range [0..8192)
	if(i==(i|0x4000)) i=(1<<15)-i;
	i=(i&0x7FFF)>>1;

	// The following section implements the formula:
	// = y * 2^-n * ( A1 - 2^(q-p)* y * 2^-n * y * 2^-n * [B1 - 2^-r * y * 2^-n * C1 * y]) * 2^(a-q)
	// Where the constants are defined as follows:
	enum {A1=3370945099UL, B1=2746362156UL, C1=292421UL};
	enum {n=13, p=32, q=31, r=3, a=12};

	uint32_t y = (C1*((uint32_t)i))>>n;
	y=B1 - (((uint32_t)i*y)>>r);
	y=(uint32_t)i * (y>>n);
	y=(uint32_t)i * (y>>n);
	y=A1 - (y>>(p-q));
	y=(uint32_t)i * (y>>n);
	y=(y+(1UL<<(q-a-1)))>>(q-a); // Rounding

	return c?-y:y;
}

//Cos(x) = sin(x + pi/2)
#define fpcos(i) isin_S5((int16_t)(((uint16_t)(i)) + 8192U))

// 8 bit sine table
static int8_t sintbl8[UINT8_MAX+1];

SHL void sintbl8_init(void){
	sintbl8[        1+(UINT8_MAX>>2)]= INT8_MAX;
	sintbl8[UINT8_MAX-(UINT8_MAX>>2)]=-INT8_MAX;
	for(int i=0;i<=UINT8_MAX>>2;++i){
		double fi=(double)i*(1/(double)(UINT8_MAX+1));
		double f=sin(fi*M_PI*2);
		f*=(double)(INT8_MAX+1);
		//int n=isin_S3(i<<7)/257;
		//int n=isin_S4(i<<7)>>5;
		int n=isin_S5(i<<7)>>5;
		int error=f-n;
		//printf("% 2i",error);
		//putchar((i+1)%16?' ':'\n');
		sintbl8[i]=f;
		sintbl8[(UINT8_MAX>>1)+1-i]=n;
		n=-n;
		sintbl8[(UINT8_MAX>>1)+1+i]=n;
		sintbl8[UINT8_MAX+1-i]=n;
	}
	//putchar('\n');
	//for(int i=0;i<=UINT8_MAX;++i){
	//	printf("% 4i",sintbl8[i]);
	//	putchar((i+1)%16?' ':'\n');
	//}
}

SHL int8_t sin8(uint8_t n){
	return sintbl8[n];
}

// 16 bit sine table

#define SINTBL16_SIZE UINT16_MAX
static int16_t sintbl16[SINTBL16_SIZE+1];

SHL void sintbl16_init(void){
	int max=0;
	int min=0;
	sintbl16[            1+(SINTBL16_SIZE>>2)]= INT16_MAX;
	sintbl16[SINTBL16_SIZE-(SINTBL16_SIZE>>2)]=-INT16_MAX;
	for(int i=0;i<=SINTBL16_SIZE>>2;++i){
		double fi=(double)i*(1/(double)SINTBL16_SIZE);
		double f=sin(fi*M_PI*2);
		f*=(double)INT16_MAX+1;
		int n=isin_S5(i>>1)<<3;
		if(n>INT16_MAX) n=INT16_MAX;
		int error=f-n;
		//n=f;
		min=min(min,n);
		max=max(max,n);
		sintbl16[i]=n;
		sintbl16[(SINTBL16_SIZE>>1)+1-i]=n;
		n=-n;
		sintbl16[(SINTBL16_SIZE>>1)+1+i]=n;
		sintbl16[SINTBL16_SIZE+1-i]=n;
		//if(i<256){
			//printf("%i ",n);
			//printf("%i ",sintbl16[i]);
			//printf("% 3i",error);
			//putchar((i+1)%16?' ':'\n');
		//}
	}
	//for(int i=0;i<16;++i){
	//	printf("% 3i ",sintbl16[i]);
	//}
	//putchar('\n');
	//for(int i=0;i<16;++i){
	//	printf("% 3i ",sintbl16[SINTBL16_SIZE-i]);
	//}
	//putchar('\n');
	//printf("\nmin %i max %i\n",min,max);
}

SHL int16_t sin16(uint16_t n){
	n&=SINTBL16_SIZE;
	return sintbl16[n];
}

SHL int16_t cos16(uint16_t n){
	n+=(uint16_t)(SINTBL16_SIZE/4);
	return sin16(n);
}

SHL uint16_t float_to_sintbl(float n){
	return n*(1/(M_PI*2))*(double)SINTBL16_SIZE;
}

SHL int fp_to_sintbl(fp_t n){
	n=fp_mul(n,float_to_fp((1/(M_PI*2))*(double)SINTBL16_SIZE));
	return fp_to_int(n);
}

SHL fp_t sinfp(fp_t n){
	fp_t s=int_to_fp(sin16(fp_to_sintbl(n)));
	return fp_mul(s,float_to_fp(1/((1+(double)0xFFFF)/2)));
}

SHL fp_t cosfp(fp_t n){
	fp_t s=int_to_fp(cos16(fp_to_sintbl(n)));
	return fp_mul(s,float_to_fp(1/((1+(double)0xFFFF)/2)));
}

SHL float fast_sin(float n){
	float s=sin16(float_to_sintbl(n));
	s*=1.0/((1.0+(float)0xFFFF)/2.0);
	return s;
}

SHL float fsinfp(fp_t n){
	float s=sin16(float_to_sintbl(fp_to_float(n)));
	s*=1.0/((1.0+(float)0xFFFF)/2.0);
	return s;
}

SHL float fcosfp(fp_t n){
	float s=cos16(float_to_sintbl(fp_to_float(n)));
	s*=1.0/((1.0+(float)0xFFFF)/2.0);
	return s;
}

SHL float fast_cos(float n){
	float s=cos16(float_to_sintbl(n));
	s*=1.0/((1.0+(float)0xFFFF)/2.0);
	return s;
}
