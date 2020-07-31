#include"common.h"
#include"sysutil.h"
#include"session.h"

int main(void)
{
    if(getuid()!=0){
        fprintf(stderr,"ftp_server :must be started as root\n");
        exit(EXIT_FAILURE);
    }
    /*
    typedef struct session{
        //控制连接
        char cmdline[MAX_COMMAND_LINE];     //保存命令行
        char cmd[MAX_COMMAND];//解析命令行的命令
        char arg[MAX_ARG];  //参数
        //父子进程通信
        int parent_fd;
        int child_fd;
        
    }session_t;*/
    session_t sess = {-1,"","","",-1,-1};  //初始

    int listenfd = tcp_server(NULL,5188);
    int conn;
    pid_t pid;
    //接受客户端连接  使用多进程方式
    while(1){
            conn = accept_timeout(listenfd,NULL,0);
            if(conn==-1){
                ERR_EXIT("accept _timeout");
            }
            pid = fork();
            if(pid == -1){
                ERR_EXIT("fork");
            }
            if(pid==0){
                close(listenfd);
                sess.ctrl_fd = conn;
                begin_session(&sess);    //启动一个会话 
            }else{
                close(conn);
            }
    }
    return 0;
}