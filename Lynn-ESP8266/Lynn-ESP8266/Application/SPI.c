#include "spi.h"
#include "spi_interface.h"
#include "spi_register.h"
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "MyDefine.h"


typedef struct
{
	xQueueHandle rxQueue;
	xQueueHandle txQueue;
}SPI_T;


// SPI interrupt callback function.
void SPI_ISR(void *para)
{
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
			// User can get data from the W0~W7
			os_printf("spi_slave_isr_sta : SPI_SLV_WR_BUF_DONE\n\r");
		}
		else if (regvalue & SPI_SLV_RD_BUF_DONE) 
		{
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
	hSpiAttr.speed = 0;
	hSpiAttr.mode = SpiMode_Slave;
	hSpiAttr.subMode = SpiSubMode_0;
	// Init HSPI GPIO
	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);//configure io to spi mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);//configure io to spi mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);//configure io to spi mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);//configure io to spi mode
	os_printf("\r\n ============= spi init slave =============\r\n");
	SPIInit(SpiNum_HSPI, &hSpiAttr);

	SPIIntEnable(SpiNum_HSPI, SpiIntSrc_RdBufDoneEn | SpiIntSrc_WrBufDoneEn);	//开启读写数据的中断
}


/******************************************************************
* @函数说明：   构造函数
* @输入参数：   int port			服务器监听端口号
* @返回参数：   SOCKET_SERVER_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
******************************************************************/
SPI_HANDLE_T SPI_Create()
{
	SPI_T *spi = os_malloc(sizeof(SPI_T));

	CheckNull(spi);
	SPI_InitHardware();
	SPI_SlaveRecvData(SpiNum_HSPI, SPI_ISR, spi);

	return spi;
}


void SPI_Write(SPI_HANDLE_T hHandle, uint8 *pData, uint32 unLength)
{
	CheckNull(hHandle);


}
