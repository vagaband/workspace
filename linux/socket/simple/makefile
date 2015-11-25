main:server.o client.o
	gcc -g server.o -oserver -lpthread
	gcc -g client.o -oclient -lpthread
server.o:server.c clientmsg.h
	gcc -g -c server.c
client.o:client.c clientmsg.h
	gcc -g -c client.c
