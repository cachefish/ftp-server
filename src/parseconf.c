#include"parseconf.h"
#include"common.h"
#include"tunable.h"
#include"str.h"
/*
extern int tunable_pasv_enable;     //是否开启被动模式
extern int tunable_port_enable;      //是否开启主动模式

extern unsigned int tunable_listen_port;        //FTP服务端口
extern unsigned int tunable_max_clients;      //最大连接数
extern unsigned int tunable_max_per_ip;       //每个ip最大连接数

extern unsigned int tunable_accept_timeout;     //accept超时时间
extern unsigned int tunable_connect_timeout;    //connect超时时间
extern unsigned int tunable_idle_session_timeout;       //控制连接超时时间
extern unsigned int tunable_data_connection_timeout;        //数据连接超时时间

extern unsigned int tunable_local_umask;        //掩码
extern unsigned int tunable_upload_max_rate;        //最大上传速度
extern unsigned int tunable_download_max_rate;      //最大下载速度

extern const char *tunable_listen_address;      //FTP服务器ip地址

*/

//找到表中相对应的配置名称，然后将配置信息保存到相对应的变量中
//字符串  布尔  unsigned int   三个表格
static struct parseconf_bool_setting {
  const char *p_setting_name;
  int *p_variable;
}

parseconf_bool_array[] = {
	{ "pasv_enable", &tunable_pasv_enable },
	{ "port_enable", &tunable_port_enable },
	{ NULL, NULL }
};

static struct parseconf_uint_setting {
	const char *p_setting_name;
	unsigned int *p_variable;
}
parseconf_uint_array[] = {
	{ "listen_port", &tunable_listen_port },
	{ "max_clients", &tunable_max_clients },
	{ "max_per_ip", &tunable_max_per_ip },
	{ "accept_timeout", &tunable_accept_timeout },
	{ "connect_timeout", &tunable_connect_timeout },
	{ "idle_session_timeout", &tunable_idle_session_timeout },
	{ "data_connection_timeout", &tunable_data_connection_timeout },
	{ "local_umask", &tunable_local_umask },
	{ "upload_max_rate", &tunable_upload_max_rate },
	{ "download_max_rate", &tunable_download_max_rate },
	{ NULL, NULL }
};

static struct parseconf_str_setting {
	const char *p_setting_name;
	const char **p_variable;
}

parseconf_str_array[] = {
	{ "listen_address", &tunable_listen_address },
	{ NULL, NULL }
};

void parseconf_load_file(const char *path)    //加载配置文件
{
    FILE *fp = fopen(path,"r");
    if(fp==NULL){
        ERR_EXIT("fopen");
    }
    char setting_line[1024] = {0};
    while(fgets(setting_line,sizeof(setting_line),fp)!=NULL){
        if(strlen(setting_line)==0||setting_line[0]=='#'||str_all_space(setting_line)){
            continue;
        }
        //去除\r\n
        str_trim_crlf(setting_line);
        //将相应的配置信息保存到对应的
        parseconf_load_setting(setting_line);
        //再获取下一行
        memset(setting_line,0,sizeof(setting_line));
    }
    fclose(fp);
}



void parseconf_load_setting(const char *setting)      //将配置项加载到相应的变量
{
    //得到了一行配置数据,加载到相应
    while(isspace(*setting)){       //去左空格
        setting++;
    }
    //解析出key--value
    char key[128]={0};
    char value[128] = {0};
    str_split(setting,key,value,'=');
    if(strlen(value)==0){
        fprintf(stderr,"missing value in config file for:%s\n",key);
        exit(EXIT_FAILURE);
    }
    //字符串型表
    {
        const struct parseconf_str_setting *p_str_setting = parseconf_str_array; 
        while(p_str_setting->p_setting_name!=NULL){
            if(strcmp(key,p_str_setting->p_setting_name)==0){       //找到，存放
                const char **p_cur_setting = p_str_setting->p_variable;     //指向这个地址
                if(*p_cur_setting){         //有数据    
                    free((char*)*p_cur_setting);
                }
                *p_cur_setting = strdup(value);     //就指针指向value,先申请内存，然后将value拷贝到里面
                return;
            }

            p_str_setting++;
        }
    }
    //bool类型表
    {
        const struct parseconf_bool_setting *p_bool_setting = parseconf_bool_array; 
        while(p_bool_setting->p_setting_name!=NULL){
            if(strcmp(key,p_bool_setting->p_setting_name)==0){
                str_upper(value);
                if(strcmp(value,"YES")==0||strcmp(value,"TRUE")||strcmp(value,"1")==0){
                    *(p_bool_setting->p_variable) = 1;
                }else if(strcmp(value,"NO")==0||strcmp(value,"FALSE")||strcmp(value,"0")==0){
                    *(p_bool_setting->p_variable) = 0;
                }else{
                    fprintf(stderr,"bad bool value in config file for:%s\n",key);
                    exit(EXIT_FAILURE);
                }
                return;
            }
            p_bool_setting++;
        }
    }

    {
        const struct parseconf_uint_setting *p_uint_setting = parseconf_uint_array; 
        while(p_uint_setting->p_setting_name!=NULL){
            if(strcmp(key,p_uint_setting->p_setting_name)==0){
                if(value[0]=='0')       //八进制
                {
                    *(p_uint_setting->p_variable) = str_octal_to_uint(value);
                }else{
                    *(p_uint_setting->p_variable)=atoi(value);
                }
                return;
            }
            p_uint_setting++;
        }
    }
/*
	{ "listen_port", &tunable_listen_port },
	{ "max_clients", &tunable_max_clients },
	{ "max_per_ip", &tunable_max_per_ip },
	{ "accept_timeout", &tunable_accept_timeout },
	{ "connect_timeout", &tunable_connect_timeout },
	{ "idle_session_timeout", &tunable_idle_session_timeout },
	{ "data_connection_timeout", &tunable_data_connection_timeout },
	{ "local_umask", &tunable_local_umask },
	{ "upload_max_rate", &tunable_upload_max_rate },
	{ "download_max_rate", &tunable_download_max_rate },
	{ NULL, NULL }
*/

}