#ifndef _FTP_PROTO_H_
#define _FTP_PROTO_H_
#include"session.h"
//服务进程的一些函数

void handle_child(session_t *sess);

int list_common(session_t*sess);




#endif