/**
*  MARCO MAESTRONI
*
*  In the main code I do the following steps: 
*
* - Set the registers to read the accelerometer on both the 3 axis
* - I read the address of the EEPROM where it's stored the address of the control register 1,setting the frequency
* - I read the value of the outputs on the 3 axis and I prepare the data to be sent through UART                                      
* 
*/

// Include required header files
#include "InterruptRoutines.h"
#include "I2C_Interface.h"
#include "project.h"
#include "stdio.h"


/**
*   \brief 7-bit I2C address of the slave device.
*   SDO connected to ground
*/
#define LIS3DH_DEVICE_ADDRESS 0x18

/**
*   \brief Address of the WHO AM I register
*/
#define LIS3DH_WHO_AM_I_REG_ADDR 0x0F

/**
*   \brief Address of the Status register
*/
#define LIS3DH_STATUS_REG 0x27

//new data available in status register (bit 5)
#define LIS3DH_STATUS_REG_NEW_DATA  0x08

/**
*   \brief Address of the Control register 1
*/
#define LIS3DH_CTRL_REG1 0x20

/**
*   \brief Address of the Control register 4
*/
#define LIS3DH_CTRL_REG4 0x23

/**
*   \ Address of HIGH RESOLUTION MODE in control registers
*/
#define LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1 0x07
#define LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG4 0x08 

//address of different frequencies in control register 1

#define LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_1_HZ    0x17
#define LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_10_HZ   0x27
#define LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_25_HZ   0x37
#define LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_50_HZ   0x47
#define LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_100_HZ  0x57
#define LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_200_HZ  0x67


/**
*   \brief Address of the Control register 4
*/
#define LIS3DH_CTRL_REG4 0x23

/**
*   \brief Address of the X,Y and Z output LSB register
*/
#define LIS3DH_OUT_X_L 0x28
#define LIS3DH_OUT_Y_L 0x2A
#define LIS3DH_OUT_Z_L 0x2C


// EEPROM startup register

#define EEPROM_STARTUP_ADDRESS   0x00

//define states
#define FREQ_1_HZ          1
#define FREQ_10_HZ         2
#define FREQ_25_HZ         3
#define FREQ_50_HZ         4
#define FREQ_100_HZ        5
#define FREQ_200_HZ        6

//init variables
int state=1;
int newstate=1;

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    isr_Button_StartEx(ChangeFreq);
    I2C_Peripheral_Start();
    UART_Debug_Start();
    EEPROM_Start();
           
    // String to print out messages on the UART
    char message[50] = {'\0'};
    
    //-------------------------------------------------------
    //set registers
    // ----------------------------------------------
    //set HIGH RESOLUTION MODE 
    
    //CTRL_REG1
    uint8_t ctrl_reg1; 
    
    //I set ODR[3:0]=0000 because I will set the data rate later based on the cycling of states of the switch button
    ctrl_reg1=LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1;
    
    ErrorCode error = I2C_Peripheral_WriteRegister(LIS3DH_DEVICE_ADDRESS,
                                                   LIS3DH_CTRL_REG1,
                                                   ctrl_reg1);
    
    if (error == NO_ERROR)
    {
        sprintf(message, "CONTROL REGISTER 1 successfully written as: 0x%02X\r\n", ctrl_reg1);
        UART_Debug_PutString(message); 
    }
    else
    {
        UART_Debug_PutString("Error occurred during I2C comm to set control register 1\r\n");   
    }
    
    //------------------------------------------------------------------------------------
    //CTRL_REG4
    //FS=+-2g (seen in datasheet "mechanical characteristhics")
    uint8_t ctrl_reg4;
    
    ctrl_reg4=LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG4;
    
    error = I2C_Peripheral_WriteRegister(LIS3DH_DEVICE_ADDRESS,
                                              LIS3DH_CTRL_REG4,
                                                    ctrl_reg4);
    
    if (error == NO_ERROR)
    {
        sprintf(message, "CONTROL REGISTER 4 successfully written as: 0x%02X\r\n", ctrl_reg4);
        UART_Debug_PutString(message); 
    }
    else
    {
        UART_Debug_PutString("Error occurred during I2C comm to set control register 4\r\n");   
    }
    //------------------------------------------------------------------------------
    
    
    //CyDelay(5); //"The boot procedure is complete about 5 milliseconds after device power-up."
      
    
    uint8_t header = 0xA0;
    uint8_t footer = 0xC0;
    uint8_t OutArray [8];
    OutArray[0] = header;
    OutArray[7] = footer;
    //since the sensitivity is set to 1 mg/digit
    //because FS=+-2g (seen in datasheet "mechanical characteristhics"), we have:
    //
    //    1 digit= 1 mg= 9.8 * 10^-3 m/s^2  
    //
    //we set the conversion factor in the following way:
    float conversion = 0.00981; //9.8 * 10^-3
    int dirtytrick = 1000; // in order to send int values to rescale in BCP
    
    float XDataOutConv;
    float YDataOutConv;
    float ZDataOutConv;
      
    uint8_t XData[2];
    uint8_t YData[2];
    uint8_t ZData[2];
    
    int16 XDataOut=0;
    int16 YDataOut=0;
    int16 ZDataOut=0;
    
    //at startup I read the address of the EEPROM where it's stored the address of the control register 1 setting the frequency.
    //I do not set any frequency here because anyway I'll enter in a if condition later that sets the frequency.


    for(;;)
    {
        //CyDelay(100);
        
        //based on the frequency set by the switch,
        //I set the right frequency in control register and I update the value of the EEPROM
        
        if(newstate==FREQ_1_HZ)
        {
            state=FREQ_1_HZ;
            //CHECK -- UART_Debug_PutString("Sampling frequency: 1 Hz \r\n");
            
            //write the specified address in the EEPROM startup address
            EEPROM_UpdateTemperature();
            EEPROM_WriteByte(LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_1_HZ,
                                                     EEPROM_STARTUP_ADDRESS);
            //read the specified address in the EEPROM startup address,
            //and write it on control register 1 to set the frequency
            ctrl_reg1=EEPROM_ReadByte(EEPROM_STARTUP_ADDRESS);
       
            error = I2C_Peripheral_WriteRegister(LIS3DH_DEVICE_ADDRESS,
                                                      LIS3DH_CTRL_REG1,
                                                            ctrl_reg1);
            /*---CHECK ---
            if (error == NO_ERROR)
            {
                sprintf(message, "CONTROL REGISTER 1 successfully written as: 0x%02X\r\n", ctrl_reg1);
                UART_Debug_PutString(message); 
            }
            else
            {
                UART_Debug_PutString("Error occurred during I2C comm to set control register 1\r\n");   
            }  
            ---------*/
        }
        
        if(newstate==FREQ_10_HZ)
        {
            state=FREQ_10_HZ;
            //CHECK -- UART_Debug_PutString("Sampling frequency: 10 Hz \r\n");
            
            //write the specified address in the EEPROM startup address
            EEPROM_UpdateTemperature();
            EEPROM_WriteByte(LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_10_HZ,
                                                     EEPROM_STARTUP_ADDRESS);
            //read the specified address in the EEPROM startup address,
            //and write it on control register 1 to set the frequency
            ctrl_reg1=EEPROM_ReadByte(EEPROM_STARTUP_ADDRESS);
       
            error = I2C_Peripheral_WriteRegister(LIS3DH_DEVICE_ADDRESS,
                                                      LIS3DH_CTRL_REG1,
                                                            ctrl_reg1);
            /*---CHECK ---
            if (error == NO_ERROR)
            {
                sprintf(message, "CONTROL REGISTER 1 successfully written as: 0x%02X\r\n", ctrl_reg1);
                UART_Debug_PutString(message); 
            }
            else
            {
                UART_Debug_PutString("Error occurred during I2C comm to set control register 1\r\n");   
            }  
            ---------*/
        }
        
        if(newstate==FREQ_25_HZ)
        {
            state=FREQ_25_HZ;
            //CHECK -- UART_Debug_PutString("Sampling frequency: 25 Hz \r\n");
            
            //write the specified address in the EEPROM startup address
            EEPROM_UpdateTemperature();
            EEPROM_WriteByte(LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_25_HZ,
                                                     EEPROM_STARTUP_ADDRESS);
            //read the specified address in the EEPROM startup address,
            //and write it on control register 1 to set the frequency
            ctrl_reg1=EEPROM_ReadByte(EEPROM_STARTUP_ADDRESS);
       
            error = I2C_Peripheral_WriteRegister(LIS3DH_DEVICE_ADDRESS,
                                                      LIS3DH_CTRL_REG1,
                                                            ctrl_reg1);
            /*---CHECK ---
            if (error == NO_ERROR)
            {
                sprintf(message, "CONTROL REGISTER 1 successfully written as: 0x%02X\r\n", ctrl_reg1);
                UART_Debug_PutString(message); 
            }
            else
            {
                UART_Debug_PutString("Error occurred during I2C comm to set control register 1\r\n");   
            }  
            ---------*/
        }
        
        if(newstate==FREQ_50_HZ)
        {
            state=FREQ_50_HZ;
            //CHECK -- UART_Debug_PutString("Sampling frequency: 50 Hz \r\n");
            
            //write the specified address in the EEPROM startup address
            EEPROM_UpdateTemperature();
            EEPROM_WriteByte(LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_50_HZ,
                                                     EEPROM_STARTUP_ADDRESS);
            //read the specified address in the EEPROM startup address,
            //and write it on control register 1 to set the frequency
            ctrl_reg1=EEPROM_ReadByte(EEPROM_STARTUP_ADDRESS);
       
            error = I2C_Peripheral_WriteRegister(LIS3DH_DEVICE_ADDRESS,
                                                      LIS3DH_CTRL_REG1,
                                                            ctrl_reg1);
            /*---CHECK ---
            if (error == NO_ERROR)
            {
                sprintf(message, "CONTROL REGISTER 1 successfully written as: 0x%02X\r\n", ctrl_reg1);
                UART_Debug_PutString(message); 
            }
            else
            {
                UART_Debug_PutString("Error occurred during I2C comm to set control register 1\r\n");   
            }  
            ---------*/
        }
        
        if(newstate==FREQ_100_HZ)
        {
            state=FREQ_100_HZ;
            //CHECK -- UART_Debug_PutString("Sampling frequency: 100 Hz \r\n");
            
            //write the specified address in the EEPROM startup address
            EEPROM_UpdateTemperature();
            EEPROM_WriteByte(LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_100_HZ,
                                                     EEPROM_STARTUP_ADDRESS);
            //read the specified address in the EEPROM startup address,
            //and write it on control register 1 to set the frequency
            ctrl_reg1=EEPROM_ReadByte(EEPROM_STARTUP_ADDRESS);
       
            error = I2C_Peripheral_WriteRegister(LIS3DH_DEVICE_ADDRESS,
                                                      LIS3DH_CTRL_REG1,
                                                            ctrl_reg1);
            /*---CHECK ---
            if (error == NO_ERROR)
            {
                sprintf(message, "CONTROL REGISTER 1 successfully written as: 0x%02X\r\n", ctrl_reg1);
                UART_Debug_PutString(message); 
            }
            else
            {
                UART_Debug_PutString("Error occurred during I2C comm to set control register 1\r\n");   
            }  
            ---------*/
        }
        
        if(newstate==FREQ_200_HZ)
        {
            state=FREQ_200_HZ;
            //CHECK -- UART_Debug_PutString("Sampling frequency: 200 Hz \r\n");
            
            //write the specified address in the EEPROM startup address
            EEPROM_UpdateTemperature();
            EEPROM_WriteByte(LIS3DH_HIGH_RESOLUTION_MODE_CTRL_REG1_FREQ_200_HZ,
                                                     EEPROM_STARTUP_ADDRESS);
            //read the specified address in the EEPROM startup address,
            //and write it on control register 1 to set the frequency
            ctrl_reg1=EEPROM_ReadByte(EEPROM_STARTUP_ADDRESS);
       
            error = I2C_Peripheral_WriteRegister(LIS3DH_DEVICE_ADDRESS,
                                                      LIS3DH_CTRL_REG1,
                                                            ctrl_reg1);
            /*---CHECK ---
            if (error == NO_ERROR)
            {
                sprintf(message, "CONTROL REGISTER 1 successfully written as: 0x%02X\r\n", ctrl_reg1);
                UART_Debug_PutString(message); 
            }
            else
            {
                UART_Debug_PutString("Error occurred during I2C comm to set control register 1\r\n");   
            }  
            ---------*/
        }
        
        //I read the registers where the output (12 bit)of the accelerometer is stored
        
        //XData read
        
        error = I2C_Peripheral_ReadRegisterMulti(LIS3DH_DEVICE_ADDRESS,
                                                        LIS3DH_OUT_X_L,
                                                                     2,
                                                                XData);
        
        if (error == NO_ERROR)
        {
            /*--CHECK --
            sprintf(message, "XData is: %s\r\n", XData);
            UART_Debug_PutString(message); 
            ----*/
            
            //XDataOut is 12 bit long
            XDataOut = (int16)((XData[0] | (XData[1]<<8)))>>4;
            XDataOutConv = XDataOut * conversion;
        
            XDataOut = (int16) (XDataOutConv * dirtytrick);   
        }
        else
        {
            //UART_Debug_PutString("Error");   
        }  
        
        
        //YData read
        error = I2C_Peripheral_ReadRegisterMulti(LIS3DH_DEVICE_ADDRESS,
                                                        LIS3DH_OUT_Y_L,
                                                                     2,
                                                                YData);
        
        
        if (error == NO_ERROR)
        {
            /*---CHECK ---
            sprintf(message, "YData is: %s\r\n", YData);
            UART_Debug_PutString(message); 
            ---------*/
            //YDataOut is 12 bit long
            YDataOut = (int16)((YData[0] | (YData[1]<<8)))>>4;
            YDataOutConv = YDataOut * conversion;
        
            YDataOut = (int16) (YDataOutConv * dirtytrick);
        }
        else
        {
            //UART_Debug_PutString("Error");   
        }  
        
        
        //ZData read
        error = I2C_Peripheral_ReadRegisterMulti(LIS3DH_DEVICE_ADDRESS,
                                                        LIS3DH_OUT_Z_L,
                                                                     2,
                                                                ZData);
        
        
        if (error == NO_ERROR)
        {
            /*---CHECK ---
            sprintf(message, "ZData is: %s\r\n", ZData);
            UART_Debug_PutString(message);
            ---------*/
            //ZDataOut is 12 bit long
            ZDataOut = (int16)((ZData[0] | (ZData[1]<<8)))>>4;
            ZDataOutConv = ZDataOut * conversion;
        
            ZDataOut = (int16) (ZDataOutConv * dirtytrick);
       
        }
        else
        {
            //UART_Debug_PutString("Error");   
        }  
        
        //read of the status register to see if a set of new data is available
        //STATUS_REG
        uint8_t status_reg;  
    
        error = I2C_Peripheral_ReadRegister(LIS3DH_DEVICE_ADDRESS,
                                                LIS3DH_STATUS_REG,
                                                     &status_reg);
    
        /*--CHECK---
        if (error == NO_ERROR)
        {
            sprintf(message, "STATUS REGISTER 0x%02X\r\n", status_reg);
            UART_Debug_PutString(message); 
        }
        else
        {
            UART_Debug_PutString("Error\r\n");   
        }
        ----*/
        
        //------------------
        //if(status_reg & LIS3DH_STATUS_REG_NEW_DATA)
        //{
            //put together the array of X,Y and Z data to send to BCP
            OutArray[1] = (uint8_t)(XDataOut & 0xFF);
            OutArray[2] = (uint8_t)(XDataOut >> 8);
            
            OutArray[3] = (uint8_t)(YDataOut & 0xFF);
            OutArray[4] = (uint8_t)(YDataOut >> 8);
            
            OutArray[5] = (uint8_t)(ZDataOut & 0xFF);
            OutArray[6] = (uint8_t)(ZDataOut >> 8);
            
       
            UART_Debug_PutArray(OutArray, 8);  
        //}
        //else
        //{
            //UART_Debug_PutString("no new data available\r\n"); 
            //UART_Debug_PutArray(OutArrayOld, 8);  
        //}
    }
}

/* [] END OF FILE */
