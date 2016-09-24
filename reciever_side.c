#include "lpc17xx_gpio.h"
#include "lpc17xx.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_pwm.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"

//the motor is controlled using L293D motor driver, 1 motor drive controls 2 motors
//input 1&2 controls motor 1, input 3&4 controls motor 2
//if input 1&2 is 01 -> motor rotates clockwise, 10 -> motor rotates anti clockwise, 00 -> motor stops

uint8_t data;
uint8_t flag = 0;
int MotorSpeed = 0;

void Delay(int time)
{
	Timer0_Wait(time);
}

static void pinconfig_PWM(int port, int pin)
{
	//Mingyu
	PINSEL_CFG_Type PinCfg;

	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 2;
	PinCfg.Portnum = port;
	PinCfg.Pinnum = pin;
	PINSEL_ConfigPin (&PinCfg);

}

void PWM_init()
{

	    // ensure PWM peripheral is powered on (LPC17xx manual table 46, pg 63)
	    // it is powered on by default
	        LPC_SC->PCONP |= (1 << 6);

	        LPC_PWM1->TCR = 2;                      // bring PWM module into reset
	        LPC_PWM1->IR = 0xff;                    // clear any pending interrupts

	    // Setup and provide the clock to PWM peripheral (LPC17xx manual table 40 and 42, pg 56 and 57).
	    // By default, SystemCoreClock is divided by 4 i.e PCLK = CCLK/4 (CCLK = 120MHz in LPC1769)
	    // i.e
	    // LPC_SC->PCLKSEL0 |= ((1 << 13)|(1 << 12)); // write 1 into bit 12 and 1 into bit 13
	                                                  // change PCLK = CCLK/8 (LPC17xx manual 42, pg 57)

	    // Select P2.0 as PWM1.1 Function (Pulse Width Modulator 1, channel 1 output)
	        pinconfig_PWM(2, 0);

	        /*
	           PINSEL_CFG_Type PinCfg;

	        	PinCfg.Funcnum = 1;    // function 01 is PWM 1.1
	        	PinCfg.OpenDrain = 0;
	        	PinCfg.Pinmode = 2;    // enable neither pull up nor pull down.
	        	PinCfg.Portnum = 2;    // P2.0
	        	PinCfg.Pinnum = 0;     // P2.0
	        	PINSEL_ConfigPin (&PinCfg);
	        */

	    // Set prescale so we have a resolution of 1us
	    // TC rate in Hz = SystemCoreClock / Peripheral divider value / (1 + Prescale Register).
	    // -> Prescale Register (PR) = SystemCoreClock / (Peripheral divider value * TC rate in Hz) - 1
	        LPC_PWM1->PR = SystemCoreClock / (4 * 1000000) - 1;

	    // Reset timer on Match0 (LPC17xx manual table 449, pg 517)
	        LPC_PWM1->MCR = 1 << 1;
	    // set Period/Duty Cycle
	        LPC_PWM1->MR0 = 20000;                  // set the period in us (20000*10^-6 = 20ms) -> 50Hz rate
	        LPC_PWM1->MR1 = 7000;                  // set duty cycle


	        LPC_PWM1->LER = (1 << 0) | (1 << 1);    // Load MR0 and MR1 value to apply changes

	        LPC_PWM1->PCR = (1 << 9);               // enable PWM1 with single-edge operation (LPC17xx manual table 451, pg 519)

	        LPC_PWM1->TCR = (1 << 3) | (1 << 0);    // Enable the timer/counter and PWM (LPC17xx manual table 447, pg 516)

}


//P0.1 RXD3
void pinsel_uart3(void)
{
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 2;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);
}

void UART3_Init(void)
{

	UART_CFG_Type uartCfg;
	//uartCfg.Baud_rate = 100000;
	uartCfg.Baud_rate = 2400;
	uartCfg.Databits = UART_DATABIT_8;
	uartCfg.Parity = UART_PARITY_NONE;
	uartCfg.Stopbits = UART_STOPBIT_1;
	//pin select for uart3
	pinsel_uart3();
	//supply power & setup working par.s for uart3
	UART_Init(LPC_UART3, &uartCfg);
	//enable transmit for uart3
	UART_TxCmd(LPC_UART3, ENABLE);
}

static void init_GPIO(int port, int pin, int dir){
	
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 2;
	PinCfg.Portnum = port;
	PinCfg.Pinnum = pin;
	PINSEL_ConfigPin (&PinCfg);

	GPIO_SetDir (port, 1<<pin, dir);
}




void turn_right(int time){
	//2500 for 90
	//5000 for 180 cw

	GPIO_ClearValue(0,1<<4);
	GPIO_SetValue(0,1<<5);
	Delay(time);
	GPIO_ClearValue(0,1<<4);
	GPIO_ClearValue(0,1<<5);
}

void turn_right_back(int time){
	GPIO_ClearValue(0,1<<5);
	GPIO_SetValue(0,1<<4);
	Delay(time);
	GPIO_ClearValue(0,1<<4);
	GPIO_ClearValue(0,1<<5);
}

void turn_left(int time){

	//2300 for 90
	GPIO_ClearValue(0,1<<10);
	GPIO_SetValue(0,1<<11);
	Delay(time);
	GPIO_ClearValue(0,1<<11);
	GPIO_ClearValue(0,1<<10);
}

void turn_left_back(int time){
	GPIO_ClearValue(0,1<<11);
	GPIO_SetValue(0,1<<10);
	Delay(time);
	GPIO_ClearValue(0,1<<11);
	GPIO_ClearValue(0,1<<10);
}


void turn_180degree_anticw(void){
	GPIO_ClearValue(0,1<<5);
	GPIO_SetValue(0,1<<4);
	Delay(4800);
	GPIO_ClearValue(0,1<<4);
	GPIO_ClearValue(0,1<<5);
}

void go_straight_forward(int time){
	 GPIO_ClearValue(0,1<<10);
	 GPIO_SetValue(0,1<<11);
	 GPIO_ClearValue(0,1<<4);
	 GPIO_SetValue(0,1<<5);
	 Delay(time);
	 GPIO_ClearValue(0,1<<11);
	 GPIO_ClearValue(0,1<<5);
}


void go_back(int time){

	GPIO_ClearValue(0,1<<5);
	GPIO_SetValue(0,1<<4);

	GPIO_ClearValue(0,1<<11);
	GPIO_SetValue(0,1<<10);

	Delay(time);
	GPIO_ClearValue(0,1<<10);
	GPIO_ClearValue(0,1<<4);
}

void stop(void){
	GPIO_ClearValue(0,1<<10);
	GPIO_ClearValue(0,1<<5);
	GPIO_ClearValue(0,1<<11);
	GPIO_ClearValue(0,1<<4);
}

void open_arm(void){
	GPIO_ClearValue(2,1<<9);
	GPIO_SetValue(0,1<<20);
	Delay(3000);
	GPIO_ClearValue(0,1<<20);
	GPIO_ClearValue(2,1<<9);
}

void close_arm(void){

	GPIO_ClearValue(0,1<<20);
	GPIO_SetValue(2,1<<9);
	Delay(4000);
	GPIO_ClearValue(0,1<<20);
	GPIO_ClearValue(2,1<<9);
}

void borrow_book1(void){
	   turn_right(2150);
	   Delay(1000);
	   go_straight_forward(800);
	   Delay(1000);
	   close_arm();
	   Delay(1000);
	   go_back(800);
	   Delay(1000);
	   turn_right_back(1850);
	   Delay(1000);
	   turn_left(2350);
	   Delay(1000);
	   open_arm();
	   Delay(1000);
	   turn_left_back(1900);
}

void borrow_book2(void){
	  go_straight_forward(1400);
	  Delay(1000);
	  turn_right(2200);
	  Delay(1000);
	  go_straight_forward(800);
	  Delay(1000);
	  close_arm();
	  Delay(1000);
	  go_back(800);
	  Delay(1000);
	  turn_right_back(1800);
	  Delay(1000);
	  go_back(1200);
	  Delay(1000);
	  turn_left(2100);
	  Delay(1000);
	  open_arm();
	  Delay(1000);
	  turn_left_back(1900);
}

void borrow_book3(void){
	  go_straight_forward(2700);
	  Delay(1000);
	  turn_right(2100);
	  Delay(1000);
	  go_straight_forward(800);
	  Delay(1000);
	  close_arm();
	  Delay(1000);
	  go_back(800);
	  Delay(1000);
	  turn_right_back(1600);
	  Delay(1000);
	  go_back(2200);
	  Delay(1000);
	  turn_left(2100);
	  Delay(1000);
	  open_arm();
	  Delay(1000);
	  turn_left_back(1900);
}

void return_book1(void){
	turn_left(2350);
	Delay(1000);
	close_arm();
	turn_left_back(1900);
	Delay(1000);
	turn_right(2100);
	Delay(1000);
	go_straight_forward(800);
	Delay(1000);
	open_arm();
	Delay(1000);
	go_back(800);
	Delay(1000);
	turn_right_back(1800);
}

void return_book2(void){
	  turn_left(2350);
	  Delay(1000);
	  close_arm();
	  Delay(1000);
	  turn_left_back(1900);
	  Delay(1000);
	  go_straight_forward(1300);
	  Delay(1000);
	  turn_right(1950);
	  Delay(1000);
	  go_straight_forward(800);
	  Delay(1000);
	  open_arm();
	  Delay(1000);
	  go_back(800);
	  Delay(1000);
	  turn_right_back(1800);
	  Delay(1000);
	  go_back(1200);
}

void return_book3(void){
	  turn_left(2250);
	  Delay(1000);
	  close_arm();
	  Delay(1000);
	  turn_left_back(1900);
	  Delay(1000);
	  go_straight_forward(2400);
	  Delay(1000);
	  turn_right(1950);
	  Delay(1000);
	  go_straight_forward(800);
	  Delay(1000);
	  open_arm();
	  Delay(1000);
	  go_back(800);
	  Delay(1000);
	  turn_right_back(1500);
	  Delay(1000);
	  go_back(2200);
}

int main(void)
{
		PWM_init();
		UART3_Init();
		UART_IntConfig(LPC_UART3, UART_INTCFG_RBR, ENABLE);
		//NVIC_EnableIRQ(UART3_IRQn);

	    /* Enable GPIO Clock */
	    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPIO, ENABLE);
	    /* Motor Driver 1  Output  */
	    init_GPIO(0,11,1);
	    init_GPIO(0,10,1);
	    /* Motor Driver 1 left Output  */
	    init_GPIO(0,4,1);
	    init_GPIO(0,5,1);
	    /* Motor Driver 2  Output  */
	    init_GPIO(2,9,1);
	    init_GPIO(0,20,1);

while (flag==0)
{
		data = UART_ReceiveData(LPC_UART3);	
		if(data=='1')
		{
			borrow_book1();
		}
		
		if(data=='2')
		{
			borrow_book2();
		}
		
		if(data=='3')
		{
			borrow_book3();
		}
					
		if(data=='4')
		{
			rerurn_book1();
		}
			
		if(data=='5')
		{
			rerurn_book2();
		}
			
		if(data=='6')
		{
			rerurn_book3();
		}
	
}
	    return 0;
}


