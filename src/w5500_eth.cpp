/*
 ****************************************************************************
 *
 * w5500_ethernet - a simulated representation of the wiznet 5500 ethernet
 * controller for use with the Simulavr simulator. Note that not all register
 * functionality has been implemented, especially registers that are not
 * consistent witht the W5100 ethernet controller.
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
#include <string.h>
#include "wiz_spi.h"
#include "w5500_eth.h"

w5500_eth::w5500_eth() {
    model = 5500;
}

w5500_eth::~w5500_eth()
{}

enum State {WAITING,  ADDR1, ADDR2, CMD, DATA};
#define COMMON_BASE       0x0000
#define SOCKET0_BASE      0x0400
#define SOCKET1_BASE      0x0500
#define SOCKET2_BASE      0x0600
#define SOCKET3_BASE      0x0700
#define SOCKET0_TX_BASE   0x4000
#define SOCKET1_TX_BASE   0x4800
#define SOCKET2_TX_BASE   0x5000
#define SOCKET3_TX_BASE   0x5800
#define SOCKET0_RX_BASE   0x6000
#define SOCKET1_RX_BASE   0x6800
#define SOCKET2_RX_BASE   0x7000
#define SOCKET3_RX_BASE   0x7800
#define RESERVED          0x9000

void w5500_eth::processSpiTransaction() {
    static State state = State::WAITING;
    static unsigned int addr;
    static unsigned char cmd;
    static unsigned char buffer[2048];
    static unsigned int bytesReceived;
    static bool isWrite = false;
    static unsigned int regBase[] = {
        COMMON_BASE, SOCKET0_BASE, SOCKET0_TX_BASE, SOCKET0_RX_BASE,
        RESERVED, SOCKET1_BASE, SOCKET1_TX_BASE, SOCKET1_RX_BASE,
        RESERVED, SOCKET2_BASE, SOCKET2_TX_BASE, SOCKET2_RX_BASE,
        RESERVED, SOCKET3_BASE, SOCKET3_TX_BASE, SOCKET3_RX_BASE,
        RESERVED, RESERVED, RESERVED, RESERVED,
        RESERVED, RESERVED, RESERVED, RESERVED,
        RESERVED, RESERVED, RESERVED, RESERVED,
        RESERVED, RESERVED, RESERVED, RESERVED
    };
    unsigned char ch;

    // first take care of asynchronous events
    if (!spi.isSsSet()) {
        if (isWrite) {
            // write the buffer to the socket
            writeBufToMem(regBase[(cmd>>3)&0x1F]+addr,buffer,bytesReceived);
        }
        isWrite = false;
        if (spi.isDataReady()) spi.getByte();
        state = State::WAITING;
        return;
    }

    // here if SS is set
    if (!spi.isDataReady()) return;

    // here if data is ready
    switch (state) {
    case State::ADDR1:
        case State::WAITING:
            state = State::ADDR2;
            addr = ((unsigned int)spi.getByte())<<8;
            bytesReceived=0;
            isWrite = false;
            spi.setOutputByte(0);
            break;
        case State::ADDR2:
            state = State::CMD;
            addr += spi.getByte();
            spi.setOutputByte(0);
            break;
        case State::CMD:
            state = State::DATA;
            cmd = spi.getByte();
            if ((cmd&0x4)!=0) isWrite = true;
            spi.setOutputByte(0);
            if (!isWrite) {
                readBufFromMem(regBase[(cmd>>3)&0x1F]+addr+bytesReceived,&ch,1);
                spi.setOutputByte(ch);
            }            break;
        case State::DATA:
            buffer[bytesReceived] = spi.getByte();
            if (!isWrite) {
                // operation is a read - get the first byte and place it
                // in the SPI output buffer.
                readBufFromMem(regBase[(cmd>>3)&0x1F]+addr+bytesReceived+1,&ch,1);
                spi.setOutputByte(ch);
            } else {
                spi.setOutputByte(0);
            }
            bytesReceived ++;
            break;
    }
}

