##################################################
#
# Call 'make library' to build infinity library
# Call 'make client' to build the examples
# Call 'make server' to build the server
# Call 'make all' to build everything
#
##################################################

PROJECT_NAME = distributedhashtable

##################################################

CC              = g++
CC_FLAGS        = -O3 -std=c++0x
LD_FLAGS        = -linfinity -libverbs

##################################################

INFINITY_LIB_NAME = libinfinity
INFINITY_SOURCE_FOLDER = infinity/src
CLIENT_SOURCE_FOLDER = App
SERVER_SOURCE_FOLDER = Persistence
COMMON_SOURCE_FOLDER = Common
BUILD_FOLDER     = build
RELEASE_FOLDER   = release
INCLUDE_FOLDER   = include

SERVER_FOLDER = server
CLIENT_FOLDER = client

##################################################
INFINITY_SOURCE_FILES =	$(INFINITY_SOURCE_FOLDER)/infinity/core/Context.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/Atomic.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/Buffer.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/Region.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/RegionToken.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/RegisteredMemory.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/queues/QueuePair.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/queues/QueuePairFactory.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/requests/RequestToken.cpp \
						$(INFINITY_SOURCE_FOLDER)/infinity/utils/Address.cpp

INFINITY_HEADER_FILES = $(INFINITY_SOURCE_FOLDER)/infinity/infinity.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/core/Context.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/core/Configuration.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/Atomic.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/Buffer.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/Region.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/RegionToken.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/RegionType.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/memory/RegisteredMemory.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/queues/QueuePair.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/queues/QueuePairFactory.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/requests/RequestToken.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/utils/Debug.h \
						$(INFINITY_SOURCE_FOLDER)/infinity/utils/Address.h

CLIENT_SOURCE_FILES =	$(CLIENT_SOURCE_FOLDER)/Client.cpp \
						$(CLIENT_SOURCE_FOLDER)/Remote.cpp \
						$(COMMON_SOURCE_FOLDER)/Connection.cpp \
						$(COMMON_SOURCE_FOLDER)/Message.cpp

CLIENT_HEADER_FILES = $(CLIENT_SOURCE_FOLDER)/Remote.hpp \
						$(CLIENT_SOURCE_FOLDER)/Benchmark.hpp \
						$(COMMON_SOURCE_FOLDER)/Connection.hpp \
						$(CLIENT_SOURCE_FOLDER)/Parser.hpp \
						$(COMMON_SOURCE_FOLDER)/Result.hpp \
						$(CLIENT_SOURCE_FOLDER)/ServerNode.hpp \
						$(COMMON_SOURCE_FOLDER)/Message.hpp


##################################################

OBJECT_FILES        = $(patsubst $(INFINITY_SOURCE_FOLDER)/%.cpp,$(BUILD_FOLDER)/%.o,$(INFINITY_SOURCE_FILES))
SOURCE_DIRECTORIES  = $(dir $(INFINITY_HEADER_FILES))
BUILD_DIRECTORIES   = $(patsubst $(INFINITY_SOURCE_FOLDER)/%,$(BUILD_FOLDER)/%,$(SOURCE_DIRECTORIES))

##################################################

all: library server client

##################################################

$(BUILD_FOLDER)/%.o: $(INFINITY_SOURCE_FILES) $(INFINITY_HEADER_FILES)
	mkdir -p $(BUILD_FOLDER)
	mkdir -p $(BUILD_DIRECTORIES)
	$(CC) $(CC_FLAGS) -c $(INFINITY_SOURCE_FOLDER)/$*.cpp -I $(INFINITY_SOURCE_FOLDER) -o $(BUILD_FOLDER)/$*.o

##################################################

library: $(OBJECT_FILES)
	mkdir -p $(RELEASE_FOLDER)
	ar rvs $(RELEASE_FOLDER)/$(INFINITY_LIB_NAME).a $(OBJECT_FILES)
	rm -rf $(RELEASE_FOLDER)/$(INCLUDE_FOLDER)
	cp --parents $(INFINITY_HEADER_FILES) $(RELEASE_FOLDER)
	mv $(RELEASE_FOLDER)/$(INFINITY_SOURCE_FOLDER)/ $(RELEASE_FOLDER)/$(INCLUDE_FOLDER)

##################################################

clean:
	rm -rf $(BUILD_FOLDER)
	rm -rf $(RELEASE_FOLDER)

##################################################
server:
	mkdir -p $(RELEASE_FOLDER)/$(SERVER_FOLDER)
	$(CC) Persistence/Server.cpp $(CC_FLAGS) $(LD_FLAGS) -I $(SERVER_SOURCE_FOLDER)/KeyValueStore $(COMMON_SOURCE_FOLDER) $(RELEASE_FOLDER)/$(INCLUDE_FOLDER) -L $(RELEASE_FOLDER) -o $(RELEASE_FOLDER)/$(SERVER_FOLDER)/server

##################################################

client:
	mkdir -p $(RELEASE_FOLDER)/$(CLIENT_FOLDER)
	mkdir -p $(BUILD_FOLDER)/$(CLIENT_FOLDER)
	$(CC) $(CC_FLAGS) -c $(CLIENT_SOURCE_FOLDER)/Remote.cpp -I./$(CLIENT_SOURCE_FOLDER) -I./$(COMMON_SOURCE_FOLDER) -o $(BUILD_FOLDER)/$(CLIENT_FOLDER)/Remote.o
	$(CC) $(CC_FLAGS) -c $(CLIENT_SOURCE_FOLDER)/Client.cpp -I./$(CLIENT_SOURCE_FOLDER) -I./$(COMMON_SOURCE_FOLDER) -o $(BUILD_FOLDER)/$(CLIENT_FOLDER)/Client.o
	$(CC) $(BUILD_FOLDER)/$(CLIENT_FOLDER)/Remote.o $(BUILD_FOLDER)/$(CLIENT_FOLDER)/Client.o $(CC_FLAGS) $(LD_FLAGS) -I $(RELEASE_FOLDER)/$(INCLUDE_FOLDER) -L $(RELEASE_FOLDER) -o $(RELEASE_FOLDER)/$(CLIENT_FOLDER)/client

##################################################
