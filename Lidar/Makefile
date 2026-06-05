include mak_def.inc

CXXSRC += src/sl_lidar_driver.cpp \
		  src/hal/thread.cpp\
		  src/sl_crc.cpp\
		  src/sl_serial_channel.cpp\
		  src/sl_lidarprotocol_codec.cpp\
		  src/sl_async_transceiver.cpp\
		  src/sl_tcp_channel.cpp\
		  src/sl_udp_channel.cpp
CXXSRC += $(shell find $(CURDIR)/src/dataunpacker/ -name "*.cpp")
CXXSRC += $(shell find $(CURDIR)/src/lcmtype_dir/ -name "*.cpp")
CXXSRC += lidar_driver.cpp
ifeq ($(BUILD_TARGET_PLATFORM),Linux)
	CXXSRC += src/arch/linux/net_serial.cpp \
			  src/arch/linux/net_socket.cpp \
			  src/arch/linux/timer.cpp
	endif

C_INCLUDES += -I$(CURDIR)/include \
			  -I$(CURDIR)/include/arch/linux \
			  -I$(CURDIR)/include/arch/win32 \
			  -I$(CURDIR)/include/arch/macOS \
			  -I$(CURDIR)/include/dataunpacker \
			  -I$(CURDIR)/include/dataunpacker/unpacker \
			  -I$(CURDIR)/include/hal \
			  -I$(CURDIR)/lcmtype_dir \
			  -I$(CURDIR)/src


C_LD_LIBRARY_PATH = -L./lcmlib64
C_LIB = -llcm

OBJS = $(CXXSRC:.cpp=.o)
	TARGET = lidar_driver

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(C_LD_LIBRARY_PATH) $(C_LIB)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(C_INCLUDES) -c $< -o $@ $(C_LD_LIBRARY_PATH) $(C_LIB)

clean:
	rm -rf $(OBJS) test lidar_driver
