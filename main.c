/*
 * goal 					register			Address				bit change
 *
 * clock enable				RCC_APB2ENR			0x40021018			bit4 -> 1
 * pc13 -> output			GPIOC_CRH			0x40011004			bit20 -> 0, bit 21 -> 1
 * pc13 -> 3.3v				GPIOC_ODR			0x4001100C			bit13 -> 1
 * pc13 -> 0v				GPIOC_ODR			0x4001100C			BIT13-> 0
 *
 */

#include <stdint.h>
#define RCC_APB2ENR *((uint32_t *)0x40021018)
#define RCC_APB2ENR_IOPCEN 1<<4

void delay(uint32_t time){
	for(int i =0;i<time;i++){
		for(int j=0;j<time;j++);
	}
}

int main(void)
{

	RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;
	//crh
	*((uint32_t *)0x40011004) &= ~(1<<20);
	*((uint32_t *)0x40011004) |= (1<<20);

    /* Loop forever */
	for(;;){

		delay(1000);
		*((uint32_t *)0x4001100C) |= (1<<13);
		delay(1000);
		*((uint32_t *)0x4001100C) &= ~(1<<13);
	}
}
