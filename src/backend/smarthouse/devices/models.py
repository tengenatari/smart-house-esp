from django.utils import timezone

from django.db import models
# Create your models here.


class Device(models.Model):

    id = models.AutoField(primary_key=True)
    name = models.CharField(max_length=255, unique=True)
    token = models.CharField(max_length=255)

    timeout = models.IntegerField(null=True)
    last_heartbeat = models.DateTimeField(null=True)
    is_active = models.BooleanField(default=False)
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



