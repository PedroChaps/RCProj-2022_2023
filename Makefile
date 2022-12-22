all: server/GS client/player

GS: server/GS.c
	pkill GS; gcc server/GS.c -o GS

player: client/player.c
	gcc client/player.c -o player

clean:
	rm -f server/GS client/player


