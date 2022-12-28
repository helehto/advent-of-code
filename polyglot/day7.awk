#!/bin/awk -f

BEGIN {
	cwd = "/";
}

/cd [^/].*/ {
	if ($3 != "..") {
		cwd = (cwd != "/" ? cwd : "") "/" $3
	} else {
		gsub(/\/[^/]+$/, "", cwd)
		if (!cwd) {
			cwd = "/"
		}
	}
}

/[[:digit:]]+ .*/ {
	if (cwd == "/") {
		sizes[""] += $1
		next
	}

	split(cwd, fields, "/")
	acc = ""
	for (d in fields) {
		acc = fields[d] ? acc "/" fields[d] : acc;
		sizes[acc] += $1
	}
}

END {
	unused = 70000000 - sizes[""]
	needed = 30000000 - unused

	part2 = sizes[""]
	for (d in sizes) {
		size = sizes[d]
		if (size <= 100000) {
			part1 += size
		}
		if (size < part2 && size >= needed) {
			part2 = size
		}
	}

	print part1
	print part2
}
