#include <Python.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <tuple>
#include <regex>
#include <map>
#include <cmath>
#include <windows.h>
#include <wchar.h>
#include "Helpers.hpp"
#include "PiritLib.h"
#include "Arcus.hpp"

using namespace Helpers;
using namespace std;

typedef MData(__stdcall* libMethod)(unsigned char numRequest);

string requestDecorator(int num, libMethod method) {
	MData inf = method(num);
	if (inf.errCode == 0) {
		string data(inf.data);
		return data.substr(8, inf.dataLength - 12);
	}
	else {
		return string("");
	}
};

PyObject* createPyDict(map<string, PyObject*> data) {
	PyObject* rslt = PyDict_New();
	for (auto it = data.begin(); it != data.end(); ++it) {
		PyDict_SetItem(rslt, PyUnicode_FromString(it->first.c_str()), it->second);
	}
	return rslt;
}

void setParsedData(map<char*, int> keys, string kktinfo, map<string, PyObject*> *data) {
	auto s = split(kktinfo, "\x1c");
	for (auto it = keys.begin(); it != keys.end(); ++it) {
		string value = s[it->second];
		if (regex_match(value, std::regex("^(0|[1-9]+0?)\\.[0-9]+$"))) {
			(*data)[it->first] = PyFloat_FromDouble(stold(value));
		}
		else if (regex_match(value, std::regex("^[1-9][0-9]+$"))) {
			(*data)[it->first] = PyLong_FromLongLong(stoll(value));
		}
		else {
			(*data)[it->first] = PyUnicode_FromString(value.c_str());
		}
	}
}

std::tuple<int, wstring> getExError() {
	MData inf = libGetExErrorInfo(1);
	auto r = split(inf.data, "\x1c");
	if (r.size() < 2) {
		return std::make_tuple(0, L"");
	}
	int err_code = stoi(r[1]);
	wstring message;
	switch (err_code) {
	case 1:
		message = L"Не была вызвана функция “Начало работы”";
		break;
	case 2:
		message = L"Не фискальный режим";
		break;
	case 3:
		message = L"Архив ФН закрыт";
		break;
	case 4:
		message = L"ФН не зарегистрирован";
		break;
	case 5:
		message = L"ФН уже зарегистрирован";
		break;
	case 8:
		message = L"Документ не был открыт";
		break;
	case 9:
		message = L"Предыдущий документ не закрыт";
		break;
	case 15:
		message = L"Документ закрыт в ФН";
		break;
	case 16:
		message = L"Документ не является продажей (приходом) или возвратом (возвратом прихода)";
		break;
	case 17:
		message = L"Документ не является внесением или изъятием";
		break;
	case 20:
		message = L"Смена не открыта";
		break;
	case 21:
		message = L"Фатальная ошибка ФН";
		break;
	case 22:
		message = L"ФН не в режиме получения документа для ОФД";
		break;
	default:
		err_code = 0;
	}
	return std::make_tuple(err_code, message);
}

std::tuple<int, wstring> checkStatusPrinter() {
	std::tuple<int, wstring> err = getExError();
	int err_code = std::get<0>(err);
	wstring message = std::get<1>(err);
	int fnd_stack[9]{ 2,3,4,5,8,15,20,21,22 };
	if (err_code == 1) {
		commandStart();
	}
	else if (err_code == 9) {
		//libCancelDocument();
	}
	else if (in_array<int, 9>(fnd_stack, err_code)) {
		return err;
	}
	return std::make_tuple(0, L"");
}

/* Получить сумму скидок/наценок */
void addMarckupAndDiscountInfo(map<string, PyObject*> *data) {
	string kktinfo = requestDecorator(9, libGetCountersAndRegisters);
	if (kktinfo != "") {
		map<char*, int> keys = {
			{"discount_sum_sales", 0},		// Сумма скидок по продажам
			{"marckup_sum_sales", 1},		// Сумма наценок по продажам
			{"discount_sum_returns", 2},	// Сумма скидок по возвратам
			{"marckup_sum_returns", 3},		// Сумма наценок по возвратам
		};
		setParsedData(keys, kktinfo, data);
	}
}

void addDateTime(map<string, PyObject*>* data) {
	MData res = libGetPiritDateTime();
	if (!res.errCode && res.dataLength) {
		auto datetime = split(res.data, "\x1c");
		std::stringstream stream;
		std::regex date_pattern("(\\d{2})(\\d{2})(\\d{2})");
		auto date = std::regex_replace(datetime[0].substr(6, datetime[0].size() - 6), date_pattern, "20$3-$2-$1");
		auto time = std::regex_replace(datetime[1], date_pattern, "$1:$2:$3");
		stream << date << "T" << time;
		(*data)["datetime"] = PyUnicode_FromString(stream.str().c_str());
	}
}

/* Информация о фискальнике */
void addKKTINFO(map<string, PyObject*> *data) {
	/*ИНН*/
	(*data)["inn"] = PyUnicode_FromString(requestDecorator(3, libGetKKTInfo).c_str());
	/*Количество денег в денежном ящике*/
	string cash_balance = requestDecorator(7, libGetKKTInfo);
	if (cash_balance != "") {
		(*data)["cash_balance"] = PyFloat_FromDouble(stod(cash_balance));
	}
	/*Заводской номер фискальника*/
	(*data)["fn_number"] = PyUnicode_FromString(requestDecorator(1, libGetKKTInfo).c_str());
	addDateTime(data);
};

string getChequeNumber() {
	string kktinfo = requestDecorator(2, libGetReceiptData);
	auto s = split(kktinfo, "\x1c");
	if (s.size()) {
		return s[1];
	}
	return string("");
}

int getNextChequeNumber() {
	string cheque_number = getChequeNumber();
	if (cheque_number != "") {
		auto cn = split(cheque_number, ".");
		if (cn.size() > 1) {
			int number = stoi(cn[1]);
			return number + 1;
		}
		return 1;
	}
	return 0;
}

/* Информация о последнем чеке */
void addLastChequeInfo(map<string, PyObject*> *data) {
	string kktinfo = requestDecorator(2, libGetReceiptData);
	if (kktinfo != "") {
		map<char*, int> keys = {
			{"doc_number", 3},				// Номер документа
			{"discount_sum", 5},			// Сумма скидки по чеку
			{"discount_marckup_sum", 6},	// Сумма скидки наценки по чеку
			{"fp", 7},						// Фискальный признак
			{"fd", 8},						// фискальный документ
		};
		setParsedData(keys, kktinfo, data);
		(*data)["check_number"] = PyUnicode_FromString(getChequeNumber().c_str()); // Номер чека
	}
};

/* Нарастающий итог */
void addProgressiveTotalSales(map<string, PyObject*> *data) {
	string kktinfo = requestDecorator(12, libGetKKTInfo);
	if (kktinfo != "") {
		map<char*, int> keys = {
			{"progressive_total_sales", 0},		// Нарастающий итог продажи
			{"progressive_total_returns", 1},	// Нарастающий итог возврата
		};
		setParsedData(keys, kktinfo, data);
	}
};

/* Расширенная информация об ошибке */
void addExError(map<string, PyObject*>* data) {
	auto ex_err = getExError();
	if (std::get<0>(ex_err)) {
		(*data)["description_error"] = PyUnicode_FromWideChar(std::get<1>(ex_err).c_str(), std::get<1>(ex_err).size());
	}
}

/* Номер смены */
void addShiftNumber(map<string, PyObject*>* data, bool prev = false) {
	string shift_number = requestDecorator(1, libGetCountersAndRegisters);
	if (shift_number != "") {
		long long number = stoll(shift_number);
		if (prev) {
			number -= 1;
		}
		(*data)["shift_number"] = PyLong_FromLongLong(number);
	}
}

/* Z-Отчет */
void addZReport(map<string, PyObject*> *data) {
	addMarckupAndDiscountInfo(data);
	addProgressiveTotalSales(data);
	/* Вернуть данные по последнему Z отчету */
	string kktinfo = requestDecorator(12, libGetCountersAndRegisters);
	if (kktinfo != "") {
		map<char*, int> keys = {
			{"doc_number", 1},		// Номер документа
			{"cash_balance", 2},	// Сумма в кассе
			{"count_sales", 3},		// Количество продаж
			{"sum_sales", 4},		// Сумма продаж
			{"count_returns", 5},	// Количество возваратов
			{"sum_returns", 6},		// Сумма возваратов
			{"count_canceled", 7},	// Количество аннулированных
			{"sum_canceled", 8},	// Сумма аннулированных
			{"count_insert", 9},	// Количество внесений
			{"sum_insert", 10},		// Сумма внесений
			{"count_remove", 11},	// Количество изъятий
			{"sum_remove", 12},		// Сумма изъятий
		};
		setParsedData(keys, kktinfo, data);
	}
};

void printCheque() {
	// Напечатать чек оплаты по безналу (только если открыт документ)
	ifstream cheque(L"C:\\Arcus2\\cheq.out");
	string line;
	if (cheque.is_open()) {
		while (getline(cheque, line)) {
			libPrintRequsit(0, 1, cp2oem((char*)line.c_str()).c_str(), "", "", "");
		}
		cheque.close();
	}
};

void printStringInOpenDoc(const char* print_strings) {
	string str = utf2cp((char*)print_strings);
	vector<string> print_str = split(str, "\n");
	std::map <std::string, unsigned char> mapping;
	mapping["BIG"] = 5;
	mapping["BIG_BOLD"] = 6;
	mapping["NORMAL_BOLD"] = 3;
	mapping["NORMAL"] = 1;
	mapping["MEDIUM"] = 0;
	mapping["MEDIUM_BOLD"] = 2;
	mapping["SMALL"] = 4;
	std::regex pattern("\\(font-style=([A-Z_]+)\\)");
	for (string line : print_str) {
		std::smatch matches;
		std::regex_search(line, matches, pattern);
		unsigned char style = 1;
		if (matches.size() > 0 && mapping[matches[1]] != NULL) {
			style = mapping[matches[1]];
		}
		line = std::regex_replace(line.c_str(), pattern, "");
		libPrintRequsit(0, style,(char*)cp2oem((char*)line.c_str()).c_str(), "", "", "");
	}
}

void printOrderNumber(const char* order_prefix) {
	wstring print_str = L"\n \n"
		"\n      --------------------------------------------"
		"\n(font-style=BIG_BOLD)      НОМЕР ЗАКАЗА:" 
		"\n(font-style=BIG_BOLD)          {order_number}"
		"\n      --------------------------------------------"
		"\n \n";
	std::wstringstream order_number;
	order_number << s2ws(order_prefix) << getNextChequeNumber();
	wstring to_replace(L"{order_number}");
	print_str.replace(print_str.find(to_replace), to_replace.length(), order_number.str());
	printStringInOpenDoc(ws2s(print_str).c_str());
}

PyDoc_STRVAR(cashbox_set_datetime_doc, "set_datetime(datetime)\
set datetime for pirit\
:param str datetime: - %Y-%m-%dT%H:%M:%S");
PyObject* cashbox_set_datetime(PyObject* self, PyObject* args, PyObject* kwargs) {
	const char* datetime; static char* keywords[] = { "datetime", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &datetime)) {
		return NULL;
	}
	const std::regex date_pattern("(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})");
	std::smatch datetime_match;
	string dt(datetime);
	std::regex_search(dt.cbegin(), dt.cend(), datetime_match, date_pattern);
	if (datetime_match.size() == 0) {
		PyErr_SetString(PyExc_ValueError, ws2s(L"Дата не соответсвует стандарту ISO8601(YYYY-MM-DDThh:mm:ss)").c_str());
		return NULL;
	}
	int ee = 1;
	PLDate date{
		(int)stoi(datetime_match[1].str()) - 2000,
		(unsigned char)stoi(datetime_match[2].str()),
		(unsigned char)stoi(datetime_match[3].str())
	};
	PLTime time{
		(unsigned char)stoi(datetime_match[4].str()),
		(unsigned char)stoi(datetime_match[5].str()),
		(unsigned char)stoi(datetime_match[6].str())
	};

	if (libSetPiritDateTime(date, time)) {
		PyErr_SetString(PyExc_ValueError, ws2s(L"Ошибка изменения даты").c_str());
	}
	return NULL;
}

PyDoc_STRVAR(cashbox_open_port_doc, "open_port(port, speed)\
\
Open COM Port for Pirit");
PyObject* cashbox_open_port(PyObject* self, PyObject* args, PyObject* kwargs) {
	const char* port;
	int speed = 0;
	static char* keywords[] = { "port", "speed", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si", keywords, &port, &speed)) {
		return NULL;
	}
	int err_code = openPort(port, speed);
	wstring st = L"Успешно";
	if (err_code > 0)
		st = L"Нет связи с ККТ";
	
	auto message = PyUnicode_FromWideChar(st.c_str(), st.size());
	auto error = PyBool_FromLong(err_code);
	PyObject* rslt = PyTuple_New(2);
	PyTuple_SetItem(rslt, 0, error);
	PyTuple_SetItem(rslt, 1, message);
	return rslt;
}

PyDoc_STRVAR(cashbox_close_port_doc, "close_port()\
\
Close COM Port for Pirit");
PyObject* cashbox_close_port(PyObject* self) {
	closePort();
	Py_RETURN_NONE;
}

PyDoc_STRVAR(cashbox_open_shift_doc, "open_shift(cashier)\
\
Opening a cashier shift\
:param str cashier: - Cashier name\
:return dict()");
PyObject* cashbox_open_shift(PyObject* self, PyObject* args, PyObject* kwargs) {
	const char* cashier;
	static char* keywords[] = { "cashier", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &cashier)) {
		return NULL;
	}
	std::tuple<int, wstring> res = checkStatusPrinter();
	wstring st = L"Успешно";
	int err_code = std::get<0>(res);
	if (err_code) {
		st = std::get<1>(res);
	}
	else {
		err_code = libOpenShift((char*)utf2oem((char*)cashier).c_str());
		if (err_code > 0) {
			st = L"Ошибка открытия смены";
		}
	}

	auto message = PyUnicode_FromWideChar(st.c_str(), st.size());
	auto error = PyBool_FromLong(err_code);
	
	map<string, PyObject*> data = {
		{"message",  message},
		{"error",  error},
		{"error_code", PyLong_FromLongLong(err_code)},
		{"cashier", PyUnicode_FromString(cashier)},
	};

	addExError(&data);
	
	if (err_code == 0) {
		addKKTINFO(&data);
		addProgressiveTotalSales(&data);
		addShiftNumber(&data);
	}

	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_close_shift_pin_pad_doc, "close_shift_pin_pad(cashier)\
\
Close a cashier shift for pin-pad\
:param str cashier: - Cashier name\
:return dict()");
PyObject* cashbox_close_shift_pin_pad(PyObject* self, PyObject* args, PyObject* kwargs) {
	const char* cashier;
	static char* keywords[] = { "cashier", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &cashier)) {
		return NULL;
	}
	std::tuple<int, wstring> res = checkStatusPrinter();
	wstring st = L"Успешно";
	map<string, PyObject*> data = {
		{"cashier", PyUnicode_FromString(cashier)},
	};
	int err_code = std::get<0>(res);
	if (err_code) {
		st = std::get<1>(res);
	}
	else {
		err_code = libOpenDocument(4, 1, (char*)utf2oem((char*)cashier).c_str(), 0);
		wstring st;
		if (!err_code) {
			ArcusHandlers* arcus = new ArcusHandlers();
			err_code = arcus->closeShift();
			st = s2ws(cp2utf(arcus->getMessage()).c_str());
			data["message"] = PyUnicode_FromWideChar(st.c_str(), st.size());
			if (err_code < 10) {
				err_code = 0;
				printCheque();
				libCutDocument();
				libCancelDocument();
				libCutDocument();
			}
			delete arcus;
		}
		if (err_code > 0) {
			st = L"Ошибка закрытия пакета";
		}
	}

	auto message = PyUnicode_FromWideChar(st.c_str(), st.size());
	auto error = PyBool_FromLong(err_code);

	data["error"] = error;
	data["message"] = message;
	data["error_code"] = PyLong_FromLong(err_code);
	addExError(&data);
	return createPyDict(data);
}

PyDoc_STRVAR(cashbox_close_shift_doc, "close_shift(cashier)\
\
Close a cashier shift\
:param str cashier: - Cashier name\
:return dict()");
PyObject* cashbox_close_shift(PyObject* self, PyObject* args, PyObject* kwargs) {
	const char* cashier;
	static char* keywords[] = { "cashier", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &cashier)) {
		return NULL;
	}
	map<string, PyObject*> data = {
		{"cashier", PyUnicode_FromString(cashier)},
	};
	std::tuple<int, wstring> res = checkStatusPrinter();
	wstring st = L"Успешно";
	int err_code = std::get<0>(res);
	if (err_code) {
		st = std::get<1>(res);
	}
	else {
		err_code = libPrintZReport((char*)utf2oem((char*)cashier).c_str(), 1);
		addShiftNumber(&data, true);
		addZReport(&data);
		if (err_code > 0) {
			st = L"Ошибка закрытия смены";
		}
	}

	auto message = PyUnicode_FromWideChar(st.c_str(), st.size());
	auto error = PyBool_FromLong(err_code);

	data["error"] = error;
	data["message"] = message;
	data["error_code"] = PyLong_FromLong(err_code);
	addExError(&data);
	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_force_close_shift_doc, "force_close_shift()\
Force close a cashier shift\
:return dict()");
PyObject* cashbox_force_close_shift(PyObject* self) {
	map<string, PyObject*> data = {};
	std::tuple<int, wstring> res = checkStatusPrinter();
	wstring st = L"Успешно";
	int err_code = std::get<0>(res);
	if (err_code) {
		st = std::get<1>(res);
	}
	else {
		err_code = libEmergencyCloseShift();
		addShiftNumber(&data, true);
		addZReport(&data);
		if (err_code > 0) {
			st = L"Ошибка закрытия смены";
		}
	}
	data["message"] = PyUnicode_FromWideChar(st.c_str(), st.size());;
	data["error"] = PyBool_FromLong(err_code);
	data["error_code"] = PyLong_FromLong(err_code);
	addExError(&data);
	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_cancel_payment_by_link_doc, "cancel_payment_by_link(rrn, amount)\
Cancel payment by link \
:param str rrn: ссылка платежа\
:param int amount: сумма\
:return dict()");
PyObject* cashbox_cancel_payment_by_link(PyObject* self, PyObject* args, PyObject* kwargs) {
	char* rrn = "";
	int amount = 0;
	static char* keywords[] = { "amount", "rrn", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|s", keywords, &amount, &rrn)) {
		return NULL;
	}
	ArcusHandlers* arcus = new ArcusHandlers();
	int err_code = arcus->cancelByLink((char*)to_string(amount).c_str(), rrn);
	wstring message = s2ws(cp2utf((char*)arcus->getMessage()));
	if (err_code < 10) {
		err_code = 0;
	}
	map<string, PyObject*> data = {
		{"message", PyUnicode_FromWideChar(message.c_str(), message.size())},
		{"error",  PyBool_FromLong(err_code)},
		{"error_code", PyLong_FromLong(err_code)},
	};
	delete arcus;
	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_set_zero_cash_drawer_doc, "set_zero_cash_drawer(cashier)\
Reset cash drawer \
:return dict()");
PyObject* cashbox_set_zero_cash_drawer(PyObject* self, PyObject* args, PyObject* kwargs) {
	char* cashier;
	int err_code = 0;
	static char* keywords[] = { "cashier", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &cashier)) {
		return NULL;
	}
	std::tuple<int, wstring> res = checkStatusPrinter();
	wstring st = L"Успешно";
	err_code = std::get<0>(res);
	if (err_code) {
		st = std::get<1>(res);
	}
	else {
		string _cash_balance = requestDecorator(7, libGetKKTInfo);
		if (!_cash_balance.size()) {
			err_code = 1;
			st = L"Не удалось получить баланс в денежном ящике";
		}
		else {
			long double cash_balance = stold(_cash_balance);
			if (cash_balance == 0) {
				st = L"В Денежном ящике нет наличных";
				err_code = 2;
				goto SEND_DATA;
			}
			err_code = libOpenDocument(5, 1, (char*)utf2oem(cashier).c_str(), 0);
			if (err_code > 0) {
				st = L"Не удалось открыть документ";
				libCancelDocument();
			}
			else {
				err_code = libCashInOut("", ceill(cash_balance * 100));
				if (err_code > 0) {
					st = L"Ошибка внесения/изъятия";
					libCancelDocument();
				}
				else {
					MData ans = libCloseDocument(0);
					if (ans.errCode > 0) {
						libCancelDocument();
						err_code = ans.errCode;
						st = L"Не удалось закрыть документ";
					}
					else {
						libOpenCashDrawer(0);
					}
				}
			}
		}
	}
SEND_DATA:
	map<string, PyObject*> data = {
		{"message", PyUnicode_FromWideChar(st.c_str(), st.size())},
		{"error",  PyBool_FromLong(err_code)},
		{"error_code", PyLong_FromLong(err_code)},
	};
	addExError(&data);
	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_handler_cash_drawer_doc, "handler_cash_drawer(cashier, amount, doc_type)\
Handler cash drawer \
:return dict()");
PyObject* cashbox_handler_cash_drawer(PyObject* self, PyObject* args, PyObject* kwargs) {
	int err_code = 0;
	wstring st = L"Успешно";
	char* cashier;
	int amount = 0;
	int doc_type = 0;
	static char* keywords[] = { "cashier", "amount", "doc_type", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sii", keywords, &cashier, &amount, &doc_type)) {
		return NULL;
	}
	if (!(doc_type == 4 || doc_type == 5)) {
		PyErr_SetString(PyExc_ValueError, ws2s(L"Неверный тип документа (4 - внесение, 5 - изъятие)").c_str());
		return NULL;
	}
	std::tuple<int, wstring> res = checkStatusPrinter();
	err_code = std::get<0>(res);
	if (err_code) {
		st = std::get<1>(res);
	} 
	else {
		err_code = libOpenDocument(doc_type, 1, (char*)utf2oem(cashier).c_str(), 0);
		if (err_code > 0) {
			st = L"Не удалось открыть документ";
			libCancelDocument();
		}
		else {
			err_code = libCashInOut("", amount);
			if (err_code > 0) {
				st = L"Ошибка внесения/изъятия";
				libCancelDocument();
			}
			else {
				MData ans = libCloseDocument(0);
				if (ans.errCode > 0) {
					libCancelDocument();
					err_code = ans.errCode;
					st = L"Не удалось закрыть документ";
				}
				else {
					libOpenCashDrawer(0);
				}
			}
		}
	}
	map<string, PyObject*> data = {
		{"message", PyUnicode_FromWideChar(st.c_str(), st.size())},
		{"error",  PyBool_FromLong(err_code)},
		{"error_code", PyLong_FromLong(err_code)},
	};
	addKKTINFO(&data);
	addExError(&data);
	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_last_z_report_doc, "last_z_report(print_report)\
Get z-report for last closed shift \
:param bool print_report \
:return dict()");
PyObject* cashbox_last_z_report(PyObject* self, PyObject* args, PyObject* kwargs) {
	int print_report = 0;
	static char* keywords[] = { "print_report", NULL };
	PyArg_ParseTupleAndKeywords(args, kwargs, "p", keywords, &print_report);
	PyErr_Clear();
	map<string, PyObject*> data = {};
	addShiftNumber(&data, true);
	addKKTINFO(&data);
	addZReport(&data);
	if (print_report) {
		libPrintCopyLastZReport();
	}
	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_kkt_info_doc, "kkt_info()\
Get kkt info \
:return dict()");
PyObject* cashbox_kkt_info(PyObject* self) {
	map<string, PyObject*> data = {};
	addShiftNumber(&data);
	addKKTINFO(&data);
	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_new_transaction_doc, "new_transaction(cashier, payment_type, doc_type, wares, amount, rrn, order_prefix, print_strings)\
create new transaction\
:param str cashier: - Cashier name\
:return dict()");
PyObject* cashbox_new_transaction(PyObject* self, PyObject* args, PyObject* kwargs) {
	const char* cashier;
	const char* rrn = "";
	const char* print_strings = "";
	const char* order_prefix = "";
	int payment_type = 0;
	int doc_type = 0;
	long double amount = 0;
	PyListObject *wares;
	static char* keywords_all[] = { "cashier", "payment_type", "doc_type", "wares", "amount", "rrn", "order_prefix", "print_strings", NULL };

	bool parse_ok = PyArg_ParseTupleAndKeywords(
		args, 
		kwargs,
		"siiO|dsss",
		keywords_all,
		&cashier,
		&payment_type,
		&doc_type,
		&wares,
		&amount,
		&rrn,
		&order_prefix,
		&print_strings
	);
	if (!parse_ok) {
		PyErr_SetString(
			PyExc_ValueError,
			"Invalid args. allowed formats: 'cashier: str', 'payment_type: int', 'doc_type: int', 'wares: list', 'amount: float', 'rrn: str'"
		);
		return NULL;
	}

	if(!(doc_type == 2 || doc_type == 3)) {
		PyErr_SetString(PyExc_ValueError, ws2s(L"Неверный тип документа(2 - оплата, 3 - возварат)").c_str());
		return NULL;
	}

	if (!(payment_type == 0 || payment_type == 1)) {
		PyErr_SetString(PyExc_ValueError, ws2s(L"Неверный тип оплаты (1 - безналичный, 0 - наличный)").c_str());
		return NULL;
	}

	wstring message;
	ArcusHandlers* arcus = NULL;
	int err_code = 0;
	long sum = 0;
	long double _sum = 0.0;
	long discount_sum = 0;
	long double _discount_sum = 0;
	int num_depart = 1;
	int error_open_doc = 0;
	int payment_error = 0;
	bool is_open_doc = false;
	map<string, PyObject*> data = {};

	std::tuple<int, wstring> res = checkStatusPrinter();
	err_code = std::get<0>(res);
	if (std::get<0>(res)) {
		message = std::get<1>(res);
		data["message"] = PyUnicode_FromWideChar(message.c_str(), message.size());
		data["error"] = PyBool_FromLong(err_code);
		data["error_code"] = PyLong_FromLong(err_code);
		return createPyDict(data);
	}

	for (int i = 0; i < wares->allocated; i++) {
		PyObject *ware = wares->ob_item[i];
		double price = PyFloat_AsDouble(PyDict_GetItemString(ware, "price"));
		PyObject* discount_obj = PyDict_GetItemString(ware, "discount");
		double discount = 0.0;
		if (discount_obj != NULL) {
			discount = PyFloat_AsDouble(discount_obj);
		}
		double quantity = PyFloat_AsDouble(PyDict_GetItemString(ware, "quantity"));
		_sum += price * quantity;
		if (discount > 0) {
			_discount_sum += discount;
		}
	}
	sum = (long)round(_sum * 100);
	if (_discount_sum > 0) {
		discount_sum = (long)round(_discount_sum * 100);
	}
	data["transaction_sum"] = PyFloat_FromDouble(_sum);

	if (payment_type == 1) {
		// Безнал
		amount = sum - discount_sum;
		arcus = new ArcusHandlers();
		if (doc_type == 2) {
			payment_error = arcus->purchase((char*)to_fixed(amount, 0).c_str());
			data["rrn"] = PyUnicode_FromString(arcus->getRRN());
			data["pan_card"] = PyUnicode_FromString(arcus->getPANCard());
			string cardholder_name = trim(string(arcus->getCardHolderName()));
			data["cardholder_name"] = PyUnicode_FromString(cardholder_name.c_str());
		}
		else if (doc_type == 3) {
			payment_error = arcus->cancelByLink((char*)to_fixed(amount, 0).c_str(), (char*)rrn);
		}
		if (payment_error) {
			message = s2ws(cp2utf((char*)arcus->getMessage()));
			goto NEXT;
		}
	}
	else if (payment_type == 0) {
		// Наличка
		if (doc_type == 2) {
			if (amount == 0) {
				PyErr_SetString(PyExc_ValueError, ws2s(L"При наличном платеже требуется параметр 'amount'").c_str());
				return NULL;
			}
			data["amount"] = PyFloat_FromDouble(amount);
			amount = round(amount * 100);
		}
		else {
			amount = sum - discount_sum;
		}
		data["delivery"] = PyFloat_FromDouble((long int)ceill(amount - _sum - discount_sum / 100));
		libOpenCashDrawer(0); // Открыть денежный ящик
	}

	// Открыть документ
	error_open_doc = libOpenDocument(
		doc_type,
		num_depart,
		(char*)utf2oem((char*)cashier).c_str(),
		0
	);
	
	if (error_open_doc == 0) {
		if (payment_type == 1 && payment_error == 0) {
			// Если безнал, то печатаем чек оплаты
			printCheque();
			libCutDocument();
		}
		is_open_doc = true;
		if (strlen(order_prefix) > 0) {
			printOrderNumber(order_prefix);
		}
		if (strlen(print_strings) > 0) {
			printStringInOpenDoc(print_strings);
		}
		for (int i = 0; i < wares->allocated; i++) {
			PyObject* ware = wares->ob_item[i];
			const char* name = PyUnicode_AsUTF8(PyDict_GetItemString(ware, "name"));
			const char* barcode = PyUnicode_AsUTF8(PyDict_GetItemString(ware, "barcode"));
			double price = PyFloat_AsDouble(PyDict_GetItemString(ware, "price"));
			PyObject* discount_obj = PyDict_GetItemString(ware, "discount");
			double discount = 0.0;
			if (discount_obj != NULL) {
				discount = PyFloat_AsDouble(discount_obj);
			}
			double quantity = PyFloat_AsDouble(PyDict_GetItemString(ware, "quantity"));
			int tax_number = PyLong_AsLong(PyDict_GetItemString(ware, "tax_number"));
			double discount_percent;
			err_code = libAddPosition(
				utf2oem((char*)name).c_str(),
				utf2oem((char*)barcode).c_str(),
				quantity,
				price,
				tax_number,
				0,
				num_depart,
				0, 0, 0
			);
			if (err_code) {
				if (message.size() > 0) {
					message = message + L", ";
				}
				message += L"Ошибка добавления товара " + s2ws(name);
			}
			else if (discount > 0) {
				discount_percent = (discount / price) * 100;
				libPrintRequsit(
					0, 3,
					(char*)utf2oem((char*)ws2s(
						L"СКИДКА " + s2ws((char*)to_fixed(discount_percent).c_str()) + L"% =" + s2ws((char*)to_fixed(discount).c_str())
					).c_str()).c_str(),
					"","",""
				);
			}
		}
		// Подъитог
		if (discount_sum > 0) {
			libSubTotal();
			err_code = libAddDiscount(1, "", discount_sum);
			if (err_code > 0) {
				if (message.size() > 0) {
					message = message + L", ";
				}
				message += L"Ошибка добавления скидки";
				goto NEXT;
			}
		}
		err_code = libSubTotal();
		if (err_code > 0) {
			if (message.size() > 0) {
				message = message + L", ";
			}
			message += L"Ошибка подъитога";
		}
		else {
			// добавление типа оплаты
			err_code = libAddPayment(payment_type, amount, "");
			if (err_code > 0) {
				if (message.size() > 0) {
					message = message + L", ";
				}
				message += L"Добавления типа оплаты";
			}
		}
	}
	else {
		if (message.size() > 0) {
			message = message + L", ";
		}
		message += L"Ошибка открытия документа";
		libCancelDocument();
	}
NEXT:
	if (error_open_doc || (err_code > 0 && is_open_doc)) {
		libCancelDocument();
	}
	if (payment_error == 0 && payment_type == 1 && (error_open_doc > 0 || err_code > 0)) {
		payment_error = arcus->cancelLast();
		if (payment_error) {
			if (message.size() > 0) {
				message = message + L", ";
				payment_error = 0;
			}
			message += s2ws(cp2utf(arcus->getMessage()));
			if (error_open_doc == 0) {
				libCancelDocument();
			}
		}
	}
	err_code = err_code + error_open_doc + payment_error;

	if (err_code == 0) {
		MData ans = libCloseDocument(0);
		if (ans.errCode) {
			message = L"Ошибка закрытия документа";
			err_code = ans.errCode;
			libCancelDocument();
			if (payment_error == 0 && payment_type == 2) {
				payment_error = arcus->getResponseCode();
				if (payment_error > 0) {
					err_code = payment_error;
					message = message + L", " + s2ws(cp2utf(arcus->getMessage()));
				}
			}
		}
	}

	addExError(&data);
	addDateTime(&data);
	data["message"] = PyUnicode_FromWideChar(message.c_str(), message.size());
	data["error"] = PyBool_FromLong(err_code);
	data["error_code"] = PyLong_FromLong(err_code);
	data["cashier"] = PyUnicode_FromString(cashier);
	if (err_code == 0) {
		addLastChequeInfo(&data);
	}
	delete arcus;
	return createPyDict(data);
};

/*
 * List of functions to add to cashbox in exec_cashbox().
 */
static PyMethodDef cashbox_functions[] = {
	{ "open_port", (PyCFunction)cashbox_open_port, METH_VARARGS | METH_KEYWORDS, cashbox_open_port_doc },
	{ "close_port", (PyCFunction)cashbox_close_port, METH_NOARGS, cashbox_close_port_doc },
	{ "open_shift", (PyCFunction)cashbox_open_shift, METH_VARARGS | METH_KEYWORDS, cashbox_open_shift_doc },
	{ "new_transaction", (PyCFunction)cashbox_new_transaction, METH_VARARGS | METH_KEYWORDS, cashbox_new_transaction_doc },
	{ "close_shift", (PyCFunction)cashbox_close_shift, METH_VARARGS | METH_KEYWORDS, cashbox_close_shift_doc },
	{ "force_close_shift", (PyCFunction)cashbox_force_close_shift, METH_NOARGS, cashbox_force_close_shift_doc },
	{ "set_zero_cash_drawer", (PyCFunction)cashbox_set_zero_cash_drawer, METH_VARARGS | METH_KEYWORDS, cashbox_set_zero_cash_drawer_doc },
	{ "last_z_report", (PyCFunction)cashbox_last_z_report, METH_VARARGS | METH_KEYWORDS, cashbox_last_z_report_doc },
	{ "cancel_payment_by_link", (PyCFunction)cashbox_cancel_payment_by_link, METH_VARARGS | METH_KEYWORDS, cashbox_cancel_payment_by_link_doc },
	{ "kkt_info", (PyCFunction)cashbox_kkt_info, METH_NOARGS, cashbox_kkt_info_doc },
	{ "handler_cash_drawer", (PyCFunction)cashbox_handler_cash_drawer, METH_VARARGS | METH_KEYWORDS, cashbox_handler_cash_drawer_doc },
	{ "close_shift_pin_pad", (PyCFunction)cashbox_close_shift_pin_pad, METH_VARARGS | METH_KEYWORDS, cashbox_close_shift_pin_pad_doc },
	{ "set_datetime", (PyCFunction)cashbox_set_datetime, METH_VARARGS | METH_KEYWORDS, cashbox_set_datetime_doc },
    { NULL, NULL, 0, NULL } /* marks end of array */
};

/*
 * Initialize cashbox. May be called multiple times, so avoid
 * using static state.
 */
int exec_cashbox(PyObject *module) {
    PyModule_AddFunctions(module, cashbox_functions);
    PyModule_AddStringConstant(module, "__author__", "alex-proc");
    PyModule_AddStringConstant(module, "__version__", "1.0.7");
    PyModule_AddIntConstant(module, "year", 2019);
    return 0; /* success */
}

/*
 * Documentation for cashbox.
 */
PyDoc_STRVAR(cashbox_doc, "The cashbox module");


static PyModuleDef_Slot cashbox_slots[] = {
    { Py_mod_exec, exec_cashbox },
    { 0, NULL }
};

static PyModuleDef cashbox_def = {
    PyModuleDef_HEAD_INIT,
    "cashbox",
    cashbox_doc,
    0,              /* m_size */
    NULL,           /* m_methods */
    cashbox_slots,
    NULL,           /* m_traverse */
    NULL,           /* m_clear */
    NULL,           /* m_free */
};

PyMODINIT_FUNC PyInit_cashbox() {
    return PyModuleDef_Init(&cashbox_def);
}
