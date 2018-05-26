#ifndef _SIMPLE_SIGNAL_SOLTS_H_
#define _SIMPLE_SIGNAL_SOLTS_H_

#include "stdint.h"

typedef enum
{
	ERROR = 0,
	SUCCESS = 1,
}ErrorStatus;


typedef void (*SIMPLE_SIGNAL)(void *signal, void *pArg);
typedef void (*SIMPLE_SOLTS) (void *slotsArg,void *pArg);

#define SIMPLE_SOLTS_T(FuncName)   void(FuncName)(void *slotsArg, void *pArg)

#define SIMPLE_EMIT(signal, arg)  if (signal != NULL)signal(&signal, arg)

#define SIMPLE_SIGNAL_SOLTS_MAX_SOLTS       8      //һ���ź�������Ӳ۵�����
#define SIMPLE_SIGNAL_SOLTS_MAX_SIGNAL      8      //�ź��������


ErrorStatus SimpleSignalSolts_Connect(SIMPLE_SIGNAL *signal, SIMPLE_SOLTS solts, void *arg);

#endif
