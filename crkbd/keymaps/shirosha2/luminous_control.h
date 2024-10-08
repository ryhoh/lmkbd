/********************************************************************************************************************************/
/*  luminous_control.h                                                                                                          */
/*                                                                                                                              */
/*  This file is for the luminous control.                                                                                      */
/*      - RGB LED control                                                                                                       */
/*      - OLED display control                                                                                                  */
/********************************************************************************************************************************/

/*
Copyright 2024 ryhoh/shirosha2

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/********************************************************************************************************************************/
/*  Includes                                                                                                                    */
/********************************************************************************************************************************/
#include "luminous_common.h"
#include QMK_KEYBOARD_H
#include <stdio.h>

/********************************************************************************************************************************/
/*  Variables                                                                                                                   */
/********************************************************************************************************************************/
extern const char Xc_logo_indices[];

/********************************************************************************************************************************/
/*  Functions                                                                                                                   */
/********************************************************************************************************************************/
void m_lmctl_main(void);
void m_lmctl_record(uint16_t keycode, keyrecord_t *record);
