from django.db import models

# Create your models here.

# class truck(models.Model):
#     #ride_id = models.AutoField(primary_key=True)
#     id = models.IntegerField(primary_key=True, null = False)
#     status = models.IntegerField(null = False)
#     x = models.IntegerField(null = False)
#     y = models.IntegerField(null = False)


# class package(models.Model):
#     shipid = models.IntegerField(primary_key=True, null = False)
#     whnum = models.IntegerField(null = False)
#     status = models.IntegerField(null = False)
#     dest_x = models.IntegerField(null = False)
#     dest_y = models.IntegerField(null = False)
#     descr = models.CharField(max_length = 100, null = False)
#     account = models.CharField(max_length = 100, null = False)
#     truck = models.ForeignKey(truck, on_delete=models.CASCADE)
