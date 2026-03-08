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
