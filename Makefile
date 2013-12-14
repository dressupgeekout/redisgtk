# RedisGTK Makefile
# Christian Koch <cfkoch@sdf.lonestar.org>

CC = clang -Wall
OUTBIN = redisgtk

CFLAGS = `pkg-config --cflags gtk+-2.0 vte`
LIBS = `pkg-config --libs gtk+-2.0 vte`

REDIS_SERVER_LOC = `which redis-server`

.PHONY: default clean 
.SUFFIXES: .c .o

default: $(OUTBIN)

$(OUTBIN):redisgtk.o
	$(CC) -o $(.TARGET) $(.ALLSRC) $(LIBS)

.c.o:
	$(CC) -c -o $(.TARGET) $(CFLAGS) \
		-DREDIS_SERVER_LOC=\"$(REDIS_SERVER_LOC)\" \
		$(.ALLSRC)

clean:
	rm -f $(OUTBIN) *.o *.core
