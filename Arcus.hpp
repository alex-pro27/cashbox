#pragma once
#ifndef ARCUS_H
#define ARCUS_H

#include <iostream>
#include <string>
#include <conio.h>
#include <windows.h>
#include <regex>
#include <map>

using namespace std;

typedef void* (__cdecl* ARCUS_CREATE)();
typedef void(__cdecl* ARCUS_DELETE)(void*);
typedef int(__cdecl* ARCUS_SET)(void*, const char*, const char*, int);
typedef int(__cdecl* ARCUS_GET)(void*, const char*, char*, int);
typedef int(__cdecl* ARCUS_RUN)(void*, int);
typedef int(__cdecl* ARCUS_RUN_CMD)(void*, const char*, const char*, int);
typedef int(__cdecl* ARCUS_CLEAR)(void*);

LPCSTR dll_name("\\Arcus2\\DLL\\arccom.dll");
class ArcusHandlers {

private:
	void* pos_obj = NULL;
	ARCUS_DELETE ArcusDelete = NULL;
	ARCUS_CREATE ArcusCreate;
	ARCUS_SET ArcusSet;
	ARCUS_GET ArcusGet;
	ARCUS_RUN ArcusRun;
	ARCUS_RUN_CMD ArcusRunCmd;
	ARCUS_CLEAR ArcusClear;
	string cheque;
public:
	ArcusHandlers();
	~ArcusHandlers();
	void* getPosObj();
	int closeShift(void);
	int purchase(char* amount, char* currency);
	int cancelLast();
	int cancelByLink(char*, char*);
	int universalCancel();
	void clean();
	char* getStr(char* name);
	char* getRRN();
	char* getMessage();
	char* getPANCard();
	char* getCardHolderName();
	int getResponseCode();
	string getCheque(bool new_read);
};

ArcusHandlers::ArcusHandlers() {
	HINSTANCE dll = LoadLibraryA(dll_name);
	if (dll == NULL) {
		/// exit if DLL file not loaded
		throw std::runtime_error("Arcus DLL not Load");
	}
	ArcusDelete = (ARCUS_DELETE)GetProcAddress(dll, "DeleteITPos");
	ArcusCreate = (ARCUS_CREATE)GetProcAddress(dll, "CreateITPos");
	ArcusSet = (ARCUS_SET)GetProcAddress(dll, "ITPosSet");
	ArcusGet = (ARCUS_GET)GetProcAddress(dll, "ITPosGet");
	ArcusRun = (ARCUS_RUN)GetProcAddress(dll, "ITPosRun");
	ArcusRunCmd = (ARCUS_RUN_CMD)GetProcAddress(dll, "ITPosRunCmd");
	ArcusClear = (ARCUS_CLEAR)GetProcAddress(dll, "ITPosClear");
	/// check DLL functions
	if (ArcusCreate == NULL) throw std::runtime_error("function ArcusCreate not loaded");
	if (ArcusDelete == NULL) throw std::runtime_error("function ArcusDelete not loaded");
	if (ArcusSet == NULL) throw std::runtime_error("function ArcusSet not loaded");
	if (ArcusGet == NULL) throw std::runtime_error("function ArcusGet not loaded");
	if (ArcusRun == NULL) throw std::runtime_error("function ArcusRun not loaded");
	if (ArcusRunCmd == NULL) throw std::runtime_error("function ArcusRunCmd not loaded");
	if (ArcusClear == NULL) throw std::runtime_error("function ArcusClear not loaded");
	pos_obj = ArcusCreate();
	if (pos_obj == NULL) {
		/// exit if object not created
		throw std::runtime_error("create object fail");
	}
}

void* ArcusHandlers::getPosObj() {
	return this->pos_obj;
}

ArcusHandlers::~ArcusHandlers() {
	if ((ArcusDelete != NULL) && (pos_obj != NULL)) {
		ArcusDelete(pos_obj);
	}
}

int ArcusHandlers::closeShift() {
	return ArcusRun(pos_obj, 11);
}

void ArcusHandlers::clean() {
	ArcusClear(pos_obj);
}

char* ArcusHandlers::getStr(char* name) {
	int size = ArcusGet(pos_obj, name, NULL, -1);
	if (size <= 0) {
		return "";
	}
	char* value = new char[size + 1];
	if (ArcusGet(pos_obj, name, value, size + 1) < 0) {
		return "";
	}
	return value;
}

char* ArcusHandlers::getRRN() {
	string value = "";
	string cheque = getCheque(false);
	smatch matches;
	if (std::regex_search(cheque, matches, std::regex("(?:RRN:([0-9]+))"))) {
		if (matches.size() == 2) {
			value = matches[1];
		}
	}
	if (value == "") {
		value = std::regex_replace(string(this->getStr("transaction_id")), std::regex("[^0-9]"), "");
	}
	return (char*)value.c_str();
}

char* ArcusHandlers::getMessage() {
	auto value = std::regex_replace(string(this->getStr("text_message")), std::regex("[^0-9аА-яЯёЁaA-zZ\\s]"), "");
	return (char*)value.c_str();
}

char* ArcusHandlers::getPANCard() {
	string value = "";
	string cheque = getCheque(false);
	smatch matches;
	if (std::regex_search(cheque, matches, std::regex("(?:PAN:([*0-9]+))"))) {
		if (matches.size() == 2) {
			value = matches[1];
		}
	}
	if (value == "") {
		value = std::regex_replace(string(this->getStr("pan")), std::regex("[^0-9\\*]"), "");
	}
	return (char*)value.c_str();
}

char* ArcusHandlers::getCardHolderName() {
	auto value = std::regex_replace(string(this->getStr("cardholder_name")), std::regex("[^0-9аА-яЯёЁaA-zZ\\s]"), "");
	return (char*)value.c_str();
}

int ArcusHandlers::getResponseCode() {
	char* code = getStr("response_code");
	if (strlen(code)) {
		auto value = std::regex_replace(string(code), std::regex("[^0-9]"), "");
		return stoi(value);
	}
	return 0;
}

int ArcusHandlers::purchase(char* amount, char* currency = "643") {
	if (ArcusSet(pos_obj, "amount", amount, -1) != 0) return 1;
	if (ArcusSet(pos_obj, "currency", currency, -1) != 0) return 1;
	return ArcusRun(pos_obj, 1);
}

int ArcusHandlers::cancelLast() {
	// Отмена последней транзакции
	return ArcusRun(pos_obj, 2);
}

int ArcusHandlers::universalCancel() {
	return ArcusRun(pos_obj, 4);
}

int ArcusHandlers::cancelByLink(char* amount, char* rrn = NULL) {
	if (rrn != NULL && strlen(rrn) > 3) {
		ArcusSet(pos_obj, "transaction_id", rrn, -1);
	}
	if (ArcusSet(pos_obj, "amount", amount, -1) != 0) return 1;
	return ArcusRun(pos_obj, 3);
}

string ArcusHandlers::getCheque(bool new_read = false) {
	if (new_read || !this->cheque.size()) {
		ifstream cheque(L"\\Arcus2\\cheq.out");
		ostringstream data;
		string line;
		if (cheque.is_open()) {
			while (getline(cheque, line)) {
				data << line;
			}
			cheque.close();
		}
		this->cheque = data.str();
	}
	return this->cheque;
}

#endif