#ifndef _UART_PROCESS_H_
#define _UART_PROCESS_H_

#include "c_types.h"

typedef void * UART_PROCESS_HANDLE_T;

/******************************************************************
* @����˵����   ����
* @���������  UART_Port uart_no  ʹ�õ�UART�˿ں�
* @���ز�����
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
UART_PROCESS_HANDLE_T UartProcess_Create(UART_Port uart_no, uint32 unBaudRate);

/******************************************************************
* @����˵����   ע����յ����ݵĻص�����
* @���������   IMPLE_SOLTS slots,         ��
void *pArg                �ص�ʱ�Ĳ���
* @���������   ��
* @���ز�����   ErrorStatus
* @�޸ļ�¼��   ----
******************************************************************/
ErrorStatus UartProcess_RegisterRxCB(UART_PROCESS_HANDLE_T hHadnle, SIMPLE_SOLTS slots, void *pArg);

#endif
