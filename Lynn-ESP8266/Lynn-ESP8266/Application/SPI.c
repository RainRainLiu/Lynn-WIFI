#include "gpio.h"
#include "spi.h"
#include "spi_interface.h"
#include "spi_register.h"
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "MyDefine.h"

 
//发送数据的实体
typedef struct
{
	uint8 *pData;
	uint32 unLength;
}SPI_DATA_ITEM_T;

typedef struct
{
	xTaskHandle		hThread;
	xQueueHandle	rxQueue;
	xQueueHandle	txQueue;
	uint8_t			flagWriting;	//正在写入标记
}SPI_T;


/******************************************************************
* @函数说明：   写入数据到SPI寄存器，并通知主机读取数据，
				固定长度为32byte
* @输入参数：   SPI_HANDLE_T hHandle, 句柄
				uint32_t *pData			数据指针
* @返回参数：   无
* @修改记录：   
******************************************************************/
void SPI_SendData(SPI_HANDLE_T hHandle, uint32_t *pData)
{
	SPI_T *spi = hHandle;

	SPISlaveSendData(SpiNum_HSPI, pData, 8);	//写入数据到缓冲区
	GPIO_OUTPUT(GPIO_Pin_5, 0);					//通知主机读取
	spi->flagWriting = 1;						//正在写入标志
}

// SPI interrupt callback function.
void SPI_ISR(void *para)
{
	SPI_T *spi = para;
	uint32_t data[SPI_DATA_ITEM_LENGTH / 4];
	
	portBASE_TYPE xHigherPriorityTaskWoken;

	uint32 regvalue;
	uint32 statusW, statusR, counter;
	if (READ_PERI_REG(0x3ff00020)&BIT4) {
		//following 3 lines is to clear isr signal
		CLEAR_PERI_REG_MASK(SPI_SLAVE(SpiNum_SPI), 0x3ff);
	}
	else if (READ_PERI_REG(0x3ff00020)&BIT7) 
	{ //bit7 is for hspi isr,
		regvalue = READ_PERI_REG(SPI_SLAVE(SpiNum_HSPI));
		os_printf("spi_slave_isr_sta SPI_SLAVE[0x%08x]\n\r", regvalue);
		SPIIntClear(SpiNum_HSPI);
		SET_PERI_REG_MASK(SPI_SLAVE(SpiNum_HSPI), SPI_SYNC_RESET);
		SPIIntClear(SpiNum_HSPI);
		SPIIntEnable(SpiNum_HSPI, SpiIntSrc_RdBufDoneEn | SpiIntSrc_WrBufDoneEn);	//开启读写数据的中断
		if (regvalue & SPI_SLV_WR_BUF_DONE) 
		{
			if (spi != NULL)
			{
				uint8_t i;
				for (i = 0; i < SPI_DATA_ITEM_LENGTH; i += 4)// uint32_t 格式读取
				{
					data[i] = READ_PERI_REG(SPI_W0(SpiNum_HSPI) + i);
				}
				xQueueSendFromISR(spi->rxQueue, data, &xHigherPriorityTaskWoken);	//发送到缓存区
			}
			// User can get data from the W0~W7
			os_printf("spi_slave_isr_sta : SPI_SLV_WR_BUF_DONE\n\r");
		}
		else if (regvalue & SPI_SLV_RD_BUF_DONE) 
		{
			if (spi != NULL)
			{
				xQueueReceiveFromISR(spi->txQueue, data, &xHigherPriorityTaskWoken);
				if (xHigherPriorityTaskWoken == pdTRUE)		//读取成功
				{
					SPI_SendData(spi, data);
				}
				else
				{
					GPIO_OUTPUT(GPIO_Pin_5, 1);	//没有数据了
					spi->flagWriting = 0;		//空闲
				}
			}
			// TO DO
			os_printf("spi_slave_isr_sta : SPI_SLV_RD_BUF_DONE\n\r");
		}
	}
}


/**
* @brief Configurate slave prepare for receive data.
*
*/
int ICACHE_FLASH_ATTR SPI_SlaveRecvData(SpiNum spiNum, void(*isrFunc)(void*), void *pArg)
{
	if ((spiNum > SpiNum_HSPI)) {
		return -1;
	}

	SPIIntEnable(SpiNum_HSPI, SpiIntSrc_WrStaDoneEn
		| SpiIntSrc_RdStaDoneEn | SpiIntSrc_WrBufDoneEn | SpiIntSrc_RdBufDoneEn);
	SPIIntDisable(SpiNum_HSPI, SpiIntSrc_TransDoneEn);

	// Maybe enable slave transmission liston
	SET_PERI_REG_MASK(SPI_CMD(spiNum), SPI_USR);
	//
	_xt_isr_attach(ETS_SPI_INUM, isrFunc, pArg);
	// Enable isr
	_xt_isr_unmask(1 << ETS_SPI_INUM);
	return 0;
}


void SPI_InitHardware()
{
	SpiAttr hSpiAttr;
	hSpiAttr.bitOrder = SpiBitOrder_MSBFirst;
	hSpiAttr.speed = SpiSpeed_20MHz;
	hSpiAttr.mode = SpiMode_Slave;
	hSpiAttr.subMode = SpiSubMode_0;
	// Init HSPI GPIO
	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_HSPIQ_MISO);//configure io to spi mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_HSPID_MOSI);//configure io to spi mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_HSPI_CLK);//configure io to spi mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_HSPI_CS0);//configure io to spi mode

	GPIO_ConfigTypeDef GPIO;

	GPIO.GPIO_Pin = GPIO_Pin_5;
	GPIO.GPIO_Mode = GPIO_Mode_Output;               //输入模式
	GPIO.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;     //中断为双边沿触发
	GPIO.GPIO_Pullup = GPIO_PullUp_EN;              //使能上拉

	//gpio_config();
	gpio_config(&GPIO);
	GPIO_OUTPUT(GPIO_Pin_5, 1);

	os_printf("\r\n ============= spi init slave =============\r\n");
	SPIInit(SpiNum_HSPI, &hSpiAttr);

	SPIIntEnable(SpiNum_HSPI, SpiIntSrc_RdBufDoneEn | SpiIntSrc_WrBufDoneEn);	//开启读写数据的中断
}

/******************************************************************
* @函数说明：   进程
* @输入参数：   void *pvParameters
* @返回参数：
* @修改记录：   2017/10/28 初版
******************************************************************/
static void SPI_Task(void *pvParameters)
{
	CheckNull(pvParameters);
	SPI_T *spi = pvParameters;
	
	while (1)
	{
		vTaskDelay(100);
		if (uxQueueMessagesWaiting(spi->txQueue) != 0 && spi->flagWriting == 0)	//检测错误
		{
			uint8_t *pData = os_malloc(SPI_DATA_ITEM_LENGTH);
			os_printf("\r\n************** SPI ERROR*********\r\n");
			xQueueReceive(spi->txQueue, pData, 100);
			SPI_SendData(spi, (uint32_t*)pData);
			os_free(pData);
		}
	}
}

/******************************************************************
* @函数说明：   构造函数
* @输入参数：   无
* @返回参数：   SPI_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
******************************************************************/
SPI_HANDLE_T SPI_Create()
{
	SPI_T *spi = os_malloc(sizeof(SPI_T));

	CheckNull(spi);
	SPI_InitHardware();
	SPI_SlaveRecvData(SpiNum_HSPI, SPI_ISR, spi);

	spi->txQueue = xQueueCreate(256, SPI_DATA_ITEM_LENGTH);	//256 * 32 = 16K的数据
	spi->rxQueue = xQueueCreate(256, SPI_DATA_ITEM_LENGTH);	//256 * 32 = 16K的数据

	xTaskCreate(SPI_Task, (signed char *)"SPI_Task", 256, spi, 4, NULL);
	return spi;
}

/******************************************************************
* @函数说明：   SPI 写入数据
* @输入参数：   SPI_HANDLE_T hHandle, 句柄
				uint8 *pData,	数据
				uint32 unLength	数据长度
* @返回参数：   无
* @修改记录：   2017/10/28 初版
******************************************************************/
int SPI_Write(SPI_HANDLE_T hHandle, uint8 *pData, uint32 unLength)
{
	CheckNull(hHandle);
	SPI_T *spi = hHandle;

	uint32_t i = 0;
	
	taskENTER_CRITICAL();
	if (spi->flagWriting == 0)	//SPI没有正在发送数据，则立即发送
	{
		if (uxQueueMessagesWaiting(spi->txQueue) == 0)	//这里应该是启动发送，队列中有数据应该在中断中再次发送//队列中有数据则发送队列中的，否则发送本次最前面的数据
		{
			i += SPI_DATA_ITEM_LENGTH;				//前面32个字节不加入队列，下面直接发送
			SPI_SendData(spi, (uint32_t*)pData);
		}
		else
		{
			os_printf("\r\n************** SPI ERROR*********\r\n");
		}
	}
	for (; i < unLength; i += SPI_DATA_ITEM_LENGTH)	//加入队列
	{
		if (!xQueueSend(spi->txQueue, (uint32_t*)&pData[i], 100))	//出错
		{
			break;
		}
	}
	taskEXIT_CRITICAL();
	return i;
}


/******************************************************************
* @函数说明：   SPI 读取数据，阻塞式
* @输入参数：   SPI_HANDLE_T hHandle, 句柄
				uint8 *pBuff,	数据缓存区
				uint32 unLength	数据缓存区长度	不可小于SPI_DATA_ITEM_LENGTH
* @返回参数：   返回的数据长度
* @修改记录：   2018/1/6 初版
******************************************************************/
uint32_t SPI_Read(SPI_HANDLE_T hHandle, uint8 *pBuff, uint32 unLength)
{
	CheckNull(hHandle);
	if (unLength < SPI_DATA_ITEM_LENGTH)
	{
		return -1;
	}

	SPI_T *spi = hHandle;

	if (xQueueReceive(spi->rxQueue, pBuff, portMAX_DELAY))
	{
		return SPI_DATA_ITEM_LENGTH;
	}
	else
	{
		return -1;
	}
}