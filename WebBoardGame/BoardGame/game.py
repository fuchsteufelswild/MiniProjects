import json
import random
import socket
import concurrent
import threading
import time
# Content for any cell
# Either can be an artifact or an action
class CellContent:
    def __init__(self):
        pass

    def execute_action(self, player):
        raise NotImplementedError


# Empty Action/Artifact for Cell
# Does not do anything
class EmptyContent(CellContent):
    def __init__(self):
        pass

    def execute_action(self, player):
        pass

    def __str__(self):
        return "''"

# Artifact
# Contains (name, owned status, price, and CellAction)
# Does not do anything on execute_action
# On take artifact updates cell content with the action it contains
class Artifact(CellContent):
    def __init__(self, _name, _owned, _price, _action):
        self.name = _name
        self.owned = _owned
        self.price = _price
        self.action = _action

    def execute_action(self, player):
        pass

    def take_artifact(self, player, cells):
        if(player.credit < player.target_artifact.price):
            return

        print("Player: ", player.name, " took the artifact ", self.name)
        player.credit -= player.target_artifact.price # Decrease the player credit
        if(player.target_artifact.owned): # If artifact is owned
            player.artifacts.append(player.target_artifact) # Own it
            player.owned_artifact_count += 1 # Owned artifact count
        cells[player.current_cell_no].content = player.target_artifact.action # Replace the cell with the action
        cells[player.current_cell_no].is_artifact = False # Set cell state for an artifact to False

    def __str__(self):
        return "Name: {}\nOwned: {}\nPrice: {}\nAction: {}".format(self.name, self.owned, self.price, self.action)


# Cell Action
class CellAction(CellContent):
    def __init__(self):
        pass

    # Abstract method for executing an action on the player
    def execute_action(self, player):
        raise NotImplementedError


# Jump Action upon execute_action
# Changes cell depending on the given inputs
class JumpAction(CellAction):
    def __init__(self, _relative, _value):
        self.relative = _relative
        self.value = _value

    def execute_action(self, player):
        if (self.relative):
            player.current_cell_no += self.value
        else:
            if (player.current_cell_no > self.value):
                if (player.game.cycles and (len(
                        player.game.cells) - player.current_cell_no - 1 + self.value < player.current_cell_no - self.value)):
                    player.finished_rounds += 1
            player.current_cell_no = self.value
        print("Player jumps to: ", player.current_cell_no)
        player.game.on_player_move(player)

    def __str__(self):
        return "Type: {}\nJumpValue: {}".format("jump", self.value)


# Upon Activation makes player stuck up in a
# cell it currently resides for a given number of turns
class SkipAction(CellAction):
    def __init__(self, _turns):
        self.turns = _turns

    def execute_action(self, player):
        print("Player: ", player.name, " got penalty for ", self.turns)
        player.turns_to_skip = self.turns

    def __str__(self):
        return "Type: {}\nTurns_to_Skip: {}".format("skip", self.turns)


# Upon activation decreases player's credits for a given amount
class DropAction(CellAction):
    def __init__(self, _value):
        self.drop_value = _value

    def execute_action(self, player):
        player.credit -= self.drop_value
        print("Player: ", player.name, " lost ", self.drop_value, " drops to ", player.credit)
        if (player.credit < 0):
            player.playing = False
            if(player.game.termination[0] == 'firstbroke'):
                player.game.game_over = True

    def __str__(self):
        return "Type: {}\nCredits_to_Drop: {}".format("drop", self.drop_value)


# Upon activation draws a card from the deck of cards
class DrawAction(CellAction):
    def __init__(self):
        pass

    def execute_action(self, player):
        print("Time to draw a card for ", player.name)
        player.turn(['drawcard', ''])

    def __str__(self):
        return "Type: {}".format("draw")


# Upon activation increases player's credits for a given amount
class AddAction(CellAction):
    def __init__(self, _value):
        self.credit_to_add = _value

    def execute_action(self, player):
        player.credit += self.credit_to_add
        print("Player: ", player.name, " gains ", self.credit_to_add, " raises to ", player.credit)

    def __str__(self):
        return "Type: {}\nCredits_to_Add: {}".format("add", self.credit_to_add)


# Upon activation player pays given amount of credit to a player with
# given id
class PayAction(CellAction):
    def __init__(self, _targetId, _value):
        self.credit_to_pay = _value
        self.target_id = _targetId

    def execute_action(self, player):
        if(self.target_id == -1):
            self.target_id = player.id
            print("Player ", player.id, " purchases the land")
            return
        player.game.players[self.target_id][0].credit += self.credit_to_pay
        player.credit -= self.credit_to_pay
        if(player.credit < 0):
            player.playing = False
            if(player.game.termination[0] == 'firstbroke'):
                player.game.game_over = True
        print("Player ", player.id, " pays ", self.credit_to_pay, " to ", self.target_id)

    def __str__(self):
        return "Type: {}\nCredit_to_Pay: {}\nTarget: {}".format("pay", self.credit_to_pay, self.target_id)


# Factory class for actions
# Depending on given data it creates an action and returns it
class ActionCreator:
    def __init__(self):
        pass

    def create_action(self, data):
        if (type(data) is not dict):
            return DrawAction()
        else:
            k = list(data.keys())
            if (k[0] == 'jump'):
                if (type(data[k[0]]) is int):
                    return JumpAction(True, data[k[0]])
                else:
                    sliced = data[k[0]][::-1]
                    absolute_val = 0
                    for ch in sliced:
                        if (ch != '='):
                            absolute_val += int(ch) * pow(10, 0)
                    return JumpAction(False, absolute_val)
            elif (k[0] == 'skip'):
                return SkipAction(data[k[0]])
            elif (k[0] == 'drop'):
                return DropAction(data[k[0]])
            elif (k[0] == 'add'):
                return AddAction(data[k[0]])
            elif (k[0] == 'pay'):
                return PayAction(data[k[0]][0], data[k[0]][1])


# Cell class
# Contains cell information along with the CellAction/Artifact
# Upon execute calls CellAction/Artifact's execute_action()
class Cell:
    def __init__(self, _cellId, _description, _content, _is_artifact):
        self.cellID = _cellId
        self.description = _description
        self.content = _content
        self.is_artifact = _is_artifact

    def execute(self, player):
        self.content.execute_action(player)

    def __str__(self):
        return "ID: {}|Description: {}|Content: {}\n".format(self.cellID, self.description, str(self.content))

# Card class
# Contains general card information along with the action
# Upon play_card call the underlying action's execute_action
# method will be called
class Card:
    def __init__(self, _cardId, _description, _action):
        self.cardID = _cardId
        self.description = _description
        self.action = _action
    def play_card(self, player):
        self.action.execute_action(player)

    def __str__(self):
        return "ID: {}|Description: {}|Action: {}\n".format(self.cardID, self.description, str(self.action))

# Main Game Class
class Game:
    all_games = []
    g_id = 0
    def __init__(self, config, sequence=[]):
        # Game Description
        self.name = ""  # Name of the game
        self.dice = 0  # Maximum dice can be rolled
        self.cycles = 0  # Is map contains cycles
        self.termination = ["", -1]  # Termination method
        self.cells = []  # Cells contained in the game
        self.cards = []  # Cards that can be drawn

        self.id = Game.g_id
        Game.g_id += 1
        self.config = config
        ####

        # Player Related
        self.players = {}  # Dictionary to hold all players
        self.all_players_ready = False  # Is all players ready?
        self.number_of_ready_players = 0  # Number of ready players
        self.number_of_players = 0  # Number of players joined to the game

        self.start_with_credit = 0 # The amount of credits the player starts with
        self.next_player_id = 0 # The network id of the next player to join

        self.connections = {} # Dictionary to hold connection and corresponding network ids
        ####

        self.winner = None
        self.turn_number = 0  # Current turn number
        self.v_turn_number = 0
        self.game_over = False  # Is game over
        self.is_game_started = False

        self.sequence = sequence # Given fixed sequence
        self.sequence_number = 0

        self.player_ids = {} # Mapping username to player id
        self.turn_seq = 0 # Which player is playing the turn
        self.seq_lock = threading.Semaphore(1)

        self.w_for_artifact = False

        Game.all_games.append(self)  # Append this game to the game list to later retrieve them by calling listgames()

        # Parse the json file
        self.parse(config)  # Parse given json file

    def join_helper(self, _name):
        pl = Player(self.next_player_id, _name) # Create a player
        self.player_ids[_name] = self.next_player_id
        result = self.join(pl) # Join
        if(result == None): # If joins then returns the thread for this player
            return False
        return True

    # Reset game variables
    def game_reset(self):
        self.name = ""  # Name of the game
        self.dice = 0  # Maximum dice can be rolled
        self.cycles = 0  # Is map contains cycles
        self.termination = ["", -1]  # Termination method
        self.cells = []  # Cells contained in the game
        self.cards = []  # Cards that can be drawn

        # Player Related
        self.players = {}  # Dictionary to hold all players
        self.all_players_ready = False  # Is all players ready?
        self.number_of_ready_players = 0  # Number of ready players
        self.number_of_players = 0  # Number of players joined to the game
        self.next_player_id = 0 # The network id of the next player to join
        self.start_with_credit = 0 # The amount of credits the player starts with
        ####

        self.winner = None
        self.turn_number = 0  # Current turn number
        self.v_turn_number = 0
        self.game_over = False  # Is game over
        self.is_game_started = False

        self.sequence_number = 0

        self.w_for_artifact = False
        self.player_ids = {} # Mapping username to player id
        self.turn_seq = 0 # Which player is playing the turn
        self.seq_lock = threading.Semaphore(1)

        self.parse(self.config)

    # Given player joins the game
    def join(self, player):
        print("Join to the game: " + self.name)
        if (self.number_of_ready_players > 0):
            print("No more players can join after any player is ready!")
            return None
        self.players[player.id] = [player, False]
        self.players[player.id][0].is_in_room = True
        self.players[player.id][0].game=self
        self.number_of_players += 1
        self.next_player_id += 1
        player.reset()
        return True

    # Player marks itself ready
    def ready(self, player):
        print("Player: " + player.name + " is ready on game: " + self.name)
        if (player.id not in self.players):
            raise NotImplementedError
        if (not self.players[player.id][1]):
            self.number_of_ready_players += 1
            self.players[player.id][1] = True
            if (self.number_of_ready_players == len(self.players)):  # If all players ready then reset players' information then start
                # for k, v in self.players.items():
                #    self.players[k][0].reset()
                self.is_game_started = True
                m_loop = threading.Thread(target=self.start_game) # Start the game loop
                m_loop.start()


    # Get player information
    def get_player_info(self):
        res = []
        for k, v in self.players.items():
            res.append(str(v[0]))
        return res

    @staticmethod
    def listgames():  # Returns all games name along with its state as a string
        res = []
        for game in Game.all_games:
            res.append(game.show_lobby_info())
        return res

    # Returns game state
    def state(self):
        return str(self)

    # Executes player action
    def next(self, player):
        if(player.turns_to_skip > 0):
            player.turns_to_skip -= 1
        elif (player.draw):
            drawn_card = self.cards[random.randrange(0, len(self.cards))]
            drawn_card.play_card(player)
            player.draw = False
        else:
            player.current_cell_no += player.rolled  # Increase player cell by rolled dice
            self.on_player_move(player)  # Call move event
        return str(player)  # Return players' new state information

    # Player calls upon seeing an artifact
    def pick(self, player, pickBool):
        if (pickBool):
            self.cells[player.current_cell_no].content.take_artifact(player,self.cells)
            if (self.termination[0] == 'firstcollect' and player.owned_artifact_count == self.termination[1]):
                self.winner = player
                self.game_over = True
            elif(self.termination[0] == 'firstbroke' and player.credit < 0):
                self.game_over = True
        return str(player)

    # Upon start called
    def start_game(self):
        while (not self.game_over):  # Main game loop

            self.seq_lock.acquire()
            self.turn_seq = self.v_turn_number % len(self.players)
            self.seq_lock.release()

            pl = self.players[self.v_turn_number % len(self.players)][0]  # Select player to play this turn
            if(pl.playing == False):
                self.v_turn_number += 1
                continue
            pl.turn(['roll'])
            if (not pl.started):  # If this is player's first turn
                pl.started = True
            self.turn_number += 1  # Increase the turn number
            self.v_turn_number += 1
            self.w_for_artifact = False

        print("Game finishes")
        for k, v in self.players.items(): # Send termination command to every player in the game
            v[0].playing = False


        time.sleep(6)
        print("Resetting the game")
        self.game_reset()

    # Called upon any move player makes
    def on_player_move(self, player):
        if (self.cycles or type(self.cycles) == int):  # If cycles
            if (player.current_cell_no >= len(self.cells)):  # If we exceed the length of the track increase the round count
                if(type(self.cycles) == int):
                    player.credit += self.cycles
                player.finished_rounds += 1
                player.current_cell_no -= len(self.cells)
            elif (player.current_cell_no < 0):
                player.finished_rounds -= 1
                player.current_cell_no += len(self.cells)
        elif (player.current_cell_no >= len(self.cells) - 1 and self.termination[0] == 'finish'):  # If termination is finish
            self.game_over = True
            self.winner = player
            return
        elif (player.current_cell_no >= len(self.cells) - 1):  # If reaches the end player waits here
            player.current_cell_no = len(self.cells) - 1
            player.finished = True

        if (self.termination[0] == 'round' and player.finished_rounds >= self.termination[1]):  # If completed n rounds finish the game
            self.game_over = True
            self.winner = player

        if ( self.cells[player.current_cell_no].is_artifact):  # If current cell contains an artifact
            print("I am on artifact")
            player.turn(['choice', self.cells[player.current_cell_no].content])  # Choice command is sent
        self.cells[player.current_cell_no].execute(player) # Execute the action on the player

    # Parse the config
    def parse(self, config):
        factory = ActionCreator()  # Create a factory
        parsed_dict = json.loads(config)  # Turn json into python dictionary
        for k, v in parsed_dict.items():  # Iterate over the dictionary
            if (k == "name"):
                self.name = v
            elif (k == "dice"):
                self.dice = v
            elif (k == "cycles"):
                self.cycles = v
            elif (k == "credit"):
                self.start_with_credit = v
            elif (k == "termination"):
                if (type(v) is dict):
                    for l, e in v.items():
                        self.termination[0] = l
                        self.termination[1] = e
                else:
                    self.termination[0] = v
            elif (k == "cells"):
                for cell in v:
                    if ("artifact" not in cell):  # If cell does not contain an artifact
                        if ("action" not in cell):  # If cell does not contain an action
                            self.cells.append(Cell(cell['cellno'], cell['description'], EmptyContent(), False))
                        else:
                            self.cells.append(
                                Cell(cell['cellno'], cell['description'], factory.create_action(cell['action']), False))
                    else:
                        temp = cell['artifact']  # Take artifact
                        if ("action" not in cell['artifact']):  # If artifact does not contain an action
                            self.cells.append(Cell(cell['cellno'], cell['description'],
                                                   Artifact(temp['name'], temp['owned'], temp['price'], EmptyContent()),
                                                   True))
                        else:
                            self.cells.append(Cell(cell['cellno'], cell['description'],
                                                   Artifact(temp['name'], temp['owned'], temp['price'],
                                                            factory.create_action(cell['artifact']['action'])), True))
            elif(k == "cards"):
                for card in v:
                    self.cards.append(Card(card['cardno'], card['description'], factory.create_action(card['action'])))

    def __str__(self):  # Overload string representation of the game
        result = "Turn Number: {}\n".format(self.turn_number)  # Turn Number
        result += "Game Over: {}\n".format(self.game_over)
        self.seq_lock.acquire()
        result += "Waiting for Player: {}\n".format(self.turn_seq)
        self.seq_lock.release()
        if(self.w_for_artifact):
            result += "Waiting for pick answer\n"
        else:
            result += "Waiting for roll answer\n"
        if (self.winner is not None):
            result += "Winner: {}\n".format(self.winner.name)
        result += "---Cells---\n"
        for cell in self.cells:  # Get all cell description
            result += str(cell)
        result += "---Players---\n"
        for k, v in self.players.items():  # Get player description
            result += str(v[0])
        return result

    def show_lobby_info(self):  # Upon listing the games lobby information will be written
        return "Game Name: {}\nGame ID: {}\nState:\n {}".format(self.name, self.id, self.state())

# Player reperesenter
class Player:
    def __init__(self, _id, _name):
        self.id = _id # Network ID of the player
        self.name = _name # Name of the player
        self.connection_id = -1
        self.is_in_room = False # Is in the game room
        self.game=None # Current game

        self.playing = True # Still in the game?

        self.waiting_for_roll_answer = False # Waiting for an answer from client for roll
        self.waiting_for_pick_answer = False # Waiting for answer from client for pick
        self.pick_lock = threading.Semaphore(0) # Lock to control the synchronization of actions that is dependent on client input

    # Reset player information upon starting a new game
    def reset(self):
        self.credit = self.game.start_with_credit
        self.turns_to_skip = 0
        self.owned_artifact_count = 0
        self.current_cell_no = 0
        self.finished_rounds = 0
        self.rolled = 0
        self.draw = False
        self.artifacts = []
        self.started = False
        self.finished = False
        self.playing = True

        self.waiting_for_roll_answer = False # Waiting for an answer from client for roll
        self.waiting_for_pick_answer = False # Waiting for answer from client for pick
        self.pick_lock = threading.Semaphore(0) # Lock to control the synchronization of actions that is dependent on client input


    # Join to a target game
    def join(self, game):
        if (not self.is_in_room):
            game.join(self)

    # Make state ready
    def ready(self):
        if (self.game is not None):
            self.game.ready(self)

    # Respond with next call to the game
    # depending on the action type
    def turn(self, _type):
        res = ''
        if (_type[0] == "roll"):  # Roll a dice
            self.waiting_for_roll_answer = True
            self.pick_lock.acquire()
            if (self.turns_to_skip > 0):
                # res = self.game.next(self)
                pass
            elif (len(self.game.sequence) != 0):
                self.rolled = self.game.sequence[self.game.sequence_number]
                self.game.sequence_number+=1
            else:
                self.rolled = random.randrange(1, self.game.dice + 1)
            res = self.game.next(self)
        elif (_type[0] == "drawcard"):  # Draw a card
            self.draw = True
            res = self.game.next(self)
        elif (_type[0] == "choice"):  # There is an artifact pick?
            self.game.w_for_artifact = True
            self.waiting_for_pick_answer = True
            self.pick_lock.acquire()

            self.target_artifact = _type[1]
            if(self.should_pick == 0):
                res = self.game.pick(self, False)
            elif(self.should_pick == 1):
                res = self.game.pick(self, True)

    # String representation of the player
    # Returns player description
    def __str__(self):
        return "ID: {}\nNickname: {}\nPlaying: {}\nCellNo: {}\nCredits: {}\nArtifact Count: {}\nFinished Rounds: {}\nTurns to Skip: {}\n".format(
            self.id, self.name, self.playing, self.current_cell_no, self.credit, self.owned_artifact_count, self.finished_rounds, self.turns_to_skip)



def create_game(config, seq=[]):
    Game(config, seq)
