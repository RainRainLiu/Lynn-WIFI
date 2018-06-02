#ifndef _UART_PROCESS_H_
#define _UART_PROCESS_H_

#include "c_types.h"

#include "SimpleSignalSolts.h"
#include "user_config.h"
#include "uart_register.h"
#include "uart.h"

typedef void * UART_PROCESS_HANDLE_T;

/******************************************************************
* @函数说明：   进程
* @输入参数：  UART_Port uart_no  使用的UART端口号
* @返回参数：
* @修改记录：   2017/10/28 初版
******************************************************************/
UART_PROCESS_HANDLE_T UartProcess_Create(UART_Port uart_no, uint32 unBaudRate);

/******************************************************************
* @函数说明：   注册接收到数据的回调函数
* @输入参数：   IMPLE_SOLTS slots,         槽
void *pArg                回调时的参数
* @输出参数：   无
* @返回参数：   ErrorStatus
* @修改记录：   ----
******************************************************************/
ErrorStatus UartProcess_RegisterRxCB(UART_PROCESS_HANDLE_T hHadnle, SIMPLE_SOLTS slots, void *pArg);
/******************************************************************
* @函数说明：   串口写入数据，堵塞式
* @输入参数：   UART_PROCESS_HANDLE_T hHandle 句柄
uint8_t *pData, 数据指针
uint32_t unLength	数据长度
* @返回参数：   int 写入长度
* @修改记录：   2017/10/28 初版
******************************************************************/
int UartProcess_Write(UART_PROCESS_HANDLE_T hHandle, uint8_t *pData, uint32_t unLength);
#endif
