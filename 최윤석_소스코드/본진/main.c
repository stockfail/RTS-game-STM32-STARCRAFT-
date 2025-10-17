#include "device_driver.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define LCDW			(320)
#define LCDH			(240)
#define X_MIN	 		(0)
#define X_MAX	 		(LCDW - 1)
#define Y_MIN	 		(0)
#define Y_MAX	 		(LCDH - 1)
#define TIMER_PERIOD	(1000) // 타이머 주기 (ms)

#define BACK_COLOR		(5)
#define TERRAN_COLOR	(3)

typedef enum {
    MODE_NULL,
    MODE_COMMAND,
    MODE_BARRACKS,
    MODE_FACTORY
} GameMode;

typedef struct {
    int x, y; // X, Y 좌표
    int w, h; // 가로, 세로 크기
    int ci; // 색상 인덱스
    int exists; // 활성화 상태 (0: 비활성화, 1: 활성화)
} Terran_Building;

static Terran_Building CommandCenter;
static Terran_Building Barracks;
static Terran_Building Factory;
static int SCV=4;
static int Mineral=0;
static int Mineral_Count=0;
static int Selected_Building = -1;
static GameMode current_mode = MODE_COMMAND;

static int i; // 반복문
static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK}; // 색상 배열

void Init_Buildings(void) {
    CommandCenter = (Terran_Building){10, 119, 60, 60, TERRAN_COLOR, 1};
    Barracks = (Terran_Building){80, 129, 50, 50, TERRAN_COLOR, 0};
    Factory = (Terran_Building){140, 129, 50, 50, TERRAN_COLOR, 0};
}

void Build_Barracks(void) {
    if (!Barracks.exists && Mineral >= 150) {
        Barracks.exists = 1;
        Mineral -= 150;
    }
}
void Build_Factory(void) {
    if (!Factory.exists && Mineral >= 300) {
        Factory.exists = 1;
        Mineral -= 300;
    }
}
//본진 송신
void Send_Spawn_Command(const char* unit_type)
{
    char buffer[32];
    sprintf(buffer, "SPAWN_%s\n", unit_type);  // 예: SPAWN_MAARINE
    for ( i = 0; buffer[i]; i++) {
        while (!Macro_Check_Bit_Set(USART2->SR, 7)); // 송신 준비 대기
        USART2->DR = buffer[i];
    }
}

static void Mining_Mineral(void)
{
    Mineral_Count += 1;
    if(Mineral_Count >= 4) {
        Mineral += (5*SCV); // 자원 증가
        Mineral_Count = 0;
    }
}
static void Create_SCV(void)
{
    if(Mineral >=50){
    SCV += 1; // SCV 생성
    Mineral -= 50; // 자원 감소
    }
}
static void Create_Marine(void)
{
    if(Mineral >=50){
    Send_Spawn_Command("MARINE"); // 본진에 유닛 생성 명령 전송   
    Mineral -= 50; // 자원 감소
    }
}

static void k0(void)
{   
    if(current_mode == MODE_NULL){
        current_mode = MODE_COMMAND;
    }
}

static void k1(void)
{
    if(current_mode == MODE_NULL){
        current_mode = MODE_BARRACKS;
    }
    if(current_mode == MODE_BARRACKS){
        Build_Barracks(); // 본진에 건물 생성 명령 전송
    }
}

static void k2(void)
{       
      if(current_mode == MODE_NULL){
        current_mode = MODE_FACTORY;
      }
        if(current_mode == MODE_FACTORY){
            Build_Factory(); // 본진에 건물 생성 명령 전송
        }
}

static void k3(void)
{
    if(current_mode == MODE_NULL){
        current_mode = MODE_COMMAND;
    }   
}

static void k4(void)
{
    if(current_mode == MODE_COMMAND)
    {
        Create_SCV(); // SCV 생성
    }
   if(current_mode == MODE_BARRACKS){
        Create_Marine(); // 마린 생성
    }
}

static void k5(void)
{
    current_mode = MODE_NULL;
}

static void User_Command(int key)
{
    // UP(0), DOWN(1), LEFT(2), RIGHT(3)
    static void (*key_func[])(void) = {k0, k1, k2, k3, k4, k5};
    if(key <= 5) key_func[key]();
}

static void Draw_Object(Terran_Building * obj)
{
    if(obj->exists)
    Lcd_Draw_Box(obj->x, obj->y, obj->w, obj->h, color[obj->ci]);
}


static void Game_Init(void)
{
	SCV=4;
    Mineral=0;
    Mineral_Count = 0;
    current_mode = MODE_COMMAND;
	Lcd_Clr_Screen();
    Init_Buildings();
}

extern volatile int TIM4_expired;
extern volatile int USART1_rx_ready;
extern volatile int USART1_rx_data;
extern volatile int Jog_key_in;
extern volatile int Jog_key;
extern volatile int Uart2_Rx_In;
extern volatile int Uart2_Rx_Command_Ready;
extern volatile char uart2_rx_buffer[32];

void System_Init(void)
{
	Clock_Init();
	LED_Init();
	Key_Poll_Init();
	Uart1_Init(115200);
    Uart2_Init(115200);

	SCB->VTOR = 0x08003000;
	SCB->SHCSR = 7<<16;
}

#define DIPLAY_MODE		3
const char* Mode_Str[] = {"NULL", "COMMAND", "BARRACKS", "FACTORY"};
void Main(void)
{
    static volatile int music_cnt = 0;
	System_Init();
	Uart_Printf("STARCAFT-Basecamp\n");

	Lcd_Init(DIPLAY_MODE);

	Jog_Poll_Init();
	Jog_ISR_Enable(1);
	Uart1_RX_Interrupt_Enable(1);
    Uart2_RX_TX_Interrupt_Enable(1);

	for(;;)
	{
		Game_Init();
		TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD);
        TIM3_Out_Init();

		for(;;)
		{
            music_cnt = Music(music_cnt);
            Lcd_Printf(0,0,BLUE,WHITE,2,2,"%d", SCV);
            Lcd_Printf(40,0,BLUE,WHITE,2,2,"%d", Mineral);
            Lcd_Printf(100,0,BLUE,WHITE,2,2,"%s", Mode_Str[current_mode]);

            Draw_Object(&CommandCenter);
            Draw_Object(&Barracks);
            Draw_Object(&Factory);

			if (Jog_key_in) {
                User_Command(Jog_key);
                Jog_key_in = 0;
            }

            if (TIM4_expired) {
                Mining_Mineral(); // 자원 채취
                TIM4_expired = 0;
            }

		}
	}
}