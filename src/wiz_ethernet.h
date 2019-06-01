/* wiz_etherenet.h - defines the base class for Wiznet w5100 and W5500 ethernet controllers. */
#include "simulationmember.h"

#ifndef WIZ_ETHERNET_H
#define WIZ_ETHERNET_H

class wiz_ethernet : public simulationmember
{
private:
protected:
    std::map < std::string, Pin *> allPins;
    // std::vector<Hardware *> hwResetList; - not required since the hardware configuration is static
    // std::vector<Hardware *> hwCycleList; - not required since the hardware configuration is static

    // Registers
    IOReg<wiz_ethernet> m_reg,
						ga_reg0, ga_reg1, ga_reg2, ga_reg3,
						sub_reg0, sub_reg1, sub_reg2, sub_reg3,
						sha_reg0, sha_reg1, sha_reg2, sha_reg3, sha_reg4, sha_reg5,
						sip_reg0, sip_reg1, sip_reg2, sip_reg3,
						intlvl_reg,
						rt_reg0, rt1_reg1,
						rc_reg,
	                    i_reg,
						im_reg,
						ptimer_reg,
						pmagic_reg,
						uipr_reg0 ,uipr_reg1, uipr_reg2, uipr_reg3,
						uport_reg0, uport_reg1;

    RWMemoryMember **rw;  ///memory for general purpose registers - all other memory is included in the sockets
    wiz_socket socket[4];

public:
    // SPI port pins
    Pin sspin;
    Pin misopin;
    Pin mosipin;
    Pin spickpin;


};

#endif
