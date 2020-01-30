from django.shortcuts import render
from django.http import HttpResponse, HttpResponseRedirect
from BoardGame.forms import FormUser
from django.contrib.auth import authenticate, login, logout
from django.urls import reverse
from django.contrib.auth.decorators import login_required

from BoardGame.game import *
import time
import json
from manage import user_gids
# Create your views here.



def index(request):
    return render(request, 'index.html')

@login_required
def server_lobby(request):
    username = request.user.username
    if username not in user_gids:
        user_gids[username] = -1
    if(request.method == "POST"):
        for key, value in request.POST.items():
           print('Key: %s' % (key) )
           # print(f'Key: {key}') in Python >= 3.7
           print('Value %s' % (value) )
           # print(f'Value: {value}') in Python >= 3.7
        g_id = int(request.POST.get('game_id'))
        if(int(request.POST.get('join')) == 0):
            #user_gids[username] = -1
            if(g_id > 0):
                game_state = Game.all_games[g_id - 1].state()
                return HttpResponse(game_state)
        if(g_id != -1 and Game.all_games[g_id].join_helper(username)):
            user_gids[username] = g_id
            return HttpResponse(Game.all_games[g_id].config)
            return HttpResponseRedirect('../game_lobby/')
        else:
            return  HttpResponse('./')


    game_list = Game.listgames()
    time.sleep(0.2)
    return render(request, 'lobby.html', { 'game_list' : game_list })

@login_required
def logout(request):
    logout(request)
    return HttpResponseRedirect(reverse('index'))

@login_required
def game_lobby(request):
    username = request.user.username

    curr_game = Game.all_games[user_gids[username]]

    if(curr_game.is_game_started):
        return HttpResponseRedirect('../game_scene/')

    if(request.method == 'POST'):
        ready_val = int(request.POST.get('ready_val'))

        if(ready_val == 1):
            curr_game.players[curr_game.player_ids[username]][0].ready()

    if(curr_game.is_game_started):
        return HttpResponseRedirect('../game_scene/')

    player_infos = curr_game.get_player_info()
    time.sleep(0.2)
    return render(request, 'game_lobby.html', { 'conf' : curr_game.config , 'players' : {pl[0].name : pl[1] for id, pl in curr_game.players.items()}})

@login_required
def get_game_state(request):

    username = request.user.username
    print(user_gids[username])
    return HttpResponse(Game.all_games[user_gids[username]].state())

@login_required
def in_game(request):
    username = request.user.username

    curr_game = Game.all_games[user_gids[username]]

    if(len(curr_game.players) == 0):
        return HttpResponseRedirect('../server_lobby/')

    if(request.method == "POST"):
        roll_val = int(request.POST.get('roll_val'))
        pick_val = int(request.POST.get('pick_val'))

        pl_id = curr_game.player_ids[username]
        curr_game.seq_lock.acquire()
        if(pl_id == curr_game.turn_seq):
            pl = curr_game.players[pl_id][0]
            if(pl.waiting_for_roll_answer and roll_val == 1):
                pl.pick_lock.release()
                pl.waiting_for_roll_answer = False
            elif(pl.waiting_for_pick_answer):
                if(pick_val == 1):
                    pl.should_pick = 1
                elif(pick_val == 0):
                    pl.should_pick = 0
                pl.pick_lock.release()
                pl.waiting_for_pick_answer = False
        curr_game.seq_lock.release()
        return HttpResponse(curr_game.state())
    time.sleep(0.2)

    return render(request, 'in_game.html', { 'is_pick' : curr_game.players[curr_game.player_ids[username]][0].waiting_for_pick_answer, 'game_id': curr_game.id, 'is_roll' : curr_game.players[curr_game.player_ids[username]][0].waiting_for_roll_answer, 'turn_on' : curr_game.turn_seq == curr_game.player_ids[username], 'curr_player' : curr_game.player_ids[username], 'game_state': curr_game.state(), 'players': curr_game.get_player_info() } )

def user_login(request):
    if(request.method == "POST"):
        name = request.POST.get('username')
        pw = request.POST.get('password')

        user = authenticate(username=name, password=pw)

        if(user):
            if(user.is_active):
                login(request, user)
                return HttpResponseRedirect('../server_lobby')
            else:
                return HttpResponse("./")
        else:
            return HttpResponseRedirect("./")
    else:
        return render(request, 'login.html', {})

def register(request):
    isRegistered = False

    if(request.method == "POST"):
        user_form = FormUser(data=request.POST)

        if(user_form.is_valid()):
            user = user_form.save()
            user.set_password(user.password)
            user.save()

            isRegistered = True
    else:
        user_form = FormUser()

    return render(request, 'register.html', { 'user_form':user_form,
                                              'isRegistered': isRegistered })
