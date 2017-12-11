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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <fcntl.h>


#include "message.h"


#define LOG_QUEUE "/log_queue"
#define SCKT_LOG_QUEUE "/sock_log_queue"
#define EEPROM_LOG_QUEUE "/eeprom_log_queue"
#define PATTERN_QUEUE "/pattern_queue"
#define DECISION_QUEUE "/decision_queue"
#define MAX_LOG_SIZE     9999

#define HOST_ADDR "10.0.0.166"

char logbuff[MAX_LOG_SIZE];

pthread_t socket_thread;
pthread_t eeprom_thread;
pthread_t synclogger_thread;
pthread_t shutdown_thread;
pthread_t decision_thread;

pthread_cond_t sig_logger;
pthread_mutex_t logger_mutex;

pthread_cond_t sig_from_socket;
pthread_mutex_t sig_socket_mutex;

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

int sockfd,connectionfd,ret;

int n;

char recv_sockbuff[1024];

logpacket msg_main, msg_socket, msg_eeprom;


struct sock_struct
{
	uint32_t len;
	char sock_buff[256];
};

struct sock_struct sock_val;
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
//API For Logging from EEPROM Task
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
//API For Logging from MAIN Task
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
    uint32_t audio_val = 0;
    printf("\nEntered Socket Task\n");

    char recv_buff[256];
    
    //Connect and establish Socket comm
    while(1)
    {
        usleep(5000000);
        //Wait on sock.recv or read and update audio val and then signal other tasks


        //To Do: Error Checking for Sockets
        n = read(connectionfd,&sock_val,sizeof(sock_val));
		printf("number of bytes read:%d\n",n);
		if(n<0)
		{
			perror("Receive error\n");
		}
		printf("What I received from client..:%s with size %d\n",sock_val.sock_buff,sock_val.len);
		memcpy(recv_buff,sock_val.sock_buff,(sock_val.len)*sizeof(char));
	    if(1==sscanf(recv_buff,"%*[^0123456789]%d",&audio_val))
	    {
	        printf("\nAudio value is %d\n",audio_val);
	    }
        gettimeofday(&msg_socket.time_stamp, NULL); 
        
        if(!(audio_val %2000))
        {
            audio_val = 0;
            printf("\nHigh Noise Intensity!!!");
            sprintf(msg_socket.logmsg,"High Audio Intensity : %d !! ", audio_val);
            api_send_socket_log(msg_socket); 
        }
        else
        {
            sprintf(msg_socket.logmsg,"Received Audio value is : %d !!", audio_val);
            api_send_socket_log(msg_socket); 
        }
        //To Do  : Signal EEPROM Only upon a condition
        pthread_cond_signal(&sig_from_socket);

    }
}


void *app_eeprom_task(void *args) // SocketThread/Task
{
    uint8_t soc_recv_cnt = 0;
    pthread_cond_wait(&sig_from_socket, &sig_socket_mutex);
    printf("\nEntered EEPROM Task\n");
    while(1)
    {
    	pthread_cond_wait(&sig_from_socket, &sig_socket_mutex);
    	if(!(soc_recv_cnt %253))
    		soc_recv_cnt = 0;
    	soc_recv_cnt++;
    	printf("\n Performing EEPROM Write Task After Socket recv count %d\n",soc_recv_cnt);
    }

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
        logpacket temp; 
        temp.req_type = -1;
        status = mq_receive(log_queue,(logpacket*)&temp, sizeof(temp), NULL);
        printf("\nLog Status %d\n",status);
        if(temp.logmsg !=NULL)
        {
            if(status >0)
            {
                printf("\nLog Queue message received\n");
                if(temp.sourceid == SRC_MAIN)
                {
                    sprintf(logbuff,"\n[ %ld sec, %ld usecs] MAIN :", temp.time_stamp.tv_sec,temp.time_stamp.tv_usec);
                    strncat(logbuff, temp.logmsg, strlen(temp.logmsg));
                    fwrite(logbuff,1, strlen(logbuff)*sizeof(char),fp);
                }
                if(temp.sourceid == SRC_SOCKET)
                {
                    sprintf(logbuff,"\n[ %ld sec, %ld usecs] SOCKET :", temp.time_stamp.tv_sec,temp.time_stamp.tv_usec);
                    strncat(logbuff, temp.logmsg, strlen(temp.logmsg));
                    fwrite(logbuff,1, strlen(logbuff)*sizeof(char),fp);
                }
            }
        }
        usleep(usecs);    

    }
}

void *app_decision_task(void *args) // SocketThread/Task
{
	uint8_t soc_recv_cnt = 0;
    printf("\nEntered Decision Task\n");
    while(1)
    {
    	pthread_cond_wait(&sig_from_socket, &sig_socket_mutex);
    	if(!(soc_recv_cnt %253))
    		soc_recv_cnt = 0;
    	soc_recv_cnt++;
    	printf("\n Performing Decision Task After Socket recv count %d\n",soc_recv_cnt);
    }
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

    
	char buffer[1024];
	int n;
	long counter = 0;
	struct sockaddr_in lserveraddr;
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("Error opening socket\n");
		return 0;
	}
	lserveraddr.sin_family = AF_INET;
	lserveraddr.sin_addr.s_addr = inet_addr(HOST_ADDR);
	lserveraddr.sin_port = htons(5000);

	//To configure the socket options for REUSEADDR & Timeout 
	bind(sockfd,(struct sockaddr*)&lserveraddr,sizeof(lserveraddr));
	listen(sockfd,10);
	connectionfd = accept(sockfd,(struct sockaddr*)NULL,NULL);

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
    /*mq_unlink(SCKT_LOG_QUEUE);
    sock_log_queue = mq_open(SCKT_LOG_QUEUE,O_CREAT|O_RDWR|O_NONBLOCK, 0666, &mq_attr_sock_queue);
    mq_unlink(EEPROM_LOG_QUEUE);
    eeprom_log_queue = mq_open(EEPROM_LOG_QUEUE,O_CREAT|O_RDWR|O_NONBLOCK, 0666, &mq_attr_eeprom_queue);*/
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

    if(pthread_cond_init(&sig_from_socket,NULL))
    {
        printf("\nERR: Failure to init condition\n");
        
    }
    
    if(pthread_mutex_init(&sig_socket_mutex,NULL))
    {
        printf("\nERR: Failure to init condition\n");
        
    }    
    //To modify attributes to improve scheduling efficiency and task synchronization

    /* Creating Socket Thread */

    if(pthread_create(&socket_thread, &attr, (void*)&app_socket_task, NULL))
    {
        printf("\nERR: Failure to create socket thread\n");
     
    }

    /* Creating EEPROM Thread */
    if(pthread_create(&eeprom_thread, &attr, (void*)&app_eeprom_task, NULL))
    {
        printf("\nERR: Failure to create thread\n");
         
    }
    
    /* Creating Synchronization Logger Thread */
    if(pthread_create(&synclogger_thread, &attr, (void*)&app_sync_logger, logfname))
    {
        printf("\nERR: Failure to create thread\n");
   
    }
    /* Creating Decision Thread */
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

    //Synchronization with the main thread
    pthread_join(socket_thread, NULL);
    pthread_join(eeprom_thread, NULL);
    pthread_join(synclogger_thread, NULL);
    pthread_join(decision_thread, NULL);
    exit_handler(SIGINT);
    return 0;

}