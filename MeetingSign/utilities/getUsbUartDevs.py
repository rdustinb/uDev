from serial.tools import list_ports

for p in list(list_ports.comports()):
  print(p.usb_description())

