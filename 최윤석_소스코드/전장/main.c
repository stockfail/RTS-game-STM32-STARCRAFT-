#include "device_driver.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_MODE 1 // 디버깅 모드 활성화 (1: 활성화, 0: 비활성화)

#if DEBUG_MODE
    #define DEBUG_PRINT(fmt, ...) Uart_Printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

#define CAMERA_WIDTH   4  // 카메라 뷰의 가로 비율
#define CAMERA_HEIGHT  3   // 카메라 뷰의 세로 비율
#define MAP_WIDTH      1000  // 맵의 가로 크기
#define MAP_HEIGHT     1000  // 맵의 세로 크기
#define LCDW			(320) //카메라 해상도
#define LCDH			(240) //카메라 해상도
#define X_MIN	 		(0) // 맵의 최소 X 좌표
#define X_MAX	 		(MAP_WIDTH - 1) // 맵의 최대 X 좌표
#define Y_MIN	 		(0) // 맵의 최소 Y 좌표
#define Y_MAX	 		(MAP_HEIGHT - 1) // 맵의 최대 Y 좌표
#define Respawn_X		(490) // 리스폰 X 좌표
#define Respawn_Y		(850) // 리스폰 Y 좌표

#define TIMER_PERIOD	(10) // 타이머 주기 (ms)
#define RIGHT			(1) // 오른쪽 방향
#define LEFT			(-1) // 왼쪽 방향
#define UP				(2) // 위쪽 방향
#define DOWN			(-2) // 아래쪽 방향

#define Marine_STEP		(10) // 마린 이동 속도
#define Marine_SIZE_X	(20) // 마린 가로 크기
#define Marine_SIZE_Y	(20) // 마린 세로 크기
#define Marine_damage 	(10) // 마린 공격력
#define Marine_hp		(40) // 마린 체력
#define Marine_attack_range (80) // 마린 공격 범위
#define Marine_attack_speed (100) // 마린 공격 속도 100ms 단위

#define Zergling_STEP		(5) // 저글링 이동 속도
#define Zergling_SIZE_X		(20) // 저글링 가로 크기
#define Zergling_SIZE_Y		(20) // 저글링 세로 크기
#define Zergling_damage 	(5) // 저글링 공격력
#define Zergling_hp			(35) // 저글링 체력
#define Zergling_attack_range (10) // 저글링 공격 범위

#define Hydralisk_STEP		(5) // 히드라 이동 속도
#define Hydralisk_SIZE_X	(20) // 히드라 가로 크기
#define Hydralisk_SIZE_Y	(40) // 히드라 세로 크기
#define Hydralisk_damage 	(10) // 히드라 공격력
#define Hydralisk_hp		(80) // 히드라 체력
#define Hydralisk_attack_range (80) // 히드라 공격 범위


#define Ultralisk_STEP		(5) // 울트라 이동 속도
#define Ultralisk_SIZE_X	(40) // 울트라 가로 크기
#define Ultralisk_SIZE_Y	(40) // 울트라 세로 크기
#define Ultralisk_damage 	(20) // 울트라 공격력
#define Ultralisk_hp		(200) // 울트라 체력
#define Ultralisk_attack_range (10) // 울트라 공격 범위

#define HERO_STEP		(20) // 히어로 이동 속도
#define HERO_SIZE_X		(20) // 히어로 가로 크기
#define HERO_SIZE_Y		(20) // 히어로 세로 크기

#define zergling_count  (sizeof(zergling) / sizeof(zergling[0]))
#define hydralisk_count (sizeof(hydralisk) / sizeof(hydralisk[0]))
#define ultralisk_count (sizeof(ultralisk) / sizeof(ultralisk[0]))
#define marine_count    (sizeof(marine) / sizeof(marine[0]))

#define BACK_COLOR		(5) // 배경 색상
#define ZERG_COLOR		(0) // 저글 색상
#define HERO_COLOR		(3) // 히어로 색상
#define MARINE_COLOR	(2) // 마린 색상    
#define BULLET_COLOR	(4) // 총알 색상

#define MAX_BULLETS    (30)
#define BULLET_SPEED   (15)
#define BULLET_SIZE    (5)

typedef enum {
    ZERGLING,
   	HYDRALISK,
    ULTRALISK,
   	HERO,
    TERRAN,
    ZERG,
	TYPE_COUNT // 유닛 타입의 총 개수
} UnitType;

typedef enum {
   mode_stop, // 정지 모드
   mode_move, // 이동 모드
   mode_attack, // 공격 모드
   mode_count // 모드의 총 개수
} Mode;


typedef struct unit
{
	int x,y; // X, Y 좌표
	int w,h; // 가로,세로 크기
	int step; // 이동 속도
	int hp; // 채력
	int attack_range; //공격범위
	int attack_damage; // 공격력
    int attack_speed; // 공격 속도 100ms 단위
    int ci;            // 색상
	int active; // 활성화 상태
	UnitType UnitType;    // 유닛 타입
    Mode mode; // 모드 (정지, 이동, 공격 등)
    void (*move)( struct unit *self); // 이동 함수
    void (*attack)(struct unit *attacker, struct unit *target); // 공격 함수
    void (*draw)(struct unit *self); // 그리기 함수
}UNIT;

typedef struct {
    int x, y; // 총알의 X, Y 좌표
    int w, h; // 총알의 가로, 세로 크기
    int dx, dy; // 총알의 이동 방향
    int damage; // 총알의 피해량
    int active; // 총알의 활성화 상태 (0: 비활성화, 1: 활성화)
    int owner; // 총알의 소유자 (HERO, HYDRALISK 등)
    UNIT* target;  // 명중 대상 유닛
} BULLET;

static UNIT hero;
//static UNIT Terran[20] ; // 테란 배열
static UNIT marine[8]; // 마린 배열
static UNIT zergling[10]; // 저글링 배열
static UNIT hydralisk[10]; // 히드라 배열
static UNIT ultralisk[3]; // 울트라 배열
static BULLET bullets[MAX_BULLETS]; // 총알 배열
static int camera_x = 0; // 카메라의 초기 X 좌표
static int camera_y = 0; // 카메라의 초기 Y 좌표
static int screen_x = 0; // 화면의 X 좌표(320 X 240)
static int screen_y = 0; // 화면의 Y 좌표(320 X 240)
static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK}; // 색상 배열




int Check_Collision_Box(UNIT *self, UNIT *other) {
    return (self->x < other->x + other->w && self->x + self->w > other->x &&
            self->y < other->y + other->h && self->y + self->h > other->y);
}

int Check_Bullet_Hit(BULLET *b, UNIT *target) {
    return (b->x < target->x + target->w && b->x + BULLET_SIZE > target->x &&
            b->y < target->y + target->h && b->y + BULLET_SIZE > target->y);
}

void Avoid_Collision(UNIT *self, UNIT *other){
   
    if (Check_Collision_Box(self, other)) {
           // 충돌 회피: 이동 방향을 조정
        if (self->x <= other->x) self->x -= self->step;
        else if (self->x > other->x) self->x += self->step;

         if (self->y <= other->y) self->y -= self->step;
        else if (self->y > other->y) self->y += self->step;
    }
}

void Fire_Bullet(int x, int y, int w, int h, int dx, int dy, int damage, UnitType owner, UNIT* target) {
    int i;
    for ( i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].w = w; // 총알의 가로 크기
            bullets[i].h = h; // 총알의 세로 크기
            bullets[i].dx = dx;
            bullets[i].dy = dy;
            bullets[i].damage = damage;
            bullets[i].owner = owner; // 총알의 소유자
            bullets[i].target = target; // 명중 대상 유닛 설정
            bullets[i].active = 1; // 총알 활성화
            break;
        }
    }
}

void Update_Bullets(void) {
    int i;
    for ( i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].dx;
            bullets[i].y += bullets[i].dy;
            // 화면 밖이면 비활성화
            if (bullets[i].x < camera_x || bullets[i].x > camera_x +LCDW ||
                bullets[i].y < camera_y || bullets[i].y > camera_y + LCDH) {
                bullets[i].active = 0;
            }
        }
    }
}

void Draw_Bullets(int ci) {
    int i;
    for ( i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            screen_x = bullets[i].x - camera_x;
            screen_y = bullets[i].y - camera_y;
            if (screen_x >= 0 && screen_x < LCDW && screen_y >= 0 && screen_y < LCDH) {
                Lcd_Draw_Box(screen_x, screen_y, BULLET_SIZE, BULLET_SIZE, color[ci]);
            }
        }
        else{
            screen_x = bullets[i].x - camera_x; // 이전 X 좌표
            screen_y = bullets[i].y - camera_y; // 이전 Y 좌표
            if (screen_x >= 0 && screen_x < LCDW && screen_y >= 0 && screen_y < LCDH) {
                // 이전 총알 위치를 배경 색상으로 덮어씌움
                Lcd_Draw_Box(screen_x, screen_y, BULLET_SIZE, BULLET_SIZE, color[BACK_COLOR]);
            }
        }
    }
}

void Fire_Bullet_Towards(UNIT *attacker, UNIT *target) {
    int dx = target->x - attacker->x;
    int dy = target->y - attacker->y;

    int abs_dx = abs(dx);
    int abs_dy = abs(dy);
    int max_abs = (abs_dx > abs_dy) ? abs_dx : abs_dy;

    if (max_abs > 0) {
        dx = (dx * BULLET_SPEED) / max_abs;
        dy = (dy * BULLET_SPEED) / max_abs;
    } else {
        // 공격자와 대상이 같은 위치에 있는 경우, 임의의 방향으로 발사 (예: 오른쪽)
        dx = BULLET_SPEED;
        dy = 0;
    }
    Fire_Bullet(attacker->x + attacker->w / 2, attacker->y + attacker->h / 2, BULLET_SIZE, BULLET_SIZE, dx, dy, attacker->attack_damage, attacker->UnitType, target); 
}

UNIT* Find_Closest_Enemy(UNIT *attacker) {
    
    UNIT* closest = NULL;
    int min_distance_squared = 9999999;

    int range = attacker->attack_range; // 공격자의 사거리
    int range_squared = range * range;

    UNIT* groups[] = {zergling, hydralisk, ultralisk};
    int counts[] = {zergling_count, hydralisk_count, ultralisk_count};
    int g,i;
    for ( g = 0; g < 3; g++) {
        for ( i = 0; i < counts[g]; i++) {
            UNIT* enemy = &groups[g][i];
            if (!enemy->active || enemy->hp <= 0) continue;

            int dx = enemy->x - attacker->x;
            int dy = enemy->y - attacker->y;
            int dist_sq = dx * dx + dy * dy;
            if (dist_sq <= range_squared && dist_sq < min_distance_squared) {
                min_distance_squared = dist_sq;
                closest = enemy;
            }
        }
    }
    return closest;
}

UNIT* Find_Closest_Target_For_Zerg(UNIT *attacker) {
    UNIT* closest = NULL;
    int min_distance_squared = 9999999;

    int range = attacker->attack_range; // Attacker's range
    int range_squared = range * range;

    // Check the Hero
    int dx_hero = hero.x - attacker->x;
    int dy_hero = hero.y - attacker->y;
    int dist_sq_hero = dx_hero * dx_hero + dy_hero * dy_hero;

    if (dist_sq_hero <= range_squared) {
        min_distance_squared = dist_sq_hero;
        closest = &hero;
    }

    // Check Marines
    int j;
    for ( j = 0; j < marine_count; j++) {
        if (!marine[j].active || marine[j].hp <= 0) continue;

        int dx_marine = marine[j].x - attacker->x;
        int dy_marine = marine[j].y - attacker->y; // 오타 수정
        int dist_sq_marine = dx_marine * dx_marine + dy_marine * dy_marine;

        if (dist_sq_marine <= range_squared && dist_sq_marine < min_distance_squared) {
            min_distance_squared = dist_sq_marine;
            closest = &marine[j];
        }
    }

    return closest;
}


void Check_Bullet_Collision(void) {
    int i;
    for ( i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        UNIT* target = bullets[i].target;
        if (!target || !target->active || target->hp <= 0) continue;

        if (Check_Bullet_Hit(&bullets[i], target)) {
            target->hp -= bullets[i].damage;
            if (target->hp <= 0) {
                target->hp = 0;
                target->active = 0;
                target->ci = BACK_COLOR;
            }
            bullets[i].active = 0;
        }
    }
}

void Combat_Mode_Update(void) {
    hero.attack_speed += 1; // 공격 속도 증가 (쿨타임)
    if (hero.mode == mode_attack && hero.attack_speed >= 5) {
        UNIT* closest_enemy = Find_Closest_Enemy(&hero);
        if (closest_enemy != NULL) {
            Fire_Bullet_Towards(&hero, closest_enemy);
           hero.attack_speed = 0; // 공속 쿨타임 초기화
        }
    }
}

void Zergling_Attack_Hero(UNIT *zergling) {
    if (Check_Collision_Box(zergling, &hero)) { // 저글링과 영웅이 충돌했는지 확인
        hero.hp -= zergling->attack_damage; // 영웅의 체력을 저글링의 공격력만큼 감소
        if (hero.hp < 0) hero.hp = 0; // 체력이 0 이하로 내려가지 않도록 설정
    }
}

void Ultralisk_Attack_Hero(UNIT *ultralisk) {
    if (Check_Collision_Box(ultralisk, &hero)) { // 울트라리스크와 영웅이 충돌했는지 확인
        hero.hp -= ultralisk->attack_damage; // 영웅의 체력을 울트라리스크의 공격력만큼 감소
        if (hero.hp < 0) hero.hp = 0; // 체력이 0 이하로 내려가지 않도록 설정
    }
}
// void Hydralisk_Attack_Hero(UNIT *hydralisk) {
//     hydralisk->attack_speed += 1; // 공격 속도 증가 (쿨타임)
//     if(hydralisk->attack_speed >= 10){
//     Fire_Bullet_Towards(hydralisk, &hero); // 히드라가 영웅을 향해 총알 발사
//     hydralisk->attack_speed = 0; // 공격 속도 초기화
//     }
// }
#if 0
static int Target_Range(UNIT *attacker, UNIT *target) {
    signed int dx = attacker->x - target->x; // X 좌표 차이
    signed int dy = attacker->y - target->y; // Y 좌표 차이
    int distance_squared = (dx * dx) + (dy * dy); // 거리의 제곱 계산
    int range_squared = attacker->attack_range * attacker->attack_range; // 범위의 제곱 계산
    return distance_squared <= range_squared; // 거리의 제곱이 범위의 제곱보다 작거나 같은지 확인
}
#endif


void Zerg_Move(UNIT *self, int step) {
    if (self->x < hero.x-10) self->x += step;
    else if (self->x > hero.x+10) self->x -= step;

    if (self->y < hero.y-10) self->y += step;
    else if (self->y > hero.y+10) self->y -= step;
}

void Zergling_Move(UNIT *self) {
    if(self->mode == mode_attack){
    Zerg_Move(self, Zergling_STEP);
    }
}

void Hydralisk_Move(UNIT *self) {
    UNIT* target = Find_Closest_Target_For_Zerg(self); // 가장 가까운 대상 (영웅 또는 마린) 찾기

    if (target != NULL) {
        // 공격할 대상이 사거리 안에 있으면 공격 모드
        self->mode = mode_attack;
        self->attack_speed++; // 공격 쿨타임 증가
        if (self->attack_speed >= 10) { // 공격 쿨타임 값
            Fire_Bullet_Towards(self, target); // 대상에게 총알 발사
            self->attack_speed = 0; // 쿨타임 초기화
        }
        // 공격 중에는 이동하지 않거나 대상에게서 거리를 유지하는 로직 추가 가능
        // 현재는 사거리 안에 대상이 있으면 이동 멈춤
    } else if(self->mode == mode_attack) {
        // 공격할 대상이 사거리 밖에 있으면 이동 모드, 영웅을 향해 이동
        self->attack_speed++;
        Zerg_Move(self, Hydralisk_STEP); // 영웅을 향해 이동
    }

}

void Ultralisk_Move(UNIT *self) {
    if(self->mode == mode_attack){
    Zerg_Move(self, Ultralisk_STEP);
    }
}

const int destination_list[][2] = {
    { 0, -20 }, { 20, 0 }, { 0, 20 }, { -20, 0 },
    {-20,-20},{ 20, -20 }, { 20, 20 }, { -20, 20 }
};
void Marine_Move(void) {
    int step = Marine_STEP;
    int destination_x;
    int destination_y;
    int i;
    for(i = 0; i < marine_count; i++){
        if(marine[i].active == 1 && marine[i].mode==mode_move){ // 마린이 활성화된 경우
            destination_x = hero.x + destination_list[i][0];
            destination_y = hero.y + destination_list[i][1];
            if (marine[i].x <= destination_x) marine[i].x += step;
            else if (marine[i].x > destination_x) marine[i].x -= step;
            if(marine[i].y <= destination_y) marine[i].y += step;
            else if (marine[i].y > destination_y) marine[i].y -= step;
            marine[i].attack_speed+=1; // 공격 쿨타임 증가
        }
    }
}
void Marine_Attack(void) {
    int i;
    for(i = 0; i < marine_count; i++){
        if(marine[i].active == 1 && marine[i].mode == mode_attack){ // 마린이 활성화된 경우
            marine[i].attack_speed+=1; // 공격 쿨타임 증가
            if(marine[i].attack_speed  >= 10){ 
                UNIT* closest_enemy = Find_Closest_Enemy(&marine[i]); // 가장 가까운 적 찾기

               if (closest_enemy != NULL) {
                   // 사거리 체크는 Find_Closest_Enemy에서 이미 수행됨
                   Fire_Bullet_Towards(&marine[i], closest_enemy); // 가장 가까운 적에게 총알 발사
                   marine[i].attack_speed = 0; // 쿨타임 초기화
                   
               } 
            }
        }
    }
}



static void k0(void)
{
    hero.mode = mode_move; // 이동 모드
	if(camera_y > Y_MIN) 
	{
		camera_y -= HERO_STEP;
		hero.y -= HERO_STEP;
	}
}

static void k1(void)
{
    hero.mode = mode_move; // 이동 모드
	if( (camera_y + LCDH) < Y_MAX) 
	{
		camera_y += HERO_STEP;
		hero.y += HERO_STEP;
	}
}

static void k2(void)
{       
    hero.mode = mode_move; // 이동 모드
	if(camera_x > X_MIN)
	{
		camera_x -= HERO_STEP;
		hero.x -= HERO_STEP;
	}
}

static void k3(void)
{
    hero.mode = mode_move; // 이동 모드
	if( (camera_x + LCDW) < X_MAX) 
	{
	camera_x += HERO_STEP;
	hero.x += HERO_STEP;
	}
}

static void k4(void)
{
    int i;
    for(i=0; i <marine_count; i++){
        if(marine[i].active == 1){ // 마린이 활성화된 경우
        marine[i].mode = mode_attack; // 전투 모드
        }
    }
    hero.mode = mode_attack; // 전투 모드
}

static void k5(void)
{
    int i;
    for(i=0; i <marine_count; i++){
        if(marine[i].active == 1){ // 마린이 활성화된 경우
        marine[i].mode = mode_move; // 이동 모드
        }
    }
    hero.mode = mode_move; // 이동 모드
}

static void Hero_Move(int k)
{
	// UP(0), DOWN(1), LEFT(2), RIGHT(3)
	static void (*key_func[])(void) = {k0, k1, k2, k3, k4, k5};
	if(k <= 5) key_func[k]();
}

void Add_Unit(UNIT *unit_array, int array_size, UNIT new_unit) {
    int i;
    for ( i = 0; i < array_size; i++) {
        if (!unit_array[i].active) { // 비활성화된 슬롯 찾기
            unit_array[i] = new_unit; // 새로운 유닛 추가
            unit_array[i].active = 1; // 활성화
            return;
        }
    }
    DEBUG_PRINT("Failed to add unit, no available slot in array.\n");
}

void Remove_Unit(UNIT *unit_array, int array_size, int index) {
    if (index >= 0 && index < array_size) {
        unit_array[index].active = 0; // 비활성화
    }
}

static UNIT new_marine = {
    .x = 150, // 초기 위치는 Add_Unit 호출 시 또는 다른 곳에서 설정될 수 있으므로 기본값
    .y = 50,
    .w = Marine_SIZE_X,
    .h = Marine_SIZE_Y,
    .step = Marine_STEP,
    .hp = Marine_hp,
    .attack_range = Marine_attack_range,
    .attack_damage = Marine_damage,
    .attack_speed = Marine_attack_speed,
    .ci = MARINE_COLOR,
    .active = 0, // 템플릿은 비활성 상태
    .UnitType = TERRAN,
    .mode = mode_move, // 마린의 기본 모드
    .move = NULL, // 이동 함수 포인터 설정
    .attack = NULL, // 필요에 따라 설정
    .draw = NULL    // 필요에 따라 설정
};
#if 0
static UNIT new_zergling = {
    .x = 0, // 초기 위치는 Add_Unit 호출 시 또는 다른 곳에서 설정될 수 있으므로 기본값
    .y = 0,
    .w = Zergling_SIZE_X,
    .h = Zergling_SIZE_Y,
    .step = Zergling_STEP,
    .hp = Zergling_hp,
    .attack_range = Zergling_attack_range,
    .attack_damage = Zergling_damage,
    .attack_speed = 5, // Init_Zergs에서 사용된 값 참조
    .ci = ZERG_COLOR,
    .active = 0, // 템플릿은 비활성 상태
    .UnitType = ZERGLING,
    .mode = mode_stop, // 저글링의 기본 모드
    .move = Zergling_Move, // 이동 함수 포인터 설정
    .attack = NULL, // 필요에 따라 설정
    .draw = NULL    // 필요에 따라 설정
};

static UNIT new_hydralisk = {
    .x = 0, // 초기 위치는 Add_Unit 호출 시 또는 다른 곳에서 설정될 수 있으므로 기본값
    .y = 0,
    .w = Hydralisk_SIZE_X,
    .h = Hydralisk_SIZE_Y,
    .step = Hydralisk_STEP,
    .hp = Hydralisk_hp,
    .attack_range = Hydralisk_attack_range,
    .attack_damage = 20, // Init_Zergs에서 사용된 값 참조
    .attack_speed = 10, // Init_Zergs에서 사용된 값 참조
    .ci = ZERG_COLOR,
    .active = 0, // 템플릿은 비활성 상태
    .UnitType = HYDRALISK,
    .mode = mode_stop, // 히드라의 기본 모드
    .move = Hydralisk_Move, // 이동 함수 포인터 설정
    .attack = NULL, // 필요에 따라 설정
    .draw = NULL    // 필요에 따라 설정
};

static UNIT new_ultralisk = {
    .x = 0, // 초기 위치는 Add_Unit 호출 시 또는 다른 곳에서 설정될 수 있으므로 기본값
    .y = 0,
    .w = Ultralisk_SIZE_X,
    .h = Ultralisk_SIZE_Y,
    .step = Ultralisk_STEP,
    .hp = Ultralisk_hp,
    .attack_range = Ultralisk_attack_range,
    .attack_damage = Ultralisk_damage,
    .attack_speed = 10, // Init_Zergs에서 사용된 값 참조
    .ci = ZERG_COLOR,
    .active = 0, // 템플릿은 비활성 상태
    .UnitType = ULTRALISK,
    .mode = mode_stop, // 울트라의 기본 모드
    .move = Ultralisk_Move, // 이동 함수 포인터 설정
    .attack = NULL, // 필요에 따라 설정
    .draw = NULL    // 필요에 따라 설정
};
#endif
void Init_Zergs(void) 
{
    int i;
	for (i = 0; i < zergling_count; i++) {
        zergling[i].x = 300 +(i*30); // X 좌표를 적당히 분산
        zergling[i].y = 300;  // Y 좌표를 적당히 분산
        zergling[i].w = Zergling_SIZE_X;           // 적의 너비
        zergling[i].h = Zergling_SIZE_Y;           // 적의 높이
		zergling[i].step = Zergling_STEP; // 저글링 이동 속도
        zergling[i].ci = ZERG_COLOR; // 적의 색상
        zergling[i].hp = Zergling_hp; // 저글링 체력
        zergling[i].attack_damage = Zergling_damage; // 저글링 공격력
        zergling[i].attack_range = Zergling_attack_range; // 공격 범위
        zergling[i].attack_speed =5; // 공격 속도 100ms 단위
        zergling[i].UnitType = ZERGLING;
        zergling[i].mode = mode_stop; // 초기 모드드
        zergling[i].move = Zergling_Move;
		zergling[i].active = 1; // 활성화
	}
	for (i = 0; i < hydralisk_count; i++) {
		hydralisk[i].x = 400+(i*30); // X 좌표를 적당히 분산
		hydralisk[i].y = 400;  // Y 좌표를 적당히 분산
		hydralisk[i].w = Hydralisk_SIZE_X;           // 적의 너비
		hydralisk[i].h = Hydralisk_SIZE_Y;           // 적의 높이
		hydralisk[i].step = Hydralisk_STEP; // 히드라 이동 속도
		hydralisk[i].ci = ZERG_COLOR; // 적의 색상
        hydralisk[i].hp = Hydralisk_hp; // 히드라 체력
        hydralisk[i].attack_damage = Hydralisk_damage; // 히드라 공격력
		hydralisk[i].attack_range = Hydralisk_attack_range; // 공격 범위
        hydralisk[i].attack_speed =10; // 공격 속도 100ms 단위
		hydralisk[i].attack_damage = 20; // 공격력
		hydralisk[i].UnitType = HYDRALISK;
        hydralisk[i].mode = mode_stop; // 초기 모드
		hydralisk[i].move = Hydralisk_Move;
		hydralisk[i].active = 1; // 활성화
	}
	for (i = 0; i < ultralisk_count; i++) {
		ultralisk[i].x = 700+(i*60); // X 좌표를 적당히 분산
		ultralisk[i].y = 700;  // Y 좌표를 적당히 분산
		ultralisk[i].w = Ultralisk_SIZE_X;           // 적의 너비
		ultralisk[i].h = Ultralisk_SIZE_Y;           // 적의 높이
        ultralisk[i].hp = Ultralisk_hp; // 울트라 체력
        ultralisk[i].attack_range = Ultralisk_attack_range; // 공격 범위
        ultralisk[i].attack_damage = Ultralisk_damage; // 공격력
        ultralisk[i].attack_speed =10; // 공격 속도 100ms 단위
		ultralisk[i].step = Ultralisk_STEP; // 울트라 이동 속도
		ultralisk[i].ci = ZERG_COLOR; // 적의 색상
		ultralisk[i].UnitType = ULTRALISK;
        ultralisk[i].mode = mode_stop; // 초기 모드
		ultralisk[i].move = Ultralisk_Move;
		ultralisk[i].active = 1; // 활성화
	}
}
void Init_Terran(void) {
    int i;
  for(i=0; i<marine_count; i++){
    marine[i].x = 150; 
    marine[i].y = 50;  
    marine[i].w = Marine_SIZE_X;
    marine[i].h = Marine_SIZE_Y;
    marine[i].step = Marine_STEP; // 마린 이동 속도
    marine[i].ci = MARINE_COLOR; // 마린 색상
    marine[i].hp = Marine_hp; // 마린 체력
    marine[i].attack_damage = Marine_damage; // 마린 공격력
    marine[i].attack_range = Marine_attack_range; // 공격 범위
    marine[i].attack_speed = Marine_attack_speed; // 공격 속도 100ms 단위
    marine[i].UnitType = TERRAN;
    marine[i].mode = mode_move; // 초기 모드
    marine[i].active = 0; // 활성화

  }
}

#if 1
void Move_All_Zergs_Units(void) {
    int i;
    for ( i = 0; i < zergling_count; i++) {
        if (zergling[i].active) { // 활성화된 유닛만 이동
            zergling[i].move(&zergling[i]);
            if (i > 0) {
                Avoid_Collision(&zergling[i], &zergling[i - 1]); // 충돌 회피
            }
        }
    }
    for ( i = 0; i < hydralisk_count; i++) {
        if (hydralisk[i].active) { // 활성화된 유닛만 이동
            hydralisk[i].move(&hydralisk[i]);
            if (i > 0) {
                Avoid_Collision(&hydralisk[i], &hydralisk[i - 1]); // 충돌 회피
            }
        }
    }
    for ( i = 0; i < ultralisk_count; i++) {
        if (ultralisk[i].active) { // 활성화된 유닛만 이동
            ultralisk[i].move(&ultralisk[i]);
            if (i > 0) {
                Avoid_Collision(&ultralisk[i], &ultralisk[i - 1]); // 충돌 회피
            }
        }
    }
}
#else

void Move_All_Zergs_Units(UNIT *unit_array, int unit_count) {
    for ( i = 0; i < unit_count; i++) {
        if (unit_array[i].active) { // 활성화된 유닛만 이동
            unit_array[i].move(&unit_array[i]);
            if (i > 0) {
                Avoid_Collision(&unit_array[i], &unit_array[i - 1]); // 충돌 회피
            }
        }
    }
}
#endif
void Init_Hero(void) {
    hero.x = 150;
    hero.y = 110;
    hero.w = HERO_SIZE_X;
    hero.h = HERO_SIZE_Y;
    hero.ci = HERO_COLOR;
    hero.attack_range = 120;
    hero.attack_damage = 30;
    hero.attack_speed = 5; // 공격 속도 100ms 단위
    hero.hp = 500;
    hero.UnitType = HERO;
    hero.mode = mode_move; // 초기 모드
}



static void Draw_Object(UNIT * obj)
{
	screen_x = obj->x - camera_x; // 카메라 기준 상대 좌표
    screen_y = obj->y - camera_y; // 카메라 기준 상대 좌표
	if (screen_x + obj->w > 0 && screen_x < LCDW && screen_y + obj->h > 0 && screen_y < LCDH) 
		{
            if(obj->UnitType != HERO && obj-> UnitType != TERRAN && obj->mode != mode_attack){
            obj->mode = mode_attack; // 공격 모드로 설정
            }
			Lcd_Draw_Box(screen_x, screen_y, obj->w, obj->h, color[obj->ci]);
		}
}
void Draw_All_Marine(int color) {
    int i;
    for ( i = 0; i < marine_count; i++) {
        if (marine[i].active) { // 활성화된 유닛만 그리기
            marine[i].ci = color;
            Draw_Object(&marine[i]);
        }
    }
}
#if 1
void Draw_All_Zergs(int color) {
    int i;
    for ( i = 0; i < zergling_count; i++) {
        if (zergling[i].active) { // 활성화된 유닛만 그리기
            zergling[i].ci = color;
            Draw_Object(&zergling[i]);
        }
    }
    for ( i = 0; i < hydralisk_count; i++) {
        if (hydralisk[i].active) { // 활성화된 유닛만 그리기
            hydralisk[i].ci = color;
            Draw_Object(&hydralisk[i]);
        }
    }
    for ( i = 0; i < ultralisk_count; i++) {
        if (ultralisk[i].active) { // 활성화된 유닛만 그리기
            ultralisk[i].ci = color;
            Draw_Object(&ultralisk[i]);
        }
    }
}
#else

void Draw_All_Units(UNIT *unit_array, int unit_count, int color) {
    for (int i = 0; i < unit_count; i++) {
        if (unit_array[i].active) { // 활성화된 유닛만 그리기
            unit_array[i].ci = color;
            Draw_Object(&unit_array[i]);
        }
    }
}
#endif

//전투 수신
void Process_Command(char* cmd)
{
    if (strcmp(cmd, "SPAWN_MARINE\n") == 0) {
        Add_Unit(marine, marine_count, new_marine ); // 마린 추가
    }
    else{
        DEBUG_PRINT("Unknown command: %s\n", cmd);
    }
}


static void Game_Init(void)
{
	Lcd_Clr_Screen();
	camera_x = 0; camera_y = 0;
	Init_Hero();
	Init_Zergs();
	Draw_Object(&hero);
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

void Main(void)
{
	System_Init();
	Uart_Printf("STARCAFT\n");

	Lcd_Init(DIPLAY_MODE);

	Jog_Poll_Init();
	Jog_ISR_Enable(1);
	Uart1_RX_Interrupt_Enable(1);
    Uart2_RX_TX_Interrupt_Enable(1);

	for(;;)
	{
		Game_Init();
		TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD*10);

		for(;;)
		{
            //Lcd_Printf(0,0,BLUE,WHITE,2,2,"%d", hero.hp);
			if (Jog_key_in) {
                Draw_All_Zergs(BACK_COLOR); // 저그 색상 초기화
                Draw_All_Marine(BACK_COLOR); // 마린 색상 초기화
                Draw_Bullets(BACK_COLOR); // 총알 지우기
                Hero_Move(Jog_key);
                Draw_All_Marine(MARINE_COLOR); // 마린 색상 복구
				Draw_All_Zergs(ZERG_COLOR); // 저그 색상 복구
                Draw_Bullets(BULLET_COLOR); // 총알 그리기
                Draw_Object(&hero);
                Jog_key_in = 0;
            }

            if (TIM4_expired) {
                Combat_Mode_Update(); // 전투 모드 업데이트
                Marine_Attack(); // 마린 공격
                Draw_Bullets(BACK_COLOR); // 총알 지우기
				Draw_All_Zergs(BACK_COLOR); // 저그 색상 초기화
                Draw_All_Marine(BACK_COLOR); // 마린 색상 초기화
                Update_Bullets(); // 총알 업데이트
                Check_Bullet_Collision(); // 총알과 적의 충돌 처리
				Move_All_Zergs_Units(); // 저그 이동
                Marine_Move(); // 마린 이동
                Draw_All_Marine(MARINE_COLOR); // 마린 색상 복구
                Draw_Bullets(BULLET_COLOR); // 총알 그리기
				Draw_All_Zergs(ZERG_COLOR); // 저그 색상 복구

                Lcd_Printf(0,0,BLUE,WHITE,2,2,"%d", hero.hp);
                TIM4_expired = 0;
            }

            if(Uart2_Rx_Command_Ready)
            {
                Uart2_Rx_Command_Ready = 0; // 플래그 초기화 (반드시 처리 전에)
            }

			if(hero.hp <= 0)
			{
				TIM4_Repeat_Interrupt_Enable(0, 0);
				Uart_Printf("Game Over, Please press any key to continue.\n");
				Jog_Wait_Key_Pressed();
				Jog_Wait_Key_Released();
				Uart_Printf("Game Start\n");
				break;
			}
		}
	}
}
