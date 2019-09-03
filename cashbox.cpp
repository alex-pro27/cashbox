﻿#include <Python.h>
#include <iostream>
#include <fstream>
#include <sstream>
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
};

/* Информация о последнем чеке */
void addLastChequeInfo(map<string, PyObject*> *data) {
	string kktinfo = requestDecorator(2, libGetReceiptData);
	if (kktinfo != "") {
		map<char*, int> keys = {
			{"check_number", 1},			// Номер чека
			{"doc_number", 3},				// Номер документа
			{"discount_sum", 5},			// Сумма скидки по чеку
			{"discount_marckup_sum", 6},	// Сумма скидки наценки по чеку
			{"fp", 7},						// Фискальный признак
			{"fd", 8},						// фискальный документ
		};
		setParsedData(keys, kktinfo, data);
	}
};

/* Нарастающий итог */
void addProgressiveTotalSales(map<string, PyObject*> *data) {
	/* Нарастающий итог */
	string kktinfo = requestDecorator(12, libGetKKTInfo);
	if (kktinfo != "") {
		map<char*, int> keys = {
			{"progressive_total_sales", 0},		// Нарастающий итог продажи
			{"progressive_total_returns", 1},	// Нарастающий итог возврата
		};
		setParsedData(keys, kktinfo, data);
	}
};

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
	commandStart();
	const char* cashier;
	static char* keywords[] = { "cashier", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &cashier)) {
		return NULL;
	}

	int err_code = libOpenShift((char*)utf2oem((char*)cashier).c_str());

	wstring st = L"Успешно";
	if (err_code > 0)
		st = L"Ошибка открытия смены";

	auto message = PyUnicode_FromWideChar(st.c_str(), st.size());
	auto error = PyBool_FromLong(err_code);
	
	map<string, PyObject*> data = {
		{"message",  message},
		{"error",  error},
		{"error_code", PyLong_FromLongLong(err_code)},
		{"cashier", PyUnicode_FromString(cashier)},
	};
	
	if (err_code == 0) {
		addKKTINFO(&data);
		addProgressiveTotalSales(&data);
		addShiftNumber(&data);
	}

	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_close_shift_doc, "close_shift(cashier)\
\
Close a cashier shift\
:param str cashier: - Cashier name\
:return dict()");
PyObject* cashbox_close_shift(PyObject* self, PyObject* args, PyObject* kwargs) {
	commandStart();
	const char* cashier;
	static char* keywords[] = { "cashier", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &cashier)) {
		return NULL;
	}
	map<string, PyObject*> data = {
		{"cashier", PyUnicode_FromString(cashier)},
	};

	int err_code = libPrintZReport((char*)utf2oem((char*)cashier).c_str(), 1);
	addShiftNumber(&data, true);
	addZReport(&data);

	wstring st = L"Успешно";
	if (err_code > 0)
		st = L"Ошибка закрытия смены";

	auto message = PyUnicode_FromWideChar(st.c_str(), st.size());
	auto error = PyBool_FromLong(err_code);

	data["error"] = error;
	data["message"] = message;
	data["error_code"] = PyLong_FromLong(err_code);

	return createPyDict(data);
};

PyDoc_STRVAR(cashbox_force_close_shift_doc, "force_close_shift()\
Force close a cashier shift\
:return dict()");
PyObject* cashbox_force_close_shift(PyObject* self) {
	commandStart();
	map<string, PyObject*> data = {};
	int err_code = libEmergencyCloseShift();
	addShiftNumber(&data, true);
	addZReport(&data);
	wstring st = L"Успешно";
	if (err_code > 0)
		st = L"Ошибка закрытия смены";

	auto message = PyUnicode_FromWideChar(st.c_str(), st.size());
	auto error = PyBool_FromLong(err_code);
	data["message"] = message;
	data["error"] = error;
	data["error_code"] = PyLong_FromLong(err_code);
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
	static char* keywords[] = { "rrn", "amount", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si", keywords, &rrn, &amount)) {
		return NULL;
	}
	ArcusHandlers* arcus = new ArcusHandlers();
	arcus->auth();
	arcus->cancelByLink(rrn, (char*)to_string(amount).c_str());
	wstring message = s2ws(cp2utf((char*)arcus->auth_data.text_message));
	int err_code = atoi(arcus->auth_data.responseCode);
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
	wstring message = L"Успешно";
	static char* keywords[] = { "cashier", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &cashier)) {
		return NULL;
	}
	string _cash_balance = requestDecorator(7, libGetKKTInfo);
	if (_cash_balance != "") {
		long double cash_balance = stold(_cash_balance);
		if (cash_balance == 0) {
			PyErr_SetString(PyExc_ValueError, ws2s(L"В денежном ящике нет наличных").c_str());
			return NULL;
		}
		err_code = libOpenDocument(5, 1, (char*)utf2oem(cashier).c_str(), 0);
		if (err_code > 0) {
			message = L"Не удалось открыть документ";
			libCancelDocument();
		}
		else {
			err_code = libCashInOut("", ceill(cash_balance * 100));
			if (err_code > 0) {
				message = L"Ошибка внесения/изъятия";
				libCancelDocument();
			}
			else {
				MData ans = libCloseDocument(0);
				if (ans.errCode > 0) {
					libCancelDocument();
					err_code = ans.errCode;
					message = L"Не удалось закрыть документ";
				}
				else {
					libOpenCashDrawer(0);
				}
			}
			
		}
		map<string, PyObject*> data = {
			{"message", PyUnicode_FromWideChar(message.c_str(), message.size())},
			{"error",  PyBool_FromLong(err_code)},
			{"error_code", PyLong_FromLong(err_code)},
		};
		return createPyDict(data);
	}
	PyErr_SetString(PyExc_ValueError, ws2s(L"Не удалось получить баланс в денежном ящике").c_str());
	return NULL;
};

PyDoc_STRVAR(cashbox_handler_cash_drawer_doc, "handler_cash_drawer(cashier, amount, doc_type)\
Handler cash drawer \
:return dict()");
PyObject* cashbox_handler_cash_drawer(PyObject* self, PyObject* args, PyObject* kwargs) {
	int err_code = 0;
	wstring message = L"Успешно";
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
	
	err_code = libOpenDocument(doc_type, 1, (char*)utf2oem(cashier).c_str(), 0);
	if (err_code > 0) {
		message = L"Не удалось открыть документ";
		libCancelDocument();
	}
	else {
		err_code = libCashInOut("", amount);
		if (err_code > 0) {
			message = L"Ошибка внесения/изъятия";
			libCancelDocument();
		}
		else {
			MData ans = libCloseDocument(0);
			if (ans.errCode > 0) {
				libCancelDocument();
				err_code = ans.errCode;
				message = L"Не удалось закрыть документ";
			} 
			else {
				libOpenCashDrawer(0);
			}
		}
	}
	
	map<string, PyObject*> data = {
		{"message", PyUnicode_FromWideChar(message.c_str(), message.size())},
		{"error",  PyBool_FromLong(err_code)},
		{"error_code", PyLong_FromLong(err_code)},
	};
	addKKTINFO(&data);
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

PyDoc_STRVAR(cashbox_new_transaction_doc, "new_transaction(cashier, payment_type, doc_type, wares, amount, rrn)\
create new transaction\
:param str cashier: - Cashier name\
:return dict()");
PyObject* cashbox_new_transaction(PyObject* self, PyObject* args, PyObject* kwargs) {
	const char* cashier;
	const char* rrn;
	int payment_type = 0;
	int doc_type = 0;
	long double amount = 0;
	PyListObject *wares;
	static char* keywords_all[] = { "cashier", "payment_type", "doc_type", "wares", "amount", "rrn", NULL };

	bool parse_ok = PyArg_ParseTupleAndKeywords(args, kwargs, "siiO|ds", keywords_all, &cashier, &payment_type, &doc_type, &wares, &amount, &rrn);
	if (!parse_ok) {
		PyErr_SetString(PyExc_ValueError, "Invalid args. allowed formats: 'cashier: str', 'payment_type: int', 'doc_type: int', 'wares: list', 'amount: float', 'rrn: str',");
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
			arcus->auth();
			arcus->purchase((char*)to_fixed(amount, 0).c_str());
			payment_error = atoi(arcus->auth_data.responseCode);
			data["rrn"] = PyUnicode_FromString(arcus->auth_data.rrn);
			data["pan_card"] = PyUnicode_FromString(arcus->auth_data.pan);
			string cardholder_name = trim(string(arcus->auth_data.cardholder_name));
			data["cardholder_name"] = PyUnicode_FromString(cardholder_name.c_str());
		}
		else if (doc_type == 3) {
			if (rrn == "") {
				PyErr_SetString(PyExc_ValueError, ws2s(L"Отсутсвует параметр rrn(ссылка платежа)").c_str());
				free(arcus);
				return NULL;
			}
			else {
				arcus->cancelByLink((char*)rrn, (char*)to_fixed(amount, 0).c_str());
				payment_error = atoi(arcus->auth_data.responseCode);
			}
		}
		if (payment_error > 0) {
			message = s2ws(cp2utf((char*)arcus->auth_data.text_message));
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
		data["delivery"] = PyFloat_FromDouble(ceill(amount - _sum - discount_sum / 100));
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
			// Если безнали, то печатаем чек оплаты
			printCheque();
			libCutDocument();
		}
		is_open_doc = true;
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
					0, 1,
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
	if ((payment_error > 0  && error_open_doc == 0) || (err_code > 0 && is_open_doc)) {
		libCancelDocument();
	}
	if (payment_error == 0 && payment_type == 1 && error_open_doc > 0) {
		arcus->cancelByLink(arcus->auth_data.rrn, (char*)to_string(amount).c_str());
		int pyment_error = atoi(arcus->auth_data.responseCode);
		if (pyment_error > 0) {
			if (message.size() > 0) {
				message = message + L", ";
			}
			message += s2ws(cp2utf((char*)arcus->auth_data.text_message));
			if (error_open_doc == 0) {
				libCancelDocument();
			}
		}
	}
	err_code = err_code + error_open_doc + payment_error;

	if (err_code == 0) {
		MData ans = libCloseDocument(0);
		if (ans.errCode > 0) {
			message = L"Ошибка закрытия документа";
			err_code = ans.errCode;
			libCancelDocument();
			if (payment_error == 0 && payment_type == 2) {
				payment_error = atoi(arcus->auth_data.responseCode);
				if (payment_error > 0) {
					err_code = payment_error;
					message = message + L", " + s2ws(cp2utf((char*)arcus->auth_data.text_message));
				}
			}
		}
	}

	data["message"] = PyUnicode_FromWideChar(message.c_str(), message.size());
	data["error"] = PyBool_FromLong(err_code);
	data["error_code"] = PyLong_FromLong(err_code);
	data["cashier"] = PyUnicode_FromString(cashier);

	if (err_code == 0) {
		addLastChequeInfo(&data);
	}
	free(arcus);
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
    { NULL, NULL, 0, NULL } /* marks end of array */
};

/*
 * Initialize cashbox. May be called multiple times, so avoid
 * using static state.
 */
int exec_cashbox(PyObject *module) {
    PyModule_AddFunctions(module, cashbox_functions);
    PyModule_AddStringConstant(module, "__author__", "alex-proc");
    PyModule_AddStringConstant(module, "__version__", "1.0.3");
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
