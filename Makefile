$(shell echo -e "#include \"version.h\"\n\nchar const *const GIT_COMMIT = \"$$(git describe --tags --long)\";" > version.cpp.tmp; if diff -q version.cpp.tmp version.cpp >/dev/null 2>&1; then rm version.cpp.tmp; else mv version.cpp.tmp version.cpp; fi)

PYTHON_INCLUDES=$(shell python3-config --includes)

CPPFLAGS=-std=c++11 -I./include -I/usr/include/json-c $(PYTHON_INCLUDES) -Wall -fPIC

LDFLAGS=-L./lib  -Wl,-rpath=/lib -shared
LDLIBS=-lCArcDevice -lCArcDeinterlace -lCArcFitsFile -lcfitsio -ljson-c -luuid -lpthread

TARGETS=libgnirsioc.so version.o libgnirs.o install
CPPDEPS=libgnirs.cpp
HDEPS=libgnirs.h libgnirsioc.h

all: $(TARGETS)

libgnirsioc.so: libgnirsioc.o libgnirs.o version.o
	g++ $(CPPFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

libgnirsioc.o: libgnirsioc.cpp $(HDEPS)
	g++ $(CPPFLAGS) -c $< -o $@

libgnirs.o: libgnirs.cpp $(HDEPS)
	g++ $(CPPFLAGS) -c $< -o $@

# ... rest of the Makefile remains the same ...

install:
	mkdir -p ./release/include
	mkdir -p ./release/lib/linux-x86_64
	install -m 644 libgnirsioc.h ./release/include/
	install -m 755 libgnirsioc.so ./release/lib/linux-x86_64
	install -m 644 lib/* ./release/lib/linux-x86_64


uninstall:
	rm -f $(TARGETS)
	rm -rf ./release

distclean:
	rm -f $(TARGETS)
	rm -rf ./release

clean:
	rm -f $(TARGETS)
	rm -rf ./release
