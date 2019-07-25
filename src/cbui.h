#ifndef CBUI_H
#define CBUI_H

#include "pin.h"
#include "pinatport.h"
#include "simulationmember.h"


class CbUI : public SimulationMember, HasPinNotifyFunction
{

    int fd;                   /** file descriptor for codeblocks socket       */
    Pin * tempPin;            /** a pointer to the device temperature sensor (represented as an analog pin) */

    /*=================
     *  Private Member Functions
     =================*/
    unsigned char recvFromCb();                             /* receive data from Code::Blocks */
    void sendToCb(unsigned char ch);               /* send data to Code::Blocks */
    bool isDataReady();                            /* return true if the real socket has data available */
    virtual void PinStateHasChanged(Pin*);
public:
    /*=================
     *  Public Member Functions
     =================*/
    CbUI(Pin *t, Pin *led);                     /* Constructor */
    virtual ~CbUI();                                /* Destructor */
    void reset();                                   /* reset the ui to its power-on state (do nothing) */
    virtual int Step(bool &trueHwStep, SystemClockOffset *timeToNextStepIn_ns=0);
};

#endif // CBUI_H
