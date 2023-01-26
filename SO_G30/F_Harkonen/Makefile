OBJS	= ../F_Extras/Utilities.o Harkonen.o
SOURCE	= ../F_Extras/Utilities.c Harkonen.c
HEADER	= ../F_Extras/Utilities.h Harkonen.h ../F_Extras/Error.h
OUT	= ../Harkonen
CC	 = gcc
FLAGS	 = -g -c -Wall -Wextra 
LFLAGS	 =

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

Utilities.o: ../F_Extras/Utilities.c
	$(CC) $(FLAGS) ../F_Extras/Utilities.c 

Harkonen.o: Harkonen.c
	$(CC) $(FLAGS) Harkonen.c 

clean:
	rm -f $(OBJS) $(OUT)
