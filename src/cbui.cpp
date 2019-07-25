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
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "cbui.h"

#define RXPORT 7777
#define TXPORT 8877

/********************************************************************
* CbUI()
* Constructor
*
* params:
*    t (in) - a pointer to the temperature sensor pin
*    led (in) - a pointer to the LED pin
*/
CbUI::CbUI(Pin *t, Pin * led) : tempPin(t), fd(0)
{
    struct sockaddr_in local_addr;
    int optval;

    // open tx socket
    fd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
    if (fd>0) {
        optval = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    }

    // bind the local address and local port for receiving packets.
    memset((char *)&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(0x7F000001UL);
    local_addr.sin_port = htons(RXPORT);
    if (bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        close(fd);
        fd = 0;
    }

    // set pin callback for changes to led state
    led->RegisterCallback(this);
}

/********************************************************************
* CbUI()
* Destructor
*/
CbUI::~CbUI()
{
    if (fd) close(fd);
}

/********************************************************************
* sendToCb()
* send a single byte of data to Code::Blocks on UDP port 127.0.0.1:8877
*
* params:
*    ch (IN) - the character to send
* returns: NONE
*/
void CbUI::sendToCb(unsigned char ch)
{
    if (fd<=0) return;
    struct sockaddr_in remote_addr;
    memset((char*)&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(8877); // destination port
    remote_addr.sin_addr.s_addr = htonl(0x7F000001UL);  // destination address (127.0.0.1)
    sendto(fd, &ch, 1, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
}

/********************************************************************
* recvFromCb()
* receive a single byte of data to Code::Blocks on UDP port 127.0.0.1:7777.
* If the received packet is greater than a single byte, all other bytes
* are ignored.
*
* params: NONE
* returns:
*   The character read.
*/
unsigned char CbUI::recvFromCb()
{
    if (fd<=0) return 0;
    char buffer;
    read(fd,(void*)&buffer,1);
    return buffer;
}

/********************************************************************
* PinStateHasChanged()
* This callback function is called whenever the LED pin state has changed.
* The new pin state is sent to Code::Blocks.
*
* params:
*    p (input) - a pointer to the pin that changed.
* returns:
*   None
*/
void CbUI::PinStateHasChanged(Pin*p)
{
    static char last_sent = 0;

    // get the state of the pin
    switch (p->outState) {
    case Pin::PULLUP:
    case Pin::HIGH:
        if (last_sent!='H') sendToCb('H');
        last_sent = 'H';
        break;
    case Pin::PULLDOWN:
    case Pin::LOW:
        if (last_sent!='L') sendToCb('L');
        last_sent = 'L';
        break;
    default:
        if (last_sent!='Z') sendToCb('Z');
        last_sent = 'Z';
        break;
    }
}


/********************************************************************
* Reset()
*
* Reset the Codeblocks UI to its initial state.  (Do nothing)
*/
void CbUI::reset()
{
}


/********************************************************************
* Step()
*
* Called periodically by the simulator to see if code::blocks has sent
* temperature change requests.
*/
int CbUI::Step(bool &trueHwStep, SystemClockOffset *timeToNextStepIn_ns)
{
    float val;
    *timeToNextStepIn_ns = 1000000UL;  // check every 1ms of simulation time
    if (isDataReady()) {
        // receive data
        unsigned char ch = recvFromCb();
        switch (ch) {
        case '>':
            // increase the chip temperature by about 5 degrees C
            val = tempPin->GetAnalogValue(1.1) +0.005;
            if (val>1.1) val = 1.1;
            tempPin->SetAnalogValue(val);
            break;
        case '<':
            // decrease the chip temperature by about 5 degrees C
            val = tempPin->GetAnalogValue(1.1) - 0.005;
            if (val<0) val = 0;
            tempPin->SetAnalogValue(val);
            break;
        }
    }
    return 0;
}

/********************************************************************
* isDataReady()
* return true if the receive socket has data ready.  This function can
* be polled to avoid the socket blocking on read.
*/
bool CbUI::isDataReady()
{
    //if (!descriptor) return true;   // socket is closed
    if (fd<=0) return false;

    struct timeval tv;
    fd_set  fd_set;

    // use select to see if there the port is ready
    tv.tv_sec  = 0;
    tv.tv_usec = 1;

    FD_ZERO(&fd_set);
    FD_SET(fd, &fd_set);

    if (select(FD_SETSIZE, &fd_set, 0, 0, &tv)==0) return false;

    // now check the IO controller to see if there are any bytes ready
    // if not, the port is disconnected.
    long bytes = 0;
    if (ioctl(fd, FIONREAD, &bytes)<0) return false;
    return (bytes>0);
}
