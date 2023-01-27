# Модуль для работы с Sysrepo в Klish3


## Обзор

Модуль klish-plugin-sysrepo предназначен для работы с хранилищем конфигурации
[Sysrepo](#https://www.sysrepo.org/) из программы
[Klish версии 3](#https://klish.libcode.org). Klish позволяет организовать
интерфейс командной строки, содержащий только команды, заданные пользователем при
конфигурировании klish. Хранилище Sysrepo использует язык моделирования данных
[YANG](#https://datatracker.ietf.org/doc/html/rfc6020) для формирования схемы
хранимой конфигурации, а также хранит саму конфигурацию. Обычно хранимая
конфигурация относится к какой-либо встроенной системе, например к
маршрутизатору. Принцип хранения и управления конфигурацией, используемый в
klish-plugin-sysrepo, похож на подход, применяемый в маршрутизаторах "Juniper".

Схема конфигурация описана YANG файлами, а доступ и редактирование элементов
конфигурации осуществляется с помощью специальных команд, доступных из klish.
Всю информацию об используемой YANG схеме klish-plugin-sysrepo получает из
Sysrepo автоматически.

Проект содержит файл `xml/sysrepo.xml`, который, будучи добавленным к файлам
конфигурации klish, объявляет новый `VIEW` с именем "sysrepo". Этот `VIEW`
содержит готовые команды для редактирования конфигурации Sysrepo. Вся работа
модуля с Sysrepo осуществляется с помощью библиотечных функций Sysrepo и
библиотеки [`libyang`](#https://github.com/CESNET/libyang). Проекты Sysrepo,
libyang и klish требуются для сборки и работы модуля klish-plugin-sysrepo.


## Хранилище конфигурации

Система Sysrepo поддерживает четыре хранилища конфигурации.

* `running` - текущая действующая конфигурация. Состояние системы соответствует
этой конфигурации. Но эта не та конфигурация, которую редактирует администратор.
* `candidate` - редактируемая конфигурация. "Кандидат" на то, чтобы стать
действующей конфигурацией. Однако изменения в `candidate` никак не влияют на
реальное состояние системы. Только по факту того, что администратор, специальной
командой, записывает содержимое `candidate` в `running` будут происходить
изменения в системе. Т.е. изменения применяются не по одному, а "все разом".
Таким образом поддерживается целостность и непротиворечивость действующей
конфигурации. Все описанные ниже команды работают с `candidate` конфигурацией,
если не указано иное.
* `startup` - конфигурация, сохраненная на диске. Только эта конфигурация
может пережить перезагрузку системы. В случае модуля klish-plugin-sysrepo
хранилища `startup` и `running` всегда идут в паре. Т.е. модуль поддерживает их
состояние одинаковым.
* `operational` - это хранилище не имеет прямого отношение к обсуждаемой теме и
упоминаться тут больше не будет.


## Путь KPath

Многие команды для работы с конфигурацией используют "путь". Это путь в YANG
схеме или вернее в заполненной значениями конфигурации, построенной по YANG
схеме. YANG схема является ветвистой и многоуровневой. Схема может содержать
"списки", в которых элементы идентифицируются по "ключу". Элементами пути могут
быть как узлы YANG схемы, так и ключи, потому что конфигурация ветвится в
зависимости от этих ключей. Проекты Sysrepo и libyang используют
["XPath"](#https://www.w3schools.com/xml/xpath_intro.asp) для доступа к
элементам конфигурации. Несмотря на то, что XPath имеет развитый синтаксис и
возможности, он не подходит для использования в интерфейсе командной строки.
XPath довольно сложен и не будет интуитивно понятен оператору. Поэтому для
доступа к элементам конфигурации в klish-plugin-sysrepo используется упрощенный
вариант пути. Для определенности, назовем его "KPath" (Klish path), чтобы потом
ссылаться на его формат и не путать с другими разновидностями путей.

Элементы пути в KPath отделяются друг от друга пробелами. Элементами пути
являются узлы YANG схемы, значения ключей и элементы `leaf-list` (см. YANG).
Если значение ключа или элемент `leaf-list` содержит пробелы, то они заключаются
в кавычки.

Предположим есть такой фрагмент YANG схемы:

```
module ttt {
  namespace "urn:ttt";
  prefix ttt;

  container test {
    list iface {
      key "name";

      leaf name {
        type string;
      }

      leaf comment {
        type string;
      }

      leaf type {
        type enumeration {
          enum ethernet;
          enum ppp;
          enum dummy;
        }
      }

    }
  }
}

```

Корректный KPath до элемента, определяющего тип "интерфейса" будет таким:
`test iface eth0 type`, где "eth0" - ключ списка "iface", соответствующий
элементу "name".

KPath до поля комментария для интерфейса "eth1" будет таким:
`test iface eth1 comment`.

В командах, где требуется ввести путь до элемента конфигурации, KPath будет
таким, как описано выше. Для команды `set`, которая устанавливает значение
элемента конфигурации, существует расширение формата KPath, так называемые
"однострочники". Однострочники позволяют задать значения нескольким элементам
конфигурации (`leaf`) одной строкой. Команда `set` вместе с аргументами может
выглядеть так:

```
# set test iface eth0 type ethernet comment "Comment with space"
```

В одной строке задается значение типа интерфейса и комментарий. Тут, собственно,
путь прерывается значениями полей.


## Команды конфигурирования


### Команда `set`

Команда `set` позволяет установить значение элемента конфигурации.

```
# set <KPath> <value>
```

Чтобы выбрать какому именно элементу следует присвоить значение, указывается
[путь KPath](#путь-kpath) до этого элемента а затем само значение. Если значение
содержит пробелы, то должно заключаться в кавычки.

Здесь элементу, имеющему KPath `test iface eth0 type` присваивается значение
`ethernet`:

```
# set test iface eth0 type ethernet
```

Команда `set` может присвоить значения сразу нескольким элементам конфигурации.
Читайте про "однострочники" в разделе ["Путь KPath"](#путь-kpath). Пример
однострочника:

```
# set test iface eth0 type ethernet comment "Comment with space"
```

При указании пути KPath команде `set` не обязательно, чтобы все компоненты пути
уже существовали. Команды из приведенных примеров будут работать даже если
интерфейс "eth0" еще не был создан в конфигурации. Недостающие компоненты пути
будут созданы автоматически.


### Команда `del`

Команда `del` удаляет значение элемента, либо целую ветку конфигурации.

```
# del <KPath>
```

Например можно удалить комментарий для интрерфейса:

```
# del test iface eth0 comment
```

А можно удалить все настройки для интерфейса "eth0":

```
# del test iface eth0
```

### Команда `edit`
### Команда `top`
### Команда `up`
### Команда `exit`
### Команда `insert`
### Команда `commit`
### Команда `check`
### Команда `rollback`
### Команда `show`
### Команда `diff`
### Команда `do`


## Настройки модуля

При подключении модуля к системе klish элементом `PLUGIN`, в теле элемента
`PLUGIN`  можно задать настройки для модуля. Настройки регулируют некоторые
особенности поведения модуля.


### Настройка `JuniperLikeShow`

Поле может принимать значения `y` и `n`. В случае, если задано `y`, то команды,
показывающие конфигурацию, отображают ее в виде похожем на то, как конфиг
выглядит в системах Juniper. С фигурными скобками, выделяющими секции и символом
`;` в конце строк с "листьями" (узлы `leaf`).

```
test {
    iface eth0 {
        comment "Test desc";
        type ethernet;
    }
}
```

Если задано значение `n`, то конфигурация будет отображаться в более кратком и
простом виде. Структура секций помечается отступами.

```
test
    iface eth0
        comment "Test desc"
        type ethernet
```


### Настройка `FirstKeyWithStatement`

У списков (узел `list`) может быть не один ключ, для идентификации нужного
элемента, а сразу несколько. Данное поле настройки относится только к первому
ключу списка. Поле может принимать значения `y` или `n`. Если задано значение
`y`, то при указании KPath или отображении конфигурации, перед значением ключа,
будет указываться имя элемента.

Например оператор устанавливает значение элемента `type` для интерфейса:

```
# set test iface name eth0 type ethernet
```

В KPath появился элемент `name`, который показывает название (а не только
значение) ключевого элемента. Та же самая конструкция, если
`FirstKeyWithStatement = n` будет выглядеть так:

```
# set test iface eth0 type ethernet
```


### Настройка `MultiKeysWithStatement`

Настройка аналогична полю `FirstKeyWithStatement`, только относится ко всем
ключевым элементам списка кроме первого.

Предположим, что ключами списка "интерфейсы" являются сразу и поле `name` и поле
`type`. Еще предположим что `FirstKeyWithStatement = n`. Команда установки
"комментария" для интерфейса будет выглядеть следующим образом, если
`MultiKeysWithStatement = y`:

```
# set test iface eth0 type ethernet comment "Comment"
```

Если `MultiKeysWithStatement = n` то следующим образом:

```
# set test iface eth0 ethernet comment "Comment"
```


### Настройка `Colorize`

Поле может принимать значения `y` и `n`. В случае, если задано `y`, то при
отображении конфигурации или изменений в конфигурации, характерные элементы
будут выделяться цветом.

> Сейчас цветовое выделение реализовано только в команде `diff`. Новые элементы
> выделяются зеленым, удаленные - красным, а те, что изменили свое значение -
> желтым.

Некоторые терминалы, в основном устаревшие, могут не поддерживать цвета. Поэтому
в каких то случаях может быть полезно отключить настройку `Colorize`.


### Пример настройки модуля

```
<PLUGIN name="sysrepo">
	JuniperLikeShow = y
	FirstKeyWithStatement = n
	MultiKeysWithStatement = y
	Colorize = y
</PLUGIN>
```