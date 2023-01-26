OBJS	= ../F_Extras/Utilities.o Atreides.o
SOURCE	= ../F_Extras/Utilities.c Atreides.c
HEADER	= ../F_Extras/Utilities.h Atreides.h ../F_Extras/Error.h
OUT	= ../Atreides
CC	 = gcc
FLAGS	 = -g -c -Wall -Wextra 
LFLAGS	 = -lpthread

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

Utilities.o: ../F_Extras/Utilities.c
	$(CC) $(FLAGS) ../F_Extras/Utilities.c 

Atreides.o: Atreides.c
	$(CC) $(FLAGS) Atreides.c 

clean:
	rm -f $(OBJS) $(OUT)