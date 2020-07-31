#ifndef _SESSION_H_
#define _SESSION_H_
#include"common.h"
//服务端和客户端连接过程作为一个会话 会话中包括nobody进程和服务进程
//nobody进程    主要是进行权限管理
//服务进程    处理ftp服务器相关的细节

//定义一个session结构体  保存当前会话所需变量
typedef struct session
{
        //控制连接
        int ctrl_fd;//已连接套接字
        char cmdline[MAX_COMMAND_LINE];     //保存命令行
        char cmd[MAX_COMMAND];//解析命令行的命令
        char arg[MAX_ARG];  //参数
        //父子进程通信
        int parent_fd;
        int child_fd;
}session_t;


void begin_session(session_t*sess);   //启动会话

#endif