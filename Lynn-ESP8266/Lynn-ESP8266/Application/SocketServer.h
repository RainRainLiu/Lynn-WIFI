#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "c_types.h"
#include "SimpleSignalSolts.h"
typedef struct
{
	uint8_t		*pData;
	uint32_t	unLength;
}SOCKET_SERVER_DATA_ITEM_T;

typedef void* SOCKET_SERVER_HANDLE_T;

//接收完成回调    
typedef void(*SOCKET_SERVER_RECEIVE_CB)(void *, uint8 *pData, uint32 unLength);

/******************************************************************
* @函数说明：   构造函数
* @输入参数：   int port			服务器监听端口号
* @返回参数：   SOCKET_SERVER_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
******************************************************************/
SOCKET_SERVER_HANDLE_T SocketServer_Create(int iListenPort);
/******************************************************************
* @函数说明：   向客户端写入数据
* @输入参数：   SOCKET_SERVER_HANDLE_T hHandle 句柄
uint8_t *pData, 数据指针
uint32_t unLength	数据长度
* @返回参数：   SOCKET_SERVER_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
******************************************************************/
int SocketServer_Write(SOCKET_SERVER_HANDLE_T hHandle, uint8_t *pData, uint32_t unLength);
/******************************************************************
* @函数说明：   注册接收到数据的回调函数
* @输入参数：   IMPLE_SOLTS slots,         槽
void *pArg                回调时的参数
* @输出参数：   无
* @返回参数：   ErrorStatus
* @修改记录：   ----
******************************************************************/
ErrorStatus SocketServer_RegisterRxCB(SOCKET_SERVER_HANDLE_T hHadnle, SIMPLE_SOLTS slots, void *pArg);

#endif