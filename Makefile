SHELL := /bin/bash

# CROSS-COMPILER
CROSS_COMPILE = arm-linux-gnueabihf-

CC = $(CROSS_COMPILE)gcc
RM = rm -f
CFLAGS = -Wall -O3 -mtune=arm1176jzf-s -mcpu=cortex-a53
CLINKS = -lm -pthread
TARGET = timer

# SOURCE FILES
SRCS = timer.c helper.c
OBJS = $(SRCS:.c=.o)

# TARGETS
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(CLINKS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

purge: clean
	$(RM) $(TARGET)

