#include"ftpproto.h"
#include"sysutil.h"
#include"str.h"
#include"ftpcodes.h"
#include"tunable.h"
void ftp_reply(session_t *sess,int status,const char*text);
void ftp_lreply(session_t *sess,int status,const char*text);
int get_transfer_fd(session_t* sess);

int port_active(session_t* sess);
int pasv_active(session_t* sess);

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

int list_common(session_t*sess)
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
        //过滤.xxx文件
        if(dt->d_name[0]=='.'){
            continue;
        }

        //开始解析权限位  drwxr-xr-x
        char perms[]="----------";
        perms[0]='?';

        mode_t mode = sbuf.st_mode;
        switch (mode&S_IFMT)
        {
        case S_IFREG:
            perms[0]='-';
            break;
        case S_IFDIR:
            perms[0]='d';
            break;
        case S_IFLNK:
            perms[0]='l';
            break;
        case S_IFIFO:
            perms[0]='p';
            break;
        case S_IFSOCK:
            perms[0]='s';
            break;
        case S_IFCHR:
            perms[0]='c';
            break;
        case S_IFBLK:
            perms[0]='b';
            break;
        default:
            break;
        }

        if(mode&&S_IRUSR)
        {
            perms[1]='r';
        }
         if(mode&&S_IWUSR)
        {
            perms[2]='w';
        }
         if(mode&&S_IXUSR)
        {
            perms[3]='x';
        }
         if(mode&&S_IRGRP)
        {
            perms[4]='r';
        }
         if(mode&&S_IWGRP)
        {
            perms[5]='w';
        }
         if(mode&&S_IXGRP)
        {
            perms[6]='x';
        }
         if(mode&&S_IROTH)
        {
            perms[7]='r';
        }
         if(mode&&S_IWOTH)
        {
            perms[8]='w';
        }
         if(mode&&S_IXOTH)
        {
            perms[9]='x';
        }
        if(mode&S_ISUID)
        {
            perms[3]=(perms[3]=='x')?'s':'S';
        }
        if(mode&S_ISGID)
        {
            perms[6]=(perms[6]=='x')?'s':'S';
        }
        if(mode&S_ISVTX)
        {
            perms[9]=(perms[9]=='x')?'t':'T';
        }
        
        //硬连接数   st_nlink 
        char buf[1024] = {0};
        int off = 0;
        off += sprintf(buf,"%s",perms);
        off += sprintf(buf+off,"%3ld %-8d %-8d ",sbuf.st_nlink,sbuf.st_uid,sbuf.st_gid);
        off += sprintf(buf+off,"%8lu  ",sbuf.st_size);

        //时间格式  
        const char *p_date_format = "%b %e %H:%M";
        struct timeval tv;
        gettimeofday(&tv,NULL);     //获取系统时间
        time_t local_time = tv.tv_sec;
        if(sbuf.st_mtime>local_time||(local_time-sbuf.st_mtime)>60*60*24*182)
        {
            p_date_format =  "%b  %e  %Y";
        }
        
        char databuf[64]={0};
        struct tm*p_tm = localtime(&local_time);
        strftime(databuf,sizeof(databuf),p_date_format,p_tm);
        off += sprintf(buf+off,"%s",databuf);

        
        //文件名  要判断是否是链接文件
        
        if(S_ISLNK(sbuf.st_mode)){
            char tmp[1024] = {0};
            readlink(dt->d_name,tmp,sizeof(tmp));
            off += sprintf(buf+off,"%s -> %s\r\n",dt->d_name,tmp);
       }else{
            off += sprintf(buf+off,"%s\r\n",dt->d_name);
       }

        writen(sess->data_fd,buf,sizeof(buf));
        //printf("%s",buf);
   }
   closedir(dir);
   return 1;
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
    if(sess->port_addr){
        if(port_active(sess))
        {
                fprintf(stderr,"botn port an pasv are active ");
                exit(EXIT_FAILURE);
        }
        return 1;
    }
    return 0;
}
int get_transfer_fd(session_t* sess)
{
    //判断是否收到PORT或者PASV命令
    if(!port_active(sess)&&!pasv_active(sess)){
        ftp_reply(sess,FTP_BADSENDCONN,"Use PORT or PASV first.");
            return 0;
    }
    //主动模式
    if(port_active(sess))
    {
        //创建套接字，绑定20，connect对方
        int fd =  tcp_client(0);
        if(connect_timeout(fd,sess->port_addr,tunable_data_connection_timeout))
        {
                close(fd);
                return 0;
        }

        sess->data_fd = fd;     
    } 
    if(pasv_active(sess)){  //如果是被动模式
        int fd = accept_timeout(sess->pasv_listen_fd,NULL,tunable_accept_timeout);
        close(sess->pasv_listen_fd);
        if(fd==-1)
        {          
            return 0;
        }
        sess->data_fd = fd;
    }
    if(sess->port_addr){
        free(sess->port_addr);
        sess->port_addr=NULL;
    }
    return 1;
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
    setegid(pw->pw_gid);
    seteuid(pw->pw_uid);
    chdir(pw->pw_dir);      //切换到用户目录

    //230响应
    ftp_reply(sess,FTP_LOGINOK,"Login successful.");
 
}

static void do_cwd(session_t *sess)
{

}
static void do_cdup(session_t *sess)
{

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
    sess->pasv_listen_fd = tcp_server(ip,0);
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if(getsockname(sess->pasv_listen_fd,(struct sockaddr*)&addr,&addrlen)<0)
    {
        ERR_EXIT("getsockname");
    }
    unsigned port = ntohs(addr.sin_port);
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

}
static void do_stor(session_t *sess)
{

}
static void do_appe(session_t *sess)
{

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
    list_common(sess);
    //关闭数据套接字
    close(sess->data_fd);
    //226
    ftp_reply(sess,FTP_TRANSFEROK,"Directory send OK");
}
static void do_nlst(session_t *sess)
{

}
static void do_rest(session_t *sess)
{

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
static void do_mkd(session_t *sess)
{

}
static void do_rmd(session_t *sess)
{

}
static void do_dele(session_t *sess)
{

}
static void do_rnfr(session_t *sess)
{

}
static void do_rnto(session_t *sess)
{

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
static void do_size(session_t *sess)
{

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