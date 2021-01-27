CPPFLAGS=-std=c++11 -I/home/rcardene/local/include -Wall -Werror
LDFLAGS=-L/home/rcardene/local/lib -Wl,-rpath=/home/rcardene/local/lib
LDLIBS=-lCArcDevice -lCArcDeinterlace -lCArcFitsFile -lcfitsio

TARGETS=testing # rtest ptest

all: $(TARGETS)

testing: testing.cpp libgnirs.cpp libgnirs.h
	g++ $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) libgnirs.cpp testing.cpp -o testing

clean:
	rm -f $(TARGETS)
