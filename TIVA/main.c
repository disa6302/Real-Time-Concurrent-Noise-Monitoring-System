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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "main.h"
#include "drivers/pinout.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"
#include "driverlib/uart.h"


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


#define  I2CNOTIFVAL 0
//Structure for socket packet
struct sock_struct
{
    uint32_t len;
    uint32_t cmd;
    char sock_buff[256];
};
char temp_buff[18];

void UART_Send_Command(char *buffer);
void UART_Send_Packet(struct sock_struct sock, int numOfCharacters);

#define SSID_NAME  "HOME-C345-2.4"
#define PASSKEY    "curve9567awhile"
#define PORTNUM "5000"
#define IPADDRESS "10.0.0.165"

TaskHandle_t ADCTaskHandle;
TaskHandle_t I2CTaskHandle;
uint16_t display_array_color[8] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF};
uint16_t display_array[8]= {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};

QueueHandle_t xAudioQueue;

typedef enum reqtype_t{
DEFAULT,
REQ_REMOTE_API,
REQ_ERR_REC,
REQ_PATTERN_DATA,
TX_AUDIO_DATA,
TX_ERR_LOG,
TX_INIT_LOG
}request_t;



int ADCInitFlag = 1;
int LEDInitFlag = 1;


struct sock_struct sock_val;

//Global Variables
volatile uint8_t address;
uint32_t ui32Value = 0;
uint32_t rx_value;
uint32_t ulNotifiedValue;
uint32_t output_clock_rate_hz;

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


void UARTInit()
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Set GPIO A4 and A5 as UART pins.

    GPIOPinConfigure(GPIO_PA4_U3RX);
    GPIOPinConfigure(GPIO_PA5_U3TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_4 | GPIO_PIN_5);


    // Configure the UART for 57600, 8-bit, 1 stop bit operation.

    ROM_UARTConfigSetExpClk(UART3_BASE, output_clock_rate_hz, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                    UART_CONFIG_PAR_NONE));

//    UART_SendData(d,strlen("TIVA MESSAGE"));

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

void UART_Send_Packet(struct sock_struct sock, int numOfCharacters)
{
       unsigned char *myPtr = (unsigned char *)&sock;
       const unsigned char *byteToSend;
       int numberOfBytes = sizeof(sock)-(256-numOfCharacters);
       byteToSend=myPtr;
       while(numberOfBytes--)
       {
           //UARTprintf("%c ",numberOfBytes,*byteToSend);
           SysCtlDelay(2000);
           UARTCharPutNonBlocking(UART3_BASE,*byteToSend);
           ++byteToSend;
       }
}

void UART_Send_Command(char *buffer)
{
    int lengthOfBuffer = strlen(buffer);
    while(lengthOfBuffer--)
    {
        SysCtlDelay(2000);
        UARTCharPutNonBlocking(UART3_BASE,*buffer);
        buffer++;
    }
}



void ESPInit()
{
    UART_Send_Command("AT\r\n");
    SysCtlDelay(500000);
    UART_Send_Command("AT+CWMODE=1\r\n");
    SysCtlDelay(800000);
    UART_Send_Command("AT+CIPMODE=0\r\n");
    SysCtlDelay(800000);
    UART_Send_Command("AT+CIPMUX=0\r\n");
    SysCtlDelay(800000);
}




// Main function
int main(void)
{

    // Initialize system clock to 120 MHz
    output_clock_rate_hz = ROM_SysCtlClockFreqSet(
                               (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                               SYSTEM_CLOCK);

    ASSERT(output_clock_rate_hz == SYSTEM_CLOCK);

    // Initialize the GPIO pins for the Launchpad
    PinoutSet(false, false);

    // Initialize peripherals
    UARTStdioConfig(0, 57600, SYSTEM_CLOCK);

    //Queue for storing ADC Value
    xAudioQueue = xQueueCreate(10,sizeof(uint32_t));
    if(xAudioQueue == NULL)
    {
        UARTprintf("\r\nAUnable to create message queue\n");
    }


    ADCInit();

    //I2C Initialization
    I2CInit();

    //UART Initialization
    UARTInit();


    //ESP Initialization
    ESPInit();

    //Creating Tasks
    xTaskCreate(demoADCTask, (const portCHAR *)"ADC",
                    configMINIMAL_STACK_SIZE*2, NULL, 4, &ADCTaskHandle);

    xTaskCreate(demoI2CTask, (const portCHAR *)"I2C",
                    configMINIMAL_STACK_SIZE*2, NULL, 3, &I2CTaskHandle); //Lower Priority




    vTaskStartScheduler();
    return 0;
}

void demoADCTask(void *pvParameters)
{
    char TXBuffer[30];
    while(1)
    {
        if(ADCInitFlag)
        {
            bzero(TXBuffer,sizeof(TXBuffer));
            sprintf((char*)TXBuffer, "AT+CIPSTART=\"TCP\",\"%s\",5000\r\n", (const char*)(IPADDRESS));
            UART_Send_Command(TXBuffer);
            SysCtlDelay(300000000);
            bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
            sprintf(sock_val.sock_buff,"ADC Initialization Done!");
            sock_val.len = strlen(sock_val.sock_buff);
            sock_val.cmd = (uint32_t)(TX_INIT_LOG);
            bzero(TXBuffer,sizeof(TXBuffer));
            sprintf((char*)TXBuffer, "AT+CIPSEND=%d\r\n",sizeof(sock_val)-(256-sock_val.len));
            UART_Send_Command(TXBuffer);
            SysCtlDelay(800000);

            UART_Send_Packet(sock_val,sock_val.len);
            SysCtlDelay(200000000);
            ADCInitFlag = 0;
        }
        //Wait for the ADC0 module to be ready
        while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));

        //Trigger the sample sequence
        ADCProcessorTrigger(ADC0_BASE,3);

        //Wait until the sample sequence has completed
        while(!ADCIntStatus(ADC0_BASE, 3, false));

        //Read the value from the ADC
        ADCSequenceDataGet(ADC0_BASE, 3, &ui32Value);
        bzero(TXBuffer,sizeof(TXBuffer));
        sprintf((char*)TXBuffer, "AT+CIPSTART=\"TCP\",\"%s\",5000\r\n", (const char*)(IPADDRESS));
        UART_Send_Command(TXBuffer);
        SysCtlDelay(300000000);
        bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
        sprintf(sock_val.sock_buff,"ADC Intensity : %d !! ", ui32Value);
        sock_val.len = strlen(sock_val.sock_buff);
        sock_val.cmd = (uint32_t)(TX_AUDIO_DATA);
        bzero(temp_buff,sizeof(temp_buff));
        sprintf((char*)temp_buff, "AT+CIPSEND=%d\r\n",sizeof(sock_val)-(256-sock_val.len));

        UART_Send_Command(temp_buff);
        SysCtlDelay(800000);



        UART_Send_Packet(sock_val,sock_val.len);
        if(xAudioQueue != 0)
        {
                //Wait for 10 ticks if Queue becomes full before it can be refilled again
                if(xQueueSendToBack(xAudioQueue,(void *)&ui32Value,( TickType_t )10)!=pdPASS)
                {
                    UARTprintf("\r\n[Audio Log]Unable to put value into Queue\n");

                }
                else
                {
                    UARTprintf("\r\n[Audio Log]Value put into Queue:%d\n",ui32Value);
                    //xTaskNotify(ADCTaskHandle, I2CNOTIFVAL, eIncrement);
                }
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS); //Value logged into queue every 5 seconds

    }
   // vTaskDelete(NULL); /*Deletes Current task and frees up memory*/
}

//void *pvParameters
void demoI2CTask(void *pvParameters)
{
    char TXBuffer[30];
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

    while(1)
    {
       // xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, portMAX_DELAY); /*Blocks indefinitely waiting for notification*/
        if(LEDInitFlag)
        {
        bzero(TXBuffer,sizeof(TXBuffer));
        sprintf((char*)TXBuffer, "AT+CIPSTART=\"TCP\",\"%s\",5000\r\n", (const char*)(IPADDRESS));
        UART_Send_Command(TXBuffer);
        SysCtlDelay(300000000);
        bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
        sprintf(sock_val.sock_buff,"LED Matrix Initialization Done!");
        sock_val.len = strlen(sock_val.sock_buff);
        sock_val.cmd = (uint32_t)(TX_INIT_LOG);
        bzero(TXBuffer,sizeof(TXBuffer));
        sprintf((char*)TXBuffer, "AT+CIPSEND=%d\r\n",sizeof(sock_val)-(256-sock_val.len));
        UART_Send_Command(TXBuffer);
        SysCtlDelay(800000);

        UART_Send_Packet(sock_val,sock_val.len);
        SysCtlDelay(200000000);
        LEDInitFlag = 0;
        }
        //UARTprintf("\r\nEntered while loop of I2C\n");
        if(xAudioQueue != 0 )
        {

                // Receive a message on the created queue.  Block for 10 ticks if a
                // message is not immediately available.
                if(xQueueReceive(xAudioQueue, &rx_value,( TickType_t )10))
                {
                    UARTprintf("\r\n[LED Log]Value received from Queue:%d\n",rx_value);
                    if(rx_value<1250)
                    {
                        clear();
                        write_display_intensity_control(LOW);
                        bzero(TXBuffer,sizeof(TXBuffer));
                        sprintf((char*)TXBuffer, "AT+CIPSTART=\"TCP\",\"%s\",5000\r\n", (const char*)(IPADDRESS));
                        UART_Send_Command(TXBuffer);
                        SysCtlDelay(300000000);
                        bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
                        sprintf(sock_val.sock_buff,"LED Matrix Glowing Green!");
                        sock_val.len = strlen(sock_val.sock_buff);
                        sock_val.cmd = (uint32_t)(TX_INIT_LOG);
                        bzero(TXBuffer,sizeof(TXBuffer));
                        sprintf((char*)TXBuffer, "AT+CIPSEND=%d\r\n",sizeof(sock_val)-(256-sock_val.len));
                        UART_Send_Command(TXBuffer);
                        SysCtlDelay(800000);

                        UART_Send_Packet(sock_val,sock_val.len);
                        SysCtlDelay(200000000);
                    }
                    //To blink red
                    else if(rx_value>1300)
                    {
                        clear();
                        write_display_intensity_control(HIGH);
                        bzero(TXBuffer,sizeof(TXBuffer));
                        sprintf((char*)TXBuffer, "AT+CIPSTART=\"TCP\",\"%s\",5000\r\n", (const char*)(IPADDRESS));
                        UART_Send_Command(TXBuffer);
                        SysCtlDelay(300000000);
                        bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
                        sprintf(sock_val.sock_buff,"LED Matrix Glowing Red!");
                        sock_val.len = strlen(sock_val.sock_buff);
                        sock_val.cmd = (uint32_t)(TX_INIT_LOG);
                        bzero(TXBuffer,sizeof(TXBuffer));
                        sprintf((char*)TXBuffer, "AT+CIPSEND=%d\r\n",sizeof(sock_val)-(256-sock_val.len));
                        UART_Send_Command(TXBuffer);
                        SysCtlDelay(800000);

                        UART_Send_Packet(sock_val,sock_val.len);
                        SysCtlDelay(200000000);
                    }
                    else if(rx_value>1250 && rx_value<1300)
                    {
                        clear();
                        write_display_intensity_control(MID);
                        bzero(TXBuffer,sizeof(TXBuffer));
                        sprintf((char*)TXBuffer, "AT+CIPSTART=\"TCP\",\"%s\",5000\r\n", (const char*)(IPADDRESS));
                        UART_Send_Command(TXBuffer);
                        SysCtlDelay(300000000);
                        bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
                        sprintf(sock_val.sock_buff,"LED Matrix Glowing Orange!");
                        sock_val.len = strlen(sock_val.sock_buff);
                        sock_val.cmd = (uint32_t)(TX_INIT_LOG);
                        bzero(TXBuffer,sizeof(TXBuffer));
                        sprintf((char*)TXBuffer, "AT+CIPSEND=%d\r\n",sizeof(sock_val)-(256-sock_val.len));
                        UART_Send_Command(TXBuffer);
                        SysCtlDelay(800000);

                        UART_Send_Packet(sock_val,sock_val.len);
                        SysCtlDelay(200000000);
                    }
                }
                else
                {
                    UARTprintf("\r\n[LED Log]Unable to receive value from queue\n");
                }
            }
            vTaskDelay(5000 / portTICK_PERIOD_MS); //LED Matrix displayed every 5 seconds in sync with ADC
    }
   // vTaskDelete(NULL); /*Deletes Current task and frees up memory*/
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
