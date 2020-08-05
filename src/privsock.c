#include"privsock.h"
#include"sysutil.h"

void priv_sock_init(session_t *sess)       //初始化内部进程间通信通道
{
    int sockfds[2];     //父子进程进行通信
    if(socketpair(PF_UNIX,SOCK_STREAM,0,sockfds)<0){//创建一对无名的、相互连接的套接子
        ERR_EXIT("socketpair err");
    }
    sess->parent_fd=sockfds[0];
    sess->child_fd = sockfds[1];
}
void priv_sock_close(session_t *sess)  //关闭内部进程间通信通道
{
          if(sess->parent_fd!=-1){
            close(sess->parent_fd);
            sess->parent_fd=-1;
            }
          if(sess->child_fd!=-1){
            close(sess->child_fd);
            sess->child_fd=-1;
            }
}
void priv_sock_set_parent_context(session_t *sess) //设置父进程环境
{
        if(sess->child_fd!=-1){
            close(sess->child_fd);
            sess->child_fd=-1;
        }
}
void priv_sock_set_child_context(session_t *sess)  //设置子进程环境
{
        if(sess->parent_fd!=-1){
            close(sess->parent_fd);
            sess->parent_fd=-1;
        }
}

void priv_sock_send_cmd(int fd, char cmd) //发送命令  子-->父
{
    int ret;
    ret = writen(fd,&cmd,sizeof(cmd));
    if(ret!=sizeof(cmd)){
        fprintf(stderr,"priv_sock_send_cmd err\n");
        exit(EXIT_FAILURE);
    }
    
}
char priv_sock_get_cmd(int fd)     //接收命令  父<-子
{
    int ret;
    char res;
    ret = readn(fd,&res,sizeof(res));       //接收一个字节
    if(ret!=sizeof(res)){
        fprintf(stderr,"priv_sock_get_cmd err\n");
        exit(EXIT_FAILURE);
    }
    return res;
}
void priv_sock_send_result(int fd, char res)   //发送结果  父->子
{
        int ret;
        ret = writen(fd,&res,sizeof(res));
        if(ret!=sizeof(res)){
        fprintf(stderr,"priv_sock_send_cmd err\n");
        exit(EXIT_FAILURE);
    }
}
char priv_sock_get_result(int fd)     //接收结果 子<-父
{
    int ret;
    char res;
    ret = readn(fd,&res,sizeof(res));       //接收一个字节
    if(ret!=sizeof(res)){
        fprintf(stderr,"priv_sock_get_cmd err\n");
        exit(EXIT_FAILURE);
    }
    return res;
}


void priv_sock_send_int(int fd, int the_int)  //发送一个整数
{
    int ret;
    ret = writen(fd,&the_int,sizeof(the_int));
        if(ret!=sizeof(the_int)){
        fprintf(stderr,"priv_sock_send_cmd err\n");
        exit(EXIT_FAILURE);
    }
}
int priv_sock_get_int(int fd)      //接收一个整数
{
    int ret;
    int res;
    ret = readn(fd,&res,sizeof(res));       //接收一个字节
    if(ret!=sizeof(res)){
        fprintf(stderr,"priv_sock_get_cmd err\n");
        exit(EXIT_FAILURE);
    }
    return res;
}
void priv_sock_send_buf(int fd, const char *buf, unsigned int len) //发送一个字符串
{
    priv_sock_send_int(fd,(int)len);
    int ret = writen(fd,buf,len);
    if(ret != (int)len){
        fprintf(stderr,"priv_sock_send_buf err\n");
        exit(EXIT_FAILURE);
    }
}
void priv_sock_recv_buf(int fd, char *buf, unsigned int len)  //接收一个字符串
{
    unsigned int recv_len =  (unsigned int)priv_sock_get_int(fd);
    if(recv_len>len){
         fprintf(stderr,"priv_sock_recv_buf err\n");
        exit(EXIT_FAILURE);
    }
    int ret = readn(fd,buf,recv_len);
    if(ret!=(int)recv_len){
         fprintf(stderr,"priv_sock_recv_buf err\n");
        exit(EXIT_FAILURE);
    }
}
void priv_sock_send_fd(int sock_fd, int fd)  //发送文件描述符
{
    send_fd(sock_fd,fd);
}
int priv_sock_recv_fd(int sock_fd)//接收文件描述符
{
    return recv_fd(sock_fd);
}