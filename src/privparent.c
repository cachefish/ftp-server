#include"privparent.h"

//nobody进程  不断接受服务进程发送过来的命令 写出服务进程完成相应任务
void handle_parent(session_t *sess)
{
    char cmd;
    while(1){
        read(sess->parent_fd,&cmd,sizeof(cmd));
        
    }
}