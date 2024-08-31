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
/*      #14xx: OLED display control (Main Key Released)                                                                         */
/*      #15xx: OLED display control (Idle)                                                                                      */
/*      #19xx: OLED display control (Main Easter Egg)                                                                           */
/*                                                                                                                              */
/********************************************************************************************************************************/

/********************************************************************************************************************************/
/* Includes                                                                                                                     */
/********************************************************************************************************************************/
#include "luminous_config.h"
#include "luminous_control.h"
#include "luminous_common.h"
#include QMK_KEYBOARD_H
#include <stdio.h>

#ifdef CONSOLE_ENABLE
  #include <print.h>
#endif

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

#ifdef OLED_DRIVER_ENABLE
#define Y_LMCTL_OLED_COL_NUM    (128)       /* Number of columns for the OLED display                 */
#define Y_LMCTL_OLED_ROW_NUM    (4)         /* Number of rows for the OLED display (32 / 8 = 4)       */
#define Y_LMCTL_OLED_UPDATE_INTERVAL (1)    /* [ms,64] 64 * 1 =  64ms                                 */

#define Y_LMCTL_OLED_NO_DIR     (0)         /* No direction                                           */
#define Y_LMCTL_OLED_UP         (1)         /* Up direction                                           */
#define Y_LMCTL_OLED_DOWN       (2)         /* Down direction                                         */
#define Y_LMCTL_OLED_LEFT       (3)         /* Left direction                                         */
#define Y_LMCTL_OLED_RIGHT      (4)         /* Right direction                                        */

#define Y_LMCTL_OLED_STACK_EMPTY    (0)     /* Empty stack index                                      */

#endif /* OLED_DRIVER_ENABLE */

/********************************************************************************************************************************/
/*  Macros                                                                                                                      */
/********************************************************************************************************************************/
#ifdef OLED_DRIVER_ENABLE

/****************************************************************/
/* OLED Buffer Write Macro                                      */
/****************************************************************/
/* Example:                                                     */
/* const static uint8_t Xuc_oled_buffer[Y_LMCTL_OLED_COL_NUM][Y_LMCTL_OLED_ROW_NUM] = {
 *              {0b00000000, 0b00000000, 0b00000000, 0b00000000},
 *              {0b00111000, 0b00000000, 0b00000000, 0b00000000},
 *              {0b01000100, 0b00000000, 0b00000000, 0b00000000},
 *              {0b01111100, 0b00000000, 0b00000000, 0b00000000},
 *              {0b01000100, 0b00000000, 0b00000000, 0b00000000},
 *              {0b01000100, 0b00000000, 0b00000000, 0b00000000},
 *              {0b00000000, 0b00000000, 0b00000000, 0b00000000},
 *              {0b00000000, 0b01111000, 0b00000000, 0b00000000},
 *              {0b00000000, 0b01000100, 0b00000000, 0b00000000},
 *              {0b00000000, 0b01111000, 0b00000000, 0b00000000},
 *              {0b00000000, 0b01000100, 0b00000000, 0b00000000},
 *              {0b00000000, 0b01111000, 0b00000000, 0b00000000},
 *              {0b00000000, 0b00000000, 0b00000000, 0b00000000},
 *              {0b00000000, 0b00000000, 0b00111000, 0b00000000},
 *              {0b00000000, 0b00000000, 0b01000100, 0b00000000},
 *              {0b00000000, 0b00000000, 0b01000000, 0b00000000},
 *              {0b00000000, 0b00000000, 0b01000100, 0b00000000},
 *              {0b00000000, 0b00000000, 0b00111000, 0b00000000},
 *              {0b00000000, 0b00000000, 0b00000000, 0b00000000},
 *              {0b00000000, 0b00000000, 0b00000000, 0b01111000},
 *              {0b00000000, 0b00000000, 0b00000000, 0b01000100},
 *              {0b00000000, 0b00000000, 0b00000000, 0b01000100},
 *              {0b00000000, 0b00000000, 0b00000000, 0b01000100},
 *              {0b00000000, 0b00000000, 0b00000000, 0b01111000},
 *              {0b00000000, 0b00000000, 0b00000000, 0b00000000},
 *              ...
 * };
 * 
 * This macro writes the buffer to the OLED display.
 *     +----+
 *     |    |
 *     |A   |
 *     | B  |
 *     |  C |
 *     |   D|
 *     ...
 * 
 */
#define M_LMCTL_OLED_WRITE_BUFFER(puc_buffer)   { \
    uint16_t us_WRITE_BUFFER_idx = 0; \
    for (uint16_t us_WRITE_BUFFER_row_i = 0; us_WRITE_BUFFER_row_i < Y_LMCTL_OLED_ROW_NUM; us_WRITE_BUFFER_row_i++) { \
        for (uint16_t us_WRITE_BUFFER_col_i = Y_LMCTL_OLED_COL_NUM; us_WRITE_BUFFER_col_i != 0; us_WRITE_BUFFER_col_i--) { \
            uint8_t uc_WRITE_BUFFER_tmp = puc_buffer[us_WRITE_BUFFER_col_i-1][us_WRITE_BUFFER_row_i]; \
            M_REVERSE_BIT_8(uc_WRITE_BUFFER_tmp); \
            oled_write_raw_byte(uc_WRITE_BUFFER_tmp, us_WRITE_BUFFER_idx); \
            us_WRITE_BUFFER_idx++; \
        } \
    } \
}

#define M_LMCTL_OLED_TRANSFER_BUFFER(puc_from, puc_to)   { \
    for (uint16_t us_TRANSFER_BUFFER_row_i = 0; us_TRANSFER_BUFFER_row_i < Y_LMCTL_OLED_ROW_NUM; us_TRANSFER_BUFFER_row_i++) { \
        for (uint16_t us_TRANSFER_BUFFER_col_i = 0; us_TRANSFER_BUFFER_col_i < Y_LMCTL_OLED_COL_NUM; us_TRANSFER_BUFFER_col_i++) { \
            puc_to[us_TRANSFER_BUFFER_col_i][us_TRANSFER_BUFFER_row_i] = puc_from[us_TRANSFER_BUFFER_col_i][us_TRANSFER_BUFFER_row_i]; \
        } \
    } \
}

#define M_LMCTL_OLED_SET_BIT(puc_buffer, uc_x, uc_y)   { \
    uint16_t us_OLED_SET_BIT_byte_idx = uc_x / 8; \
    uint8_t uc_OLED_SET_BIT_bit_idx = uc_x % 8; \
    puc_buffer[uc_y][us_OLED_SET_BIT_byte_idx] |= (1U << (7 - uc_OLED_SET_BIT_bit_idx)); \
}

#define M_LMCTL_OLED_CLEAR_BIT(puc_buffer, uc_x, uc_y)   { \
    uint16_t us_OLED_CLEAR_BIT_byte_idx = uc_x / 8; \
    uint8_t uc_OLED_CLEAR_BIT_bit_idx = uc_x % 8; \
    puc_buffer[uc_y][us_OLED_CLEAR_BIT_byte_idx] &= ~(1U << (7 - uc_OLED_CLEAR_BIT_bit_idx)); \
}

#define M_LMCTL_OLED_GET_BIT(puc_buffer, uc_x, uc_y)   ((puc_buffer[uc_y][uc_x / 8] & (1U << (7 - uc_x % 8))) ? 1 : 0)

#endif /* OLED_DRIVER_ENABLE */

/********************************************************************************************************************************/
/*  Structures                                                                                                                  */
/********************************************************************************************************************************/
#ifdef OLED_DRIVER_ENABLE

typedef struct {
    uint8_t uc_x;          /* X coordinate    */
    uint8_t uc_y;          /* Y coordinate    */
} lmctl_point_t;           /* Point           */

#endif /* OLED_DRIVER_ENABLE */

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

#ifdef OLED_DRIVER_ENABLE
static uint8_t zuc_LMCTL_oled_raw_buffer[Y_LMCTL_OLED_COL_NUM][Y_LMCTL_OLED_ROW_NUM] = {0}; /* [-,-] OLED raw buffer */

#if (LMCTL_1501_LABYRINTH_ENABLE == 1)
// static lmctl_point_t zst_lmctl_1501_point_stack[Y_LMCTL_OLED_COL_NUM * Y_LMCTL_OLED_ROW_NUM] = {0};    /* Point stack              */
static uint8_t zuc_lmctl_1501_point_stack_x[400] = {0};           /* Point stack x            */
static uint8_t zuc_lmctl_1501_point_stack_y[400] = {0};           /* Point stack y            */
static uint16_t zus_lmctl_1501_point_stack_idx = Y_LMCTL_OLED_STACK_EMPTY;                                /* Point stack index        */
// static uint8_t zuc_lmctl_1501_labyrinth[Y_LMCTL_OLED_COL_NUM][Y_LMCTL_OLED_ROW_NUM] = {0};             /* Labyrinth                */
#endif /* LMCTL_1501_LABYRINTH_ENABLE */
#endif /* OLED_DRIVER_ENABLE */

/********************************************************************************************************************************/
/*  Functions                                                                                                                   */
/********************************************************************************************************************************/
static void m_lmctl_data_latch_main(void);
static void m_lmctl_100_context_management(lmctl_context_t *pst_lmctl_context);
static void m_lmctl_101_judge_state(lmctl_context_t *pst_lmctl_context);

#ifdef OLED_DRIVER_ENABLE
static void m_lmctl_oled_main(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_oled_main_insp(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_1200_oled_startup_logo(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_1300_oled_current_layer(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_1500_oled_idle_management(const lmctl_context_t *pst_lmctl_context);
#if (LMCTL_1501_LABYRINTH_ENABLE == 1)
static bool m_lmctl_1501_oled_generate_labirynth(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_1501_oled_generate_labirynth_init(void);
static uint8_t m_lmctl_1501_oled_generate_labirynth_update(void);
#endif /* LMCTL_1501_LABYRINTH_ENABLE */
// static void m_lmctl_oled_init_by_frame(uint8_t puc_buffer[][Y_LMCTL_OLED_ROW_NUM], uint8_t uc_odd_size_flg);

#if (LMCTL_1501_LABYRINTH_ENABLE == 1)  /* or ... */
static void m_lmctl_oled_init_by_fill(uint8_t puc_buffer[][Y_LMCTL_OLED_ROW_NUM]);
#endif /* LMCTL_1501_LABYRINTH_ENABLE */

#endif /* OLED_DRIVER_ENABLE */

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
    // m_lmctl_1300_oled_current_layer(pst_lmctl_context);                 /* (#1300) Current layer display            */

    m_lmctl_1500_oled_idle_management(pst_lmctl_context);
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
    const uint8_t xuc_master_mode_flg = pst_lmctl_context->uc_master_mode_flg;  /* Master mode flag           */

    oled_write_ln_P(PSTR("[Inspection]"), false);
    
    if (xuc_master_mode_flg == Y_ON) {
        /* Master mode  */
        m_lmctl_1300_oled_current_layer(pst_lmctl_context);             /* (#1300) Current layer display            */
    } else {
        /* Slave mode   */
        m_lmctl_1500_oled_idle_management(pst_lmctl_context);           /* (#1500) Idle management                  */
    }
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
/*  m_lmctl_1500_oled_idle_management                           */
/*--------------------------------------------------------------*/
/*  Management function for the idle state of the OLED.         */
/*      (Luminous Control #1500)                                */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <LMCTL_Context>                                 */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_1500_oled_idle_management(const lmctl_context_t *pst_lmctl_context) {
    const uint8_t xuc_lmctl_state = pst_lmctl_context->uc_lmctl_state;      /* Luminous control state   */

    if (xuc_lmctl_state == Y_LMCTL_STATE_RUNNING) {
        /* Update idling actions   */
#if (LMCTL_1501_LABYRINTH_ENABLE == 1)
        (void)m_lmctl_1501_oled_generate_labirynth(pst_lmctl_context);      /* (#1501) Generate labirynth   */
#endif /* LMCTL_1501_LABYRINTH_ENABLE */

        /* Print to the OLED       */
        M_LMCTL_OLED_WRITE_BUFFER(zuc_LMCTL_oled_raw_buffer);               /* Write the buffer to the OLED */
    }
}

#if (LMCTL_1501_LABYRINTH_ENABLE == 1)
/****************************************************************/
/*  m_lmctl_1501_oled_generate_labirynth                        */
/*--------------------------------------------------------------*/
/*  Generate the labirynth on the OLED.                         */
/*      (Luminous Control #1501)                                */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <LMCTL_Context>                                 */
/*  Returns: [bool]<true> if the action is completed            */
/****************************************************************/
static bool m_lmctl_1501_oled_generate_labirynth(const lmctl_context_t *pst_lmctl_context) {
    static uint8_t zuc_initialize_req_flg = Y_ON;                   /* Initialized Request flag */
    static uint8_t zuc_interval_counter = 0;                        /* Interval counter         */
    static uint8_t zuc_end_flag = Y_OFF;                            /* End flag                 */

    if (zuc_initialize_req_flg == Y_ON) {
        /* Initialization   */
        m_lmctl_1501_oled_generate_labirynth_init();                            /* Initialization    */
        zuc_initialize_req_flg = Y_OFF;                                         /* Initialized request flag OFF */
    }

    if (zuc_interval_counter < Y_LMCTL_OLED_UPDATE_INTERVAL) {
        /* Do nothing */
        zuc_interval_counter++;                                                 /* Increment the interval counter */
    } else {
        zuc_end_flag = m_lmctl_1501_oled_generate_labirynth_update();           /* Update the labirynth           */
        zuc_interval_counter = 0;                                               /* Reset the interval counter     */
    }

    if (zuc_end_flag == Y_ON) {
        zuc_initialize_req_flg = Y_ON;                                          /* Initialized request flag ON    */
    }

    // M_LMCTL_OLED_TRANSFER_BUFFER(zuc_lmctl_1501_labyrinth, zuc_LMCTL_oled_raw_buffer);    /* Transfer the buffer  */

    return false;
}

/****************************************************************/
/*  m_lmctl_1501_oled_generate_labirynth_init                   */
/*--------------------------------------------------------------*/
/*  Labirynth initialization function.                          */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period: 64ms                                                */
/*  Parameters: <>                                              */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_1501_oled_generate_labirynth_init(void) {
    lmctl_point_t st_cursor;                                           /* Cursor position          */

    // m_lmctl_oled_init_by_fill(zuc_lmctl_1501_labyrinth);    /* Initialize the labirynth   */
    m_lmctl_oled_init_by_fill(zuc_LMCTL_oled_raw_buffer);    /* Initialize the labirynth   */
    
    /* Init the cursor position by random                   */
    /* Available range: 1 ~ (Y_LMCTL_OLED_ROW_NUM - 2)      */
    /* 0: wall, (Y_LMCTL_OLED_ROW_NUM - 2) and (Y_LMCTL_OLED_ROW_NUM - 1) : wall  */
    st_cursor.uc_x = rand() % (Y_LMCTL_OLED_ROW_NUM * 8 - 3) + 1;  /* bit-level coordinate so multiply by 8 */
    st_cursor.uc_y = rand() % (Y_LMCTL_OLED_COL_NUM     - 3) + 1;

    /* x,y must be odd number */
    if ((st_cursor.uc_x % 2) == 0) {
        st_cursor.uc_x++;
    }
    if ((st_cursor.uc_y % 2) == 0) {
        st_cursor.uc_y++;
    }

    /* Initialize point stack    */
    zus_lmctl_1501_point_stack_idx = Y_LMCTL_OLED_STACK_EMPTY + 1;   /* Point stack index        */
    zuc_lmctl_1501_point_stack_x[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_x;       /* X coordinate             */
    zuc_lmctl_1501_point_stack_y[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_y;       /* Y coordinate             */

    /* Set the cursor position to the labirynth            */
    // M_LMCTL_OLED_CLEAR_BIT(
    //     // zuc_lmctl_1501_labyrinth,
    //     zuc_LMCTL_oled_raw_buffer,
    //     st_cursor.uc_x,
    //     st_cursor.uc_y
    // );
}

/****************************************************************/
/*  m_lmctl_1501_oled_generate_labirynth_update                 */
/*--------------------------------------------------------------*/
/*  Labirynth update function.                                  */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period:                                                     */
/*  Parameters: <>                                              */
/*  Returns: <uint8_t> 1 if the action is completed             */
/****************************************************************/
static uint8_t m_lmctl_1501_oled_generate_labirynth_update(void) {
    lmctl_point_t st_cursor;                                        /* Cursor position          */
    lmctl_point_t st_mid_point;                                     /* Mid point                */

    while (zus_lmctl_1501_point_stack_idx > Y_LMCTL_OLED_STACK_EMPTY) {
        uint8_t uc_advanced_flg = Y_OFF;                                                        /* Advanced flag        */
        uint8_t uc_direction;                                                                   /* Direction            */
        uint8_t puc_direction_list[4] = {Y_LMCTL_OLED_NO_DIR};                                  /* Direction list           */
        uint8_t uc_direction_list_idx = 0;                                                      /* Direction list index     */
        st_cursor.uc_x = zuc_lmctl_1501_point_stack_x[zus_lmctl_1501_point_stack_idx];          /* X coordinate */  // fixme
        st_cursor.uc_y = zuc_lmctl_1501_point_stack_y[zus_lmctl_1501_point_stack_idx];          /* Y coordinate */
        zus_lmctl_1501_point_stack_idx--;                                                       /* Decrement the index  */

        /* if already visited */
        if (M_LMCTL_OLED_GET_BIT(zuc_LMCTL_oled_raw_buffer, st_cursor.uc_x, st_cursor.uc_y) == Y_OFF) {
            continue;                                                                           /* Skip the point        */
        }

        /* Check the directions     */
        /* if (have 2 pixels to advance AND not visited) */
        if ((st_cursor.uc_x > 2)
         && (M_LMCTL_OLED_GET_BIT(zuc_LMCTL_oled_raw_buffer, st_cursor.uc_x - 2, st_cursor.uc_y) == Y_ON)) {
            puc_direction_list[uc_direction_list_idx] = Y_LMCTL_OLED_LEFT;      /* Left direction available */
            uc_direction_list_idx++;                                            /* Increment the index       */
        }
        if ((st_cursor.uc_x < (Y_LMCTL_OLED_ROW_NUM * 8 - 4)  // Thicker Wall (2pixels)
         && (M_LMCTL_OLED_GET_BIT(zuc_LMCTL_oled_raw_buffer, st_cursor.uc_x + 2, st_cursor.uc_y) == Y_ON))) {
            puc_direction_list[uc_direction_list_idx] = Y_LMCTL_OLED_RIGHT;     /* Right direction available */
            uc_direction_list_idx++;                                            /* Increment the index       */
        }
        if ((st_cursor.uc_y > 2)
         && (M_LMCTL_OLED_GET_BIT(zuc_LMCTL_oled_raw_buffer, st_cursor.uc_x, st_cursor.uc_y - 2) == Y_ON)) {
            puc_direction_list[uc_direction_list_idx] = Y_LMCTL_OLED_UP;        /* Up direction available    */
            uc_direction_list_idx++;                                            /* Increment the index       */
        }
        if ((st_cursor.uc_y < (Y_LMCTL_OLED_COL_NUM - 4))  // Thicker Wall (2pixels)
         && (M_LMCTL_OLED_GET_BIT(zuc_LMCTL_oled_raw_buffer, st_cursor.uc_x, st_cursor.uc_y + 2) == Y_ON)) {
            puc_direction_list[uc_direction_list_idx] = Y_LMCTL_OLED_DOWN;      /* Down direction available  */
            uc_direction_list_idx++;                                            /* Increment the index       */
        }

        /* Push the point stack         */
        if (uc_direction_list_idx == 0) {
            continue;                                                           /* No direction               */
                                                                                /* Back to the previous point */
        } else {
            /* Shuffle the direction list    */
            for (uint8_t uc_i = 0; uc_i < uc_direction_list_idx; uc_i++) {
                uint8_t uc_tmp = puc_direction_list[uc_i];                          /* Temporary variable         */
                uint8_t uc_j = rand() % uc_direction_list_idx;                      /* Random index               */
                puc_direction_list[uc_i] = puc_direction_list[uc_j];                /* Swap the direction         */
                puc_direction_list[uc_j] = uc_tmp;                                  /* Swap the direction         */
            }
            
            for (uint8_t uc_i = 0; uc_i < uc_direction_list_idx; uc_i++) {
                uc_direction = puc_direction_list[uc_i];                        /* Direction                  */
                switch (uc_direction) {
                    case Y_LMCTL_OLED_LEFT:
                        zus_lmctl_1501_point_stack_idx++;             /* Increment the index        */
                        zuc_lmctl_1501_point_stack_x[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_x - 2;    /* X coordinate */
                        zuc_lmctl_1501_point_stack_y[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_y;        /* Y coordinate */
                        st_mid_point.uc_x = st_cursor.uc_x - 1;       /* Mid point X coordinate     */
                        st_mid_point.uc_y = st_cursor.uc_y;           /* Mid point Y coordinate     */
                        uc_advanced_flg = Y_ON;                                     /* Advanced flag              */
                        break;
                    case Y_LMCTL_OLED_RIGHT:
                        zus_lmctl_1501_point_stack_idx++;             /* Increment the index        */
                        zuc_lmctl_1501_point_stack_x[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_x + 2;    /* X coordinate */
                        zuc_lmctl_1501_point_stack_y[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_y;        /* Y coordinate */
                        st_mid_point.uc_x = st_cursor.uc_x + 1;       /* Mid point X coordinate     */
                        st_mid_point.uc_y = st_cursor.uc_y;           /* Mid point Y coordinate     */
                        uc_advanced_flg = Y_ON;                                     /* Advanced flag              */
                        break;
                    case Y_LMCTL_OLED_UP:
                        zus_lmctl_1501_point_stack_idx++;             /* Increment the index        */
                        zuc_lmctl_1501_point_stack_x[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_x;        /* X coordinate */
                        zuc_lmctl_1501_point_stack_y[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_y - 2;    /* Y coordinate */
                        st_mid_point.uc_x = st_cursor.uc_x;           /* Mid point X coordinate     */
                        st_mid_point.uc_y = st_cursor.uc_y - 1;       /* Mid point Y coordinate     */
                        uc_advanced_flg = Y_ON;                                     /* Advanced flag              */
                        break;
                    case Y_LMCTL_OLED_DOWN:
                        zus_lmctl_1501_point_stack_idx++;             /* Increment the index        */
                        zuc_lmctl_1501_point_stack_x[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_x;        /* X coordinate */
                        zuc_lmctl_1501_point_stack_y[zus_lmctl_1501_point_stack_idx] = st_cursor.uc_y + 2;    /* Y coordinate */
                        st_mid_point.uc_x = st_cursor.uc_x;           /* Mid point X coordinate     */
                        st_mid_point.uc_y = st_cursor.uc_y + 1;       /* Mid point Y coordinate     */
                        uc_advanced_flg = Y_ON;                                     /* Advanced flag              */
                        break;
                    default:
                        break;
                }
            }
        }

        /* If advanced, exit this function  */
        if (uc_advanced_flg == Y_ON) {
            M_LMCTL_OLED_CLEAR_BIT(
                // zuc_lmctl_1501_labyrinth,
                zuc_LMCTL_oled_raw_buffer,
                st_mid_point.uc_x,
                st_mid_point.uc_y
            );
            break;
        }
    }

    /* Update the labyrinth    */
    M_LMCTL_OLED_CLEAR_BIT(
        // zuc_lmctl_1501_labyrinth,
        zuc_LMCTL_oled_raw_buffer,
        st_cursor.uc_x,
        st_cursor.uc_y
    );

    if (zus_lmctl_1501_point_stack_idx == Y_LMCTL_OLED_STACK_EMPTY) {
        return Y_ON;    /* Completed    */
    } else {
        return Y_OFF;   /* Not completed    */
    }
}
#endif /* LMCTL_1501_LABYRINTH_ENABLE */

/****************************************************************/
/*  m_lmctl_oled_init_by_frame                                  */
/*--------------------------------------------------------------*/
/*  OLED buffer nitialization function (frame).                 */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period:                                                     */
/*  Parameters: <Buffer>, <odd_size_flg>                        */
/*  Returns:                                                    */
/****************************************************************/
#if 0
#define Y_LMCTL_OLED_ALL_WALL           (0xFF)     /* Wall                          */
#define Y_LMCTL_OLED_LEFT_WALL          (0x80)     /* Left wall                     */
#define Y_LMCTL_OLED_RIGHT_THICK_WALL   (0x03)     /* Right wall (2 pixels thick)   */
#define Y_LMCTL_OLED_RIGHT_WALL         (0x03)     /* Right wall                    */
static void m_lmctl_oled_init_by_frame(
    uint8_t puc_buffer[][Y_LMCTL_OLED_ROW_NUM],    /* Buffer to be initialized */
    uint8_t uc_odd_size_flg                        /* Odd size flag            */
) {
    /* In Y_LMCTL_OLED_COL_NUM x Y_LMCTL_OLED_ROW_NUM binary table, */
    /*       1: Wall, 0: Path                                       */
    /*       The edge is always the wall                            */

    uint8_t uc_right_wall_thickness;    /* Right wall thickness    */

    if (uc_odd_size_flg == Y_ON) {
        uc_right_wall_thickness = Y_LMCTL_OLED_RIGHT_THICK_WALL;    /* Right wall thickness    */
    } else {
        uc_right_wall_thickness = Y_LMCTL_OLED_RIGHT_WALL;          /* Right wall thickness    */
    }

    /* Left edge -> Wall        */
    for (uint16_t us_i = 1; us_i < Y_LMCTL_OLED_COL_NUM-1; us_i++) {
        puc_buffer[us_i][0] = Y_LMCTL_OLED_LEFT_WALL;
    }

    /* Right edge -> Wall       */
    for (uint16_t us_i = 1; us_i < Y_LMCTL_OLED_COL_NUM-1; us_i++) {
        puc_buffer[us_i][Y_LMCTL_OLED_ROW_NUM-2] = uc_right_wall_thickness;     /* Right or Right-1 */
    }

    /* Top edge -> Wall         */
    for (uint16_t us_i = 0; us_i < Y_LMCTL_OLED_ROW_NUM; us_i++) {
        puc_buffer[0][us_i] = Y_LMCTL_OLED_ALL_WALL;
    }

    /* Bottom edge -> Wall      */
    if (uc_odd_size_flg == Y_ON) {
        for (uint16_t us_i = 0; us_i < Y_LMCTL_OLED_ROW_NUM; us_i++) {   
            puc_buffer[Y_LMCTL_OLED_COL_NUM-2][us_i] = Y_LMCTL_OLED_ALL_WALL;       /* Buttom - 1   */
        }
    }

    for (uint16_t us_i = 0; us_i < Y_LMCTL_OLED_ROW_NUM; us_i++) {   
        puc_buffer[Y_LMCTL_OLED_COL_NUM-1][us_i] = Y_LMCTL_OLED_ALL_WALL;           /* Buttom       */
    }
}
#endif

#if (LMCTL_1501_LABYRINTH_ENABLE == 1)  /* or ... */
/****************************************************************/
/*  m_lmctl_oled_init_by_fill                                   */
/*--------------------------------------------------------------*/
/*  OLED buffer nitialization function (fill).                  */
/*                                                              */
/*--------------------------------------------------------------*/
/*  Period:                                                     */
/*  Parameters: <Buffer>                                        */
/*  Returns:                                                    */
/****************************************************************/
static void m_lmctl_oled_init_by_fill(
    uint8_t puc_buffer[][Y_LMCTL_OLED_ROW_NUM]     /* Buffer to be initialized */
) {
    for (uint16_t us_i = 0; us_i < Y_LMCTL_OLED_COL_NUM; us_i++) {
        for (uint16_t us_j = 0; us_j < Y_LMCTL_OLED_ROW_NUM; us_j++) {
            puc_buffer[us_i][us_j] = 0xFF;
        }
    }
}
#endif /* LMCTL_1501_LABYRINTH_ENABLE */

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
