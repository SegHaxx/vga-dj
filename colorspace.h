SHL void HSVtoRGB16(
		uint16_t* r,
		uint16_t* g,
		uint16_t* b,
		uint16_t h,
		uint16_t s,
		uint16_t v
){
	if(s==0){
		*r=*g=*b=v;
		return;
	}

	uint16_t f = (h%10923)*6;
	// fixed point multiplication
	#define mul16(a,b) ((((uint32_t)a)*((uint32_t)b))>>16)
	uint16_t p = mul16(v,65535-s);
	uint16_t q = mul16(v,65535-mul16(s,f));
	uint16_t t = mul16(v,65535-mul16(s,65535-f));

	switch(h/10923){
		case 0: *r=v; *g=t; *b=p; break;
		case 1: *r=q; *g=v; *b=p; break;
		case 2: *r=p; *g=v; *b=t; break;
		case 3: *r=p; *g=q; *b=v; break;
		case 4: *r=t; *g=p; *b=v; break;
		default: *r=v; *g=p; *b=q; break;
	}
}
