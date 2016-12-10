  /***
    The ENC28J60 Communicates over SPI mode 0,0 only: indicating IDLE is 0 (SS or CS is LO) and the clock edge to
    capture is positive edge. The SPI bus expects the ***most significant bit*** of the ***least significant byte***
    first. The ENC28J60 datasheet is *obviously* written to allow the designer ease of use with a PIC microcontroller,
    but in our case we just need to know how to set this up with an Arduino.

    The ENC28J60 utilizes an 8-bit command at the beginning of the SPI transaction to indicate what data *if any*
    follows in the transaction. The SPI interface also utilizes a *set* of trasactions to complete one overall operation
    such as reading a system register from the ENC28J60.

    Command Structure                             |   Instruction Set (OpCode + Argument)
    Bit   7   6   5   4   3   2   1   0           |   3'b000 + 5'b<address>   Read Control Register
         ___________ ___________________          |   3'b001 + 5'b11010       Read Buffer Memory
        |__Opcode___|_____Argument______|         |   3'b010 + 5'b<address>   Write Control Register
                                                  |   3'b011 + 5'b11010       Write Buffer Memory
                                                  |   3'b100 + 5'b<address>   Bit Field Set
                                                  |   3'b101 + 5'b<address>   Bit Field Clear
                                                  |   3'b110 + 5'bXXXXX       RESERVED
                                                  |   3'b111 + 5'b11111       System Reset

    __Read Control Register Sequence__
    The ENC28J60 expects a command byte driven by the master indicating a Read Control Register operation, followed by
    the address to be read. Following the final bit of the address the ENC28J60 will begin driving the MSb of the
    addressed register byte back to the master. The Arduino must make a SPI read of 1 byte.

      1. CS LO
      2. RCR opcode + Address = 000b + Address
      3. Single Byte in via MISO
      4. CS HI

          SPI Sequence
                __                 __
            CS    |_______________|
                ____ _____ __________
            SI  _X__|_CMD_|_____X____
                __________ ____ _____
            SO  _____X____|DATA|__X__

          *X indicates we don't care what the bus is at that point in the transaction.

    This sequence is common for all addresses except the MAC and MII registers, where the ENC28J60 will *first* drive
    out a *dummy byte* followed second by the actual data byte of the register read. The Arduino must make a SPI read of
    2 bytes and throw away the first byte received.

      1. CS LO
      2. RCR opcode + Address = 000b + Address
      3. Single Dummy Byte in via MISO, throw away
      3. Single Byte in via MISO
      4. CS HI

          SPI Sequence
                __                      __
            CS    |____________________|
                ____ _____ _______________
            SI  _X__|_CMD_|_______X_______
                __________ _____ ____ ____
            SO  _____X____|DUMMY|DATA|__X_

          *X indicates we don't care what the bus is at that point in the transaction.

    __Read Buffer Memory Sequence__
    The ENC28J60 has a shared 8kbyte buffer used by both the receiver and transmitter (enough for a single Jumbo Frame,
    or several standard sized frames). The FIFO is divided into an Rx and Tx buffer by the respective start and end
    pointers for each channel. It is possible to perform random access of the memory, though it makes more sense for most
    operations to allow the internal logic to access the buffers incrementally by setting the AUTOINC bit to 1. This will
    cause the ENC28J60 to handle the next address to be read or written (Rx or Tx, respectively) without requiring the
    master device to increment the address manually between each byte read or written.

      1. CS LO
      2. RBM opcode + constant = 0x3A
      3. Data In via MISO, until CS HI
      4. CS HI

    __Write Control Register Sequence__
    This command will write a single byte to any of the 32 Control Registers. CS must be maintained LO through the entire
    opcode+address+data byte sequence, otherwise the command is terminated.

      1. CS LO
      2. WCR opcode + Address = 010b + Address
      3. Single Data Byte out via MOSI
      4. CS HI

    Writing the PHY registers is an indirect write event. The host controller must first write the PHY register address
    to the MIREGADR register, write the 16-bit register value into the MIWRL, then MIWRH registers (in that order), then
    check the MISTAT registers BUSY until it goes LO, signalling the write to the PHY register is now complete. There are
    only 9 PHY registers totalling. Since the actual WCR transaction only handles a single byte to a single address at a
    time, writing any of the PHY registers actually takes 3 WCR transactions.

      1.  CS LO
      2.  WCR opcode + MIREGADR Address = 010b + MIREGADR Address
      3.  PHY Address out via MOSI
      4.  CS HI
      5.  CS LO
      6.  WCR opcode + MIWRL Address = 010b + MIWRL Address
      7.  PHY Register Byte Low out via MOSI
      8.  CS HI
      9.  CS LO
      10. WCR opcode + MIWRH Address = 010b + MIWRH Address
      11. PHY Register Byte High out via MOSI
      12. CS HI
      13. RCR Commands to Monitor BUSY bit in MISTAT Register

    __Write Buffer Memory Sequence__
    The Tx and Rx data share the same unified 8k Memory. The burden of maintaining where each buffer of data resides is
    up to the host controller. The Tx and Rx buffers are fully programmable by the host controller.

    Rx Buffer                                                       Tx Buffer
      - Circular                                                      - Not Circular, no pointer management by hardware
      - Managed by Hardware, write to pointer locations are RO        - Fully host-programmable and controlled
      - ERXSTH:ERXDTL (inclusive, configurable when Rx Inactive)      - ETXST, ETXND
      - ERXNDH:ERXNDL (inclusive, configurable when Rx Inactive)      - No Rx Buffer overlap prevention, up to host
      - Overflow drop method (new data doesn't overwrite current)     -

    __Control, MAC, MII Register Space__
    Most of the registers are partitioned into four banks which can be indirectly addressed by selecting the appropriate
    bank in the ECON1 Registers BSEL0/BSEL1 bits. The ECON1, ECON2, ESTAT, EIR and EIE Registers are accessible from all
    of the Banks.

    __Packet Structure__
    The ENC28J60 device really only handles the PHY layer of things, though technically it handles part of the MAC layer
    as the Preamble, Delimeter, Padding and FCS are handled by the chip. These fields are all added to a user packet when
    transmission is selected, and they are stripped off when reception of a packet occurs, except the padding bytes (if
    any) which will end up in the Rx Buffer.

    __Function Hierarchy__

    read_control_reg                  write_control_reg                         read_buffer_mem         write_buffer_mem
        ^     ^                         ^   ^   ^                                       ^                       ^
        |     |                         |   |   |___________________                    |                       |
        |     |           ______________|   |_______________________|___________________|___________________    |
        |     |__________/__________________________________________|_______________\___|__________________ \   |
        |_________      /                                      \    |             \  \  |                  \ \  |
                  \    /                                        \   |              \  \ |                   \ \ |
                  read_phy_reg                                write_phy_reg         rx_packet               tx_packet

  ***/

// Macros
// How to define this?
#define MACADDR   0x001122000001

#define HALF      0
#define FULL      1

#define NOP       0x0

#define BANK0     0x0
#define BANK1     0x1
#define BANK2     0x2
#define BANK3     0x3

#define GOOD      0
#define BADLEN    1
#define BADSPIWR  2
struct badiniterror {
  uint16_t MACON1_config_err  = 0x0300;
  uint16_t MACON3_config_err  = 0x0301;
  uint16_t MACON4_config_err  = 0x0302;
  uint16_t MABBIPG_config_err = 0x0303;
  uint16_t MAADR6_config_err  = 0x0304;
  uint16_t MAADR5_config_err  = 0x0305;
  uint16_t MAADR4_config_err  = 0x0306;
  uint16_t MAADR3_config_err  = 0x0307;
  uint16_t MAADR2_config_err  = 0x0308;
  uint16_t MAADR1_config_err  = 0x0309;
  uint16_t PHCON1_config_err  = 0x030A;
  uint16_t PHLCON_config_err  = 0x030B;
  uint16_t EIE_config_err     = 0x030C;
  uint16_t PHIE_config_err    = 0x030D;
}BADINITWR;

#define EIE       0x1B
#define EIR       0x1C
#define ESTAT     0x1D
#define ECON2     0x1E
#define ECON1     0x1F

#define ERXFCON   0x18

#define MACON1    0x00
#define MACON3    0x02
#define MACON4    0x03
#define MABBIPG   0x04
#define MAMXFLL   0x0A
#define MAMXFLH   0x0A

#define MAADR5    0x00
#define MAADR6    0x01
#define MAADR3    0x02
#define MAADR4    0x03
#define MAADR1    0x04
#define MAADR2    0x05
#define MISTAT    0x0A

#define MICMD     0x12
#define MIREGADR  0x14
#define MIWRL     0x16
#define MIWRH     0x17
#define MIRDL     0x18
#define MIRDH     0x19

#define PHCON1    0x00
#define PHSTAT1   0x01
#define PHID1     0x02
#define PHID2     0x03
#define PHCON2    0x10
#define PHSTAT2   0x11
#define PHIE      0x12
#define PHIR      0x13
#define PHLCON    0x14

/*** Basic Controls ***/

uint8_t __bank_select;

// Get Current Bank Selected
uint8_t get_bank_select(){
  return __bank_select;
}

// Set Current Bank Selected
uint8_t set_bank_select(uint8_t value){
  // Just write the bank needed and if that bank is already selected, nothing happens, or this function will change the
  // local bank selected value and change the selected bank on the ENC28J60
  if(value != __bank_select){
    __bank_select = value;
    // Configure the ENC28J60 to the correct BANK
    if((value & 0x3) == BANK3){
      bitset_control_reg(ECON1, 0x3);
    }else if((value & 0x3) == BANK2){
      bitclear_control_reg(ECON1, 0x3);
      bitset_control_reg(ECON1, 0x2);
    }else if((value & 0x3) == BANK1){
      bitclear_control_reg(ECON1, 0x3);
      bitset_control_reg(ECON1, 0x1);
    }else{
      bitclear_control_reg(ECON1, 0x3);
    }
  }
}

uint8_t read_control_reg(uint8_t address){
  digitalWrite(ethCS, LOW);
  // OpCode for Read Control is b000 in the upper three bits, simply mask them out
  uint8_t read_address = address & 0x1F;
  SPI.transfer(read_address);
  // Check if we are accessing the MAC/MII Registers, need a dummy transfer
  if((get_bank_select() == BANK2) || (get_bank_select() == BANK3)){
    // Dummy Transfer
    SPI.transfer(NOP);
  }
  // The Next Byte is the Data
  uint8_t reg_data = SPI.transfer(NOP);
  // Close Transaction
  digitalWrite(ethCS, HIGH);
  // Return the Byte
  return reg_data;
}

void write_control_reg(uint8_t address, uint8_t data){
  digitalWrite(ethCS, LOW);
  // OpCode for Write Control is b010 in the upper three bits
  uint8_t read_address = address & 0x1F + 0x40;
  SPI.transfer(read_address);
  // Data Is the second transfer
  SPI.transfer(data);
  // Close Transaction
  digitalWrite(ethCS, HIGH);
}

void bitset_control_reg(uint8_t address, uint8_t mask){
  digitalWrite(ethCS, LOW);
  // OpCode for Bit Field Set is b100 in the upper three bits
  uint8_t bitset_address = 0x80 + (address & 0x1F);
  SPI.transfer(bitset_address);
  // Send the Mask to set the specific bits in the register
  SPI.transfer(mask);
  // Close Transaction
  digitalWrite(ethCS, HIGH);
}

void bitclear_control_reg(uint8_t address, uint8_t mask){
  digitalWrite(ethCS, LOW);
  // OpCode for Bit Field Clear is b101 in the upper three bits
  uint8_t bitclear_address = 0xA0 + (address & 0x1F);
  SPI.transfer(bitclear_address);
  // Send the Mask to clear the specific bits in the register
  SPI.transfer(mask);
  // Close Transaction
  digitalWrite(ethCS, HIGH);
}

uint8_t read_buffer_mem(uint8_t* data_buff_ptr, uint16_t byte_count){
  // Check that byte_count is reasonable
  if(byte_count > 8000){
    return BADLEN;
  }
  // Abstraction function, this simply fills up a buffer with byte_count number of bytes and returns, the data isn't
  // actually passed, only the original data buffer pointer is passed to this function to have it fill up.
  // The way this driver will work is it will assume the AUTOINC bit is set. This could change in the future if necessary
  // but for now the address is assumed to be handled by the hardware or the next higher level of abstraction.
  digitalWrite(ethCS, LOW);
  // Create the RBM OpCode
  uint8_t opcode = 0x20 + 0x1A;
  // Start the Buffer Read by sending the OpCode
  SPI.transfer(opcode);
  // Loop Through Byte Count
  for(uint16_t i=0; i<byte_count; i++){
    data_buff_ptr[i] = SPI.transfer(NOP);
  }
  // Close Transaction
  digitalWrite(ethCS, HIGH);
  // Return everything good
  return GOOD;
}

uint8_t write_buffer_mem(uint8_t* data_buff_ptr, uint16_t byte_count){
  // Check that byte_count is reasonable
  if(byte_count > 8000){
    return BADLEN;
  }
  // Abstraction function, this simply takes an array pointer and a byte_count and writes the bytes over SPI to the
  // ENC28J60 after first sending the appropriate OpCode.
  digitalWrite(ethCS, LOW);
  // Create the Write Buffer Memory (WBM) OpCode
  uint8_t opcode = 0x60 + 0x1A;
  // Send the OpCode to start the Write Buffer Transaction
  SPI.transfer(opcode)
  // Loop Through Byte Count
  for(uint16_t i=0; i<byte_count; i++){
    SPI.transfer(data_buff_ptr[i]);
  }
  // Close Transaction
  digitalWrite(ethCS, HIGH);
  // Return everything good
  return GOOD;
}

uint16_t read_phy_reg(uint8_t phy_address){
  set_bank_select(BANK2);
  // Perform a WCR to the MIREGADR to specify the PHY Address we want to read
  write_control_reg(MIREGADR, phy_address);
  // Perform a Bit Set to the MICMD register and set the MIIRD bit (0) to signal a PHY Register Read
  bitset_control_reg(MICMD, 0x1);
  // Delay at least 11us
  // Clear bit 0 in the MICMD register (WCR)
  bitclear_control_reg(MICMD, 0x1);
  // Perform an RCR for the MIRDL register (lower 8 bits)
  uint16_t phy_reg_data = 0x0000 + read_control_reg(MIRDL);
  // Perform an RCR for the MIRDH register (upper 8 bits)
  phy_reg_data = (phy_reg_data & 0x00FF) + ((read_control_reg(MIRDH)<<8) & 0xFF00);
  // Return the read PHY Register
  return phy_reg_data;
}

uint8_t write_phy_reg(uint8_t phy_address, uint16_t data){
  set_bank_select(BANK2);
  // Perform a WCR to the MIREGADR to specify the PHY Address we want to write
  write_control_reg(MIREGADR, phy_address);
  // Perform a WCR to the MIWRL to write the Low Byte
  write_control_reg(MIWRL, (uint8_t)(data & 0x00FF));
  // Perform a WCR to the MIREGADR to write the High Byte, this kicks off the PHY Write
  write_control_reg(MIWRH, (uint8_t)((data & 0xFF00)>>8));
  // Check the MISTAT Register for Busy Bit
  uint8_t write_phy_dly_count = 0;
  uint8_t status = GOOD;
  set_bank_select(BANK3);
  while((read_control_reg(MISTAT) & 0x1) == 0x1){
    if(write_phy_dly_count == 5){
      // Force a break after a few reads, as the PHY Write should only take 10.24us
      status = BADSPIWR;
      break;
    }else{
      write_phy_dly_count++;
    }
  }
  // Return the value
  return status;
}

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
