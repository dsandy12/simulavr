/*
 ****************************************************************************
 *
 * wiz_socket - a simulated socket representation for wiznet 5xxx ethernet
 * controllers for use with the Simulavr simulator.
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
#ifndef WIZ_SOCKET_H
#define WIZ_SOCKET_H

#include <cstdint>
#include <sys/socket.h>

class wiz_socket
{
private:
    /*=================
     *  Typedefs and structs
     =================*/
    // structure for NTP packet
    typedef struct {
        uint32_t header;
        uint32_t root_delay;
        uint32_t root_dispersion;
        uint32_t reference_id;
        uint32_t ref_timestamp[2];
        uint32_t orig_timestamp[2];
        uint32_t rx_timestamp[2];
        uint32_t tx_timestamp[2];
    } ntp_packet;

    /* structure for dhcp support */
    typedef struct __attribute__((packed)) dhcppacket
    {
        unsigned char  header[8];
        unsigned char  op;
        unsigned char  htype;
        unsigned char  hlen;
        unsigned char  hops;
        uint32_t       xid;
        uint16_t       secs;
        uint16_t       flags;
        unsigned char  ciaddr[4];
        unsigned char  yiaddr[4];
        unsigned char  siaddr[4];
        unsigned char  giaddr[4];
        unsigned char  chaddr[16];
        unsigned char  sname[64];
        unsigned char  file[128];
        unsigned char  magic_cookie[4];
        unsigned char  option1[3];  // response type
        unsigned char  option2[6];  // subnet mask
        unsigned char  option3[6];  // gateway
        unsigned char  option4[7];  // dhcp server;
    } dhcp_packet_type;

    /*=================
     *  Constants
     =================*/
    /* offsets of each of the registers */
    static const int MR_OFFSET     = 0x0000;   /** Mode Register offset */
    static const int CR_OFFSET     = 0x0001;   /** Command Register offset */
    static const int IR_OFFSET     = 0x0002;   /** Interrupt Register offset */
    static const int SR_OFFSET     = 0x0003;   /** Status Register offset */
    static const int PORT_OFFSET   = 0x0004;   /** Source Port Register offset (2 bytes) */
    static const int DHAR_OFFSET   = 0x0006;   /** Destination Hardware Address Register (MAC, 6 bytes) */
    static const int DIPR_OFFSET   = 0x000C;   /** Destination IP Address Register (IP, 4 bytes) */
    static const int DPORT_OFFSET  = 0x0010;   /** Destination Port Register (2 bytes) */
    static const int TX_FSR_OFFSET = 0x0020;   /** Transmit Free Size Register (2 bytes) */
    static const int TX_RR_OFFSET  = 0x0022;   /** Transmit Read Pointer Register (2 bytes) */
    static const int TX_WR_OFFSET  = 0x0024;   /** Transmit Write Pointer Register (2 bytes) */
    static const int RX_RSR_OFFSET = 0x0026;   /** Receive Received Size Register (2 bytes) */
    static const int RX_RD_OFFSET  = 0x0028;   /** Receive Read Pointer Register (2 bytes) */

    /* values that can be read from the Status register. */
    static const unsigned char SKT_SR_CLOSED      = 0x00;   /** Status: Closed */
    static const unsigned char SKT_SR_INIT        = 0x13;   /** Status: Init state */
    static const unsigned char SKT_SR_LISTEN      = 0x14;   /** Status: Listen state */
    static const unsigned char SKT_SR_ESTABLISHED = 0x17;   /** Status: Connected */
    static const unsigned char SKT_SR_UDP         = 0x22;   /** Status: UDP socket */

    /* values for selecting commands in the Command register */
    static const unsigned char SKT_CR_OPEN      = 0x01;     /** Command: Open the socket */
    static const unsigned char SKT_CR_LISTEN    = 0x02;     /** Command: Wait for TCP connection (server mode) */
    static const unsigned char SKT_CR_CONNECT   = 0x04;     /** Command: Listen for TCP connection (client mode) */
    static const unsigned char SKT_CR_DISCON    = 0x08;     /** Command: Close TCP connection */
    static const unsigned char SKT_CR_CLOSE     = 0x10;     /** Command: Mark socket as closed (does not close TCP connection) */
    static const unsigned char SKT_CR_SEND      = 0x20;     /** Command: Transmit data in TX buffer */
    static const unsigned char SKT_CR_RECV      = 0x40;     /** Command: Receive data into RX buffer */

    /* values written to a socket's Mode register to select the
     * protocol and other operating characteristics. */
    static const unsigned char SKT_MR_CLOSE     = 0x00;     /** Mode: Unused socket */
    static const unsigned char SKT_MR_TCP       = 0x01;     /** Mode: TCP */
    static const unsigned char SKT_MR_UDP       = 0x02;     /** Mode: UDP */
    static const unsigned char SKT_MR_MULTI     = 0x80;     /** Mode: support multicasting (with UDP only) */

    /* Interrupt flags that may be set in the socket's Interrupt register.*/
    static const unsigned char SKT_IR_SEND_OK  = (1<<4);    /** Interrupt Flag: Send has occured without errors */
    static const unsigned char SKT_IR_RECV     = (1<<2);    /** Interrupt Flag: Data has been received */

    /* Miscellaneous buffer sizes for the socket */
    static const int REG_SPACE_SIZE            = 0x30;      /** The amount of space to allocate for socket registers */
    static const int TX_BUFFER_SIZE            = 0x800;     /** The amount of space to allocate for the transmit buffer (2kB) */
    static const int RX_BUFFER_SIZE            = 0x800;     /** The amount of space to allocate for the receive buffer (2kB) */

    /*=================
     *  Member data
     =================*/
	unsigned char * regs;   /** pointer to the socket's register array    */
    unsigned char * rxbuf;  /** pointer to the simulated receive buffer   */
    unsigned char * txbuf;  /** pointer to the simulated transmit buffer  */
    int fd;                 /** file descriptor for the real socket       */
    int tcpfd;              /** file descriptor for accepted tcp connection */
    int rx_write_idx;       /** index into the rx buffer where new data will be placed */

    /*=================
     *  Private Member Functions
     =================*/
    void getFromRealSocket(int fd);                /* receive data from the real socket and place it in the rxBuffer */
    void loopback(const char*, int);               /* loop back a simulated response without using the real socket */
    void loopbackDhcpResponse(unsigned char*,int); /* loop back a Dhcp response without using the real socket */
    void loopbackNtpResponse(unsigned char*,int);  /* loop back an NTP response without using the real socket */
    void processCommand(unsigned char newCommand); /* process a command byte written to the socket */
    bool isDisconnected();                         /* return true if the real socket is disconnected */
    bool isDataReady();                            /* return true if the real socket has data available */
    bool isConnectionAvailable();                  /* return true if the real socket has a connection available */
public:
    /*=================
     *  Public Member Functions
     =================*/
    wiz_socket();                                   /* Constructor */
    ~wiz_socket();                                  /* Destructor */
    void setRegValue(unsigned int,unsigned char);   /* Set the specified socket register with the specified value */
    unsigned char getRegValue(unsigned int);        /* return the value of the specified socket register */
    void setRxBufferValue(unsigned int offset, unsigned char value); /* set the value at the specified Rx buffer offset */
    void setTxBufferValue(unsigned int offset, unsigned char value ); /* set the value at the specified Tx buffer offset */
    unsigned char getRxBufferValue(unsigned int);   /* get the value at the specified Rx buffer offset */
    unsigned char getTxBufferValue(unsigned int);   /* get the value at the specified Tx buffer offset */
    void reset();                                   /* reset the socket to its power-on state */
    void step();                                    /* advance the socket state, checking for received data or new/lost connections */
};

#endif
