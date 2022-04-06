$(shell echo -e "#include \"version.h\"\n\nchar const *const GIT_COMMIT = \"$$(git describe --tags --long)\";" > version.cpp.tmp; if diff -q version.cpp.tmp version.cpp >/dev/null 2>&1; then rm version.cpp.tmp; else mv version.cpp.tmp version.cpp; fi)


CPPFLAGS=-std=c++11 -I./include -I/usr/include/json-c12 -Wall  -Werror 
LDFLAGS=-L./lib  -Wl,-rpath=/lib 
LDLIBS=-lCArcDevice -lCArcDeinterlace -lCArcFitsFile -lcfitsio -ljson-c12 -luuid -lpthread

TARGETS=libgnirsioc.o libgnirsioc.a version.o libgnirs.o 
CPPDEPS=libgnirs.cpp
HDEPS=libgnirs.h libgnirsioc.h

all: $(TARGETS)

libgnirsioc.a: libgnirsioc.o libgnirs.o version.o 
	ar -r -c -s $@ $^

%: %.cpp $(CPPDEPS) $(HDEPS)
	g++ $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) $< $(CPPDEPS) -o $@

install:
	mkdir ./release
	install -m 644 libgnirsioc.a ./release/
	install -m 644 libgnirsioc.h ./release/
	cp ./release/* ../gem_test/gnirsdc/gnirsDCApp/src/

clean:
	rm -f $(TARGETS)
	rm -rf ./release
