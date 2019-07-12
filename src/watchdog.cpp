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
#include <unistd.h> //

#define WDCE 0x10
#define WDRE 0x08
#define WDIE 0x40

int printing; //


WatchDog::WatchDog(AvrDevice *c, HWIrqSystem *i):
    HWWado(c),
    irqSystem(i),
    core(c),
    wdtcsr_reg(this, "WDTCSR", this, &WatchDog::GetWdtcsr, &WatchDog::SetWdtcsr) {
	core->AddToCycleList(this);
	irqSystem->DebugVerifyInterruptVector(6, this);
	Reset();
}

void WatchDog::SetWdtcsr(unsigned char val) {
	if (((val & WDRE) != 0) && ((wdtcsr & WDCE) != 0)) { //Reset enabled
		wdtcsr = ((val & WDRE) | (wdtcsr & 0xf7));

	}

	if  (((val & 0x27) != 0) && ((wdtcsr & WDCE) != 0)) { //Changing prescalers
        wdtcsr = ((val & 0x27) | (wdtcsr & 0xd8));
        Wdr();

	}

	if ((val & WDCE) != 0) { //Change enabled
		counter = 4;
		wdtcsr = ((val & WDCE) | (wdtcsr & 0xef));

	}

	if ((val & WDIE) != 0) { //Interrupt enabled
        wdtcsr = ((val & WDIE) | (wdtcsr & 0xbf));

	}

}

unsigned int WatchDog::CpuCycle() {
    //std::cout << timeOutAt << std::endl;
	if (counter > 0) { //Count for WDCE
		counter--;

	}

	if (counter == 0) wdtcsr &= (0xff - WDCE); //after 4 cpu cycles

	if ((((wdtcsr & WDRE) != 0) && ((wdtcsr & WDIE) == 0)) && (timeOutAt == SystemClock::Instance().GetCurrentTime() )) { //Reset mode
		core->Reset();
	}


	if (((wdtcsr & WDRE) == 0) && ((wdtcsr & WDIE) != 0)) {
     if  ((timeOutAt <= SystemClock::Instance().GetCurrentTime()) && (timeOutAt)) { //Interrupt mode

            //std::cout << "I am about to interrupt" << std::endl;

            irqSystem->SetIrqFlag(this, 6);
            timeOutAt = 0;

            //std::cout << "I signaled the interrupt vector" << std::endl;


            //printing = 1; //

     } else {

     //std::cout << timeOutAt << " " << SystemClock::Instance().GetCurrentTime() << std::endl;
     }


    }


    /*if (((((wdtcsr & WDRE) != 0) && (wdtcsr & WDIE) != 0)) && (timeOutAt < SystemClock::Instance().GetCurrentTime() && (timeOutOnce != 1))) { //Interrupt and reset mode (Interrupt portion)
        irqSystem->SetIrqFlag(this, 6);
        timeOutOnce = 1;
        timeOutAt = 0;

    }

    if (((((wdtcsr & WDRE) != 0) && (wdtcsr & WDIE) != 0)) && (timeOutAt < SystemClock::Instance().GetCurrentTime() && (timeOutOnce == 1))) { //Interrupt and reset mode (Reset portion)
        core->Reset();

    }
    */
    return 0;
}

void WatchDog::ClearIrqFlag(unsigned int vector){
    if(vector == 6) {
        irqSystem->ClearIrqFlag(6);
    }
}


void WatchDog::Reset() {
	timeOutAt = 0;
	wdtcsr = 0;

}


void WatchDog::Wdr() {
	SystemClockOffset currentTime= SystemClock::Instance().GetCurrentTime();
	switch (wdtcsr & 0x27) {
		case 0:
		    //std::cout << "I accessed the 16ms" << std::endl;
		    //wdtcsr = 0x20;
			timeOutAt= currentTime+ 16000000; //16ms
			break;

		case 1:
		    //std::cout << "I accessed the 32ms" << std::endl;
			timeOutAt= currentTime+ 32000000; //32ms
			break;

		case 2:
            //std::cout << "I accessed the 64ms" << std::endl;
			timeOutAt= currentTime+ 64000000; //64 ms
			break;

		case 3:
			timeOutAt= currentTime+ 125000000; //125 ms
			break;

		case 4:
			timeOutAt= currentTime+ 250000000; //250 ms
			break;

        case 5:
            timeOutAt= currentTime+ 500000000; //500 ms
            break;

		case 6:
			timeOutAt= currentTime+ 1000000000ULL; //1 s
			break;

		case 7:
		    //std::cout << "I accessed the 2s";
			timeOutAt= currentTime+ 2000000000ULL; //2 s
			break;

		case 0x20:
		    //std::cout << "I accessed the 4s";
			timeOutAt= currentTime+ 4000000000ULL; //4 s
			break;

        case 0x21:
            //std::cout << "I accessed the 8s";
            timeOutAt= currentTime+ 8000000000ULL; //8 s
            break;

	}
}
