#CC = gcc
#CC=/usr/local/opt/llvm/bin/clang
CC=clang
CFLAGS = -O2 -fopenmp
CFLAGS += -I/usr/local/opt/libomp/include
LDFLAGS = -fopenmp
LDFLAGS += -L /usr/local/opt/libomp/lib

EXEC = 	bubble.run	\
	mergesort.run	\
	odd-even.run

HEADER_FILES = $(wildcard *.h)

RAND_INIT=0

ifeq ($(RAND_INIT), 1)
$(Initialization of the vector is random)
CONFIG_FLAGS += -DRINIT
endif

all: $(EXEC)

%.run: %.o utils.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c $(HEADER_FILES)
	$(CC) -c $(CONFIG_FLAGS) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXEC) *.o *~

.PHONY: clean
