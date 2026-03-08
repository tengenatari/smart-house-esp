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