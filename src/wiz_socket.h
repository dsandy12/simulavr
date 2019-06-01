#ifndef WIZ_SOCKET_H
#define WIZ_SOCKET_H

#include <sys/socket.h>
#include "rwmem.h"
#include "wiz_ethernet.h"

class wiz_socket
{
private:
	wiz_ethernet *chip;
    RWMemoryMember **socketrw;  // memory associated with the socket registers
    RWMemoryMember **txrw;      // memory associated with the transmit buffer
    RWMemoryMember **rxrw;      // memory associated with the receive buffer
    IOReg<wiz_socket> mreg,
		creg,
		ireg,
		sreg,
		portreg,
		dhareg0,
		dhareg1,
		dhareg2,
		dhareg3,
		dhareg4,
		dhareg5,
		dipreg0,
		dipreg1,
		dipreg2,
		dipreg3,
		dportreg0,
		dportreg1,
		mssreg0,
		mssreg1,
		tosreg,
		ttlreg,
		txfsreg0,
		txfsreg1,
		txrdreg0,
		txrdreg1,
		txwrreg0,
		txwrreg1,
		rxrsreg0,
		rxrsreg1,
		rxrdreg0,
		rxrdreg1;
};

#endif
