#ifdef __cplusplus
extern "C"
{
#endif
	
#include <esp_common.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <pin_mux_register.h>
#include <gpio.h>
#include <uart.h>
	
void user_init(void);
	
#ifdef __cplusplus
}
#endif

#ifdef ESP8266_GDBSTUB
#include <gdbstub.h>
#endif

#define RAMFUNC __attribute__((section(".entry.text")))
	
static void RAMFUNC LEDBlinkTask(void *pvParameters)
{
	for (int tick = 0;;tick++)
	{
		vTaskDelay(300 / portTICK_RATE_MS);
		gpio_output_conf(0, BIT1, BIT1, 0);
		vTaskDelay(300 / portTICK_RATE_MS);
		gpio_output_conf(BIT1, 0, BIT1, 0);
	}
}

/******************************************************************
* @函数名称：   Uart0_rx_intr_handler
* @函数说明：   串口中断接收处理
* @输入参数：   无
* @返回参数：   无
* @修改记录：   2016/12/12/ 初版
******************************************************************/
LOCAL void Uart0_rx_intr_handler(void *para)
{
	uint8 RcvChar;
	uint8 uart_no = UART0; //UartDev.buff_uart_no;
	uint8 fifo_len = 0;
	uint8 buf_idx = 0;
	uint8 fifo_tmp[128] = { 0 };

	uint32 uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no));

	while (uart_intr_status != 0x0) {
		if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST)) {
			os_printf("FRM_ERR\r\n");
			WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
		}
		else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) {
			//os_printf("full\r\n");

			WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
		}
		else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST)) {
			//os_printf("tout\r\n");

			WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
		}
		else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST)) {
			os_printf("empty\n\r");
			WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TXFIFO_EMPTY_INT_CLR);
			CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
		}
		else {
			//skip
		}

		RcvChar = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;

		//UartData_ParseAndSendMsg(RcvChar);
		uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no));
	}
}
/******************************************************************
* @函数名称：   Uart0_Init_App
* @函数说明：   串口初始化
* @输入参数：   无
* @返回参数：   无
* @修改记录：   2016/12/12/ 初版
******************************************************************/
void  Uart0_Init_App(uint32 nBaudRate)
{
	UART_WaitTxFifoEmpty(UART0);

	UART_ConfigTypeDef uart_config;
	uart_config.baud_rate = nBaudRate;
	uart_config.data_bits = UART_WordLength_8b;
	uart_config.parity = USART_Parity_None;
	uart_config.stop_bits = USART_StopBits_1;
	uart_config.flow_ctrl = USART_HardwareFlowControl_None;
	uart_config.UART_RxFlowThresh = 120;
	uart_config.UART_InverseMask = UART_None_Inverse;
	UART_ParamConfig(UART0, &uart_config);

	UART_IntrConfTypeDef uart_intr;
	uart_intr.UART_IntrEnMask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
	uart_intr.UART_RX_FifoFullIntrThresh = 10;
	uart_intr.UART_RX_TimeOutIntrThresh = 2;
	uart_intr.UART_TX_FifoEmptyIntrThresh = 20;
	UART_IntrConfig(UART0, &uart_intr);

	//UART_SetPrintPort(UART0);
	UART_intr_handler_register(Uart0_rx_intr_handler, NULL);
	ETS_UART_INTR_ENABLE();
}

//Unless you explicitly define the functions as RAMFUNC, they will be placed in the SPI FLASH and the debugger
//won't be able to set software breakpoints there.
void RAMFUNC user_init(void)  
{
#ifdef ESP8266_GDBSTUB
	gdbstub_init();
#endif
	
	UART_SetPrintPort(UART1);
	
	Uart0_Init_App(115200);
	
	

#ifdef ESP8266_GDBSTUB
//#error The LED on the Olimex board is multiplexed with the TXD line used by the GDB stub. In order to use the stub, select a different LED pin below.
#endif
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
	gpio_output_conf(0, BIT1, BIT1, 0);

	xTaskCreate(LEDBlinkTask, (signed char *)"LEDBlinkTask", 256, NULL, 2, NULL);
}

