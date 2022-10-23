DIRS     := src src/fillers src/PathDataParser src/PointsOnPath src/PointsOnCurve
SOURCES  := $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
OBJS     := $(patsubst %.cpp, %.o, $(SOURCES))
OBJS     := $(foreach o,$(OBJS),./bin/$(o))
DEPFILES := $(patsubst %.o, %.P, $(OBJS))

INCLUDE  = 
LIBS     = -lexpat
CFLAGS   = -std=c++20 -Wall -Os $(INCLUDE)
LFLAGS   = -Os $(LIBS)
COMPILER = g++

bin/sketchify: $(OBJS)
	$(COMPILER) $(OBJS) $(LFLAGS) -o bin/sketchify

#generate dependency information and compile
bin/%.o : %.cpp
	@mkdir -p $(@D)
	$(COMPILER) $(CFLAGS) -c -o $@ -MMD $<

#remove all generated files
.PHONY: clean
clean:
	rm -rf bin

.PHONY: example
example: bin/sketchify
	$< -o example/sample-out.svg example/sample-in.svg

install: bin/sketchify
	cp $<  /usr/local/bin/

#include the dependency information
-include $(DEPFILES)

