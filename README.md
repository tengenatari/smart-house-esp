# smart-house-esp
 
Проект по эмуляции умного дома с использованием ESP32, HTTP и WOKWI.

Запуск проекта
```
docker-compose up -d --build
docker-compose exec web python manage.py migrate
docker-compose exec web python manage.py createsuperuser
```

<!-- [[[cog
import cog

content = open("./src/devices/README.md").read()
open_pattern = "--!<"
close_pattern = ">--"

open_pattern = open_pattern[::-1]
close_pattern = close_pattern[::-1]

while (content.find(open_pattern) != -1):
    content = content[:content.find(open_pattern)] + content[content.find(close_pattern)+len(close_pattern):]

cog.out(content)
]]] -->
# Devices
Каталог контроллеров и датчиков. Настройки и код.


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