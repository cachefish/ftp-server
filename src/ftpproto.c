#include"ftpproto.h"
#include"sysutil.h"
#include"str.h"
#include"ftpcodes.h"
#include"tunable.h"
#include"privsock.h"
void ftp_reply(session_t *sess,int status,const char*text);
void ftp_lreply(session_t *sess,int status,const char*text);

int list_common(session_t*sess,int detail);

void upload_common(session_t* sess,int is_append);

void limit_rate(session_t *sess,int bytes_transfered,int is_upload);

void start_data_alarm(void);
void start_cmdio_alarm();



int get_transfer_fd(session_t* sess);
int get_pasv_fd(session_t* sess);
int port_active(session_t* sess);
int pasv_active(session_t* sess);
int get_port_fd(session_t *sess);
static void do_user(session_t *sess);
static void do_pass(session_t*sess);
static void do_cwd(session_t *sess);
static void do_cdup(session_t *sess);
static void do_quit(session_t *sess);
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_type(session_t *sess);
//static void do_stru(session_t *sess);
//static void do_mode(session_t *sess);
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);
static void do_appe(session_t *sess);
static void do_list(session_t *sess);
static void do_nlst(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);
static void do_mkd(session_t *sess);
static void do_rmd(session_t *sess);
static void do_dele(session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_site(session_t *sess);
static void do_syst(session_t *sess);
static void do_feat(session_t *sess);
static void do_size(session_t *sess);
static void do_stat(session_t *sess);
static void do_noop(session_t *sess);
static void do_help(session_t *sess);

typedef struct ftpcmd {
	const char *cmd;
	void (*cmd_handler)(session_t *sess);
} ftpcmd_t;


static ftpcmd_t ctrl_cmds[] = {
	/* 访问控制命令 */
	{"USER",	do_user	},
	{"PASS",	do_pass	},
	{"CWD",		do_cwd	},
	{"XCWD",	do_cwd	},
	{"CDUP",	do_cdup	},
	{"XCUP",	do_cdup	},
	{"QUIT",	do_quit	},
	{"ACCT",	NULL	},
	{"SMNT",	NULL	},
	{"REIN",	NULL	},
	/* 传输参数命令 */
	{"PORT",	do_port	},
	{"PASV",	do_pasv	},
	{"TYPE",	do_type	},
	{"STRU",	/*do_stru*/NULL	},
	{"MODE",	/*do_mode*/NULL	},

	/* 服务命令 */
	{"RETR",	do_retr	},
	{"STOR",	do_stor	},
	{"APPE",	do_appe	},
	{"LIST",	do_list	},
	{"NLST",	do_nlst	},
	{"REST",	do_rest	},
	{"ABOR",	do_abor	},
	{"\377\364\377\362ABOR", do_abor},
	{"PWD",		do_pwd	},
	{"XPWD",	do_pwd	},
	{"MKD",		do_mkd	},
	{"XMKD",	do_mkd	},
	{"RMD",		do_rmd	},
	{"XRMD",	do_rmd	},
	{"DELE",	do_dele	},
	{"RNFR",	do_rnfr	},
	{"RNTO",	do_rnto	},
	{"SITE",	do_site	},
	{"SYST",	do_syst	},
	{"FEAT",	do_feat },
	{"SIZE",	do_size	},
	{"STAT",	do_stat	},
	{"NOOP",	do_noop	},
	{"HELP",	do_help	},
	{"STOU",	NULL	},
	{"ALLO",	NULL	}
};

session_t*p_sess;
void handle_alarm_timeout(int sig)
{
    shutdown(p_sess->ctrl_fd,SHUT_RD);
    ftp_reply(p_sess,FTP_IDLE_TIMEOUT,"timeout");
    shutdown(p_sess->ctrl_fd,SHUT_WR);
    exit(EXIT_FAILURE);
}
void handle_sigalrm(int sig)
{
    if(!p_sess->data_process){
        ftp_reply(p_sess,FTP_IDLE_TIMEOUT,"Data Timeout. Reconnect");
        exit(EXIT_FAILURE);
    }
    //否则，当前处于数据传输的状态收到了超时信号
    p_sess->data_process = 0;
    start_data_alarm();
}
//启动闹钟
void start_cmdio_alarm()
{
    if(tunable_idle_session_timeout>0){
        //设置信号
        signal(SIGALRM,handle_alarm_timeout);
        alarm(tunable_idle_session_timeout);
    }
}
 
void start_data_alarm(void)
{
    if(tunable_data_connection_timeout>0){
         //设置信号
        signal(SIGALRM,handle_sigalrm);
        alarm(tunable_idle_session_timeout);
    }else if(tunable_idle_session_timeout>0){
        //关闭先前安装的闹钟
        alarm(0);
    }
}


//从客户端一行一行接收数据，按行读取
void handle_child(session_t *sess)
{
    //writen(sess->ctrl_fd,"220 (ftpd_server 0.1)\r\n",strlen("220 (ftpd_server 0.1)\r\n"));
    ftp_reply(sess,FTP_GREET,"(ftpd_server 0.1)");
    int ret;
    while(1){
        memset(sess->cmdline,0,sizeof(sess->cmdline));
        memset(sess->cmd,0,sizeof(sess->cmd));
        memset(sess->arg,0,sizeof(sess->arg));

        //启动闹钟
        start_cmdio_alarm();

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
    
        /*
        //处理ftp命令
        if(strcmp("USER",sess->cmd)==0){
            do_user(sess);
        }else if(strcmp("PASS",sess->cmd)==0){
            do_pass(sess);
        }
        */
       int i;
       int size = sizeof(ctrl_cmds)/sizeof(ctrl_cmds[0]);    //数组长度
            for(i=0;i<size;i++){
            if(strcmp(ctrl_cmds[i].cmd,sess->cmd)==0){
                if(ctrl_cmds[i].cmd_handler!=NULL){
                    ctrl_cmds[i].cmd_handler(sess);
                }else{
                    ftp_reply(sess,FTP_COMMANDNOTIMPL,"Unimplement command.");
                }
                break;
            }

            if(i==size){
                ftp_reply(sess,FTP_BADCMD,"Unknown command");
            }
        }
        return ;
    }
}
//专门用来处理响应信息
void ftp_reply(session_t *sess,int status,const char*text)
{
    char buf[1024] = {0};
    sprintf(buf,"%d %s \r\n",status,text);
    writen(sess->ctrl_fd,buf,strlen(buf));
}
void ftp_lreply(session_t *sess,int status,const char*text)
{
    char buf[1024] = {0};
    sprintf(buf,"%d-%s \r\n",status,text);
    writen(sess->ctrl_fd,buf,strlen(buf));
}

int list_common(session_t*sess,int detail)
{
   DIR *dir =  opendir(".");
   if(dir==NULL){
       return 0;
   }
   //遍历
   struct dirent* dt;
   struct stat sbuf;
   while((dt =  readdir(dir))!=NULL)    //当前目录
   {
        if(lstat(dt->d_name,&sbuf)<0)//获取文件状态，保存到第二个参数中
        {
            continue;
        }   
        char buf[1024] = {0};
        if(detail)
        {
                        const char *perms = statbuf_get_perms(&sbuf);
                
                //硬连接数   st_nlink 
                
                int off = 0;
                off += sprintf(buf,"%s",perms);
                off += sprintf(buf+off,"%3ld %-8d %-8d ",sbuf.st_nlink,sbuf.st_uid,sbuf.st_gid);
                off += sprintf(buf+off,"%8lu  ",sbuf.st_size);

                
                const char *databuf = statbuf_get_date(&sbuf);
                off += sprintf(buf+off,"%s",databuf);

                
                //文件名  要判断是否是链接文件
                
                if(S_ISLNK(sbuf.st_mode)){
                    char tmp[1024] = {0};
                    readlink(dt->d_name,tmp,sizeof(tmp));
                    off += sprintf(buf+off,"%s -> %s\r\n",dt->d_name,tmp);
            }else{
                    off += sprintf(buf+off,"%s\r\n",dt->d_name);
            }
        }else{
            sprintf(buf,"%s\r\n",dt->d_name);
        }  
        //过滤.xxx文件
        if(dt->d_name[0]=='.'){
            continue;
        }
        writen(sess->data_fd,buf,sizeof(buf));
        //printf("%s",buf);
   }
   closedir(dir);
   return 1;
}

 void limit_rate(session_t *sess,int bytes_transfered,int is_upload)
 {
     sess->data_process = 1;
    //设置限速   睡眠时间 =(当前传输速度/最大传输速度-1)*当前传输时间
    long curr_sec = get_time_sec();
    long curr_usec = get_time_usec();

    double elapsed;
    elapsed = (double)curr_sec-(sess->bw_transfer_start_sec);
    elapsed+=(double)(curr_usec-sess->bw_transfer_start_usec)/(double)1000000;
    if(elapsed<=(double)0){
        elapsed = (double)0.01;
    }

    //计算当前传输速度
    unsigned int bw_rate = (unsigned int)((double)bytes_transfered/elapsed);

    double rate_ratio;
    if(is_upload){
        if(bw_rate<sess->bw_upload_rate_max){
            //不需要限速
            sess->bw_transfer_start_sec = curr_sec;
            sess->bw_transfer_start_usec = curr_usec;
            return;
        }
        rate_ratio = bw_rate/sess->bw_upload_rate_max;
    }else{
         if(bw_rate<sess->bw_download_rate_max){
            //不需要限速
             sess->bw_transfer_start_sec = curr_sec;
            sess->bw_transfer_start_usec = curr_usec;
            return;
        }
        rate_ratio = bw_rate/sess->bw_download_rate_max;
    }
    //开始计算
    double pause_time;
    pause_time = (rate_ratio-(double)1)*elapsed;        //睡眠时间

    nano_sleep(pause_time);     //睡眠函数

    sess->bw_transfer_start_sec = get_time_sec();
    sess->bw_transfer_start_usec=get_time_usec();

}
void upload_common(session_t* sess,int is_append)  //上传
{
    //创建数据连接
    if(get_transfer_fd(sess)==0)
    {
        return;
    }

    long long offset = sess->restart_pos;
    sess->restart_pos = 0;


    //打开文件
    int fd = open(sess->arg,O_CREAT|O_WRONLY,0666);
    if(fd == -1){
        ftp_reply(sess,FTP_UPLOADFAIL,"Could not create File.");
        return;
    }
    //给文件加写锁
    int ret;
    ret = lock_file_write(fd);
    if(ret==-1)
    {
          ftp_reply(sess,FTP_FILEFAIL,"Could not create File.");
        return;
    }

    //STOR
    //REST + STOR
    //APPE
 if (!is_append && offset == 0) {   // STOR
		ftruncate(fd, 0);
		if (lseek(fd, 0, SEEK_SET) < 0) {
			ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
			return;
		}
	}
	else if (!is_append && offset != 0) { // REST+STOR
		if (lseek(fd, offset, SEEK_SET) < 0) {
			ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
			return;
		}
	}
	else if (is_append) {   // APPE  //追加，将文件指针偏移到文件末尾
		if (lseek(fd, 0, SEEK_END) < 0) {
			ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
			return;
		}
	}
    if(offset!=0)   //说明有断点
    {
        ret = lseek(fd,offset,SEEK_SET);
        if(ret == -1){
           ftp_reply(sess,FTP_FILEFAIL,"Failed to open File.");
        return;
        }
    }


	struct stat sbuf;
	ret = fstat(fd, &sbuf);
	if (!S_ISREG(sbuf.st_mode)) {
		ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
		return;
	}
    //150   模式判定  二进制
    char text[2048] ={0};
    if(sess->is_ascii)
    {
        sprintf(text,"Opening ASCII mode data connection for %s (%lld bytes).",
            sess->arg,(long long)sbuf.st_size);
    }else{
        sprintf(text,"Opening BINARY mode data connection for %s (%lld bytes).",
            sess->arg,(long long)sbuf.st_size);
    }    
    ftp_reply(sess,FTP_DATACONN,text);

    //上传文件
    int flag=0;
    char buf[65536];

    //设置限速   睡眠时间 =(当前传输速度/最大传输速度-1)*当前传输时间
    sess->bw_upload_rate_max = get_time_sec();
    sess->bw_download_rate_max=get_time_usec();

   while(1)
    {
        ret = read(sess->data_fd,buf,sizeof(buf));   //会涉及到系统调用
        if(ret == -1){
            if(errno == EINTR)
            {
                continue;
            }else{
                flag =2;
                break;
            }        
        }else if(ret ==0){
            flag = 0;
            break;
        }
        //读取到一定数据，判断下是否需要限速
        limit_rate(sess,ret,1);

        if(writen(sess->data_fd,buf,ret))
        {
            flag = 1;
            break;
        }
    }


    //关闭数据套接字
    list_common(sess,1);
    close(sess->data_fd);
    sess->data_fd = -1;
    close(fd);
    if(flag==0){
         //226
        ftp_reply(sess,FTP_TRANSFEROK,"Transfer complete.");
    }else if(flag == 1){    //读取数据失败  426
        ftp_reply(sess,FTP_BADSENDFILE,"Failure writting from local file.");
    }else if(flag ==2){ //写入失败  451
        ftp_reply(sess,FTP_BADSENDNET,"Failure reading from local file.");
    }
    //重新开启控制连接通道闹钟
   start_cmdio_alarm();
}


int port_active(session_t* sess)
{
    if(sess->port_addr!=NULL){
        if(pasv_active(sess))//不能主动 被动同时产生
        {
            fprintf(stderr,"botn port pasv are active");
            exit(EXIT_FAILURE);
        }
        return 1;
    }
    //
    return 0;
}
int pasv_active(session_t* sess)
{
    /*
    if(sess->port_listenfd !=-1){
        if(port_active(sess))
        {
                fprintf(stderr,"botn port an pasv are active ");
                exit(EXIT_FAILURE);
        }
        return 1;
    }*/
    priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PASV_ACTIVE);  //向nobody发送请求
    int  active = priv_sock_get_int(sess->child_fd);
    if(active){
          if(port_active(sess))
        {
                fprintf(stderr,"botn port an pasv are active ");
                exit(EXIT_FAILURE);
        }
        return 1;
    }
    return 0;
}

int get_port_fd(session_t *sess)
{
        //nobody进程协助完成数据连接通道的创建
    priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_GET_DATA_SOCK);
    unsigned short port = ntohs(sess->port_addr->sin_port);
    char *ip = inet_ntoa(sess->port_addr->sin_addr);

    priv_sock_send_int(sess->child_fd,(int)port);

    priv_sock_send_buf(sess->child_fd,ip,strlen(ip));
    char res = priv_sock_get_result(sess->child_fd);
    if(res == PRIV_SOCK_RESULT_BAD){
        return 0;
    }else if(res==PRIV_SOCK_RESULT_OK){     //成功应答
        sess->data_fd =  priv_sock_recv_fd(sess->child_fd);
    }
    return 1;
}
int get_pasv_fd(session_t* sess)
{
    priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PASV_ACCEPT);
    char res =  priv_sock_get_result(sess->child_fd);
    if(res == PRIV_SOCK_RESULT_BAD){
        return 0;
    }else if(res == PRIV_SOCK_RESULT_OK){
        //接收缩回传回来的数据套接字
        sess->data_fd = priv_sock_recv_fd(sess->child_fd);
    }
    return 1;
}
int get_transfer_fd(session_t* sess)
{
    //判断是否收到PORT或者PASV命令
    if(!port_active(sess)&&!pasv_active(sess)){
        ftp_reply(sess,FTP_BADSENDCONN,"Use PORT or PASV first.");
            return 0;
    }
    int ret =1;
    //主动模式
    if(port_active(sess))
    {
        //创建套接字，绑定20，connect对方
        /*int fd =  tcp_client(0);
        if(connect_timeout(fd,sess->port_addr,tunable_data_connection_timeout))
        {
                close(fd);
                return 0;
        } 
        sess->data_fd = fd;    
        */ 
        if(get_port_fd(sess)==0){
            ret = 0;
        }
    } 
    if(pasv_active(sess)){  //如果是被动模式
        /*
        int fd = accept_timeout(sess->pasv_listen_fd,NULL,tunable_accept_timeout);
        close(sess->pasv_listen_fd);
        if(fd==-1)
        {          
            return 0;
        }
        sess->data_fd = fd;
        */
       if(get_pasv_fd(sess)==0)     //获取被动套接字
       {
           ret = 0;
       }
    }
     if(sess->port_addr){
        free(sess->port_addr);
        sess->port_addr=NULL;
    }
    if(ret){
        //重新安装时钟信号，并启动闹钟
    start_data_alarm();
    }
    return ret;
}


static void do_user(session_t *sess)
{
    //USER AAA
    struct passwd *pw= getpwnam(sess->arg);
    if(pw==NULL){
        //用户不存在
        //530响应
         ftp_reply(sess,FTP_LOGINERR,"Login incorrect.");
        return;
    }
    //存在，保存pw_uid，用与比较
    sess->uid = pw->pw_uid;
    //331 响应
    ftp_reply(sess,FTP_GIVEPWORD,"Please specify the password.");
    

}

static void do_pass(session_t*sess)
{
    //PASS 213122

    struct passwd *pw= getpwuid(sess->uid); //通过用户的uid查找用户的passwd数据
    if(pw==NULL){
        //用户不存在
        //530响应
         ftp_reply(sess,FTP_LOGINERR,"Login incorrect.");
        return;
    }
    struct spwd *sp = getspnam(pw->pw_name);     //读取/etc/shadow中数据。存放到 sp中。
    if(sp==NULL){   //未找到
         ftp_reply(sess,FTP_LOGINERR,"Login incorrect.");
        return;
    }
    //找到，获取密码
    //将明文密码加密后与密码比较
    char*encrypted_pass =  crypt(sess->arg,sp->sp_pwdp);     //得到加密
    //验证密码
    if(strcmp(encrypted_pass,sp->sp_pwdp)!=0){
          ftp_reply(sess,FTP_LOGINERR,"Login incorrect.");
        return;
    }
    umask(tunable_local_umask);
    setegid(pw->pw_gid);
    seteuid(pw->pw_uid);
    chdir(pw->pw_dir);      //切换到用户目录

    //230响应
    ftp_reply(sess,FTP_LOGINOK,"Login successful.");
 
}

static void do_cwd(session_t *sess)  //改变当前路径
{
    if(chdir(sess->arg)<0)
    {
        ftp_reply(sess,FTP_FILEFAIL,"Failed to change firectory.");
        return;
    }
    ftp_reply(sess,FTP_CWDOK,"Directory sucessfully changed.");
}
static void do_cdup(session_t *sess)
{
    if(chdir("..")<0){
         ftp_reply(sess,FTP_FILEFAIL,"Failed to change firectory.");
        return;
    }
    ftp_reply(sess,FTP_CWDOK,"Directory sucessfully changed.");
}
static void do_quit(session_t *sess)
{

}

//port命令
static void do_port(session_t *sess)
{
    //port  192,168,1,108,123,233  六个整数
    unsigned int v[6];
    sscanf(sess->arg,"%u,%u,%u,%u,%u,%u",&v[2],&v[3],&v[4],&v[5],&v[0],&v[1]);
    sess->port_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    memset(sess->port_addr,0,sizeof(struct sockaddr_in));
    sess->port_addr->sin_family = AF_INET;
    unsigned char *p=(unsigned char*)&sess->port_addr->sin_port;
    p[0] = v[0];
    p[1] = v[1];
    p=(unsigned char*)&sess->port_addr->sin_addr;
    p[0]=v[2];
    p[0]=v[3];
    p[0]=v[4];
    p[0]=v[5];

    ftp_reply(sess,FTP_PORTOK,"PORT command sucessful.Consider using PASV");
    
}
static void do_pasv(session_t *sess)
{
    //Entering Passive Mode (192,168,244,100,101,46)
    char ip[16] = {0};
    getlocalip(ip);
    
    /*
    sess->pasv_listen_fd = tcp_server(ip,0);
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if(getsockname(sess->pasv_listen_fd,(struct sockaddr*)&addr,&addrlen)<0)
    {
        ERR_EXIT("getsockname");
    }
       unsigned port = ntohs(addr.sin_port);
    */
    priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PASV_LISTEN);
    unsigned short port =  (unsigned short)priv_sock_get_int(sess->child_fd);

 
    unsigned int v[4];
    sscanf(ip,"%u.%u.%u.%u",&v[0],&v[1],&v[2],&v[3]);
    //格式化文本信息
    char text[1024] = {0};
    sprintf(text,"Entering Passive Mode (%u,%u,%u,%u,%u,%u).",
            v[0],v[1],v[2],v[3],port>>8,port&0xFF);
    ftp_reply(sess,FTP_PASVOK,text);
}
static void do_type(session_t *sess)
{
    if(strcmp(sess->arg,"A")==0){
        sess->is_ascii=1;
        ftp_reply(sess,FTP_TYPEOK,"Switching to ASCII mode");
    }else if(strcmp(sess->arg,"I")==0){
        sess->is_ascii=0;
        ftp_reply(sess,FTP_TYPEOK,"Switching to Binary mode");
    }else if(strcmp(sess->arg,"M")==0){
        ftp_reply(sess,FTP_BADCMD,"Unrecognised TYPE command.");
    }
}
//static void do_stru(session_t *sess);
//static void do_mode(session_t *sess);
static void do_retr(session_t *sess)
{
    //下载文件  断点续载
    //创建数据连接
    if(get_transfer_fd(sess)==0)
    {
        return;
    }

    long long offset = sess->restart_pos;
    sess->restart_pos = 0;


    //打开文件
    int fd = open(sess->arg,O_RDONLY);
    if(fd == -1){
        ftp_reply(sess,FTP_FILEFAIL,"Failed to open File.");
        return;
    }
    //给文件加锁
    int ret;
    ret = lock_file_read(fd);
    if(ret==-1)
    {
          ftp_reply(sess,FTP_FILEFAIL,"Failed to open File.");
        return;
    }
    //是否是普通文件
    struct stat sbuf;
    ret = fstat(fd,&sbuf);
    if(!S_ISREG(sbuf.st_mode)){
        ftp_reply(sess,FTP_FILEFAIL,"Failed to open File.");
        return;
    }

    if(offset!=0)   //说明有断点
    {
        ret = lseek(fd,offset,SEEK_SET);
        if(ret == -1){
           ftp_reply(sess,FTP_FILEFAIL,"Failed to open File.");
        return;
        }
    }


    //150   模式判定  二进制
    char text[2048] ={0};
    if(sess->is_ascii)
    {
        sprintf(text,"Opening ASCII mode data connection for %s (%lld bytes).",
            sess->arg,(long long)sbuf.st_size);
    }else{
        sprintf(text,"Opening BINARY mode data connection for %s (%lld bytes).",
            sess->arg,(long long)sbuf.st_size);
    }    
    ftp_reply(sess,FTP_DATACONN,text);

    //下载文件
    int flag=0;
    //char buf[4096];
   /* while(1)
    {
        ret = read(fd,buf,sizeof(4096));   //会涉及到系统调用
        if(ret == -1){
            if(errno == EINTR)
            {
                continue;
            }else{
                flag =1;
                break;
            }        
        }else if(ret ==0){
            flag = 0;
            break;
        }
        if(writen(sess->data_fd,buf,ret))
        {
            flag = 2;
            break;
        }
    }*/

    //使用sendfile来发送文件
    // ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
    long long bytes_to_send = sbuf.st_size;
    if(offset>bytes_to_send){   //过大
            bytes_to_send = 0;
    }else{
        bytes_to_send -= offset;        //断点到文件结尾
    }
    sess->bw_transfer_start_sec = get_time_sec();
    sess->bw_transfer_start_usec = get_time_usec();
    while(bytes_to_send){
        long long num_this_time =  bytes_to_send>4096?4096:bytes_to_send;
        ret = sendfile(sess->data_fd,fd,NULL,num_this_time);
        if(ret == -1){
            flag =2;
            break;
        }
        limit_rate(sess,ret,0);
        bytes_to_send-=ret;
    }
    if(bytes_to_send==0){
        flag =0;
    }
    //关闭数据套接字
    //list_common(sess,1);
    close(sess->data_fd);
    sess->data_fd = -1;
    close(fd);
    if(flag==0){
         //226
        ftp_reply(sess,FTP_TRANSFEROK,"Transfer complete.");
    }else if(flag == 1){    //读取数据失败  426
        ftp_reply(sess,FTP_BADSENDFILE,"Failure reading from local file.");
    }else if(flag ==2){ //写入失败  451
        ftp_reply(sess,FTP_BADSENDNET,"Failure writting from local file.");
    }
    //重新开启控制连接通道闹钟
    start_cmdio_alarm();
}
static void do_stor(session_t *sess)        //上传文件
{   
    upload_common(sess,0);


}
static void do_appe(session_t *sess)
{
    upload_common(sess,1);
}
//150
static void do_list(session_t *sess)
{
    //创建数据连接
    if(get_transfer_fd(sess)==0)
    {
        return;
    }
    //150
    ftp_reply(sess,FTP_DATACONN,"Here comes the directory listing.");
    //传输列表
    list_common(sess,1);
    //关闭数据套接字
    close(sess->data_fd);
    //226
    ftp_reply(sess,FTP_TRANSFEROK,"Directory send OK");
}
//实现目录的短清单
static void do_nlst(session_t *sess)
{
    //创建数据连接
    if(get_transfer_fd(sess)==0)
    {
        return;
    }
    //150
    ftp_reply(sess,FTP_DATACONN,"Here comes the directory listing.");
    //传输列表
    list_common(sess,0);
    //关闭数据套接字
    close(sess->data_fd);
    //226
    ftp_reply(sess,FTP_TRANSFEROK,"Directory send OK");
}
static void do_rest(session_t *sess)
{
    sess->restart_pos = str_to_longlong(sess->arg);
    char text[1024] = {0};
    sprintf(text,"Restart position accepted (%lld)",sess->restart_pos);
    ftp_reply(sess,FTP_TRANSFEROK,text);

}
static void do_abor(session_t *sess)
{

}
//获取路径
static void do_pwd(session_t *sess)
{
    char text[1024] = {0};
    char dir[PATH_MAX] = {0};

    getcwd(dir,1024);
    sprintf(text,"\"%s\"",dir);

    ftp_reply(sess,FTP_PWDOK,text);

}
static void do_mkd(session_t *sess) //创建目录
{
        if(mkdir(sess->arg,0777)<0){
            return;
        }
        char text[4096]={0};
        if(sess->arg[0]=='/')
        {
            sprintf(text,"%s created",sess->arg);
        }else{
            char dir[4096+1]={0};
            getcwd(dir,4096);
            if(dir[strlen(dir)-1]=='/')
            {
                sprintf(text,"%s%s created",dir,sess->arg);
            }else{
                sprintf(text,"%s%s created",dir,sess->arg);
            }
        }
        ftp_reply(sess,FTP_MKDIROK,text);
}
static void do_rmd(session_t *sess)
{
    if(rmdir(sess->arg)<0){
          ftp_reply(sess,FTP_FILEFAIL,"Remove directory operation failed.");
        return;
    }
    ftp_reply(sess,FTP_RMDIROK,"Remove directory operation successful.");

}
static void do_dele(session_t *sess)
{
    if(unlink(sess->arg)<0)
    {
        ftp_reply(sess,FTP_FILEFAIL,"Delete operation failed.");
        return;
    }
    ftp_reply(sess,FTP_DELEOK,"Delete operation successful.");

}
static void do_rnfr(session_t *sess)    //要重命名的文件名
{
   sess->rnfr_name = (char *)malloc(strlen(sess->arg)+1);
    memset(sess->rnfr_name,0,strlen(sess->arg)+1);
    strcpy(sess->rnfr_name,sess->arg);
    ftp_reply(sess,FTP_RENAMEOK,"Ready for RNTO.");

}
static void do_rnto(session_t *sess)    //重命名文件后的文件名称
{  
    if(sess->rnfr_name==NULL){
        ftp_reply(sess,FTP_NEEDRNFR,"RNFR required first.");
        return;
    }
    rename(sess->rnfr_name,sess->arg);
    ftp_reply(sess,FTP_RENAMEOK,"Rename successful.");
    free(sess->rnfr_name);
    sess->rnfr_name=NULL;
}
static void do_site(session_t *sess)
{

}
static void do_syst(session_t *sess)
{
    ftp_reply(sess,FTP_SYSTOK,"UNIX TYPE: L8");
}
static void do_feat(session_t *sess)
{
    ftp_lreply(sess,FTP_FEAT,"Features:");
    writen(sess->ctrl_fd,"EPRT\r\n",strlen("EPRT\r\n"));
    writen(sess->ctrl_fd,"EPSV\r\n",strlen("EPSV\r\n"));
    writen(sess->ctrl_fd,"MDTM\r\n",strlen("MDTM\r\n"));
    writen(sess->ctrl_fd,"PASV\r\n",strlen("PASV\r\n"));
    writen(sess->ctrl_fd,"REST STREAM\r\n",strlen("REST STREAM\r\n"));
    writen(sess->ctrl_fd,"SIZE\r\n",strlen("SIZE\r\n"));
    writen(sess->ctrl_fd,"TVFS\r\n",strlen("TVFS\r\n"));
    writen(sess->ctrl_fd,"UTF8\r\n",strlen("UTF8\r\n"));
     ftp_reply(sess,FTP_FEAT,"End");
}
static void do_size(session_t *sess)        //查看文件大小
{
    struct stat buf;
    if(stat(sess->arg,&buf)<0){
        ftp_reply(sess,FTP_FILEFAIL,"SIZE operation failed");
        return;
    }
    //判断是否是普通文件
    if(!S_ISREG(buf.st_mode)){
        ftp_reply(sess,FTP_FILEFAIL,"Could not get file size."); 
        return;
    }
    char text[1024]={0};
    sprintf(text,"%lld",(long long)buf.st_size);
    ftp_reply(sess,FTP_SIZEOK,text); 

}
static void do_stat(session_t *sess)
{

}
static void do_noop(session_t *sess)
{

}
static void do_help(session_t *sess)
{

}