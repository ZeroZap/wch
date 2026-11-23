/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_runtime.h
 * Description        : Shared runtime for a NOR cartridge game image.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __GAME_RUNTIME_H
#define __GAME_RUNTIME_H

#include "game.h"

void game_runtime_run(const game_desc_t *g) __attribute__((noreturn));

#endif /* __GAME_RUNTIME_H */
