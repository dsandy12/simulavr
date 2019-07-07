#include "pin.h"
#include "pinatport.h"
#include "simulationmember.h"
#include "avrdevice.h"
#include "net.h"

class hw_spi_avr : public AVRDevice{
	private:
	protected:


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
    portb(this, "B", true);
    
};

int main(){
hw_spi_avr *dev ;
PinAtPort(&portb, 14); //ss
PinAtPort(&portb, 15); //mosi
PinAtPort(&portb, 16); //miso
PinAtPort(&portb, 17); //clk
Net mosiNet=new Net();
mosiNet.Add(dev.getPin("mosipin");
Net misoNet=new Net();
misoNet.Add(dev.getPin("misopin");
Net ssNet=new Net();
ssNet.Add(dev.getPin("sspin");
Net clkNet=new Net();
clkNet.Add(dev.getPin("spickpin");
return 0;
}

