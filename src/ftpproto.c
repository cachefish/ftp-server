#include"ftpproto.h"
#include"sysutil.h"
#include"str.h"

//从客户端一行一行接收数据，按行读取
void handle_child(session_t *sess)
{
    writen(sess->ctrl_fd,"220 (ftpd_server 0.1)\r\n",strlen("220 (ftpd_server 0.1)\r\n"));
    int ret;
    while(1){
        memset(sess->cmdline,0,sizeof(sess->cmdline));
        memset(sess->cmd,0,sizeof(sess->cmd));
        memset(sess->arg,0,sizeof(sess->arg));

        ret = readline(sess->ctrl_fd,sess->cmdline,MAX_COMMAND_LINE); 
        if(ret==-1){
            ERR_EXIT("readline");
        }else if(ret ==0){
            exit(EXIT_SUCCESS);
        }
        //到这里就接收到了一行数据，开始解析
         //得到一行数据后，开始解析ftp命令与参数，根据解析到的命令处理ftp命令
        //向nobody进程可以发送命令
        
        printf("cmdline=[%s]\n",sess->cmdline);
        //去除\r\n
        str_trim_crlf(sess->cmdline);
        printf("cmdline=[%s]\n",sess->cmdline);
        //解析FTP参数----对命令行进行分割
        str_split(sess->cmdline,sess->cmd,sess->arg,' ');
        printf("cmd = [%s] arg=[%s]\n",sess->cmd,sess->arg);
        //将命令转换为大写，方便处理
        str_upper(sess->cmd);

        //处理ftp命令
        return 0;
    }
}