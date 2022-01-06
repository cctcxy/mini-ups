from django.shortcuts import render, redirect
from django.http import HttpResponse
from django.contrib.auth import login, authenticate
from django.contrib.auth.forms import UserCreationForm
from django.contrib import messages
from .forms import ContactForm, PackageForm, UserRegisterForm
from django.http import HttpResponseRedirect
from django.urls import reverse
from .jobs import current_user

from django.conf import settings
from django.core.mail import send_mail
import socket
import sys
import json
import psycopg2

# Create your views here.

def home(request):
    current_user(request)
    try:
        connection = psycopg2.connect(user="postgres",
                                    password="postgres",
                                    host="db",
                                    database="postgres")
        cursor = connection.cursor()
        print("Connect to database successful")
        postgreSQL_select_Query = "select shipid, status from package"
        cursor.execute(postgreSQL_select_Query)
        ship_records = cursor.fetchall()

    except (Exception, psycopg2.Error) as error:
        print("Error ", error)

    finally:
        # closing database connection.
        if connection:
            cursor.close()
            connection.close()
    records = []
    for ship_record in ship_records:
        print(ship_record)

        if ship_record[1] == 0:
            status_str = "send truck to warehouse"
        if ship_record[1] == 1:
            status_str = "wait for Amazon to say it's loaded"
        if ship_record[1] == 2:
            status_str = "send it for delivery"
        if ship_record[1] == 3:
            status_str = "delivered"
        
        records.append({
            'packageid': ship_record[0],
            'status': status_str
        })

    context = {
        'records':records
    }
    
    return render(request, 'ups/home.html', context)


def own(request):
    current_user(request)
    user = request.user
    try:
        connection = psycopg2.connect(user="postgres",
                                    password="postgres",
                                    host="db",
                                    database="postgres")
        cursor = connection.cursor()
        print("Connect to database successfully")
        print(user.username)
        postgreSQL_select_Query = "select shipid, status, dest_x, dest_y, descr, account from package"
        cursor.execute(postgreSQL_select_Query)
        ship_records = cursor.fetchall()

    except (Exception, psycopg2.Error) as error:
        print("Error while fetching data from PostgreSQL", error)

    finally:
        # closing database connection.
        if connection:
            cursor.close()
            connection.close()

    own_records = []
    for ship_record in ship_records:
        print(ship_record)
        if ship_record[1] == 0:
            status_str = "truck heading to the warehouse"
            canRedirect = 1
        if ship_record[1] == 1:
            status_str = "waiting to be loaded"
            canRedirect = 1
        if ship_record[1] == 2:
            status_str = "delivering"
            canRedirect = 0
        if ship_record[1] == 3:
            status_str = "delivered"
            canRedirect = 0
        
        if ship_record[5]==user.username:
            own_records.append({
                'packageid': ship_record[0],
                'can_redirect': canRedirect,
                'status': status_str,
                'destination_x': ship_record[2],
                'destination_y': ship_record[3],
                'dscription': ship_record[4],
            })

    context = {
        'records':own_records
    }
    
    return render(request, 'ups/own.html', context)


def redirect(response, shipid):
    # current_user(request)
    if response.method == "POST":
        form = ContactForm(response.POST)
        print("form is made")
        print(form.errors)
        if form.is_valid():
            
            # Do something with the form data 
            # packageId = form.cleaned_data["packageId"]
            packageId = shipid
            newAddress_x= form.cleaned_data["x"]
            newAddress_y = form.cleaned_data["y"]
            print(newAddress_x, newAddress_y)
            message = "redirect\n"+ str(packageId) +" "+ str(newAddress_x) +" " + str(newAddress_y)+" "


            try: 
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                port = 12346
                host_ip = socket.gethostbyname('ups')
                s.connect((host_ip, port)) 
                print ("the socket has successfully connected to server 12346")

            except:
                print ("socket error")
                sys.exit() 

            # s = connectToServer()

            Message = message.encode("utf-8")
            # _EncodeVarint(s.send, len(Message), None)
            s.send(Message)
            
            try:
                data = s.recv(8).decode()
            except:
                pass
            s.close()
            context = {
                'x' : newAddress_x,
                'y' : newAddress_y,
                'response' : data
            }
            return render(response, 'ups/redirect.html', context)
    else:
        form = ContactForm()

    return render(response, 'ups/redirect.html', {'form': form})



def redirectSuccess(request):
    current_user(request)
    return render(request, 'ups/redirectSuccess.html')

def register(request):
    current_user(request)
    if request.method == 'POST':
        form = UserRegisterForm(request.POST)
        if form.is_valid():
            form.save()
            username = form.cleaned_data.get('username')
            messages.success(request, f'Account created for {username}!')
            # subject = 'welcome to ups'
            # message = f'Hi {username}, thank you for registering in ups.'
            # email_from = settings.EMAIL_HOST_USER
            # user = request.user
            # print(user.id)
            # recipient_list = [user.email, ]
            # user.email_user(subject, message, email_from)
            # send_mail( subject, message, email_from, recipient_list )
            return HttpResponseRedirect(reverse('home'))
    else:
        form = UserRegisterForm()
    return render(request, "ups/register.html", {"form": form})


def searchPackgae(response):
    current_user(response)
    if response.method == "POST":
        form = PackageForm(response.POST)
        if form.is_valid():
            
            # Do something with the form data 
            id= form.cleaned_data["packageId"]
            

            try: 
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                port = 12346
                host_ip = socket.gethostbyname('ups')
                s.connect((host_ip, port)) 
                print ("the socket has successfully connected to server 12346")

            except:
                print ("socket error")
                sys.exit() 

            # s = connectToServer()
            message = "query\n"+str(id)+" "
            Message = message.encode("utf-8")
            # _EncodeVarint(s.send, len(Message), None)
            s.send(Message)
            try:
                res = s.recv(1024)
            except:
                pass
            records = []
            sql = "select shipid, status from package where shipid = "+str(id)
            try:
                connection = psycopg2.connect(user="postgres",
                                            password="postgres",
                                            host="db",
                                            database="postgres")
                cursor = connection.cursor()
                print("Connect to database successfully")
                postgreSQL_select_Query = sql
                cursor.execute(postgreSQL_select_Query)
                package_records = cursor.fetchall()

            except (Exception, psycopg2.Error) as error:
                print("Error while fetching data from PostgreSQL", error)

            finally:
                # closing database connection.
                if connection:
                    cursor.close()
                    connection.close()
            if package_records ==[]:
                context = {
                    "form": form,
                    "error": "do not have this package"
                }
                return render(response, "ups/search.html", context)

            if package_records[0][1] == 0:
                status_str = "truck heading to the warehouse"
            if package_records[0][1] == 1:
                status_str = "waiting to be loaded"
            if package_records[0][1] == 2:
                status_str = "delivering"
            if package_records[0][1] == 3:
                status_str = "delivered"

            records.append({
                'packageid': package_records[0][0],
                'status': status_str
            })
            context = {
                "form": form,
                "result": records
            }
            return render(response, "ups/search.html", context)
        
    else:
        form = PackageForm()

    return render(response, "ups/search.html", {'form': form})

