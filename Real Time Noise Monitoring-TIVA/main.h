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

#ifndef MAIN_H_
#define MAIN_H_
// System clock rate, 120 MHz
#define SYSTEM_CLOCK    120000000U

//Slave Address of MEMS Audio Sensor
#define SLAVE_ADDR 0x70
//LED MATRIX SETUP REGISTER
#define SYSTEM_SETUP_REG 0x21
//LED MATRIX ROW OUTPUT SET
#define ROW_INT_REG 0xA0
//LED MATRIX DISPLAY SETUP REGISTER
#define DISP_SETUP_REG 0x81
//LED MATRIX BRIGHTNESS ADJUSTMENT COMMAND
#define BRIGHTNESS_CMD 0xE0

//ENUMERATION For Sound Intensities
enum Ctrl_t{HIGH,LOW,MID};

// Demo Task declarations

//Demo ADC Task for MEMS Sensor
void demoADCTask(void *pvParameters);

//Demo I2C Task for LED Matrix
void demoI2CTask(void *pvParameters);

//ADC Initialization Function
void ADCInit(void);

//I2C Initialization Function
void I2CInit(void);

//Function to Clear LED Matrix
void clear();

//Function to Set Brightness of LED Matrix
void setBrightness(uint8_t val);

//Function to Control LED Color display
void write_display_intensity_control(enum Ctrl_t ctrl);

//Unique to Clear entire display
void writeDisplay(void);

//Function to Control Average Sound Intensity based LED Color display
void write_display_intensity_mid_control();



#endif /* MAIN_H_ */
