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
