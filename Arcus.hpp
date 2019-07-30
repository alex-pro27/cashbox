#pragma once
#ifndef ARCUS_H
#define ARCUS_H

#ifdef _MSC_VER
#pragma pack(pop)
#endif
#define IMPORTDLL extern "C" __declspec(dllimport)

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>

using namespace std;

enum {
	lnProcCode = 7,
	lnRFU = 60,
	lnPan = 20,
	lnExpiry = 5,
	lnAmount = 13,
	lnCurrency = 4,
	lnTerminalID = 9,
	lnSpdhTerminalID = 17,
	lnRetrievalReference = 13,
	lnAuthIdentResponse = 9,
	lnResponseCode = 4,
	lnCardType = 80,
	lnDate = 7,
	lnTime = 7,
	lnBatchNum = 8,
	lnRFUCredit = 50,
	lnPinblock = 17,
	lnPayData = 50,
	lnPayId = 3,
	lnMtid = 5,
	lnReceivedTextMsg = 80,
	lnAID = 80,
	lnApplicationLabel = 80,
	lnTVR = 80,
	lnCardholderName = 27,
	lnTraceID = 61,
	lnDateTime = 13
};

struct UserAuthIntFull {
	int handle;
	int abg_id;
	int operType;							//[in] Код операции (кассовый)
	char rfu[lnRFU];
	char pan[lnPan];
	char expiry[lnExpiry];
	char pay_acc[lnPan];
	char additional_payment_data[80];
	char amount[lnAmount];					//[in] Сумма в копейках
	char original_amount[lnAmount];			//[in] Оригинальная сумма в копейках
	char currency[lnCurrency];				//[in] Код валюты
	char terminalID[lnTerminalID];
	char rrn[lnRetrievalReference];			//[in][out] Ссылка (только для операций для которых
											//нужно, в остальных случаях - пуста)
	char authCode[lnAuthIdentResponse];		//[out][in] Код авторизации
	char responseCode[lnResponseCode];		//[out] Код ответа
	char cardType[lnCardType];				//[out] Название типа карты
	char date[lnDate];						//[out] Дата транзакции
	char time[lnTime];						//[out] Время транзакции
	char payment_data[lnPayData];					// данные для отправки на хост
	char data_to_print[lnPayData];					// данные для печати на чеке
	char home_operator[lnPayData];					// данные для печати на чеке
	char received_text_message[lnReceivedTextMsg];
	char text_message[lnReceivedTextMsg];	//[out] Расшифровка
	char AID[lnAID];
	char ApplicationLabel[lnApplicationLabel];
	char TVR[lnTVR];
	int system_res;
	unsigned short receiptNumber;			//номер чека
	unsigned short Invoice;					//номер платежа
	int operatopn_type;						//тип операции
	char card_number[20];					//номер карты
	char merchant[80];						//Идентификатор организации
	char date_buffer[100];
	unsigned short debit_number;
	char debit_total[7];
	char debit_clear_total[7];
	unsigned short return_number;
	char return_total[7];
	char return_clear_total[7];
	unsigned long butch_num;				//номер смены / пакета
	char TrAmount[13];
	char enc_data[64];
	char cardholder_name[lnCardholderName];	//имя владельца карт
	char max_discount[lnAmount];
	char min_discount[lnAmount];
	char commission_amount[lnAmount + 1];	//сумма комиссии (в минимальных единицах валюты)
	char PaymentsData[102]; //в unipay (каждое поле должно быть нултерминировано)
	//PaymentsData[0:2] – идентификатор платежа;
	//PaymentsData[3:35] – Поле платежа, значение 1;
	//PaymentsData[36:68] - Поле платежа, значение 2;
	//PaymentsData[69:101] - Поле платежа, значение 3;
	char BIN[7];
	char Hash[41];
	char Last4Digits[7];
	char isDiscoutedGoods;
	char Balance[lnAmount + 1];
	char TraceID[lnTraceID];
	char OriginalDateTime[lnDateTime];
	unsigned short CashRecieptNumber;
	char OutPutTransactionData[256];
	char TransactionID[19];
	char ComissionOffline[lnAmount];
	char ComissionOfflineCurrency[lnCurrency];
	char ComissionAcquirer[lnAmount];
	char ComissionAcquirerCurrency[lnCurrency];
	char ComissionIssuer[lnAmount];
	char ComissionIssuerCurrency[lnCurrency];
	char AuthorizedAmount[lnAmount];
	char AuthorizedAmountCurrency[lnCurrency];
	char HostResponseCode[lnResponseCode];
	char HostDialectName[32];
	unsigned char CardTypeLimitation; //признак процессинга, см. CARD_TYPE_*
	char RequestedDiscount[lnAmount]; //сумма в минимальных единицах валюты, котору пользователь хочет оплатить баллами
	unsigned char PosEntryMode; //режим ввода карты
	char NoICCremoving;
	//char Envelope9F70[LLC_ENVELOPELEN]; //содержимое тега 9F70 карточки (проект LLC)
	//char EqualDeal[LLC_EQUALDEALLEN]; //содержимое тега EqualDeal (проект LLC)
	//char HostDesc[LLC_HOSTDESCLEN]; //содержимое тега HostDesc (проект LLC)
	int EqualDealLen; //длина тега EqualDeal (проект LLC)
	int HostDescLen; //длина тега HostDesc (проект LLC)
	//union {
		//struct TLV_data_t as_tlv;
		//unsigned char storage[34];
	//} Tag9F6E;
	int OriginalOperType; //[in] Код оригинальной операции при отмене вне смены(только для продукта OW / Fuel + )
		/* add boundary padding here if needed */
};

typedef int(__cdecl* _PCPOSFUNCFULL)(UserAuthIntFull* auth_st_full, int size);

class ArcusHandlers {

private:
	_PCPOSFUNCFULL ProcessOwFull;
	HMODULE hLib;
public:
	UserAuthIntFull auth_data;
	ArcusHandlers();
	~ArcusHandlers();
	void auth(void);
	void purchase(char*);
	void cancel();
	void cancelLast();
	void cancelByLink(char*, char*);
	void apply();
	string getCheque(void);
};


ArcusHandlers::ArcusHandlers() {
	LPCSTR lib_path("C:\\Arcus2\\DLL\\ArcCom.dll");
	this->hLib = LoadLibraryA(lib_path);
	this->ProcessOwFull = (_PCPOSFUNCFULL)GetProcAddress(this->hLib, "ProcessOwFull");
}

ArcusHandlers::~ArcusHandlers() {
	FreeLibrary(this->hLib);
}

void ArcusHandlers::apply() {
	ProcessOwFull(&this->auth_data, sizeof(UserAuthIntFull));
}

void ArcusHandlers::auth() {
	UserAuthIntFull auth_data;
	memset(&auth_data, 0, sizeof(UserAuthIntFull));
	this->auth_data = auth_data;
}

void ArcusHandlers::purchase(char* amount) {
	this->auth_data.operType = 1;
	strncpy_s(this->auth_data.amount, sizeof(this->auth_data.amount), amount, sizeof(amount) * 4);
	this->apply();
}

void ArcusHandlers::cancel() {
	this->auth_data.operType = 3;
	this->apply();
}

void ArcusHandlers::cancelLast() {
	this->auth_data.operType = 2;
	this->apply();
}

void ArcusHandlers::cancelByLink(char* rrn, char* amount) {
	this->auth_data.operType = 3;
	strncpy_s(this->auth_data.rrn, sizeof(this->auth_data.rrn), rrn, sizeof(rrn) * 4);
	strncpy_s(this->auth_data.amount, sizeof(this->auth_data.amount), amount, sizeof(amount) * 4);
	this->apply();
}

string ArcusHandlers::getCheque() {
	ifstream cheque(L"C:\\Arcus2\\cheq.out");
	ostringstream data;
	string line;
	if (cheque.is_open()) {
		while (getline(cheque, line)) {
			data << line;
		}
		cheque.close();
	}
	return data.str();
}

#endif