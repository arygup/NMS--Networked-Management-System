all: server client1 storage_srvr

server: server.c
	gcc -Wall server.c -o server -lpthread

client1: client1.c
	gcc -Wall client1.c -o client1 -lpthread

storage_srvr: storage_srvr.c
	gcc -Wall storage_srvr.c -o storage_srvr -lpthread

clean:
	rm -f server client1 storage_srvr
