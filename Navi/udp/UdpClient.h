#pragma once

//typedef bool                    BOOL;
//
//typedef float                   REAL;
//typedef float                   FLOAT;
//typedef double                  DOUBLE;
//
//typedef char                    INT8;
//typedef unsigned char           UINT8;
//typedef short                   INT16;
//typedef unsigned short          UINT16;
//typedef int                     INT32;
//typedef unsigned int            UINT32;
//typedef long                    INT64;
//
//typedef unsigned char           BYTE;
//typedef unsigned short          WORD;
//typedef unsigned int            DWORD;



void *UDP_Send_LoopFunc(void *);
void *UDP_Recv_LoopFunc(void *);
void UdpInit();
void UdpUninit();
