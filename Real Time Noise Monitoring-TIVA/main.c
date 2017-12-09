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
#include "driverlib/pin_map.h"

//FreeRTOS includes for ADC Drivers and SYSCTL Drivers
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "inc/hw_memmap.h"


uint16_t display_array_color[8] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF};
uint16_t display_array[8]= {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
// Demo Task declarations
void demoLEDTask(void *pvParameters);
void demoSerialTask(void *pvParameters);
void demoADCTask(void *pvParameters);
void UARTInit(void);
void ADCInit(void);

//Global Variables
volatile uint8_t address;
uint32_t ui32Value = 0;



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

//Initialize the I2C Master
void I2CInit()
{
    //Configure and enable GPIOs for I2C0 SDA and SCL


    //Enable port G
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);


    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOG));

    //PIN Muxing
    ROM_GPIOPinConfigure(GPIO_PG0_I2C1SCL);
    ROM_GPIOPinConfigure(GPIO_PG1_I2C1SDA);

    //ENable PG0-SCL and PG1-SDA
    ROM_GPIOPinTypeI2C(GPIO_PORTG_BASE, GPIO_PIN_1);
    GPIOPinTypeI2CSCL(GPIO_PORTG_BASE, GPIO_PIN_0);


    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C1);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_I2C1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);

    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_I2C1));


    ROM_I2CMasterInitExpClk(I2C1_BASE, SYSTEM_CLOCK, true); //400kbps

}

void setBrightness(uint8_t val)
{
    if(val>15) val = 15;
    ROM_I2CMasterSlaveAddrSet(I2C1_BASE, SLAVE_ADDR, false);
    ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_SINGLE_SEND);
    //Transmitting I2C Command for Setting Brightness of LED
    ROM_I2CMasterDataPut(I2C1_BASE, BRIGHTNESS_CMD|val);
    while(I2CMasterBusy(I2C1_BASE));

}


void write_display_intensity_control(enum Ctrl_t ctrl)
{
    uint8_t i=0,j=0;
    //If Low noise Intensity : Glow Green
    if(ctrl==LOW)
    {
        address = 0x00;
    }
    //If Low noise Intensity : Glow RED
    else if(ctrl == HIGH)
    {
        address = 0x01;
    }
    //If Low noise Intensity : Glow Orange
    else if(ctrl == MID)
    {
        write_display_intensity_mid_control();
    }

    for(j=0;j<8;j++)
    {
        ROM_I2CMasterSlaveAddrSet(I2C1_BASE, SLAVE_ADDR, false);
        ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);


        ROM_I2CMasterDataPut(I2C1_BASE, address);

        SysCtlDelay(2000); //2ms delay
        while(I2CMasterBusy(I2C1_BASE));
        for(i=0;i<8;i++)
        {
            ROM_I2CMasterDataPut(I2C1_BASE, (display_array_color[i])&0xFF);
            while(I2CMasterBusy(I2C1_BASE));
            ROM_I2CMasterDataPut(I2C1_BASE, display_array_color[i]>>8);
            while(I2CMasterBusy(I2C1_BASE));
        }
        address+=2;

        ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    }
}


//Row8-15 configures RED LED and Row0-7 configures GREEN LED
void writeDisplay()
{
    uint8_t i=0,j=0;
    address = 0x00; //Starting from Base address 0x00
    for(j=0;j<16;j++)
    {
        ROM_I2CMasterSlaveAddrSet(I2C1_BASE, SLAVE_ADDR, false);
        ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);
        ROM_I2CMasterDataPut(I2C1_BASE, address);
        SysCtlDelay(2000); //2ms delay
        //Wait till Master is available from previous transmission
        while(I2CMasterBusy(I2C1_BASE));

        //Output the Display buffer for all the rows
        for(i=0;i<8;i++)
        {
            ROM_I2CMasterDataPut(I2C1_BASE, (display_array[i])&0xFF);
            while(I2CMasterBusy(I2C1_BASE));
            ROM_I2CMasterDataPut(I2C1_BASE, display_array[i]>>8);
            while(I2CMasterBusy(I2C1_BASE));
        }
        //Move to next Row Address
        address++;

        ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    }
}

void write_display_intensity_mid_control()
{
    uint8_t i=0,j=0;
    uint8_t address = 0x00;//Starting from Base address 0x00
    for(j=0;j<16;j++)
    {
        ROM_I2CMasterSlaveAddrSet(I2C1_BASE, SLAVE_ADDR, false);
        ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);


        ROM_I2CMasterDataPut(I2C1_BASE, address);
        SysCtlDelay(2000); //2ms delay
        //Wait till Master is available from previous transmission
        while(I2CMasterBusy(I2C1_BASE));
        //Output the Display buffer for all the rows
        for(i=0;i<8;i++)
        {
            ROM_I2CMasterDataPut(I2C1_BASE, (display_array_color[i])&0xFF);
            while(I2CMasterBusy(I2C1_BASE));
            ROM_I2CMasterDataPut(I2C1_BASE, display_array_color[i]>>8);
            while(I2CMasterBusy(I2C1_BASE));
        }
        //Output the Display buffer for all the rows
        address++;

        ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    }
}

void clear()
{
    writeDisplay();
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

    // Initialize peripherals
    UARTStdioConfig(0, 57600, SYSTEM_CLOCK);
    ADCInit();

    //Creating Tasks
    xTaskCreate(demoADCTask, (const portCHAR *)"ADC",
                    configMINIMAL_STACK_SIZE, NULL, 4, NULL);

    xTaskCreate(demoI2CTask, (const portCHAR *)"I2C",
                    configMINIMAL_STACK_SIZE, NULL, 1, NULL); //Lower Priority
    vTaskStartScheduler();
    return 0;
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
//void *pvParameters
void demoI2CTask(void *pvParameters)
{

    //Configure System Setup Register
    ROM_I2CMasterSlaveAddrSet(I2C1_BASE, SLAVE_ADDR, false);
    ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);


    ROM_I2CMasterDataPut(I2C1_BASE, SYSTEM_SETUP_REG);
    ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    //Wait till Master is available from previous transmission
    while(I2CMasterBusy(I2C1_BASE));


    //1uS delay
    SysCtlDelay(100);

    //Setup display register to enable LED with blink period OFF
    ROM_I2CMasterSlaveAddrSet(I2C1_BASE, SLAVE_ADDR, false);
    ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //1uS delay
    SysCtlDelay(100);

    ROM_I2CMasterDataPut(I2C1_BASE, DISP_SETUP_REG);
    ROM_I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    //Wait till Master is available from previous transmission
    while(I2CMasterBusy(I2C1_BASE));


    //Set brightness to maximum value
    setBrightness(15);

    //1uS delay
    SysCtlDelay(100);
    //To Glow GREEN
    clear();
	SysCtlDelay(400000);//Delay for 4 secs
    write_display_intensity_control(LOW);
    SysCtlDelay(400000);//Delay for 4 secs
    //To Glow RED
    clear();
	SysCtlDelay(400000);//Delay for 4 secs
    write_display_intensity_control(HIGH);
    SysCtlDelay(400000);//Delay for 4 secs
    //To Glow Orange
    clear();
	SysCtlDelay(400000);//Delay for 4 secs
    write_display_intensity_control(MID);
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
