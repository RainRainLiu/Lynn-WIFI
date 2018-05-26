/******************************************************************************
* @ File name --> SimpleSignalSolts.c
* @ Author    --> By@ LiuYu
* @ Version   --> V1.0
* @ Date      --> 2017 - 06 - 15
* @ Brief     --> ���������ź����
* @           --> һ���źŶ�Ӧ����ۣ�������
*
* @ Copyright (C) 20**
* @ All rights reserved
*******************************************************************************
*
*                                  File Update
* @ Version   --> V1.1
* @ Author    --> By@ LiuYu
* @ Date      --> 2017 - 09 - 12
* @ Revise    --> �����˲۵��Զ���������ڻص���ʱʹ��
* @           --> 
*
*
******************************************************************************
*
*                                  File Update
* @ Version   --> V1.2
* @ Author    --> By@ LiuYu
* @ Date      --> 2017 - 09 - 29
* @ Revise    --> �޸����ź����ӵ�һ����ʱ�����޷���ȷ�洢��BUG
* @           --> 
*
*
******************************************************************************/
#include "SimpleSignalSolts.h"
#include <string.h>

//�źŽṹ
typedef struct
{
    void            *signleID;    //�źŵ�ָ���ָ�룬�����źŵ�ָ�룬����ָ��ĵ�ַȷ���Ƿ���Ψһ
    SIMPLE_SOLTS    soltsTable[SIMPLE_SIGNAL_SOLTS_MAX_SOLTS];
    void *          soltsArg[SIMPLE_SIGNAL_SOLTS_MAX_SOLTS];
    uint8_t         soltsCount;
}SIMPLE_SIGNAL_T;
//�źű�ṹ
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
    
    
    for (i = 0; i < handle.signalCount; i++) //�����Ƿ���ͬһ���ź�
    {
        if (handle.signalTable[i].signleID == signal)  //����ע�⣬signleID�������ָ��ĵ�ַ��
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
  * @����˵����   �����ź����
  * @���������   SIMPLE_SIGNAL *singnal �źŵ�ָ�루ָ���ָ�룩 
                 SIMPLE_SOLTS solts     ��
  * @���������   ��
  * @���ز�����   ErrorStatus 
  * @�޸ļ�¼��   ----
******************************************************************/
ErrorStatus SimpleSignalSolts_Connect(SIMPLE_SIGNAL *signal, SIMPLE_SOLTS solts, void *arg)
{
    if (signal == NULL || solts == NULL)    //���
    {
        return ERROR;
    }

    uint8_t i;

    if (handle.signalCount > SIMPLE_SIGNAL_SOLTS_MAX_SIGNAL)    //����
    {
        handle.signalCount = 0;
    }

    for (i = 0; i < handle.signalCount; i++) //�����Ƿ���ͬһ���ź�
    {
        if (handle.signalTable[i].signleID == signal)  //����ע�⣬signleID�������ָ��ĵ�ַ��
        {
            if (handle.signalTable[i].soltsCount > SIMPLE_SIGNAL_SOLTS_MAX_SOLTS)
            {
                handle.signalTable[i].soltsCount = 0;
            }

            if (handle.signalTable[i].soltsCount == SIMPLE_SIGNAL_SOLTS_MAX_SOLTS) //����
            {
                return ERROR;
            }
            else
            {
                handle.signalTable[i].soltsArg[handle.signalTable[i].soltsCount] = arg;
                handle.signalTable[i].soltsTable[handle.signalTable[i].soltsCount++] = solts; //�����
                
                return SUCCESS; //����
            }
        }
    }

    if (handle.signalCount == SIMPLE_SIGNAL_SOLTS_MAX_SIGNAL)   //����
    {
        return ERROR;
    }
    else
    {
        handle.signalTable[handle.signalCount].signleID = signal; //�����µ��ź�
        handle.signalTable[handle.signalCount].soltsTable[0] = solts; //�����
        handle.signalTable[handle.signalCount].soltsArg[0] = arg; //����۲���   //2017/9/29
        handle.signalTable[handle.signalCount++].soltsCount = 1;
        *signal = SimpleSignalSolts_Signal;
        return SUCCESS;
    }
}
/******************************************************************
  * @����˵����   �Ͽ��źŲ�
  * @���������   SIMPLE_SIGNAL *singnal �źŵ�ָ�루ָ���ָ�룩 
                 SIMPLE_SOLTS solts     ��
  * @���������   ��
  * @���ز�����   ErrorStatus 
  * @�޸ļ�¼��   ----
******************************************************************/
ErrorStatus SimpleSignalSolts_Disconnect(SIMPLE_SIGNAL *signal, SIMPLE_SOLTS solts)
{
    if (signal == NULL || solts == NULL || handle.signalCount == 0)    //���
    {
        return ERROR;
    }
    uint8_t i, j;

    for (i = 0; i < handle.signalCount; i++) //�����Ƿ���ͬһ���ź�
    {
        if (handle.signalTable[i].signleID == signal)  //����ע�⣬signleID�������ָ��ĵ�ַ��
        {
            for (j = 0; j < handle.signalTable[i].soltsCount; j++)
            {
                 if (handle.signalTable[i].soltsTable[j] == solts)  //�ҵ���  �Ƴ�
                 {
                     memcpy(&handle.signalTable[i].soltsTable[j], &handle.signalTable[i].soltsTable[j +1], 
                                (handle.signalTable[i].soltsCount - j - 1) * sizeof(SIMPLE_SOLTS));
                     handle.signalTable[i].soltsCount--;
                 }
            }
            if (handle.signalTable[i].soltsCount == 0)  //���ź�û�����Ӳ���
            {   
                memcpy(&handle.signalTable[i], &handle.signalTable[i + 1], (handle.signalCount - i - 1) * sizeof(SIMPLE_SIGNAL_T));
                handle.signalCount--;
            }
            return SUCCESS;
        }
    }
    return ERROR;
}




