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
/*  Luminous Control Function Assignments                                                                                       */
/*                                                                                                                              */
/*       #1xx: Management of the entire luminous control                                                                        */
/*       #2xx: RGB LED control (Startup)                                                                                        */
/*                                                                                                                              */
/*      #12xx: OLED display control (Startup)                                                                                   */
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
    uint8_t uc_dummy[3];                /* Dummy                                        */
} lmctl_context_t;

/********************************************************************************************************************************/
/*  Defines                                                                                                                     */
/********************************************************************************************************************************/
#define Y_LMCTL_TASK_INTERVAL   (64)  /* [ms,64] 64ms                                           */
#define Y_LMCTL_STARTUP_TIME    (64)  /* [ms,64] 64 * Y_LMCTL_TASK_INTERVAL(64) = 4.096s        */

/********************************************************************************************************************************/
/*  Variables                                                                                                                   */
/********************************************************************************************************************************/

/********************************************************************************************************************************/
/*  Functions                                                                                                                   */
/********************************************************************************************************************************/
static void m_lmctl_100_context_management(lmctl_context_t *pst_lmctl_context);
static void m_lmctl_oled_main(const lmctl_context_t *pst_lmctl_context);
static void m_lmctl_1200_oled_startup_logo(const lmctl_context_t *pst_lmctl_context);

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
    static lmctl_context_t zst_lmctl_context = {0};         /* LMCTL_Context            */

    m_lmctl_100_context_management(&zst_lmctl_context);     /* (#100) Management        */

#ifdef OLED_DRIVER_ENABLE
    m_lmctl_oled_main(&zst_lmctl_context);                  /* OLED Main                */
#endif /* OLED_DRIVER_ENABLE */

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
void m_lmctl_100_context_management(lmctl_context_t *pst_lmctl_context) {
    uint32_t ul_app_timestamp = pst_lmctl_context->ul_app_timestamp;    /* [ms,64] System timestamp         */
    uint8_t uc_master_mode_flg;                                         /* Master mode flag                 */

    {
        M_CLIP_INC(ul_app_timestamp, UINT32_MAX)                        /* Increment the system timestamp   */

        if (is_keyboard_master()) {
            uc_master_mode_flg = Y_ON;                                  /* Master mode flag ON              */
        } else {
            uc_master_mode_flg = Y_OFF;                                 /* Master mode flag OFF             */
        }
    }

    pst_lmctl_context->ul_app_timestamp = ul_app_timestamp;             /* Update the system timestamp      */
    pst_lmctl_context->uc_master_mode_flg = uc_master_mode_flg;         /* Update the master mode flag      */
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
    const uint32_t xul_app_timestamp = pst_lmctl_context->ul_app_timestamp;    /* [ms,64] App timestamp     */

    {
        if (xul_app_timestamp < Y_LMCTL_STARTUP_TIME) {
            oled_write_P(Xc_logo_indices, false);                       /* Display the logo     */
        } else if (xul_app_timestamp == Y_LMCTL_STARTUP_TIME) {
            oled_clear();                                               /* Clear the display    */
        } else /* if (xul_app_timestamp > Y_LMCTL_STARTUP_TIME) */ {
            /* Do nothing */                                            /* Do nothing           */
        }
    }
}


#endif /* OLED_DRIVER_ENABLE */
