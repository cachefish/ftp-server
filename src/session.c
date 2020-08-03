#include"session.h"
#include"common.h"
#include"ftpproto.h"
#include"privparent.h"

//父进程为nobody进程  子进程为服务进程
void begin_session(session_t *sess)
{
   

    int sockfds[2];     //父子进程进行通信
    if(socketpair(PF_UNIX,SOCK_STREAM,0,sockfds)<0){//创建一对无名的、相互连接的套接子
        ERR_EXIT("socketpair err");
    }
    pid_t pid;
    pid = fork();
    if(pid<0){
        ERR_EXIT("fork");
    }
    if(pid==0){
        close(sockfds[0]);
        //ftp服务进程   包括控制连接 和数据连接
        sess->child_fd = sockfds[1];
        handle_child(sess);
    }else{
         struct  passwd *pw =  getpwnam("nobody");     //设置父进程为nobody进程
        if(pw==NULL){
            return; //直接退出
        }
            //设置组id  用户id  没改之前都是0
        if(setgid(pw->pw_gid)<0){
            ERR_EXIT("setgid");
        }
        if(setuid(pw->pw_uid)<0){
            ERR_EXIT("setuid");
        }

        //nobody进程
        close(sockfds[1]);
        sess->parent_fd = sockfds[0];
        handle_parent(sess);
    }
}