from django.contrib import admin
from .models import Device, Field, TypeDevice, Message, Trigger, Group


class DeviceAdmin(admin.ModelAdmin):
    list_display = ['id', 'name', 'last_heartbeat', 'timeout', 'is_active', 'type_device']

class ReferenceAdmin(admin.ModelAdmin):
    list_display = ['id', 'name']

class MessageAdmin(admin.ModelAdmin):
    list_display = ['id', 'device', 'field', 'value']

admin.site.register(Device, DeviceAdmin)
admin.site.register(Field, ReferenceAdmin)
admin.site.register(TypeDevice, ReferenceAdmin)
admin.site.register(Message, MessageAdmin)

admin.site.register(Trigger)
admin.site.register(Group)