
/**********************************************************************************************
​ * ​ ​ Redistribution,​ ​ modification​ ​ or​ ​ use​ ​ of​ ​ this​ ​ software​ ​ in​ ​ source​ ​ or​ ​ binary
​ * ​ ​ forms​ ​ is​ ​ permitted​ ​ as​ ​ long​ ​ as​ ​ the​ ​ files​ ​ maintain​ ​ this​ ​ copyright.​ ​ Users​ ​ are
​ * ​ ​ permitted​ ​ to​ ​ modify​ ​ this​ ​ and​ ​ use​ ​ it​ ​ to​ ​ learn​ ​ about​ ​ the​ ​ field​ ​ of​ ​ embedded
​ * ​ ​ software.​ ​ Alex​ ​ Fosdick​ ​ and​ ​ the​ ​ University​ ​ of​ ​ Colorado​ ​ are​ ​ not​ ​ liable​ ​ for
​ * ​ ​ any​ ​ misuse​ ​ of​ ​ this​ ​ material.
​ *
***********************************************************************************************/
/**
​ * ​ ​ @file​ ​ decision_led.h
​ * ​ ​ @brief​ ​ User LED Access header file
 *
​ * ​ ​ @author​ ​  Bhallaji Venkatesan, Divya Sampath Kumar
​ * ​ ​ @date​ ​    12/10/2017
​ *
​ */
#ifndef __DECISION_LED_H_INCLUDED
#define __DECISION_LED_H_INCLUDED

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

uint8_t dec_led_toggle();
uint8_t dec_led1_off();
uint8_t dec_led1_on();
uint8_t dec_led2_toggle();
uint8_t dec_led2_off();
uint8_t dec_led2_on();

#endif

