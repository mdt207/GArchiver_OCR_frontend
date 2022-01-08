
CC = g++

main: main.cpp 
#	$(CC) -Wall -std=c++17 main.cpp utils.cpp `pkg-config vips-cpp gtk+-3.0 --cflags --libs` -o region_of_interest
#ctasnif.o: ctasnif.cpp
#	$(CC) `pkg-config gtk+-2.0 --cflags` -c ctasnif.cpp
clean: 
	rm -f *.o main
