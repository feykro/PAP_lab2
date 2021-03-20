#CC=gcc
CC=clang
CFLAGS = -O2 -fopenmp
CFLAGS += -I/usr/local/opt/libomp/include
LDFLAGS = -fopenmp
LDFLAGS += -L /usr/local/opt/libomp/lib
CPPFLAGS = -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk

EXEC = 	bubble.run	\
	mergesort.run	\
	odd-even.run	\
	quick-sort.run

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
