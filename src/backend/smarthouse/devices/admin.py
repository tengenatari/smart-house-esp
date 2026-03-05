from django.contrib import admin
from .models import Device, Field, TypeDevice, Message, Trigger, Group
from django.db import models
from django_json_widget import widgets
class DeviceAdmin(admin.ModelAdmin):
    formfield_overrides = {
        models.JSONField: {'widget': widgets.JSONEditorWidget(options={
                    'mode': 'tree'
                })}
    }

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

