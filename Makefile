CC = clang
CFLAGS = -Wall -Werror -Wextra -pedantic   
LDFLAGS = -pthread
OBJS = util.o queue.o list.o request.o auditlog.o  

all: httpserver 

httpserver: httpserver.o $(OBJS)
	$(CC) -o httpserver httpserver.o $(OBJS) $(LDFLAGS) 

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f httpserver httpserver.o $(OBJS)

format:
	clang-format -i -style=file *.c *.h

debug: CFLAGS += -g

debug: clean all
