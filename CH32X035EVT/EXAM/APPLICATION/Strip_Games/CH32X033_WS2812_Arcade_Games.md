# CH32X033 + WS2812B 一维灯带游戏机设计方案与完整源码（三合一街机版）

## 1. 系统概述与硬件选型

本方案基于沁恒 **CH32X033F8P6**（48MHz RISC-V 内核，62KB Flash，20KB SRAM，内置 USB 2.0 与 PIOC 协议协处理器）设计，利用硬件 **PIOC** 零 CPU 阻塞驱动 60 颗 WS2812B 全彩灯珠，配合无源蜂鸣器与双按键，实现包含**《能量对波》**、**《拆弹轮盘》**、**《狭路相逢》**在内的三合一桌面街机游戏机。

### 1.1 为什么选用 PIOC 驱动而非 SPI+DMA？
* **显存零膨胀**：WS2812B 通信速率固定为 800Kbps（单颗灯珠固定耗时 `30μs`）。SPI+DMA 需要用 4~8 bit 模拟 1 bit 信号，显存膨胀 4~8 倍；而 **PIOC 协处理器直接读取标准 RGB 字节流**，60 颗灯珠仅占 `180 字节` 显存，且无需 CPU 执行位运算编解码循环。
* **零 CPU 与 DMA 占用**：主核将颜色数据写入共享缓冲区并触发启动信号后，PIOC 在后台独立完成精确波形发送，主核可全力运行 60 FPS 物理碰撞与亚像素渲染。

---

## 2. 硬件原理图与接线指南

### 2.1 系统电路拓扑图

```text
                       +5V (USB VBUS)
                         │
          ┌──────────────┼──────────────┐
          │              │              │
       [1000uF]      [AMS1117-3.3]   [1N4007] (降压至~4.3V，完美兼容3.3V信号)
       电解电容          │              │
          │            +3.3V            ▼
         GND             │         ┌─────────┐
                         ├────────►│ VDD     │
                         │         │         │
                   ┌─────┴─────┐   │ WS2812B │
                   │ VDD   VSS ├──►│ GND     │ (必须与MCU共地!)
                   │           │   │ (60颗)  │
  P1按键 ──[10k]──►│ PA0  PC18 ├───► DIN     │ (PIOC 硬件单线输出)
  (按下接地)       │           │   └─────────┘
                   │           │
  P2按键 ──[10k]──►│ PA1  PC16 ├───► USB D- (支持USB免下载器烧录/通信)
  (按下接地)       │      PC17 ├───► USB D+
                   │           │
 蜂鸣器(+)─[1k]───►│ PA2       │ (TIM1_CH1 PWM 无源蜂鸣器)
 蜂鸣器(-)────────►│ GND       │
                   └───────────┘
                   CH32X033F8P6
```

### 2.2 硬件设计避坑要点
1. **3.3V 转 5V 电平匹配**：WS2812B 高电平逻辑识别阈值为 $0.7 	imes V_{DD}$。在 5V 供电下，3.3V 信号处于临界边缘。在灯带 `VDD` 供电线上串联一颗 **1N4007 二极管**，可将灯带供电降至 `4.3V`（此时逻辑高电平阈值降至 $0.7 	imes 4.3	ext{V} = 3.01	ext{V} < 3.3	ext{V}$），实现 100% 稳定通信且肉眼亮度无感差异。
2. **电源去耦与防复位**：灯带电源端必须并联一颗 **470μF ~ 1000μF 电解电容**，防止爆闪瞬间电流拉低 USB 5V 电压导致单片机复位。
3. **全局亮度限流**：代码中已限制最大亮度为 `25%`，整机峰值电流控制在 `350mA` 以内，可直接由电脑 USB 口或普通充电宝供电。

---

## 3. 游戏操作说明

### 3.1 全局模式切换
* **同时长按 P1 键 + P2 键超过 1.5 秒**：蜂鸣器发出提示音，循环切换以下三款游戏。

### 3.2 游戏列表与玩法
| 模式 | 游戏名称 | 视觉特征 | 按键操作与规则 |
| :--- | :--- | :--- | :--- |
| **模式 1** | **能量对波**（双人拔河） | 两端蓝/红基地护盾灯 | **P1 (PA0)** 与 **P2 (PA1)** 对战。<br>• **短按**：发射高速普通光弹，相撞抵消。<br>• **长按 >0.5s 松开**：发射蓄力重击波，可直接碾压吞噬对方普通光弹。<br>• 击穿对方底线扣除 1 格护盾，护盾归零判负并触发胜利音效。 |
| **模式 2** | **拆弹轮盘**（反应力） | 灯带中央绿色安全区 | 单人或双人抢拍均可。<br>• 红色高速光点在灯带来回反弹，在经过**绿色安全区**瞬间按下任意键即拆弹成功。<br>• 每成功一次，光点速度加快且绿色安全区缩小；按偏则触发爆炸音效并重置。 |
| **模式 3** | **狭路相逢**（1D 跑酷） | 左侧白色玩家光点 | 迎面随机刷出红色障碍物。<br>• **P1 (PA0) 跳跃**：玩家变**青色**并进入滞空状态，可越过红色地面陷阱。<br>• **P2 (PA1) 射击**：向前发射黄色激光，可直接摧毁迎面而来的障碍物。 |

---

## 4. 完整固件源代码

### 4.1 工程配置说明
1. 下载沁恒官方 **`CH32X035EVT.ZIP`**（CH32X033 与 CH32X035 共用同一套 SDK 与内核）。
2. 在 MounRiver Studio 中打开例程 `EVT/EXAM/PIOC/PIOC_WS2812`。
3. 保留例程中的 `PIOC_INC` 文件夹（内含 PIOC 协议机机器码），使用下方提供的 `pioc_ws2812_adapter.c` 与 `main.c` 即可编译运行。

### 4.2 PIOC 驱动适配层 (`pioc_ws2812_adapter.c`)
> 说明：WS2812B 芯片内部接收颜色字节顺序为 **GRB**（绿-红-蓝），此适配层负责在拷贝显存至 PIOC 共享缓冲区时自动完成 `RGB -> GRB` 字节序转换，并触发 PIOC 硬件非阻塞发送。

```c
#include "debug.h"
#include <string.h>

// 引用官方 PIOC_WS2812 例程中的底层接口与缓冲区定义
// 注：具体符号名请与官方 EVT/EXAM/PIOC/PIOC_WS2812/User/main.c 保持一致
extern void PIOC_Init(void);
extern void RGB_1W_Send(uint8_t *pbuf, uint16_t len);

typedef struct {
    uint8_t r, g, b;
} RGB_t;

// 转换缓冲区（GRB 字节序）
static uint8_t grb_tx_buf[300 * 3];

void PIOC_WS2812_Init(void) {
    PIOC_Init();
}

void PIOC_WS2812_Send(RGB_t *data, uint16_t num) {
    if (num > 300) num = 300;
    for (uint16_t i = 0; i < num; i++) {
        grb_tx_buf[i * 3 + 0] = data[i].g; // Green
        grb_tx_buf[i * 3 + 1] = data[i].r; // Red
        grb_tx_buf[i * 3 + 2] = data[i].b; // Blue
    }
    // 调用官方 PIOC 协处理器异步发送函数（0 CPU 阻塞）
    RGB_1W_Send(grb_tx_buf, num * 3);
}
```

### 4.3 游戏主程序与非阻塞音效引擎 (`main.c`)

```c
#include "debug.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ================= 硬件与系统参数 =================
#define LED_NUM         60
#define BRIGHTNESS      0.25f

#define P1_KEY_PIN      GPIO_Pin_0  // PA0
#define P2_KEY_PIN      GPIO_Pin_1  // PA1
#define BUZZER_PIN      GPIO_Pin_2  // PA2 (TIM1_CH1)

typedef struct { uint8_t r, g, b; } RGB_t;

// 外部 PIOC 驱动声明
extern void PIOC_WS2812_Init(void);
extern void PIOC_WS2812_Send(RGB_t *data, uint16_t num);

RGB_t led_buf[LED_NUM];
volatile uint8_t frame_flag = 0;

// 游戏模式枚举
typedef enum {
    GAME_TUG_OF_WAR = 0, // 1. 能量对波
    GAME_BOMB_DEFUSE,    // 2. 拆弹轮盘
    GAME_1D_RUNNER,      // 3. 狭路相逢
    GAME_MODE_COUNT
} GameMode_t;

GameMode_t current_mode = GAME_TUG_OF_WAR;
uint16_t both_hold_ticks = 0; // 双键同时长按计数（切游戏用）

// ================= 1. 非阻塞蜂鸣器音效引擎 =================
typedef struct {
    uint16_t freq;     // 频率 (Hz), 0 表示静音
    uint8_t  frames;   // 持续帧数 (1帧 = 16.6ms)
} Note_t;

#define SFX_QUEUE_LEN 8
Note_t sfx_queue[SFX_QUEUE_LEN];
uint8_t sfx_head = 0, sfx_tail = 0, sfx_timer = 0;

void Buzzer_PWM_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 48MHz / 48 = 1MHz 计数时钟
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 48 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0; // 默认静音
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

void Buzzer_SetFreq(uint16_t freq) {
    if (freq == 0) {
        TIM_SetCompare1(TIM1, 0);
    } else {
        uint16_t arr = (1000000 / freq) - 1;
        TIM_SetAutoreload(TIM1, arr);
        TIM_SetCompare1(TIM1, arr / 2); // 50% 占空比方波
    }
}

void SFX_Push(uint16_t freq, uint8_t frames) {
    uint8_t next = (sfx_tail + 1) % SFX_QUEUE_LEN;
    if (next != sfx_head) {
        sfx_queue[sfx_tail] = (Note_t){freq, frames};
        sfx_tail = next;
    }
}

// 预设组合音效
void SFX_Play_Shoot(void)   { SFX_Push(880, 2); SFX_Push(1320, 2); }
void SFX_Play_Heavy(void)   { SFX_Push(300, 3); SFX_Push(450, 3); SFX_Push(700, 4); }
void SFX_Play_Hit(void)     { SFX_Push(200, 3); SFX_Push(120, 4); }
void SFX_Play_Jump(void)    { SFX_Push(520, 2); SFX_Push(780, 3); }
void SFX_Play_Win(void)     { SFX_Push(523, 4); SFX_Push(659, 4); SFX_Push(784, 4); SFX_Push(1046, 8); }
void SFX_Play_Switch(void)  { SFX_Push(1000, 3); SFX_Push(0, 2); SFX_Push(1500, 4); }

void SFX_Update(void) {
    if (sfx_timer > 0) {
        sfx_timer--;
        if (sfx_timer == 0) Buzzer_SetFreq(0);
    } else if (sfx_head != sfx_tail) {
        Note_t n = sfx_queue[sfx_head];
        sfx_head = (sfx_head + 1) % SFX_QUEUE_LEN;
        Buzzer_SetFreq(n.freq);
        sfx_timer = n.frames;
    }
}

// ================= 2. 渲染辅助工具 =================
void Add_Pixel(int idx, uint8_t r, uint8_t g, uint8_t b) {
    if (idx < 0 || idx >= LED_NUM) return;
    uint16_t nr = led_buf[idx].r + (uint8_t)(r * BRIGHTNESS);
    uint16_t ng = led_buf[idx].g + (uint8_t)(g * BRIGHTNESS);
    uint16_t nb = led_buf[idx].b + (uint8_t)(b * BRIGHTNESS);
    led_buf[idx].r = (nr > 255) ? 255 : nr;
    led_buf[idx].g = (ng > 255) ? 255 : ng;
    led_buf[idx].b = (nb > 255) ? 255 : nb;
}

void Draw_Subpixel(float pos, uint8_t r, uint8_t g, uint8_t b) {
    int base = (int)pos;
    float frac = pos - base;
    Add_Pixel(base,     (uint8_t)(r * (1.0f - frac)), (uint8_t)(g * (1.0f - frac)), (uint8_t)(b * (1.0f - frac)));
    Add_Pixel(base + 1, (uint8_t)(r * frac),          (uint8_t)(g * frac),          (uint8_t)(b * frac));
}

// ================= 3. 游戏一：《能量对波》 =================
typedef struct { uint8_t active, owner, is_heavy; float pos, vel; } Bullet_t;
Bullet_t bullets[16];
uint8_t p1_hp, p2_hp;
uint16_t p1_hold, p2_hold;

void Game1_Init(void) {
    memset(bullets, 0, sizeof(bullets));
    p1_hp = 5; p2_hp = 5; p1_hold = 0; p2_hold = 0;
}

void Spawn_Bullet(uint8_t owner, uint8_t heavy) {
    for (int i = 0; i < 16; i++) {
        if (!bullets[i].active) {
            bullets[i] = (Bullet_t){1, owner, heavy, (owner == 1) ? 0.0f : (LED_NUM - 1),
                                    (owner == 1) ? (heavy ? 0.45f : 0.75f) : (heavy ? -0.45f : -0.75f)};
            heavy ? SFX_Play_Heavy() : SFX_Play_Shoot();
            break;
        }
    }
}

void Game1_Tick(uint8_t p1_down, uint8_t p2_down) {
    if (p1_hp == 0 || p2_hp == 0) { SFX_Play_Win(); Game1_Init(); return; }

    if (p1_down) p1_hold++;
    else if (p1_hold > 0) {
        if (p1_hold >= 30) Spawn_Bullet(1, 1);
        else if (p1_hold >= 2) Spawn_Bullet(1, 0);
        p1_hold = 0;
    }
    if (p2_down) p2_hold++;
    else if (p2_hold > 0) {
        if (p2_hold >= 30) Spawn_Bullet(2, 1);
        else if (p2_hold >= 2) Spawn_Bullet(2, 0);
        p2_hold = 0;
    }

    for (int i = 0; i < 16; i++) {
        if (!bullets[i].active) continue;
        bullets[i].pos += bullets[i].vel;
        if (bullets[i].owner == 1 && bullets[i].pos >= LED_NUM - 1) { bullets[i].active = 0; p2_hp--; SFX_Play_Hit(); }
        else if (bullets[i].owner == 2 && bullets[i].pos <= 0) { bullets[i].active = 0; p1_hp--; SFX_Play_Hit(); }
    }

    for (int i = 0; i < 16; i++) {
        if (!bullets[i].active) continue;
        for (int j = i + 1; j < 16; j++) {
            if (!bullets[j].active || bullets[i].owner == bullets[j].owner) continue;
            if (fabsf(bullets[i].pos - bullets[j].pos) < 1.2f) {
                if (bullets[i].is_heavy == bullets[j].is_heavy) { bullets[i].active = 0; bullets[j].active = 0; SFX_Play_Hit(); }
                else if (bullets[i].is_heavy) { bullets[j].active = 0; }
                else { bullets[i].active = 0; }
            }
        }
    }

    // 蓄力提示微光
    if (p1_hold > 5) Add_Pixel(0, (p1_hold >= 30) ? 255 : p1_hold * 8, 255, 255);
    if (p2_hold > 5) Add_Pixel(LED_NUM - 1, 255, (p2_hold >= 30) ? 255 : p2_hold * 8, 255);

    for (int i = 0; i < p1_hp; i++) Add_Pixel(i, 0, 40, 120);
    for (int i = 0; i < p2_hp; i++) Add_Pixel(LED_NUM - 1 - i, 120, 20, 0);
    for (int i = 0; i < 16; i++) {
        if (!bullets[i].active) continue;
        uint8_t r = (bullets[i].owner == 1) ? 0 : 255;
        uint8_t g = bullets[i].is_heavy ? 180 : 30;
        uint8_t b = (bullets[i].owner == 1) ? 255 : 0;
        Draw_Subpixel(bullets[i].pos, r, g, b);
    }
}

// ================= 4. 游戏二：《拆弹轮盘》 =================
float ball_pos, ball_vel;
uint8_t safe_radius, last_btn;

void Game2_Init(void) {
    ball_pos = 0.0f; ball_vel = 0.55f; safe_radius = 4; last_btn = 0;
}

void Game2_Tick(uint8_t p1_down, uint8_t p2_down) {
    uint8_t btn = (p1_down || p2_down);
    uint8_t edge = (btn && !last_btn);
    last_btn = btn;

    ball_pos += ball_vel;
    if (ball_pos >= LED_NUM - 1) { ball_pos = LED_NUM - 1; ball_vel = -ball_vel; }
    if (ball_pos <= 0)           { ball_pos = 0;           ball_vel = -ball_vel; }

    int center = LED_NUM / 2;
    if (edge) {
        if (fabsf(ball_pos - center) <= safe_radius) {
            ball_vel = (ball_vel > 0 ? 1.0f : -1.0f) * (fabsf(ball_vel) + 0.12f);
            if (safe_radius > 1) safe_radius--;
            SFX_Play_Win();
        } else {
            SFX_Play_Hit();
            Game2_Init();
            return;
        }
    }

    for (int i = center - safe_radius; i <= center + safe_radius; i++) Add_Pixel(i, 0, 120, 20);
    Draw_Subpixel(ball_pos, 255, 60, 0);
}

// ================= 5. 游戏三：《狭路相逢 (1D跑酷)》 =================
uint8_t jump_timer, last_p1, last_p2;
float laser_pos;
typedef struct { uint8_t active; float pos, speed; } Enemy_t;
Enemy_t enemies[6];
uint16_t spawn_timer;

void Game3_Init(void) {
    memset(enemies, 0, sizeof(enemies));
    jump_timer = 0; laser_pos = -1.0f; spawn_timer = 30;
}

void Game3_Tick(uint8_t p1_down, uint8_t p2_down) {
    if (p1_down && !last_p1 && jump_timer == 0) { jump_timer = 22; SFX_Play_Jump(); }
    if (p2_down && !last_p2 && laser_pos < 0)   { laser_pos = 4.0f; SFX_Play_Shoot(); }
    last_p1 = p1_down; last_p2 = p2_down;

    if (jump_timer > 0) jump_timer--;
    if (laser_pos >= 0) {
        laser_pos += 1.4f;
        if (laser_pos >= LED_NUM) laser_pos = -1.0f;
    }

    if (--spawn_timer == 0) {
        spawn_timer = 45 + (rand() % 40);
        for (int i = 0; i < 6; i++) {
            if (!enemies[i].active) {
                enemies[i] = (Enemy_t){1, (float)(LED_NUM - 1), 0.35f + (rand() % 10) * 0.03f};
                break;
            }
        }
    }

    for (int i = 0; i < 6; i++) {
        if (!enemies[i].active) continue;
        enemies[i].pos -= enemies[i].speed;

        if (laser_pos >= 0 && fabsf(enemies[i].pos - laser_pos) < 1.5f) {
            enemies[i].active = 0; laser_pos = -1.0f; SFX_Play_Hit();
            continue;
        }
        if (fabsf(enemies[i].pos - 3.0f) < 0.8f) {
            if (jump_timer == 0) { SFX_Play_Hit(); Game3_Init(); return; }
        }
        if (enemies[i].pos < 0) enemies[i].active = 0;
        else Draw_Subpixel(enemies[i].pos, 255, 0, 0);
    }

    if (jump_timer > 0) Add_Pixel(3, 0, 255, 255);
    else                Add_Pixel(3, 255, 255, 255);
    if (laser_pos >= 0) Draw_Subpixel(laser_pos, 255, 220, 0);
}

// ================= 6. 系统初始化与主循环 =================
void System_Hardware_Init(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = P1_KEY_PIN | P2_KEY_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseStructure.TIM_Period = 166 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 4800 - 1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    TIM_Cmd(TIM2, ENABLE);

    Buzzer_PWM_Init();
    PIOC_WS2812_Init();
}

void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        frame_flag = 1;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

void Switch_To_Mode(GameMode_t mode) {
    current_mode = mode;
    SFX_Play_Switch();
    if (mode == GAME_TUG_OF_WAR)  Game1_Init();
    if (mode == GAME_BOMB_DEFUSE) Game2_Init();
    if (mode == GAME_1D_RUNNER)   Game3_Init();
}

int main(void) {
    System_Hardware_Init();
    Switch_To_Mode(GAME_TUG_OF_WAR);

    while (1) {
        if (frame_flag) {
            frame_flag = 0;

            uint8_t p1 = (GPIO_ReadInputDataBit(GPIOA, P1_KEY_PIN) == Bit_RESET);
            uint8_t p2 = (GPIO_ReadInputDataBit(GPIOA, P2_KEY_PIN) == Bit_RESET);

            // 检测双键同时长按 1.5 秒 (90 帧) 切换游戏
            if (p1 && p2) {
                both_hold_ticks++;
                if (both_hold_ticks == 90) {
                    Switch_To_Mode((current_mode + 1) % GAME_MODE_COUNT);
                    both_hold_ticks = 0;
                    continue;
                }
            } else {
                both_hold_ticks = 0;
            }

            memset(led_buf, 0, sizeof(led_buf));

            switch (current_mode) {
                case GAME_TUG_OF_WAR:  Game1_Tick(p1, p2); break;
                case GAME_BOMB_DEFUSE: Game2_Tick(p1, p2); break;
                case GAME_1D_RUNNER:   Game3_Tick(p1, p2); break;
                default: break;
            }

            SFX_Update();
            PIOC_WS2812_Send(led_buf, LED_NUM);
        }
    }
}
```
