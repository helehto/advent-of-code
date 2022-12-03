
all: day3

day3.o: day3.s
	$(AS) -o $@ $<

day3: day3.o
	$(LD) -o $@ $<

.PHONY: clean
clean:
	$(RM) day3 day3.o
