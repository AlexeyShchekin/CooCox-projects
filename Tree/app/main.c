#include <stm32f10x.h>
#include "sys_init.h"

#define TIME 30

int main(void)
{
	Start();

    while(1)
    {
    	L1_ON;
    	WAIT(TIME);
    	L1_OFF;
    	WAIT(TIME);
    	L2_ON;
		WAIT(TIME);
		L2_OFF;
		WAIT(TIME);
		L3_ON;
		WAIT(TIME);
		L3_OFF;
		WAIT(TIME);
		L4_ON;
		WAIT(TIME);
		L4_OFF;
		WAIT(TIME);
		L5_ON;
		WAIT(TIME);
		L5_OFF;
		WAIT(TIME);
		L6_ON;
		WAIT(TIME);
		L6_OFF;
		WAIT(TIME);
		L7_ON;
		WAIT(TIME);
		L7_OFF;
		WAIT(TIME);
		L8_ON;
		WAIT(TIME);
		L8_OFF;
		WAIT(TIME);
		L9_ON;
		WAIT(TIME);
		L9_OFF;
		WAIT(TIME);
		L10_ON;
		WAIT(TIME);
		L10_OFF;
		WAIT(TIME);
    }
}
