#include "NWGlobal.h"

unsigned int	g_nTick = 0;
unsigned int    g_nTimeDifference = 0;  // �� �ð����� �ð���
int g_nRecvBufSize;
int g_nSendDataCount;

void UpdateTick()
{
#ifdef WIN32
    unsigned int   nTempTick = g_nTick; 
    g_nTick = GetTickCount();
    g_nTimeDifference = g_nTick - nTempTick;
#else
    struct timeval t;
    gettimeofday (&t, 0);
    g_nTick = t.tv_sec * 1000 + t.tv_usec / 1000;
#endif
}

void UpdateTick(unsigned int nTick)
{
    g_nTimeDifference = nTick - g_nTick;
    g_nTick = nTick;
}