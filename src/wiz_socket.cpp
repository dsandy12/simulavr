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

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include "wiz_socket.h"
using namespace std;

/* macros for reading and writing 16 bit socket registers */
#define RDREG16(portnum) ((((int)regs[portnum])<<8)+regs[portnum+1])
#define WRREG16(portnum,value) {regs[portnum]=(value)>>8; regs[portnum+1]=(value)&0xff;}

// special ports under 1024 that we are interested in (note that ports under 1024
// require root access).  In order to make these friendly with user level code,
// we will place them in a different port range.
#define	DHCP_SERVER_PORT	67	/* from server to client - REMAP TO 6967 (currently unassigned by iana */
#define DHCP_CLIENT_PORT	68	/* from client to server - REMAP to 6968 (currently unassigned by iana */
#define NTP_PORT            123 /* UDP port for NTP / SNTP - REMAP to 10123 (currently unassigned by iana */

/********************************************************************
 * wiz_socket()
 * The constructor for the wiz_socket class.  The socket is initialized
 * to power-on reset values and any dynamically allocated variables are
 * assigned memory from the heap.  The socket state is initialized as
 * "CLOSED".
 *
 * params:  None
 * returns: None
 */
wiz_socket::wiz_socket() :
        fd(0), tcpfd(0), rx_write_idx(0)
{
    regs = new unsigned char [REG_SPACE_SIZE];      // memory associated with the socket registers
    txbuf = new unsigned char   [TX_BUFFER_SIZE];   // memory associated with the transmit buffer
    rxbuf = new unsigned char   [RX_BUFFER_SIZE];   // memory associated with the receive buffer
    reset();
}

/********************************************************************
 * ~wiz_socket()
 * The destructor for the wiz_socket class.  Any dynamic variables are
 * deleted, and any open sockets are closed.
 *
 * params:  None
 * returns: None
 */
wiz_socket::~wiz_socket()
{
    if (regs) delete[] regs;
    if (txbuf) delete[] txbuf;
    if (rxbuf) delete[] rxbuf;
    if (tcpfd) close(tcpfd);
    if (fd) close(fd);
}

/********************************************************************
 * remapPort()
 * This function remaps the server ports for DHCP and NTP to addresses
 * above 1000.  This is required so that the program does not need
 * to run with root access - Linux protects binding to ports with
 * addresses less than 1000.
 * DHCP ports are remapped to 6967 and 6968.  Ntp is remapped to 10123.
 * All of these ports are currently unallocated by the IETF. All other
 * ports are not remapped.
 *
 * params:
 *    portnum (IN) - the port number that will be remapped.
 * returns:
 *    the remapped port number.
 */
unsigned int remapPort(int portnum) {
    if (portnum==DHCP_CLIENT_PORT) return portnum+6900;
    if (portnum==DHCP_SERVER_PORT) return portnum+6900;
    if (portnum==NTP_PORT) return portnum+10000;
    return portnum;
}

/********************************************************************
* processCommand()
* This function processes changes in the command register value.
* When the command register is written, the socket operational state
* must be updated based on the current mode and status of the socket.
* Changes might include: socket setup, teardown, and transferring of
* data.
*
* params:
*    newCommand (IN) - the newest command written to the command register
* returns: NONE
* changes:
*    the socket state will be updated based on the command.
*/
void wiz_socket::processCommand(unsigned char newCommand) {
    struct sockaddr_in remote_addr;
    struct sockaddr_in local_addr;
    int portno = remapPort(RDREG16(PORT_OFFSET));
    int dportno = remapPort(RDREG16(DPORT_OFFSET));
    unsigned char buffer[TX_BUFFER_SIZE];
    int tx_start;
    int tx_end;
    int len;
    int optval;

    switch (regs[MR_OFFSET]&0x0F) {
        case SKT_MR_TCP:
            // depending on the connection state, valid TCP Commands are:
            // OPEN, LISTEN, CONNECT, DISCONNECT, CLOSE, SEND, RECV, SEND_KEEP (not used)
            switch (regs[SR_OFFSET]) {
                case SKT_SR_CLOSED:
                    // accept open commands
                    if ((newCommand)==SKT_CR_OPEN) {
                        // open the new socket connection
                        if (tcpfd>0) close(tcpfd);
                        if (fd>0) close(fd);
                        fd = socket(AF_INET, SOCK_STREAM, 0);
                        if (fd>0) {
                            optval = 1;
                            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

                            regs[SR_OFFSET] = SKT_SR_INIT;
                            regs[CR_OFFSET] = 0;

                            WRREG16(RX_RSR_OFFSET, 0x0000);
                            WRREG16(RX_RD_OFFSET,0x0000);
                            rx_write_idx = 0x0000;

                            WRREG16(TX_RR_OFFSET,0x0000);
                            WRREG16(TX_FSR_OFFSET,0x0800);
                            WRREG16(TX_WR_OFFSET,0x0000);
                        }
                    }
                    break;
                case SKT_SR_INIT:
                    // here after an open command is sent.  Accept
                    // close, Listen or Connect commands
                    switch (newCommand) {
                        case SKT_CR_CLOSE:
                            if (tcpfd) close(tcpfd);
                            close(fd);
                            regs[SR_OFFSET] = SKT_SR_CLOSED;
                            regs[CR_OFFSET] = 0;
                            break;
                        case SKT_CR_CONNECT:
                            // bind the socket to the remote host
                            memset((char *) &remote_addr, 0, sizeof(remote_addr));
                            remote_addr.sin_family = AF_INET;
                            memcpy(&remote_addr.sin_addr, &regs[DIPR_OFFSET],4);
                            remote_addr.sin_port = htons(dportno);
                            if (connect(fd,(struct sockaddr *) &remote_addr,sizeof(remote_addr)) < 0) break;
                            regs[SR_OFFSET] = SKT_SR_ESTABLISHED;
                            regs[CR_OFFSET] = 0;
                            break;

                        case SKT_CR_LISTEN:
                            // place the socet in passive listen mode
                            // Periodic polling will detect if a remote
                            // client has connected.
                            memset((char *) &local_addr, 0, sizeof(local_addr));
                            portno = RDREG16(PORT_OFFSET);  // port to listen on
                            local_addr.sin_family = AF_INET;
                            local_addr.sin_addr.s_addr = htonl(0x7f000064);  // ip address to listen on (127.0.0.100}
                            local_addr.sin_port = htons(portno);
                            if (bind(fd, (struct sockaddr *) &local_addr,sizeof(local_addr)) < 0) break;
                            listen(fd,5);
                            regs[SR_OFFSET] = SKT_SR_LISTEN;
                            regs[CR_OFFSET] = 0;
                            break;
                    }
                    break;
                case SKT_SR_LISTEN:
                    // here if the socket is listening for a connection
                    // once the connection is established, go to established.
                    // only close is a valid command here.
                    if ((newCommand)== SKT_CR_CLOSE) {
                        if (tcpfd) close(tcpfd);
                        close(fd);
                        regs[SR_OFFSET] = SKT_SR_CLOSED;
                        regs[CR_OFFSET] = 0;
                    }
                    break;
                case SKT_SR_ESTABLISHED:
                    // connection is established between the two endpoints.
                    // valid commands are SEND,  RECV, CLOSE and DISCONNECT
                    switch (newCommand) {
                        case SKT_CR_DISCON:
                        case SKT_CR_CLOSE:
                            if (tcpfd) close(tcpfd);
                            close(fd);
                            regs[SR_OFFSET] = SKT_SR_CLOSED;
                            regs[CR_OFFSET] = 0;
                            break;
                        case SKT_CR_SEND:
                            // SEND DATA FROM THE TRANSMIT BUFFER - UPDATE POINTERS WHEN DONE

                            // get the write pointers
                            tx_start = RDREG16(TX_RR_OFFSET);
                            tx_end = RDREG16(TX_WR_OFFSET);

                            if (tx_end<tx_start) {
                                memcpy(buffer,txbuf+tx_start,TX_BUFFER_SIZE-tx_start);
                                memcpy(buffer+(TX_BUFFER_SIZE-tx_start),txbuf,tx_end);
                                len = tx_end+TX_BUFFER_SIZE-tx_start;
                            } else {
                                memcpy(buffer,txbuf+tx_start,tx_end-tx_start);
                                len = tx_end-tx_start;
                            }
                            if (tcpfd) {
                                write(tcpfd,buffer,len);
                            } else {
                                write(fd,buffer,len);
                            }
                            regs[CR_OFFSET] = 0;

                            // set the transmit pointers and free size
                            tx_start = (tx_start+len)&(TX_BUFFER_SIZE-1);
                            WRREG16(TX_RR_OFFSET,tx_start);
                            WRREG16(TX_FSR_OFFSET,TX_BUFFER_SIZE);

                            // set the ir flag
                            regs[IR_OFFSET] |= SKT_IR_SEND_OK;

                            break;

                        case SKT_CR_RECV:
                            // Set the new value of the RSR register
                            len = rx_write_idx + RX_BUFFER_SIZE - RDREG16(RX_RD_OFFSET);
                            while (len>=RX_BUFFER_SIZE) len-=RX_BUFFER_SIZE;
                            WRREG16(RX_RSR_OFFSET,len);
                            regs[CR_OFFSET] = 0;
                            break;
                    }
                    break;
                default:
                    // do nothing
                    break;
            }
            break;
        case SKT_MR_UDP:
            // UDP is connectionless.  Valid commands are Open, Close, Send, Seend_MAC, RECV
            // depending on the connection state, valid TCP Commands are:
            // OPEN, LISTEN, CONNECT, DISCONNECT, CLOSE, SEND, RECV, SEND_KEEP (not used)
            switch (regs[SR_OFFSET]) {
                case SKT_SR_CLOSED:
                    // accept open commands
                    if ((newCommand)==SKT_CR_OPEN) {
                        // open the new socket connection
                        if (fd>0) close(fd);
                        fd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
                        if (fd>0) {
                            optval = 1;
                            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

                            // multicast?
                            if (regs[MR_OFFSET]&0x80) {
                                /* Set local interface for outbound multicast datagrams. */
                                /* The IP address specified must be associated with a local, */
                                /* multicast capable interface. */
                                struct in_addr localInterface;
                                memset(&localInterface,0,sizeof(in_addr));
                                memcpy(&localInterface.s_addr,&regs[DIPR_OFFSET],4);
                                setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface));
                            }

                            // bind the local address and local port for receiving packets.  Since this is a simulated
                            // socket, we know that the outbound ip address will always be assigned to 127.0.0.100
                            memset((char *)&local_addr, 0, sizeof(local_addr));
                            local_addr.sin_family = AF_INET;
                            local_addr.sin_addr.s_addr = htonl(0x7F000064UL);
                            local_addr.sin_port = htons(remapPort(RDREG16(PORT_OFFSET)));
                            if (bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
                                break;
                            } else {
                                // bind succeeded
                                regs[SR_OFFSET] = SKT_SR_UDP;
                                regs[CR_OFFSET] = 0;
                            }
                        }
                    }
                    break;
                case SKT_SR_UDP:
                    // valid commands are SEND,  RECV, CLOSE and DISCONNECT
                    switch (newCommand) {
                        case SKT_CR_DISCON:
                        case SKT_CR_CLOSE:
                            close(fd);
                            regs[SR_OFFSET] = SKT_SR_CLOSED;
                            regs[CR_OFFSET] = 0;
                            break;
                        case SKT_CR_SEND:
                            // SEND DATA FROM THE TRANSMIT BUFFER - UPDATE POINTERS WHEN DONE
                            // get the write pointers
                            tx_start = RDREG16(TX_RR_OFFSET);
                            tx_end = RDREG16(TX_WR_OFFSET);

                            if (tx_end<tx_start) {
                                memcpy(buffer,txbuf+tx_start,TX_BUFFER_SIZE-tx_start);
                                memcpy(buffer+(TX_BUFFER_SIZE-tx_start),txbuf,tx_end);
                                len = tx_end+TX_BUFFER_SIZE-tx_start;
                            } else {
                                memcpy(buffer,txbuf+tx_start,tx_end-tx_start);
                                len = tx_end-tx_start;
                            }

                            // Special processing for DHCP and NTP packets
                            if (RDREG16(DPORT_OFFSET)==DHCP_SERVER_PORT) {
                                // process a DHCP server request
                                loopbackDhcpResponse(buffer,len);  // DHCP Offer
                            } else if (RDREG16(DPORT_OFFSET)==NTP_PORT) {
                                // process an NTP request
                                loopbackNtpResponse(buffer,len);
                            } else {
                                memset((char*)&remote_addr, 0, sizeof(remote_addr));
                                remote_addr.sin_family = AF_INET;
                                remote_addr.sin_port = htons(dportno); // destination port
                                memcpy((void *)&remote_addr.sin_addr, &regs[DIPR_OFFSET], 4);
                                sendto(fd, buffer, len, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
                            }
                            regs[CR_OFFSET] = 0;

                            // set the transmit pointer and free size
                            tx_start = (tx_start+len)&(TX_BUFFER_SIZE-1);
                            WRREG16(TX_RR_OFFSET,tx_start);
                            WRREG16(TX_FSR_OFFSET,TX_BUFFER_SIZE);

                            // set the ir flag
                            regs[IR_OFFSET] |= SKT_IR_SEND_OK;
                            break;
                        case SKT_CR_RECV:
                            // Set the new value of the RSR register
                            len = rx_write_idx + RX_BUFFER_SIZE - RDREG16(RX_RD_OFFSET);
                            while (len>=RX_BUFFER_SIZE) len-=RX_BUFFER_SIZE;
                            WRREG16(RX_RSR_OFFSET,len);
                            regs[CR_OFFSET] = 0;
                            break;
                    }
                    break;
                default:
                    // do nothing
                    break;
            }
            break;
        default:  // do nothing, the socket mode is not supported
            break;
    }
}

/********************************************************************
 * setRegValue()
 * set the value of the specified socket register to the specified value.
 * If changing the register requires special processing, handle that too.
 *
 * params:
 *    regnum (IN) - the register address of the register to set
 *    value (IN) - the new value to write to the register
 * returns: None
 */
void wiz_socket::setRegValue(unsigned int regnum, unsigned char value) {
    int tx_start;
    int tx_end;
    int len;
    switch (regnum) {
        case MR_OFFSET:
            // Update the value.  if a connection is open, close the connection
            regs[regnum] = value;
            if (fd) {close(fd); fd=0;}
            break;
        case CR_OFFSET:
            // changes to the command register will invoke changes in the
            // connection state machine for the real socket.
            processCommand(value);
            break;
        case IR_OFFSET:
            // clear interrupt flags for any bit set to 1 in the new value;
            regs[regnum] &= ((~value)|(0xE0));
            break;
        case SR_OFFSET:
            // do nothing - this register is read only
            break;
        case PORT_OFFSET:
        case PORT_OFFSET+1:
            regs[regnum] = value;
            break;
        case DHAR_OFFSET:
        case DHAR_OFFSET+1:
        case DHAR_OFFSET+2:
        case DHAR_OFFSET+3:
        case DHAR_OFFSET+4:
        case DHAR_OFFSET+5:
            regs[regnum] = value;
            break;
        case DIPR_OFFSET:
        case DIPR_OFFSET+1:
        case DIPR_OFFSET+2:
        case DIPR_OFFSET+3:
            regs[regnum] = value;
            break;
        case DPORT_OFFSET:
        case DPORT_OFFSET+1:
            regs[regnum] = value;
            break;
        case TX_FSR_OFFSET:
        case TX_FSR_OFFSET+1:
            // do nothing.  THis register is read only
            break;
        case TX_RR_OFFSET:
        case TX_RR_OFFSET+1:
            // do nothing.  This register is read only
            break;
        case TX_WR_OFFSET:
            regs[regnum] = value&((TX_BUFFER_SIZE-1)>>8);
            tx_start = RDREG16(TX_WR_OFFSET);
            tx_end = RDREG16(TX_RR_OFFSET);
            len = (tx_end + TX_BUFFER_SIZE - tx_start)&(TX_BUFFER_SIZE-1);
            WRREG16(TX_FSR_OFFSET,len);
            break;
        case TX_WR_OFFSET+1:
            regs[regnum] = value;
            tx_start = RDREG16(TX_WR_OFFSET);
            tx_end = RDREG16(TX_RR_OFFSET);
            len = (tx_end + TX_BUFFER_SIZE - tx_start)&(TX_BUFFER_SIZE-1);
            WRREG16(TX_FSR_OFFSET,len);
            break;
        case RX_RSR_OFFSET:
        case RX_RSR_OFFSET+1:
            // do nothing. This register is read only
            break;
        case RX_RD_OFFSET:
            regs[regnum] = value&((RX_BUFFER_SIZE-1)>>8);
            break;
        case RX_RD_OFFSET+1:
            regs[regnum] = value&0xff;
            break;
        default:
            break;
    }
}

/********************************************************************
 * getRegValue()
 * Return the value of the specified register.
 *
 * params:
 *     regnum (IN) - the address of the register to read.
 * returns:
 *     the value read from the specified register.
 */
unsigned char wiz_socket::getRegValue(unsigned int regnum) {
    if (regnum>=REG_SPACE_SIZE) return 0;
    return regs[regnum];
}

/********************************************************************
 * setRxBufferValue()
 * Set the value of the specified rx buffer location to a specified value.
 *
 * params:
 *     bufferoffset (IN) - the address within the buffer to write.
 *     value (IN) - the new value to write to the buffer.
 * returns: None
 */
void wiz_socket::setRxBufferValue(unsigned int buffoffset,unsigned char value) {
    if (buffoffset>=RX_BUFFER_SIZE) return;
    rxbuf[buffoffset]=value;
}

/********************************************************************
 * getRxBufferValue()
 * Return the value of the specified rx buffer location.
 *
 * params:
 *     bufferoffset (IN) - the address within the buffer to read.
 * returns:
 *     the value read from the specified buffer location.
 */
unsigned char wiz_socket::getRxBufferValue(unsigned int buffoffset) {
    if (buffoffset>=RX_BUFFER_SIZE) return 0;
    return rxbuf[buffoffset];
}

/********************************************************************
 * setTxBufferValue()
 * Set the value of the specified tx buffer location to a specified value.
 *
 * params:
 *     bufferoffset (IN) - the address within the buffer to write.
 *     value (IN) - the new value to write to the buffer.
 * returns: None
 */
void wiz_socket::setTxBufferValue(unsigned int buffoffset,unsigned char value) {
    if (buffoffset>=TX_BUFFER_SIZE) return;
    txbuf[buffoffset]=value;
}

/********************************************************************
 * getTxBufferValue()
 * Return the value of the specified rx buffer location.
 *
 * params:
 *     bufferoffset (IN) - the address within the buffer to read.
 * returns:
 *     the value read from the specified buffer location.
 */
unsigned char wiz_socket::getTxBufferValue(unsigned int buffoffset) {
    if (buffoffset>=TX_BUFFER_SIZE) return 0;
    return txbuf[buffoffset];
}

/********************************************************************
* Reset()
*
* Reset the socket to its initial state, closing any connections
* and setting all registers to their power-on values.
*
* Parmaters: None
* Returns: Nothing
* Changes: closes any existing socket connection and resets the
*          socket's registers to their power-on values.
*/
void wiz_socket::reset() {
    /* register power on reset values */
    static const unsigned char POR_VALUES[REG_SPACE_SIZE] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02,
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x40, 0x00, 0x00
    };

    if (fd) close(fd);
    for (int i=0;i<REG_SPACE_SIZE;i++) regs[i] = POR_VALUES[i];
}

/********************************************************************
* getFromRealSocket()
* read data from the real socket and place the date in the simulated
* device's receive buffer.
*
* params:
*    fd (IN) - the file descriptor of the real socket to read from
* returns: None
* changes:
*    on exit, data will be placed in the simulated receive buffer and
*    the rx buffer's write pointer, interrupt flag, and number of
*    received bytes will be updated.
*/
void wiz_socket::getFromRealSocket(int fd) {
    char buffer[RX_BUFFER_SIZE];
    int bytes_read = read(fd,(void*)buffer,RX_BUFFER_SIZE);
    if (bytes_read<=0) return;
    if (bytes_read+rx_write_idx>=RX_BUFFER_SIZE) {
        int firstLen = RX_BUFFER_SIZE - rx_write_idx;
        memcpy(&rxbuf[rx_write_idx],buffer,firstLen);
        memcpy(&rxbuf,&buffer[firstLen],bytes_read-firstLen);
    } else {
        memcpy(&rxbuf[rx_write_idx],buffer,bytes_read);
    }
    // update the write pointer
    rx_write_idx = (rx_write_idx+bytes_read)&(RX_BUFFER_SIZE-1);

    // update the receive size
    WRREG16(RX_RSR_OFFSET,RDREG16(RX_RSR_OFFSET)+bytes_read);

    // update the interrupt flag
    regs[IR_OFFSET] |= SKT_IR_RECV;
}

/********************************************************************
* loopback()
* insert data into the rx buffer and update the registers
* as if data was received on the socket interface.
*
* params:
*    buffer (IN) - a pointer to the data to copy into the socket's rx
*                  buffer.
*    bufsize (IN) - the number of bytes to copy into the rx buffer.
* returns: None
* changes:
*    on exit, data will be placed in the simulated receive buffer and
*    the rx buffer's write pointer, interrupt flag, and number of
*    received bytes will be updated.
*/
void wiz_socket::loopback(const char *buffer, int bufsize) {
    if (bufsize<=0) return;
    if (bufsize+rx_write_idx>=RX_BUFFER_SIZE) {
        int firstLen = RX_BUFFER_SIZE - rx_write_idx;
        memcpy(&rxbuf[rx_write_idx],buffer,firstLen);
        memcpy(&rxbuf,&buffer[firstLen],bufsize-firstLen);
    } else {
        memcpy(&rxbuf[rx_write_idx],buffer,bufsize);
    }
    // update the write pointer
    rx_write_idx = (rx_write_idx+bufsize)&(RX_BUFFER_SIZE-1);

    // update the receive size
    WRREG16(RX_RSR_OFFSET,RDREG16(RX_RSR_OFFSET)+bufsize);

    // update the interrupt flag
    regs[IR_OFFSET] |= SKT_IR_RECV;
}

/********************************************************************
* loopbackDhcpResponse()
* based on the DHCP message passed in the first paramter, send the
* appropriate DHCP response.  If the message is a DHCP_DISCOVER, then
* the response will be a DHCP offer.  If the message is a DHCP request,
* the response will be a DHCP ack.
* Responses will be looped back to the virtual socket without being
* forwarded to the real socket.  This allows for a virtual DHCP service
* without the need of setting up the host environment for DHCP.
* the network address assigned by the Virtual DHCP server will always
* be 127.0.0.100 (a local loopback address).
*
* params:
*    buffer (IN) - a pointer to the DHCP message to respond to
*    len (IN) - the length of the DCHP message to respond to
* returns: NONE
* changes:
*    upon exit, the DHCP response will be placed in the simulated
*    socket's rx buffer.
*/
void wiz_socket::loopbackDhcpResponse(unsigned char *buffer,int len) {
    dhcppacket packet;
    if (len<0) return;
    if ((unsigned int)len<sizeof(packet)-8) {
        memcpy(&packet.op,buffer,len);
    } else {
        memcpy(&packet.op,buffer,sizeof(packet)-8);
    }
    // header - dhcp server address
    packet.header[0] = 127; packet.header[1]=0; packet.header[2]=0; packet.header[3]=0;
    // header -dhcp server port
    packet.header[4] = (DHCP_SERVER_PORT>>8);
    packet.header[5] = (DHCP_SERVER_PORT&0xff);
    // boot reply
    packet.op = 2;  // BOOTREPLY
    // leased address
    packet.yiaddr[0] = 127; packet.yiaddr[1]=0; packet.yiaddr[2]=0; packet.yiaddr[3]=100;
    // server address
    packet.siaddr[0] = 127; packet.siaddr[1]=0; packet.siaddr[2]=0; packet.siaddr[3]=0;
    // response type
    // TODO: A more robust method of determining the initial DHCP message type should be implemented
    // this code assumes that the first option after the magic cookie is the message tyupe.
    if (packet.option1[2] == 1) {
        // send DHCP offer
        packet.option1[0] = 53; packet.option1[1] = 1; packet.option1[2] = 2;
    } else {
        // Send DHCP Ack
        packet.option1[0] = 53; packet.option1[1] = 1; packet.option1[2] = 5;
    }
    // subnet
    packet.option2[0] = 1;  packet.option2[1] = 4; packet.option2[3] = 255; packet.option2[4] = 255; packet.option2[5] = 255; packet.option2[6] = 0;
    // router
    packet.option3[0] = 3;  packet.option3[1] = 4; packet.option3[3] = 127; packet.option3[4] = 0; packet.option3[5] = 0; packet.option3[6] = 1;
    // DHCP Server
    packet.option4[0] = 54;  packet.option4[1] = 4; packet.option4[3] = 127; packet.option4[4] = 0; packet.option4[5] = 0; packet.option4[6] = 1;
    // send the response
    loopback((char*)&packet,sizeof(packet));
}

/********************************************************************
* loopbackNtpResponse()
* Responses will be looped back to the virtual socket without being
* forwarded to the real socket.  This allows for a virtual NTP service
* without the need of setting up the host environment for NTP.
*
* params:
*    buffer (IN) - a pointer to the NTP message to respond to
*    len (IN) - the length of the NTP message to respond to
* returns: NONE
* changes:
*    upon exit, the NTP response will be placed in the simulated
*    socket's rx buffer.
*/
void wiz_socket::loopbackNtpResponse(unsigned char *buffer,int len) {
    char outbuf[sizeof(ntp_packet)+8];
    ntp_packet *packet = (ntp_packet*)(outbuf+8);
    if (len<0) return;
    if ((unsigned int)len!=sizeof(ntp_packet)) {
        return;
    }

    memcpy(packet,buffer,sizeof(ntp_packet));

    // set the timestamp - this is the only field that the NTP client checks
    // the addition is to add the number of seconds from 1/1/1970 to the linux epoch.
    packet->tx_timestamp[0] = htonl(time(0)+2208988800UL);
    packet->tx_timestamp[1] = 0;
    *((uint32_t *)(outbuf)) = htonl(0x7f000064ul);
    *((uint16_t*)(outbuf+4)) = htons(NTP_PORT);
    *((uint16_t*)(outbuf+6)) = htons(len);

    // send the response
    loopback(outbuf,sizeof(ntp_packet)+8);
}

/********************************************************************
* Step()
*
* called at each simulation step to see if the socket state has
* changed.  In particular, the following events could have occured:
*
* An open TCP host socket could have been disconnected by the client
* A listening TCP host stocket could have connected with a client
* An open TCP host socket could have received new data
* An open UDP socket could have new data available
*
*/
void wiz_socket::step() {
    if ((regs[SR_OFFSET]==SKT_SR_LISTEN)&&(isConnectionAvailable())) {
        struct sockaddr_in clientname;
        unsigned int sizesock;
        sizesock = sizeof (clientname);
        tcpfd = accept (fd,(struct sockaddr *) &clientname, &sizesock);
        if (tcpfd>0) {
            // connection has been established
            regs[SR_OFFSET] = SKT_SR_ESTABLISHED;

            // set the destination ip and port
            regs[DIPR_OFFSET]   = ntohl(clientname.sin_addr.s_addr)>>24;
            regs[DIPR_OFFSET+1] = (ntohl(clientname.sin_addr.s_addr)>>16)&0xff;
            regs[DIPR_OFFSET+2] = (ntohl(clientname.sin_addr.s_addr)>>8)&0xff;
            regs[DIPR_OFFSET+3] = (ntohl(clientname.sin_addr.s_addr)>>0)&0xff;
            WRREG16(DPORT_OFFSET,ntohs(clientname.sin_port));
            return;
        } else {
            tcpfd=0;
        }
    }
    if ((regs[SR_OFFSET]==SKT_SR_ESTABLISHED)&&(isDisconnected())) {
        // port has disconnected - set the state in the state register
        regs[SR_OFFSET] = SKT_SR_CLOSED;
        return;
    }
    if ((regs[SR_OFFSET]==SKT_SR_ESTABLISHED)&&(isDataReady())) {
        // receive data
        if (tcpfd) {
            getFromRealSocket(tcpfd);
        } else {
            getFromRealSocket(fd);
        }
        return;
    }
    if ((regs[SR_OFFSET]==SKT_SR_UDP)&&(isDataReady())) {
        // receive data
        getFromRealSocket(fd);
        return;
    }
}

/********************************************************************
* isDisconnected()
* return true if the real socket has disconnected.
*/
bool wiz_socket::isDisconnected()
{
    if (!fd) return true;   // socket is closed

    struct timeval tv;
    fd_set  fd_set;

    // use select to see if there the port is ready - a disconnected
    // port will return true;
    tv.tv_sec  = 0;
    tv.tv_usec = 1;

    FD_ZERO(&fd_set);
    FD_SET(fd, &fd_set);
    if (tcpfd) {
        FD_SET(tcpfd, &fd_set);
    }

    if (select(FD_SETSIZE, &fd_set, 0, 0, &tv)==0) return false;

    // now check the IO controller to see if there are any bytes ready
    // if not, the port is disconnected.
    long bytes = 0;
    if (tcpfd) {
        if (ioctl(tcpfd, FIONREAD, &bytes)<0) return true;
    } else {
        if (ioctl(fd, FIONREAD, &bytes)<0) return true;
    }

    return (bytes==0);
}

/********************************************************************
* isDataReady()
* return true if the real socket has data ready.  This function can
* be polled to avoid the socket blocking on read.
*/
bool wiz_socket::isDataReady()
{
    //if (!descriptor) return true;   // socket is closed
    int descriptor = (tcpfd>0)?tcpfd:fd;
    if (descriptor<=0) return false;

    struct timeval tv;
    fd_set  fd_set;

    // use select to see if there the port is ready - a disconnected
    // port will return true;
    tv.tv_sec  = 0;
    tv.tv_usec = 1;

    FD_ZERO(&fd_set);
    FD_SET(fd, &fd_set);
    if (descriptor!=fd) FD_SET(descriptor,&fd_set);

    if (select(FD_SETSIZE, &fd_set, 0, 0, &tv)==0) return false;

    // now check the IO controller to see if there are any bytes ready
    // if not, the port is disconnected.
    long bytes = 0;
    if (ioctl(descriptor, FIONREAD, &bytes)<0) return false;
    return (bytes>0);
}

/********************************************************************
* isConnectionAvailable()
* return true if the real socket has a connection available.  This
* function can be polled in order to avoid the real socket blocking
* on connect.
*/
bool wiz_socket::isConnectionAvailable() {
    struct timeval tv;
    fd_set  fd_set;

    // use select to see if there the port is ready - a disconnected
    // port will return true;
    tv.tv_sec  = 0;
    tv.tv_usec = 1;

    FD_ZERO(&fd_set);
    FD_SET(fd, &fd_set);

    return (select(FD_SETSIZE, &fd_set, 0, 0, &tv)>0);
}
