CXX = g++
LD = g++

OFLAGS = -c -I../Common
INCLUDE_DIR = -I../Common
LFLAGS = $(CFLAGS) -L/usr/lib/ -lpthread
CFLAGS += $(INCLUDE_DIR)

SOURCES = $(wildcard *.cpp KeyValueStore/*.cpp utils/*.cpp ../Common/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

DEBUG = no
PROFILE = no
PEDANTIC = no
OPTIMIZATION = -O3

ifeq ($(DEBUG), yes)
	CFLAGS += -g
	OPTIMIZATION = -O0
endif

ifeq ($(PROFILE), yes)
	CFLAGS += -pg
endif

CFLAGS += $(OPTIMIZATION)

all: test

test: $(OBJECTS)
	$(CXX) $(OBJECTS) $(CFLAGS) -lpthread -o test

%.o: %.cpp
	$(CXX) $(CFLAGS) -lpthread -c $< -o $@

clean:
	rm -rf *.o test

rebuild: clean all

.PHONY : clean
.SILENT : clean