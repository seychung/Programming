#!/usr/bin/env python
import serial
import time
import os
import sys
import glob
# Convert a char to ASC2 value represents the argument
# Example: Ser.write converts '0' to NULL by default
# Therefore, use this convertion to convert '0' char to '0' ASC2 
def convert(a):
    if len(a) > 1:
      return -1
    elif len(a) <= 0:
      return -1
    if ord(a) < 48:
      return -1
    elif ord(a) > 57:
      return -1
    else:      
      return chr(ord(a) - 48)

# Searches a list of active ports
def serial_ports():

    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result
    
while True:
  portname = ''
  while "/dev/ttyACM" not in portname:
    portname = serial_ports()
    portname = portname[0] #Assume the first port is what we want
  portname = serial_ports()
  portname = portname[0]
  ser = serial.Serial(portname,9600)
  f = open("/var/www/html/values.txt", 'r')
  lines = f.readlines()
  print(portname)
  line0 = lines[0]
  line1 = lines[1]
  line2 = lines[2]
  line3 = lines[3]
  line4 = lines[4]
  line5 = lines[5]
  line0 = line0.rstrip("\r\n") #no decimal
  line1 = line1.rstrip("\r\n") #yes
  line2 = line2.rstrip("\r\n") #no decimal
  line3 = line3.rstrip("\r\n") #yes
  line4 = line4.rstrip("\r\n") #no decimal
  line5 = line5.rstrip("\r\n") #yes
  if line1.find('.') == -1:
    line1 += ".0"
  if line3.find('.') == -1:
    line3 += ".0"
  if line5.find('.') == -1:
    line5 += ".0"   
  inputsize = convert(chr(len(line0)+48)) + convert(chr(len(line1)+47)) + convert(chr(len(line2)+48)) + convert(chr(len(line3)+47)) + convert(chr(len(line4)+48)) + convert(chr(len(line5)+47)) 
  msg0 = ""
  msg2 = ""
  msg4 = ""
  msg1 = ""
  msg3 = ""
  msg5 = ""
  for x in range(0, len(line0)) :
    msg0 += convert(line0[x])
  for x in range(0, len(line2)) :
    msg2 += convert(line2[x])
  for x in range(0, len(line4)) :
    msg4 += convert(line4[x])  
  
  if len(line1) == 3:
    msg1 = convert(line1[0]) + convert(line1[2])
  elif len(line1) == 4:
    msg1 = convert(line1[0]) + convert(line1[1]) + convert(line1[3])
  if len(line3) == 3:
    msg3 = convert(line3[0]) + convert(line3[2])
  elif len(line3) == 4:
    msg3 = convert(line3[0]) + convert(line3[1]) + convert(line3[3])
  if len(line5) == 3:
    msg5 = convert(line5[0]) + convert(line5[2])
  elif len(line5) == 4:    
    msg5 = convert(line5[0]) + convert(line5[1]) + convert(line5[3])
  
  packet = msg0 + msg1 + msg2 + msg3 + msg4 + msg5
  total = inputsize + packet
  ser.write(total)
  time.sleep(1.5)
