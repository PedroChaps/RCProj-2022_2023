# RCProj-2022_2023

This is the project submission of the group 31, in the RC class of the academic year 2022-2023.  

99181 - António Afonso  
99298 - Pedro Chaparro

## Structure
The file structure is as follows:
```
.
├── client
│   ├── files
│   │   ├── monotremata.jpg
│   │   ├── scoreboard.txt
│   │   ├── SP_vac.jpg
│   │   └── state_099298.txt
│   └── player.c
├── README.md
└── server
    ├── GAMES
    │   ├── 099181
    │   │   ├── 20221219_221841_F.txt
    │   │   └── 20221222_003950_W.txt
    │   ├── 099182
    │   │   ├── 20221219_221744_F.txt
    │   │   └── 20221222_003923_F.txt
    │   ├── 099298
    │   │   ├── 20221219_220320_F.txt
    │   │   ├── 20221219_221744_F.txt
    │   │   └── ...
    │   ├── curr_word_099298.txt
    │   ├── game_099298.txt
    │   └── state_099298.txt
    ├── GS.c
    ├── HINTS
    │   ├── 2D-struct.png
    │   ├── card.jpg
    │   └── ...
    ├── SCORES
    │   ├── 050_099181_20221222_003950.txt
    │   └── ...
    ├── server
    └── word_eng.txt
```

Where:
- `client` - has everything related to the client;
- `server` - has everything related to the server;
- `client/files` - has every file that the client can store. This includes the hints (Eg. `monotremata.jpg`, `SP_vac.jpg`, ...), the latest asked scoreboard (`scoreboard.txt`) and the latest asked state (Eg. `state_099298.txt`);
- `client/player.c` - the code for the player;
- `server/GS.c` -
- `server/word_eng.txt` - 
- `server/GAMES` - has directories for each player, that store the previous games for that player, as well as the active games for each player, storing information in `curr_word_PLID.txt`, `game_PLID.txt` and `state_PLID.txt`;
- `server/HINTS` - has the hint files that the servers sends to the user; 
- `server/SCORES` - has the scores of each game.

## How to run

The project includes a makefile that can be used to compile the project.  
The makefile has multiple targets:
- `make` - Runs all the following commands;
- `make clean` - Cleans the created files and kills the processes' Children;
- `make player` - Compiles `player.c`, to the output file `player`;
- `make server` - Compiles `server.c`, to the output file `GS`.

To run the server, open the directory `server/` and run `./GS word_file [-p GSport] [-v]`.  
By default, if no port is passed, the server will run in the port 58031 (58000 + GN).  

To run the client, open the directory `client/` and run `./player [-n GSIP] [-p GSport]`.  
By default, 
- if no IP is passed, the client will communicate to the local host (127.0.0.1);
- if no port is passed, the client will communicate to the port 58031 (58000 + GN).  


## Notes
- After the client process starts (`./client`), commands that use the player ID will fail, because the command `start PLID` needs to be called first to set a player ID.