该程序由 ultra_simple 示例程序修改形成的，编译完形成 lidar_driver 程序。

在终端执行 ./lidar_driver 可以驱动雷达，其效果相当于在示例程序 ultra_simple 执行 ./ultra_simple --channel --serial /dev/ttyUSB0 115200 。
若要修改端口号，需要在 lidar_driver.cpp 中找到 opt_channel_param_first。

该程序功能为收集激光点信息，存放到 laser_t 结构体中，并通过 lcm 上的 HOKUYO_LIDAR 通道发布出去。

`include/arch/linux/thread.h` 为线程相关的头文件，由于鸿蒙的交叉编译工具链不支持pthread_cancel函数，故第62行暂时注释掉，等待后续修改