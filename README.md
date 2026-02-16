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

<!-- [[[end]]] -->