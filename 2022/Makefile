
all: 3 6 9 14

3.o: 3.s
	$(AS) -o $@ $<

3: 3.o
	$(LD) -o $@ $<

# PHONY since it always fails to compile.
.PHONY: 5
5: 5.cpp
	$(CXX) -std=c++14 -o /dev/null $<

6: 6.o
	$(CC) -static -nostartfiles -nostdlib -o $@ $<

6.o: 6.c
	$(CC) -Wall -Wextra -Os -fno-stack-protector -g -march=native -c -o $@ $<

9: 9.cr
	crystal build -o $@ $<

14: 14.zig
	zig build-exe -O ReleaseFast $<

.PHONY: clean
clean:
	$(RM) 3 3.o 6 6.o 9 14 14.o
	$(RM) -r zig-cache
