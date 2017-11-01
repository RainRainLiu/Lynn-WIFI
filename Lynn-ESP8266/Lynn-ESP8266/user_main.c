#ifdef __cplusplus
extern "C"
{
#endif
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "SocketServer.h"
#include "user_config.h"
#include "MyDefine.h"
void user_init(void);
#ifdef __cplusplus
}
#endif

#ifdef ESP8266_GDBSTUB
#include <gdbstub.h>
#endif




static void Socket_RxCB(void *pvParameters, uint8 *pData, uint32 unLength)
{
	os_printf("Socket RX %d\r\n", unLength);
}



void ServerTask(void *pvParameters)
{
	while (1)
	{
		vTaskDelay(1000);
		
	}
}

void dhcps_lease_test(void)
{
	struct dhcps_lease dhcp_lease;
	IP4_ADDR(&dhcp_lease.start_ip, 192, 168, 123, 100);
	IP4_ADDR(&dhcp_lease.end_ip, 192, 168, 123, 105);
	wifi_softap_set_dhcps_lease(&dhcp_lease);
}



void RAMFUNC user_init(void)
{
#ifdef ESP8266_GDBSTUB
	gdbstub_init();
#endif

	os_printf("\r\n******************************************\r\n");
	os_printf("SDK version:%s\n", system_get_sdk_version());
	os_printf("Chip ID:%x\r\n", system_get_chip_id());
	os_printf("CPU frequency：%d\r\n", system_get_cpu_freq());
	os_printf("\r\n******************************************\r\n");

	system_phy_set_max_tpw(WIFI_TX_POWER);	//设置发射功率


	struct ip_info info;
	struct softap_config *config = os_malloc(sizeof(struct softap_config));
	wifi_softap_get_config(config);
	config->authmode = AUTH_WPA_WPA2_PSK;   //加密模式
	config->beacon_interval = 100;          //广播间隔
	config->channel = 11;
	config->max_connection = 4;
	memset(config->ssid, 0, 32);
	sprintf(config->ssid, "Lynn");
	sprintf(config->password, "hll500520");
	wifi_softap_set_config(config);
	os_free(config);
	wifi_set_opmode(SOFTAP_MODE);
	
	wifi_softap_dhcps_stop();
	IP4_ADDR(&info.ip, 192, 168, 123, 1);
	IP4_ADDR(&info.gw, 192, 168, 123, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	wifi_set_ip_info(SOFTAP_IF, &info);
	dhcps_lease_test();
	wifi_softap_dhcps_start();



	os_printf("statrt\r\n");

	SocketServer_Create(6666, Socket_RxCB, NULL);


	xTaskCreate(ServerTask, (signed char *)"Server", 256, NULL, 2, NULL);
}

