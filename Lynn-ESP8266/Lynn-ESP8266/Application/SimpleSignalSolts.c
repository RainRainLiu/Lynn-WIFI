/******************************************************************************
* @ File name --> SimpleSignalSolts.c
* @ Author    --> By@ LiuYu
* @ Version   --> V1.0
* @ Date      --> 2017 - 06 - 15
* @ Brief     --> 轻量级的信号与槽
* @           --> 一个信号对应多个槽，查表调用
*
* @ Copyright (C) 20**
* @ All rights reserved
*******************************************************************************
*
*                                  File Update
* @ Version   --> V1.1
* @ Author    --> By@ LiuYu
* @ Date      --> 2017 - 09 - 12
* @ Revise    --> 增加了槽的自定义参数，在回调槽时使用
* @           --> 
*
*
******************************************************************************
*
*                                  File Update
* @ Version   --> V1.2
* @ Author    --> By@ LiuYu
* @ Date      --> 2017 - 09 - 29
* @ Revise    --> 修复了信号连接第一个槽时参数无法正确存储的BUG
* @           --> 
*
*
******************************************************************************/
#include "SimpleSignalSolts.h"
#include <string.h>

//信号结构
typedef struct
{
    void            *signleID;    //信号的指针的指针，保存信号的指针，根据指针的地址确定是否是唯一
    SIMPLE_SOLTS    soltsTable[SIMPLE_SIGNAL_SOLTS_MAX_SOLTS];
    void *          soltsArg[SIMPLE_SIGNAL_SOLTS_MAX_SOLTS];
    uint8_t         soltsCount;
}SIMPLE_SIGNAL_T;
//信号表结构
typedef struct
{
    SIMPLE_SIGNAL_T signalTable[SIMPLE_SIGNAL_SOLTS_MAX_SIGNAL];
    uint8_t         signalCount;
}SIMPLE_SIGNAL_SOLTS_HANDLE_T;



SIMPLE_SIGNAL_SOLTS_HANDLE_T handle = 
{
    .signalCount = 0,
};


static void SimpleSignalSolts_Signal(void *signal, void *pArg)
{
    uint8_t i, j;
    
    
    for (i = 0; i < handle.signalCount; i++) //查找是否是同一个信号
    {
        if (handle.signalTable[i].signleID == signal)  //这里注意，signleID保存的是指针的地址，
        {
            for (j = 0; j < handle.signalTable[i].soltsCount; j++)
            {
                SIMPLE_SOLTS solts = handle.signalTable[i].soltsTable[j];
                 if (solts != NULL)
                 {
                      solts( handle.signalTable[i].soltsArg[j], pArg);
                 }
            }
        }
    }
}

/******************************************************************
  * @函数说明：   连接信号与槽
  * @输入参数：   SIMPLE_SIGNAL *singnal 信号的指针（指针的指针） 
                 SIMPLE_SOLTS solts     槽
  * @输出参数：   无
  * @返回参数：   ErrorStatus 
  * @修改记录：   ----
******************************************************************/
ErrorStatus SimpleSignalSolts_Connect(SIMPLE_SIGNAL *signal, SIMPLE_SOLTS solts, void *arg)
{
    if (signal == NULL || solts == NULL)    //查空
    {
        return ERROR;
    }

    uint8_t i;

    if (handle.signalCount > SIMPLE_SIGNAL_SOLTS_MAX_SIGNAL)    //错误
    {
        handle.signalCount = 0;
    }

    for (i = 0; i < handle.signalCount; i++) //查找是否是同一个信号
    {
        if (handle.signalTable[i].signleID == signal)  //这里注意，signleID保存的是指针的地址，
        {
            if (handle.signalTable[i].soltsCount > SIMPLE_SIGNAL_SOLTS_MAX_SOLTS)
            {
                handle.signalTable[i].soltsCount = 0;
            }

            if (handle.signalTable[i].soltsCount == SIMPLE_SIGNAL_SOLTS_MAX_SOLTS) //满了
            {
                return ERROR;
            }
            else
            {
                handle.signalTable[i].soltsArg[handle.signalTable[i].soltsCount] = arg;
                handle.signalTable[i].soltsTable[handle.signalTable[i].soltsCount++] = solts; //保存槽
                
                return SUCCESS; //结束
            }
        }
    }

    if (handle.signalCount == SIMPLE_SIGNAL_SOLTS_MAX_SIGNAL)   //满了
    {
        return ERROR;
    }
    else
    {
        handle.signalTable[handle.signalCount].signleID = signal; //保存新的信号
        handle.signalTable[handle.signalCount].soltsTable[0] = solts; //保存槽
        handle.signalTable[handle.signalCount].soltsArg[0] = arg; //保存槽参数   //2017/9/29
        handle.signalTable[handle.signalCount++].soltsCount = 1;
        *signal = SimpleSignalSolts_Signal;
        return SUCCESS;
    }
}
/******************************************************************
  * @函数说明：   断开信号槽
  * @输入参数：   SIMPLE_SIGNAL *singnal 信号的指针（指针的指针） 
                 SIMPLE_SOLTS solts     槽
  * @输出参数：   无
  * @返回参数：   ErrorStatus 
  * @修改记录：   ----
******************************************************************/
ErrorStatus SimpleSignalSolts_Disconnect(SIMPLE_SIGNAL *signal, SIMPLE_SOLTS solts)
{
    if (signal == NULL || solts == NULL || handle.signalCount == 0)    //查空
    {
        return ERROR;
    }
    uint8_t i, j;

    for (i = 0; i < handle.signalCount; i++) //查找是否是同一个信号
    {
        if (handle.signalTable[i].signleID == signal)  //这里注意，signleID保存的是指针的地址，
        {
            for (j = 0; j < handle.signalTable[i].soltsCount; j++)
            {
                 if (handle.signalTable[i].soltsTable[j] == solts)  //找到槽  移除
                 {
                     memcpy(&handle.signalTable[i].soltsTable[j], &handle.signalTable[i].soltsTable[j +1], 
                                (handle.signalTable[i].soltsCount - j - 1) * sizeof(SIMPLE_SOLTS));
                     handle.signalTable[i].soltsCount--;
                 }
            }
            if (handle.signalTable[i].soltsCount == 0)  //此信号没有连接槽了
            {   
                memcpy(&handle.signalTable[i], &handle.signalTable[i + 1], (handle.signalCount - i - 1) * sizeof(SIMPLE_SIGNAL_T));
                handle.signalCount--;
            }
            return SUCCESS;
        }
    }
    return ERROR;
}




