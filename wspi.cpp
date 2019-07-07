#include "wiz_ethernet.h"
#include "simulationmember.h"
#include "pin.h"
#include "net.h"
#include <string>
//four functions to check data send and receive works properly or not
int bytearr[8];int statusflag=0,buffer[8];
//initially the arr elements are set to 99 so that it can be distinguished from the data entry of 0 and 1 bits
for(int i=0;i<8;i++){
	bytearr[i]=99;
}
//these functions would be called from the main function after sending and receiving data to check the status
bool is_Rx_byte_Ready(int bytearr[8]){
	
	for(int i=0;i<8;i++){
		if(bytearr[i]==99)
		statusflag=1;
}
if(statusflag==1)
return false;
//receive again code needs to be implemented
else
return true;
}

char get_Rx_byte(int i){
//if all 8 bits received properly store them in a buffer
if (statusflag==0)
for(int i=0;i<8;i++){
	cin>>buffer[i];
}
}

void set_Tx_byte(){

}

bool is_SS_asserted(string opcode){
	if(opcode=="OF")
	//write operation code needs to be inserted
	if(opcode=="FO")
	//read operation code needs to be inserted
}

class w_spi_ethernet : public wiz_ethernet{
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
hw_spi_avr *eth ;
PinAtPort(&portb, 29); //ss
PinAtPort(&portb, 28); //mosi
PinAtPort(&portb, 27); //miso
PinAtPort(&portb, 30); //clk
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

