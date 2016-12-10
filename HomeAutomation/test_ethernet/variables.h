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
