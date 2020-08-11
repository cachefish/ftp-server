#include"common.h"
#include"sysutil.h"
#include"session.h"
#include"parseconf.h"
#include"tunable.h"
#include"ftpproto.h"
#include"ftpcodes.h"
#include"hash.h"

extern session_t*p_sess;
static unsigned int  s_children;

static hash_t*s_ip_count_hash;
static hash_t*s_pid_ip_hash;

void check_limits(session_t* sess);    //检查连接数限制

void handle_sigchld(int sig);

unsigned int hash_func(unsigned int buckets, void*key);

unsigned int handle_ip_count(void *ip); 

void    drop_ip_count(void* ip);  //-1


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
                                        NULL,-1,-1,0,
                                        //限速
                                        0,0,0,0,
                                        /*父子进程通道*/
                                        -1,-1,
                                        /*FTP协议状态*/
                                        0,0,NULL,0,
                                        //最大连接数
                                        0,0
                                        };  //初始

    p_sess = &sess;

    sess.bw_upload_rate_max = tunable_upload_max_rate;
    sess.bw_download_rate_max = tunable_download_max_rate;

    s_ip_count_hash = hash_alloc(256,hash_func);
    s_pid_ip_hash = hash_alloc(256,hash_func);

    signal(SIGCHLD,handle_sigchld);   //忽略

    int listenfd = tcp_server(tunable_listen_address,tunable_listen_port);
    int conn;
    pid_t pid;
    //接受客户端连接  使用多进程方式
    struct sockaddr_in addr;
    while(1){
            
            conn = accept_timeout(listenfd,&addr,0);
            if(conn==-1){
                ERR_EXIT("accept _timeout");
            }
            unsigned int ip =  addr.sin_addr.s_addr;
            handle_ip_count(&ip); 

            ++s_children;
            sess.num_client = s_children;
            sess.num_this_ip = handle_ip_count(&ip);

            pid = fork();
            if(pid == -1){
                s_children--;
                ERR_EXIT("fork");
            }
            if(pid==0){
                close(listenfd);
                sess.ctrl_fd = conn;
                check_limits(&sess);    //检查连接数限制
                signal(SIGCHLD,SIG_IGN);
                begin_session(&sess);    //启动一个会话 
            }else{
                hash_add_entry(s_pid_ip_hash,&pid,sizeof(pid),(void*)&ip,sizeof(unsigned int));
                close(conn);
            }
    }
    return 0;
}


void check_limits(session_t* sess)    //检查连接数限制
{
    if(tunable_max_clients>0&&sess->num_client>tunable_max_clients){
        ftp_reply(sess,FTP_TOO_MANY_USERS,"There are too many users,please try later.");
        exit(EXIT_FAILURE);
    }
       if(tunable_max_per_ip>0&&sess->num_this_ip>tunable_max_per_ip){
        ftp_reply(sess,FTP_IP_LIMIT,"There are too  many conections from your internet address.");
        exit(EXIT_FAILURE);
    }

}

void handle_sigchld(int sig)
{
    //当一个客户端推出的时候，那么该客户端对应ip的连接数要减1，
    //处理过程是这样的，首先时客户端退出的时候，
    //父进程需要知道这个客户端的ip，这可以通过在s_pid_ip_hash查找得到
    //得到ip后进而可以在s_ip_count_hash表中找到对应的连接数，从而进行减1操作
    pid_t pid;
    while((pid = waitpid(-1,NULL,WNOHANG))>0)
    {
        --s_children;
        unsigned int *ip =  hash_lookup_entry(s_pid_ip_hash,&pid,sizeof(pid));
        if(ip==NULL){
            continue;
        }
        drop_ip_count(ip);  //-1
        hash_free_entry(s_pid_ip_hash,&pid,sizeof(pid));
    }

}


unsigned int hash_func(unsigned int buckets, void*key)
{
    //关键码ip  int
    unsigned int *number = (unsigned int*) key;
    return (*number)%buckets;

}


unsigned int handle_ip_count(void *ip)
{
    //当一个客户登录的时候，要在s_ip_count_hash更新这个表中的对应表项，
    //即该ip对应的连接数要加1，如果这个表项不存在，就添加一条记录，并且将ip对应的连接数置一
    unsigned int count;
    unsigned int * p_count = (unsigned int*)hash_lookup_entry(s_ip_count_hash,ip,sizeof(unsigned int));
    if(p_count==NULL){
        count=1;
        hash_add_entry(s_ip_count_hash,ip,sizeof(unsigned int),&count,sizeof(unsigned int));

    }else{
        count = *p_count;
        ++count;
        *p_count = count;
    }
    return count;
}


void    drop_ip_count(void* ip) //-1
{
    unsigned int count;
    unsigned int * p_count = (unsigned int*)hash_lookup_entry(s_ip_count_hash,ip,sizeof(unsigned int));
    if(p_count==NULL){
        return;
    }
    count =*p_count;
    if(count<=0){
        return;
    }
    --count;
    *p_count = count;
    if(*p_count==0){
        hash_free_entry(s_ip_count_hash,&ip,sizeof(unsigned int));
    }
 
}
