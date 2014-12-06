CC = gcc
FLAGS = -Wall -pedantic

.PHONY: clean play

play: game
	./game

game: kbhit.o DaBattle.c
	$(CC) $(FLAGS) $^ -o $@

debug: kbhit.o DaBattle.c
	$(CC) $(FLAGS) -DDEBUG $^ -o $@

kbhit.o: kbhit.c kbhit.h
	$(CC) $(FLAGS) -c $<

clean:
	rm -f game debug kbhit.o