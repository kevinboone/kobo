VERSION := 0.0.1
CC      := gcc
CFLAGS  := -Wall -DVERSION=\"$(VERSION)\" -g -I include
LIBS    := 
INCLUDE := /usr/include/kmediascanner
DESTDIR := /usr
TARGET	:= kobo 
SOURCES := $(shell find src/ -type f -name *.c)
OBJECTS := $(patsubst src/%,build/%,$(SOURCES:.c=.o))
DEPS	:= $(OBJECTS:.o=.deps)

all: $(TARGET)

$(TARGET): $(OBJECTS) 
	echo $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LIBS) 

build/%.o: src/%.c
	@mkdir -p build/
	$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -c -o $@ $<

clean:
	$(RM) -r build/ $(TARGET)

install: $(TARGET)
	cp -p $(TARGET) ${DESTDIR}${PREFIX}/bin/

-include $(DEPS)

.PHONY: clean
