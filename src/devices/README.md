# Devices
Каталог контроллеров и датчиков. Настройки и код.

<!-- [[[cog
import cog
cog.out(open("./relay/docs/README.md").read())
]]] -->
### Модуль управления реле

#### Схема цепи

[Проект Wokwi](https://wokwi.com/projects/456035055362315265)

![Схема цепи](/src/devices/relay/docs/circuit.png)

#### Взаимодействие с сервером

**Формат запросов:**

HTTP POST 

URL: /api/v1/heartbeat/{device-name}

Пример тела запроса:
```
{
    "token": "abacaba",
    "values": [
        "state": 1.0
    ]
}
```

**Формат ответов:**

HTTP 200 

Пример тела ответа:
```
{
	"trigger_active": false,
	"state": 0.0,
}
```
<!-- [[[end]]] -->

<!-- [[[cog
import cog
cog.out(open("./dht22/docs/README.md").read())
]]] -->
### Модуль температуры и влажности

#### Схема цепи

[Проект Wokwi](https://wokwi.com/projects/456108578902750209)

![Схема цепи](/src/devices/dht22/docs/circuit.png)

#### Взаимодействие с сервером

**Формат запросов:**

HTTP POST 

URL: /api/v1/heartbeat/{device-name}

Пример тела запроса:
```
{
    "token": "abacaba",
    "values": [
        "temperature": 25.00,
        "humidity": 50.00
    ]
}
```

**Формат ответов:**

HTTP 200 
<!-- [[[end]]] -->

<!-- [[[cog
import cog
cog.out(open("./signaling/docs/README.md").read())
]]] -->
### Модуль сигнализации

#### Схема цепи

[Проект Wokwi](https://wokwi.com/projects/457487642705258497)

![Схема цепи](/src/devices/signaling/docs/image.png)

#### Описание

Для мониторинга параметров пожарной безопасности и связи с облачным сервером используется микроконтроллер ESP-32.

В системе используются следующие датчики:

- Датчик дыма (MQ-2) - подключен к 12 цифровому пину ESP-32. Сигнал "1" означает обнаружение дыма.

- Датчик пламени - подключен к 13 цифровому пину ESP-32. Сигнал "1" означает обнаружение пламени.

- Датчик температуры (DS18B20) - подключен к 4 цифровому пину ESP-32. Измеряет температуру в градусах Цельсия.


Сигнализация активируется при выполнении любого из условий:

- Обнаружение дыма (датчик дыма)

- Обнаружение пламени (датчик пламени)

- Превышение температуры порога в 60°C

#### Взаимодействие с сервером

Во время эмуляции микроконтроллер подключается к Wi-Fi сети, отправляет HTTP-запросы к серверу. В запросах отправляется текущее состояние всех датчиков и статус сигнализации. Сервер принимает данные для мониторинга состояния системы.

**Формат запросов:**

HTTP POST 

URL: /api/v1/heartbeat/FIRE-ALARM-1

body:
```
{
    "values": {
        "smoke": 1,
        "flame": 1,
        "temperature": 23.5,
        "alarm": 1
    },
    "token": "fire_alarm_token"
}
```

**Формат ответов:**

HTTP 200 - подтверждение получения данных 

```
{
    "message": "OK", 
    "trigger_active": false, 
    "state": 0.0, 
    "metadata": {}
}
```
<!-- [[[end]]] -->

<!-- [[[cog
import cog
cog.out(open("./door/docs/README.md").read())
]]] -->
### Модуль Двери с RFID считывателем

#### Схема цепи

[Проект Wokwi](https://wokwi.com/projects/456849249505441793)

![Схема цепи](/src/devices/door/docs/door-circuit.png)

#### Взаимодействие с сервером

**Формат запросов:**

HTTP POST 

URL: /api/v1/heartbeat/{device-name}

Пример тела запроса:
```
{
    "token": "walkieneelitalkie",
    "values": [
        "state": 0
    ]
}
```

**Формат ответов:**

HTTP 200 
```
{
    "metadata" = {
        "authorized_keys" = [
            "01:02:03:04",
            ...
        ];
    }
}
```
<!-- [[[end]]] -->


<!-- [[[cog
import cog
cog.out(open("./light/docs/README.md").read())
]]] -->
### Модуль температуры и влажности

#### Схема цепи

[Проект Wokwi](https://wokwi.com/projects/457945071017151489)

![Схема цепи](/src/devices/light/docs/circuit.png)

#### Описание

Датчик считывает освещение в люксах, если больше 100 люксов тогда считается, что светло, иначе темно

#### Взаимодействие с сервером

Во время эмуляции микроконтроллер подключается к Wifi-сети, отправляет HTTP-запросы к серверу. В запросах отправляется текущее состояние датчика. Температура передаётся в целочисленном формате с учётом двух разрядов дробной части, например 2500 означает 25,00 градусов цельсия. Относительная влажность передаётся аналогичным образом, например 5000 означает 50.00 % влажности.

**Формат запросов:**

HTTP POST 

URL: /api/v1/heartbeat/{DEVICE-NAME}

body:
```
{
    "token": str
    "values": {
        "is_dark": bool
    }
}
```

**Формат ответов:**

HTTP 200 
<!-- [[[end]]] -->


<!-- [[[cog
import cog
cog.out(open("./pid/docs/README.md").read())
]]] -->
### Модуль датчика присутствия

#### Схема цепи

[Проект Wokwi](https://wokwi.com/projects/457402782664985601)

![Схема цепи](/src/devices/pid/docs/circuit.png)

#### Взаимодействие с сервером

**Формат запросов:**

HTTP POST 

URL: /api/v1/heartbeat/{device-name}

Пример тела запроса:
```
{
    "token": "abacaba",
    "values": [
        "pid": 1.0
    ]
}
```

**Формат ответов:**

HTTP 200 
<!-- [[[end]]] -->


<!-- [[[cog
import cog
cog.out(open("./watch/docs/README.md").read())
]]] -->
### Модуль часов и будильника

#### Схема цепи

[Проект Wokwi](https://wokwi.com/projects/456035055362315265)

![Схема цепи](/src/devices/watch/docs/circuit.png)

#### Взаимодействие с сервером

**Формат запросов:**

HTTP POST 

URL: /api/v1/heartbeat/{device-name}

body:
```
{
    "token": "abacaba",
    "values": [
        "state": 0.0
    ]
}
```

**Формат ответов:**

HTTP 200 

body:
```
{
	"metadata": {
		"alarms": [
			{
				"time": "16:05:00"
			},
			{
				"time": "16:10:00"
			}
		]
	}
}
```
<!-- [[[end]]] -->

<!-- [[[cog
import cog
cog.out(open("./plant-watering/docs/README.md").read())
]]] -->
<!-- [[[end]]] -->
