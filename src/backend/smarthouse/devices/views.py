from django.shortcuts import render

from rest_framework import status
from rest_framework.decorators import api_view, permission_classes
from rest_framework.permissions import AllowAny
from django.http import JsonResponse
from .models import Device
# Register your models here.

@api_view(['POST'])
@permission_classes([AllowAny])
def heartbeat(request, name):
    token = request.data.get('token')

    device = Device.objects.filter(name=name, token=token).first()

    if not device:
        return JsonResponse({'error': 'Device not found'}, status=status.HTTP_404_NOT_FOUND)

    device.update_last_heartbeat()


    return JsonResponse({"message": "OK"}, status=status.HTTP_200_OK)

