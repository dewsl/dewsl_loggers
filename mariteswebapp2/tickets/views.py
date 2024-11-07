from django.shortcuts import render
from django.template import loader
from django.http import HttpResponse    

def activeTickets(request):
    template = loader.get_template('home.html')
    return HttpResponse(template.render())

# Create your views here.
