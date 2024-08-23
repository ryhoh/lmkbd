/********************************************************************************************************************************/
/*  luminous_common.h                                                                                                           */
/*                                                                                                                              */
/*  This file is for the common control.                                                                                        */
/*                                                                                                                              */
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
/*  defines                                                                                                                     */
/********************************************************************************************************************************/
#define Y_LMCTL_SW_VERSION  "v0.2.0"        /* Software version */
#define Y_ON  (1)                           /* On   */
#define Y_OFF (0)                           /* Off  */
#define Y_BIT0  (0x01U)                     /* Bit 0    */
#define Y_BIT1  (0x02U)                     /* Bit 1    */
#define Y_BIT2  (0x04U)                     /* Bit 2    */
#define Y_BIT3  (0x08U)                     /* Bit 3    */
#define Y_BIT4  (0x10U)                     /* Bit 4    */
#define Y_BIT5  (0x20U)                     /* Bit 5    */
#define Y_BIT6  (0x40U)                     /* Bit 6    */
#define Y_BIT7  (0x80U)                     /* Bit 7    */

#define LM_INSP (QK_USER_0)                 /* Luminous control inspection mode toggle key  */

/********************************************************************************************************************************/
/*  Macros                                                                                                                      */
/********************************************************************************************************************************/

/****************************************************************/
/*  M_CLIP_INC                                                  */
/*--------------------------------------------------------------*/
/*  Increment the value of the variable by the specified amount */
/*                                                              */
/****************************************************************/
#define M_CLIP_INC(value, max)  { (value) = (value) < (max) ? (value) + 1 : (value); }
