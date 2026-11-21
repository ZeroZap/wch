/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Description        : WS2812 strip arcade, three games on a 1D strip.
 *
 * Ported from CH32X033_WS2812_Arcade_Games.md (energy duel / bomb defuse /
 * one-dimensional runner) onto this repository's infrastructure:
 *
 *   - WS2812 output : PIOC 1-wire engine from WS2812_Panel_IAP/User/ws2812.c
 *                     (zero CPU while shifting, current limit, dithering)
 *   - layout        : application half of IAP_USB_SIGNED, linked at 0x2800
 *   - USB           : CDC, protocol in strip_usb.c (INFO / SET_GAME /
 *                     SET_BRIGHT / UPGRADE), so firmware updates need no tool
 *                     other than iap_flash.py
 *
 * Hardware (see board_config.h):
 *   WS2812 DIN  LQFP48 PC7 / TSSOP20 PC18
 *   P1 button   PA0, pull-up, press to GND
 *   P2 button   PA1, pull-up, press to GND
 *   Buzzer      PA2, TIM2_CH3 (passive buzzer, 50% square wave)
 *
 * Frame timing: TIM3 gives a 1 ms tick (wsp_tick), the loop runs every
 * GAME_FRAME_MS (16 ms at 60 fps).  TIM2 is free for the buzzer PWM.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "game_config.h"
#include "ws2812.h"
#include "wsp_tick.h"
#include "strip_usb.h"

#define LED_NUM                 GAME_LEDS
#define BRIGHTNESS              GAME_BRIGHTNESS

/*---------------------------------------------------------------------------
 * Game state
 *---------------------------------------------------------------------------*/
typedef enum {
    GAME_TUG_OF_WAR = 0,
    GAME_BOMB_DEFUSE,
    GAME_1D_RUNNER,
    GAME_MODE_COUNT
} GameMode_t;

static GameMode_t current_mode = GAME_TUG_OF_WAR;
static uint16_t   both_hold_ticks;     /* frames both buttons have been held */
static uint8_t    switch_latched;      /* one switch per press, released to rearm */

static const char *const game_names[GAME_MODE_COUNT] = {
    "energy duel", "bomb defuse", "1D runner"
};

uint8_t game_mode(void)
{
    return (uint8_t)current_mode;
}

const char *game_name(uint8_t mode)
{
    return (mode < GAME_MODE_COUNT) ? game_names[mode] : "?";
}

/*---------------------------------------------------------------------------
 * Buzzer: TIM2_CH3 on PA2, 1 MHz counting clock, ARR sets the note
 *---------------------------------------------------------------------------*/
typedef struct {
    uint16_t freq;      /* 0 = silence */
    uint8_t  frames;    /* 1 frame = GAME_FRAME_MS */
} Note_t;

#define SFX_QUEUE_LEN   8
static Note_t  sfx_queue[SFX_QUEUE_LEN];
static uint8_t sfx_head, sfx_tail, sfx_timer;

static void buzzer_init(void)
{
    GPIO_InitTypeDef        gpio = {0};
    TIM_OCInitTypeDef       oc = {0};
    TIM_TimeBaseInitTypeDef tb = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    gpio.GPIO_Pin   = WSP_BUZZER_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(WSP_BUZZER_PORT, &gpio);

    /* 48 MHz / 48 = 1 MHz count clock */
    tb.TIM_Period       = 1000 - 1;
    tb.TIM_Prescaler    = 48 - 1;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_CounterMode  = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tb);

    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 0;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC3Init(TIM2, &oc);

    TIM_CtrlPWMOutputs(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

static void buzzer_set_freq(uint16_t freq)
{
    if (freq == 0u) {
        TIM_SetCompare3(TIM2, 0);
    } else {
        uint16_t arr = (uint16_t)((1000000u / freq) - 1u);
        TIM_SetAutoreload(TIM2, arr);
        TIM_SetCompare3(TIM2, (uint16_t)(arr / 2u));
    }
}

static void sfx_push(uint16_t freq, uint8_t frames)
{
    uint8_t next = (uint8_t)((sfx_tail + 1u) % SFX_QUEUE_LEN);

    if (next != sfx_head) {
        sfx_queue[sfx_tail].freq   = freq;
        sfx_queue[sfx_tail].frames = frames;
        sfx_tail = next;
    }
}

static void sfx_shoot(void)  { sfx_push(880, 2); sfx_push(1320, 2); }
static void sfx_heavy(void)  { sfx_push(300, 3); sfx_push(450, 3); sfx_push(700, 4); }
static void sfx_hit(void)    { sfx_push(200, 3); sfx_push(120, 4); }
static void sfx_jump(void)   { sfx_push(520, 2); sfx_push(780, 3); }
static void sfx_switch(void) { sfx_push(1000, 3); sfx_push(0, 2); sfx_push(1500, 4); }

static void sfx_win(void)
{
    sfx_push(523, 4);
    sfx_push(659, 4);
    sfx_push(784, 4);
    sfx_push(1046, 8);
}

static void sfx_update(void)
{
    if (sfx_timer > 0u) {
        sfx_timer--;
        if (sfx_timer == 0u) {
            buzzer_set_freq(0);
        }
    } else if (sfx_head != sfx_tail) {
        Note_t n = sfx_queue[sfx_head];
        sfx_head = (uint8_t)((sfx_head + 1u) % SFX_QUEUE_LEN);
        buzzer_set_freq(n.freq);
        sfx_timer = n.frames;
    }
}

/*---------------------------------------------------------------------------
 * Render helpers
 *---------------------------------------------------------------------------*/
static uint8_t led_buf[LED_NUM * 3u];

static void add_pixel(int idx, uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t nr, ng, nb;

    if ((idx < 0) || (idx >= (int)LED_NUM)) {
        return;
    }

    nr = (uint16_t)(led_buf[idx * 3 + 0] + (uint8_t)(r * BRIGHTNESS));
    ng = (uint16_t)(led_buf[idx * 3 + 1] + (uint8_t)(g * BRIGHTNESS));
    nb = (uint16_t)(led_buf[idx * 3 + 2] + (uint8_t)(b * BRIGHTNESS));

    led_buf[idx * 3 + 0] = (nr > 255u) ? 255u : (uint8_t)nr;
    led_buf[idx * 3 + 1] = (ng > 255u) ? 255u : (uint8_t)ng;
    led_buf[idx * 3 + 2] = (nb > 255u) ? 255u : (uint8_t)nb;
}

/* Anti-aliased dot: split between the two nearest LEDs */
static void draw_subpixel(float pos, uint8_t r, uint8_t g, uint8_t b)
{
    int   base = (int)pos;
    float frac = pos - (float)base;

    add_pixel(base, (uint8_t)(r * (1.0f - frac)),
                    (uint8_t)(g * (1.0f - frac)),
                    (uint8_t)(b * (1.0f - frac)));
    add_pixel(base + 1, (uint8_t)(r * frac),
                        (uint8_t)(g * frac),
                        (uint8_t)(b * frac));
}

/*---------------------------------------------------------------------------
 * Game 1: energy duel (two players, bullets cancel, charged shots pierce)
 *---------------------------------------------------------------------------*/
typedef struct {
    uint8_t active, owner, is_heavy;
    float   pos, vel;
} Bullet_t;

static Bullet_t bullets[16];
static uint8_t  p1_hp, p2_hp;
static uint16_t p1_hold, p2_hold;

static void game1_init(void)
{
    memset(bullets, 0, sizeof(bullets));
    p1_hp = 5u; p2_hp = 5u; p1_hold = 0u; p2_hold = 0u;
}

static void spawn_bullet(uint8_t owner, uint8_t heavy)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (!bullets[i].active) {
            bullets[i].active   = 1u;
            bullets[i].owner    = owner;
            bullets[i].is_heavy = heavy;
            bullets[i].pos      = (owner == 1u) ? 0.0f : (float)(LED_NUM - 1);
            bullets[i].vel      = (owner == 1u) ? (heavy ? 0.45f : 0.75f)
                                                : (heavy ? -0.45f : -0.75f);
            if (heavy) {
                sfx_heavy();
            } else {
                sfx_shoot();
            }
            break;
        }
    }
}

static void game1_tick(uint8_t p1_down, uint8_t p2_down)
{
    int i, j;

    if ((p1_hp == 0u) || (p2_hp == 0u)) {
        sfx_win();
        game1_init();
        return;
    }

    if (p1_down) {
        p1_hold++;
    } else if (p1_hold > 0u) {
        if (p1_hold >= GAME_CHARGE_FRAMES) {
            spawn_bullet(1, 1);
        } else if (p1_hold >= 2u) {
            spawn_bullet(1, 0);
        }
        p1_hold = 0u;
    }

    if (p2_down) {
        p2_hold++;
    } else if (p2_hold > 0u) {
        if (p2_hold >= GAME_CHARGE_FRAMES) {
            spawn_bullet(2, 1);
        } else if (p2_hold >= 2u) {
            spawn_bullet(2, 0);
        }
        p2_hold = 0u;
    }

    for (i = 0; i < 16; i++) {
        if (!bullets[i].active) {
            continue;
        }
        bullets[i].pos += bullets[i].vel;
        if ((bullets[i].owner == 1u) && (bullets[i].pos >= (float)(LED_NUM - 1))) {
            bullets[i].active = 0u;
            if (p2_hp > 0u) {
                p2_hp--;
            }
            sfx_hit();
        } else if ((bullets[i].owner == 2u) && (bullets[i].pos <= 0.0f)) {
            bullets[i].active = 0u;
            if (p1_hp > 0u) {
                p1_hp--;
            }
            sfx_hit();
        }
    }

    for (i = 0; i < 16; i++) {
        if (!bullets[i].active) {
            continue;
        }
        for (j = i + 1; j < 16; j++) {
            if (!bullets[j].active || (bullets[i].owner == bullets[j].owner)) {
                continue;
            }
            if (fabsf(bullets[i].pos - bullets[j].pos) < 1.2f) {
                if (bullets[i].is_heavy == bullets[j].is_heavy) {
                    bullets[i].active = 0u;
                    bullets[j].active = 0u;
                    sfx_hit();
                } else if (bullets[i].is_heavy) {
                    bullets[j].active = 0u;
                } else {
                    bullets[i].active = 0u;
                }
            }
        }
    }

    /* charge indicator */
    if (p1_hold > 5u) {
        add_pixel(0, (p1_hold >= GAME_CHARGE_FRAMES) ? 255u : (uint8_t)(p1_hold * 8u), 255, 255);
    }
    if (p2_hold > 5u) {
        add_pixel(LED_NUM - 1, 255, (p2_hold >= GAME_CHARGE_FRAMES) ? 255u : (uint8_t)(p2_hold * 8u), 255);
    }

    for (i = 0; i < (int)p1_hp; i++) {
        add_pixel(i, 0, 40, 120);
    }
    for (i = 0; i < (int)p2_hp; i++) {
        add_pixel(LED_NUM - 1 - i, 120, 20, 0);
    }

    for (i = 0; i < 16; i++) {
        if (!bullets[i].active) {
            continue;
        }
        draw_subpixel(bullets[i].pos,
                      (bullets[i].owner == 1u) ? 0u : 255u,
                      bullets[i].is_heavy ? 180u : 30u,
                      (bullets[i].owner == 1u) ? 255u : 0u);
    }
}

/*---------------------------------------------------------------------------
 * Game 2: bomb defuse (press while the ball crosses the green zone)
 *---------------------------------------------------------------------------*/
static float   ball_pos, ball_vel;
static uint8_t safe_radius, last_btn;

static void game2_init(void)
{
    ball_pos = 0.0f;
    ball_vel = 0.55f;
    safe_radius = 4u;
    last_btn = 0u;
}

static void game2_tick(uint8_t p1_down, uint8_t p2_down)
{
    uint8_t btn  = (uint8_t)(p1_down || p2_down);
    uint8_t edge = (uint8_t)(btn && !last_btn);
    int     center = (int)(LED_NUM / 2u);
    int     i;

    last_btn = btn;

    ball_pos += ball_vel;
    if (ball_pos >= (float)(LED_NUM - 1)) {
        ball_pos = (float)(LED_NUM - 1);
        ball_vel = -ball_vel;
    }
    if (ball_pos <= 0.0f) {
        ball_pos = 0.0f;
        ball_vel = -ball_vel;
    }

    if (edge) {
        if (fabsf(ball_pos - (float)center) <= (float)safe_radius) {
            float speed = fabsf(ball_vel) + 0.12f;
            /* cap the speed: beyond ~1.6 LEDs per frame the ball can skip the
             * green zone entirely and the game becomes unwinnable */
            if (speed > 1.6f) {
                speed = 1.6f;
            }
            ball_vel = ((ball_vel > 0.0f) ? 1.0f : -1.0f) * speed;
            if (safe_radius > 1u) {
                safe_radius--;
            }
            sfx_win();
        } else {
            sfx_hit();
            game2_init();
            return;
        }
    }

    for (i = center - (int)safe_radius; i <= center + (int)safe_radius; i++) {
        add_pixel(i, 0, 120, 20);
    }
    draw_subpixel(ball_pos, 255, 60, 0);
}

/*---------------------------------------------------------------------------
 * Game 3: 1D runner (P1 jumps, P2 shoots)
 *---------------------------------------------------------------------------*/
typedef struct { uint8_t active; float pos, speed; } Enemy_t;

static Enemy_t  enemies[6];
static uint8_t  jump_timer, last_p1, last_p2;
static float    laser_pos;
static uint16_t spawn_timer;

static void game3_init(void)
{
    memset(enemies, 0, sizeof(enemies));
    jump_timer = 0u;
    last_p1 = 0u;
    last_p2 = 0u;
    laser_pos = -1.0f;
    spawn_timer = 30u;
}

static void game3_tick(uint8_t p1_down, uint8_t p2_down)
{
    int i;

    if (p1_down && !last_p1 && (jump_timer == 0u)) {
        jump_timer = 22u;
        sfx_jump();
    }
    if (p2_down && !last_p2 && (laser_pos < 0.0f)) {
        laser_pos = 4.0f;
        sfx_shoot();
    }
    last_p1 = p1_down;
    last_p2 = p2_down;

    if (jump_timer > 0u) {
        jump_timer--;
    }
    if (laser_pos >= 0.0f) {
        laser_pos += 1.4f;
        if (laser_pos >= (float)LED_NUM) {
            laser_pos = -1.0f;
        }
    }

    if (--spawn_timer == 0u) {
        spawn_timer = (uint16_t)(45 + (rand() % 40));
        for (i = 0; i < 6; i++) {
            if (!enemies[i].active) {
                enemies[i].active = 1u;
                enemies[i].pos    = (float)(LED_NUM - 1);
                enemies[i].speed  = 0.35f + (float)(rand() % 10) * 0.03f;
                break;
            }
        }
    }

    for (i = 0; i < 6; i++) {
        if (!enemies[i].active) {
            continue;
        }
        enemies[i].pos -= enemies[i].speed;

        if ((laser_pos >= 0.0f) && (fabsf(enemies[i].pos - laser_pos) < 1.5f)) {
            enemies[i].active = 0u;
            laser_pos = -1.0f;
            sfx_hit();
            continue;
        }
        if (fabsf(enemies[i].pos - 3.0f) < 0.8f) {
            if (jump_timer == 0u) {
                sfx_hit();
                game3_init();
                return;
            }
        }
        if (enemies[i].pos < 0.0f) {
            enemies[i].active = 0u;
        } else {
            draw_subpixel(enemies[i].pos, 255, 0, 0);
        }
    }

    if (jump_timer > 0u) {
        add_pixel(3, 0, 255, 255);
    } else {
        add_pixel(3, 255, 255, 255);
    }
    if (laser_pos >= 0.0f) {
        draw_subpixel(laser_pos, 255, 220, 0);
    }
}

/*---------------------------------------------------------------------------
 * Mode switching
 *---------------------------------------------------------------------------*/
void game_select(uint8_t mode)
{
    if (mode >= GAME_MODE_COUNT) {
        return;
    }

    current_mode = (GameMode_t)mode;
    sfx_switch();

    switch (current_mode) {
    case GAME_TUG_OF_WAR:  game1_init(); break;
    case GAME_BOMB_DEFUSE: game2_init(); break;
    default:               game3_init(); break;
    }

    printf("mode: %u (%s)\r\n", (unsigned)mode, game_name(mode));
}

/*---------------------------------------------------------------------------
 * main
 *---------------------------------------------------------------------------*/
int main(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint32_t         next_frame;

    SystemCoreClockUpdate();
    Delay_Init();
#if WSP_DEBUG_UART
    USART_Printf_Init(115200);
#endif
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    tick_init();

    /* both buttons: pull-up, press to GND */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin  = WSP_P1_PIN | WSP_P2_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(WSP_P1_PORT, &gpio);

    buzzer_init();
    ws2812_init();

    srand(tick_ms());

    printf("\r\nStrip games on %s: %u LEDs, %u fps\r\n",
           WSP_BOARD_NAME, (unsigned)GAME_LEDS, (unsigned)GAME_FPS);

    strip_usb_init();
    game_select(GAME_TUG_OF_WAR);

    next_frame = tick_ms();

    while (1) {
        uint8_t p1, p2;
        uint32_t now = tick_ms();

        /* USB commands: switch game, brightness, upgrade request */
        strip_usb_service();

        if ((now - next_frame) < GAME_FRAME_MS) {
            continue;
        }
        /* a long USB reply or a flash write can stall the loop; resync instead
         * of replaying every missed frame back to back */
        if ((uint32_t)(now - next_frame) > (4u * GAME_FRAME_MS)) {
            next_frame = now;
        }
        next_frame += GAME_FRAME_MS;

        p1 = (uint8_t)(GPIO_ReadInputDataBit(WSP_P1_PORT, WSP_P1_PIN) == Bit_RESET);
        p2 = (uint8_t)(GPIO_ReadInputDataBit(WSP_P2_PORT, WSP_P2_PIN) == Bit_RESET);

        /* Both buttons held: next game.  5 s is 10x the charge threshold, so a
         * normal hold-and-fire in game 1 never reaches it.  The hold is silent:
         * the game keeps rendering and playing exactly as usual, it only fires
         * once the full 5 s has elapsed.  One switch per press; release both to
         * rearm. */
        if (p1 && p2) {
            if (!switch_latched) {
                both_hold_ticks++;
                if (both_hold_ticks >= GAME_SWITCH_FRAMES) {
                    game_select((uint8_t)((current_mode + 1u) % GAME_MODE_COUNT));
                    switch_latched = 1u;
                    continue;
                }
            }
        } else {
            both_hold_ticks = 0u;
            switch_latched = 0u;
        }

        memset(led_buf, 0, sizeof(led_buf));

        switch (current_mode) {
        case GAME_TUG_OF_WAR:  game1_tick(p1, p2); break;
        case GAME_BOMB_DEFUSE: game2_tick(p1, p2); break;
        default:               game3_tick(p1, p2); break;
        }

        sfx_update();

        ws2812_show(led_buf, LED_NUM);
    }
}
