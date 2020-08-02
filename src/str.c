#include"str.h"
#include"common.h"


void str_trim_crlf(char *str)     //去除每行最后的\r\n
{
        char *p = &str[strlen(str)-1];   //指向最后一个字符
        while(*p=='\r'||*p=='\n'){
            *p --= '\0';
        }

}

void str_split(const char *str , char *left, char *right, char c)      //字符串分割
{
    char *p=strchr(str,c);  //查找c所在位置
    if(p==NULL){    //没有空格
        strcpy(left,str);   //将一整串拷贝到left中，right为空串
    }else{
        strncpy(left,str,p-str);            
        strcpy(right,p+1);
    }
}
int str_all_space(const char *str)     //判读是否全是空白字符串 为空白字符串就返回1 
{
    while(*str){
        if(!isspace(*str)){
            return 0;
        }
        str++;
    }
    return 1;
}
void str_upper(char *str)      //字符串转化为大写格式
{
    while(*str){
        *str = toupper(*str);
        str++;
    }
}
long long str_to_longlong(const char *str)     // 将字符串转换为long long 类型
{
   long long result = 0;
	long long mult = 1;
	unsigned int len = strlen(str);
	int i;

	if (len > 15)
		return 0;

	for (i=len-1; i>=0; i--) {  //因为unsigned int i   i--  0-1,不是-1
		char ch = str[i];
		long long val;
		if (ch < '0' || ch > '9')
			return 0;

		val = ch - '0';
		val *= mult;
		result += val;
		mult *= 10;
	}
	return result;
}
unsigned int str_octal_to_uint(const char *str)    //将字符串(八进制)转化为无符号整型
{
    unsigned int result = 0;
    int seen_non_zero_figit = 0;
    while(*str){
        int digit = *str;
        if(!isdigit(digit)||digit>'7')
            break;
        if(digit != '0'){
            seen_non_zero_figit = 1;
        }
        if(seen_non_zero_figit){
            result<<=3;  //*8
            result += (digit-'0');
        }
        str++;
    }
    return result;
}
