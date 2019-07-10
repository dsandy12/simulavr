#include "wiz_spi.h"
#include "wiz_ethernet.h"

/********************************************************************
 * wiz_spi()
 * constructor. Initialize the controller.
 *
 * parameters:
 *    chip - a pointer the ethernet object that contains this spi
 *           controller
 */
wiz_spi::wiz_spi(wiz_ethernet *chip)
{
    eth = chip;
    *(eth->getMosiPin()) = 't';
    *(eth->getSsPin()) = 't';
    *(eth->getSckPin()) = 't';
    dataReady = false;
}

/********************************************************************
 * isSsSet()
 * return true if the chip's ss pin is LOW (slave state is active low),
 * return false.
 */
bool wiz_spi::isSsSet()
{
    if (!eth) return false;
    return (!(bool)*(eth->getSsPin()));
}

/********************************************************************
* isDataReady()
* return true if data is ready to be received by the transaction layer
*/
bool wiz_spi::isDataReady()
{
    return dataReady;
}

/********************************************************************
* getByte()
* receive the data byte from the spi master - this function is called
* by the transaction layer.
*/
unsigned char wiz_spi::getByte() {
    dataReady = false;  // reset the state of the data ready flag
    return data;
}

/*******************************************************************
* setOutputByte()
* called by the transaction layey - set the data byte that will be
* sent to the master.
*/
void wiz_spi::setOutputByte(unsigned char ch)  {
    dataOut = ch;
};

/*******************************************************************
* reset()
* reset the state of the spi controller - called by chip reset.
*/
void wiz_spi::reset()
{
    dataReady = false;
}

/*******************************************************************
* step()
* update the state of the spi based on the input pins.
*/
void wiz_spi::step()
{
    static unsigned int bitsRead = 0;
    static unsigned int shiftOut = 0;
    static unsigned int shiftCount = 0;
    static bool lastClk = false;

    if (!isSsSet()) {
        bitsRead = 0;
        shiftCount = 0;
        shiftOut = dataOut;
        lastClk = false;
        return;
    }

    // here if Ss is set - look for a clock transition from low to high
    bool clk = *(eth->getSckPin());
    if ((clk)&&(clk!=lastClk)) {
        // a rising clock edge has been detected shift the mosi bit into datain
        bitsRead = bitsRead<<1;
        shiftOut = 0xff&(shiftOut<<1);
        if ((bool)*(eth->getMosiPin())) bitsRead |=1;
        shiftCount ++;
    } else if (!clk) {
        // clock is low - make sure that the miso bit is being sent
        if (shiftCount==0) shiftOut = dataOut;
        if (shiftOut&0x80) {
            eth->getMisoPin()->outState = Pin::HIGH;
        } else {
            eth->getMisoPin()->outState = Pin::LOW;
        }
    }
    lastClk = clk;

    if (shiftCount == 8) {
        // all bits for the byte of been received - reset for the next byte,
        // and signal that data is ready
        data = bitsRead;
        bitsRead = 0;
        shiftCount = 0;
        dataReady = true;
    }
}
