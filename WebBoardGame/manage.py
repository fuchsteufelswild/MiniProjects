#!/usr/bin/env python
"""Django's command-line utility for administrative tasks."""
import os
import sys
from BoardGame import game


s1 = """ { "name": "Test All Actions",
"dice" :6,
"cycles": 3,
"termination": {"round": 5},
"cells" : [ {"cellno" : 0, "description" : "Start"},
{"cellno" : 1, "description": "", "action": "drawcard"},
{"cellno" : 2, "description" : "Bug", "action": {"skip": 1} },
{"cellno" : 3, "description" : "New version", "action": {"jump": 3} },
{"cellno" : 4, "description" : "Runtime error", "action": {"jump": "=0"} },
{"cellno" : 5, "description" : "", "action": {"drop": 4}},
{"cellno" : 6, "description" : "New IDE",
"artifact": {"name": "ide", "owned": false,
"price": 0, "action": {"add": 10}}},
{"cellno" : 7, "description" : "", "action": {"pay": [-1, 4]} }],
"cards": [{"cardno" : 0, "description" : "KG", "action" : {"jump": 3 } }] } """

s2 = """ { "name": "Test First Broke",
"dice" :6,
"cycles": 3,
"termination": "firstbroke",
"cells" : [ {"cellno" : 0, "description" : "Start"},
{"cellno" : 1, "description": "", "action": "drawcard"},
{"cellno" : 2, "description" : "Bug", "action": {"skip": 1} },
{"cellno" : 3, "description" : "New version", "action": {"jump": 3} },
{"cellno" : 4, "description" : "Runtime error", "action": {"jump": "=0"} },
{"cellno" : 5, "description" : "", "action": {"drop": 4}},
{"cellno" : 6, "description" : "New IDE",
"artifact": {"name": "ide", "owned": false,
"price": 0, "action": {"add": 10}}},
{"cellno" : 7, "description" : "", "action": {"pay": [-1, 4]} }],
"cards": [{"cardno" : 0, "description" : "KG", "action" : {"jump": 3 } }] } """

s3 = """ { "name": "Test Finish",
"dice" :6,
"cycles": false,
"termination": "finish",
"cells" : [ {"cellno" : 0, "description" : "Start"},
{"cellno" : 1, "description": "", "action": "drawcard"},
{"cellno" : 2, "description" : "Bug", "action": {"skip": 1} },
{"cellno" : 3, "description" : "New version", "action": {"jump": 3} },
{"cellno" : 4, "description" : "Runtime error", "action": {"jump": "=0"} },
{"cellno" : 5, "description" : "", "action": {"drop": 4}},
{"cellno" : 6, "description" : "New IDE",
"artifact": {"name": "ide", "owned": false,
"price": 0, "action": {"add": 10}}},
{"cellno" : 7, "description" : "", "action": {"pay": [-1, 4]} }],
"cards": [{"cardno" : 0, "description" : "KG", "action" : {"jump": 3 } }] } """

s4 = """ { "name": "Test Artifacts and FirstCollect",
"dice" :6,
"cycles": true,
"credit": 4,
"termination": {"firstcollect": 3},
"cells" : [ {"cellno" : 0, "description" : "Start"},
{"cellno" : 1, "description": "",
"artifact" : {"name": "a1", "owned": true, "price": 2}},
{"cellno" : 2, "description" : "Bug",
"artifact": {"name": "a2", "owned": true, "price": 10} },
{"cellno" : 3, "description" : "New version" },
{"cellno" : 4, "description" : "Runtime error",
"artifact": {"name": "a3", "owned": true, "price": 4, "action": {"add": 10} } },
{"cellno" : 5, "description" : "", "action": {"add": 20}},
{"cellno" : 6, "description" : "New IDE"},
{"cellno" : 7, "description" : "",
"artifact": {"name": "a4", "owned": true, "price": 3, "action": {"jump": 3}} },
{"cellno": 8, "description": ""} ],
"cards": [{"cardno" : 0, "description" : "KG", "action" : {"jump": 3 } }] } """

s5 = """ { "name": "Negative Credit Player Down",
"dice" :6,
"cycles": 3,
"termination": {"round": 5},
"cells" : [ {"cellno" : 0, "description" : "Start"},
{"cellno" : 1, "description": "", "action": "drawcard"},
{"cellno" : 2, "description" : "Bug", "action": {"skip": 1} },
{"cellno" : 3, "description" : "New version", "action": {"jump": 3} },
{"cellno" : 4, "description" : "Runtime error", "action": {"jump": "=0"} },
{"cellno" : 5, "description" : "", "action": {"drop": 4}},
{"cellno" : 6, "description" : "New IDE",
"artifact": {"name": "ide", "owned": false,
"price": 0, "action": {"add": 10}}},
{"cellno" : 7, "description" : "", "action": {"pay": [-1, 4]} }],
"cards": [{"cardno" : 0, "description" : "KG", "action" : {"jump": 3 } }] } """

seq1 = [1, 0, 2, 0, 0, 1, 0, 1, 0, 6, 0, 4, 0, 1, 0] # Test for all actions and the termination of the game with cycles = True
seq2 = [5, 0, 1, 0] # First broke s2
seq3 = [3, 0, 2]          # Finish s3
seq4 = [2, 4, 3, 1, 2, 1, 1, 2] # First collect s4
seq5 = [5, 1, 1, 1, 1, 1] # Negative credit game over for player

def main():
    game.create_game(s1, seq1)
    game.create_game(s2, seq2)
    game.create_game(s3, seq3)
    game.create_game(s4, seq4)
    game.create_game(s5, seq5)

    os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'Phase3.settings')
    try:
        from django.core.management import execute_from_command_line
    except ImportError as exc:
        raise ImportError(
            "Couldn't import Django. Are you sure it's installed and "
            "available on your PYTHONPATH environment variable? Did you "
            "forget to activate a virtual environment?"
        ) from exc
    execute_from_command_line(sys.argv)


if __name__ == '__main__':
    main()
