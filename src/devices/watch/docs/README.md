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
