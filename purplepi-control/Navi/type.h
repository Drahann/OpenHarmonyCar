#pragma once


#include     <stdio.h>     
#include     <stdlib.h>     
#include     <unistd.h>    
#include     <sys/types.h>  
#include     <sys/stat.h>   
#include     <fcntl.h>      
#include     <termios.h>    
#include     <errno.h>    
#include     <pthread.h>   
#include     <signal.h>
#include     <string.h>

#include     <sys/types.h>  
#include     <sys/stat.h>
#include     <sys/wait.h>
#include     <sys/time.h>

typedef unsigned char       BYTE;
#define Integer_MIN_VALUE -99999999
#define  Integer_MAX_VALUE 99999999



#define Double_MAX_VALUE  999999999999.9999
#define PI (3.1415926)

typedef struct {
	double	x;
	double 	y;
	double 	theta;
} Pose;

typedef struct tagPOINT
{
	long x;
	long y;

}POINT;

