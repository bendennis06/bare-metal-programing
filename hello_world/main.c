/*
 * goal 					register			Address				bit change
 *
 * clock enable				RCC_APB2ENR			0x40021018			bit4 -> 1
 * pc13 -> output			GPIOC_CRH			0x40011004			bit20 -> 0, bit 21 -> 1
 * pc13 -> 3.3v				GPIOC_ODR			0x4001100C			bit13 -> 1
 * pc13 -> 0v				GPIOC_ODR			0x4001100C			BIT13-> 0
 *
 *
 *
 *note:
 Use =  when you’re initializing a register (you know it’s reset to 0).
 Use |=  when you’re changing or adding bits later (to not erase others).
 */



#include <stdint.h>

//clocks
#define RCC_APB2ENR *((volatile uint32_t *)0x40021018)
// use 1U (unsigned(can only be pos or 0)) to match register type and avoid signed shift issues
#define RCC_IOPAEN (1U<<2) //enable GPIOA clock
#define RCC_USART1EN (1U<<14) //enable USART1 clock

//GPIO
#define GPIOA_CRH *((volatile uint32_t *)0x40010804)

//USART1
#define USART1_SR (*(volatile uint32_t*)0x40013800) //status(tx ready, rx ready)
#define USART1_DR (*(volatile uint32_t*)0x40013804) //data register, send and receive here
#define USART1_BRR (*(volatile uint32_t*)0x40013808) //baud rate setting
#define USART1_CR1 (*(volatile uint32_t*)0x4001380C) //control register

//bit helpers
#define CR1_UE (1U<<13) //USART enable
#define CR1_TE (1U<<3) //transmit enable
#define SR_TXE (1U<<7) //tx register empty flag

//setup USART1 for TX on PA9
static void usart1_init(uint32_t baud){

	RCC_APB2ENR |= RCC_IOPAEN | RCC_USART1EN; // turn on GPIOA and USART1 clocks

	GPIOA_CRH &= ~(0xFU <<4); //clear for PA9, F in binary is 1111. write 4 bits then invert to clear registers
	GPIOA_CRH |= (0xBU <<4); //MODE =11, CNF = 10, B in binary is 1011

	USART1_BRR = 8000000 / 9600; //set baud rate

	USART1_CR1 = CR1_UE | CR1_TE; //turn on USART and enable TX
}

static void usart1_write(char ch){

	while(!(USART1_SR & SR_TXE)); //wait until tx is empty
	USART1_DR = (uint32_t)ch; //write character
}

static void usart1_write_str(const char *s){

	while(*s)
		usart1_write(*s++); //send chars until string ends
}


void delay(uint32_t time){
	for (int i=0;i<time;i++)
		for(int j=0;j<time;j++);

}

int main(void)
{

	usart1_init(9600); //init usart1 at 9600 baud rate

    /* Loop forever */
	for(;;){

		usart1_write_str("hello world!\r\n");
		delay(10000);

	}
}
