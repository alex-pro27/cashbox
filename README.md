# Cashbox
### Python api для управления ККТ:
#### Пин-пад Ingenico IPP 320/350 (Arcus)
#### Фискальный регистратор Pirit2f

Системные требования Windows >= 7, Python(x32) >= 3.7.4

### Установка:

```
pip install git+https://github.com/alex-pro27/cashbox
```

**Arcus:**
<a target="_blank" href="http://ftp.f-trade.ru/Payment_terminals/Ingenico_ICT220-250/Integration/arcus2-setup/">-> Документация и драйверы для Arcus</a>

**Pirit2F**
<a target="_blank" href="https://www.crystals.ru/support/download/kkt-pirit-2f">->Документация и драйверы для Пирит 2Ф</a>

### Методы:
Открыть порт:
```python
open_port("COM9", 57600)
```
Закрыть порт:
```python
close_port()
```
Открыть смену:
```python
open_shift("Имя кассира")
```

Закрыть смену на пин-паде:
```python
close_shift_pin_pad("Имя кассира")
```

Закрыть смену:
```python
close_shift("Имя кассира")
```
Аварийное закрытие смены:
```python
force_close_shift()
```
Информация о ККТ:
```python
kkt_info()
```
Z-Отчет по последней закрытой смене:
```python
:param bool print_repor: - Печатать или нет
last_z_report(print_report)
```
Обнулить денежный ящик:
```python
set_zero_cash_drawer(cashier)
:param str cashier: - Кассир
```
Новая транзакция:
```python
"""
:param str cashier: - Имя кассира
:param int payment_type: - Тип оплаты 1 - безнал, 0 - Наличка
:param int doc_type: - Тип документа 2 - Оплата, 3 - Возврат
:param list wares: - Список товаров [
	{
		"name":"Вино шато",  Наименование
		"barcode":"123124", Штрих-код
		"quantity":1, Количество
		"price":56.7, - Цена
		"tax_number":0, - Налог 1 - 10%, 0 - 20%
		"discount": 0.7 - Скидка
	},
],
:param float or int amount: - Необязательный аргумент, Количество денег отданых кассиру передается в случае если оплата по наличному расчету
:param str rrn: - Необязательный аргумент, ID Платежа, Передается в случае возврата, по безналичному платежу
:param str order_prefix: - Префикс для заказа, если передан, то в распечатается номер документа + префикск(order_prefix)
:param str print_strings: - Необязательный аргумент, печать строк в документе -
- для перехода на новую строку добавьте символ переноса строки - \n
- пример применения стилей для текста (по умолчанию стиль NORMAL )(
	(font-style=NORMAL)Нормальный
	\n(font-style=NORMAL_BOLD)Нормальный жирный
	\n(font-style=MEDIUM)Средний
	\n(font-style=BIG)Большой
	\n(font-style=BIG_BOLD)Большой жирный
	\n(font-style=SMALL)Маленький
)
"""
new_transaction(cashier, payment_type, doc_type, wares, amount, rrn, print_strings)
```
Отменить безналичный платеж по ID
```python
"""
:param int amount: Сумма в копейках
:param str rrn: ID Платежа (Ссылка)
"""
cancel_payment_by_link(amount, rrn)
```
Внесение/Изъятие из денежного ящика
```python
"""
:param str cashier: Кассир
:param int amount: Количество вносимых/изъимаемых денег (в копейках)
:param int doc_type: Тип документа 4 - внесение, 5 - изъятие
"""
handler_cash_drawer(cashier, amount, doc_type)
```

### Описание возвращаемых значений:
```
cashier str - Кассир,
cash_balance float - Количество денег в ящике
fn_number str - Заводской номер фискального регистратора
inn str -  Инн
shift_number int - Номер смены
progressive_total_sales float - Нарастающий итог продажи
progressive_total_returns float - Нарастающий итог возвратов
discount_sum_sales float - Сумма скидок по продажам
marckup_sum_sale float - Сумма наценок по продажам
discount_sum_return float -  Сумма скидок по возвратам
marckup_sum_returns  float - Сумма наценок по возвратам
doc_numbe  int -  Номер документа
count_sales int - Количество продаж
sum_sales float - Сумма продаж
count_insert int Количество внесений
count_remove int - Количество изъятий
count_returns int - Количество возвартов
sum_canceled float - Сумма аннулированных
sum_insert float - Сумма внесений
sum_remove float - Сумма изъятий
sum_returns float - Сумма возвратов
rrn str - ссылка безналичного платежа(ID платежа) 
check_number int - Номер чека
discount_sum float - Скидка
doc_number int - Номер документа
pan_card str - Пан карты
cardholder_name str - Имя держателя карты
transaction_sum float - Сумма транзакции
delivery float - Сдача,
amount float - Количество вносимых / изъятых денег
datetime string - текущая дата и время фискального регистратора
```
