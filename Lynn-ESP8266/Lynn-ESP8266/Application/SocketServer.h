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

//������ɻص�    
typedef void(*SOCKET_SERVER_RECEIVE_CB)(void *, uint8 *pData, uint32 unLength);

/******************************************************************
* @����˵����   ���캯��
* @���������   int port			�����������˿ں�
* @���ز�����   SOCKET_SERVER_HANDLE_T ���
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
SOCKET_SERVER_HANDLE_T SocketServer_Create(int iListenPort);
/******************************************************************
* @����˵����   ��ͻ���д������
* @���������   SOCKET_SERVER_HANDLE_T hHandle ���
uint8_t *pData, ����ָ��
uint32_t unLength	���ݳ���
* @���ز�����   SOCKET_SERVER_HANDLE_T ���
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
int SocketServer_Write(SOCKET_SERVER_HANDLE_T hHandle, uint8_t *pData, uint32_t unLength);
/******************************************************************
* @����˵����   ע����յ����ݵĻص�����
* @���������   IMPLE_SOLTS slots,         ��
void *pArg                �ص�ʱ�Ĳ���
* @���������   ��
* @���ز�����   ErrorStatus
* @�޸ļ�¼��   ----
******************************************************************/
ErrorStatus SocketServer_RegisterRxCB(SOCKET_SERVER_HANDLE_T hHadnle, SIMPLE_SOLTS slots, void *pArg);

#endif