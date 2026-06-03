#include "serial.h"

int fd;         // 轮子的串口文件描述符
char *portname; // 串口设备名
struct termios tty;

/* 初始化串口通信 */
bool whellInit() {
    // portname = findUSBDev("wheel");  // 动态查找设备名
    portname = "/dev/ttyUSB1"; // "/dev/ttyUSB1 // 硬编码设备名
    if (portname == NULL) {
        fprintf(stderr, "Error: Failed to find wheel serial port\n");
        return false;
    }
    printf("Wheel serial port: %s\n", portname);
    // 以读写模式打开串口
    // O_RDWR：可读可写模式
    // O_NOCTTY：不将设备设置为控制终端
    // O_SYNC：同步I/O
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", portname, strerror(errno));
        return false;
    }

    // 设置串口参数
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error from tcgetattr: %s\n", strerror(errno));
        return false;
    }

    cfsetospeed(&tty, B115200); // 设置输出波特率为115200
    cfsetispeed(&tty, B115200); // 设置输入波特率为115200

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // ignore break signal
    tty.c_lflag = 0;                            // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;     // no remapping, no delays
    tty.c_cc[VMIN] = 0;  // read doesn't block
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);        // ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= 0;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error from tcsetattr: %s\n", strerror(errno));
        return false;
    }
    return true;
}

/* 通过串口发送控制命令 */
bool wheelSend(byte a, byte a_v, byte b, byte b_v) {
    // 数据格式为 7 字节的自定义协议：{0x53, 0x05, a, a_v, b, b_v, checksum}
    // 0x53：固定的起始字节
    // 0x05：数据包长度
    // a，a_v：第一个轮子编号和速度值
    // b，b_v：第二个轮子编号和速度值
    // checksum：校验和（前6字节异或运算的结果）
    unsigned char data[7] = {0x53, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}; // 数据格式为 7 字节的自定义协议
    byte checksum = 0;
    data[2] = a;
    // TODO: 速度转换
    // 因为我们的机器人左右轮在相同速度数值之下，真实的速度不一致
    // int temp = ((int)a_v + 2);  // 所以在这里需要调整左轮速度
    int temp = ((int)a_v);  // 不需要在这里需要调整左轮速度
    a_v = (byte)temp;
    ////////////////////////
    data[3] = a_v;
    data[4] = b;
    data[5] = b_v;

    for (int i = 0; i < 6; i++) {
        checksum ^= data[i];
    }
    data[6] = checksum;

    // 发送数据
    if (write(fd, data, 7) != 7) {
        fprintf(stderr, "Failed to write to the serial port\n");
        return false;
    }

    printf("Data %02X %02X %02X %02X %02X %02X %02X wheelSend successfully!\n",
           data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
    return true;
}