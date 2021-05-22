CPPFLAGS=-std=c++11 -I/home/hstecher/local/include -I/usr/include/json-c12 -Wall -g  -Werror 
LDFLAGS=-L/home/hstecher/local/lib -g -Wl,-rpath=/home/hstecher/local/lib 
LDLIBS=-lCArcDevice -lCArcDeinterlace -lCArcFitsFile -lcfitsio -ljson-c12 -luuid -lpthread

TARGETS=testing continuous unified # rtest ptest
CPPDEPS=libgnirs.cpp
HDEPS=libgnirs.h

all: $(TARGETS)

%: %.cpp $(CPPDEPS) $(HDEPS)
	g++ $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) $< $(CPPDEPS) -o $@

clean:
	rm -f $(TARGETS)
