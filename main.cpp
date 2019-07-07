    #include "../spi.h"
    #include "../delay.h"
    #include "pinatport.h"
    #include "simulationmember.h"
    #include "avrdevice.h"
    #include "net.h"
    #include "wiz_ethernet.h"
    /* macro definitions for hardware registers used by these functions */
    #define SREG  (*((volatile unsigned char *)0x5F))
    #define DDRB (*((volatile unsigned char *)0x24))
    #define PORTB (*((volatile unsigned char *)0x25))
    #define SPCR0 (*((volatile unsigned char *)0x4C))
    #define SPSR0 (*((volatile unsigned char *)0x4D))
    #define SPDR0 (*((volatile unsigned char *)0x4E))
    /* pin definitions for SPI for ATMEGA 328P */
    #define SSBIT   2
    #define MOSIBIT 3
    #define MISOBIT 4
    #define SCKBIT  5
    static unsigned char locked = 0;
    /************************************************************************
    * spi_init()
    *
    * Set the ATMGEA register pins for the proper directions to use the SPI
    * bus.
    *
    * changes:
    *     the state of Port B output values and direction bits so that
    *     MOSI, SS and CLK are outputs.  SS is set to high in order to
    *     disable any connected devices.
    */
    void spi_init()
    {
      /* disable interrupts during initialization */
      unsigned char sreg = SREG;
      __builtin_avr_cli();
      /* Set SS to high so a connected chips will be "deselected" */
      if ((DDRB&(1<<SSBIT))==0) PORTB |= (1<<SSBIT);
      /* set SS as an output */
      DDRB |= (1<<SSBIT);
      /* Enable SPI in master mode */
      SPCR0 |= (1<<MSTR0);
      SPCR0 |= (1<<SPE0);
      /* Set SCK and MOSI pins as output. */
      DDRB |= (1<<SCKBIT);
      DDRB |= (1<<MOSIBIT);
      /* unlock the spi hardware for use */
      locked = 0;
      /* restore the state of the global interrupt flag */
      SREG = sreg;
    }
    /************************************************************************
    * spi_is_locked()
    *
    * Returns true if the spi bus is in the process of a transfer (begin_transfer
    * has been called and end_transfer has not yet been called).  This
    * function can be used if multiple devices may use the spi bus in order
    * to ensure that they do not corrupt the other's data.
    *
    * returns:
    *     1 if begin_transfer has been called and end_transfer has not yet been
    *     called.  Otherwise, returns 0.
    */
    unsigned char spi_is_locked()
    {
        return locked;
    }
    /************************************************************************
    * spi_begin_transfer()
    *
    * If not locked, program the SPI bus controller so that its configuration
    * matches the requirements given for a specific device.  The SPI bus will
    * then be locked from further configuration until spi_end_transaction()
    * occurs.
    *
    * parametres:
    *     spcr - the bit value to program into the SPI bus configuration register
    *     spsr - the bit value to program into the SPI bus status register
    * changes:
    *     the spi bus controller will be reconfigured using the values of spcr
    *     and spsr.  The lock flag will be set.
    */
    void spi_begin_transfer(unsigned char spcr,unsigned char spsr)
    {
        if (locked) return;
        unsigned char sreg = SREG;
        __builtin_avr_cli();
        SPCR0 = spcr;
        SPSR0 = spsr;
        locked = 1;
        SREG = sreg;
        /* Set SS to LOW so a connected chips will be "selected" */
        PORTB &= (~(1<<SSBIT));
    }
    /************************************************************************
    * spi_transfer()
    *
    * write a byte to the SPI bus on the MOSI pin and read a byte on the
    * MISO pin.
    *
    * parameters:
    *     data - the byte of data to write out on MOSI.
    * returns:
    *     the data received on the MISO pin.
    * changes:
    *     the state of devices on the SPI bus may be altered based on this
    *     function.
    */
    unsigned char spi_transfer(unsigned char data)
    {
       SPDR0 = data;
        /* the following NOP adds a small delay an allows for the loop to
        * potentially be skipped.  This trick was observed int the Arduino
        * library code
        */
       asm volatile("nop");
       /* wait for the transfer to complete */
       while (!(SPSR0 & (1<<SPIF0))) ;
       return SPDR0;
    }
    /************************************************************************
    * spi_end_transfer()
    *
    * unlock configuration of the spi bus so that it can be used for different
    * device.
    *
    * changes:
    *    The lock flag will be cleared.
    */
    void spi_end_transfer(void)
    {
        /* Set SS to high so a connected chips will be "deselected" */
        PORTB |= (1<<SSBIT);
        locked = 0;
    }

class hw_spi_avr : public AVRDevice{

public:
    // SPI port pins
    Pin sspin;
    Pin misopin;
    Pin mosipin;
    Pin spickpin;
	//SPI nets
    Net mosiNet;    
    Net misoNet;   
    Net ssNet;    
    Net clkNet;
};

class w_spi_ethernet : public wiz_ethernet{
	
	public:
    // SPI port pins
    Pin sspin;
    Pin misopin;
    Pin mosipin;
    Pin spickpin;
//SPI nets
    Net mosiNet;    
    Net misoNet;   
    Net ssNet;    
    Net clkNet;
   

};

int main(){
hw_spi_avr *dev ;
Net mosiNet=new Net();
mosiNet.Add(dev.getPin("mosipin");
Net misoNet=new Net();
misoNet.Add(dev.getPin("misopin");
Net ssNet=new Net();
ssNet.Add(dev.getPin("sspin");
Net clkNet=new Net();
clkNet.Add(dev.getPin("spickpin");
hw_spi_avr *eth ;
Net mosiNet=new Net();
mosiNet.Add(eth.getPin("mosipin");
Net misoNet=new Net();
misoNet.Add(eth.getPin("misopin");
Net ssNet=new Net();
ssNet.Add(eth.getPin("sspin");
Net clkNet=new Net();
clkNet.Add(eth.getPin("spickpin");
return 0;
}
