/* ========================================
 *
 * MARCO MAESTRONI
 *
 * Here, I check when the switch is pressed and I set the different states
 * representing different frequencies defined in main code
 * 
 * ========================================
*/
#include "InterruptRoutines.h"
#include "project.h"

/* Everytime an INTERRUPT occurs, it means the button has been pushed and so 
* I change state for switching different frequencies.
* The following incremental code configuration:
*         state=state+1;
* gave me problems, so I choose a more explicit way to increment it.
* The variable newstate is needed in order to not recursively enter in the following if conditions.
*
*/
CY_ISR(ChangeFreq)
{
    if(state==1)
    {
        //CHECK ---UART_Debug_PutString("Sampling frequency: 10 Hz \r\n");
        newstate=2;
    }
    if(state==2)
    {
        //CHECK  ---UART_Debug_PutString("Sampling frequency: 25 Hz \r\n");    
        newstate=3;
    }
    if(state==3)
    {
        //CHECK  ---UART_Debug_PutString("Sampling frequency: 50 Hz \r\n"); 
        newstate=4;
    }
    if(state==4)
    {
        //CHECK  ---UART_Debug_PutString("Sampling frequency: 100 Hz \r\n"); 
        newstate=5;
    }
    if(state==5)
    {
        //CHECK  ---UART_Debug_PutString("Sampling frequency: 200 Hz \r\n"); 
        newstate=6;
    }
    if(state==6)
    {
        //CHECK  ---UART_Debug_PutString("Sampling frequency: 1 Hz \r\n");
        newstate=1;
    }
}


/* [] END OF FILE */
