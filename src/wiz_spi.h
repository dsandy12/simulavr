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
        bool dataReady;                       /// is data ready from the master
};


#endif // WIZ_SPI_H
