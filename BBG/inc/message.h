/*
* FileName        :    message.h
* Description     :    This file contains the necessary SW support for message queue, logpackets and link layer functions
*                        
* File Author Name:    Bhallaji Venkatesan, Divya Sampath Kumar
* Tools used      :    gcc, gedit, Sublime Text
* References      :    
*
*
*/


#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <time.h>
#include <mqueue.h>

/*
​ * ​ ​ @brief​    : Enum for indicating I2C read/write status
​
 *   FAIL      : Data R/W failed
 *   SUCCESS   : Data R/W Success
 
​ */
enum Status {SUCCESS,FAIL} stat;

//Level of logs for criticality demarcation in the logfile
typedef enum loglevel_t{
LOG_DATA_VALID,     
LOG_DATA_REQ,
LOG_DATA_CONV,
LOG_CRITICAL,
LOG_INIT,
LOG_ERR
}loglevel;

//Source Task 
typedef enum srcid_t{
SRC_MAIN,
SRC_SOCKET,
SRC_EEPROM,
SRC_DECISION,
SRC_DEFAULT
}srcid;


typedef enum reqtype_t{
DEFAULT,
REQ_REMOTE_API,
REQ_ERR_REC,
REQ_PATTERN_DATA,
TX_AUDIO_DATA,
TX_ERR_LOG,
TX_INIT_LOG
}request_t;


//Log Packet
typedef struct log_packet_t{
struct timeval time_stamp;  //Timestamps use time.h
loglevel level;        //Log Levels based on criticality of logs
srcid sourceid;         //Source of Logs 
int32_t crc;           //CRC check for error checking mechanism 
request_t req_type;  
char logmsg[256];
}logpacket;


#endif