#!/usr/bin/perl
use 5.34.0;
use strict;
use warnings;

my %VALUES = (
	A => 0, B => 1, C => 2,
	X => 0, Y => 1, Z => 2,
);

my $p1 = 0;
my $p2 = 0;

while (my $line = <>) {
	my ($a, $b) = map $VALUES{$_}, split(/\s+/, $line);

	if (($a + 1) % 3 == $b) {
		$p1 += $b + 7;
	} elsif ($a == $b) {
		$p1 += $b + 4;
	} else {
		$p1 += $b + 1;
	}

	if ($b == 0) {
		$p2 += ($a + 2) % 3 + 1;
	} elsif ($b == 1) {
		$p2 += 3 + ($a % 3) + 1;
	} else {
		$p2 += 6 + ($a + 1) % 3 + 1;
	}
}

say $p1;
say $p2;
