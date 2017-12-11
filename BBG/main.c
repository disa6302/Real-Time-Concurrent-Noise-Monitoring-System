/*
* FileName        :    main.c
* Description     :    This file contains concurrent SW implementation of a Realtime
                       Concurrent Noise Monitoring System
*                        
* File Author Name:    Bhallaji Venkatesan, Divya Sampath Kumar
* Tools used      :    gcc, gedit, Sublime Text
* References      :    
*
*
*/

#include <stdio.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include "message.h"

#define LOG_QUEUE "/log_queue"
#define SCKT_LOG_QUEUE "/sock_log_queue"
#define EEPROM_LOG_QUEUE "/eeprom_log_queue"
#define PATTERN_QUEUE "/pattern_queue"
#define DECISION_QUEUE "/decision_queue"
#define MAX_LOG_SIZE     9999

char logbuff[MAX_LOG_SIZE];

pthread_t socket_thread;
pthread_t eeprom_thread;
pthread_t synclogger_thread;
pthread_t shutdown_thread;
pthread_t decision_thread;

pthread_cond_t sig_logger;
pthread_mutex_t logger_mutex;

struct mq_attr mq_attr_log_queue;
struct mq_attr mq_attr_sock_queue;
struct mq_attr mq_attr_eeprom_queue;
struct mq_attr mq_attr_pattern_queue;
struct mq_attr mq_attr_decision_queue;

static mqd_t log_queue;
static mqd_t sock_log_queue;
static mqd_t eeprom_log_queue;
static mqd_t pattern_queue;
static mqd_t decision_queue;

static volatile sig_atomic_t counter =0;
FILE *fp;


logpacket msg_main, msg_socket, msg_eeprom;



//Create Following Tasks
/* 
1.Socket Task
2.Logger Task
3.EEPROM Task
4.Pattern Task
5.Shutdown Task
6.Decision Thread
*/


//Create Following Message Queues
/*
1.MQ for Socket-Logger tasks
2.MQ for EEPROM-Logger tasks
3.MQ for Pattern Task Logger to read from EEPROM/Logger task
*/ 


//Exit Handler
void exit_handler(int sig) {
    if (sig != SIGINT) {
        printf("Received invalid signum = %d in exit_handler()\n", sig);
        exit(0);
    }
    
    printf("Received SIGINT. Exiting Application\n");
    pthread_cancel(socket_thread);
    pthread_cancel(eeprom_thread);
    pthread_cancel(synclogger_thread);
    pthread_cancel(decision_thread);
    mq_close(sock_log_queue);
    mq_unlink(SCKT_LOG_QUEUE);
    mq_close(log_queue);
    mq_unlink(LOG_QUEUE);
    mq_close(eeprom_log_queue);
    mq_unlink(EEPROM_LOG_QUEUE);
    mq_close(pattern_queue);
    mq_unlink(PATTERN_QUEUE);
    mq_close(decision_queue);
    mq_unlink(DECISION_QUEUE);    
    if(fp!=NULL)
    fclose(fp);
    exit(0);
}


//API For Logging from Socket Task
enum Status api_send_socket_log(logpacket msg)
{
    uint8_t status;
    msg.sourceid = SRC_SOCKET;
    
    status=mq_send(log_queue, (const logpacket*)&msg, sizeof(msg),1);
    if(status == -1)
    {
        printf("\nERR: Socket Task was unable to send log message\n");
        return FAIL;
    }
    pthread_cond_signal(&sig_logger);


    return SUCCESS;
}

//API for Logging from EEPROM Task
enum Status api_send_eeprom_log(logpacket msg)
{
    uint8_t status;
    msg.sourceid = SRC_EEPROM;
    status=mq_send(log_queue, (const logpacket*)&msg, sizeof(msg),1);
    if(status == -1)
    {
        printf("\nERR: EEPROM Task was unable to send log message\n");
        return FAIL;
    }
    pthread_cond_signal(&sig_logger);


    return SUCCESS;
}

//API For logging from MAIN
enum Status api_send_main_log(logpacket msg)
{
    uint8_t status;
    msg.sourceid = SRC_MAIN;
    status=mq_send(log_queue, (const logpacket*)&msg, sizeof(msg),1);
    if(status == -1)
    {
        printf("\nERR: EEPROM Task was unable to send log message\n");
        return FAIL;
    }
    pthread_cond_signal(&sig_logger);


    return SUCCESS;
}

void *app_socket_task(void *args) // SocketThread/Task
{
    uint32_t audio_val = 1996;
    printf("\nEntered Socket Task\n");
}


void *app_eeprom_task(void *args) // SocketThread/Task
{
    printf("\nEntered EEPROM Task\n");
}

void *app_sync_logger(void *args) // SocketThread/Task
{
    printf("\nEntered Logger Task\n");
    if(!args)
    {
        printf("\nERR:Invalid Log Filename!\n");
        pthread_exit(NULL);
    }
    char *fname = (char*)args;
    
    int status =0;
    if(!(fp= fopen(fname,"w+")))
    {
        printf("\nERR:Invalid Log Filename fp!\n");
        pthread_exit(NULL);
    } 
    uint32_t usecs;
    usecs = 2000000;

    //Wait on temperature or light Thread to log
    while(1)
    {
        pthread_cond_wait(&sig_logger, &logger_mutex);
        usleep(usecs);    

    }
}

void *app_decision_task(void *args) // SocketThread/Task
{
    printf("\nEntered Decision Task\n");
}

int main(int argc, char **argv)
{
    
    if (argc < 2)
    {
      printf("USAGE:  <log filename>\n");
      exit(1);
    }
    char *logfname = malloc(50 *sizeof(char));
    if(!logfname)
    {
    	printf("\nERR:Malloc Failed!\n");
		return 1;

    }
    if(!memcpy(logfname,argv[1],strlen(argv[1])))
  	{
  		printf("\nERR:Memcpy Failed!\n");
		return 1;
  	}
    
    printf("%ld",sizeof(argv[1]));
    printf("%s",logfname);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int temp_counter = 659;
    int status;
  
    uint32_t usecs_send;
    usecs_send = 4000000;
    uint32_t usecs_total = 4000000;


    signal(SIGINT, exit_handler);
    counter = 0;
    mq_attr_log_queue.mq_maxmsg = 10;
    mq_attr_log_queue.mq_msgsize = sizeof(logpacket);
    mq_attr_sock_queue.mq_maxmsg = 10;
    mq_attr_sock_queue.mq_msgsize = sizeof(logpacket);

    mq_attr_eeprom_queue.mq_maxmsg = 10;
    mq_attr_eeprom_queue.mq_msgsize = sizeof(logpacket);
    
    mq_attr_pattern_queue.mq_maxmsg = 10;
    mq_attr_pattern_queue.mq_msgsize = sizeof(logpacket);

    mq_attr_decision_queue.mq_maxmsg = 10;
    mq_attr_decision_queue.mq_msgsize = sizeof(logpacket);

    mq_unlink(LOG_QUEUE);
    log_queue = mq_open(LOG_QUEUE,O_CREAT|O_RDWR|O_NONBLOCK, 0666, &mq_attr_log_queue);
    mq_unlink(PATTERN_QUEUE);
    pattern_queue = mq_open(PATTERN_QUEUE,O_CREAT|O_RDWR|O_NONBLOCK, 0666, &mq_attr_pattern_queue);
    mq_unlink(DECISION_QUEUE);
    decision_queue = mq_open(DECISION_QUEUE,O_CREAT|O_RDWR|O_NONBLOCK, 0666, &mq_attr_decision_queue);

    if(log_queue == -1)
    {
        printf("\nUnable to open Log message queue! Exiting\n");
        exit(0);
    }

    if(sock_log_queue == -1)
    {
        printf("\nUnable to open Socket message queue! Exiting\n");
        exit(0);
    }

    if(eeprom_log_queue == -1)
    {
        printf("\nUnable to open eeprom message queue! Exiting\n");
        exit(0);
    }

    if(pattern_queue == -1)
    {
        printf("\nUnable to open pattern message queue! Exiting\n");
        exit(0);
    }

    if(decision_queue == -1)
    {
        printf("\nUnable to open decision message queue! Exiting\n");
        exit(0);
    }
    if(pthread_cond_init(&sig_logger,NULL))
    {
        printf("\nERR: Failure to init condition\n");
        
    }
    
    if(pthread_mutex_init(&logger_mutex,NULL))
    {
        printf("\nERR: Failure to init condition\n");
        
    }
    /*
    //To modify attributes to improve scheduling efficiency and task synchronization

    /* Creating Temp Sensor Thread */

    if(pthread_create(&socket_thread, &attr, (void*)&app_socket_task, NULL))
    {
    	printf("\nERR: Failure to create socket thread\n");
     
    }

    /* Creating Light Sensor Thread */
    if(pthread_create(&eeprom_thread, &attr, (void*)&app_eeprom_task, NULL))
    {
    	printf("\nERR: Failure to create thread\n");
    	 
    }
    
    /* Creating Synchronization Logger Thread */
    if(pthread_create(&synclogger_thread, &attr, (void*)&app_sync_logger, logfname))
    {
    	printf("\nERR: Failure to create thread\n");
   
    }
    
    if(pthread_create(&decision_thread, &attr, (void*)&app_decision_task, NULL))
    {
        printf("\nERR: Failure to create thread\n");
       
    }
    
    while(1)
    {
        usleep(6000000);
        strcpy(msg_main.logmsg,"Log From Main");
        gettimeofday(&msg_main.time_stamp, NULL);   
        api_send_main_log(msg_main);    
    }

    pthread_join(socket_thread, NULL);
 	pthread_join(eeprom_thread, NULL);
	pthread_join(synclogger_thread, NULL);
    pthread_join(decision_thread, NULL);
    exit_handler(SIGINT);
	return 0;

}