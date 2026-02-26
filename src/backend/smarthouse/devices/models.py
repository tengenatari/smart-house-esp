from django.utils import timezone

from django.db import models


class Group(models.Model):
    id = models.AutoField(primary_key=True)
    name = models.CharField(max_length=255, unique=True)
    description = models.TextField(blank=True)

    def __str__(self):
        return self.name

class Field(models.Model):

    id = models.AutoField(primary_key=True)
    name = models.CharField(max_length=255, unique=True)
    measurement_unit = models.CharField(max_length=16)

    def __str__(self):
        return f"{self.name} ({self.measurement_unit})"


class TypeDevice(models.Model):
    id = models.AutoField(primary_key=True)
    name = models.CharField(max_length=255, unique=True)

    fields = models.ManyToManyField(Field)

    def __str__(self):
        return self.name


class Device(models.Model):

    id = models.AutoField(primary_key=True)
    name = models.CharField(max_length=255, unique=True)
    token = models.CharField(max_length=255)



    timeout = models.IntegerField(null=True)
    last_heartbeat = models.DateTimeField(null=True)
    is_active = models.BooleanField(default=False)

    type_device = models.ForeignKey(TypeDevice, on_delete=models.PROTECT, null=True)
    group = models.ForeignKey(Group, on_delete=models.PROTECT, null=True, blank=True)

    triggers = models.ManyToManyField('Trigger', blank=True)

    description = models.TextField(blank=True)

    def update_last_heartbeat(self) -> None:
        """
        Обновляет состояние датчика и его последний heartbeat
        """
        self.last_heartbeat = timezone.now()
        self.is_active = True
        self.save()

    def update_health(self) -> None:
        """
        Обновляет состояние датчика
        :return:
        """

        is_active = False
        if self.last_heartbeat is not None:
            timediff = timezone.now() - self.last_heartbeat
            is_active = timediff.total_seconds() > self.timeout

        self.is_active = is_active
        self.save()
    def __str__(self):
        return self.name


class Message(models.Model):

    id = models.AutoField(primary_key=True)

    device = models.ForeignKey(Device, on_delete=models.PROTECT)
    field = models.ForeignKey(Field, on_delete=models.PROTECT)
    value = models.FloatField()
    created_at = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"{self.device.name}: {self.value} {self.field.measurement_unit}"


class Trigger(models.Model):

    id = models.AutoField(primary_key=True)
    name = models.CharField(max_length=255, unique=True, null=True)


    condition = models.TextField()
    state = models.TextField()

    related_conditions = models.ManyToManyField('self',
        symmetrical=True,
        blank=True,
        related_name='related_devices')

    block_related_conditions = models.BooleanField()
    block_on = models.IntegerField()

    last_block = models.DateTimeField(null=True, blank=True)

    description = models.TextField(blank=True)

    def __str__(self):
        return f"{self.name}"

