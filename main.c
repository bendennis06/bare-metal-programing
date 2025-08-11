
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
#include <stdbool.h>
#define RCC_APB2ENR_REGISTER *((uint32_t *)0x40021018)
#define RCC_APB2ENR_IOPCEN 1<<4 //ENABLE CLOCK TO GPIOC
#define RCC_APB2ENR_USART1 1<<14 //ENABLE CLOCK TO USART1
#define RCC_APB2ENR_IOPAEN 1<<2 //ENABLE CLOCK TO GPIOA

#define GPIOA_CRH_REGISTER *((uint32_t *)0x40010804)
#define GPIOC_CRH_REGISTER *((uint32_t *)0x40011004)

#define SYST_CSR *((uint32_t *)0xE000E010) //CONTROL AND STATUS
#define SYST_RVR *((uint32_t *)0xE000E014) //RELOAD VALUE
#define SYST_CVR *((uint32_t *)0xE000E018) //CURRENT VALUE
#define SYSTICK_ENABLE (1<<2)
#define SYSTICK_TICKINIT (1<<1)
#define SYSTICK_CLKSOURCE (1<<0)

volatile uint32_t ms_ticks = 0;
volatile uint32_t m_ticks = 0;
volatile uint32_t s_ticks = 0;
volatile uint32_t h_ticks = 0;
volatile uint8_t stopwatch_running = 0;

#define USART_BRR *((uint32_t *)0x40013808)
#define USART_CR1 *((uint32_t *)0x4001380C)
#define CR1_UE (1U<<13)
#define CR1_TE (1U<<3)
#define CR1_RE (1U<<2)
#define USART_SR *((uint32_t *)0x40013800)
#define ISR_TXE (1U<<7) //TRANSMIT DATA REGISTER EMPTY
#define USART_DR *((uint32_t *)0x40013804)

void usart1_set_baud(uint32_t baud){
	USART_BRR = (72000000/baud);
}

void usart1_CR1_init(void){
	USART_CR1 = CR1_UE | CR1_TE | CR1_RE;
}

void usart1_write(int ch){
	//make sure transmit register is not empty
	//while(!(USART_SR & ISR_TXE)); //wait for TX is set(ready)
	USART_DR = (ch & 0xFF);//FF masks lower 8 bits of ch so only 1 byte value is written
}

void systick_init(uint32_t ticks){
	SYST_RVR = ticks - 1;
	SYST_CVR = 0;  //resets to 0 and clears countflag
	SYST_CSR = SYSTICK_ENABLE | SYSTICK_TICKINIT | SYSTICK_CLKSOURCE;
}

void SysTick_Handler(void){ //assuming it fires every millisecond
	if(!stopwatch_running){
		return;
	}
	ms_ticks++;
	if(ms_ticks >= 1000){
		ms_ticks = 0;
		s_ticks++;
		if(s_ticks >=60){
			s_ticks=0;
			m_ticks++;
			if(m_ticks>=60){
				m_ticks=0;
				h_ticks++;
			}
		}
	}
	//code that runs every tick
}

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
	GPIOA_CRH_REGISTER &= ~(0b1111<<8);//PA_10
	GPIOA_CRH_REGISTER |= (0b0100<<8);

	//GPIOA PA9(TX) AND PA10(RX)

	stopwatch_running = 1;
	systick_init(72000); // 72MHz / 72000 = 1ms
	usart1_set_baud(9600);
	usart1_CR1_init();

	delay(500000);

	*((uint32_t *)0x4001100C) |= (1<<13);  // LED ON

	usart1_write('H');

	*((uint32_t *)0x4001100C) &= ~(1<<13);  // LED off

	usart1_write('i');
	usart1_write('\r');
	usart1_write('\n');


    /* Loop forever */
	for(;;){

		delay(1000);
		*((uint32_t *)0x4001100C) |= (1<<13);
		delay(1000);
		*((uint32_t *)0x4001100C) &= ~(1<<13);
	}
}


