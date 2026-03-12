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
from .models import Device, Field, Message, Trigger, Group
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
    """
    Обработчик получения сообщений от устройств
    :param request:
    Объект http-запроса
    :param name:
    Имя устройства
    :return:
    Объект http-ответа
    """
    token = request.data.get('token')
    device = Device.objects.filter(name=name, token=token).first()

    if not device:
        return JsonResponse({'message': 'Device not found'}, status=status.HTTP_404_NOT_FOUND)

    fields = device.type_device.fields.all()
    values = request.data.get("values")


    fill_fields(device, fields, values)
    device.update_last_heartbeat()
    conditions = check_conditions(device)

    metadata = request.data.get("metadata")

    if metadata is not None:
        device.metadata = metadata
        device.save()



    message = {
        "message": "OK",
        "trigger_active": False,
        "state": 0.0,
        "metadata": device.metafield,
        }



    if conditions:
        try:
            message["trigger_active"] = True
            message["state"] = float(conditions[0])
        except TypeError:
            return JsonResponse({'message': 'Invalid conditions'}, status=status.HTTP_400_BAD_REQUEST)

    return JsonResponse(message, status=status.HTTP_200_OK)


def fill_fields(device: Device, fields: list[Field], values: dict) -> None:
    """
    Функция заполнения полей девайса
    :param device:
    Объект девайса
    :param fields:
    Объекты полей девайса
    :param values:
    Значения параметров
    :return:
    None
    """
    for field in fields:
        value = values.get(field.name)
        if value is None:
            continue

        device_message = Message(device=device, field=field, value=value)
        device_message.save()


def get_latest_field_values(group: Group) -> list:
    """
    Получает последние значения для всех полей всех устройств в группе
    :param group:
    Объект группы устройств
    :return:
    Список значений последних сообщений устройств
    """
    if not group:
        return []

    latest_subquery = Message.objects.filter(
        device=OuterRef('device'),
        field=OuterRef('field')
    ).order_by('-created_at').values('value')[:1]

    messages = Message.objects.filter(
        device__group=group
    ).annotate(
        latest_value=Subquery(latest_subquery)
    ).values(
        'device__id',
        'device__name',
        'field__name',
        'latest_value'
    ).distinct()
    return messages


def sympify_messages(expr, messages) -> float:
    for message in messages:
        expr = expr.replace(f'{{{message['device__name']}}}.{{{message['field__name']}}}', str(message['latest_value']))
    try:
        result = sympify(expr)
    except SympifyError:
        return 0.0
    return result


def check_conditions(device: Device) -> list[float]:
    """
    Функция проверки триггера состояний девайсов
    :param device:
    Объект девайса
    :return:
    Список состояний объекта, которые он должен принять.
    """
    triggers = device.triggers.all()
    latest_messages = get_latest_field_values(device.group)

    triggered = []
    for trigger in triggers:
        if sympify_messages(trigger.condition, latest_messages):
            triggered.append(sympify_messages(trigger.state, latest_messages))

    return triggered



