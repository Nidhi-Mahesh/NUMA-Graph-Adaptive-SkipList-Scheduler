CC	:= gcc
CFLAGS	:= -DINTEL -Wall -std=c99
LDFLAGS	:= -lpthread -lm

OS	:= $(shell uname -s)
    ifeq ($(OS),Linux)
	CFLAGS  += -DCACHE_LINE_SIZE=`getconf LEVEL1_DCACHE_LINESIZE`
        LDFLAGS += -lrt
    endif
    ifeq ($(OS),Darwin)
	CFLAGS += -DCACHE_LINE_SIZE=`sysctl -n hw.cachelinesize`
    endif

ifeq ($(DEBUG),true)
	CFLAGS+=-DDEBUG -O0 -ggdb3 #-fno-omit-frame-pointer -fsanitize=address
else
	CFLAGS+=-O3
endif


VPATH	:= gc
DEPS	+= Makefile $(wildcard *.h) $(wildcard gc/*.h)

TARGETS := perf_meas numa_perf_meas graph_perf_meas graph_numa_perf_meas unittests


all:	$(TARGETS)

clean:
	rm -f $(TARGETS) core *.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

perf_meas: CFLAGS+=-DNDEBUG
perf_meas: perf_meas.o ptst.o gc.o prioq.o common.o
	$(CC) -o $@ $^ $(LDFLAGS)

numa_perf_meas: CFLAGS+=-DNDEBUG
numa_perf_meas: numa_perf_meas.o numa_prioq.o ptst.o gc.o prioq.o common.o
	$(CC) -o $@ $^ $(LDFLAGS)

graph_perf_meas: CFLAGS+=-DNDEBUG
graph_perf_meas: graph_perf_meas.o graph_sched.o numa_prioq.o ptst.o gc.o prioq.o common.o
	$(CC) -o $@ $^ $(LDFLAGS)

graph_numa_perf_meas: CFLAGS+=-DNDEBUG
graph_numa_perf_meas: graph_numa_perf_meas.o graph_sched.o numa_prioq.o ptst.o gc.o prioq.o common.o
	$(CC) -o $@ $^ $(LDFLAGS)

unittests: unittests.o ptst.o gc.o prioq.o common.o
	$(CC) -o $@ $^ $(LDFLAGS)

test: unittests
	./unittests

.PHONY: all clean test
