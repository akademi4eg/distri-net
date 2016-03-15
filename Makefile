CXX = g++
CPPFLAGS = -Wall
CXXFLAGS = -g
LDFLAGS =
WORKER_BINARY = distnet_worker
RM = rm -f
BUILD_PATH = build
SRC_PATH = src

all: worker

worker:
	$(CXX) -o $(BUILD_PATH)/$(WORKER_BINARY) $(SRC_PATH)/*.cpp

clean:
	$(RM) $(BUILD_PATH)/*.o $(BUILD_PATH)/$(WORKER_BINARY)
