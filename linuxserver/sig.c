#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<pthread.h>

#define MAX_EVENT_NUMBER 1024
static int pipefd[2]

int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL)
	int new_option = old_option | O_NONBLOCK
	fcntl(fd, F_SETTL, new_option)
}

void addfd(int epollfd, int fd){
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN|EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblock(fd);
}

/*signal handler*/

void sig_handler(int sig){
	/*save history errno, and restore it at the end of handler*/
	int save_errno = errno;
	int msg = sig;
	send(pipefd[1], (char*)&msg, 1, 0);
	errno = save_errno;
}

/*set signal handler*/
void addsig(int sig){
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = sig_handler;
	sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);	
	assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char* argv[]){
	if(argc <= 2){
		printf("usage: %s ip_address por_number\n", basename(argv[0]));
		return 1;
	}
	const char *ip = argv[1];
	int port = atoi(argv[2]);
	
	int ret = 0;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);
	
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);
	
	ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	if(ret == -1){
		printf("errno is %d\n", errno);
	}

	ret = listen(listenfd, 5);
	assert(ret != -1);
	
	epoll_event events[MAX_EVENT_NUMBER];
	int epollfd = epoll_create(5);
	assert(epollfd != -1);
	addfd(epollfd, listenfd);
	/*use socketpair to create a pipe and register a read event in pipefd[0]*/
	ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
	assert(ret != -1);
	setnonblocking(pipefd[1]);
	addfd(epollfd, pipefd[0]);
	
	/*set some signal handlers*/
	addsig(SIGHUP);
	addsig(SIGCHLD);
	addsig(SIGTERM);
	addsig(SIGINT);
	bool stop_server = false;
	
	while( !stop_server ){
		int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if((number < 0) && (errno != EINTR)){
			printf("epoll failure\n");
			break;
		}
		
		for(int i = 0; i < number; i++){
			int sockfd = events[i].data.fd;
			/*if listenfd in ready, handle the new connection*/
			if(sockfd == listenfd){
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
				addfd(epollfd, connfd);
			}
			/*if pipefd[0] in ready, handle the sinal*/
			else if((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)){
				int sig;
				char signals[1024];
				ret = recv(pipefd[0], signals, sizeof(signals), 0);
				if(ret == -1){
					continue;
				}else{
					/*every signal occupies one byte, so we accept the signal one by one*/
					for(int i = 0; i < ret; ++i){
						switch(signals[i]){
							case SIGCHLD:
							case SIGHUP:{
								continue;
							}
							case SIGTERM:
							case SIGINT:{
								stop_server = true;
							}
						}
					}
				}
			}
			else{}
		}
	}
	printf("close fds\n");
	close(listenfd);
	close(pipefd[1]);
	close(pipefd[0]);
	return 0;
}
