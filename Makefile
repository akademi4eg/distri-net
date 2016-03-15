CXX = g++
CPPFLAGS = -Wall
CXXFLAGS = -std=c++11 -g
LDFLAGS =
WORKER_BINARY = distnet_worker
MANAGER_BINARY = distnet_manager
RM = rm -f
BUILD_PATH = build
SRC_PATH = src

all: worker manager

worker:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $(BUILD_PATH)/$(WORKER_BINARY) $(SRC_PATH)/worker/*.cpp
	
manager:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $(BUILD_PATH)/$(MANAGER_BINARY) $(SRC_PATH)/manager/*.cpp

clean:
	$(RM) $(BUILD_PATH)/*.o $(BUILD_PATH)/$(WORKER_BINARY) $(BUILD_PATH)/$(MANAGER_BINARY)
