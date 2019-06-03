all: server client

server: server.c util.c server_main.c server.h util.h
	gcc -pthread server.c util.c server_main.c -o server

client: client.c util.c util.h
	gcc -pthread client.c util.c -o client

.PHONY: clean

clean:
	rm server client
