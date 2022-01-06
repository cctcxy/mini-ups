from django.apps import AppConfig
import os
import socket
import sys


class UpsConfig(AppConfig):
    name = 'ups'

    def ready(self):
        from . import jobs

        # if os.environ.get('RUN_MAIN', None) != 'true':
        jobs.start_scheduler()
        
        
