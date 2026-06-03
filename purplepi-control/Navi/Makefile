################################################################################
# created by lishen 2014-9-11
################################################################################


#########################################################
# tools chain
#########################################################
NAVI_TOP_DIR = ./
SUBDIR = $(shell ls $(NAVI_TOP_DIR)/./ -R | grep /)
SUBDIRS = $(subst :,/,$(SUBDIR))
SOURCE = $(foreach NAVI_TOP_DIR, $(SUBDIRS),$(wildcard $(NAVI_TOP_DIR)*~))

VPATH=udp:map:math:planner:lcmtype_dir:gaussestimator:particlefilter

CFLAGS+=-Iudp/
CFLAGS+=-Imap/
CFLAGS+=-Imath/
CFLAGS+=-Iplanner/
CFLAGS+=-Iparticlefilter/
CFLAGS+=-Igaussestimator/
CFLAGS+=-Ilcmtype_dir/
CFLAGS+=-Ilcmtype_dir/lcm/
 
CLIBS+=-Llcmlib64 -Llcmlib -Lzliblib

Depend_Lib  =  -lpthread -llcm -lm -lz 
#CC = g++
CC = $(CXX)

#AR = ar

OBJECT = UdpClient.o Attributes.o ContourExtractor.o Graph.o Gridmap.o GNode.o GXYTNode.o MapServer.o   MultiResolutionScanMatcher.o ScanMatcher.o UnionFindSimple.o LinAlg.o MathUtil.o mymath.o GridMapRenderer.o IntMaxHeap.o PathPlanner.o WaveFront.o NaviInterface.o laser_t.o path_t.o pose_t.o robot_control_t.o grid_map_t.o CholeskyDecomposition.o CMatrix.o CMultiGauss.o CSRAllocator.o LUDecomposition.o MultiGaussionEstimator.o Vec.o Astarplanner.o CleanPlanner.o ant.o ParticleFilter.o KLD.o main.o

all:navigation	 

navigation:$(OBJECT)
	$(CC)   -g -o navigation   $(OBJECT) $(CLIBS) $(Depend_Lib)
	
%.o:%.cpp 
	$(CC)  $(CFLAGS) $(ASFLAGS) -g -c $<

clean:
	rm -f *.o
	rm -f navigation
	rm -f $(SOURCE)