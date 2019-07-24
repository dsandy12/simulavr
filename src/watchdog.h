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

#ifndef Watchdog
#define Watchdog

#include "hardware.h"
#include "avrdevice.h"
#include "rwmem.h"
#include "systemclocktypes.h"
#include "traceval.h"
#include "hwwado.h"


class AvrDevice;
class HWIrqSystem;

/** Watchdog (WDT) peripheral. */
class WatchDog: public HWWado {
	protected:
	unsigned char wdtcsr;
	unsigned char counter; //4 cycles counter for unsetting the wde
	unsigned int timeOutOnce;
	SystemClockOffset timeOutAt;
	AvrDevice *core;
	HWIrqSystem *irqSystem;

	public:
		WatchDog(AvrDevice *,HWIrqSystem *); // { irqSystem= s;}
		virtual unsigned int CpuCycle();
        virtual void ClearIrqFlag(unsigned int vector);
		void setWdtcsr(unsigned char val);
		unsigned char getWdtcsr() { return wdtcsr; }
		virtual void Wdr(); //reset the wado counter
		virtual void Reset();

        IOReg<WatchDog> wdtcsr_reg;
};


#endif
