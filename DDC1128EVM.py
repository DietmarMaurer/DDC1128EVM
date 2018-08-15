import time
import ctypes
import numpy as np
import matplotlib.pyplot as plt

CHANNEL = 256
CHANNEL_1 = CHANNEL * 2
len_In = 10

#configuration register if data format is 1
Hard_reser_arr = [0x0000, 0x15FF, 0x15FF, 0x1500, 0x1500, 0x15FF, 0x15FF] 

#Argument types for DLL function
short_p = ctypes.POINTER(ctypes.c_short) #16bit representation ...2 bytes
int_p = ctypes.POINTER(ctypes.c_int) #32bit representation ...4 bytes
double_p = ctypes.POINTER(ctypes.c_double) #64bit representation ...8 bytes

#arrays definitions for DLL functions
#For XferINTDataIn function
short_arr_len = ctypes.c_short * 4096
short_arr_len_1 = ctypes.c_short * 2048

#For FPGAwrite function
int_arr_256 = ctypes.c_int * 256

#For FPGAwrite function
double_arr_256 = ctypes.c_double * CHANNEL
double_arr_512 = ctypes.c_double * CHANNEL_1

#load DLL
dll = ctypes.WinDLL("USB_IO_for_VB6.dll")
#dll = ctypes.WinDLL("dlltest.dll")

#XferINTDataIn
xfer_data_In = dll.XferINTDataIn
xfer_data_In.argtypes = [short_p, short_p, int_p]
xfer_data_In.restype = ctypes.c_short

#XferINTDataOut
xfer_data_Out = dll.XferINTDataOut
xfer_data_Out.argtypes = [short_p, short_p, short_p, short_p, short_p]
xfer_data_Out.restype = ctypes.c_short

# FPGAwrite
Write_fpga = dll.WriteFPGARegsC
Write_fpga.argtypes = [short_p, int_p, int_p, int_p, int_p]
Write_fpga.restype = ctypes.c_int

# Fast data access
fast_data = dll.FastAllDataCap
fast_data.argtypes = [double_p, double_p, double_p, double_p, double_p,
                      ctypes.c_int, ctypes.c_int, ctypes.c_int, double_p,
                      int_p]
fast_data.restype = ctypes.c_int

#Arguments initialisation for function fast data
AVGArr = double_arr_512()
RMSArr = double_arr_512()
P2PArr = double_arr_512()
MAXArr = double_arr_512()
MINArr = double_arr_512()
ArrSize = ctypes.c_int(CHANNEL_1)
Channels = ctypes.c_int(CHANNEL)
Samples = ctypes.c_int(2)
DataArray = double_arr_512()
AllDataAorBfirst = ctypes.c_int(0)

#Arguments for function FPGAwrite
USBdev = ctypes.c_short()
DUTSelect = ctypes.c_int()
RegsIn = int_arr_256()
RegsOut = int_arr_256()
RegsEnable = int_arr_256()

#Arguments for function XferINTDataIn
Array_Data = short_arr_len()
Array_Data_1 = short_arr_len_1()
DataLen = ctypes.c_int(512)

# Hard reset
count = 0
for i in range(7):
    RegH = ctypes.c_short((Hard_reser_arr[i]>>12) & 0x000F)
    RegL = ctypes.c_short((Hard_reser_arr[i]>>8) & 0x000F)
    DataH = ctypes.c_short((Hard_reser_arr[i]>>4) & 0x000F)
    DataL = ctypes.c_short((Hard_reser_arr[i]) & 0x000F)
    ret = xfer_data_Out(USBdev, RegH, RegL, DataH, DataL)
    if ret == 0:
        count= count+1
if count == 7:
    print("Hard reset given!!")
else:
    print("Hard reset Unsucessfull!!")

# Format
RegsIn[0x09] = 24 
RegsEnable[0x09] = 1

# to Ignore
RegsIn[0x0C] = 0x00
RegsEnable[0x0C] = 1

# to Read MSB
RegsIn[0x0D] = 0x00
RegsEnable[0x0D] = 1

# to read MIDB
RegsIn[0x0E] = 0x04
RegsEnable[0x0E] = 1

# to read LSB
RegsIn[0x0F] = 0x00
RegsEnable[0x0F] = 1

ret = Write_fpga(USBdev, DUTSelect, RegsIn, RegsOut, RegsEnable)
print("Function returned: {:d}".format(ret))
print("Firmware version: {:d}".format(RegsOut[0x5E]<<8|RegsOut[0x5F]))
print("Output register array: {:s}".format(" ".join(["{:d}".format(item) for item in RegsOut])))
print("\nDVALIDS (Samples) to Ignore: {:d}".format(RegsOut[0x0C]))
print("DVALIDS (Samples) to Read MSB: {:d}".format(RegsOut[0x0D] << 16| RegsOut[0x0E] << 8 | RegsOut[0x0F]))
print("Load CONV Low:  {:d}", RegsOut[0x01] << 16 | RegsOut[0x02] << 8 | RegsOut[0x03]);
print("Load CONV High: {:d}", RegsOut[0x04] << 16 | RegsOut[0x05] << 8 | RegsOut[0x06]);

# Fastdata aquire and plot
ret = fast_data(AVGArr, RMSArr, P2PArr, MAXArr, MINArr, ArrSize, Channels, Samples, DataArray, ctypes.byref(AllDataAorBfirst))

print("DataArray[0]   = {:f}".format(DataArray[0]))
print("DataArray[1]   = {:f}".format(DataArray[1]))
print("DataArray[191] = {:f}".format(DataArray[191]))
print("DataArray[192] = {:f}".format(DataArray[192]))

plt.plot(DataArray)
plt.show()
