/*
 *note:
 Use =  when you’re initializing a register (you know it’s reset to 0).
 Use |=  when you’re changing or adding bits later (to not erase others).
 */

#include <stdint.h>
#include <stdio.h> //for sprintf
#include <string.h> //strcmp

//clocks
#define RCC_APB2ENR *((volatile uint32_t *)0x40021018)
// use 1U (unsigned(can only be pos or 0)) to match register type and avoid signed shift issues
#define RCC_IOPAEN (1U<<2) //enable GPIOA clock
#define RCC_USART1EN (1U<<14) //enable USART1 clock

//GPIO
#define GPIOA_CRH *((volatile uint32_t *)0x40010804) //config for PA9

//USART1
#define USART1_SR (*(volatile uint32_t*)0x40013800) //status(tx ready, rx ready)
#define USART1_DR (*(volatile uint32_t*)0x40013804) //data register, send and receive here
#define USART1_BRR (*(volatile uint32_t*)0x40013808) //baud rate setting
#define USART1_CR1 (*(volatile uint32_t*)0x4001380C) //control register

//bit helpers
#define CR1_UE (1U<<13) //USART enable
#define CR1_TE (1U<<3) //transmit enable
#define CR1_RE (1U<<2) //reciever enable
#define SR_TXE (1U<<7) //tx register empty flag
#define SR_RXNE (1U<<5) //rx data register not empty

//systick
#define SYST_CSR  (*(volatile uint32_t*)0xE000E010) //control/ status register
#define SYST_RVR (*(volatile uint32_t*)0xE000E014) //reload value: sets how many ticks till interrupt
#define SYST_CVR (*(volatile uint32_t*)0xE000E018) //current value register
//The SysTick calibration value is set to 9000, which gives a reference time base of 1 ms

//global variables
volatile uint32_t msTicks = 0; //incremented by systick every ms
volatile uint32_t user_is_typing = 0;
volatile uint32_t is_paused = 0;
#define CMD_BUFFER_SIZE 64
uint32_t seconds = 0;
uint32_t minutes = 0;
uint32_t hours = 0;
uint32_t lastTick = 0;

int displayTimer = 0;

//flag to see if theres data to read
static int usart1_read_char(char* ch){
	if(USART1_SR & SR_RXNE){ //has a char been recieved
		*ch = (char)USART1_DR;
		return 1; //data available
	}else{
		return 0; //no data available
	}
}

void SysTick_Handler(void){ //must be "SysTick_Handler"
	msTicks++; //increment every ms
}

static void systick_init(uint32_t ticks){
	SYST_RVR = ticks - 1; // set reload value. interupt service routine(isr) fires when ticks - 1 = 0
	SYST_CVR = 0; //// Clear current counter and COUNTFLAG
	SYST_CSR =  (1U<<0) | (1U<<1) | (1U<<2); //enable | interupt enable | clock source
}

//setup USART1 for TX on PA9
static void usart1_init(uint32_t baud){

	RCC_APB2ENR |= RCC_IOPAEN | RCC_USART1EN; // turn on GPIOA and USART1 clocks

	//PA9 (USART1_TX) alternate function push pull 50MHz
	GPIOA_CRH &= ~(0xFU <<4); //clear for PA9, F in binary is 1111. write 4 bits then invert to clear registers
	GPIOA_CRH |= (0xBU <<4); //MODE =11, CNF = 10, B in binary is 1011

	//PA10 (USART1_RX): INPUT FLOATING
	//mode = 00 (input) CNF = 01(floating input)
	GPIOA_CRH &= ~(0xFU << 8);
	GPIOA_CRH |= (0x4U << 8);

	USART1_BRR = 8000000 / baud; //set baud rate

	USART1_CR1 = CR1_UE | CR1_TE | CR1_RE; //turn on USART and enable TX and RX
}

static void usart1_write(char ch){

	while(!(USART1_SR & SR_TXE)); //wait until tx is empty
	USART1_DR = (uint32_t)ch; //write character
}

static void usart1_write_str(const char *s){

	while(*s)
		usart1_write(*s++); //send chars until string ends
}


void delay_ms(uint32_t ms){
	uint32_t start = msTicks;
	while((msTicks - start) < ms); //wait until ms elapsed

}

static void process_command(const char *cmd){
	if(cmd[0] == '\0') return; //case: entered nothing

	if(strcmp(cmd, "reset") == 0 || strcmp(cmd, "Reset") == 0){ //returns 0 if match
		seconds = 0;
		minutes = 0;
		hours = 0;
		usart1_write_str("\r\nTimer reset\r\n");

	}else if(strcmp(cmd, "help") == 0 || strcmp(cmd, "Help") == 0){
		usart1_write_str("\r\nAvailable commands:\r\n");
		usart1_write_str("\r\nReset - resets timer\r\n");
		usart1_write_str("\r\nStart - start displaying timer\r\n");
		usart1_write_str("\r\nStop - stop displaying timer\r\n");
		usart1_write_str("\r\nPause - pause the time\r\n");

	}else if((strcmp(cmd, "start") == 0) || strcmp(cmd, "Start") == 0){
		displayTimer = 1;
		usart1_write_str("\r\nTimer display started\r\n");

	}else if(strcmp(cmd, "stop") == 0 || strcmp(cmd, "Stop") == 0){
		displayTimer = 0;
		usart1_write_str("\r\nTimer display stopped\r\n");

	}else if(strcmp(cmd,"pause") == 0 || strcmp(cmd, "Pause") == 0){
		displayTimer = 0;
		is_paused = 1;
		usart1_write_str("\r\nTimer is paused\r\n");
	}else if(strcmp(cmd, "resume") == 0 || strcmp(cmd, "Resume") == 0){
		is_paused = 0;
		displayTimer = 1;
		usart1_write_str("\r\nTimer resumed\r\n");
	}else{
		usart1_write_str("\r\nUnkown command, type 'help' for help\r\n");
	}
}

int main(void)
{

	usart1_init(9600); //init usart1 at 9600 baud rate
	systick_init(8000); // 1 ms tick at 8 MHz

	//comand buffer variables
	char cmdBuffer[CMD_BUFFER_SIZE];
	uint32_t cmdIndex = 0;

	usart1_write_str("\r\n=== STM32 Terminal ===\r\n");
	usart1_write_str("Type 'help' for commands\r\n> ");

    /* Loop forever */
	for(;;){
		//If 1000 milliseconds have passed since the last time I printed count one more second
		if((msTicks - lastTick) >= 1000){
			lastTick = msTicks; //reset 1 second timer

			if(!is_paused){
				seconds++;

				if(seconds >= 60){
					minutes++;
					seconds = 0;

					if(minutes >= 60){
						hours++;
						minutes = 0;
					}
				}
			}

			if(displayTimer == 1 && !user_is_typing){
				char buf[64];
				sprintf(buf, "Time: %lu hours %lu minutes %lu seconds\r\n", (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds);
				usart1_write_str(buf); //print it
			}
		}

		char ch;
		if(usart1_read_char(&ch)){
			if(ch == '\n' || ch == '\r'){
				cmdBuffer[cmdIndex] = '\0';

				process_command(cmdBuffer); //process cmd

				cmdIndex = 0; //reset buffer

				user_is_typing = 0;

			} else if(ch == 127 || ch == 8){ //check for backspace
				if(cmdIndex > 0){
					user_is_typing = 1;
					cmdIndex--;
					usart1_write_str("\b \b"); // '\b' moves cursor to the left

				}
			}

			else { //normal char, add to the buffer
				if(cmdIndex < CMD_BUFFER_SIZE - 1){ //safety check, last char '\0'
					user_is_typing = 1;
					cmdBuffer[cmdIndex] = ch;
					cmdIndex++;

					usart1_write(ch);
				}
			}
		}
	}
}

