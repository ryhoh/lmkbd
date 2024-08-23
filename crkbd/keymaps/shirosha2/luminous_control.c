/********************************************************************************************************************************/
/*  luminous_control.c                                                                                                          */
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

/********************************************************************************************************************************/
/*  Luminous Control Function Number Assignments                                                                                */
/*                                                                                                                              */
/*       #1xx: Management of the entire luminous control                                                                        */
/*       #2xx: RGB LED control (Startup)                                                                                        */
/*                                                                                                                              */
/*      #12xx: OLED display control (Startup)                                                                                   */
/*      #13xx: OLED display control (Main Key Pressed)                                                                          */
/*      #19xx: OLED display control (Main Easter Egg)                                                                           */
/*                                                                                                                              */
/********************************************************************************************************************************/

/********************************************************************************************************************************/
/* Includes                                                                                                                     */
/********************************************************************************************************************************/
#include "luminous_control.h"
#include "luminous_common.h"
#include QMK_KEYBOARD_H
#include <stdio.h>

/********************************************************************************************************************************/
/*  Typedefs                                                                                                                    */
/********************************************************************************************************************************/
typedef struct {
    uint32_t ul_app_timestamp;          /* [ms,64] Timestamp on the application side    */
    uint8_t uc_master_mode_flg;         /* Master mode flag                             */
    uint8_t uc_lmctl_state;             /* Luminous control state                       */
    uint16_t us_layer_state;            /* Layer state                                  */
} lmctl_context_t;

/********************************************************************************************************************************/
/*  Defines                                                                                                                     */
/********************************************************************************************************************************/
#define Y_LMCTL_TASK_INTERVAL   (64)        /* [ms,64] 64ms                                           */
#define Y_LMCTL_STARTUP_TIME    (64)        /* [ms,64] 64 * Y_LMCTL_TASK_INTERVAL(64) = 4.096s        */

#define Y_LMCTL_STATE_INIT      (0x00)      /* Initialization state (t = 0)                           */
#define Y_LMCTL_STATE_STARTUP   (0x01)      /* Startup state (t < Y_LMCTL_STARTUP_TIME)               */
#define Y_LMCTL_STATE_IGNITION  (0x02)      /* Ignition state (t = Y_LMCTL_STARTUP_TIME)              */
#define Y_LMCTL_STATE_RUNNING   (0x03)      /* Running state (t > Y_LMCTL_STARTUP_TIME)               */

#define Y_LMCTL_LAYER_BASE      (0x00)      /* Base layer                                             */
#define Y_LMCTL_LAYER_LOWER     (Y_BIT1)    /* Lower layer                                            */
#define Y_LMCTL_LAYER_RAISE     (Y_BIT2)    /* Raise layer                                            */
#define Y_LMCTL_LAYER_ADJUST    (Y_BIT3)    /* Adjust layer                                           */

/********************************************************************************************************************************/
/*  Variables                                                                                                                   */
/********************************************************************************************************************************/
#if 0
const static char Xc_LMCTL_qmk_code_to_char[57][4] = {
/*              0x00   0x01    0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0A   0x0B   0x0C   0x0D   0x0E   0x0F  */
/*  0x00  */    "   ", "   ",  "   ", "   ", "a  ", "b  ", "c  ", "d  ", "e"  , "f  ", "g  ", "h  ", "i  ", "j  ", "k  ", "l  ",
/*  0x10  */    "m  ", "n  ",  "o  ", "p  ", "q  ", "r  ", "s  ", "t  ", "u  ", "v  ", "w  ", "x  ", "y  ", "z  ", "1  ", "2  ",
/*  0x20  */    "3  ", "4  ",  "5  ", "6  ", "7  ", "8  ", "9  ", "0  ", "Ret", "Esc", "Bsp", "Tab", "Sp ", "-  ", "=  ", "[  ",
/*  0x30  */    "]  ", "\\  ", "#  ", ";  ", "'  ", "`  ", ",  ", ".  ", "/  "
};
#endif

static layer_state_t zus_LMCTL_layer_state = Y_LMCTL_LAYER_BASE;    /* [-,-] Layer state for the luminous control    */
static uint16_t zus_LMCTL_last_keycode = 0;                         /* [-,-] Last keycode                            */
static uint8_t zuc_LMCTL_insp_mode_flg = Y_OFF;                     /* [-,-] Inspection mode flag                    */

/********************************************************************************************************************************/
/*  Functions                                                                                                                   */
/********************************************************************************************************************************/
static void m_lmctl_data_latch_main(void);
static void m_lmctl_100_context_management(lmctl_context_t *pst_lmctl_context);
static void m_lmctl_101_judge_state(lmctl_context_t *pst_lmctl_context);
static void m_lmctl_oled_main(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_oled_main_insp(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_1200_oled_startup_logo(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_1300_oled_current_layer(const lmctl_context_t *pst_lmctl_context);

/****************************************************************/
/*  m_lmctl_main                                                */
/*--------------------------------------------------------------*/
/*  Main function for the luminous control.                     */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters:                                                 */
/*  Returns:                                                    */
/****************************************************************/
void m_lmctl_main(void) {
    static lmctl_context_t zst_lmctl_context = {0};             /* LMCTL_Context            */

    m_lmctl_data_latch_main();                                  /* Data latch (main)        */

    if (zuc_LMCTL_insp_mode_flg == Y_ON) {                      /* Inspection mode          */

#ifdef OLED_DRIVER_ENABLE
        m_lmctl_oled_main_insp(&zst_lmctl_context);             /* OLED Main (insp)         */
#endif /* OLED_DRIVER_ENABLE */
        
    } else {                                                    /* Normal mode              */
        m_lmctl_100_context_management(&zst_lmctl_context);     /* (#100) Management        */
        m_lmctl_101_judge_state(&zst_lmctl_context);            /* (#101) Judge state       */

#ifdef OLED_DRIVER_ENABLE
        m_lmctl_oled_main(&zst_lmctl_context);                  /* OLED Main                */
#endif /* OLED_DRIVER_ENABLE */

    }

}

/****************************************************************/
/*  m_lmctl_data_latch_main                                     */
/*--------------------------------------------------------------*/
/*  Data latch function for the luminous control.               */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters:                                                 */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_data_latch_main(void) {
    zus_LMCTL_layer_state = layer_state;                       /* Update the layer state   */
}

/****************************************************************/
/*  m_lmctl_100_context_management                              */
/*--------------------------------------------------------------*/
/*  Management function for the luminous control.               */
/*      (Luminous Control #100)                                 */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <LMCTL_Context>                                 */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_100_context_management(lmctl_context_t *pst_lmctl_context) {
    uint32_t ul_app_timestamp = pst_lmctl_context->ul_app_timestamp;    /* [ms,64] System timestamp         */
    uint8_t uc_master_mode_flg;                                         /* Master mode flag                 */

    {
        /* System timestamp     */
        M_CLIP_INC(ul_app_timestamp, UINT32_MAX)                        /* Increment the system timestamp   */

        /* Master mode flag     */
        if (is_keyboard_master()) {
            uc_master_mode_flg = Y_ON;                                  /* Master mode flag ON              */
        } else {
            uc_master_mode_flg = Y_OFF;                                 /* Master mode flag OFF             */
        }
    }

    pst_lmctl_context->ul_app_timestamp = ul_app_timestamp;             /* Update the system timestamp      */
    pst_lmctl_context->uc_master_mode_flg = uc_master_mode_flg;         /* Update the master mode flag      */
    pst_lmctl_context->us_layer_state = zus_LMCTL_layer_state;      /* Update the layer state           */
}

/****************************************************************/
/*  m_lmctl_101_judge_state                                     */
/*--------------------------------------------------------------*/
/*  Judge the luminous control state.                           */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <LMCTL_Context>                                 */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_101_judge_state(lmctl_context_t *pst_lmctl_context) {
    const uint32_t xul_app_timestamp = pst_lmctl_context->ul_app_timestamp;     /* [ms,64] App timestamp     */
    uint8_t uc_lmctl_state;                                                     /* Luminous control state    */

    {
        if (xul_app_timestamp == 0) {
            uc_lmctl_state = Y_LMCTL_STATE_INIT;                                /* Initialization state      */
        } else if (xul_app_timestamp < Y_LMCTL_STARTUP_TIME) {
            uc_lmctl_state = Y_LMCTL_STATE_STARTUP;                             /* Startup state             */
        } else if (xul_app_timestamp == Y_LMCTL_STARTUP_TIME) {
            uc_lmctl_state = Y_LMCTL_STATE_IGNITION;                            /* Ignition state            */
        } else /* if (xul_app_timestamp > Y_LMCTL_STARTUP_TIME) */ {
            uc_lmctl_state = Y_LMCTL_STATE_RUNNING;                             /* Running state             */
        }
    }

    pst_lmctl_context->uc_lmctl_state = uc_lmctl_state;                         /* Update the system state   */
}

#if (OLED_DRIVER_ENABLE == 1)
/****************************************************************/
/*  m_lmctl_oled_main                                           */
/*--------------------------------------------------------------*/
/*  Main function for the oled control.                         */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <LMCTL_Context>                                 */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_oled_main(const lmctl_context_t *pst_lmctl_context) {
    m_lmctl_1200_oled_startup_logo(pst_lmctl_context);                  /* (#1100) Startup logo display             */
    m_lmctl_1300_oled_current_layer(pst_lmctl_context);                 /* (#1300) Current layer display            */
}

/****************************************************************/
/*  m_lmctl_oled_main_insp                                      */
/*--------------------------------------------------------------*/
/*  Main function for the oled control in the inspection mode.  */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <LMCTL_Context>                                 */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_oled_main_insp(const lmctl_context_t *pst_lmctl_context) {
    oled_write_ln_P(PSTR("[Inspection]"), false);

    m_lmctl_1200_oled_startup_logo(pst_lmctl_context);                  /* (#1100) Startup logo display             */
    m_lmctl_1300_oled_current_layer(pst_lmctl_context);                 /* (#1300) Current layer display            */
}

/****************************************************************/
/*  m_lmctl_1200_oled_startup_logo                              */
/*--------------------------------------------------------------*/
/*  Display the startup logo on the OLED.                       */
/*      (Luminous Control #1200)                                */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <LMCTL_Context>                                 */
/*  Returns:                                                    */
/*--------------------------------------------------------------*/
/*  Requirements:  Declare the following in the keymap.c        */
/*      - const char PROGMEM Xc_logo_indices[]                  */
/****************************************************************/
static void m_lmctl_1200_oled_startup_logo(const lmctl_context_t *pst_lmctl_context) {
    const uint8_t xuc_lmctl_state = pst_lmctl_context->uc_lmctl_state;  /* Luminous control state   */

    {
        if ((xuc_lmctl_state == Y_LMCTL_STATE_INIT)
         || (xuc_lmctl_state == Y_LMCTL_STATE_STARTUP)) {
            oled_write_P(Xc_logo_indices, false);                       /* Display the logo         */
        } else if (xuc_lmctl_state == Y_LMCTL_STATE_IGNITION) {
            oled_clear();                                               /* Clear the display        */
        } else /* if (xuc_lmctl_state == Y_LMCTL_STATE_RUNNING) */ {
            /* Do nothing */                                            /* Do nothing               */
        }
    }
}

/****************************************************************/
/*  m_lmctl_1300_oled_current_layer                             */
/*--------------------------------------------------------------*/
/*  Display the current layer on the OLED.                      */
/*      (Luminous Control #1300)                                */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <LMCTL_Context>                                 */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_1300_oled_current_layer(const lmctl_context_t *pst_lmctl_context) {
    const uint16_t xus_layer_state = pst_lmctl_context->us_layer_state;     /* Layer state              */
    const uint8_t xuc_lmctl_state = pst_lmctl_context->uc_lmctl_state;      /* Luminous control state   */

    {
        if (xuc_lmctl_state == Y_LMCTL_STATE_RUNNING) {
            oled_write_ln_P(PSTR("Layer: "), false);
            switch (xus_layer_state) {
                case Y_LMCTL_LAYER_BASE:
                    oled_write_ln_P(PSTR("Default"), false);
                    break;
                case Y_LMCTL_LAYER_LOWER:
                    oled_write_ln_P(PSTR("Lower"), false);
                    break;
                case Y_LMCTL_LAYER_RAISE:
                    oled_write_ln_P(PSTR("Raise"), false);
                    break;
                case Y_LMCTL_LAYER_ADJUST:
                case Y_LMCTL_LAYER_ADJUST | Y_LMCTL_LAYER_LOWER:
                case Y_LMCTL_LAYER_ADJUST | Y_LMCTL_LAYER_RAISE:
                case Y_LMCTL_LAYER_ADJUST | Y_LMCTL_LAYER_LOWER | Y_LMCTL_LAYER_RAISE:
                    oled_write_ln_P(PSTR("Adjust"), false);
                    break;
                default:
                    oled_write_ln_P(PSTR("Undef"), false);
                    break;
            }
        }
    }
}


/****************************************************************/
/*  m_lmctl_record                                              */
/*--------------------------------------------------------------*/
/*  Entry function when key record is updated.                  */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: key record is updated                               */
/*  Parameters: uint16_t keycode, keyrecord_t *record           */
/*  Returns:                                                    */
/****************************************************************/
void m_lmctl_record(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        zus_LMCTL_last_keycode = keycode;                                   /* Update the last keycode  */

        /* Toggle the inspection mode flag  */
        if (keycode == LM_INSP) {
            if (zuc_LMCTL_insp_mode_flg == Y_OFF) {
                zuc_LMCTL_insp_mode_flg = Y_ON;                             /* Inspection mode ON       */
            } else {
                zuc_LMCTL_insp_mode_flg = Y_OFF;                            /* Inspection mode OFF      */
            }
        }
    }
}

#endif /* OLED_DRIVER_ENABLE */
