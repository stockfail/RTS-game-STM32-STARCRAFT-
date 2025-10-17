#include "device_driver.h"

#define TIM2_TICK			(20) 					// usec
#define TIM2_FREQ			(1000000/TIM2_TICK) 	// Hz
#define TIME2_PLS_OF_1ms	(1000/TIM2_TICK)
#define TIM2_MAX			(0xffffu)
#define TIM3_FREQ 	  		(8000000) 	      	// Hz
#define TIM3_TICK	  		(1000000/TIM3_FREQ)	// usec
#define TIME3_PLS_OF_1ms  	(1000/TIM3_TICK)

void TIM2_Delay(int time)
{
	Macro_Set_Bit(RCC->APB1ENR, 0);

	TIM2->CR1 = (1<<4)|(1<<3);
	TIM2->PSC = (unsigned int)(TIMXCLK/(double)TIM2_FREQ + 0.5)-1;
	TIM2->ARR = TIME2_PLS_OF_1ms * time;

	Macro_Set_Bit(TIM2->EGR,0);
	Macro_Clear_Bit(TIM2->SR, 0);
	Macro_Set_Bit(TIM2->DIER, 0);
	Macro_Set_Bit(TIM2->CR1, 0);
}

int TIM2_Check_Timeout(void)
{
	if(Macro_Check_Bit_Set(TIM2->SR, 0))
	{
		Macro_Clear_Bit(TIM2->SR, 0);
		return 1;
	}
	else
	{
		return 0;
	}
}

void TIM3_Out_Freq_Generation(unsigned short freq)
{
	TIM3->CR1 = (1<<4)|(0<<3);
	// Timer 주파수가 TIM3_FREQ가 되도록 PSC 설정
	TIM3->PSC= TIMXCLK/TIM3_FREQ -1;
	// 요청한 주파수가 되도록 ARR 설정
	TIM3->ARR= TIM3_FREQ/freq;
	// Duty Rate 50%가 되도록 CCR3 설정
	TIM3->CCR3= TIM3->ARR /2;
	// Manual Update(UG 발생)
	Macro_Set_Bit(TIM3->EGR,0);
	// Down Counter, Repeat Mode, Timer Start
	Macro_Set_Bit(TIM3->CR1, 0);
}

void TIM3_Out_Stop(void)
{
	Macro_Clear_Bit(TIM3->CR1, 0);
	Macro_Clear_Bit(TIM3->DIER, 0);
}

void TIM3_Out_Init(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 1);
	Macro_Set_Bit(RCC->APB2ENR, 3);
	Macro_Write_Block(GPIOB->CRL,0xf,0xb,0);
	Macro_Write_Block(TIM3->CCMR2,0x7,0x6,4);
	TIM3->CCER = (0<<9)|(1<<8);
}


#define TIM4_TICK	  (20) 							// usec
#define TIM4_FREQ 	  (1000000/TIM4_TICK) 			// Hz
#define TIME4_PLS_OF_1ms  (1000/TIM4_TICK)
#define TIM4_MAX	  (0xffffu)

void TIM4_Repeat_Interrupt_Enable(int en, int time)
{
  if(en)
  {
    Macro_Set_Bit(RCC->APB1ENR, 2);

    TIM4->CR1 = (1<<4)+(0<<3)+(0<<0);
    TIM4->PSC = (unsigned int)(TIMXCLK/(double)TIM4_FREQ + 0.5)-1;
    TIM4->ARR = TIME4_PLS_OF_1ms * time;

    Macro_Set_Bit(TIM4->EGR,0);
    Macro_Set_Bit(TIM4->SR, 0);
    NVIC_ClearPendingIRQ((IRQn_Type)30);
    Macro_Set_Bit(TIM4->DIER, 0);
    NVIC_EnableIRQ((IRQn_Type)30);
    Macro_Set_Bit(TIM4->CR1, 0);
  }

  else
  {
    NVIC_DisableIRQ((IRQn_Type)30);
    Macro_Clear_Bit(TIM4->CR1, 0);
    Macro_Clear_Bit(TIM4->DIER, 0);
    Macro_Clear_Bit(RCC->APB1ENR, 2);
  }
}

#define BASE  (500) //msec
static volatile int valid = 0;
static void Buzzer_Beep(unsigned char tone, int duration)
{
	const static unsigned short tone_value[] = {261,277,293,311,329,349,369,391,415,440,466,493,523,554,587,622,659,698,739,783,830,880,932,987};

	TIM3_Out_Freq_Generation(tone_value[tone]);
	TIM2_Delay(duration);
}

int Music(int i)
{
	enum key{C1, C1_, D1, D1_, E1, F1, F1_, G1, G1_, A1, A1_, B1, C2, C2_, D2, D2_, E2, F2, F2_, G2, G2_, A2, A2_, B2};

	enum note{N16=BASE/4, N8=BASE/2, N4=BASE, N2=BASE*2, N1=BASE*4};
	//const int song1[][2] = {{G1,N4},{G1,N4},{E1,N8},{F1,N8},{G1,N4},{A1,N4},{A1,N4},{G1,N2},{G1,N4},{C2,N4},{E2,N4},{D2,N8},{C2,N8},{D2,N2}};
	const int song1[][2] = {
		{G1, N8}, {D2, N8}, {G2, N8}, {F2, N8}, {D2, N8}, {C2, N8},
		{A1, N8}, {D2, N8}, {G1, N8}, {B1, N8},
		{C2, N4}, {C2, N4},
		{G1, N8}, {A1, N8}, {F1, N4}, {G1, N4},
		{D1, N8}, {G1, N8}, {A1, N8}, {D2, N8},
		{G1, N4}, {G1, N8}, {F1, N8}, {E1, N4}
	};
	const char * note_name[] = {"C1", "C1#", "D1", "D1#", "E1", "F1", "F1#", "G1", "G1#", "A1", "A1#", "B1", "C2", "C2#", "D2", "D2#", "E2", "F2", "F2#", "G2", "G2#", "A2", "A2#", "B2"};
	int song_len = sizeof(song1) / sizeof(song1[0]);

	if(TIM2_Check_Timeout()){
		valid = 0;
		if(i<song_len) i++;
		else i = 0;
	}	

	if(valid == 0){
		Buzzer_Beep(song1[i][0], song1[i][1]);
		valid = 1;
	}
	return i;
}
