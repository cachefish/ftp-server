#include"privparent.h"
#include"privsock.h"
#include"sysutil.h"
#include "tunable.h"

static void privop_pasv_get_data_sock(session_t *sess);
static void privop_pasv_active(session_t *sess);
static void privop_pasv_listen(session_t *sess);
static void privop_pasv_accept(session_t *sess);
int capset(cap_user_header_t hdrp, const cap_user_data_t datap)
{
	return syscall(__NR_capset, hdrp, datap);
}
void minimize_privilege()
{
        struct  passwd *pw =  getpwnam("nobody");     //设置父进程为nobody进程
        if(pw==NULL){
            return; //直接退出
        }
            //设置组id  用户id  没改之前都是0
        if(setgid(pw->pw_gid)<0){
            ERR_EXIT("setgid");
        }
        if(setuid(pw->pw_uid)<0){
            ERR_EXIT("setuid");
        }
      //获取和设置进程的权限
        /*typedef struct __user_cap_header_struct {
	__u32 version;
	int pid;
} *cap_user_header_t;

typedef struct __user_cap_data_struct {
        __u32 effective;
        __u32 permitted;
        __u32 inheritable;
} *cap_user_data_t;
*/
        struct __user_cap_header_struct cap_header;
        struct __user_cap_data_struct cap_data;
        memset(&cap_header,0,sizeof(cap_header));
        memset(&cap_data,0,sizeof(cap_data));
        cap_header.version = _LINUX_CAPABILITY_VERSION_2;               //版本号
        cap_header.pid = 0;

        __u32 cap_mask = 0;
        cap_mask |=(1 << CAP_NET_BIND_SERVICE);
        cap_data.effective = cap_data.permitted = cap_mask;    //设置绑定端口特权
        cap_data.inheritable=0;

        capset(&cap_header,&cap_data);

}
//nobody进程  不断接受服务进程发送过来的命令 写出服务进程完成相应任务
void handle_parent(session_t *sess)
{

        minimize_privilege();

        char cmd;
        while(1){
            //read(sess->parent_fd,&cmd,sizeof(cmd));
            cmd = priv_sock_get_cmd(sess->parent_fd);
            //解析内部命令           
            //处理内部命令
            switch (cmd) {
            case PRIV_SOCK_GET_DATA_SOCK:
                privop_pasv_get_data_sock(sess);
                break;
            case PRIV_SOCK_PASV_ACTIVE:
                privop_pasv_active(sess);
                break;
            case PRIV_SOCK_PASV_LISTEN:
                privop_pasv_listen(sess);
                break;
            case PRIV_SOCK_PASV_ACCEPT:
                privop_pasv_accept(sess);
                break;		
            }
        }  
}


static void privop_pasv_get_data_sock(session_t *sess)
{
        /*nobody进程接收到PRIV_SOCK_GET_DATA_SOCK命令
        进一步接受一个整数，也就是端口号
        接受一个字符串，也就是ip
        fd = socket
        bind(20);
        connect(ip,port); */

    unsigned short port = (unsigned short)priv_sock_get_int(sess->parent_fd);
    char ip[16] = {0};
    priv_sock_recv_buf(sess->parent_fd,ip,sizeof(ip));
    
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr= inet_addr(ip);
    int fd = tcp_client(0);
    if(fd==-1)
    {
        priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
        return;
    }
    if(connect_timeout(fd,&addr,tunable_connect_timeout)<0)
    {
        close(fd);
        priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
        return;
    }
    priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
    priv_sock_send_fd(sess->parent_fd,fd);
    close(fd);
 
}
static void privop_pasv_active(session_t *sess)
{
    int active;
    if(sess->pasv_listen_fd != -1){
        active = 1;
    }else{
        active = 0;
    }
    priv_sock_send_int(sess->parent_fd,active);

}
static void privop_pasv_listen(session_t *sess)
{
    char ip[16]={0};
    getlocalip(ip);
    sess->pasv_listen_fd = tcp_server(ip,0);
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if(getsockname(sess->pasv_listen_fd,(struct sockaddr*)&addr,&addrlen)<0)
    {
        ERR_EXIT("getsockname");
    }
    unsigned short port = ntohs(addr.sin_port);

    priv_sock_send_int(sess->parent_fd,(int)port);
}
static void privop_pasv_accept(session_t *sess)
{
     int fd = accept_timeout(sess->pasv_listen_fd,NULL,tunable_accept_timeout);
     close(sess->pasv_listen_fd);
     sess->pasv_listen_fd = -1;
    if(fd==-1)
    {          
        priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
        return ;
    }
    priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_OK);
    priv_sock_send_fd(sess->parent_fd,fd);
    close(fd);
}  
