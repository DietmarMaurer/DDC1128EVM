// DDC1128EVM.cpp (c) 2018 Dietmar Maurer
//

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

///////////////////////////////////////////////////

// extern "C" int __declspec(dllexport) XferINTDataIn(int *USBdev, int *INTData, long *DataLength);
typedef int16_t(WINAPI *XferINTDataIn_type)(int16_t *USBdev, int16_t *INTData, int32_t *DataLength);

// extern "C" int __declspec(dllexport) XferINTDataOut(int *USBdev, int *Data01, int *Data02, int *Data03, int *Data04);
typedef int16_t(WINAPI *XferINTDataOut_type)(int16_t *USBdev, int16_t *Data01, int16_t *Data02, int16_t *Data03, int16_t *Data04);

// extern "C" long __declspec(dllexport) WriteFPGARegsC(int *USBdev, long DUTSelect, long * RegsIn, long *  RegsOut, long * RegEnable);
typedef int32_t(WINAPI *WriteFPGARegsC_type)(int16_t *USBdev, int32_t *DUTSelect, int32_t *RegsIn, int32_t *RegsOut, int32_t *RegEnable);

// extern "C" long __declspec(dllexport) FastAllDataCap(double * AVGArr, double * RMSArr, double * P2PArr, double * MAXArr, double * MINArr, long ArrSize, long Channels, long nDVALIDReads, double * DataArray, long * AllDataAorBfirst);
typedef int32_t(WINAPI *FastAllDataCap_type)(double *AVGArr, double *RMSArr, double *P2PArr, double *MAXArr, double *MINArr, int32_t ArrSize, int32_t Channels, int32_t Samples, double *AllData, int32_t *AllDataAorBfirst);

///////////////////////////////////////////////////

int main()
{
    HMODULE hResDDC1128DLL = LoadLibrary(TEXT("USB_IO_for_VB6.dll"));

    if (!hResDDC1128DLL) {
        DWORD e = GetLastError();
        printf("LoadLibrary failed, err=%d\n", e);
        return 0;
    }
    else {
        printf("LoadLibrary ok\n");
    }

    XferINTDataIn_type  XferINTDataIn = (XferINTDataIn_type)GetProcAddress(hResDDC1128DLL, "XferINTDataIn");
    XferINTDataOut_type XferINTDataOut = (XferINTDataOut_type)GetProcAddress(hResDDC1128DLL, "XferINTDataOut");
    WriteFPGARegsC_type WriteFPGARegsC = (WriteFPGARegsC_type)GetProcAddress(hResDDC1128DLL, "WriteFPGARegsC");
    FastAllDataCap_type FastAllDataCap = (FastAllDataCap_type)GetProcAddress(hResDDC1128DLL, "FastAllDataCap");

    if (NULL == XferINTDataIn || NULL == XferINTDataOut || NULL == WriteFPGARegsC || NULL == FastAllDataCap) {
        printf("GetProcAddress failed\n");
        FreeLibrary(hResDDC1128DLL);
        return 0;
    }
    else {
        printf("GetProcAddress ok\n");
    }

    int32_t Channels = 256;
    int16_t USBdev = 0;
    int32_t ret = 0;
    int32_t DUTSelect = 0;

    int32_t* RegsIn = (int32_t*)malloc(Channels * sizeof(int32_t));
    int32_t* RegsOut = (int32_t*)malloc(Channels * sizeof(int32_t));;
    int32_t* RegsEnable = (int32_t*)malloc(Channels * sizeof(int32_t));;

    memset(RegsIn, 0, Channels * sizeof(int32_t));
    memset(RegsOut, 0, Channels * sizeof(int32_t));
    memset(RegsEnable, 0, Channels * sizeof(int32_t));

    RegsIn[0x09] = 24;   RegsEnable[0x09] = 1; // Format
    RegsIn[0x0C] = 0x00; RegsEnable[0x0C] = 1; // to Ignore
    RegsIn[0x0D] = 0x00; RegsEnable[0x0D] = 1; // to Read MSB
    RegsIn[0x0E] = 0x04; RegsEnable[0x0E] = 1; // to read MIDB
    RegsIn[0x0F] = 0x00; RegsEnable[0x0F] = 1; // to read LSB

    ret = WriteFPGARegsC(&USBdev, &DUTSelect, RegsIn, RegsOut, RegsEnable);
    if (ret == 0)
        printf("WriteFPGARegsC ok\n");
    else
        printf("WriteFPGARegsC failed\n");

    printf("Firmware version: %d\n", RegsOut[0x5E] << 8 | RegsOut[0x5F]);

    printf("Load CONV Low:  %d\n", RegsOut[0x01] << 16 | RegsOut[0x02] << 8 | RegsOut[0x03]);
    printf("Load CONV High: %d\n", RegsOut[0x04] << 16 | RegsOut[0x05] << 8 | RegsOut[0x06]);
    printf("DVALIDS (Samples) Format: %d\n", RegsOut[0x09]);
    printf("DVALIDS (Samples) to Ignore: %d\n", RegsOut[0x0C]);
    printf("DVALIDS (Samples) to Read: %d\n", RegsOut[0x0D] << 16 | RegsOut[0x0E] << 8 | RegsOut[0x0F]);

    free(RegsIn);     RegsIn = nullptr;
    free(RegsOut);    RegsOut = nullptr;
    free(RegsEnable); RegsEnable = nullptr;

#if 1
    int Hard_reser_arr[] = { 0x0000, 0x15FF, 0x15FF, 0x1500, 0x1500, 0x15FF, 0x15FF };
    int count = 0;

    for (int i = 0; i < 7; i++) {
        int16_t RegH = (Hard_reser_arr[i] >> 12) & 0x000F;
        int16_t RegL = (Hard_reser_arr[i] >> 8) & 0x000F;
        int16_t DataH = (Hard_reser_arr[i] >> 4) & 0x000F;
        int16_t DataL = (Hard_reser_arr[i]) & 0x000F;
        int16_t ret = XferINTDataOut(&USBdev, &RegH, &RegL, &DataH, &DataL);
        if (ret == 0)
            ++count;
    }
    if (count == 7)
        printf("Hard reset ok\n");
    else
        printf("Hard reset failed\n");
#endif

    int32_t ArrSize = 2 * Channels;
    int32_t Samples = 1024;
    int32_t AllDataAorBfirst = 0;

    double* AVGArr = (double*)malloc(ArrSize * sizeof(double));
    double* RMSArr = (double*)malloc(ArrSize * sizeof(double));
    double* P2PArr = (double*)malloc(ArrSize * sizeof(double));
    double* MAXArr = (double*)malloc(ArrSize * sizeof(double));
    double* MINArr = (double*)malloc(ArrSize * sizeof(double));

    double* AllData = (double*)malloc(Samples * Channels * sizeof(double));

    memset(AVGArr,  0, ArrSize * sizeof(double));
    memset(RMSArr,  0, ArrSize * sizeof(double));
    memset(P2PArr,  0, ArrSize * sizeof(double));
    memset(MAXArr,  0, ArrSize * sizeof(double));
    memset(MINArr,  0, ArrSize * sizeof(double));
    memset(AllData, 0, ArrSize * sizeof(double));

    ret = FastAllDataCap(AVGArr, RMSArr, P2PArr, MAXArr, MINArr, ArrSize, Channels, Samples, AllData, &AllDataAorBfirst);
    if (ret < 0) {
        printf("FastAllDataCap failed, ret=%d\n", ret);
        return 1;
    }

    // show some values

    for (int c = 0; c < ArrSize; c++) {
        printf("[%d]=%lf  ", c, AllData[c]);
        if (c > 0 && c % 7 == 0)
            printf("\n");
    }
    printf("\n");

    printf("AVGArr[0]=%lf AVGArr[1]=%lf AVGArr[510]=%lf AVGArr[511]=%lf\n", AVGArr[0], AVGArr[1], AVGArr[510], AVGArr[511]);
    printf("RMSArr[0]=%lf RMSArr[1]=%lf RMSArr[510]=%lf RMSArr[511]=%lf\n", RMSArr[0], RMSArr[1], RMSArr[510], RMSArr[511]);
    printf("P2PArr[0]=%lf P2PArr[1]=%lf P2PArr[510]=%lf P2PArr[511]=%lf\n", P2PArr[0], P2PArr[1], P2PArr[510], P2PArr[511]);
    printf("MAXArr[0]=%lf MAXArr[1]=%lf MAXArr[510]=%lf MAXArr[511]=%lf\n", MAXArr[0], MAXArr[1], MAXArr[510], MAXArr[511]);
    printf("MINArr[0]=%lf MINArr[1]=%lf MINArr[510]=%lf MINArr[511]=%lf\n", MINArr[0], MINArr[1], MINArr[510], MINArr[511]);
    printf("\n");

    printf("DataArray[0]   = %lf DataArray[1]   = %lf DataArray[2]   = %lf\n", AllData[0],   AllData[1],   AllData[2]);
    printf("DataArray[191] = %lf DataArray[192] = %lf DataArray[193] = %lf\n", AllData[191], AllData[192], AllData[193]);

    free(AVGArr);
    free(RMSArr);
    free(P2PArr);
    free(MAXArr);
    free(MINArr);
    free(AllData);

    FreeLibrary(hResDDC1128DLL);

    return 0;
}

