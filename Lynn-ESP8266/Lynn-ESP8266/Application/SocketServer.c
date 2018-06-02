#include "SocketServer.h"
#include "MyDefine.h"
#include "user_config.h"
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "SimpleSignalSolts.h"

typedef struct
{
	xQueueHandle	hClientQueue;
	int				nSocketHandle;
	SIMPLE_SIGNAL	hSignalRx;
	xTaskHandle		hTaskRead;
	xTaskHandle		hTaskWrite;
	void *			hServer;
}SOCKET_CLIENT_T;

typedef struct
{
	int nServerHandle; 	//����˾��
	xQueueHandle hClientQueue;
	xTimerHandle hRxTimer;	//�������ݵĶ�ʱ������ʱ����ʱ�ſ�����������д������
	SIMPLE_SIGNAL	hSignalRx;
	SOCKET_CLIENT_T	*paClientTable[SOCKET_SERVER_MAXIMUN_CONNECTION];
	
}SOCKET_SERVER_T;


/******************************************************************
* @����˵����   �رտͻ��ˣ��ͷſͻ���ռ�õ���Դ
* @���������   SOCKET_CLIENT_T *client, 
			    xTaskHandle nowTask
* @���ز�����
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
static void SocketServer_CloseClient(SOCKET_CLIENT_T *client, xTaskHandle nowTask)
{
	CheckNull(client);
	SOCKET_SERVER_T *server = client->hServer;

	uint32_t i;
	for (i = 0; i < SOCKET_SERVER_MAXIMUN_CONNECTION; i++)
	{
		if (server->paClientTable[i] == client)
		{
			server->paClientTable[i] = 0;
		}
	}
	if (nowTask == NULL)//�ڹر�SOCKET����֮ǰҪ�ر�����һ�����̣��Է��ظ��ر�
	{
		vTaskDelete(client->hTaskWrite);	//ɾ��д����
		vTaskDelete(client->hTaskRead);	//ɾ��д����
	}
	else if (nowTask == client->hTaskRead)	//�ȹر�����һ��
	{
		vTaskDelete(client->hTaskWrite);	//ɾ��д����
	}
	else
	{
		vTaskDelete(client->hTaskRead);	//ɾ��д����
	}

	close(client->nSocketHandle);		//�ر�����
	vQueueDelete(client->hClientQueue);
	os_free(client);					//�ͷ�����

	if (nowTask != NULL)
	{
		vTaskDelete(nowTask);
	}
}

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
	SOCKET_CLIENT_T *client = pvParameters;
	uint8_t *rxBuf;
	rxBuf = os_malloc(SCOKET_SERVER_RECEIVE_BUFF_LENGTH);  //�����ڴ�
	SOCKET_SERVER_DATA_ITEM_T sData;

	while (1)
	{
		int length = read(client->nSocketHandle, rxBuf, SCOKET_SERVER_RECEIVE_BUFF_LENGTH);	//��������
		if (length > 0)
		{
			sData.pData = rxBuf;
			sData.unLength = length;
			SIMPLE_EMIT(client->hSignalRx, &sData);
		}
		else
		{
			os_free(rxBuf);
			SocketServer_CloseClient(client, client->hTaskRead);
			break;
		}
	}
	os_free(rxBuf);
	vTaskDelete(NULL);	//ɾ������
}

/******************************************************************
* @����˵����   ���տͻ��˷��͹��������ݣ�������ն���
Ϊ�˷�ֹ����ͻ���ͬʱд������ʱ�����������ݰ�˳��
Ҫ�ȴ������ͻ���д��������д��
* @���������   void *pvParameters
* @���ز�����
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
static void SocketServer_WriteClient(void *pvParameters)
{
	CheckNull(pvParameters);
	SOCKET_CLIENT_T *client = pvParameters;

	SOCKET_SERVER_DATA_ITEM_T sData;

	while (1)
	{
		if (xQueueReceive(client->hClientQueue, &sData, portMAX_DELAY) == pdTRUE)
		{
			int result = write(client->nSocketHandle, sData.pData, sData.unLength);
			os_free(sData.pData);

			if (result != sData.unLength)
			{
				SocketServer_CloseClient(client, client->hTaskWrite);
				break;
			}
		}
	}
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
			uint8 i;
			for (i = 0; i < SOCKET_SERVER_MAXIMUN_CONNECTION; i++)
			{
				if (server->paClientTable[i] == 0)
				{
					SOCKET_CLIENT_T *client = os_malloc(sizeof(SOCKET_CLIENT_T));
					server->paClientTable[i] = client;
					client->hSignalRx = server->hSignalRx;
					client->hClientQueue = xQueueCreate(32, sizeof(SOCKET_SERVER_DATA_ITEM_T));
					xTaskCreate(SocketServer_ReveiveClient, (signed char *)"ReveiveClient", 128, client, 5, NULL);
					xTaskCreate(SocketServer_WriteClient, (signed char *)"WriteClient", 128, client, 5, NULL);
					break;
				}
			}
			if (i >= SOCKET_SERVER_MAXIMUN_CONNECTION)	//�ͻ��������ﵽ����
			{
				close(clientHandle);
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
	//server->pReading = 0;
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
	SOCKET_SERVER_DATA_ITEM_T dataItem;

	uint8_t i;

	for (i = 0; i < SOCKET_SERVER_MAXIMUN_CONNECTION; i++)
	{
		if (server->paClientTable[i] != 0)
		{
			dataItem.pData = pData;
			dataItem.unLength = unLength;
			if (NMIIrqIsOn)
			{
				portBASE_TYPE  worken;
				xQueueSendFromISR(server->paClientTable[i]->hClientQueue, &dataItem, &worken);
			}
			else
			{
				xQueueSend(server->paClientTable[i]->hClientQueue, &dataItem, 1000);
			}
		}
	}
	return 1;
}

/******************************************************************
* @����˵����   ע����յ����ݵĻص�����
* @���������   IMPLE_SOLTS slots,         ��
void *pArg                �ص�ʱ�Ĳ���
* @���������   ��
* @���ز�����   ErrorStatus
* @�޸ļ�¼��   ----
******************************************************************/
ErrorStatus SocketServer_RegisterRxCB(SOCKET_SERVER_HANDLE_T hHadnle, SIMPLE_SOLTS slots, void *pArg)
{
	if (hHadnle == NULL)
	{
		return ERROR;
	}
	SOCKET_SERVER_T *server = hHadnle;
	return SimpleSignalSolts_Connect(&server->hSignalRx, slots, pArg);
}