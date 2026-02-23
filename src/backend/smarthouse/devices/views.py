import sys

from django.db import transaction
from django.db.models import Max, OuterRef, Subquery
from django.shortcuts import render
from django.views.decorators.http import condition
from drf_spectacular.utils import extend_schema, OpenApiResponse, inline_serializer

from rest_framework import status, serializers
from rest_framework.decorators import api_view, permission_classes
from rest_framework.permissions import AllowAny
from django.http import JsonResponse
from .models import Device, Field, Message, Condition
from .serializers import simple_message, heartbeat

from sympy import sympify, SympifyError



@extend_schema(
    summary='heartbeat устройства',
    description='Обновляет heartbeat устройства по имени',
    tags=['Devices'],
    request= heartbeat,
    responses={
        200: simple_message,
        404: simple_message
    }
)
@api_view(['POST'])
@permission_classes([AllowAny])
def heartbeat(request, name):
    token = request.data.get('token')
    device = Device.objects.filter(name=name, token=token).first()

    if not device:
        return JsonResponse({'message': 'Device not found'}, status=status.HTTP_404_NOT_FOUND)

    fields = device.type_device.fields.all()
    values = request.data.get("values")

    if fields and not values:
        return JsonResponse({'message': 'Missing values'}, status=status.HTTP_400_BAD_REQUEST)

    fill_fields(device, fields, values)
    device.update_last_heartbeat()
    conditions = check_conditions(device)

    message = {"message": "OK"}

    if conditions:
        message["state"] = conditions[0]


    return JsonResponse(message, status=status.HTTP_200_OK)


def fill_fields(device: Device, fields: list[Field], values: dict) -> None:
    """

    :param device:
    Объект девайса
    :param fields:
    Объекты полей девайса
    :param values:
    значения параметров
    :return:
    None
    """

    for field in fields:
        value = values.get(field.name)
        if not value:
            continue

        device_message = Message(device=device, field=field, value=value)
        device_message.save()


def get_latest_field_values(device: Device) -> dict:
    latest_subquery = Message.objects.filter(
        device=device,
        field=OuterRef('field')
    ).order_by('-created_at').values('value')[:1]

    messages = Message.objects.filter(device=device).annotate(
        latest_value=Subquery(latest_subquery)
    ).values('field__name', 'latest_value').distinct()

    print(str(messages))
    return messages

def check_conditions(device: Device) -> list[float]:
    """
    Функция проверки триггера состояний девайсов
    :param device:
    Объект девайса
    :return:
    Список состояний объекта, которые он должен принять.
    """


    conditions = Condition.objects.filter(device=device).all()
    latest_messages = get_latest_field_values(device)
    triggered = []

    for cond in conditions:
        expr = cond.condition
        for message in latest_messages:
            expr = expr.replace(f'{{{message['field__name']}}}', str(message['latest_value']))
        result = sympify(expr)
        if result:
            triggered.append(cond.state)


    return triggered



