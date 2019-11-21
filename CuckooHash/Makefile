ROOT = ./

EXTRA_FLAGS = -lpthread -O3 -std=c++11
.PHONY:	all clean

all:	main

Parallelize.o: Parallelize.hpp Cuckoo.hpp
	g++ $(CFLAGS) $(EXTRA_PATHS) $(EXTRA_FLAGS) -c -o $(BUILDIR)./Parallelize.o Parallelize.cpp

test.o: test.cpp Parallelize.hpp Cuckoo.hpp
	g++ $(CFLAGS) $(EXTRA_PATHS) $(EXTRA_FLAGS) -c -o $(BUILDIR)./test.o test.cpp

main: Parallelize.o test.o
	g++ $(CFLAGS) $(EXTRA_PATHS) $(EXTRA_FLAGS) $(BUILDIR)./Parallelize.o $(BUILDIR)./test.o -o run $(LDFLAGS)

clean:
	-rm -f $(BINS)
