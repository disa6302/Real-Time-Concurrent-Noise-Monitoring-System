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

struct sock_struct
{
	uint32_t len;
	char sock_buff[256];
};

#define HOST_ADDR "10.0.0.166"

int main()
{
	int socketfd,n,fd;
	struct sock_struct sock_val;
	uint32_t audio_val = 1985;



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
    	sprintf(sock_val.sock_buff,"Audio Intensity : %d !! ", audio_val);
		sock_val.len = strlen(sock_val.sock_buff);
		n = write(socketfd,&sock_val,sizeof(sock_val));
		printf("number of bytes written:%d\n",n);
		if(n<0)
		{
			perror("writing into buffer\n");
		}
		audio_val++;
	}
}