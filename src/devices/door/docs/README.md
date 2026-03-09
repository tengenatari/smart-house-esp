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