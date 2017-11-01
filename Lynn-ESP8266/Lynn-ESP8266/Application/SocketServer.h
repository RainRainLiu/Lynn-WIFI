#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "c_types.h"

typedef void* SOCKET_SERVER_HANDLE_T;

//接收完成回调    
typedef void(*SOCKET_SERVER_RECEIVE_CB)(void *, uint8 *pData, uint32 unLength);

/******************************************************************
* @函数说明：   构造函数
* @输入参数：   int port			服务器监听端口号
* @返回参数：   SOCKET_SERVER_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
******************************************************************/
SOCKET_SERVER_HANDLE_T SocketServer_Create(int iListenPort, SOCKET_SERVER_RECEIVE_CB ReceiveCB, void *pvParameters);
#endif