/*
 ****************************************************************************
 *
 * w5100_ethernet - a simulated representation of the wiznet 5100 ethernet
 * controller for use with the Simulavr simulator. Note that not all register
 * functionality has been implemented, especially registers that are not
 * related to TCP/UDP operation.
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
#include "w5100_eth.h"

w5100_eth::w5100_eth() {
    model = 5100;
}

w5100_eth::~w5100_eth()
{}

enum State {WAITING, ADDR1, ADDR2, CMD, DATA};

void w5100_eth::processSpiTransaction() {
    static State state = State::WAITING;
    static unsigned int addr;
    static unsigned char cmd;
    static bool isWrite = false;
    unsigned char ch;

    // first take care of asynchronous events
    if (!spi.isSsSet()) {
        if (spi.isDataReady()) spi.getByte();
        state = State::WAITING;
        return;
    }

    // here if SS is set
    if (!spi.isDataReady()) return;

    // here if data is ready
    switch (state) {
        case State::WAITING:
        case State::CMD:
            cmd = spi.getByte();
            if (cmd==0xf0) {
                isWrite = true;
                state = State::ADDR1;
            } else if (cmd==0x0f) {
                isWrite = false;
                state = State::ADDR1;
            } else {
                state = State::WAITING;
            }
            spi.setOutputByte(0);
            break;
        case State::ADDR1:
            state = State::ADDR2;
            addr = ((unsigned int)spi.getByte())<<8;
            spi.setOutputByte(0);
            break;
        case State::ADDR2:
            state = State::DATA;
            addr += spi.getByte();
            spi.setOutputByte(0);
            break;
        case State::DATA:
            ch = spi.getByte();
            if (!isWrite) {
                // operation is a read - get the byte and place it
                // in the SPI output buffer.
                readBufFromMem(addr,&ch,1);
                spi.setOutputByte(ch);
            } else {
                // operation is a write - write the byte to the
                // ethernet memory space
                writeBufToMem(addr,&ch,1);
                spi.setOutputByte(0);
            }
            state = State::WAITING;
            break;
    }
}

void w5100_eth::writeBufToMem(unsigned int addr,unsigned char *buffer,unsigned int len)
{
    if (addr<0x100) {
        if (addr+len>=0x100) {
            memcpy(&regs[addr],buffer,0x100-addr);
        } else {
            memcpy(&regs[addr],buffer,len);
        }
    } else if (addr<0x4000) {
        // socket gp register
        for (unsigned int i=0;i<len;i++) socket[(addr-0x400)/0x100].setRegValue((addr+i)&0xff,buffer[i]);
    } else if (addr<0x6000) {
        for (unsigned int i=0;i<len;i++) socket[(addr-0x4000)/0x800].setTxBufferValue((addr+i)&0x7ff,buffer[i]);
    } else {
        for (unsigned int i=0;i<len;i++) socket[(addr-0x6000)/0x800].setRxBufferValue((addr+i)&0x7ff,buffer[i]);
    }
}

void w5100_eth::readBufFromMem(unsigned int addr,unsigned char *buffer,unsigned int len)
{
    if (addr<0x100) {
        if (addr+len>=0x100) {
            memcpy(buffer,&regs[addr],0x100-addr);
        } else {
            memcpy(buffer,&regs[addr],len);
        }
    } else if (addr<0x4000) {
        // socket gp register
        for (unsigned int i=0;i<len;i++) buffer[i] = socket[(addr-0x400)/0x100].getRegValue((addr+i)&0xff);
    } else if (addr<0x6000) {
        for (unsigned int i=0;i<len;i++) buffer[i] = socket[(addr-0x4000)/0x800].getTxBufferValue((addr+i)&0x7ff);
    } else {
        for (unsigned int i=0;i<len;i++) buffer[i] = socket[(addr-0x6000)/0x800].getRxBufferValue((addr+i)&0x7ff);
    }
}

