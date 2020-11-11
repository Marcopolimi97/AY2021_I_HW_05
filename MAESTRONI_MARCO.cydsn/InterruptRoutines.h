/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

extern int state;
extern int newstate;

#ifndef _INTERRUPT_ROUTINES_H_
    // Header guard
    #define _INTERRUPT_ROUTINES_H_
    
    #include "project.h"
    
    /**
    *   \brief ISR Code.
    */
    
    CY_ISR_PROTO(ChangeFreq);
    
#endif

/* [] END OF FILE */
