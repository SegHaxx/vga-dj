#include "time.h"

static uclock_t timer_ref;

SHL void timer_reset(void){
	timer_ref=uclock();
}

SHL uclock_t timer_elapsed(void){
	return uclock()-timer_ref;
}

SHL float timer_to_fsec(uclock_t t){
	return (float)t/(float)UCLOCKS_PER_SEC;
}

SHL float timer_elapsed_fsec(void){
	return timer_to_fsec(timer_elapsed());
}
