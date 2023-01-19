CC = g++
FLAGS = -w -std=c++17 -g -o

INCLUDE = -I/usr/X11R6/include -I/usr/include/GL -I/usr/include -I ./
LIBDIR = -L/usr/X11R6/lib -L/usr/local/lib
LIBS = -lGLEW -lGL -lGLU -lglut -lm


test: test.cpp
	$(CC) $(FLAGS) smooth $(INCLUDE) $(LIBDIR) smooth.cpp $(LIBS)

smooth: smooth.cpp
	$(CC) $(FLAGS) smooth $(INCLUDE) $(LIBDIR) smooth.cpp $(LIBS)

clean:
	rm -f *.o smooth

all: clean smooth

.PHONY: clean
