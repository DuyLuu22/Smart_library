#include "LPC17xx.h"
#include "LPC17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include <stdio.h>
#include <string.h>
#include "lpc17xx_uart.h"
#include "temp.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_pwm.h"


#define NOTE_PIN_HIGH() GPIO_SetValue(2, 1<<11);
#define NOTE_PIN_LOW()  GPIO_ClearValue(2, 1<<11);

#define LCD_Port 1
#define LCD_E 24
#define LCD_RW 21
#define LCD_RS 18

#define LCD_DB0 27
#define LCD_DB1 28
#define LCD_DB2 25
#define LCD_DB3 26
#define LCD_DB4 29 //4.21
#define LCD_DB5 19 //0.19
#define LCD_DB6 20 //0.20
#define LCD_DB7 9  //2.9

//Universal variables declaration
unsigned char char_info[];
int book_num=0;			//count the number of spaces when key in book number
int num=0;				//count book number
int num_b = 0;
int num_borrowed = 0;
int keypad_button=0;
int page_num=0;
int choice=0;
int keypad_row=0;
int LCD_pos;
int loop1,loop2,loop3; // for the 2 pages
int loop;
int delay;
int book_num_max = 3;
int book_num_max_new = 3;
int book_num_max_fic = 3;
int book_num_max_non_fic = 3;
int enter=0;
int status=0;
int book_data[10][10]={0};
int end_status=0;
int end_status1=0;
int exit=0;
int menu=0;
int book;
int book_cat = 0;
int backspace=0;
int s1,s2=0;
int func_num = 0;
int login;
int id_flag, book1_flag,book2_flag, book3_flag = 0;
int book_rec = 0;
uint8_t msg[10];
int num_seat;
int temp,light;
int alarm=0;
int ir_num = 9;




static uint32_t msTicks; // counter for 1ms SysTicks

//  SysTick_Handler - just increment SysTick counter
void SysTick_Handler(void) {
  msTicks++;
}
static uint32_t notes[] = {
        2272, // A - 440 Hz
        2024, // B - 494 Hz
        3816, // C - 262 Hz
        3401, // D - 294 Hz
        3030, // E - 330 Hz
        2865, // F - 349 Hz
        2551, // G - 392 Hz
        1136, // a - 880 Hz
        1012, // b - 988 Hz
        1912, // c - 523 Hz
        1703, // d - 587 Hz
        1517, // e - 659 Hz
        1432, // f - 698 Hz
        1275, // g - 784 Hz
        956,  //h(c) - 1046Hz
};
static void playNote(uint32_t note, uint32_t durationMs) {

    uint32_t t = 0;

    if (note > 0) {

        while (t < (durationMs*1000)) {
            NOTE_PIN_HIGH();
            Timer0_us_Wait(note / 2);
            //delay32Us(0, note / 2);

            NOTE_PIN_LOW();
            Timer0_us_Wait(note / 2);
            //delay32Us(0, note / 2);

            t += note;
        }

    }
    else {
    	Timer0_Wait(durationMs);
        //delay32Ms(0, durationMs);
    }
}

static uint32_t getNote(uint8_t ch){
    if (ch >= 'A' && ch <= 'G')
        return notes[ch - 'A'];

    if (ch >= 'a' && ch <= 'h')
        return notes[ch - 'a' + 7];

    return 0;
}

static uint32_t getDuration(uint8_t ch){
    if (ch < '0' || ch > '9')
        return 800;

    /* number of ms */

    return (ch - '0') * 200;
}

static uint32_t getPause(uint8_t ch){
    switch (ch) {
    case '+':
        return 0;
    case ',':
        return 10;
    case '.':
        return 40;
    case '_':
        return 60;
    case '-':
    	return 800;
    case '=':
    	return 400;
    default:
        return 10;
    }
}

static void playSong(uint8_t *song) {
    uint32_t note = 0;
    uint32_t dur  = 0;
    uint32_t pause = 0;

    /*
     * A song is a collection of tones where each tone is
     * a note, duration and pause, e.g.
     *
     * "E2,F4,"
     */

    while(*song != '\0') {
        note = getNote(*song++);
        if (*song == '\0')
            break;
        dur  = getDuration(*song++);
        if (*song == '\0')
            break;
        pause = getPause(*song++);

        playNote(note, dur);
        //delay32Ms(0, pause);
        Timer0_Wait(pause);

    }
}

static uint8_t * song = (uint8_t*)
		"b4,b4,";
		//"E2,G1,C5.A2,c1,F5.B2,c1,d1,c4="; //Icecream
		//"G4.c6.e2,g6.c2,B6.e2,g6.g2,a6.b2,h2,b4.a2,g8-e2,d2,c6.c2,c4.e2,d2,c6.c2,c4.d2,e2,d6.A2,c4.B4.c8-"; //the moon represents my heart
		//"c6.c2,c4.c4.B4.c8.c4.B4.c8.d4.e8.d8.c6.c2,c4.c4.B4.c8.c4.G8."; //my heart will go on
        //(uint8_t*)"C2.C2,D4,C4,F4,E8,C2.C2,D4,C4,G4,F8,C2.C2,c4,A4,F4,E4,D4,A2.A2,H4,F4,G4,F8,";
        //"D4,B4,B4,A4,A4,G4,E4,D4.D2,E4,E4,A4,F4,D8.D4,d4,d4,c4,c4,B4,G4,E4.E2,F4,F4,A4,A4,G8,";

static void init_GPIO(int port, int pin, int dir)
{
	PINSEL_CFG_Type PinCfg;

	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = port;
	PinCfg.Pinnum = pin;
	PINSEL_ConfigPin (&PinCfg);

	GPIO_SetDir (port, 1<<pin, dir);
}

static void init_GPIO_pulldown(int port, int pin, int dir)
{
	PINSEL_CFG_Type PinCfg;

	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 3;
	PinCfg.Portnum = port;
	PinCfg.Pinnum = pin;
	PINSEL_ConfigPin (&PinCfg);

	GPIO_SetDir (port, 1<<pin, dir);
}
void init_LED(void)
{
	init_GPIO(2,12,1);
	init_GPIO(2,10,1);
}
void init_7segment(void)
{
	init_GPIO(0,17,1);	//a
	init_GPIO(0,18,1);	//b
	init_GPIO(0,6,1);	//c
	init_GPIO(0,22,1);	//d
	init_GPIO(0,21,1);	//e
	init_GPIO(0,3,1);	//f
	init_GPIO(0,2,1);	//g
	init_GPIO(1,31,1);	//db
}

void clear_segment(void)
{
	GPIO_ClearValue(0,1<<17);
	GPIO_ClearValue(0,1<<18);
	GPIO_ClearValue(0,1<<6);
	GPIO_ClearValue(0,1<<22);
	GPIO_ClearValue(0,1<<21);
	GPIO_ClearValue(0,1<<3);
	GPIO_ClearValue(0,1<<2);
	GPIO_ClearValue(1,1<<31);
}

void seven_display(int var)
{
	clear_segment();
	switch (var)
	{
		case 0:
			GPIO_SetValue(0,1<<17);	//a
			GPIO_SetValue(0,1<<18);	//b
			GPIO_SetValue(0,1<<6);	//c
			GPIO_SetValue(0,1<<22);	//d
			GPIO_SetValue(0,1<<21);	//e
			GPIO_SetValue(0,1<<3);	//f
			break;
		case 1:
			GPIO_SetValue(0,1<<18);	//b
			GPIO_SetValue(0,1<<6);	//c
			break;
		case 2:
			GPIO_SetValue(0,1<<17);	//a
			GPIO_SetValue(0,1<<18);	//b
			GPIO_SetValue(0,1<<2);	//g
			GPIO_SetValue(0,1<<21);	//e
			GPIO_SetValue(0,1<<22);	//d
			break;
		case 3:
			GPIO_SetValue(0,1<<17);	//a
			GPIO_SetValue(0,1<<18);	//b
			GPIO_SetValue(0,1<<2);	//g
			GPIO_SetValue(0,1<<6);	//c
			GPIO_SetValue(0,1<<22);	//d
			break;
		case 4:
			GPIO_SetValue(0,1<<18);	//b
			GPIO_SetValue(0,1<<3);	//f
			GPIO_SetValue(0,1<<2);	//g
			GPIO_SetValue(0,1<<6);	//c
			break;
		case 5:
			GPIO_SetValue(0,1<<17);	//a
			GPIO_SetValue(0,1<<3);	//f
			GPIO_SetValue(0,1<<2);	//g
			GPIO_SetValue(0,1<<6);	//c
			GPIO_SetValue(0,1<<22);	//d
			break;
		case 6:
			GPIO_SetValue(0,1<<17);	//a
			GPIO_SetValue(0,1<<3);	//f
			GPIO_SetValue(0,1<<2);	//g
			GPIO_SetValue(0,1<<6);	//c
			GPIO_SetValue(0,1<<22);	//d
			GPIO_SetValue(0,1<<21);	//e
			break;
		case 7:
			GPIO_SetValue(0,1<<17);	//a
			GPIO_SetValue(0,1<<18);	//b
			GPIO_SetValue(0,1<<6);	//c
			break;
		case 8:
			GPIO_SetValue(0,1<<17);	//a
			GPIO_SetValue(0,1<<18);	//b
			GPIO_SetValue(0,1<<6);	//c
			GPIO_SetValue(0,1<<22);
			GPIO_SetValue(0,1<<21);
			GPIO_SetValue(0,1<<3);
			GPIO_SetValue(0,1<<2);
			break;
		case 9:
			GPIO_SetValue(0,1<<17);	//a
			GPIO_SetValue(0,1<<18);	//b
			GPIO_SetValue(0,1<<6);	//c
			GPIO_SetValue(0,1<<22);	//d
			GPIO_SetValue(0,1<<3);	//f
			GPIO_SetValue(0,1<<2);	//g
			break;
	}
}
void init_ADC(void)
{
	//configure P0.24 to adc mode
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 24;
	PINSEL_ConfigPin(&PinCfg);
}

void init_LDR(void)
{
	//configure P1.30 to adc mode
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 25;
	PINSEL_ConfigPin(&PinCfg);
}

void UART1_Config(void)
{
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 1;
	PinCfg.Pinnum = 15;
	PinCfg.Pinmode = 2;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 16;
	PINSEL_ConfigPin(&PinCfg);
}

//own UART1_Init() initialisation function. not to be mistaken with UART_Init()
void UART1_Init(void)
{
	UART_CFG_Type uartCfg;
	uartCfg.Baud_rate = 2400;
	uartCfg.Databits = UART_DATABIT_8;
	uartCfg.Parity = UART_PARITY_NONE;
	uartCfg.Stopbits = UART_STOPBIT_1;
	UART1_Config();
	UART_Init(LPC_UART1, &uartCfg);
	UART_TxCmd(LPC_UART1, ENABLE);

//	UART_IntConfig(LPC_UART1,UART_INTCFG_RBR,ENABLE);
//	NVIC_EnableIRQ (UART1_IRQn);

	init_GPIO(0,23,1);  //set P0.23 as output to enable the RFID reader
}

void check_fire(void)
{
	int i;
	int j;
	while (!(ADC_ChannelGetStatus(LPC_ADC,1,ADC_DATA_DONE)));
			temp = ADC_ChannelGetData(LPC_ADC, 1);
			temp=temp*330/4095;
			if (temp>27) alarm = 1;
			printf("temperature: %d\n",temp);

	while (alarm==1)
	{
		for (j=0;j<10;j++)
		{
		init_GPIO(2,12,0); //init push button to disable alarm

		LCD_command(0x01);
		playSong(song);
		LCD_command(0x83);
		LCD_string("WARNING!!!");
		for (i=0;i<1700000;i++);
		LCD_command(0x08);
		for (i=0;i<400000;i++);
		LCD_command(0x0c);
		printf("temperature_alarm: %d\n",temp);
		}
		alarm = 0;
	}
}

void check_light(void)
{
	int i;
	while (!(ADC_ChannelGetStatus(LPC_ADC,2,ADC_DATA_DONE)));
			light = ADC_ChannelGetData(LPC_ADC, 2);
//			light = light*330/4095;
			printf("light: %d\n",light);
	if (light<600)
	{
		GPIO_ClearValue(2,1<<10);
		GPIO_ClearValue(2,1<<12);
	}
	else if ((light>600)&&(light<1500))
	{
		GPIO_SetValue(2,1<<10);
		GPIO_ClearValue(2,1<<12);
	}
	else if (light>1400)
	{
		light = ADC_ChannelGetData(LPC_ADC, 2);
		if (light>1400)
		{
			GPIO_SetValue(2,1<<10);
			GPIO_SetValue(2,1<<12);
		}
	}
}
void read_RFID_1(void)//for ID only
{
	//RFID variables
		uint8_t null[12] = {10,10,10,10,10,10,10,10,10,10,10,10};
		uint8_t id[12] = {10, 48, 70, 48, 50, 70, 50, 67, 53, 53, 51, 13}; //0F02F2C553
		uint8_t data = 0;
		uint8_t data1[12];
		uint8_t count_RFID = 0;
		uint8_t count_id = 0;
		uint8_t end = 0;
		backspace = 0;
		exit = 0;
		menu = 0;
		char c;
		int j;
		int count_0 = 0;

		printf("234\n");
	while((end == 0)&&(exit==0)&&(backspace==0)&&(menu==0))
	{
		count_id = 0;

		printf("271\n");
		count_RFID = 0;

		GPIO_SetValue(0,1<<23);  // disable RFID (high is disable)
		for (j=0;j<3;j++)
			UART_Receive(LPC_UART1, &data, 3, NONE_BLOCKING);
		printf("278 data %d \n",data);
		GPIO_ClearValue(0,1<<23);  //enable RFID (low is enable)

		while ((count_RFID < 12 )&&(exit==0)&&(backspace==0)&&(menu==0))
		{
			UART_Receive(LPC_UART1, &data, 1, BLOCKING);
			printf("%d",data);
			data1[count_RFID] = data;
			if(data1[1] != 10)
				count_RFID++;
		}
		printf(" end\n");

		data = 0;

		for(count_RFID=0; count_RFID<12; count_RFID++)
		{
			if(data1[count_RFID] == id[count_RFID])
				count_id++;
		}

		if(count_id==12)
		{
			GPIO_SetValue(0,1<<23);  // disable RFID (high is disable)
			LCD_busy();
			end = 1;
			id_flag = 1;
		}
	}
}

void read_RFID(void)
{
	//RFID variables
		uint8_t null[12] = {10,10,10,10,10,10,10,10,10,10,10,10};
		uint8_t id[12] = {10, 48, 70, 48, 50, 70, 50, 67, 53, 53, 51, 13}; //0F02F2C553
		uint8_t id2[12] = {10, 48, 70, 48, 51, 48, 52, 50, 66, 70, 70, 13}; // 0F03042DFF
		uint8_t book1[12] = {10, 48, 70, 48, 50, 70, 50, 68, 66, 48, 48, 13}; // 0F02F2DB00
		uint8_t book2[12] = {10, 48, 70, 48, 50, 70, 50, 69, 48, 53, 48, 13}; // 0F02F2E050
		uint8_t book3[12] = {10, 48, 70, 48, 50, 70, 50, 66, 66 ,55, 50, 13}; //0F02F2
		uint8_t data = 0;
		uint8_t data1[12];
		uint8_t count_RFID = 0;
		uint8_t count_id = 0;
		uint8_t count_book1,count_book2,count_book3,count_null = 0;
		uint8_t end = 0;
		backspace = 0;
		exit = 0;
		menu = 0;
		char c;
		int j;
		int count_0 = 0;

		printf("234\n");
	while((end == 0)&&(exit==0)&&(backspace==0)&&(menu==0))
	{
		count_id = 0;
		count_book1=0;
		count_book2=0;
		count_book3=0;

		printf("271\n");
		count_RFID = 0;

		GPIO_SetValue(0,1<<23);  // disable RFID (high is disable)
		for (j=0;j<3;j++)
			UART_Receive(LPC_UART1, &data, 3, NONE_BLOCKING);
		printf("278 data %d \n",data);
		GPIO_ClearValue(0,1<<23);  //enable RFID (low is enable)

		printf("281\n");
		count_0 = 0;
		while (((data==0)||(data==10))&&(count_0<10000))
		{
			UART_Receive(LPC_UART1, &data, 1, NONE_BLOCKING);
			printf("%d",data);
			count_0++;
		}
		printf(" 289\n");
		printf("290 data %d\n",data);


		if ((data!=0)&&(count_0<9900))
		{
		while ((count_RFID < 12 )&&(exit==0)&&(backspace==0)&&(menu==0))
		{
			UART_Receive(LPC_UART1, &data, 1, BLOCKING);
			printf("%d",data);
			data1[count_RFID] = data;
			if(data1[1] != 10)
				count_RFID++;
		}
		printf(" end\n");

		data = 0;

		for(count_RFID=0; count_RFID<12; count_RFID++)
		{
			if(data1[count_RFID] == id[count_RFID])
				count_id++;
			if(data1[count_RFID] == book1[count_RFID])
				count_book1++;
			if(data1[count_RFID] == book2[count_RFID])
				count_book2++;
			if(data1[count_RFID] == book3[count_RFID])
				count_book3++;
			if(data1[count_RFID] == null[count_RFID])
				count_null++;
		}

		printf("count book3 %d \n",count_book3);
//		if(count_null==12)
//		{
//			GPIO_SetValue(0,1<<23);  // disable RFID (high is disable)
//			LCD_busy();
//			end = 1;
//			id_flag = 1;
//			printf("fck\n");
//		}

		if(count_id==12)
		{
			GPIO_SetValue(0,1<<23);  // disable RFID (high is disable)
			LCD_busy();
			end = 1;
			id_flag = 1;
			book1_flag = 0;
			book2_flag = 0;
			book3_flag = 0;
		}

		else if(count_book1==12)
		{
			GPIO_SetValue(0,1<<23);
			LCD_busy();
			end = 1;
			id_flag = 0;
			book1_flag = 1;
			book2_flag = 0;
			book3_flag = 0;
		}

		else if(count_book2==12)
		{
			GPIO_SetValue(0,1<<23);
			LCD_busy();
			end = 1;
			id_flag = 0;
			book1_flag = 0;
			book2_flag = 1;
			book3_flag = 0;
		}
		else if(count_book3==12)
		{
			GPIO_SetValue(0,1<<23);
			LCD_busy();
			end = 1;
			id_flag = 0;
			book1_flag = 0;
			book2_flag = 0;
			book3_flag = 1;
		}
		}
		else
		{
			end = 0;
			exit = 0;
			backspace = 1;
			menu = 1;
		}
		printf("book 3 %d\n",book3_flag);

	}
}


void Keypad_init(void)
{
	init_GPIO(2,3,1); //GPIO
	init_GPIO(2,4,1);
	init_GPIO(2,5,1);
	init_GPIO(2,6,1);
	init_GPIO_pulldown(0,4,0); //Interrupt
	init_GPIO_pulldown(0,5,0);
	init_GPIO_pulldown(0,10,0);
	init_GPIO_pulldown(0,11,0);
}

void Keypad_loop(void)
{
		for(keypad_row=0;keypad_row<4;keypad_row++)
		{
			if (keypad_row==0)			//4th row
    		{
    			GPIO_ClearValue(2,1<<4);
    			GPIO_ClearValue(2,1<<5);
    			GPIO_ClearValue(2,1<<6);
    			GPIO_SetValue(2,1<<3);
    		}
    		else if (keypad_row==1)		//3rd row
    		{
    			GPIO_ClearValue(2,1<<5);
    			GPIO_ClearValue(2,1<<6);
    			GPIO_ClearValue(2,1<<3);
    			GPIO_SetValue(2,1<<4);
    		}
    		else if (keypad_row==2)		//2nd row
    		{
    			GPIO_ClearValue(2,1<<6);
    			GPIO_ClearValue(2,1<<3);
    			GPIO_ClearValue(2,1<<4);
    			GPIO_SetValue(2,1<<5);
    		}
    		else if (keypad_row==3)		//1st row
    		{
    			GPIO_ClearValue(2,1<<3);
    			GPIO_ClearValue(2,1<<4);
    			GPIO_ClearValue(2,1<<5);
    			GPIO_SetValue(2,1<<6);
    		}
		}
}

void book_num_max_func(int var)
{
	if (var==1)
		book_num_max=book_num_max_new;
	else if (var==2)
		book_num_max=book_num_max_fic;
	else if (var==3)
		book_num_max=book_num_max_non_fic;
}

void book_num_max_func_rev(int var)
{
	if (var==1)
		book_num_max_new=book_num_max;
	else if (var==2)
		book_num_max_fic=book_num_max;
	else if (var==3)
		book_num_max_non_fic=book_num_max;
}

void LCD_busy(void)
{
	Timer0_Wait(1);
}

void LCD_load_data(uint8_t value)
{
	if ((value & (1 << 0)) >> 0)
		GPIO_SetValue(1,1 << LCD_DB0);
	else
		GPIO_ClearValue(1,1 << LCD_DB0);

	if ((value & (1 << 1)) >> 1)
		GPIO_SetValue(1,1 << LCD_DB1);
	else
		GPIO_ClearValue(1,1 << LCD_DB1);

	if ((value & (1 << 2)) >> 2)
		GPIO_SetValue(3,1 << LCD_DB2);
	else
		GPIO_ClearValue(3,1 << LCD_DB2);

	if ((value & (1 << 3)) >> 3)
		GPIO_SetValue(3,1 << LCD_DB3);
	else
		GPIO_ClearValue(3,1 << LCD_DB3);

	if ((value & (1 << 4)) >> 4)
		GPIO_SetValue(4,1 << LCD_DB4);
	else
		GPIO_ClearValue(4,1 << LCD_DB4);

	if ((value & (1 << 5)) >> 5)
		GPIO_SetValue(0,1 << LCD_DB5);
	else
		GPIO_ClearValue(0,1 << LCD_DB5);

	if ((value & (1 << 6)) >> 6)
		GPIO_SetValue(0,1 << LCD_DB6);
	else
		GPIO_ClearValue(0,1 << LCD_DB6);

	if ((value & (1 << 7)) >> 7)
		GPIO_SetValue(2,1 << LCD_DB7);
	else
		GPIO_ClearValue(2,1 << LCD_DB7);
}

void LCD_command(uint8_t var)
{
	LCD_load_data(var);

	GPIO_ClearValue(LCD_Port,1<<LCD_RS);	// RS: Select command register
	GPIO_ClearValue(LCD_Port,1<<LCD_RW);	// RW: Write

	GPIO_SetValue(LCD_Port,1<<LCD_E);
	GPIO_ClearValue(LCD_Port,1<<LCD_E);	// Enable H->L

	LCD_busy();
}

void LCD_init(void)
{
	init_GPIO(1,27,1); //8-bit data
	init_GPIO(1,28,1);
	init_GPIO(3,25,1);
	init_GPIO(3,26,1);
	init_GPIO(4,29,1);
	init_GPIO(0,19,1);
	init_GPIO(0,20,1);
	init_GPIO(2,9,1);

	init_GPIO(LCD_Port,LCD_E,1); 	//E
	init_GPIO(LCD_Port,LCD_RW,1); 	//RW
	init_GPIO(LCD_Port,LCD_RS,1); 	//RS

	LCD_busy();

	LCD_command(0x38);	//8-bit, 2 Line, 5x7 Dots
	LCD_command(0x0c); //Display on, Cursor blinking
	LCD_command(0x01); //Clear screen, cursor home
	LCD_command(0x06);	//Increment cursor to the right when writing, don't shift screen
}

//Used when keying number so that we can keep tracks of the number keyed in
void LCD_char_num(unsigned char var)
{
	LCD_load_data(var);
	char_info[book_num] = var;

	GPIO_SetValue(LCD_Port,1<<LCD_RS);		// RS: Select data register
	GPIO_ClearValue(LCD_Port,1<<LCD_RW);	// RW: Write
	GPIO_SetValue(LCD_Port,1<<LCD_E);
	GPIO_ClearValue(LCD_Port,1<<LCD_E);		// Enable H->L

	LCD_busy();
	book_num++;
}

void LCD_char(unsigned char var)
{
	LCD_load_data(var);

	GPIO_SetValue(LCD_Port,1<<LCD_RS);		// RS: Select data register
	GPIO_ClearValue(LCD_Port,1<<LCD_RW);	// RW: Write
	GPIO_SetValue(LCD_Port,1<<LCD_E);
	GPIO_ClearValue(LCD_Port,1<<LCD_E);		// Enable H->L

	LCD_busy();
}

void LCD_string(unsigned char *var)
{
	while(*var) 				//till string ends
	{
		LCD_char(*var++);		//send characters one by one
	}
}

void LCD_build(int location, int *pointer)
{
	int i;
	if (location<8)
	{
		LCD_command(0x40+(location*8));
		for (i=0;i<8;i++)
			LCD_char(pointer[i]);
	}
}

void display_page(int var)
{
	if (var==1)
	{
		page_1();
	}
	else if (var == 2)
	{
		page_2();
	}
	else if (var ==3)
	{
		page_3();
	}
}

void page_1(void)
{
	int j;
	backspace=0;
	Int_dis();
	printf("435\n");
	LCD_command(0x80);
	LCD_string("1.Borrow");
	LCD_command(0x88);
	LCD_string("2.Return");
	LCD_command(0xc0);
	LCD_string("3.Books borrowed");

	loop1 = 1;
	loop2 = 1;
	end_status1=0;
	keypad_button=0;

	while (loop1==1)
	{
		check_light();
		check_fire();
		Keypad_loop();
		if (keypad_button==1)
		{
			func_num = 1;
			while (loop2==1)
			{
				page_2();
			}
			loop1=0;
		}
		else if (keypad_button==2)
		{
			func_num = 2;
			book_return1();
			loop1=0;
		}
		else if (keypad_button==3)
		{
			func_num=3;
			exit=0;
			menu=0;
			backspace=0;
			num_b=0;
			while ((enter==0)&&(end_status1==0)&&(exit==0))
			{
				Keypad_loop();
				book_borrowed();
				if (backspace==1)
				{
					end_status1=1;
				}
				else if (menu==1)
				{
					end_status1=1;
				}
				else if (exit==1)
				{
					end_status1=1;
					login=0;
				}
			}
			loop1=0;
		}
		else if (menu==1)
		{
			LCD_command(0x01);
			LCD_command(0x80);
			LCD_string("You are already");
			LCD_command(0xc0);
			LCD_string("at the menu page");
			for (j=0;j<10000000;j++);
			loop1=0;
		}
		else if (exit==1)
		{
			loop1=0;
			login=0;
		}
		else if (keypad_button!=0)
		{
			LCD_command(0x01);
			LCD_command(0x80);
			LCD_string("Don't anyhow");
			LCD_command(0xc0);
			LCD_string("press please");
			for (j=0;j<10000000;j++);
			loop1=0;
		}
	}
}

void page_2(void)
{
	int j;
	Int_dis();

	LCD_command(0x01);
	LCD_command(0x80);
	LCD_string("1.New Arrivals");
	LCD_command(0xc0);
	LCD_string("2.Fict.");
	LCD_command(0xc7);
	LCD_string("3.Non-fic");

	for (j=0;j<2000000;j++);

	loop3 = 1;
	keypad_button=0;
    book_num = 0;
	enter=0;
	menu=0;
	exit=0;
	end_status1=0;
	backspace=0;


	while (loop3==1)
	{
		Keypad_loop();
		if (backspace==1)
		{
			loop3=0;
			loop2=0;
			end_status1=1;
			book_cat = 0;
		}
		else if (menu==1)
		{
			loop3=0;
			loop2=0;
			end_status1=1;
			book_cat = 0;
		}
		else if (exit==1)
		{
			loop3=0;
			loop2=0;
			end_status1=1;
			login=0;
			book_cat = 0;
		}
		else if (keypad_button==1)
		{
			book_cat=1;
			loop3=0;
		}
		else if (keypad_button==2)
		{
			book_cat=2;
			loop3=0;
		}
		else if (keypad_button==3)
		{
			book_cat=3;
			loop3=0;
		}
	}

	LCD_command(0x01);
	LCD_command(0x80);
	Int_dis();
	if ((backspace==0)&&(menu==0)&&(exit==0))
	{
		LCD_string("Please choose");
		for (j=0;j<6000000;j++);
	}
	LCD_command(0x01);
	LCD_command(0x80);
	backspace=0;
	keypad_button=0;

	book_num_max_func(book_cat);
	printf("book num max %d\n",book_num_max);
	printf("book cat %d\n",book_cat);
	while ((backspace==0)&&(end_status1==0))
	{
		num=0;
		enter = 0;
	while ((enter==0)&&(end_status1==0)&&(exit==0))
	{
		Keypad_loop();
		book_borrow();
		if (backspace==1)
		{
			loop2=1;
			end_status1=1;
		}
		else if (menu==1)
		{
			end_status1=1;
			loop2=0;
			loop1=0;
		}
		else if (exit==1)
		{
			end_status1=1;
			loop2=0;
			loop1=0;
			login=0;
		}
	}
	if ((exit==0)&&(end_status1==0))
	{
		book_chosen1();
		num_borrowed++;
		int num1=num+1;
		book_data[book_cat][num1]=0;
		loop2=1;
		loop1=1;
	}
	}

	book_num_max_func_rev(book_cat);

	end_status1=0;

	Int_dis();
}

void Keypad_char(int var)
{
	int j;
	switch (var)
	{
		case 1:
			LCD_char_num('1');
			break;
		case 2:
//			LCD_char_num('2');
			break;
		case 3:
			LCD_char_num('3');
			break;
		case 4:
			LCD_char_num('4');
			break;
		case 5:
//			LCD_char_num('5');
			if (func_num==1)
				num_minus();
			else if (func_num==3)
				num_borrowed_minus();
			break;
		case 6:
			LCD_char_num('6');
			break;
		case 7:
			LCD_char_num('7');
			break;
		case 8:
//			LCD_char_num('8');
			if ((end_status==0)&&(func_num==1))
				num++;
			else if ((end_status==0)&&(func_num==3))
				num_b++;
			break;
		case 9:
			LCD_char_num('9');
			break;
		case 10:
//			LCD_char_num('0');
			break;
		case 11:					//Enter
			enter = 1;
			break;
		case 12:					//Back to previous page
			backspace=1;
			break;
		case 13:					//Menu
			menu=1;
			break;
		case 14:					//Exit
			exit=1;
			break;
		case 15:
			LCD_char('*');
			break;
		case 16:
			LCD_char('#');
			break;
	}
	return;
}

void num_minus(void)
{
	int j;
	int i;
	for (i=num;i>0;i--)
	{
		if (book_data[book_cat][i]==1)
		{
			num = i - 1;
			i = 0;
		}
	}
}

void num_borrowed_minus(void)
{
	int i,j;
	for (i=num_b-1;i>=0;i--)
	{
		borrow_init(i);
		if (book_data[s1][s2]==0)
		{
			num_b = i;
			i=0;
		}
	}
}


//Interrupt disable
void Int_dis(void)
{
	GPIO_ClearValue(2,1<<6);
	GPIO_ClearValue(2,1<<5);
	GPIO_ClearValue(2,1<<4);
	GPIO_ClearValue(2,1<<3);
}

void EINT3_IRQHandler(void)
{
	if ((LPC_GPIOINT->IO0IntStatF>>11)&0x1)		//1st column
	{
		Int_dis();
		if (keypad_row==1)				//4th row
			keypad_button=15;			// "*"
		else if (keypad_row==2)			//3rd row
			keypad_button=7;
		else if (keypad_row==3)			//2nd row
			keypad_button = 4;
		else if (keypad_row==0)			//1st row
			keypad_button = 1;
		printf("keypad_button %d\n",keypad_button);

		for (delay=0;delay<1000000;delay++);
		LPC_GPIOINT->IO0IntClr = 1<<11;
	}

	if ((LPC_GPIOINT->IO0IntStatF>>10)&0x1)		//2nd column
	{
		Int_dis();
		if (keypad_row==1)				//4th row
			keypad_button=10;			// "0"
		else if (keypad_row==2)			//3rd row
			keypad_button=8;
		else if (keypad_row==3)			//2nd row
			keypad_button=5;
		else if (keypad_row==0)			//1st row
			keypad_button = 2;

		Keypad_char(keypad_button);
		printf("keypad_button %d\n",keypad_button);
		for (delay=0;delay<1000000;delay++);

		LPC_GPIOINT->IO0IntClr = 1<<10;			//clear interrupt
	}

	if ((LPC_GPIOINT->IO0IntStatF>>5)&0x1)		//3rd column
	{
		Int_dis();
		if (keypad_row==1)				//4th row
			keypad_button=16;			// "#"
		else if (keypad_row==2)			//3rd row
			keypad_button = 9;
		else if (keypad_row==3)			//2nd row
			keypad_button = 6;
		else if (keypad_row==0)			//1st row
			keypad_button = 3;
		printf("keypad_button %d\n",keypad_button);
		for (delay=0;delay<1000000;delay++);

		LPC_GPIOINT->IO0IntClr = 1<<5;			//clear interrupt
	}

	if ((LPC_GPIOINT->IO0IntStatF>>4)&0x1)		//4th column
	{
		Int_dis();
		if (keypad_row==1)				//4th row
			keypad_button=14;			// "D"
		else if (keypad_row==2)			//3rd row
			keypad_button=13;			// "C"
		else if (keypad_row==3)			//2nd row
			keypad_button=12;			// "B"
		else if (keypad_row==0)			//1st row
			keypad_button=11;			// "A"

		Keypad_char(keypad_button);
		printf("keypad_button %d\n",keypad_button);
		for (delay=0;delay<1000000;delay++);

		LPC_GPIOINT->IO0IntClr = 1<<4;			//clear interrupt
	}

	if((LPC_GPIOINT->IO2IntStatF>>12) & 0x1){	//clear alarm
		alarm = 0;
		//clear GPIO Interrupt 2.12
		LPC_GPIOINT->IO2IntClr = 1<<12;
	}

	if((LPC_GPIOINT->IO0IntStatF>>9) & 0x1)
	{
		int j;
		ir_num--;
		if (ir_num<0)
			ir_num = 9;
		printf("IR int in\n");
		seven_display(ir_num);
		LPC_GPIOINT->IO0IntClr = 1<<9;
		for (j=0;j<1000000;j++);
	}

	if((LPC_GPIOINT->IO0IntStatF>>8) & 0x1)
	{
		int j;
		ir_num++;
		if (ir_num==10)
			ir_num = 0;
		printf("IR int in\n");
		seven_display(ir_num);
		LPC_GPIOINT->IO0IntClr = 1<<8;
		for (j=0;j<1000000;j++);
	}
	Int_dis(); //Disable keypad
}

void display_book(int cat,int var)
{
	if (cat==3)
	{
		switch (var)
		{
			case 1:
				LCD_string("Black holes    ");
				break;
			case 2:
				LCD_string("Quantumized    ");
				break;
			case 3:
				LCD_string("Time travel    ");
				break;
		}
	}

	else if (cat==1)
	{
		switch (var)
		{
			case 1:
				LCD_string("Abstract City    ");
				break;
			case 2:
				LCD_string("Shangri-La       ");
				break;
			case 3:
				LCD_string("$ - The new God  ");
				break;
		}
	}
	else if (cat==2)
	{
		switch (var)
		{
			case 1:
				LCD_string("Harry Potter     ");
				break;
			case 2:
				LCD_string("Hunger Games     ");
				break;
			case 3:
				LCD_string("Twilight         ");
				break;
		}
	}

}

void borrow_init(int var)
{
	switch (var)
	{
		case 0:
			s1=1;
			s2=1;
			break;
		case 1:
			s1=1;
			s2=2;
			break;
		case 2:
			s1=1;
			s2=3;
			break;
		case 3:
			s1=2;
			s2=1;
			break;
		case 4:
			s1=2;
			s2=2;
			break;
		case 5:
			s1=2;
			s2=3;
			break;
		case 6:
			s1=3;
			s2=1;
			break;
		case 7:
			s1=3;
			s2=2;
			break;
		case 8:
			s1=3;
			s2=3;
			break;

	}
}
void book_borrowed(void)
{
	end_status = 0;
	int buffer;
	int j;
	if (num_b>=9)
		num_b=8;
	if (num_b<0)
		num_b=0;

	end_status1=0;

		for (buffer=num_b;buffer<9;buffer++)
		{
			borrow_init(buffer);
			if (book_data[s1][s2]==0)
			{
				LCD_command(0x80);
				display_book(s1,s2);
				LCD_string("  ");
				end_status1 = 0;
				num_b = buffer;
				buffer = 9;
			}
			else
			{
				end_status1=1;
			}
		}
//	printf("end %d\n",end_status1);
	if (end_status1==1)
	{
		LCD_command(0x80);
		LCD_string("There is no           ");
		LCD_command(0xc0);
		LCD_string("book borrowed         ");
		for (j=0;j<20000000;j++);
	}
	else
	{
			end_status=1;

			for (buffer=num_b+1;buffer<9;buffer++)
				{
					borrow_init(buffer);
					if (book_data[s1][s2]==0)
					{
						LCD_command(0xc0);
						display_book(s1,s2);
						LCD_string("  ");
						buffer = 9;
						end_status=0;
					}
				}

		if (end_status==1)
			{
				char c[10];
				LCD_command(0xc0);
				LCD_string("Total: ");
				sprintf(c,"%d",num_borrowed);
				LCD_string(c);
				if (num_borrowed==1)
					LCD_string(" book        ");
				else
					LCD_string(" books      ");
				buffer = 9;
			}
	}

}
void book_borrow(void)
{
	int j;	//counting variables
	int i,i1;
	end_status=0;

		if (num<0)
			num=0;
		if (num==book_num_max)
			num=book_num_max-1;
		if (num>book_num_max)
			num=book_num_max-1;

		for (j=num;j<book_num_max;j++)
		{
			i = j+1;
			if (book_data[book_cat][i]==1)
			{
				LCD_command(0x80);
				display_book(book_cat,i);
				end_status1 = 0;
				j = book_num_max;
				num = i-1;
			}
			else
			{
				end_status1=1;
			}
		}

		if (end_status1==1)
		{
			LCD_command(0x80);
			LCD_string("There is no       ");
			LCD_command(0xc0);
			LCD_string("book left         ");
			for (j=0;j<10000000;j++);
			num =0;
			end_status1=1;
		}
		else
		{
			LCD_command(0x8f);
			LCD_char('<');


			for (j=i;j<=book_num_max;j++)
			{
				i1 = j+1;
				if (book_data[book_cat][i1]==1)
				{
					LCD_command(0xc0);
					display_book(book_cat,i1);
					j = book_num_max+1;
					end_status=0;
				}
				else
					end_status=1;
			}
			if (end_status==1)
			{
				LCD_command(0xc0);
				LCD_string("End of list      ");
				j=book_num_max+1;
			}
		}
	LCD_command(0x90);
}

void book_chosen1(void)
{
	LCD_command(0x01);
	int i;
	int j;
	int a;
	i = num+1;
	LCD_command(0x80);
	LCD_string("You've chosen");
	LCD_command(0xc0);
	display_book(book_cat,i);
	book_rec = book_cat;

	for (j=0;j<10000000;j++);

}

void book_return1(void)
{
	int j;
	printf("1097\n");
	LCD_command(0x01);
	LCD_command(0x80);
	LCD_string("Pls scan book");
	book1_flag = 0;
	book2_flag = 0;
	book3_flag = 0;


	while ((book1_flag == 0)&&(book2_flag==0)&&(book3_flag==0)&&(exit==0)&&(backspace==0)&&(menu==0))
	{
		printf("1108\n");
		Keypad_loop();
		read_RFID();
		GPIO_ClearValue(0,1<<23);

//	    NVIC_EnableIRQ (UART1_IRQn);


	}
	GPIO_SetValue(0,1<<23);
	LCD_command(0x01);
	if (book1_flag==1)
	{
		LCD_command(0x01);
		LCD_command(0x80);
		LCD_string("You've returned");
		LCD_command(0xc0);
		display_book(2,1);
		book_data[2][1]=1;
		for (j=0;j<10000000;j++);
		num_borrowed--;
	}
	else if (book2_flag==1)
	{
		LCD_command(0x01);
		LCD_command(0x80);
		LCD_string("You've returned");
		LCD_command(0xc0);
		display_book(2,2);
		book_data[2][2]=1;
		for (j=0;j<10000000;j++);
		num_borrowed--;
	}
	else if (book3_flag==1)
	{
		LCD_command(0x01);
		LCD_command(0x80);
		LCD_string("You've returned");
		LCD_command(0xc0);
		display_book(2,3);
		book_data[2][3]=1;
		for (j=0;j<10000000;j++);
		num_borrowed--;
	}
}

void book_recommend(void)
{
	int rec=1;
	int i,j;
	int book;
	while (rec==1)
	{
		keypad_button = 0;
		for (i=1;i<4;i++)
		{
			if ((book_data[book_rec][i]==1)&&(book_rec!=0))
			{
				book = i;
				LCD_command(0x80);
				LCD_string("Would you like   ");
				LCD_command(0xc0);
				LCD_string("to borrow        ");
				for (j=0;j<10000000;j++);
				LCD_command(0x01);
				LCD_command(0x80);
				display_book(book_rec,book);
				LCD_command(0xcf);
				LCD_char('?');
				for (j=0;j<10000000;j++);
				i = 4;
				rec = 0;
			}
		}
		while (keypad_button==0)
			Keypad_loop();
		if (keypad_button == 6)
		{
			Int_dis();
			keypad_button = 0;
			LCD_command(0x80);
			LCD_string("You've chosen     ");
			LCD_command(0xc0);
			display_book(book_rec,book);
			book_data[book_rec][book]=0;
			for (j=0;j<10000000;j++);
			LCD_command(0x80);
			LCD_string("Would you like   ");
			LCD_command(0xc0);
			LCD_string("to continue?     ");
			while (keypad_button==0)
						Keypad_loop();
			if (keypad_button==6)
			{
				rec = 0;
				login = 1;
			}
			else if (keypad_button ==9)
			{
				rec = 0;
				login = 0;
			}
			keypad_button = 0;

		}
		else if (keypad_button == 9)
		{
			rec = 0;
			keypad_button = 0;
		}
	}
}

int main(void)
{
	//Initialization
	Keypad_init();
	UART1_Init();	//uart17
	LCD_init();
	init_ADC();		//temp
	init_7segment();
	init_LDR();
	init_GPIO(0,9,0);	//IR in
	init_GPIO(0,8,0);	//IR out
	init_GPIO(2,11,1); //Speakers
	init_LED();

    /* Enable GPIO Clock */
    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPIO, ENABLE);
    //right
    init_GPIO(1,26,1);
    init_GPIO(1,25,1);
    //left
    init_GPIO(1,20,1);
    init_GPIO(1,29,1);
    //arm
    init_GPIO(1,22,1);
    init_GPIO(1,23,1);

	ADC_Init(LPC_ADC, 200000);
	ADC_BurstCmd(LPC_ADC, ADC_START_NOW);
	ADC_ChannelCmd (LPC_ADC, 1, ENABLE);
	ADC_BurstCmd(LPC_ADC, ADC_START_NOW);
	ADC_ChannelCmd (LPC_ADC, 2, ENABLE);

	SysTick_Config(SystemCoreClock / 1000) ;



	LCD_command(0x38);

	//Variables declaration
	int j,i;
	int wait_time;
	char count[20];
	int page_loop;
	//Book database
	int row;
	char data;
	for (book_cat=1;book_cat<=3;book_cat++)
		for (row=0;row<=book_num_max;row++)
		{
			book_data[book_cat][row]=1;
		}

	//Building customized characters
	int bell[8]={0x04,0x0E,0x0E,0x0E,0x1F,0x00,0x04,0x00};
	int smile[8]={0x00,0x00,0xa,0x00,0x11,0xe,0x00,0x00};
	int degree[8]={0x07,0x04,0x07,0x0,0x0,0x0,0x0,0x0};
	int wheel_1[8]={0x00,0x04,0x04,0x1f,0x04,0x04,0x00,0x00};
	int wheel_2[8]={0x00,0x01,0x12,0x04,0x09,0x10,0x00,0x00};
	int wheel_3[8]={0x00,0x010,0x9,0x04,0x12,0x1,0x00,0x00};
	int num_ppl;

	LCD_build(0,degree);
	LCD_build(1,wheel_1);
	LCD_build(2,wheel_2);
	LCD_build(3,wheel_3);

/*	int wheel;

	//Displaying Loading screen
	LCD_command(0x01);
	for(j=0;j<7;j++)
	{
		LCD_command(0x01);
		LCD_command(0x80);
		LCD_string("Loading...");
		LCD_command(0xc7);
		LCD_char(0x01);
		LCD_char(0x01);
		LCD_char(0x01);
		for(wheel=0;wheel<2000000;wheel++);
		LCD_command(0xc7);
		LCD_char(0x02);
		LCD_char(0x02);
		LCD_char(0x02);
		for(wheel=0;wheel<2000000;wheel++);
		LCD_command(0xc7);
		LCD_char(0x03);
		LCD_char(0x03);
		LCD_char(0x03);
		for(wheel=0;wheel<2000000;wheel++);
	}
*/
	LCD_command(0x01);

	LCD_command(0x80);



	//Enabling interrupts
	LPC_GPIOINT->IO0IntEnF |=1<<4;  //keypad interrupt
	LPC_GPIOINT->IO0IntEnF |=1<<5;	//
	LPC_GPIOINT->IO0IntEnF |=1<<10;	//
	LPC_GPIOINT->IO0IntEnF |=1<<11;	//

	LPC_GPIOINT->IO0IntEnF |=1<<9;	//ir in
	LPC_GPIOINT->IO0IntEnF |=1<<8;	//ir out

	LPC_GPIOINT->IO2IntEnF |= 1<<12; //push button to disable alarm
    NVIC_EnableIRQ (EINT3_IRQn);

    seven_display(9);

    while (1)
    {
    	login = 1;
    	exit = 0;
    	id_flag = 0;
    	LCD_command(0x01);
    	LCD_command(0x80);
		LCD_string("Pls Scan Card");
		for (j=0;j<1000000;j++);
		check_fire();
		check_light();

		while(id_flag == 0)
    	{
    		GPIO_ClearValue(0,1<<23);  //enable RFID (low is enable)
    		read_RFID_1();
    	}
    	GPIO_SetValue(0,1<<23);

    	printf("1323\n");
    	if (id_flag==1)
    	{
    		LCD_command(0x01);
    		LCD_command(0x80);
    		LCD_string("Welcome Alice Tan");
    		for (j=0;j<10000000;j++);
    	}
    	if (book_rec!=0)
    		book_recommend();

    	while (login==1)
    	{
    		menu=0;
    		loop=1;
			page_loop=1;
			page_num = 1;
			LCD_command(0x01);
			LCD_command(0x80);
			page_1();
    	}
    }
	LCD_busy();

	return 0;
}
