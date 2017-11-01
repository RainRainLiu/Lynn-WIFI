#include "spi_interface.h"
#include "spi_register.h"
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



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

void ICACHE_FLASH_ATTR spi_slave_test()
{
	// SPI initialization configuration, speed = 0 in slave mode
	SpiAttr hSpiAttr;
	hSpiAttr.bitOrder = SpiBitOrder_MSBFirst;
	hSpiAttr.speed = SpiSpeed_20MHz;
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
	// Set spi interrupt information.
	//SpiIntInfo spiInt;
	//spiInt.src = (SpiIntSrc_TransDone
	//	| SpiIntSrc_WrStaDone
	//	| SpiIntSrc_RdStaDone
	//	| SpiIntSrc_WrBufDone
	//	| SpiIntSrc_RdBufDone);
	//spiInt.isrFunc = spi_slave_isr_sta;
	//SPIIntCfg(SpiNum_HSPI, &spiInt);
	// SHOWSPIREG(SpiNum_HSPI);
	SPISlaveRecvData(SpiNum_HSPI);
	uint32_t sndData[8] = { 0 };
	sndData[0] = 0x35343332;
	sndData[1] = 0x39383736;
	sndData[2] = 0x3d3c3b3a;
	sndData[3] = 0x11103f3e;
	sndData[4] = 0x15141312;
	sndData[5] = 0x19181716;
	sndData[6] = 0x1d1c1b1a;
	sndData[7] = 0x21201f1e;
	// write 8 word (32 byte) data to SPI buffer W8~W15
	SPISlaveSendData(SpiNum_HSPI, sndData, 8);
	// set the value of status register
	WRITE_PERI_REG(SPI_RD_STATUS(SpiNum_HSPI), 0x8A);
	WRITE_PERI_REG(SPI_WR_STATUS(SpiNum_HSPI), 0x83);
}


// SPI interrupt callback function.
void spi_slave_isr_sta(void *para)
{
	uint32 regvalue;
	uint32 statusW, statusR, counter;
	if (READ_PERI_REG(0x3ff00020)&BIT4) {
		//following 3 lines is to clear isr signal
		CLEAR_PERI_REG_MASK(SPI_SLAVE(SpiNum_SPI), 0x3ff);
	}
	else if (READ_PERI_REG(0x3ff00020)&BIT7) { //bit7 is for hspi isr,
			regvalue = READ_PERI_REG(SPI_SLAVE(SpiNum_HSPI));
		os_printf("spi_slave_isr_sta SPI_SLAVE[0x%08x]\n\r",
			regvalue);
		SPIIntClear(SpiNum_HSPI);
		SET_PERI_REG_MASK(SPI_SLAVE(SpiNum_HSPI), SPI_SYNC_RESET);
		SPIIntClear(SpiNum_HSPI);
		//SPIIntEnable(SpiNum_HSPI, SpiIntSrc_WrStaDone
		//	| SpiIntSrc_RdStaDone
		//	| SpiIntSrc_WrBufDone
		//	| SpiIntSrc_RdBufDone);
		if (regvalue & SPI_SLV_WR_BUF_DONE) {
			// User can get data from the W0~W7
			os_printf("spi_slave_isr_sta : SPI_SLV_WR_BUF_DONE\n\r");
		}
		else if (regvalue & SPI_SLV_RD_BUF_DONE) {
			// TO DO
			os_printf("spi_slave_isr_sta : SPI_SLV_RD_BUF_DONE\n\r");
		}
		if (regvalue & SPI_SLV_RD_STA_DONE) {
			statusR = READ_PERI_REG(SPI_RD_STATUS(SpiNum_HSPI));
			statusW = READ_PERI_REG(SPI_WR_STATUS(SpiNum_HSPI));
			os_printf("spi_slave_isr_sta : SPI_SLV_RD_STA_DONE[R = 0x%08x, W = 0x%08x]\n\r", 
				statusR, statusW);
		}
		if (regvalue & SPI_SLV_WR_STA_DONE) {
			statusR = READ_PERI_REG(SPI_RD_STATUS(SpiNum_HSPI));
			statusW = READ_PERI_REG(SPI_WR_STATUS(SpiNum_HSPI));
			os_printf("spi_slave_isr_sta :SPI_SLV_WR_STA_DONE[R = 0x%08x, W = 0x%08x]\n\r", statusR, statusW);
		}
		if ((regvalue & SPI_TRANS_DONE) && ((regvalue & 0xf) == 0)) {
			os_printf("spi_slave_isr_sta : SPI_TRANS_DONE\n\r");
		}
		SHOWSPIREG(SpiNum_HSPI);
	}
}