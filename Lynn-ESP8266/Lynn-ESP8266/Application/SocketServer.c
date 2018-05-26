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
	int				hTaskRead;
	int				hTaskWrite;
}SOCKET_CLIENT_T;

typedef struct
{
	int nServerHandle; 	//服务端句柄
	xQueueHandle hClientQueue;
	xTimerHandle hRxTimer;	//接收数据的定时器，定时器超时才可以往缓存区写入数据
	SIMPLE_SIGNAL	hSignalRx;
	SOCKET_CLIENT_T	*paClientTable[SOCKET_SERVER_MAXIMUN_CONNECTION];
}SOCKET_SERVER_T;





/******************************************************************
* @函数说明：   接收客户端发送过来的数据，放入接收队列
为了防止多个客户端同时写入数据时，打乱了数据包顺序
要等待其它客户端写完数据再写入
* @输入参数：   void *pvParameters
* @返回参数：
* @修改记录：   2017/10/28 初版
******************************************************************/
static void SocketServer_ReveiveClient(void *pvParameters)
{
	CheckNull(pvParameters);
	SOCKET_CLIENT_T *client = pvParameters;
	uint8_t *rxBuf;
	rxBuf = os_malloc(SCOKET_SERVER_RECEIVE_BUFF_LENGTH);  //分配内存
	SOCKET_SERVER_DATA_ITEM_T sData;

	while (1)
	{
		int length = read(client->nSocketHandle, rxBuf, SCOKET_SERVER_RECEIVE_BUFF_LENGTH);	//接收数据
		if (length > 0)
		{
			sData.pData = rxBuf;
			sData.unLength = length;
			SIMPLE_EMIT(client->hSignalRx, &sData);
		}
		else
		{
			vTaskDelete(client->hTaskWrite);	//删除写进程
			close(client->nSocketHandle);		//关闭连接
			vQueueDelete(client->hClientQueue);
			os_free(client);					//释放自身
			break;
		}
	}
	os_free(rxBuf);
	vTaskDelete(NULL);	//删除自身
}

/******************************************************************
* @函数说明：   接收客户端发送过来的数据，放入接收队列
为了防止多个客户端同时写入数据时，打乱了数据包顺序
要等待其它客户端写完数据再写入
* @输入参数：   void *pvParameters
* @返回参数：
* @修改记录：   2017/10/28 初版
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
				vTaskDelete(client->hTaskRead);		//删除读取进程
				close(client->nSocketHandle);		//关闭连接
				vQueueDelete(client->hClientQueue);
				os_free(client);					//释放自身
				break;
			}
		}
	}
	vTaskDelete(NULL);	//删除自身
}

/******************************************************************
  * @函数名称：   COM_Socket_CreatServer
  * @函数说明：   创建服务器
  * @输入参数：   int *sockte_handle   socket句柄返回
  * 			  int port			服务器端口号
  * @返回参数：   无 
  * @修改记录：   2016/11/29 初版
******************************************************************/
static bool SocketServer_CreateServer(int *sockte_handle, int port)
{
	int sta_socket = socket(PF_INET, SOCK_STREAM, 0); 	//创建socket
	
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
* @函数说明：   进程
* @输入参数：   void *pvParameters
* @返回参数：   
* @修改记录：   2017/10/28 初版
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

		int clientHandle = accept(server->nServerHandle, (struct sockaddr*)&client_addr, &length);	//等待连接

		os_printf("clientHandle %d\r\n", clientHandle);
		if (clientHandle > 0)
		{
			SOCKET_CLIENT_T *client = os_malloc(sizeof(SOCKET_CLIENT_T));

			client->hSignalRx = server->hSignalRx;
			client->hClientQueue = xQueueCreate(32, sizeof(SOCKET_SERVER_DATA_ITEM_T));
			xTaskCreate(SocketServer_ReveiveClient, (signed char *)"ReveiveClient", 128, client, 5, NULL);
			xTaskCreate(SocketServer_WriteClient, (signed char *)"WriteClient", 128, client, 5, NULL);
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
* @函数说明：   构造函数
* @输入参数：   int port			服务器监听端口号
* @返回参数：   SOCKET_SERVER_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
******************************************************************/
SOCKET_SERVER_HANDLE_T SocketServer_Create(int iListenPort)
{
	SOCKET_SERVER_T *server = os_malloc(sizeof(SOCKET_SERVER_T));

	memset(server, 0, sizeof(SOCKET_SERVER_T));
	if (SocketServer_CreateServer(&server->nServerHandle, iListenPort) == false)	//创建服务器监听端口
	{
		os_free(server);
		return NULL;
	}

	server->hRxTimer = xTimerCreate("SoeketServer", 10, false, server, SocketServer_TimerCB);
	xTaskCreate(SocketServer_Task, (signed char *)"SoeketServer", 256, server, 4, NULL);
	return server;
}

/******************************************************************
* @函数说明：   向客户端写入数据
* @输入参数：   SOCKET_SERVER_HANDLE_T hHandle 句柄
			    uint8_t *pData, 数据指针
			    uint32_t unLength	数据长度
* @返回参数：   SOCKET_SERVER_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
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