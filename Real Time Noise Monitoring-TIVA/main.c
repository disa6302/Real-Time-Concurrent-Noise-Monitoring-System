/* FreeRTOS 8.2 Tiva Demo
 *
 * main.c
 *
 * Andy Kobyljanec
 *
 * Edited by Divya Sampath and Bhallaji Venkatesan
 *
 * This is a simple demonstration project of FreeRTOS 8.2 on the Tiva Launchpad
 * EK-TM4C1294XL.  TivaWare driverlib sourcecode is included.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "main.h"
#include "drivers/pinout.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"


// TivaWare includes
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

// FreeRTOS includes
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

//FreeRTOS includes for ADC Drivers and SYSCTL Drivers
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"


// Demo Task declarations
void demoLEDTask(void *pvParameters);
void demoSerialTask(void *pvParameters);
void demoADCTask(void *pvParameters);
void UARTInit(void);
void ADCInit(void);

uint32_t ui32Value = 0;

//UART Initialization
void UARTInit()
{
        UARTStdioConfig(0, 57600, SYSTEM_CLOCK);
}

//ADC Initialization
void ADCInit()
{
    //Enable Port E for channel selection
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    //Enable the ADC0 module
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    //Accept input from channel 0-pin PE3 of TM4C129
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);

    //Configure sample sequencer 3-captures 1 sample
    //Assuming a timer is triggering ADC sampling-need to configure TimerControlTrigger()
    ADCSequenceConfigure(ADC0_BASE,3,ADC_TRIGGER_PROCESSOR,0);

    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0);

    ADCSequenceEnable(ADC0_BASE, 3);

    //Ensure interrupts are cleared
    ADCIntClear(ADC0_BASE,3);

}
// Main function
int main(void)
{
    // Initialize system clock to 120 MHz

    uint32_t output_clock_rate_hz;
    output_clock_rate_hz = ROM_SysCtlClockFreqSet(
                               (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                               SYSTEM_CLOCK);

    ASSERT(output_clock_rate_hz == SYSTEM_CLOCK);

    // Initialize the GPIO pins for the Launchpad
    PinoutSet(false, false);

    //Initialize peripherals
    UARTInit();
    ADCInit();


      xTaskCreate(demoADCTask, (const portCHAR *)"ADC",
                    configMINIMAL_STACK_SIZE, NULL, 1, NULL);


    vTaskStartScheduler();
    return 0;
}


// Flash the LEDs on the launchpad
void demoLEDTask(void *pvParameters)
{
    for (;;)
    {
        // Turn on LED 1
        LEDWrite(0x0F, 0x01);
        vTaskDelay(1000);

        // Turn on LED 2
        LEDWrite(0x0F, 0x02);
        vTaskDelay(1000);

        // Turn on LED 3
        LEDWrite(0x0F, 0x04);
        vTaskDelay(1000);

        // Turn on LED 4
        LEDWrite(0x0F, 0x08);
        vTaskDelay(1000);
    }
}



void demoADCTask(void *pvParameters)
{

    while(1)
    {
        //Wait for the ADC0 module to be ready
        while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));


        //Trigger the sample sequence
        ADCProcessorTrigger(ADC0_BASE,3);

        //Wait until the sample sequence has completed
        while(!ADCIntStatus(ADC0_BASE, 3, false));

        //Read the value from the ADC
        ADCSequenceDataGet(ADC0_BASE, 3, &ui32Value);

        //Log into UART
        UARTprintf("\r\nADC Value:%d",ui32Value);
        vTaskDelay(5000 / portTICK_PERIOD_MS); //Value logged every 5 seconds

    }
}
/*  ASSERT() Error function
 *
 *  failed ASSERTS() from driverlib/debug.h are executed in this function
 */
void __error__(char *pcFilename, uint32_t ui32Line)
{
    // Place a breakpoint here to capture errors until logging routine is finished
    while (1)
    {
    }
}
