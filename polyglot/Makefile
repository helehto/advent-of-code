
all: day3

day3.o: day3.s
	$(AS) -o $@ $<

day3: day3.o
	$(LD) -o $@ $<

# PHONY since it always fails to compile.
.PHONY: day5
day5: day5.cpp
	$(CXX) -std=c++14 -o /dev/null $<

.PHONY: clean
clean:
	$(RM) day3 day3.o
