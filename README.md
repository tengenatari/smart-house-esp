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
cog.out(open("./src/devices/README.md").read())
]]] -->
<!-- [[[end]]] -->