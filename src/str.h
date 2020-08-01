#ifndef _STR_H_
#define _STR_H_
/*
对字符串进行操作的函数

*/

void str_trim_crlf(char *str);      //去除\r\n
void str_split(const char *str , char *left, char *right, char c);      //字符串分割
int str_all_space(const char *str);     //判读是否全是空白字符  
void str_upper(char *str);      //字符串转化为大写格式
long long str_to_longlong(const char *str);     // 将字符串转换为long long 类型
unsigned int str_octal_to_uint(const char *str);    //将字符串(八进制)转化为无符号整型


#endif /* _STR_H_ */

