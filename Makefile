CXX = g++
INCLUDE = src
CPPFLAGS = -Wall
CXXFLAGS = -std=c++11 -g
LDFLAGS = -lamqpcpp -lPocoFoundation -lPocoNet
WORKER_BINARY = distnet_worker
MANAGER_BINARY = distnet_manager
RM = rm -f
BUILD_PATH = build
SRC_PATH = src

all: worker manager

worker:
	$(CXX) -I$(INCLUDE) $(CPPFLAGS) $(CXXFLAGS) -o $(BUILD_PATH)/$(WORKER_BINARY) $(SRC_PATH)/*.cpp $(SRC_PATH)/worker/*.cpp $(LDFLAGS)
	
manager:
	$(CXX) -I$(INCLUDE) $(CPPFLAGS) $(CXXFLAGS) -o $(BUILD_PATH)/$(MANAGER_BINARY) $(SRC_PATH)/*.cpp $(SRC_PATH)/manager/*.cpp $(LDFLAGS)

clean:
	$(RM) $(BUILD_PATH)/*.o $(BUILD_PATH)/$(WORKER_BINARY) $(BUILD_PATH)/$(MANAGER_BINARY)
