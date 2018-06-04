#include "UartProcess.h"
#include "esp_common.h"
#include "MyDefine.h"
#include "user_config.h"
#include "uart_register.h"
#include "uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "SimpleSignalSolts.h"

typedef struct
{
	SIMPLE_SIGNAL               hSignalRx;          //���յ����ݵ��ź�
	xQueueHandle				hRxQueue;
	UART_Port					uart_no;
}UART_PROCESS_T;

LOCAL void uart_tx_one_char(uint8 uart, uint8 TxChar)
{
	while (true) {
		uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);

		if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
			break;
		}
	}

	WRITE_PERI_REG(UART_FIFO(uart), TxChar);
	return;
}

/******************************************************************
* @�������ƣ�   Uart0_rx_intr_handler
* @����˵����   �����жϽ��մ���
* @���������   ��
* @���ز�����   ��
* @�޸ļ�¼��   2016/12/12/ ����
******************************************************************/
void Uart0_rx_intr_handler(void *para)
{
	uint8 RcvChar;
	uint8 uart_no = UART0;//UartDev.buff_uart_no;
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
		if (para != NULL)
		{
			UART_PROCESS_T *uartProcess = para;
			portBASE_TYPE  worken;
			xQueueSendFromISR(uartProcess->hRxQueue, &RcvChar, &worken);
		}
		uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no));
	}
}



/******************************************************************
* @�������ƣ�   Uart0_Init_App
* @����˵����   ���ڳ�ʼ��
* @���������   UART_Port uart_no,	�˿ں�
				uint32 nBaudRate,	������
				void *pArgCB		�жϻص�ʱ�Ĳ�������
* @���ز�����   ��
* @�޸ļ�¼��   
******************************************************************/
void  Uart0_Init_App(UART_Port uart_no, uint32 nBaudRate, void *pArgCB)
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
	UART_ParamConfig(uart_no, &uart_config);

	UART_IntrConfTypeDef uart_intr;
	uart_intr.UART_IntrEnMask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
	uart_intr.UART_RX_FifoFullIntrThresh = 10;
	uart_intr.UART_RX_TimeOutIntrThresh = 2;
	uart_intr.UART_TX_FifoEmptyIntrThresh = 20;
	UART_IntrConfig(uart_no, &uart_intr);

	UART_intr_handler_register(Uart0_rx_intr_handler, pArgCB);
	ETS_UART_INTR_ENABLE();
	
}

/******************************************************************
* @����˵����   ����
* @���������   void *pvParameters
* @���ز�����
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
static void UartProcess_Task(void *pvParameters)
{
	UART_PROCESS_T *uartProcess = pvParameters;
	uint8 data;
	while (1)
	{
		if (xQueueReceive(uartProcess->hRxQueue, &data, portMAX_DELAY) == pdTRUE)
		{
			SIMPLE_EMIT(uartProcess->hSignalRx, &data);
		}
	}
}
/******************************************************************
* @����˵����   ����
* @���������  UART_Port uart_no  ʹ�õ�UART�˿ں� 
* @���ز�����
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
UART_PROCESS_HANDLE_T UartProcess_Create(UART_Port uart_no, uint32 unBaudRate)
{
	UART_PROCESS_T *uartProcess = os_malloc(sizeof(UART_PROCESS_T));
	uartProcess->hRxQueue = xQueueCreate(1, 64);

	Uart0_Init_App(uart_no, unBaudRate, uartProcess);

	xTaskCreate(UartProcess_Task, (signed char *)"UartProcess", 256, uartProcess, 4, NULL);

	return uartProcess;
}

/******************************************************************
* @����˵����   ע����յ����ݵĻص�����
* @���������   IMPLE_SOLTS slots,         ��
				void *pArg                �ص�ʱ�Ĳ���
* @���������   ��
* @���ز�����   ErrorStatus 
* @�޸ļ�¼��   ----
******************************************************************/
ErrorStatus UartProcess_RegisterRxCB(UART_PROCESS_HANDLE_T hHadnle, SIMPLE_SOLTS slots, void *pArg)
{
	if (hHadnle == NULL)
	{
		return ERROR;
	}
	UART_PROCESS_T *uartProcess = hHadnle;
	return SimpleSignalSolts_Connect(&uartProcess->hSignalRx, slots, pArg);
}
/******************************************************************
* @����˵����   ����д�����ݣ�����ʽ
* @���������   UART_PROCESS_HANDLE_T hHandle ���
				uint8_t *pData, ����ָ��
				uint32_t unLength	���ݳ���
* @���ز�����   int д�볤��
* @�޸ļ�¼��   2017/10/28 ����
******************************************************************/
int UartProcess_Write(UART_PROCESS_HANDLE_T hHandle, uint8_t *pData, uint32_t unLength)
{
	CheckNull(hHandle);
	UART_PROCESS_T *uartProcess = hHandle;
	uint32_t i;
	for (i = 0; i < unLength; i++)
	{
		uart_tx_one_char(uartProcess->uart_no, pData[i]);
	}
	
	return unLength;
}