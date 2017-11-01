#include "SocketServer.h"
#include "MyDefine.h"
#include "user_config.h"

#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"



typedef struct
{
	int nServerHandle; 	//服务端句柄
	int anClientHandleTalbe[SOCKET_SERVER_MAXIMUN_CONNECTION];	//客户端句柄列表

	SOCKET_SERVER_RECEIVE_CB ReceiveCB;
	void *pArgCB;
}SOCKET_SERVER_T;


typedef struct
{
	int *pnSocketHandle;
	SOCKET_SERVER_T *hServer;
}SOCKET_RECEIVE_T;

/******************************************************************
* @函数说明：   进程
* @输入参数：   void *pvParameters
* @返回参数：
* @修改记录：   2017/10/28 初版
******************************************************************/
static void SocketServer_ReveiveClient(void *pvParameters)
{
	CheckNull(pvParameters);

	SOCKET_RECEIVE_T *receive = pvParameters;

	int socketHandle = *receive->pnSocketHandle;
	uint8_t *rxBuf;
	rxBuf = os_malloc(SCOKET_SERVER_RECEIVE_BUFF_LENGTH);  //分配内存

	while (1)
	{
		int length = read(socketHandle, rxBuf, SCOKET_SERVER_RECEIVE_BUFF_LENGTH);	//接收数据
		if (length > 0)
		{
			receive->hServer->ReceiveCB(receive->hServer->pArgCB, rxBuf, length);	//接收完成
		}
		else
		{
			close(socketHandle);
			receive->pnSocketHandle = 0;
			os_printf("read error %d\r\n", length);
			break;
		}
	}
	os_free(rxBuf);
	os_free(receive);
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

/******************************************************************
* @函数说明：   构造函数
* @输入参数：   int port			服务器监听端口号
* @返回参数：   SOCKET_SERVER_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
******************************************************************/
SOCKET_SERVER_HANDLE_T SocketServer_Create(int iListenPort, SOCKET_SERVER_RECEIVE_CB ReceiveCB, void *pvParameters)
{
	SOCKET_SERVER_T *server = os_malloc(sizeof(SOCKET_SERVER_T));

	memset(server, 0, sizeof(SOCKET_SERVER_T));
	if (SocketServer_CreateServer(&server->nServerHandle, iListenPort) == false)	//创建服务器监听端口
	{
		os_free(server);
		return NULL;
	}

	server->ReceiveCB = ReceiveCB;
	server->pArgCB = pvParameters;

	xTaskCreate(SocketServer_Task, (signed char *)"SoeketServer", 256, server, 4, NULL);

}