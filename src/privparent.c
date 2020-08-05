#include"privparent.h"
#include"privsock.h"

static void privop_pasv_get_data_sock(session_t *sess);
static void privop_pasv_active(session_t *sess);
static void privop_pasv_listen(session_t *sess);
static void privop_pasv_accept(session_t *sess);

//nobody进程  不断接受服务进程发送过来的命令 写出服务进程完成相应任务
void handle_parent(session_t *sess)
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

}
static void privop_pasv_active(session_t *sess)
{

}
static void privop_pasv_listen(session_t *sess)
{

}
static void privop_pasv_accept(session_t *sess)
{
    
}
