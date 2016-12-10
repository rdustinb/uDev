  /***
    __Function Hierarchy__

    read_control_reg                  write_control_reg
        ^   ^   ^                       ^   ^   ^
        |   |   |             __________|   |___|___________________
        |   |___|____________/__________________|______________     |
        |_______|_____      /                   |              \    |
                |     \    /                    |               \   |
                |   read_phy_reg                |             write_phy_reg
                |       ^                       |                   ^
                |       |                       |   ________________|
                |      /                        |  /
                |     /                         | /
            verify_init                        init

  ***/

#include <variables.h>
#include <enc28j60.h>

void init_enc28j60(uint8_t duplex_mode, uint8_t* macaddr, uint8_t led_function){
  /***********************
    This function should allow only a few changes to initialization that affects the overall operation of the ENC28J60
    but should not alter the stack in any way.

    duplex_mode
    macaddr
    led_function
  ***********************/
  uint8_t MACON1_config = 0x0;
  // MACON3 Register
  // Enable MAC CRC generation and appension Tx Packets
  // Enable Frame Length / Type Checking for Rx and Tx Packets
  // Enable Full Duplex Mode
  // Automatic 0-padding to 64 bytes
  uint8_t MACON3_config = 0xE0 | 0x10 | 0x2;
  uint8_t MACON4_config = 0x40; // Defer Tx in Half-Duplex
  uint8_t MABBIPG_config = 0x15; // Set to 0x15, corresponds to Full-Duplex IPG Delay IEEE Minimum
  uint8_t PHCON1_config = 0x0;
  uint16_t PHLCON_config = 0x3212;
  uint8_t EIE_config = 0xFB;
  uint16_t PHIE_config = 0x0012;

  // Vary the config based on mode
  switch(duplex_mode){
    case FULL : MACON1_config = MACON1_config | 0xD; // Tx of Pause Frames
    case HALF : MACON1_config = MACON1_config | 0x5; // No Tx of Pause Frames
  };
  switch(duplex_mode){
    case FULL : MACON3_config = MACON3_config | 0x1; // Full-Duplex
    case HALF : MACON3_config = MACON3_config | 0x0; // Half-Duplex
  };
  switch(duplex_mode){
    case FULL : PHCON1_config = PHCON1_config | 0x10; // Set the PDPXMD Bit for Full-Duplex
    case HALF : PHCON1_config = PHCON1_config | 0x00; // Clr the PDPXMD Bit for Half-Duplex
  };

  /////////////////////////
  //  Rx Buffer Initialization - default is 6.5kB (6665 Bytes)
  /////////////////////////

  /////////////////////////
  //  Tx Buffer Initialization - default is 1.5kB (1530 Bytes)
  /////////////////////////

  /////////////////////////
  //  Rx Filters
  /////////////////////////
  /*
    The Rx Filters should NOT filter anything until DHCP is setup properly? Otherwise the multicast/broadcast from the
    DHCP Server might be dropped. Need to look at this more...

    Then after DHCP Config the filtering can be enabled.
  */

  /////////////////////////
  //  MAC Initialization
  /////////////////////////
  // MACON1 Register
  // Enable TXPAUS and RXPAUS to allow the MAC to transmit Pause frames
  // Enable Rx
  set_bank_select(BANK2);
  write_control_reg(MACON1, MACON1_config);
  write_control_reg(MACON3, MACON3_config);

  // MACON4 Register - Unused in Full-Duplex Mode
  if(duplex_mode == HALF){
    write_control_reg(MACON4, MACON4_config);
  }

  // MABBIPG Register
  write_control_reg(MABBIPG, MABBIPG_config);

  // MAMXFLL / MAMXFLH Registers
  // Set the maximum allowable frame size, default is 1536
  // FUTURE - don't alter the default for now

  // MAADR1 - MAADR6 Registers
  // Program the nodes MAC address
  set_bank_select(BANK3);
  write_control_reg(MAADR6, (uint8_t)(macaddr[0]));  // Byte 0 of MAC
  write_control_reg(MAADR5, (uint8_t)(macaddr[1]));  // Byte 1 of MAC
  write_control_reg(MAADR4, (uint8_t)(macaddr[2]));  // Byte 2 of MAC
  write_control_reg(MAADR3, (uint8_t)(macaddr[3]));  // Byte 3 of MAC
  write_control_reg(MAADR2, (uint8_t)(macaddr[4]));  // Byte 4 of MAC
  write_control_reg(MAADR1, (uint8_t)(macaddr[5]));  // Byte 5 of MAC

  /***********************
    PHY Initialization
  ***********************/
  // PHCON1 Register
  write_phy_reg(PHCON1, PHCON1_config);

  // PHLCON Register
  // Setup LEDs, LEDA is Receive Activity, LEDB is Transmit Activity, Stretch by Tnstrch (40ms), Stretch Active
  write_phy_reg(PHLCON, PHLCON_config);

  /////////////////////////
  //  Interrupt Setup
  /////////////////////////
  /*
    This section should setup the interrupts block...we want to receive interrupts whenever there is an incoming packet
    or there are link issues / errors.
    This is accomplished by setting the bits in the EIE register (0x1B in ALL banks) and then reading the EIR register
    (0x1C in ALL banks) whenever the host receives any interrupt. The internal controller FSM should handle priority
    decoding of the interrupts and branch in an appropriate order to handle all service information as needed.

    Things to consider: what to do with an Rx packet when a link error also occurs? Does the MAC chip handle this
    situation to prevent corrupt packets from being presented to the host controller if the link goes down while a
    packet is being received?
  */
  write_control_reg(EIE, EIE_config);   // bit 2 is always 0, set all active interrupts
  write_phy_reg(PHIE, PHIE_config);    // Set the Link Status Interrupt Enable bit and the global Phy Interrupt Bit
}

uint16_t verify_init_enc28j60(uint8_t duplex_mode, uint8_t* macaddr, uint8_t led_function){
  /*
    The point of this function is to be called after the init_enc28j60 function was called to verify the configuration
    occured as expected. This is always a good practice to write and then read configuration registers to confirm the
    target device is actually writing what we are telling it to before jumping into more complex operations.
  */
  uint8_t MACON1_config = 0x0;
  uint8_t MACON3_config = 0xE0 | 0x10 | 0x2;
  uint8_t MACON4_config = 0x40; // Defer Tx in Half-Duplex
  uint8_t MABBIPG_config = 0x15; // Set to 0x15, corresponds to Full-Duplex IPG Delay IEEE Minimum
  uint8_t PHCON1_config = 0x0;
  uint16_t PHLCON_config = 0x3212;
  uint8_t EIE_config = 0xFB;
  uint16_t PHIE_config = 0x0012;

  // Vary the config based on mode
  switch(duplex_mode){
    case FULL : MACON1_config = MACON1_config | 0xD; // Tx of Pause Frames
    case HALF : MACON1_config = MACON1_config | 0x5; // No Tx of Pause Frames
  };
  switch(duplex_mode){
    case FULL : MACON3_config = MACON3_config | 0x1; // Full-Duplex
    case HALF : MACON3_config = MACON3_config | 0x0; // Half-Duplex
  };
  switch(duplex_mode){
    case FULL : PHCON1_config = PHCON1_config | 0x10; // Set the PDPXMD Bit for Full-Duplex
    case HALF : PHCON1_config = PHCON1_config | 0x00; // Clr the PDPXMD Bit for Half-Duplex
  };

  /////////////////////////
  //  Rx Buffer Initialization - default is 6.5kB (6665 Bytes)
  /////////////////////////

  /////////////////////////
  //  Tx Buffer Initialization - default is 1.5kB (1530 Bytes)
  /////////////////////////

  /////////////////////////
  //  Rx Filters
  /////////////////////////
  /*
    The Rx Filters should NOT filter anything until DHCP is setup properly? Otherwise the multicast/broadcast from the
    DHCP Server might be dropped. Need to look at this more...

    Then after DHCP Config the filtering can be enabled.
  */

  /////////////////////////
  //  MAC Initialization
  /////////////////////////
  // MACON1 Register
  set_bank_select(BANK2);
  uint16_t status = 0x0000;
  if(MACON1_config != read_control_reg(MACON1)){
    status = BADINITWR.MACON1_config_err;
  }
  if(MACON3_config != read_control_reg(MACON3)){
    status = BADINITWR.MACON3_config_err;
  }

  // MACON4 Register - Unused in Full-Duplex Mode
  if(duplex_mode == HALF){
    if(MACON4_config != read_control_reg(MACON4)){
      status = BADINITWR.MACON4_config_err;
    }
  }else{
    if(0x0 != read_control_reg(MACON4)){
      status = BADINITWR.MACON4_config_err;
    }
  }

  // MABBIPG Register
  if(MABBIPG_config != read_control_reg(MABBIPG)){
    status = BADINITWR.MABBIPG_config_err;
  }

  // MAMXFLL / MAMXFLH Registers
  // FUTURE - don't alter the default for now

  // MAADR1 - MAADR6 Registers
  set_bank_select(BANK3);
  if((uint8_t)(macaddr[0]) != read_control_reg(MAADR6)){  // Byte 0 of MAC
    status = BADINITWR.MAADR6_config_err;
  }
  if((uint8_t)(macaddr[1]) != read_control_reg(MAADR5)){  // Byte 1 of MAC
    status = BADINITWR.MAADR5_config_err;
  }
  if((uint8_t)(macaddr[2]) != read_control_reg(MAADR4)){  // Byte 2 of MAC
    status = BADINITWR.MAADR4_config_err;
  }
  if((uint8_t)(macaddr[3]) != read_control_reg(MAADR3)){  // Byte 3 of MAC
    status = BADINITWR.MAADR3_config_err;
  }
  if((uint8_t)(macaddr[4]) != read_control_reg(MAADR2)){  // Byte 4 of MAC
    status = BADINITWR.MAADR2_config_err;
  }
  if((uint8_t)(macaddr[5]) != read_control_reg(MAADR1)){  // Byte 5 of MAC
    status = BADINITWR.MAADR1_config_err;
  }

  /***********************
    PHY Initialization
  ***********************/
  // PHCON1 Register
  if(PHCON1_config != read_phy_reg(PHCON1)){
    status = BADINITWR.PHCON1_config_err;
  }
  if(PHLCON_config != read_phy_reg(PHLCON)){
    status = BADINITWR.PHLCON_config_err;
  }

  /////////////////////////
  //  Interrupt Setup
  /////////////////////////
  if(EIE_config != read_control_reg(EIE)){  // bit 2 is always 0, set all active interr)){
    status = BADINITWR.EIE_config_err;
  }
  if(PHIE_config != read_phy_reg(PHIE)){    // Set the Link Status Interrupt Enable bit and the global Phy Interrupt)){
    status = BADINITWR.PHIE_config_err;
  }

  return status;
}
