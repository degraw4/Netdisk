SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:=server
CC:=gcc
CFLAGS:=-g -Wall -I /usr/include/mysql/
$(ELF):$(OBJS)
	gcc -o $@ $^ -pthread -lmysqlclient 
clean:
	rm -rf $(OBJS) $(ELF)
