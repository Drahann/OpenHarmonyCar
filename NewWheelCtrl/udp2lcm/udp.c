#include "udp2lcm.h"
#include <time.h>

static long long nowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static bool isDiscoveryPing(const char *buffer, int bytesReceived) {
    return buffer != NULL && bytesReceived >= 1 &&
           (unsigned char)buffer[0] == 0x06;
}

static bool isHeartbeatMessage(const char *buffer, int bytesReceived) {
    return buffer != NULL && bytesReceived >= 1 &&
           (unsigned char)buffer[0] == 0x00;
}

static bool isZeroWheelStopMessage(const char *buffer, int bytesReceived) {
    if (buffer == NULL || bytesReceived < 9 || (unsigned char)buffer[0] != 0x01) {
        return false;
    }
    for (int i = 1; i < 9; i++) {
        if ((unsigned char)buffer[i] != 0x00) {
            return false;
        }
    }
    return true;
}

static void updateClientTarget(const struct sockaddr_in *addr) {
    pthread_mutex_lock(&heartBeatMutex);
    clientAddr = *addr;
    clientAddr.sin_port = htons(PORT);
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
    pthread_mutex_unlock(&heartBeatMutex);
}

static bool copyClientTarget(struct sockaddr_in *target) {
    pthread_mutex_lock(&heartBeatMutex);
    *target = clientAddr;
    bool hasTarget = target->sin_family == AF_INET && target->sin_port != 0;
    pthread_mutex_unlock(&heartBeatMutex);
    return hasTarget;
}

static void sendDiscoveryResponse(int socketfd, const struct sockaddr_in *addr,
                                  socklen_t addrLen) {
    int8_t response[9] = {0};
    response[0] = 0x06;
    response[1] = 0;
    response[2] = 0;

    pthread_mutex_lock(&heartBeatMutex);
    for (int i = 3; i < 9; i++) {
        response[i] = heartBeat[i];
    }
    pthread_mutex_unlock(&heartBeatMutex);

    sendto(socketfd, (const char *)response, 9, 0,
           (const struct sockaddr *)addr, addrLen);
}

void *udpRecvHandler(void *args) {
    int socketfd = -1;
    char buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in serverAddr;
    struct sockaddr_in recvAddr;
    socklen_t addrLen;
    int bytesReceived;

    int retval;
    struct pollfd fds;

    printf("udpRecvHandler\n");
    if ((socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "Error: Failed to create socket\n");
        return NULL;
    }
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    memset(buffer, 0, sizeof(buffer));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ==
        -1) {
        fprintf(stderr, "Error: Bind failed\n");
        close(socketfd);
        return NULL;
    }

    while (true) {
        memset((char *)&recvAddr, 0, sizeof(recvAddr));
        addrLen = sizeof(recvAddr);
        bytesReceived = recvfrom(socketfd, buffer, MAX_BUFFER_SIZE, 0,
                                 (struct sockaddr *)&recvAddr, &addrLen);
        if (bytesReceived == -1) {
            fprintf(stderr, "Error: Failed to receive data\n");
            close(socketfd);
            return NULL;
        }
        if (isDiscoveryPing(buffer, bytesReceived)) {
            sendDiscoveryResponse(socketfd, &recvAddr, addrLen);
            continue;
        }
        updateClientTarget(&recvAddr);
        break;
    }
    if (!isHeartbeatMessage(buffer, bytesReceived) &&
        !isZeroWheelStopMessage(buffer, bytesReceived)) {
        printf("Start UDP session from %s\n", clientIP);
        fflush(stdout);
    }

    /*
     * 接收到来自手机端的第一条消息之后：
     * - 启动udpSend线程，用于向手机端发送心跳信息
     * - 通过ROBOT_COMTROL信道向算法模块发送建图开始消息
     *   - 7号命令初始坐标、初始角度、雷达扫描最大范围、机器人半径、机器人高
     *   - 30号命令开始建图，参数为单个像素点大小
     */
    if (pthread_create(&udpSend, NULL, udpSendHandler, NULL) != 0) {
        fprintf(stderr, "Error creating udpSend thread\n");
        close(socketfd);
        exit(-1);
    }

    robot_control_t robotCtrlData;
    robotCtrlInit(&robotCtrlData, 0, 7, 0, 7, 1, 0, 0);
    robotCtrlData.iparams[0] = 1;
    robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
    freeRobotCtrl(&robotCtrlData);
    // // 随即下达30号命令，开始建图
    // robotCtrlInit(&robotCtrlData, 0, 30, 0, 1, 1, 0, 0);
    // robotCtrlData.dparams[0] = 0.05;
    // robotCtrlData.niparams = 1;
    // robotCtrlData.iparams[0] = 1;
    // robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
    // freeRobotCtrl(&robotCtrlData);

    /*
     * 开始接收来自手机端的命令
     * 限定时间为3s，超过3s未收到消息则停止轮子
     * 但不退出循环，防止是网络抖动
     */
    const path_ctrl_t path = {0, 0}; // 就是停止命令，不能改
    fds.fd = socketfd;
    fds.events = POLLIN;
    long long lastControlMsgMs = nowMs();
    while (true) {
        long long elapsedMs = nowMs() - lastControlMsgMs;
        int timeoutMs = 3000 - (int)elapsedMs;
        if (timeoutMs <= 0) {
            fprintf(stderr, "No data within three seconds.\n");
            path_ctrl_t_publish(lcm, "wheel_ctrl", &path);
            lastControlMsgMs = nowMs();
            timeoutMs = 3000;
        }
        retval = poll(&fds, 1, timeoutMs);
        if (retval == -1) {
            fprintf(stderr, "Error: select failed\n");
            break;
        } else if (retval == 0) {
            // 超出3s，报错并停止前进
            fprintf(stderr, "No data within three seconds.\n");
            path_ctrl_t_publish(lcm, "wheel_ctrl", &path);
            lastControlMsgMs = nowMs();
        } else {
            memset((char *)&recvAddr, 0, sizeof(recvAddr));
            addrLen = sizeof(recvAddr);
            bytesReceived = recvfrom(socketfd, buffer, MAX_BUFFER_SIZE, 0,
                                     (struct sockaddr *)&recvAddr, &addrLen);
            if (bytesReceived == -1) {
                fprintf(stderr, "Error: Failed to receive data\n");
                continue;
            }
            if (isDiscoveryPing(buffer, bytesReceived)) {
                sendDiscoveryResponse(socketfd, &recvAddr, addrLen);
                continue;
            }
            updateClientTarget(&recvAddr);
            lastControlMsgMs = nowMs();
            parseCmd(buffer, bytesReceived);
        }
    }
}

void *udpSendHandler(void *args) {
    // 创建UDP套接字
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "Error: Failed to create socket\n");
        return NULL;
    }

    while (true) {
        struct sockaddr_in targetAddr;
        if (!copyClientTarget(&targetAddr)) {
            usleep(500000);
            continue;
        }
        pthread_mutex_lock(&heartBeatMutex);
        sendto(sockfd, (const char *)heartBeat, 9, 0,
               (struct sockaddr *)&targetAddr, sizeof(targetAddr));
        pthread_mutex_unlock(&heartBeatMutex);
        usleep(500000);
    }
}
