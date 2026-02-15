# smart-house-esp
Project about emulating smart house using ESP32, HTTP and wokwi


Starting project

docker-compose up -d --build
docker-compose exec web python manage.py migrate
docker-compose exec web python manage.py createsuperuser
