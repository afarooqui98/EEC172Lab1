//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - UART Demo
// Application Overview - The objective of this application is to showcase the 
//                        use of UART. The use case includes getting input from 
//                        the user and display information on the terminal. This 
//                        example take a string as input and display the same 
//                        when enter is received.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup uart_demo
//! @{
//
//*****************************************************************************

// Driverlib includes
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "uart.h"
#include "interrupt.h"
#include "pin_mux_config.h"
#include "utils.h"
#include "prcm.h"
#include "gpio.h"

// Common interface include
#include "uart_if.h"
#include "gpio_if.h"

//*****************************************************************************
//                          MACROS                                  
//*****************************************************************************
#define APPLICATION_VERSION  "1.4.0"
#define APP_NAME             "Lab1"
#define CONSOLE              UARTA0_BASE
#define UartGetChar()        MAP_UARTCharGet(CONSOLE)
#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
#define MAX_STRING_LENGTH    80


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

volatile int g_iCounter = 0;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//                      LOCAL DEFINITION                                   
//*****************************************************************************

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t *************************************************\n\r");
    Report("\t        CC3200 GPIO Application       \n\r");
    Report("\t *************************************************\n\r");
    Report("\n\n\r");
    Report("\t****************************************************\n\r");
    Report("\t Push SW3 to start LED binary counting  \n\r");
    Report("\t Push SW2 to blink LEDs on and off \n\r") ;
    Report("\t ****************************************************\n\r");
    Report("\n\n\n\r");
}

//*****************************************************************************
//
//! Configures the pins as GPIOs and peroidically toggles the lines
//!
//! \param None
//!
//! This function
//!    1. Configures 3 lines connected to LEDs as GPIO
//!    2. Sets up the GPIO pins as output
//!    3. Periodically toggles each LED one by one by toggling the GPIO line
//!
//! \return None
//
//*****************************************************************************
void blinkAllRoutine()
{
    //
    // Toggle the lines initially to turn off the LEDs.
    // The values driven are as required by the LEDs on the LP.
    //
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    //
    // Alternately toggle hi-low each of the GPIOs
    // to switch the corresponding LED on/off.
    //
    MAP_UtilsDelay(8000000);
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    MAP_UtilsDelay(8000000);
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
}

//using binary division to return the binary digits
void countingRoutine(int* val, int* mostSig, int* middleSig, int* leastSig) {
    int rem;

    *val = *val + 1;
    if (*val == 8){
        *val = 0;
    }

    *mostSig = *val / 4;
    rem = *val % 4;
    *middleSig = rem / 2;
    rem = rem % 2;
    *leastSig = rem;
}

//binary counting with rollover and persistent state
void countingBlinkyRoutine(int mostSig, int middleSig, int leastSig){
    MAP_UtilsDelay(8000000);
    if(mostSig){
        GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    }else{
        GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
    }

    if(middleSig){
        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    }else{
        GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    }

    if(leastSig){
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    }else{
        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    }
    MAP_UtilsDelay(8000000);
}



//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//! Main function handling the uart echo. It takes the input string from the
//! terminal while displaying each character of string. whenever enter command 
//! is received it will echo the string(display). if the input the maximum input
//! can be of 80 characters, after that the characters will be treated as a part
//! of next string.
//!
//! \param  None
//!
//! \return None
//! 
//*****************************************************************************
void main()
{
    //
    // Initailizing the board
    //
    BoardInit();
    //
    // Muxing for Enabling UART_TX and UART_RX.
    //
    PinMuxConfig();
    //
    // Initialising the Terminal.
    //
    InitTerm();
    //
    // Clearing the Terminal.
    //
    ClearTerm();
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    DisplayBanner(APP_NAME);

    long sw2Status = 0;
    long sw3Status = 0;
    int sw2Flag = 0;
    int sw3Flag = 0;

    int val = 0;
    int mostSig = 0;
    int middleSig = 0;
    int leastSig = 0;

    while(1)
    {
        sw3Status = GPIOPinRead(GPIOA1_BASE, 0x20);
        sw2Status = GPIOPinRead(GPIOA2_BASE, 0x40);

        if((sw2Status == 64 && !sw2Flag) || sw2Flag){
            if(!sw2Flag){
                //pressing for the first time or switching routines
                Report("SW2 pressed");
                GPIOPinWrite(GPIOA3_BASE, 0x10, 0xFF);
            }
            //set flags
            sw3Flag = false;
            sw2Flag = true;

            //standard blink
            blinkAllRoutine();
        }

        if((sw3Status == 32 && !sw3Flag) || sw3Flag){
            if(!sw3Flag){
                //pressing for the first time or switching routines
                 Report("SW3 pressed");
                 GPIOPinWrite(GPIOA3_BASE, 0x10, 0xAA);
             }
            //set flags
            sw2Flag = false;
            sw3Flag = true;

            //compute the bitwise pattern
            countingRoutine(&val, &mostSig, &middleSig, &leastSig);
            //visualize on LEDs
            countingBlinkyRoutine(mostSig, middleSig, leastSig);
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

    

