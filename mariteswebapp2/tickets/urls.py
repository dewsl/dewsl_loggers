from django.urls import path
from . import views

urlpatterns = [
    path('activeTickets/', views.activeTickets, name='Active Tickets'),
]