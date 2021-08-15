$(shell echo -e "#include \"version.h\"\n\nchar const *const GIT_COMMIT = \"$$(git describe --tags --long)\";" > version.cpp.tmp; if diff -q version.cpp.tmp version.cpp >/dev/null 2>&1; then rm version.cpp.tmp; else mv version.cpp.tmp version.cpp; fi)


CPPFLAGS=-std=c++11 -I/home/hstecher/local/include -I/usr/include/json-c12 -Wall  -Werror 
LDFLAGS=-L/usr/local/lib/arc  -Wl,-rpath=/usr/local/lib/arc 
LDLIBS=-lCArcDevice -lCArcDeinterlace -lCArcFitsFile -lcfitsio -ljson-c12 -luuid -lpthread

TARGETS=testing continuous unified libgnirsioc.o libgnirsioc.a version.o  # rtest ptest
CPPDEPS=libgnirs.cpp
HDEPS=libgnirs.h libgnirsioc.h

all: $(TARGETS)

libgnirsioc.a: libgnirsioc.o libgnirs.o version.o 
	ar -r -c -s $@ $^

%: %.cpp $(CPPDEPS) $(HDEPS)
	g++ $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) $< $(CPPDEPS) -o $@

install:
	install -m 644 libgnirsioc.a /home/hstecher/work/gem_test/gnirsdc/gnirsDCApp/src/
	install -m 644 libgnirsioc.h /home/hstecher/work/gem_test/gnirsdc/gnirsDCApp/src/

clean:
	rm -f $(TARGETS)
