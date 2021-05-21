CPPFLAGS=-std=c++11 -I/home/hstecher/local/include -I/usr/include/json-c12 -Wall -Werror
LDFLAGS=-L/home/hstecher/local/lib -Wl,-rpath=/home/hstecher/local/lib
LDLIBS=-lCArcDevice -lCArcDeinterlace -lCArcFitsFile -lcfitsio -ljson-c12 -luuid -lpthread

TARGETS=testing continuous # rtest ptest
CPPDEPS=libgnirs.cpp
HDEPS=libgnirs.h

all: $(TARGETS)

%: %.cpp $(CPPDEPS) $(HDEPS)
	g++ $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) $< $(CPPDEPS) -o $@

clean:
	rm -f $(TARGETS)
