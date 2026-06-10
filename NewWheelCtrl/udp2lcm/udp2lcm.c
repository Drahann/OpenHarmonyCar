// Encoded in UTF-8
#include "udp2lcm.h"
#include <sys/signal.h>

pthread_t udpRecv/* UDP 接收线程，处理来自手机或其他设备的命令 */, udpSend/* 发送线程 */;
int8_t heartBeat[9] = {0};
pthread_mutex_t heartBeatMutex;
struct sockaddr_in clientAddr;
char clientIP[20] = "";
lcm_t *lcm;

pid_t httpServerPid = -1;

static int16_t diag_pt1_x = -1, diag_pt1_y = -1;
static int16_t diag_pt2_x = -1, diag_pt2_y = -1;
static int mapCreateRequestedFromHeartbeat = 0;

static bool defaultMapExists(void) {
    return access("/data/test/defultMap.txt", F_OK) == 0;
}

/* 捕获 SIGINT 信号（通常是 Ctrl+C），清理资源并退出程序 */
void sigIntHandler(int sig) {
    if (httpServerPid != -1) {
        kill(httpServerPid, SIGINT);
    }
    printf("Exiting...\n");
    exit(sig);
}

int main() {
    // 当遇到Ctrl-C时，递归地杀死所有子进程
    signal(SIGINT, sigIntHandler);
    // 初始化LCM
    if (!LCMInit()) {
        fprintf(stderr, "Error: Failed to initialize LCM\n");
        return -1;
    }
    // 初始化线程锁
    pthread_mutex_init(&heartBeatMutex, NULL);
    // 开启udp接收线程
    pthread_create(&udpRecv, NULL, udpRecvHandler, NULL);
    // 订阅 LCM 通道 CURRENTPOSE，使用回调函数 poseHandler 处理位置消息
    pose_t_subscribe(lcm, "CURRENTPOSE", poseHandler, NULL);
    if ((httpServerPid = fork()) == 0) {
        // 拉起服务器进程，App 通过 http://<紫派IP>:8000/defultMap.txt 拉取地图
        // ATTENTION: 这里的路径需要根据实际存储地图的路径来修改
        chdir("/data/test");
        /*
         * execlp使用系统调用exec()执行一个程序
         * 第一个参数是文件名，将在PATH中查找，最后一个参数必须是NULL
         * 其他的是传递过去的参数，按一般约定，第一个参数是程序名，不被使用
         * 因而这里出现了两个python
         */
        execlp("python", "python", "-m", "http.server", NULL);
    }
    while (true) {
        // pause();
        lcm_handle(lcm);
    }
    return 0;
}

bool LCMInit() {
    // 多播地址范围：224.0.0.0~239.255.255.255
    if ((lcm = lcm_create(NULL)) == NULL) {
        fprintf(stderr, "Error: Failed to create Path Ctrl LCM\n");
        return false;
    }
    return true;
}

/* 根据接收到的 buffer（UDP 消息内容）执行相应的命令逻辑，包括路径规划、建图、导航等 */
void parseCmd(const char *buffer, int bytesReceived) {
    if (buffer == NULL) {
        fprintf(stderr, "Error: Invalid message\n");
        return;
    }
    printf("Received UDP message (9 bytes): ");
    for (int i = 0; i < 9; i++) {
        printf("%02hhx ", buffer[i]);
    }
    printf("\n");

    path_ctrl_t path;
    robot_control_t robotCtrlData;
    pose_t curPos;
    if (buffer[0] == 0) {
        /*
         * 消息类型由 buffer[0] 指定：
         *  0：建立心跳连接
         *  1：控制小车移动
         *  2：结束建图并发布控制命令
         *  3：设置目标点
         *  4：取消导航
         *  5：加载地图
         */
        // 手机端用来与udp2lcm服务器建立连接的初始化消息
        // 也是手机端的心跳
        if (defaultMapExists() || mapCreateRequestedFromHeartbeat) {
            printf("Heartbeat received, skip automatic create map. mapExists=%d requested=%d\n",
                   defaultMapExists() ? 1 : 0, mapCreateRequestedFromHeartbeat);
            return;
        }
        // 兼容旧平板：首次连接且没有默认地图时，才自动下达30号建图命令。
        mapCreateRequestedFromHeartbeat = 1;
        robotCtrlInit(&robotCtrlData, 0, 30, 0, 1, 1, 0, 0);
        robotCtrlData.dparams[0] = 0.05;    // double类型数据值为0.05
        robotCtrlData.niparams = 1; // int类型数据个数为1
        robotCtrlData.iparams[0] = 1;   // int类型数据值为1
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        freeRobotCtrl(&robotCtrlData);
    } else if (buffer[0] == 1) {
        // 建图过程中，手机端遥控小车进行移动，命令下发至轮控模块
        path.cmd = buffer[1];
        path.speed = buffer[2];
        path_ctrl_t_publish(lcm, "wheel_ctrl", &path);
    } else if (buffer[0] == 2) {
        /*
         * 结束建图，主要有以下几个任务：
         * - 向轮控模块发送停止命令
         * - 通知算法模块结束建图
         * - 拉起http服务器进程（改由main拉起，省的每次拉起浪费资源）
         * - 向手机端通知建图完成
         * 开始通过lcm接受算法给出的实时坐标，存储至heartBeat数组中（对应格式）
         */
        path.cmd = 0;   // cmd = 0 表示停止
        path.speed = 0;
        // 通过发布 wheel_ctrl 消息，通知轮控模块停止所有动作
        path_ctrl_t_publish(lcm, "wheel_ctrl", &path);

        printf("Save Map!\n");
        robotCtrlInit(&robotCtrlData, 0, 32, 0, 0, 0, 0, 0);    // 32号命令，保存地图
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        freeRobotCtrl(&robotCtrlData);

        sleep(2);

        robotCtrlInit(&robotCtrlData, 0, 10, 0, 7, 0, 0, 0);
        pthread_mutex_lock(&heartBeatMutex);
        // 通过 heartBeat 数据获取实时坐标，并设置控制命令的参数
        int16_t x, y, sita;
        // 提取实时坐标信息并转换字节序
        x = swapEndian(*(int16_t *)&heartBeat[3]);
        y = swapEndian(*(int16_t *)&heartBeat[5]);
        sita = swapEndian(*(int16_t *)&heartBeat[7]);
        // 打印当前位置
        printf("NOW point: x: %d, y: %d, sita: %d\n", x, y, sita);
        // 将坐标数据转换为浮点类型并存储到命令参数中
        robotCtrlData.dparams[4] = (double)x / 20;
        robotCtrlData.dparams[5] = (double)y / 20;
        robotCtrlData.dparams[6] = (double)sita * M_PI / 180;
        pthread_mutex_unlock(&heartBeatMutex);
        // 发布包含实时坐标的命令到 "ROBOT_CONTROL" 通道
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        // 释放资源
        freeRobotCtrl(&robotCtrlData);
    } else if (buffer[0] == 3) {
        // 消息类型为 3：设置目标点
        // 手机发来的终点坐标，大端党，获取之后转发给算法模块
        int x, y, sita;
        // 提取并转换目标点坐标
        x = swapEndian(*(int16_t *)&buffer[3]);
        y = swapEndian(*(int16_t *)&buffer[5]);
        sita = 0;   // 默认目标角度为 0
        // 初始化控制命令，命令 ID 为 20
        robotCtrlInit(&robotCtrlData, 0, 20, 0, 3, 0, 0, 0);
        // 设置目标点的坐标参数
        robotCtrlData.dparams[0] = (double)x / 20;
        robotCtrlData.dparams[1] = (double)y / 20;
        robotCtrlData.dparams[2] = sita;
        // 发布目标点到 "ROBOT_CONTROL" 通道
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        // 释放资源
        freeRobotCtrl(&robotCtrlData);
    } else if (buffer[0] == 4) {
        // 消息类型为 4：取消导航
        // 取消导航，删除上一个目标点
        // 初始化停止导航命令，命令 ID 为 23
        robotCtrlInit(&robotCtrlData, 0, 23, 0, 0, 0, 0, 0);
        // TODO 第一次停止指令
        // 第一次下达停止指令
        printf("Cancel Navigation!\n");
        path.cmd = 4;
        path_ctrl_t_publish(lcm, "wheel_ctrl", &path);
        /////////////////////////////
        // 发布取消导航命令
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        // TODO 再次下达停止指令
        // 等待 2 秒后再次下达停止指令
        sleep(2);
        path.cmd = 5;
        printf("Stop but can recieve cmd\n");
        path_ctrl_t_publish(lcm, "wheel_ctrl", &path);
        /////////////////////////
    } else if (buffer[0] == 5) {
        // 消息类型为 5：加载地图
        /*
        加载地图，需要注意的是所加载的地图是由电脑控制端下发的
        该部分由建图部分修改
        */
        /*
         * 加载地图的逻辑：
         * - 停止小车
         * - 获取当前坐标并重置相关参数
         */
        path.cmd = 0;   // 停止指令
        path.speed = 0;
        path_ctrl_t_publish(lcm, "wheel_ctrl", &path);

        printf("Load Map!\n");
        // robotCtrlInit(&robotCtrlData, 0, 32, 0, 0, 0, 0, 0);
        // robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        // freeRobotCtrl(&robotCtrlData);

        // sleep(2);

        // 获取实时坐标并初始化相关参数
        robotCtrlInit(&robotCtrlData, 0, 10, 0, 7, 0, 0, 0);
        pthread_mutex_lock(&heartBeatMutex);
        int16_t x, y, sita;
        x = swapEndian(*(int16_t *)&heartBeat[3]);
        y = swapEndian(*(int16_t *)&heartBeat[5]);
        sita = swapEndian(*(int16_t *)&heartBeat[7]);
        printf("NOW point: x: %d, y: %d, sita: %d\n", x, y, sita);
        // 设置默认参数为 0（表示清除）
        robotCtrlData.dparams[4] = 0;
        robotCtrlData.dparams[5] = 0;
        robotCtrlData.dparams[6] = 0;
        pthread_mutex_unlock(&heartBeatMutex);
        // 发布加载地图命令
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        // 释放资源
        freeRobotCtrl(&robotCtrlData);
    } else if (buffer[0] == 'm') {
        printf("Force Create Map!\n");
        mapCreateRequestedFromHeartbeat = 1;
        robotCtrlInit(&robotCtrlData, 0, 30, 0, 1, 2, 0, 0);
        robotCtrlData.dparams[0] = 0.05;
        robotCtrlData.iparams[0] = 1;
        robotCtrlData.iparams[1] = 1;
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        freeRobotCtrl(&robotCtrlData);
    } else if (buffer[0] == 'f') {
        // 消息类型为 f：全息路径规划
        printf("Start Full Path Planning!\n");
        // 初始化全息路径规划命令，命令 ID 为 127
        robotCtrlInit(&robotCtrlData, 0, 127, 0, 0, 1, 0, 0);
        printf("%d ", buffer[1]);
        robotCtrlData.iparams[0] = buffer[1];
        printf("%d\n", robotCtrlData.iparams[0]);
        // 发布执行全息路径规划命令
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
    } else if (buffer[0] == 'h') {
        // 消息类型为 h：全息路径规划选择房间顶点
        printf("buffer[0] = 'h'!\n");
        // 初始化选择房间顶点命令，命令 ID 为 125
        robotCtrlInit(&robotCtrlData, 0, 125, 0, 3, 1, 0, 0);
        robotCtrlData.iparams[0] = buffer[1];
        // 手机发来的终点坐标，大端党，获取之后转发给算法模块
        int x, y, sita;
        // 提取并转换目标点坐标
        x = swapEndian(*(int16_t *)&buffer[3]);
        y = swapEndian(*(int16_t *)&buffer[5]);
        sita = 0;   // 默认目标角度为 0
        // 设置目标点的坐标参数
        robotCtrlData.dparams[0] = (double)x / 20;
        robotCtrlData.dparams[1] = (double)y / 20;
        robotCtrlData.dparams[2] = sita;
        // 发布房间顶点到 "ROBOT_CONTROL" 通道
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        // 释放资源
        freeRobotCtrl(&robotCtrlData);
    } else if (buffer[0] == 'g') {
        // 消息类型为 g：取消全息路径规划
        printf("Cancel Full Path Planning!\n");
        // 初始化取消全息路径规划命令，命令 ID 为 126
        robotCtrlInit(&robotCtrlData, 0, 126, 0, 0, 0, 0, 0);
        // 发布取消全息路径规划命令
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
    } else if (buffer[0] == 'i') {
        // 消息类型为 i：从属机拉取主机地图
        printf("Start Getting The Map!\n");
        // 初始化从属机拉取主机地图命令，命令 ID 为 124
        robotCtrlInit(&robotCtrlData, 0, 124, 0, 0, 4, 0, 0);
        robotCtrlData.iparams[0] = buffer[1];
        robotCtrlData.iparams[1] = buffer[2];
        robotCtrlData.iparams[2] = buffer[4];
        robotCtrlData.iparams[3] = buffer[6];
        // 发布从属机拉取主机地图命令
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
    }  else if (buffer[0] == 'j') {
        // 'j' 消息表示加载地图并接受主机下发目标点位置
        printf("Received GOAL pose from master via 'j' message\n");
        fflush(stdout);
        path.cmd = 0;   // 停止指令
        path.speed = 0;
        path_ctrl_t_publish(lcm, "wheel_ctrl", &path);

        printf("Load Map!\n");
        fflush(stdout);
        // robotCtrlInit(&robotCtrlData, 0, 32, 0, 0, 0, 0, 0);
        // robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        // freeRobotCtrl(&robotCtrlData);

        // sleep(2);

        // 获取实时坐标并初始化相关参数
        robotCtrlInit(&robotCtrlData, 0, 10, 0, 7, 0, 0, 0);
        pthread_mutex_lock(&heartBeatMutex);
        int16_t x1, y1, sita;
        x1 = swapEndian(*(int16_t *)&heartBeat[3]);
        y1 = swapEndian(*(int16_t *)&heartBeat[5]);
        sita = swapEndian(*(int16_t *)&heartBeat[7]);
        printf("NOW point: x: %d, y: %d, sita: %d\n", x1, y1, sita);
        fflush(stdout);
        // 设置默认参数为 0（表示清除）
        robotCtrlData.dparams[4] = x1 / 20;
        robotCtrlData.dparams[5] = y1 / 20;
        robotCtrlData.dparams[6] = sita * M_PI / 180.0;
        pthread_mutex_unlock(&heartBeatMutex);
        // 发布加载地图命令
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        // 释放资源
        freeRobotCtrl(&robotCtrlData);
        //加载地图完毕
        // 解析坐标信息（注意为大端存储）
        int16_t x_raw = (buffer[3] << 8) | buffer[4];
        int16_t y_raw = (buffer[5] << 8) | buffer[6];
        int16_t theta_raw = 0; // 默认角度为 0
        // 转换为实际单位（米 / 弧度）
        double x = (double)x_raw / 20.0;
        double y = (double)y_raw / 20.0;
        double theta = (double)theta_raw * M_PI / 180.0;


        // 构造 LCM 消息，发布目标点（20号命令）
        robot_control_t robotCtrlData;
        robotCtrlInit(&robotCtrlData, 0, 20, 0, 3, 0, 0, 0);  // commandid=20 代表设置目标点
        robotCtrlData.dparams[0] = x;
        robotCtrlData.dparams[1] = y;
        robotCtrlData.dparams[2] = theta;

        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        freeRobotCtrl(&robotCtrlData);
    } else if (buffer[0] == 'k') {
        diag_pt1_x = swapEndian(*(int16_t *)&buffer[3]);
        diag_pt1_y = swapEndian(*(int16_t *)&buffer[5]);
        printf("Received diagonal point 1: (%d, %d)\n", diag_pt1_x, diag_pt1_y);fflush(stdout);
    } else if (buffer[0] == 'l') {
        int robot_id = buffer[1];  // 0：主机，1：从机
        diag_pt2_x = swapEndian(*(int16_t *)&buffer[3]);
        diag_pt2_y = swapEndian(*(int16_t *)&buffer[5]);
        printf("Received diagonal point 2: (%d, %d)\n", diag_pt2_x, diag_pt2_y);fflush(stdout); 
        
        if (diag_pt1_x != -1 && diag_pt2_x != -1 && robot_id == 1) {
            printf("build FullRoad file for robot %d\n", robot_id);
            pthread_mutex_lock(&heartBeatMutex);
            robotCtrlInit(&robotCtrlData, 0, 10, 0, 7, 0, 0, 0);
            // 通过 heartBeat 数据获取实时坐标，并设置控制命令的参数
            int16_t x, y, sita;
            // 提取实时坐标信息并转换字节序
            x = swapEndian(*(int16_t *)&heartBeat[3]);
            y = swapEndian(*(int16_t *)&heartBeat[5]);
            sita = swapEndian(*(int16_t *)&heartBeat[7]);
            // 打印当前位置
            printf("NOW point: x: %d, y: %d, sita: %d\n", x, y, sita);fflush(stdout);
            // 将坐标数据转换为浮点类型并存储到命令参数中
            pthread_mutex_unlock(&heartBeatMutex);
            robotCtrlData.dparams[4] = x / 20.0;
            robotCtrlData.dparams[5] = y / 20.0;
            robotCtrlData.dparams[6] = sita * M_PI / 180.0;
            // 发布加载地图命令
            robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
            // 释放资源
            freeRobotCtrl(&robotCtrlData);
            robot_control_t robotCtrlData;
            robotCtrlInit(&robotCtrlData, 0, 122, 0, 7, 0, 0, 0);  // 调用FullRoad路径规划命令
            robotCtrlData.dparams[0] = (double)x / 20;
            robotCtrlData.dparams[1] = (double)y / 20;
            robotCtrlData.dparams[2] = (double)sita * M_PI / 180;
            robotCtrlData.dparams[3] = (double)diag_pt1_x / 20;
            robotCtrlData.dparams[4] = (double)diag_pt1_y / 20;
            robotCtrlData.dparams[5] = (double)diag_pt2_x / 20;
            robotCtrlData.dparams[6] = (double)diag_pt2_y / 20;

            printf("Publishing FullRoad path planning command (122)...\n");fflush(stdout);
            robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
            freeRobotCtrl(&robotCtrlData);
        }
        
        printf("Robot %d start distributed path following\n", robot_id);fflush(stdout);

        // 主机器人(非建图机器人)加载地图
        if (robot_id == 0) {
            path.cmd = 0;
            path.speed = 0;
            path_ctrl_t_publish(lcm, "wheel_ctrl", &path);

            robot_control_t robotCtrlData;
            robotCtrlInit(&robotCtrlData, 0, 10, 0, 7, 0, 0, 0);  // 加载地图命令
            pthread_mutex_lock(&heartBeatMutex);
            int16_t x = swapEndian(*(int16_t *)&heartBeat[3]);
            int16_t y = swapEndian(*(int16_t *)&heartBeat[5]);
            int16_t sita = swapEndian(*(int16_t *)&heartBeat[7]);
            printf("NOW point: x: %d, y: %d, sita: %d\n", x, y, sita);fflush(stdout);
            pthread_mutex_unlock(&heartBeatMutex);

            robotCtrlData.dparams[4] = x / 20.0;
            robotCtrlData.dparams[5] = y / 20.0;
            robotCtrlData.dparams[6] = sita * M_PI / 180.0;
            robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
            freeRobotCtrl(&robotCtrlData);
        }

        // 调用分布式路径执行逻辑
        robot_control_t robotCtrlData;
        robotCtrlInit(&robotCtrlData, 0, 123, 0, 1, 1, 0, 0);  // 分布式路径跟踪执行命令
        robotCtrlData.iparams[0] = robot_id;  // 标记机器人编号
        robot_control_t_publish(lcm, "ROBOT_CONTROL", &robotCtrlData);
        freeRobotCtrl(&robotCtrlData);
    } else {
        // 未知的消息类型，输出错误信息
        fprintf(stderr, "Error: Invalid command: ");
        for (int i = 0; i < 9; i++) {
            fprintf(stderr, "%02hhx ", buffer[i]);  // 打印消息内容
        }
        fprintf(stderr, "\n");
    }
}

/* 
 * 处理位姿（pose）消息的回调函数。
 * 接收 LCM 消息后，将机器人的当前位置转换为指定格式，
 * 并存储到全局的 `heartBeat` 数组中以供其他模块使用。
 */
/* 函数从 LCM 消息中接收位姿信息，包含位置（pos[0], pos[1]）和角度（pos[2]）*/
void poseHandler(const lcm_recv_buf_t *rbuf, const char *channel,
                 const pose_t *msg, void *userdata) {
    // 定义变量存储当前位置的整数表示
    int16_t x, y, sita;
    // 将收到的位置信息转换为整数格式
    // msg->pos[0] 是 x 坐标（单位：米），乘以 20 转换为整数（单位：厘米）
    x = (int16_t)((double)msg->pos[0] * 20);
    // msg->pos[1] 是 y 坐标（单位：米），乘以 20 转换为整数（单位：厘米）
    y = (int16_t)((double)msg->pos[1] * 20);
    // msg->pos[2] 是角度（单位：弧度），先转换为角度（乘以 180 / π），再转换为整数
    // 将角度 sita 限制在 [-180, 180] 范围内
    sita = (int16_t)((double)msg->pos[2] * 180 / M_PI);
    while (sita > 180)
        sita -= 360;    // 如果角度大于 180，减去 360 使其回归范围
    while (sita < -180)
        sita += 360;    // 如果角度小于 -180，加上 360 使其回归范围
    // 加锁，保护对全局数组 heartBeat 的访问，避免多线程并发修改    
    pthread_mutex_lock(&heartBeatMutex);
    // 填充 heartBeat 数组，用于存储当前的位姿信息
    heartBeat[0] = 0x03;    // 消息标志位，表示心跳类型为位姿更新
    heartBeat[1] = heartBeat[2] = 0;    // 保留字段，初始化为 0
    // 将 x 坐标拆分为高字节和低字节，存储到 heartBeat 数组
    heartBeat[3] = x >> 8;  // x 的高 8 位
    heartBeat[4] = x & 0xff;    // x 的低 8 位
    // 将 y 坐标拆分为高字节和低字节，存储到 heartBeat 数组
    heartBeat[5] = y >> 8;  // y 的高 8 位
    heartBeat[6] = y & 0xff;    // y 的低 8 位
    // 将角度 sita 拆分为高字节和低字节，存储到 heartBeat 数组
    heartBeat[7] = sita >> 8;   // sita 的高 8 位
    heartBeat[8] = sita;    // sita 的低 8 位
    // 解锁互斥锁，允许其他线程访问 heartBeat 数组
    pthread_mutex_unlock(&heartBeatMutex);
}

/*
 * 功能: 将一个 16 位整数 (int16_t) 的字节顺序进行高低字节交换。
 * 应用场景: 用于不同端系统（大端和小端）之间的数据交换。
 * 
 * 参数:
 * - val: 输入的 16 位整数，表示需要进行字节序转换的值。
 * 
 * 返回值:
 * - 返回一个 16 位整数，其字节顺序已被交换。
 */
int16_t swapEndian(int16_t val) { return (val << 8) | ((val >> 8) & 0xFF); }
// (val << 8): 将低 8 位移到高 8 位的位置。
// (val >> 8) & 0xFF: 将高 8 位移到低 8 位的位置，同时使用 & 0xFF 保留低 8 位。
// | : 将处理后的高 8 位和低 8 位合并成一个 16 位的整数。
