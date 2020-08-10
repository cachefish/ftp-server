#include"session.h"
#include"common.h"
#include"ftpproto.h"
#include"privparent.h"
#include"privsock.h"
#include"sysutil.h"

//父进程为nobody进程  子进程为服务进程
void begin_session(session_t *sess)
{
    activate_oobinline(sess->ctrl_fd);
    priv_sock_init(sess);
    /*int sockfds[2];     //父子进程进行通信
    if(socketpair(PF_UNIX,SOCK_STREAM,0,sockfds)<0){//创建一对无名的、相互连接的套接子
        ERR_EXIT("socketpair err");
    }*/
    pid_t pid;
    pid = fork();
    if(pid<0){
        ERR_EXIT("fork");
    }
    if(pid==0){
        
        /*close(sockfds[0]);
        //ftp服务进程   包括控制连接 和数据连接
        sess->child_fd = sockfds[1];
        */
        priv_sock_set_child_context(sess);
        handle_child(sess);
    }else{
      
        //nobody进程
        priv_sock_set_parent_context(sess);
        /*close(sockfds[1]);
        sess->parent_fd = sockfds[0];*/
        handle_parent(sess);
    }
}