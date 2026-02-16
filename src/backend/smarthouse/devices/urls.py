from django.urls import path
from . import views

urlpatterns = [
    path('heartbeat/<str:name>', views.heartbeat, name='health_check'),
]

