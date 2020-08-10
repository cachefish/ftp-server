#ifndef _SYS_UTIL_H_
#define _SYS_UTIL_H_
#include "common.h"   //整合头文件

int tcp_client(unsigned short port);
int tcp_server(const char *host, unsigned short port);

int getlocalip(char *ip);       //获取本地ip

void activate_nonblock(int fd);     //设置文件描述符为非阻塞
void deactivate_nonblock(int fd);   //设置为阻塞模式

int read_timeout(int fd, unsigned int wait_seconds);        //读超时函数
int write_timeout(int fd, unsigned int wait_seconds);        //写超时函数
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);        //接受连接超时函数
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);      //连接超时函数

//读写系列函数
ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
ssize_t recv_peek(int sockfd, void *buf, size_t len);
ssize_t readline(int sockfd, void *buf, size_t maxline);

//发送、接受文件描述符
void send_fd(int sock_fd, int fd);
int recv_fd(const int sock_fd);

const char* statbuf_get_perms(struct stat *sbuf);
const char* statbuf_get_date(struct stat *sbuf);

int lock_file_read(int fd);
int lock_file_write(int fd);
int unlock_file(int fd);

long get_time_sec(void);
long get_time_usec(void);
void nano_sleep(double seconds);

void activate_oobinline(int fd);        //开启紧急模式接收数据
void activate_sigurg(int fd);
#endif /* _SYS_UTIL_H_ */

