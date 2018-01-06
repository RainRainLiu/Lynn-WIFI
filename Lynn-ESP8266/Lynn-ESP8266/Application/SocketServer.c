#include "SocketServer.h"
#include "MyDefine.h"
#include "user_config.h"

#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "freertos/queue.h"
#include "freertos/timers.h"



typedef struct
{
	int nServerHandle; 	//����˾��
	int anClientHandleTalbe[SOCKET_SERVER_MAXIMUN_CONNECTION];	//�ͻ��˾���б�
	uint8_t *pReading;	//���ڶ�ȡ������ָ��
	xQueueHandle hClientQueue;
	xTimerHandle hRxTimer;	//�������ݵĶ�ʱ������ʱ����ʱ�ſ�����������д������
}SOCKET_SERVER_T;


typedef struct
{
	int *pnSocketHandle;
	uint8_t bReading;
	SOCKET_SERVER_T *hServer;
}SOCKET_RECEIVE_T;

/******************************************************************
* @����˵����   ���տͻ��˷��͹��������ݣ�������ն���
				Ϊ�˷�ֹ����ͻ���ͬʱд������ʱ�����������ݰ�˳��
			    Ҫ�ȴ������ͻ���д��������д��
* @���������   void *pvParameters
* @���ز�����
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
static void SocketServer_ReveiveClient(void *pvParameters)
{
	CheckNull(pvParameters);

	SOCKET_RECEIVE_T *receive = pvParameters;

	int socketHandle = *receive->pnSocketHandle;
	receive->bReading = 0;
	uint8_t *rxBuf;
	rxBuf = os_malloc(SCOKET_SERVER_RECEIVE_BUFF_LENGTH);  //�����ڴ�

	while (1)
	{
		int length = read(socketHandle, rxBuf, SCOKET_SERVER_RECEIVE_BUFF_LENGTH);	//��������
		if (length > 0)
		{
			if (receive->hServer->pReading == NULL)
			{
				
			}
			else if (*receive->hServer->pReading && receive->hServer->pReading != &receive->bReading)	//����߳���������д������
			{
				while (*receive->hServer->pReading)	//�ȴ����
				{
					vTaskDelay(1);
				}
			}
			
			receive->bReading = 1;	//æ�����ڶ�ȡ
			receive->hServer->pReading = &receive->bReading;	//ִ�е�ǰ�����æ��־
			xQueueSend(receive->hServer->hClientQueue, rxBuf, 1000);
			xTimerReset(receive->hServer->hRxTimer, 1000);
		}
		else
		{
			close(socketHandle);
			receive->pnSocketHandle = 0;	//���ָ��
			os_printf("read error %d\r\n", length);
			break;
		}
	}
	os_free(rxBuf);
	os_free(receive);
	vTaskDelete(NULL);	//ɾ������
}

/******************************************************************
  * @�������ƣ�   COM_Socket_CreatServer
  * @����˵����   ����������
  * @���������   int *sockte_handle   socket�������
  * 			  int port			�������˿ں�
  * @���ز�����   �� 
  * @�޸ļ�¼��   2016/11/29 ����
******************************************************************/
static bool SocketServer_CreateServer(int *sockte_handle, int port)
{
	int sta_socket = socket(PF_INET, SOCK_STREAM, 0); 	//����socket
	
	if(sta_socket >= 0)
	{
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr)); /* Zero out structure */
        
		server_addr.sin_family = AF_INET; /* Internet address family */
		server_addr.sin_addr.s_addr = INADDR_ANY; /* Any incoming interface */
		server_addr.sin_len = sizeof(server_addr);
		server_addr.sin_port = htons(port); /* Local port */
        
		int result = bind(sta_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
        
		if (result == 0) 
		{
			result = listen(sta_socket, SOCKET_SERVER_MAXIMUN_CONNECTION);
			if (result == 0)
			{
				*sockte_handle = sta_socket;
				return TRUE;
			}
			os_printf("linten error\r\n");
		}
		os_printf("bind error\r\n");
	}
	else
	{
		os_printf("creat scoket error\r\n");
	}
	close(sta_socket);
	return false;
}
/******************************************************************
* @����˵����   ����
* @���������   void *pvParameters
* @���ز�����   
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
static void SocketServer_Task(void *pvParameters)
{
	CheckNull(pvParameters);

	SOCKET_SERVER_T *server = pvParameters;

	os_printf("Server lintening\r\n");
	while (1)
	{
		struct sockaddr_in client_addr;
		socklen_t length = sizeof(client_addr);

		int clientHandle = accept(server->nServerHandle, (struct sockaddr*)&client_addr, &length);	//�ȴ�����

		os_printf("clientHandle %d\r\n", clientHandle);
		if (clientHandle > 0)
		{
			uint8_t i;

			for (i = 0; i < SOCKET_SERVER_MAXIMUN_CONNECTION; i++)
			{
				if (server->anClientHandleTalbe[i] == 0)
				{
					server->anClientHandleTalbe[i] = clientHandle;

					SOCKET_RECEIVE_T *receive = os_malloc(sizeof(SOCKET_RECEIVE_T));
					receive->pnSocketHandle = &server->anClientHandleTalbe[i];
					receive->hServer = server;
					xTaskCreate(SocketServer_ReveiveClient, (signed char *)"SoeketReceive", 256, receive, 5, NULL);
					break;
				}
			}
		}
		else
		{
			close(clientHandle);
			os_printf("Server accept error\r\n");
		}
	}
	
}

void SocketServer_TimerCB(xTimerHandle xTimer)
{
	SOCKET_SERVER_T *server = pvTimerGetTimerID(xTimer);
	server->pReading = 0;
}

/******************************************************************
* @����˵����   ���캯��
* @���������   int port			�����������˿ں�
* @���ز�����   SOCKET_SERVER_HANDLE_T ���
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
SOCKET_SERVER_HANDLE_T SocketServer_Create(int iListenPort)
{
	SOCKET_SERVER_T *server = os_malloc(sizeof(SOCKET_SERVER_T));

	memset(server, 0, sizeof(SOCKET_SERVER_T));
	if (SocketServer_CreateServer(&server->nServerHandle, iListenPort) == false)	//���������������˿�
	{
		os_free(server);
		return NULL;
	}

	server->hRxTimer = xTimerCreate("SoeketServer", 10, false, server, SocketServer_TimerCB);
	xTaskCreate(SocketServer_Task, (signed char *)"SoeketServer", 256, server, 4, NULL);
	return server;
}

/******************************************************************
* @����˵����   ��ͻ���д������
* @���������   SOCKET_SERVER_HANDLE_T hHandle ���
			    uint8_t *pData, ����ָ��
			    uint32_t unLength	���ݳ���
* @���ز�����   SOCKET_SERVER_HANDLE_T ���
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
int SocketServer_Write(SOCKET_SERVER_HANDLE_T hHandle, uint8_t *pData, uint32_t unLength)
{
	CheckNull(hHandle);

	SOCKET_SERVER_T *server = hHandle;
	uint8_t i;

	for (i = 0; i < SOCKET_SERVER_MAXIMUN_CONNECTION; i++)
	{
		if (server->anClientHandleTalbe[i] != 0)
		{
			write(server->anClientHandleTalbe[i], pData, unLength);
		}
	}
	return 1;
}