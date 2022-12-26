
all: day3 day6 day14

day3.o: day3.s
	$(AS) -o $@ $<

day3: day3.o
	$(LD) -o $@ $<

# PHONY since it always fails to compile.
.PHONY: day5
day5: day5.cpp
	$(CXX) -std=c++14 -o /dev/null $<

day6: day6.o
	$(CC) -static -nostartfiles -nostdlib -o $@ $<

day6.o: day6.c
	$(CC) -Wall -Wextra -Os -fno-stack-protector -g -march=native -c -o $@ $<

day14: day14.zig
	zig build-exe -O ReleaseFast $<

.PHONY: clean
clean:
	$(RM) day3 day3.o day6 day6.o day14 day14.o
	$(RM) -r zig-cache
