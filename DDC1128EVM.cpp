// DDC1128EVM.cpp (c) 2018 Dietmar Maurer
//

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

///////////////////////////////////////////////////

// extern "C" int __declspec(dllexport) XferINTDataIn(int *USBdev, int *INTData, long *DataLength);
typedef int16_t(WINAPI *XferINTDataIn_def)(int16_t *USBdev, int16_t *INTData, int32_t *DataLength);

// extern "C" int __declspec(dllexport) XferINTDataOut(int *USBdev, int *Data01, int *Data02, int *Data03, int *Data04);
typedef int16_t(WINAPI *XferINTDataOut_def)(int16_t *USBdev, int16_t *Data01, int16_t *Data02, int16_t *Data03, int16_t *Data04);

// extern "C" long __declspec(dllexport) WriteFPGARegsC(int *USBdev, long DUTSelect, long * RegsIn, long *  RegsOut, long * RegEnable);
typedef int32_t(WINAPI *WriteFPGARegsC_def)(int16_t *USBdev, int32_t *DUTSelect, int32_t *RegsIn, int32_t *RegsOut, int32_t *RegEnable);

// extern "C" long __declspec(dllexport) FastAllDataCap(double * AVGArr, double * RMSArr, double * P2PArr, double * MAXArr, double * MINArr, long ArrSize, long Channels, long nDVALIDReads, double * DataArray, long * AllDataAorBfirst);
typedef int32_t(WINAPI *FastAllDataCap_def)(double *AVGArr, double *RMSArr, double *P2PArr, double *MAXArr, double *MINArr, int32_t ArrSize, int32_t Channels, int32_t Samples, double *AllData, int32_t *AllDataAorBfirst);

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

    XferINTDataIn_def xferINTDataIn = (XferINTDataIn_def)GetProcAddress(hResDDC1128DLL, "XferINTDataIn");
    XferINTDataOut_def xferINTDataOut = (XferINTDataOut_def)GetProcAddress(hResDDC1128DLL, "XferINTDataOut");
    WriteFPGARegsC_def writefpgaregs = (WriteFPGARegsC_def)GetProcAddress(hResDDC1128DLL, "WriteFPGARegsC");
    FastAllDataCap_def fastalldatacap = (FastAllDataCap_def)GetProcAddress(hResDDC1128DLL, "FastAllDataCap");

    if (NULL == xferINTDataIn || NULL == xferINTDataOut || NULL == writefpgaregs || NULL == fastalldatacap) {
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

    RegsIn[0x09] = 24; RegsEnable[0x09] = 1; // Format
    RegsIn[0x0C] = 0x00; RegsEnable[0x0C] = 1; // to Ignore
    RegsIn[0x0D] = 0x00; RegsEnable[0x0D] = 1; // to Read MSB
    RegsIn[0x0E] = 0x04; RegsEnable[0x0E] = 1; // to read MIDB
    RegsIn[0x0F] = 0x00; RegsEnable[0x0F] = 1; // to read LSB
#if 0
    RegsIn[0x09] = 0x10; RegsEnable[0x09] = 1; // Format
    RegsIn[0x0C] = 0xFF; RegsEnable[0x0C] = 1; // to Ignore
    RegsIn[0x0D] = 0x00; RegsEnable[0x0D] = 1; // to Read MSB
    RegsIn[0x0E] = 0x04; RegsEnable[0x0E] = 1; // to read MIDB
    RegsIn[0x0F] = 0x00; RegsEnable[0x0F] = 1; // to read LSB
#endif
    ret = writefpgaregs(&USBdev, &DUTSelect, RegsIn, RegsOut, RegsEnable);
    if (ret == 0)
        printf("writefpgaregs ok\n");
    else
        printf("writefpgaregs failed\n");

    printf("Firmware version: %d\n", RegsOut[0x5E] << 8 | RegsOut[0x5F]);

    printf("Load CONV Low MSB: %d\n", RegsOut[0x01]);
    printf("Load CONV Low MIDB: %d\n", RegsOut[0x02]);
    printf("Load CONV Low LSB: %d\n", RegsOut[0x03]);
    printf("nDVALIDS (Samples) Format: %d\n", RegsOut[0x09]);
    printf("nDVALIDS (Samples) to Ignore: %d\n", RegsOut[0x0C]);
    printf("nDVALIDS (Samples) to Read: %d\n", RegsOut[0x0D] << 16 | RegsOut[0x0E] << 8 | RegsOut[0x0F]);

#if 1
    int Hard_reser_arr[] = { 0x0000, 0x15FF, 0x15FF, 0x1500, 0x1500, 0x15FF, 0x15FF };
    int count = 0;

    for (int i = 0; i < 7; i++) {
        int16_t RegH = (Hard_reser_arr[i] >> 12) & 0x000F;
        int16_t RegL = (Hard_reser_arr[i] >> 8) & 0x000F;
        int16_t DataH = (Hard_reser_arr[i] >> 4) & 0x000F;
        int16_t DataL = (Hard_reser_arr[i]) & 0x000F;
        int16_t ret = xferINTDataOut(&USBdev, &RegH, &RegL, &DataH, &DataL);
        if (ret == 0)
            count++;
    }
    if (count == 7)
        printf("Hard reset ok\n");
    else
        printf("Hard reset Unsucessfull\n");
#endif

#if 1
    int32_t ArrSize = 2 * Channels;
    int32_t Samples = 1024;
    int32_t AllDataAorBfirst = 0;

    double* AVGArr = (double*)malloc(ArrSize * sizeof(double));
    double* RMSArr = (double*)malloc(ArrSize * sizeof(double));
    double* P2PArr = (double*)malloc(ArrSize * sizeof(double));
    double* MAXArr = (double*)malloc(ArrSize * sizeof(double));
    double* MINArr = (double*)malloc(ArrSize * sizeof(double));

    double* AllData = (double*)malloc(Samples*Channels * sizeof(double));

    memset(AVGArr, 0, ArrSize * sizeof(double));
    memset(RMSArr, 0, ArrSize * sizeof(double));
    memset(P2PArr, 0, ArrSize * sizeof(double));
    memset(MAXArr, 0, ArrSize * sizeof(double));
    memset(MINArr, 0, ArrSize * sizeof(double));
    memset(AllData, 0, ArrSize * sizeof(double));

    ret = fastalldatacap(AVGArr, RMSArr, P2PArr, MAXArr, MINArr, ArrSize, Channels, Samples, AllData, &AllDataAorBfirst);
    if (ret < 0) {
        printf("fastalldatacap returned %d\n", ret);
        return 1;
    }

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

    printf("DataArray[0]   = %lf\n", AllData[0]);
    printf("DataArray[1]   = %lf\n", AllData[1]);
    printf("DataArray[2]   = %lf\n", AllData[2]);
    printf("...\n");
    printf("DataArray[191] = %lf\n", AllData[191]);
    printf("DataArray[192] = %lf\n", AllData[192]);
    printf("DataArray[193] = %lf\n", AllData[193]);
    printf("...\n");

    free(RegsIn);
    free(RegsOut);
    free(RegsEnable);

    free(AVGArr);
    free(RMSArr);
    free(P2PArr);
    free(MAXArr);
    free(MINArr);
    free(AllData);

    FreeLibrary(hResDDC1128DLL);
#endif
    return 0;
}

