#pragma once
#ifndef PrintLib_H
#define PrintLib_H

#pragma comment(lib, "C:\\Pirit2f\\PiritLib.lib")
#define IMPORTDLL extern "C" __declspec(dllimport)
#define PIRITLIB_CALL __stdcall

#define CORRECTION_INDEPENDENT 0x00
#define CORRECTION_PRESCRIPTION 0x01
#define CORRECTION_INCOMING 0x00
#define CORRECTION_OUTGOING 0x02
#define CORRECTION_TAX_1 0x00
#define CORRECTION_TAX_2 0x04
#define CORRECTION_TAX_4 0x08
#define CORRECTION_TAX_8 0x0C
#define CORRECTION_TAX_16 0x10
#define CORRECTION_TAX_32 0x14

#define DEVICE_MODE_DATA_ENCRYPTION 0x00000001UL
#define DEVICE_MODE_OFFLINE 0x00000002UL
#define DEVICE_MODE_AUTOMATIC 0x00000004UL
#define DEVICE_MODE_SERVICE_SIGN 0x00000008UL
#define DEVICE_MODE_BSO_SING 0x00000010UL
#define DEVICE_MODE_CALC_ONLINE_SIGN 0x00000020UL
#define DEVICE_MODE_SALE_EXCISABLE_GOODS 0x00000040UL
#define DEVICE_MODE_SIGN_OF_GAMBLING 0x00000080UL
#define DEVICE_MODE_SIGN_OF_BANK_PAYMENT_AGENTS 0x00000100UL
#define DEVICE_MODE_SIGN_OF_BANK_PAYMENT_SUBAGENTS 0x00000200UL
#define DEVICE_MODE_SIGN_OF_PAYMENT_AGENTS 0x00000400UL
#define DEVICE_MODE_SIGN_OF_PAYMENT_SUBAGENTS 0x00000800UL
#define DEVICE_MODE_SIGN_OF_ATTORNEY 0x00001000UL
#define DEVICE_MODE_SIGN_OF_COMISSIONERS 0x00002000UL
#define DEVICE_MODE_SIGN_OF_AGENTS 0x00004000UL
#define DEVICE_MODE_SIGN_OF_LOTTERY 0x00008000UL

enum CoefficientTypes
{
	NONE = 0, DISCOUNT_PERCENT, DISCOUNT_SUM, MARGIN_PERCENT, MARGIN_SUM
};

typedef struct tagPLDateTime {
	int year;
	unsigned char month;
	unsigned char day;
	unsigned char hours;
	unsigned char minutes;
	unsigned char seconds;
}PLDateTime;

typedef struct tagPLDate {
	int year;
	unsigned char month;
	unsigned char day;
}PLDate;

typedef struct tagPLTime {
	unsigned char hours;
	unsigned char minutes;
	unsigned char seconds;
}PLTime;

typedef struct tagMData {
	int errCode;
	char data[256];
	int dataLength;
}MData;

IMPORTDLL int PIRITLIB_CALL openPort(const char *fileName, long speed);
IMPORTDLL int PIRITLIB_CALL closePort();
IMPORTDLL int PIRITLIB_CALL checkPortAsync(const char *fileName, long speed);
IMPORTDLL int PIRITLIB_CALL commandStart();
IMPORTDLL MData PIRITLIB_CALL libGetStatusFlags();
IMPORTDLL int PIRITLIB_CALL getStatusFlags(int *fatalStatus, int *currentFlagsStatus, int *documentStatus);
IMPORTDLL MData PIRITLIB_CALL libGetCountersAndRegisters(unsigned char numRequest);
IMPORTDLL MData PIRITLIB_CALL libGetKKTInfo(unsigned char numRequest);
IMPORTDLL int PIRITLIB_CALL getKKTInfo(unsigned char numRequest, char *data);
IMPORTDLL MData PIRITLIB_CALL libGetReceiptData(unsigned char numRequest);
IMPORTDLL MData PIRITLIB_CALL libGetPrinterStatus();
IMPORTDLL MData PIRITLIB_CALL libGetServiceInfo(unsigned char numRequest);
IMPORTDLL MData PIRITLIB_CALL libGetExErrorInfo(unsigned char numRequest);
IMPORTDLL int PIRITLIB_CALL scrollPaper();
IMPORTDLL int PIRITLIB_CALL libCommandStart(PLDate mpDate, PLTime mpTime);
IMPORTDLL MData PIRITLIB_CALL libReadSettingsTable(unsigned char number, int index);
IMPORTDLL int PIRITLIB_CALL libWriteSettingsTable(unsigned char number, int index, const char* data);
IMPORTDLL MData PIRITLIB_CALL libGetPiritDateTime();
IMPORTDLL int PIRITLIB_CALL libSetPiritDateTime(PLDate mpDate, PLTime mpTime);
IMPORTDLL int PIRITLIB_CALL libLoadLogo(int size, unsigned char* data);
IMPORTDLL int PIRITLIB_CALL libDeleteLogo();
IMPORTDLL int PIRITLIB_CALL libLoadReceiptDesign(int size, unsigned char* data);
IMPORTDLL int PIRITLIB_CALL libLoadPicture(int width, int height, int sm, const char* name, int number, unsigned char* data);
IMPORTDLL int PIRITLIB_CALL libPrintXReport(const char* nameCashier);
IMPORTDLL int PIRITLIB_CALL libPrintZReport(const char* nameCashier, int options);
IMPORTDLL int PIRITLIB_CALL libOpenDocument(unsigned char type, unsigned char numDepart, char* nameCashier, long docNumber);
IMPORTDLL int PIRITLIB_CALL libOpenDocumentEx(unsigned char type, unsigned char numDepart, char* nameCashier, long docNumber, unsigned char taxN);
IMPORTDLL unsigned long PIRITLIB_CALL libSetBuyerAddress(const char *buyerAddress);
IMPORTDLL unsigned long PIRITLIB_CALL libGetBuyerAddress(char *buyerAddress, unsigned long baLength);
IMPORTDLL void PIRITLIB_CALL libCleanBuyerAddress();
IMPORTDLL MData PIRITLIB_CALL libCloseDocument(unsigned char cutPaper);
IMPORTDLL int PIRITLIB_CALL libCancelDocument();
IMPORTDLL int PIRITLIB_CALL libPostponeDocument(const char* info);
IMPORTDLL int PIRITLIB_CALL libCutDocument();
IMPORTDLL int PIRITLIB_CALL libPrintString(char* textStr, unsigned char attribute);
IMPORTDLL int PIRITLIB_CALL libPrintBarCode(unsigned char posText, unsigned char widthBarCode, unsigned char heightBarCode, unsigned char typeBarCode, const char* barCode);
IMPORTDLL int PIRITLIB_CALL libAddPosition(const char* goodsName, const char* barcode, double quantity, double price, unsigned char taxNumber, int numGoodsPos, unsigned char numDepart, unsigned char coefType, const char *coefName, double coefValue);
IMPORTDLL int PIRITLIB_CALL libAddPositionEx(const char* goodsName, const char* barcode, double quantity, double price, unsigned char taxNumber, int numGoodsPos, unsigned char numDepart, long signMethodCalculation, long signCalculationObject);
IMPORTDLL int PIRITLIB_CALL libAddPositionLarge(const char* goodsName, const char* barcode, double quantity, double price, unsigned char taxNumber, int numGoodsPos, unsigned char numDepart, unsigned char coefType, double coefValue, long signMethodCalculation, long signCalculationObject);
IMPORTDLL int PIRITLIB_CALL libDelPosition(const char* goodsName, const char* barcode, double quantity, double price, unsigned char taxNumber, int numGoodsPos, unsigned char numDepart);
IMPORTDLL int PIRITLIB_CALL libSubTotal();
IMPORTDLL int PIRITLIB_CALL libAddDiscount(unsigned char typeDiscount, const char* nameDiscount, long sum);
IMPORTDLL int PIRITLIB_CALL libAddMargin(unsigned char typeMargin, const char* nameMargin, long sum);
IMPORTDLL int PIRITLIB_CALL libAddPayment(unsigned char typePayment, long long sum, const char* infoStr);
IMPORTDLL int PIRITLIB_CALL libCashInOut(const char* infoStr, long long sum);
IMPORTDLL int PIRITLIB_CALL libPrintRequsit(unsigned char codeReq, unsigned char attributeText, const char* str1, const char* str2, const char* str3, const char* str4);
IMPORTDLL int PIRITLIB_CALL libPrintRequsitOFD(int codeReq, unsigned char attributeText, const char* reqName, const char* reqStr);
IMPORTDLL int PIRITLIB_CALL libRegisterSumToDepart(unsigned char typeOperation, unsigned char numberDepart, long sum);
IMPORTDLL int PIRITLIB_CALL libRegisterTaxSum(unsigned char numberTax, long sum);
IMPORTDLL int PIRITLIB_CALL libCompareSum(long sum);
IMPORTDLL int PIRITLIB_CALL libOpenCopyReceipt(unsigned char type, unsigned char numDepart, const char* nameCashier, int numCheck, int numCash, PLDate mpDate, PLTime mpTime);
IMPORTDLL int PIRITLIB_CALL libSetToZeroCashInCashDrawer();
IMPORTDLL int PIRITLIB_CALL libPrintPictureInDocument(int width, int height, int sm, unsigned char* data);
IMPORTDLL int PIRITLIB_CALL libPrintPreloadedPicture(int sm, int number);
IMPORTDLL int PIRITLIB_CALL libTechnologicalReset(const PLDateTime *dateTime);
IMPORTDLL int PIRITLIB_CALL libFiscalization(const char *oldPassword, const char *regNumber, const char *INN, const char *newPassword);
IMPORTDLL int PIRITLIB_CALL libPrintFiscalReportByShifts(unsigned char typeReport, int startShiftNumber, int endShiftNumber, const char *password);
IMPORTDLL int PIRITLIB_CALL libPrintFiscalReportByDate(unsigned char typeReport, PLDate startDate, PLDate endDate, const char *password);
IMPORTDLL int PIRITLIB_CALL libActivizationECT();
IMPORTDLL int PIRITLIB_CALL libCloseArchiveECT();
IMPORTDLL int PIRITLIB_CALL libCloseFN(const char *cashierName);
IMPORTDLL int PIRITLIB_CALL libPrintControlTapeFromECT(int shiftNumber);
IMPORTDLL int PIRITLIB_CALL libPrintDocumentFromECT(int KPKNumber);
IMPORTDLL int PIRITLIB_CALL libPrintReportFromECTByShifts(unsigned char typeReport, int startShiftNumber, int endShiftNumber);
IMPORTDLL int PIRITLIB_CALL libPrintReportFromECTByDate(unsigned char typeReport, PLDate startDate, PLDate endDate);
IMPORTDLL int PIRITLIB_CALL libPrintReportActivizationECT();
IMPORTDLL int PIRITLIB_CALL libPrintReportFromECTByShift(int shiftNumber);
IMPORTDLL MData PIRITLIB_CALL libGetInfoFromECT(unsigned char number, long dataL1, long dataL2);
IMPORTDLL int PIRITLIB_CALL libOpenCashDrawer(int pulseDuration);
IMPORTDLL MData PIRITLIB_CALL libGetCashDrawerStatus();
IMPORTDLL int PIRITLIB_CALL getCashDrawerStatus(int *drawerStatus);
IMPORTDLL int PIRITLIB_CALL libBeep(int duration);
IMPORTDLL int PIRITLIB_CALL libAuthorization(PLDate mpDate, PLTime mpTime, const char *numKKT);
IMPORTDLL MData PIRITLIB_CALL libReadMemoryBlock(unsigned char type, long startAdress, long numBytes);
IMPORTDLL int PIRITLIB_CALL libSetSpeed(unsigned char numSpeed);
IMPORTDLL int PIRITLIB_CALL libPrintServiceData();
IMPORTDLL int PIRITLIB_CALL libEmergencyCloseShift();
IMPORTDLL int PIRITLIB_CALL libPrintCopyLastZReport();
IMPORTDLL int PIRITLIB_CALL libEnableServiceChannelToECT();
IMPORTDLL int PIRITLIB_CALL libPrintCopyReportFiscalization();
IMPORTDLL int PIRITLIB_CALL getCurMPTime(PLDate *mpDate, PLTime *mpTime);
IMPORTDLL int PIRITLIB_CALL saveLogoToFile(wchar_t *fileName);
IMPORTDLL void PIRITLIB_CALL setDebugLevel(int level);
IMPORTDLL long long PIRITLIB_CALL getDriverVersion();
IMPORTDLL int PIRITLIB_CALL libPrintDocsFromECTSDByNumberDoc(int startNumber, int endNumber);
IMPORTDLL int PIRITLIB_CALL libPrintDocsFromECTSDByNumberShift(int startNumber, int endNumber);
IMPORTDLL int PIRITLIB_CALL libPrintDocsFromECTSDByDate(PLDate mpDateStart, PLDate mpDateEnd);
IMPORTDLL int PIRITLIB_CALL libGetInfoFromECT_NumberDoc(int *numDoc);
IMPORTDLL int PIRITLIB_CALL libGetInfoFromECT_ShiftNumber(int *shiftNumber);
IMPORTDLL int PIRITLIB_CALL libBLRPrintControlTapeFromECT();
IMPORTDLL int PIRITLIB_CALL getCountersAndRegisters(int requestNumber, int *data);
IMPORTDLL void PIRITLIB_CALL setAmountDecimalPlaces(int decimalPlaces);
IMPORTDLL void PIRITLIB_CALL setQuantityDecimalPlaces(int decimalPlaces);
IMPORTDLL void PIRITLIB_CALL setPercentageDecimalPlaces(int decimalPlaces);
IMPORTDLL int PIRITLIB_CALL getAmountDecimalPlaces();
IMPORTDLL int PIRITLIB_CALL getQuantityDecimalPlaces();
IMPORTDLL int PIRITLIB_CALL getPercentageDecimalPlaces();
IMPORTDLL int PIRITLIB_CALL libGetCountersAndRegisters_12(int data[], int maxElement, char *str);
IMPORTDLL int PIRITLIB_CALL getPrinterStatus(int* result);
IMPORTDLL int PIRITLIB_CALL getPiritDateTime(int* cDate, int* cTime);
IMPORTDLL int PIRITLIB_CALL libOpenShift(const char* nameCashier);
IMPORTDLL int PIRITLIB_CALL libSetExtraRequisite(const char* nomenclatureCode, const char* extReq, const char* measureName, int agentSign, const char* supplierINN, const char* supplierPhone, const char* supplierName, const char* operatorAddress, const char* operatorINN, const char* operatorName, const char* operatorPhone, const char* payAgentOperation, const char* payAgentPhone, const char* recOperatorPhone);
IMPORTDLL int PIRITLIB_CALL libDoCheckCorrection(const char *nameCashier, double cash, double cashless, unsigned char correctionFlags);
IMPORTDLL int PIRITLIB_CALL libDoCheckCorrectionEx(const char *nameCashier, double cash, double cashless, double sum1, double sum2, double sum3, unsigned char correctionFlags, const PLDate *docDate, const char *docNumber, const char *correctionName, double sum_18, double sum_10, double sum_0, double sum_WT, double sum_18_118, double sum_10_110);
IMPORTDLL int PIRITLIB_CALL libCurrentStatusReport(char* nameCashier);
IMPORTDLL int PIRITLIB_CALL libAddPaymentD(unsigned char typePayment, double sum, const char* infoStr);
IMPORTDLL int PIRITLIB_CALL libRegistration(unsigned char type, const char *regNumber, const char *INN, int systemTax, int rej, const char *cashierName);
IMPORTDLL int PIRITLIB_CALL libRegistrationEx(unsigned char type, const char *regNumber, const char *INN, int systemTax, int rej, const char *cashierName, PLDateTime regDateTime, const char *userName1, const char *userName2, const char *addressSettle1, const char *addressSettle2, const char *placeSettle, const char *autoNumber, const char *OFDName, const char *OFDINN, const char *senderEmail, const char *FNSEmail, bool automatic);
IMPORTDLL int PIRITLIB_CALL libGetInfoFromECT_NumberFP(char *data);
IMPORTDLL int PIRITLIB_CALL libGetInfoFromECT_FP(int numDoc, char *data);
IMPORTDLL unsigned long PIRITLIB_CALL libFormatMessage(int errorCode, char *msgBuffer, unsigned long cbBuffer);

#endif // PrintLib_H