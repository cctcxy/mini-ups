from django import forms
from django.contrib.auth.models import User
from django.contrib.auth.forms import UserCreationForm

class ContactForm(forms.Form):
    # packageId = forms.IntegerField(label = "packageId")
    x = forms.IntegerField(label = "x")
    y = forms.IntegerField(label = "y")

class PackageForm(forms.Form):
    packageId = forms.IntegerField(label = "packageId")

class UserRegisterForm(UserCreationForm):
    email = forms.EmailField()

    class Meta:
        model = User
        fields = ['username', 'email', 'password1', 'password2']