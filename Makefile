
CC = g++

main: main.cpp 
#	$(CC) `pkg-config gtk+-3.0 --cflags` `pkg-config vips --cflags` main.cpp Image.cpp -Wall -O2 -std=c++17 -o region_of_interest `pkg-config gtk+-3.0 --libs` `pkg-config vips --libs`
#	$(CC) -g -Wall -std=c++17 main.cpp Image.cpp `pkg-config vips gtk+-3.0 --cflags --libs` -o region_of_interest
#	$(CC) -g -Wall -std=c++17 main.cpp Image.cpp `pkg-config vips-cpp gtk+-3.0 --cflags --libs` -o region_of_interest
	$(CC) -g -Wall -std=c++17 main.cpp utils.cpp `pkg-config vips-cpp gtk+-3.0 --cflags --libs` -o region_of_interest
#ctasnif.o: ctasnif.cpp
#	$(CC) `pkg-config gtk+-2.0 --cflags` -c ctasnif.cpp
clean: 
	rm -f *.o main
