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
#include "wiz_ethernet.h"

#ifndef W5500_ETH_H
#define W5500_ETH_H


class w5500_eth : public wiz_ethernet
{
    public:
        w5500_eth();
        virtual ~w5500_eth();

    protected:
        virtual void processSpiTransaction();
        //virtual void writeBufToMem(unsigned int addr,unsigned char *buffer,unsigned int len);
        //virtual void readBufFromMem(unsigned int addr,unsigned char *buffer,unsigned int len);
    private:
};

#endif // W5500_ETH_H
