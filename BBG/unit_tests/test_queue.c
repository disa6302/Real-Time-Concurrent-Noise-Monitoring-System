/*
* FileName        : test-light.c
* Description     : A software that performs unint test using unity framework for light sensor driver
* File Author Name: Bhallaji Venkatesan,Divya Sampath Kumar
* Tools used      : gcc,gdb
* Reference       : HW-2 Professor Alex Fosdick's Unity framework link
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "unity.h"
#include "message.h"
#include "decision_led.h"

#define UNOPENED_QUEUE 1234
void test_wrong_queuename(void)
{
    mqd_t log_queue;
    log_queue = mq_open("WRONG_NAME", O_CREAT | O_WRONLY | O_NONBLOCK, 0666, NULL);
    TEST_ASSERT_EQUAL_INT32(-1, (int32_t)log_queue);
}

void test_correct_queuename(void)
{
    mqd_t log_queue;
    log_queue = mq_open("/log_queue", O_CREAT | O_WRONLY | O_NONBLOCK, 0666, NULL);
    TEST_ASSERT_GREATER_THAN(-1, (int32_t)log_queue);
}

void test_unopened_queue(void)
{
    mqd_t log_queue;
    int returnval = 0;
    returnval = mq_close(UNOPENED_QUEUE);
    TEST_ASSERT_EQUAL_INT32(-1, (int32_t)returnval);   
}

void test_opened_queue(void)
{
    mqd_t decision_queue;
    int returnval = 0;
    decision_queue = mq_open("/decision_queue", O_CREAT | O_WRONLY | O_NONBLOCK, 0666, NULL);
    returnval = mq_close(decision_queue);
    TEST_ASSERT_GREATER_THAN(-1, (int32_t)returnval);   
}



int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wrong_queuename);
    RUN_TEST(test_correct_queuename);
    RUN_TEST(test_unopened_queue);
    RUN_TEST(test_opened_queue);
    return UNITY_END();
}