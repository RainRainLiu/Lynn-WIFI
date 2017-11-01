#ifndef __MY_DEFINE_H__
#define __MY_DEFINE_H__


#define CheckNull(x) if (x == NULL) {while(1);}

#define RAMFUNC __attribute__((section(".entry.text")))

#endif