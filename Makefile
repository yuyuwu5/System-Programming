all: bidding_system.c host.c player.c
	gcc bidding_system.c -o bidding_system
	gcc player.c -o player
	gcc host.c -o host

clean:
	rm -f bidding_system player host
