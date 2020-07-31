#include"ftpproto.h"
#include"sysutil.h"


//从客户端一行一行接收数据，按行读取
void handle_child(session_t *sess)
{
    writen(sess->ctrl_fd,"220 (ftpd_server 0.1)\r\n",strlen("220 (ftpd_server 0.1)\r\n"));
    while(1){
        memset(sess->cmdline,0,sizeof(sess->cmdline));
        memset(sess->cmd,0,sizeof(sess->cmd));
        memset(sess->arg,0,sizeof(sess->arg));

        readline(sess->ctrl_fd,sess->cmdline,MAX_COMMAND_LINE);
        //得到一行数据后，开始解析ftp命令与参数，根据解析到的命令处理ftp命令
        //想nobody进程可以发送命令
    }
}