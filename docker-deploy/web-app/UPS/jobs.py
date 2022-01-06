from schedule import Scheduler
import threading
import time
from django.conf import settings
from django.core.mail import send_mail
from django.contrib.auth.models import User
from django.contrib.auth.forms import UserCreationForm
import psycopg2

user = None
def current_user(request):
    global user
    user = request.user

def sendEmailOnceDelievered():
    if user != None:
        try:
            # print("sendEmailOnceDelievered")
            connection = psycopg2.connect(user="postgres",
                                        password="postgres",
                                        host="db",
                                        database="postgres")
            cursor = connection.cursor()
            # print("Connect to database successful")
            postgreSQL_select_Query = "select shipid from delivered_history;"
            cursor.execute(postgreSQL_select_Query)
            delivered_ship_records = cursor.fetchall()

            for ship_record in delivered_ship_records:
                print(ship_record)
                sql = "select account, descr from package where shipid = %s" 
                cursor.execute(sql, (ship_record,))
                shipinfo = cursor.fetchall()
                try:
                    uid = User.objects.get(username=shipinfo[0][0])

                    subject = 'package delivered'
                    message = f'Hi {shipinfo[0][0]}, your package with shipId: {ship_record} has been delivered.'
                    email_from = settings.EMAIL_HOST_USER
                    recipient_list = [uid.email , ]
                    send_mail( subject, message, email_from, recipient_list )
                    sql1 = "delete from delivered_history where shipid = %s"
                    cursor.execute(sql1, (ship_record,))
                    connection.commit()
                except User.DoesNotExist:
                    return None

        except (Exception, psycopg2.Error) as error:
            print("Error ", error)

        finally:
                # closing database connection.
                if connection:
                    cursor.close()
                    connection.close()


def run_continuously(self, interval=10):
    """Continuously run, while executing pending jobs at each elapsed
    time interval.
    @return cease_continuous_run: threading.Event which can be set to
    cease continuous run.
    Please note that it is *intended behavior that run_continuously()
    does not run missed jobs*. For example, if you've registered a job
    that should run every minute and you set a continuous run interval
    of one hour then your job won't be run 60 times at each interval but
    only once.
    """

    cease_continuous_run = threading.Event()

    class ScheduleThread(threading.Thread):

        @classmethod
        def run(cls):
            while not cease_continuous_run.is_set():
                self.run_pending()
                time.sleep(interval)

    continuous_thread = ScheduleThread()
    continuous_thread.setDaemon(True)
    continuous_thread.start()
    return cease_continuous_run

Scheduler.run_continuously = run_continuously


def start_scheduler():
    scheduler = Scheduler()
    scheduler.every().second.do(sendEmailOnceDelievered)
    scheduler.run_continuously()