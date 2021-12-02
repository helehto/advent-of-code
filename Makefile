FILES := $(patsubst %.S, %, $(wildcard *.S))

all: $(FILES)

clean:
	$(RM) $(FILES)

.PHONY: all clean
