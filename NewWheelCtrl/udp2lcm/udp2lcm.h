#ifndef UDP2LCM_H
#define UDP2LCM_H

#include "path_ctrl_t.h"
#include "pose_t.h"
#include "robot_control_t.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024
#define PORT 5001

extern pthread_t udpRecv, udpSend;
extern int8_t heartBeat[9]; // 用于存储需要发送的心跳信息
extern pthread_mutex_t heartBeatMutex; // 需要给heartBeat加线程锁
// clientAddr/clientIP record the latest control client IP. In scheme A,
// heartbeat is always sent back to clientIP:PORT (5001).
extern struct sockaddr_in clientAddr;
extern char clientIP[20];
extern lcm_t *lcm;

bool LCMInit();
void *udpSendHandler(void *args);
void *udpRecvHandler(void *args);
void parseCmd(const char *buffer, int bytesReceived);
void serviceCommandHandler(const lcm_recv_buf_t *rbuf, const char *channel,
                           const robot_control_t *msg, void *userdata);
void robotCtrlInit(robot_control_t *robotCtrlData, int64_t utime,
                   int8_t commandid, int8_t robotid, int8_t ndparams,
                   int8_t niparams, int8_t nsparams, int64_t nbparams);
void freeRobotCtrl(robot_control_t *robotCtrlData);
void poseHandler(const lcm_recv_buf_t *rbuf, const char *channel,
                 const pose_t *msg, void *userdata);
int16_t swapEndian(int16_t val);

#endif
