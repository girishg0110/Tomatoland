OUTPUT = client server
CFLAGS = -g -Wall -Wvla -I inc -D_REENTRANT
LSFLAGS = -L lib -pthread
LCFLAGS = -L lib -lSDL2 -lSDL2_image -lSDL2_ttf -pthread
OBJS = csapp.o 
PORT = 3000

%.o: %.c %.h
	gcc $(CFLAGS) -c $<

%.o: %.c
	gcc $(CFLAGS) -c $<

all: $(OUTPUT)

runclient: client; LD_LIBRARY_PATH=lib ./client localhost $(PORT)

client: client.o $(OBJS)
	gcc $(CFLAGS) -o $@ $^ $(LCFLAGS)

server: server.o $(OBJS); gcc $(CFLAGS) -o $@ $^ $(LSFLAGS)

clean:
	rm -f $(OUTPUT) *.o
