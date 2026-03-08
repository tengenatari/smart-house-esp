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
