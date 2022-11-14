CC = gcc

server: server.c service.c
	$(CC) -Wall $^ -o $@

clean:
	rm server