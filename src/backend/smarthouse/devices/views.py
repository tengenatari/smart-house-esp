from django.shortcuts import render
from drf_spectacular.utils import extend_schema, OpenApiResponse, inline_serializer

from rest_framework import status, serializers
from rest_framework.decorators import api_view, permission_classes
from rest_framework.permissions import AllowAny
from django.http import JsonResponse
from .models import Device
from .serializers import message, heartbeat
# Register your models here.



@extend_schema(
    summary='heartbeat устройства',
    description='Обновляет heartbeat устройства по имени',
    tags=['Devices'],
    request= heartbeat,
    responses={
        200: message,
        404: message
    }
)
@api_view(['POST'])
@permission_classes([AllowAny])
def heartbeat(request, name):
    token = request.data.get('token')

    device = Device.objects.filter(name=name, token=token).first()

    if not device:
        return JsonResponse({'message': 'Device not found'}, status=status.HTTP_404_NOT_FOUND)

    device.update_last_heartbeat()

    return JsonResponse({"message": "OK"}, status=status.HTTP_200_OK)

