
from drf_spectacular.utils import inline_serializer
from rest_framework import serializers

simple_message = inline_serializer(
            name='simple_message',
            fields={
                'simple_message': serializers.CharField(),
            }) #сериализатор для простого сообщения

heartbeat = inline_serializer(
            name='heartbeat',
            fields={
                'token': serializers.CharField(),
            }) #сериализатор для токена