OBJS	= Fremen.o ../F_Extras/Utilities.o
SOURCE	= Fremen.c ../F_Extras/Utilities.c
HEADER	= Fremen.h ../F_Extras/Utilities.h ../F_Extras/Error.h 
OUT	= ../Fremen
CC	 = gcc
FLAGS	 = -g -c -Wall -Wextra 
LFLAGS	 = 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)
	
Fremen.o: Fremen.c
	$(CC) $(FLAGS) Fremen.c 

Utilities.o: ../F_Extras/Utilities.c
	$(CC) $(FLAGS) ../F_Extras/Utilities.c

clean:
	rm -f $(OBJS) $(OUT)