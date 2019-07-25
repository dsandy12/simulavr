/*
 ****************************************************************************
 *
 * wiz_spi - a simulated representation for wiznet 5xxx ethernet
 * spi interface for use with the Simulavr simulator.
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
#ifndef WIZ_SPI_H
#define WIZ_SPI_H

class wiz_ethernet;

class wiz_spi
{
    public:
        wiz_spi(wiz_ethernet *chip);

        unsigned char getByte();              /// slave get a byte from the spi device (called by transaction layer)
        bool isSsSet();                       /// return the state of the SS pin (called by the transaction layer)
        bool isDataReady();                   /// check to see if data is ready from the master (called by the transaction layer)
        void setOutputByte(unsigned char);    /// place a byte in the buffer to send to the master (called by the transaction layer)

        void step();                          /// update the state of the spi device (called by from the ethernet step function)
        void reset();                         /// reset the spi interface (as a hardware chip reset would do)
    protected:

    private:
        wiz_ethernet *eth;
        unsigned char data;                     /// data byte received from the master
        bool dataReady;                         /// is data ready from the master
        unsigned char dataOut;                  /// data to be sent to the master.  Set by the transaction layer
};


#endif // WIZ_SPI_H
