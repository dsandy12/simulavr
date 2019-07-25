/*
 ****************************************************************************
 *
 * CbUI - a user interface to provide device temperature control and LED
 * visualization from within the Code::Blocks IDE.
 *
 * Copyright (C) 2019   Douglas Sandy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 ****************************************************************************
 */
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
