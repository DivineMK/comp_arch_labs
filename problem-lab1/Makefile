SRC     := $(wildcard src/*.c)
INPUT   ?= $(wildcard inputs/*/*.x)
CFLAGS  ?= -g -O2
DEFINES ?=

.PHONY: all verify clean

all: sim

sim: $(SRC)
	gcc $(CFLAGS) $(DEFINES) $^ -o $@

basesim: $(SRC)
	gcc $(CFLAGS) $(DEFINES) $^ -o $@

run: sim
	@python run.py $(INPUT)

clean:
	rm -rf *.o *~ sim basesim
