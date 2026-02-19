
from drf_spectacular.utils import inline_serializer
from rest_framework import serializers

message = inline_serializer(
            name='message',
            fields={
                'message': serializers.CharField(),
            }) #сериализатор для простого сообщения

heartbeat = inline_serializer(
            name='heartbeat',
            fields={
                'token': serializers.CharField(),
            }) #сериализатор для токена