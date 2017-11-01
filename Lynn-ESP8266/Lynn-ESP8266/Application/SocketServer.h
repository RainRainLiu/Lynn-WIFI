#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "c_types.h"

typedef void* SOCKET_SERVER_HANDLE_T;

//������ɻص�    
typedef void(*SOCKET_SERVER_RECEIVE_CB)(void *, uint8 *pData, uint32 unLength);

/******************************************************************
* @����˵����   ���캯��
* @���������   int port			�����������˿ں�
* @���ز�����   SOCKET_SERVER_HANDLE_T ���
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
SOCKET_SERVER_HANDLE_T SocketServer_Create(int iListenPort, SOCKET_SERVER_RECEIVE_CB ReceiveCB, void *pvParameters);
#endif