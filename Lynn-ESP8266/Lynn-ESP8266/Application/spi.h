#ifndef __SPI_H__
#define __SPI_H__

#include "c_types.h"

#define SPI_DATA_ITEM_LENGTH	32

typedef void* SPI_HANDLE_T;

/******************************************************************
* @函数说明：   构造函数
* @输入参数：   无
* @返回参数：   SPI_HANDLE_T 句柄
* @修改记录：   2017/10/28 初版
******************************************************************/
SPI_HANDLE_T SPI_Create();
/******************************************************************
* @函数说明：   SPI 写入数据
* @输入参数：   SPI_HANDLE_T hHandle, 句柄
uint8 *pData,	数据
uint32 unLength	数据长度
* @返回参数：   无
* @修改记录：   2017/10/28 初版
******************************************************************/
int SPI_Write(SPI_HANDLE_T hHandle, uint8 *pData, uint32 unLength);

/******************************************************************
* @函数说明：   SPI 读取数据，阻塞式
* @输入参数：   SPI_HANDLE_T hHandle, 句柄
uint8 *pBuff,	数据缓存区
uint32 unLength	数据缓存区长度	不可小于SPI_DATA_ITEM_LENGTH
* @返回参数：   返回的数据长度
* @修改记录：   2018/1/6 初版
******************************************************************/
uint32_t SPI_Read(SPI_HANDLE_T hHandle, uint8 *pBuff, uint32 unLength);

#endif