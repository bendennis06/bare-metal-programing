
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
#define RCC_APB2ENR_REGISTER *((uint32_t *)0x40021018)
#define RCC_APB2ENR_IOPCEN 1<<4 //ENABLE CLOCK TO GPIOC
#define RCC_APB2ENR_USART1 1<<14 //ENABLE CLOCK TO USART1
#define RCC_APB2ENR_IOPAEN 1<<2 //ENABLE CLOCK TO GPIOA

#define GPIOA_CRH_REGISTER *((uint32_t *)0x40010804)
#define GPIOC_CRH_REGISTER *((uint32_t *)0x40011004)

void delay(uint32_t time){
	for(int i =0;i<time;i++){
		for(int j=0;j<time;j++);
	}
}

int main(void)
{
	//CLOCK
	RCC_APB2ENR_REGISTER |= RCC_APB2ENR_USART1; //ENABLE USART1
	RCC_APB2ENR_REGISTER |= RCC_APB2ENR_IOPCEN; //ENABLE GPIOC(LED)
	RCC_APB2ENR_REGISTER |= RCC_APB2ENR_IOPAEN; //ENABLE GPIOA(USART PINS)

	//CRH
	GPIOC_CRH_REGISTER &= ~(1<<20); //LED
	GPIOC_CRH_REGISTER |= (1<<20);
	GPIOA_CRH_REGISTER &= ~(0b1111<<4);//PA_9
	GPIOA_CRH_REGISTER |= (0b1010<<4);
	GPIOA_CRH_REGISTER &= ~(0b1111<<8);
	GPIOA_CRH_REGISTER |= (0b0100<<8);





    /* Loop forever */
	for(;;){

		delay(1000);
		*((uint32_t *)0x4001100C) |= (1<<13);
		delay(1000);
		*((uint32_t *)0x4001100C) &= ~(1<<13);
	}
}

