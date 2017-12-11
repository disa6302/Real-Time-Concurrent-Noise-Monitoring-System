/*
* FileName        : lclient.c
* Description     :	Client end on PC sending commands to user application on BBG to log audio
					sensor values into a log file

* File Author Name:	Divya Sampath Kumar. Bhallaji Venkatesan
* Tools used	  :	gcc,gdb
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>

typedef enum reqtype_t{
DEFAULT,
REQ_REMOTE_API,
REQ_ERR_REC,
REQ_PATTERN_DATA,
TX_AUDIO_DATA,
TX_ERR_LOG,
TX_INIT_LOG
}request_t;

struct sock_struct
{
	uint32_t len;
	request_t cmd;
	char sock_buff[256];
};

#define HOST_ADDR "10.0.0.166"

int main()
{
	int socketfd,n,fd;
	struct sock_struct sock_val;
	uint32_t audio_val = 1985;
	int count = 0;



	struct sockaddr_in lclientaddr;

	if((socketfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("Error opening socket\n");
		return 0;
	}

	lclientaddr.sin_family = AF_INET;
	lclientaddr.sin_addr.s_addr = inet_addr(HOST_ADDR);
	lclientaddr.sin_port = htons(5000);

	if(connect(socketfd,(struct sockaddr*)&lclientaddr,sizeof(lclientaddr))<0)
	{
		perror("Connection failed\n");
		return 0;
	}

    while(1)
    {
    	usleep(5000000);
    	//Test Requests for Initialization Logs
    	if(!count)
    	{
    		bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
    		strncpy(sock_val.sock_buff,"Initialization of TIVA-C Board!!",strlen("Initialization of TIVA-C Board!!"));
    		sock_val.len = strlen(sock_val.sock_buff);
    		sock_val.cmd = TX_INIT_LOG;
    	}
    	else if(count == 1)
    	{
    		bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
    		strncpy(sock_val.sock_buff,"Initialization of ADC MEMS Sensor!!",strlen("Initialization of ADC MEMS Sensor!!"));
    		sock_val.len = strlen(sock_val.sock_buff);
    		sock_val.cmd = TX_INIT_LOG;    		
    	}
    	else if(count == 2)
    	{
    		bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
    		strncpy(sock_val.sock_buff,"Initialization of Beautiful LED Matrix I2C!!",strlen("Initialization of Beatiful LED Matrix I2C!!"));
    		sock_val.len = strlen(sock_val.sock_buff);
    		sock_val.cmd = TX_INIT_LOG;    		
    	}  
    	//Test Requests for Simulated Audio ADC Data Logs 
    	else
    	{
    		bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
    		sprintf(sock_val.sock_buff,"Audio Intensity : %d !! ", audio_val);
			sock_val.len = strlen(sock_val.sock_buff);
			sock_val.cmd = TX_AUDIO_DATA;
		}
		//Test Requests for Simulated Error Conditions 
		if(audio_val >1999 && audio_val <2003)
		{
    		bzero(sock_val.sock_buff,sizeof(sock_val.sock_buff));
    		strcpy(sock_val.sock_buff,"ERROR: Audio Sensor Unplugged! God Save the Noisy Earth! ");
			sock_val.len = strlen(sock_val.sock_buff);
			sock_val.cmd = TX_ERR_LOG;				
		}
		count++;
		n = write(socketfd,&sock_val,sizeof(sock_val));
		printf("number of bytes written:%d\n",n);
		if(n<0)
		{
			perror("writing into buffer\n");
		}
		audio_val++;
		//Test Requests for LED off in decision task
		if(audio_val > 2005)
			audio_val = 1993;
	}
}