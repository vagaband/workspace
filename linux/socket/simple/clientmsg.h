//clientmsg between server and client
#ifndef _clientmsg
#define _clientmsg

//user msg exit for op of clientmsg

#define EXIT -1
#define USER 1
#define MSG 2
#define OK 3

#ifndef CMSGLEN
#define CMSGLEN 100
#endif

struct CLIENTMSG{
	int op;
	char username[20];
	char buf[CMSGLEN];
};

#endif
