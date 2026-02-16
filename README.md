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
import re
content = open("./src/devices/README.md").read()
pattern = r'>-- ]\]\]\dne[\[\[\ --!<?*.]\]\]\?*.goc[\[\[\ --!<'
pattern = pattern[::-1]
content = re.sub(pattern, '', content, flags=re.DOTALL)
cog.out(content)
]]] -->

<!-- [[[end]]] -->