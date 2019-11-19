ROOT = ./

BINS = $(BINDIR)./simul
EXTRA_FLAGS = -lpthread -O3 -std=c++11
.PHONY:	all clean

all:	main

Cuckoo.o: Cuckoo.hpp
	g++ $(CFLAGS) $(EXTRA_PATHS) $(EXTRA_FLAGS) -c -o $(BUILDIR)./Cuckoo.o Cuckoo.cpp

Parallelize.o: Parallelize.hpp Cuckoo.hpp
	g++ $(CFLAGS) $(EXTRA_PATHS) $(EXTRA_FLAGS) -c -o $(BUILDIR)./Parallelize.o Parallelize.cpp

test.o: test.cpp Parallelize.hpp Cuckoo.hpp
	g++ $(CFLAGS) $(EXTRA_PATHS) $(EXTRA_FLAGS) -c -o $(BUILDIR)./test.o test.cpp

main: Cuckoo.o Parallelize.o test.o 
	g++ $(CFLAGS) $(EXTRA_PATHS) $(EXTRA_FLAGS) $(BUILDIR)./Cuckoo.o $(BUILDIR)./Parallelize.o $(BUILDIR)./test.o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
