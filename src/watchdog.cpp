 /*
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002, 2003   Klaus Rudolph
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
 *
 *  $Id$
 */

#include "watchdog.h"
#include "avrdevice.h"
#include "systemclock.h"
#include "irqsystem.h"

#define WDCE 0x10
#define WDRE 0x08
#define WDIE 0x40


WatchDog::WatchDog(AvrDevice *c, HWIrqSystem *i):
    HWWado(c),
    irqSystem(i),
    core(c),
    wdtcsr_reg(this, "WDTCSR", this, &WatchDog::getWdtcsr, &WatchDog::setWdtcsr)
{
	core->AddToCycleList(this);
	irqSystem->DebugVerifyInterruptVector(6, this);
	timeoutCount = 0;
	Reset();
}

/********************************************************************
 * setWdtcsr()
 * Sets the value of the WDRE, WDIE, WDCE, and prescalers.
 *
 * params:
 *    val (UN CH) - the byte with new WDTCSR values
 *
 * returns: None
 */
void WatchDog::setWdtcsr(unsigned char val) {
	if (((val & WDRE) != 0) && ((wdtcsr & WDCE) != 0)) { //Reset enabled
		wdtcsr = ((val & WDRE) | (wdtcsr & 0xf7));
	}

	if  (((val & 0x27) != 0) && ((wdtcsr & WDCE) != 0)) { //Changing prescalers
        wdtcsr = ((val & 0x27) | (wdtcsr & 0xd8));
        Wdr(); //Updates the timeout timer to the time set by prescalers
	}

	if ((val & WDCE) != 0) { //Change enabled
		counter = 4; //Sets the 4-cycle gate of the WDCE
		wdtcsr = ((val & WDCE) | (wdtcsr & 0xef));
	}

	if ((val & WDIE) != 0) { //Interrupt enabled
        wdtcsr = ((val & WDIE) | (wdtcsr & 0xbf));
	}
}

/********************************************************************
 * CpuCycle()
 * Calls for a reset or interrupt if timeout has occurred. Also changes the 4-cycle gate of the WDCE.
 *
 * params: None
 *
 * returns: None
 */
unsigned int WatchDog::CpuCycle() {
	if (counter > 0) { //countdown for WDCE 4-cycle gate
		counter--;
	}

	if (counter == 0) wdtcsr &= (0xff - WDCE); //after 4 cpu cycles, sets WDCE to 0.

	if (((wdtcsr & WDRE) != 0) && ((wdtcsr & WDIE) == 0)) {  //Reset mode
		if  ((timeOutAt <= SystemClock::Instance().GetCurrentTime()) && (timeOutAt)) {
            core->Reset();
        }
	}

	if (((wdtcsr & WDRE) == 0) && ((wdtcsr & WDIE) != 0)) { //Interrupt mode
        if  ((timeOutAt <= SystemClock::Instance().GetCurrentTime()) && (timeOutAt)) {
            irqSystem->SetIrqFlag(this, 6);
            timeOutAt = 0;
        }
    }

    if ((((wdtcsr & WDRE) != 0) && (wdtcsr & WDIE) != 0)) { //Interrupt and reset mode
        if  ((timeOutAt <= SystemClock::Instance().GetCurrentTime()) && (timeOutAt)) {
            if (timeoutCount == 0) { //Interrupt portion
                irqSystem->SetIrqFlag(this, 6);
                Wdr();
                timeoutCount = 1;
            } else { //Reset portion
                core->Reset();
            }
        }
    }

    return 0;
}

/********************************************************************
 * ClearIrqFlag()
 * Clears interrupt flag if vector 6 was called.
 *
 * params:
 *    vector (UN IN) - The number of the interrupt vector that was initiated.
 *
 * returns: None
 */
void WatchDog::ClearIrqFlag(unsigned int vector){
    if(vector == 6) {
        irqSystem->ClearIrqFlag(6);
    }
}

/********************************************************************
 * Reset()
 * Resets the WDTCSR
 *
 * params: None
 *
 * returns: None
 */
void WatchDog::Reset() {
	timeOutAt = 0;
	wdtcsr = 0;
	timeoutCount = 0;
}

/********************************************************************
 * Wdr()
 * Sets the timeout value according to the prescaler values.
 *
 * params: None
 *
 * returns: None
 */
void WatchDog::Wdr() {
	SystemClockOffset currentTime= SystemClock::Instance().GetCurrentTime();
	timeoutCount = 0;
	switch (wdtcsr & 0x27) {
		case 0x00:
			timeOutAt= currentTime+ 16000000; //16ms
			break;

		case 0x01:
			timeOutAt= currentTime+ 32000000; //32ms
			break;

		case 0x02:
			timeOutAt= currentTime+ 64000000; //64 ms
			break;

		case 0x03:
			timeOutAt= currentTime+ 125000000; //125 ms
			break;

		case 0x04:
			timeOutAt= currentTime+ 250000000; //250 ms
			break;

        case 0x05:
            timeOutAt= currentTime+ 500000000; //500 ms
            break;

		case 0x06:
			timeOutAt= currentTime+ 1000000000ULL; //1 s
			break;

		case 0x07:
			timeOutAt= currentTime+ 2000000000ULL; //2 s
			break;

		case 0x20:
			timeOutAt= currentTime+ 4000000000ULL; //4 s
			break;

        case 0x21:
            timeOutAt= currentTime+ 8000000000ULL; //8 s
            break;
	}
}
