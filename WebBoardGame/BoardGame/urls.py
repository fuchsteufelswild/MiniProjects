from django.urls import path
from BoardGame import views

urlpatterns = [
    path('register/', views.register, name='register'),
    path('server_lobby/', views.server_lobby, name="game"),
    path('game_lobby/', views.game_lobby, name="game_lobby"),
    path('login/', views.user_login, name="login"),
    path('game_scene/', views.in_game, name="in_game"),
    path('', views.login, name='login'),
]
