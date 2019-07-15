/* wiz ethernet defines a base class for the W5100 and W5500 ethernet controllers */
#include <string.h>
#include "simulationmember.h"
#include "wiz_ethernet.h"

/// constructor
wiz_ethernet::wiz_ethernet() : spi(this)
{
}

/// destructor
wiz_ethernet::~wiz_ethernet()
{
}

Pin * wiz_ethernet::getMisoPin()
{
    return &misopin;
}

Pin * wiz_ethernet::getMosiPin()
{
    return &mosipin;
}

Pin * wiz_ethernet::getSsPin()
{
    return &sspin;
}

Pin * wiz_ethernet::getSckPin()
{
    return &spickpin;
}

/// called by the spi transaction layer to move data to device memory
void wiz_ethernet::writeBufToMem(unsigned int addr,unsigned char *buffer,unsigned int len)
{
    if (addr<0x100) {
        memcpy(&regs[addr],buffer,len);
    } else if ((addr>=100)&&(addr<0x800)) {
        // socket gp register
        for (unsigned int i=0;i<len;i++) socket[(addr-0x400)/0x100].setRegValue((addr+i)&0xff,buffer[i]);
    } else if ((addr>=0x4000)&&(addr<0x6000)) {
        for (unsigned int i=0;i<len;i++) socket[(addr-0x4000)/0x800].setTxBufferValue((addr+i)&0x7ff,buffer[i]);
    } else if ((addr>=0x6000)&&(addr<0x8000)) {
        for (unsigned int i=0;i<len;i++) socket[(addr-0x6000)/0x800].setRxBufferValue((addr+i)&0x7ff,buffer[i]);
    }
}

/// called by the spi transaction laer to move data from device memory
void wiz_ethernet::readBufFromMem(unsigned int addr,unsigned char *buffer,unsigned int len)
{
    if (addr<0x100) {
        memcpy(buffer,&regs[addr],len);
    } else if ((addr>=100)&&(addr<0x800)) {
        // socket gp register
        for (unsigned int i=0;i<len;i++) buffer[i] = socket[(addr-0x400)/0x100].getRegValue((addr+i)&0xff);
    } else if ((addr>=0x4000)&&(addr<0x6000)) {
        for (unsigned int i=0;i<len;i++) buffer[i] = socket[(addr-0x4000)/0x800].getTxBufferValue((addr+i)&0x7ff);
    } else if ((addr>=0x6000)&&(addr<0x8000)) {
        for (unsigned int i=0;i<len;i++) buffer[i] = socket[(addr-0x6000)/0x800].getRxBufferValue((addr+i)&0x7ff);
    }
}

/// step the sockets by one simulation step
void wiz_ethernet::stepSockets() {
    for (unsigned int i=0;i<4;i++) socket[i].step();
}

/// Inherited from base class - step the the spi interface and spi transaction layer
int wiz_ethernet::Step(bool &trueHwStep, SystemClockOffset *timeToNextStepIn_ns)
{
	// next step in 100 nanoSec
	*timeToNextStepIn_ns	= 100;	// Once every 100 nanosecond

	spi.step();
	processSpiTransaction();
	static unsigned int i = 0;
	i++; if (i>10000) {
        stepSockets();
        i=0;
    }
	return 0;
}
