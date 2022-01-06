from django.urls import path

from . import views

urlpatterns = [
    path('', views.home, name='home'),
    path('own/', views.own, name='own'),
    # path('own/redirect/', views.redirect, name='redirect'),
    path('own/redirect/<int:shipid>/', views.redirect, name='redirect'),
    path('search/', views.searchPackgae, name='search'),
    path('own/redirect/success/', views.redirectSuccess, name='redirect-success'),
]