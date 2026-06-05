#include "UdpClient.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

extern void NAVI_GetVisionData(float *x, float *y, int length);
extern void NAVI_ClearVisionData(void);
using namespace std;

#define MAXLINE 22
//#define SERV_PORT 9999

int Udpflag = 0;
int Sendflag = 0;
int iRet = 0;
int len;
int iAddrlen;
char buf[65536]; //�������ջ����ֽ�����
// WSADATA WSAData;
int recvSocket;
static int lastlength = 0;
static unsigned char Infoflag = 0;
struct sockaddr_in dstAddr; //���ñ��ص�ַ
struct sockaddr_in serAddr; //���ý��յ�ַ
struct sockaddr_in clientAddr;
ip_mreq mreq;

struct udp {
    int length;
    float x[2000];
    float y[2000];
};

void UdpInit() {
    if (Udpflag == 0) {
        printf("UdpInit\n");
        recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (recvSocket == 0) {
            cout << "recvSocket error!";
        }
        serAddr.sin_family = AF_INET;
        serAddr.sin_port = htons(9999);
        // inet_pton(AF_INET,INADDR_ANY,&serAddr.sin_addr);
        serAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        //��socket�󶨵�ַ
        if (::bind(recvSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == -1) {
            cout << "bind error";
            exit(EXIT_FAILURE);
            // closesocket(recvSocket);
        }
        mreq.imr_interface.s_addr = inet_addr("192.168.0.140");
        mreq.imr_multiaddr.s_addr = inet_addr("224.0.1.0");
        iRet = setsockopt(recvSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                          (char *)&mreq, sizeof(mreq));
        if (iRet != 0) {
            cout << "setsockopt multicast fail" << endl;
        }
        Sendflag = 1;
        Udpflag = 1;
        printf("UdpInit successful\n");
    }
}
#if 0
void *UDP_Send_LoopFunc(void *)
{
	while (1)
	{
		udp data;
		for (int i = 0; i < 5000; i++)
		{
			data.x[i] = 7.27f;
			data.y[i] = 0;
		}
		sendto(sendSocket, (char*)&data, sizeof(udp), 0, (struct sockaddr *)&dstAddr, sizeof(sockaddr));
		printf("send successful \n");
		
		//if (Sendflag == 0)
		//{
		//	break;
		//}
	}
}
#endif
void *UDP_Recv_LoopFunc(void *) {
    cout << "udp group start\n";
    iAddrlen = sizeof(clientAddr);
    while (true) {
        memset(buf, 0, sizeof(buf));
        len = recvfrom(recvSocket, buf, sizeof(buf), 0,
                       (struct sockaddr *)&clientAddr, (socklen_t *)&iAddrlen);
        if (len > 0) {
            struct udp rec_udp;
            memcpy(&rec_udp, buf, sizeof(rec_udp));
            if (lastlength != rec_udp.length) {
                lastlength = rec_udp.length;
                Infoflag = 1;
            }
            if ((0 == rec_udp.length) && (1 == Infoflag)) {
                NAVI_ClearVisionData();
                Infoflag = 0;
            }
            if (0 != rec_udp.length) {
                NAVI_GetVisionData(rec_udp.x, rec_udp.y, rec_udp.length);
            }
        }
    }
}
