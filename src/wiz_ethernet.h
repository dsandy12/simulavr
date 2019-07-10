/* wiz_etherenet.h - defines the base class for Wiznet w5100 and W5500 ethernet controllers. */
#include "simulationmember.h"
#include "pin.h"
#include "wiz_socket.h"
#include "wiz_spi.h"

#ifndef WIZ_ETHERNET_H
#define WIZ_ETHERNET_H

class wiz_socket;

class wiz_ethernet : public SimulationMember
{
private:
protected:
    // SPI port pins
    Pin sspin;
    Pin misopin;
    Pin mosipin;
    Pin spickpin;

    unsigned int model;         /// set by child classes to help testbench code determine model number without
                                /// creating testbed specific code in the child's code.

    wiz_ethernet();             /// constructor

    // Registers
    unsigned char regs[0x100];  /// memory for general purpose registers - all other memory is included in the sockets
    wiz_socket socket[4];       /// the ethernet controller's socket interfaces
    wiz_spi spi;                /// the ethernet controller's spi interface

    void stepSockets();         /// update the state of the ethernet sockets

    virtual void processSpiTransaction() = 0;  /// pure virtual function - child classes will implement specific transaction-layer processing
    void writeBufToMem(unsigned int addr,unsigned char *buffer,unsigned int len);  /// called by the spi transaction layer to move data to device memory
    void readBufFromMem(unsigned int addr,unsigned char *buffer,unsigned int len); /// called by the spi transaction laer to move data from device memory

    /// Inherited from base class - step the the spi interface and spi transaction layer
    virtual int Step(bool &trueHwStep, SystemClockOffset *timeToNextStepIn_ns=0);

public:
    Pin *getMosiPin();
    Pin *getMisoPin();
    Pin *getSsPin();
    Pin *getSckPin();
    virtual ~wiz_ethernet();    /// destructor

};

#endif
