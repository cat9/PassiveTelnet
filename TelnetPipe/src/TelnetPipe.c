/*
 ============================================================================
 Name        : TelnetService.c
 Author      : xr.lee
 Version     :
 Copyright   : toonan
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<sys/file.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include <net/if.h>

#include	<string.h>

#include	<stdio.h>
#include	<stdlib.h>
#include	<netdb.h>
#include	<fcntl.h>
#include	<time.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/wait.h>
#include	<sys/ipc.h>
#include	<sys/shm.h>
#include <sys/ioctl.h>

#include	<malloc.h>
#include	<getopt.h>
#include	<termios.h>	// local echo off/on;

#include "thread_pool.h"

typedef struct NetConf {
	char ip[20];
	int port;
} NetConf;

int getLocalIp(char *in_name, char *buf) {
	int socket_fd;
	struct ifreq ifr;

	if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}

	strcpy(ifr.ifr_name, in_name);
	if (ioctl(socket_fd, SIOCGIFADDR, &ifr) < 0) {
		return -1;
	}
	strcpy(buf, inet_ntoa(((struct sockaddr_in *) &(ifr.ifr_addr))->sin_addr));
	return 0;
}

//--------------------------------------------------------------------------------
// Set echo mode OFF/ON like stty proc;
//--------------------------------------------------------------------------------
static struct termios stored;
static int n_term_change = 0;

void echo_off(void) {
	struct termios new;
	tcgetattr(0, &stored);
	memcpy(&new, &stored, sizeof(struct termios));
	new.c_lflag &= (~ECHO);			// echo off ;
	new.c_lflag &= (~ICANON);		// set buffer to 1,
	new.c_cc[VTIME] = 0;				// no time-out ;
	new.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new);
	n_term_change = 1;
	return;
}

void echo_on(void) {
	if (n_term_change)
		tcsetattr(0, TCSANOW, &stored);		// restore terminal seeting ;
	n_term_change = 0;
	return;
}

int transfer(int fromfd, int tofd) {
	int readSize = -1;
	char buf[1024];
	while ((readSize = read(fromfd, buf, sizeof buf)) > 0) {
		if (write(tofd, buf, readSize) < 0) {
			return -1;
		}
	}
	printf("errno:%d\n", errno);
	if (readSize < 0 && errno != EAGAIN) {
		return -1;
	}

	if (readSize < 0 && errno == EAGAIN) {
		return 1;
	}

	return readSize;
}

int createSocketToServer(const char *dstIp, int dstPort) {
	struct sockaddr_in client_addr;
	bzero(&client_addr, sizeof(client_addr)); //把一段内存区的内容全部设置为0
	client_addr.sin_family = AF_INET;    //internet协议族
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);    //INADDR_ANY表示自动获取本机地址
	client_addr.sin_port = htons(0); //0表示让系统自动分配一个空闲端口 //创建用于internet的流协议(TCP)socket,用client_socket代表客户机
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0) {
		printf("Create Socket Failed!\n");
		return -1;
	} //把客户机的socket和客户机的socket地址结构联系起来
	int fdflags = fcntl(client_socket, F_GETFL, 0);
	if (fcntl(client_socket, F_SETFL, fdflags | O_NONBLOCK) < 0) {
		printf("set O_NONBLOCK Error!\n");
		close(client_socket);
		return -1;
	}

	if (bind(client_socket, (struct sockaddr*) &client_addr,
			sizeof(client_addr))) {
		printf("Client Bind Port Failed!\n");
		close(client_socket);
		return -1;
	}  //设置一个socket地址结构server_addr,代表服务器的internet地址, 端口
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	if (inet_aton(dstIp, &server_addr.sin_addr) == 0) //服务器的IP地址来自程序的参数
			{
		printf("Server IP Address Error!\n");
		close(client_socket);
		return -1;
	}
	server_addr.sin_port = htons(dstPort);
	printf("Connecting To %s,%d!\n", dstIp, dstPort);
	socklen_t server_addr_length = sizeof(server_addr); //向服务器发起连接,连接成功后client_socket代表了客户机和服务器的一个socket连接
	int ret = 0;
	if ((ret = connect(client_socket, (struct sockaddr*) &server_addr,
			server_addr_length)) < 0) {
		if (errno != EINPROGRESS) {
			printf("Can Not Connect To %s,%d!\n", dstIp, dstPort);
			return -1;
		}
	}
	if (ret == 0)
		goto done;

	struct timeval tval;
	tval.tv_sec = 10;
	tval.tv_usec = 0;

	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(client_socket, &rset);
	if ((ret = select(client_socket + 1, NULL, &rset, NULL, &tval)) < 0) {
		printf("Connect To %s,%d EINTR!\n", dstIp, dstPort);
		close(client_socket);
		return -1;
	}
	if (ret == 0) {
		printf("Connect To %s,%d timeout!\n", dstIp, dstPort);
		close(client_socket);
		return -1;
	}
	int error = 0;
	int len = sizeof(error);
	getsockopt(client_socket, SOL_SOCKET, SO_ERROR, (void *) &error, &len);
	if (error) {
		fprintf(stderr, "Error in connection() %d - %s/n", error,
				strerror(error));
		return -1;
	}

	done: return client_socket;
}

void closeSocket(int socketFd) {
	if (socketFd != -1) {
		close(socketFd);
	}
}

void *spawnNewConnect(void *arg) {
	NetConf *conf = (NetConf *) arg;
	char localip[20];
	memset(localip, 0, 20);
	getLocalIp("eth0", localip);
	if (strlen(localip) > 0 && strlen(conf->ip) > 0) {
		int socket_server = createSocketToServer("192.168.1.101", 23);
		int socket_client = createSocketToServer(conf->ip, conf->port);
		if (socket_server != -1 && socket_client != -1) {
			int n_select = 0;
			fd_set n_read_fds;
			int ret = -1;
			while (1) {
				FD_ZERO(&n_read_fds);
				FD_SET(socket_server, &n_read_fds);
				FD_SET(socket_client, &n_read_fds);
				n_select =
						socket_server > socket_client ?
								socket_server : socket_client;
				ret = select(n_select + 1, &n_read_fds, NULL, NULL, NULL);
				perror("SELECT END\n");
				if (ret < 0) {
					perror("Select() error. \n");
					break;
				}
				if (ret == 0) {
					perror("Select() time out. \n");
					break;
				}
				if (FD_ISSET(socket_server, &n_read_fds)) {
					perror("FD_ISSET(socket_server, &n_read_fds)");
					ret = transfer(socket_server, socket_client);
					if (ret <= 0) {
						printf(
								"transfer(socket_server,socket_client)%s0 err.\n",
								ret < 0 ? "<" : "==");
						break;
					}

				}
				if (FD_ISSET(socket_client, &n_read_fds)) {
					perror("FD_ISSET(socket_client, &n_read_fds)");
					ret = transfer(socket_client, socket_server);
					if (ret <= 0) {
						printf(
								"transfer(socket_client,socket_server)%s0 err.\n",
								ret < 0 ? "<" : "==");
						break;
					}
				}

			}

		}
		closeSocket(socket_server);
		closeSocket(socket_client);
		socket_server = -1;
		socket_client = -1;
	}
	free(conf);
}

int main(void) {
	echo_off();

	if (tpool_create(3) != 0) {
		printf("tpool_create failed\n");
		exit(1);
	}

	int sockfd;
	char ack;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	struct GuardDev *dev = NULL;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Socket Error\n");
		exit(1);
	}

	bzero(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(10109);

	if (bind(sockfd, (struct sockaddr *) (&server_addr),
			sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "Bind error\n");
		exit(1);
	}

	if (listen(sockfd, 1) == -1) {
		fprintf(stderr, "listen error\n");
		exit(1);
	}
	char receive[100];
	while (1) {
		int sin_size = sizeof(struct sockaddr_in);
		sleep(1);
		int client_fd = -1;
		if ((client_fd = accept(sockfd, (struct sockaddr *) (&client_addr),
				&sin_size)) == -1) {
			fprintf(stderr, "Accrpt error\n");
			exit(1);
		}
		printf("Server get connection from %s,clientfd=%d\n",
				(unsigned char *) inet_ntoa(client_addr.sin_addr), client_fd);
		memset(receive, 0, 100);
		int ret = recv(client_fd, receive, 64, 0);
		if (ret < 0) {
			printf("receive error: %s", strerror(errno));
			close(client_fd);
			continue;
		}
		if (strlen(receive) > 0) {
			char ip[20], port[6];
			memset(ip, 0, 20);
			memset(port, 0, 6);
			sscanf(receive, "ip=%[^&]&port=%[^&]", ip, port);
			NetConf *conf = (NetConf *) malloc(sizeof(NetConf));
			memset(conf, 0, sizeof(NetConf));
			strcpy(conf->ip, ip);
			conf->port = atoi(port);
			tpool_add_work(spawnNewConnect, (void*) conf);
		}
		close(client_fd);
	}

	tpool_destroy();
	echo_on();

	return EXIT_SUCCESS;
}

