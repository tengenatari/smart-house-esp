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

#### Описание

Для управления реле и связи с облачным сервером спользуется микроконтроллер ESP-32. 

Для индикации наличия связи с сервером используется залёный светодиод, подключённый к 0 цифровому пину ESP-32.

Реле имитирует исполняющие устройства, которые отсутствуют в эмуляции Wokwi. Реле подключено к 4 цифровому пину ESP-32.

#### Взаимодействие с сервером

Во время эмуляции микроконтроллер подключается к Wifi-сети, отправляет HTTP-запросы к серверу. В запросах отправляется текущее состояние реле. В качестве ответа получаем состояние, которое необходимо установить.

**Формат запросов:**

HTTP POST 

URL: /api/v1/имя_исполняющего_устройства/id

body:
```
{
    "active": true
}
```

**Формат ответов:**

HTTP 200 

body:
```
{
    "active": true
}
```
<!-- [[[end]]] -->