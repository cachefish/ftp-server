#include"common.h"
#include"sysutil.h"
#include"session.h"
#include"parseconf.h"
#include"tunable.h"
#include"ftpproto.h"
int main(void)
{
   //list_common(sess);

/*
int tunable_pasv_enable = 1;
int tunable_port_enable = 1;

unsigned int tunable_listen_port = 21;
unsigned int tunable_max_clients = 2000;
unsigned int tunable_max_per_ip = 50;

unsigned int tunable_accept_timeout = 60;
unsigned int tunable_connect_timeout = 60;
unsigned int tunable_idle_session_timeout = 300;
unsigned int tunable_data_connection_timeout = 300;
unsigned int tunable_local_umask = 077;
unsigned int tunable_upload_max_rate = 0;
unsigned int tunable_download_max_rate = 0;
const char *tunable_listen_address;
*/

    //测试配置
    parseconf_load_file(FTPSERVER_CONF);
    printf("tunable_pasv_enable = %d\n",tunable_pasv_enable);
    printf("tunable_port_enable=%d\n",tunable_port_enable);

    printf("tunable_listen_port=%u\n",tunable_listen_port);
    printf("tunable_max_clients=%u\n",tunable_max_clients);
    printf("tunable_max_per_ip=%u\n",tunable_max_per_ip);
    printf("tunable_accept_timeout=%u\n",tunable_accept_timeout);
    printf("tunable_connect_timeout=%u\n",tunable_connect_timeout);
    printf("tunable_idle_session_timeout=%u\n",tunable_idle_session_timeout);
    printf("tunable_data_connection_timeout=%u\n",tunable_data_connection_timeout);
    printf("tunable_local_umask=0%o\n",tunable_local_umask);
    printf("tunable_upload_max_rate=%u\n",tunable_upload_max_rate);
    printf("tunable_download_max_rate=%u\n",tunable_download_max_rate);
   if (tunable_listen_address == NULL)
		printf("tunable_listen_address=NULL\n");
	else
		printf("tunable_listen_address=%s\n", tunable_listen_address);


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
    session_t sess = {
                                        /*控制连接*/
                                        0,-1,"","","",
                                        /*数据连接*/
                                        NULL,-1,
                                        /*父子进程通道*/
                                        -1,-1,
                                        /*FTP协议状态*/
                                        0
                                        };  //初始

    int listenfd = tcp_server(tunable_listen_address,tunable_listen_port);
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