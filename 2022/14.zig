const std = @import("std");

const Reservoir = struct {
    solid: std.AutoHashMap([2]i16, void),

    fn init() Reservoir {
        return Reservoir{
            .solid = std.AutoHashMap([2]i16, void).init(std.heap.page_allocator),
        };
    }

    fn step(self: *const Reservoir, sand: [2]i16) ?[2]i16 {
        var candidate: @TypeOf(sand) = undefined;

        candidate = .{ sand[0], sand[1] + 1 };
        if (!self.solid.contains(candidate)) {
            return candidate;
        }

        candidate = .{ sand[0] - 1, sand[1] + 1 };
        if (!self.solid.contains(candidate)) {
            return candidate;
        }

        candidate = .{ sand[0] + 1, sand[1] + 1 };
        if (!self.solid.contains(candidate)) {
            return candidate;
        }

        return null;
    }

    fn markSolid(self: *Reservoir, k: [2]i16) !void {
        try self.solid.put(k, void{});
    }

    fn solidCount(self: *const Reservoir) usize {
        return self.solid.unmanaged.size;
    }
};

pub fn main() !void {
    var stdout = std.io.getStdOut().writer();
    var reader = std.io.bufferedReader(std.io.getStdIn().reader());

    // Read from stdin.
    var buf: [16384]u8 = undefined;
    const len = try reader.read(&buf);
    const contents = buf[0..len];

    var reservoir = Reservoir.init();

    var parsed = std.ArrayList([2]i16).init(std.heap.page_allocator);
    defer parsed.deinit();

    var abyss: i16 = 0;

    var line_iter = std.mem.split(u8, contents, "\n");
    while (line_iter.next()) |line| {
        if (line.len == 0) {
            break;
        }

        // Parse the line.
        var pair_iter = std.mem.split(u8, line, " -> ");
        while (pair_iter.next()) |pair| {
            var coord_iter = std.mem.split(u8, pair, ",");
            const x_str = coord_iter.next() orelse unreachable;
            const y_str = coord_iter.next() orelse unreachable;

            try parsed.append(.{
                try std.fmt.parseInt(i16, x_str, 10),
                try std.fmt.parseInt(i16, y_str, 10),
            });
        }

        // Set up the reservoir walls for this input line.
        var i: usize = 1;
        while (i < parsed.items.len) {
            const prev = parsed.items[i - 1];
            const curr = parsed.items[i];
            const x0 = @min(prev[0], curr[0]);
            const y0 = @min(prev[1], curr[1]);
            const x1 = @min(prev[0], curr[0]);
            const y1 = @min(prev[1], curr[1]);
            abyss = @max(abyss, y1);

            std.debug.assert(x0 == x1 or y0 == y1);
            if (x0 == x1) {
                var y = y0;
                while (y <= y1) {
                    try reservoir.markSolid(.{ x0, y });
                    y += 1;
                }
            } else {
                var x = x0;
                while (x <= x1) {
                    try reservoir.markSolid(.{ x, y0 });
                    x += 1;
                }
            }

            i += 1;
        }

        parsed.items.len = 0;
    }

    const wall_count = reservoir.solidCount();
    const spawn = [_]i16{ 500, 0 };

    // Part 1
    outer: while (true) {
        var sand = spawn;
        while (true) {
            if (sand[1] >= abyss) {
                break :outer;
            }

            if (reservoir.step(sand)) |next| {
                sand = next;
                continue;
            }

            try reservoir.markSolid(sand);
            break;
        }
    }
    try stdout.print("{d}\n", .{reservoir.solidCount() - wall_count});

    // Part 2
    const floor = abyss + 2;
    while (!reservoir.solid.contains(spawn)) {
        var sand = spawn;
        while (true) {
            if (sand[1] + 1 == floor) {
                try reservoir.markSolid(sand);
                break;
            }

            if (reservoir.step(sand)) |next| {
                sand = next;
                continue;
            }

            try reservoir.markSolid(sand);
            break;
        }
    }
    try stdout.print("{d}\n", .{reservoir.solidCount() - wall_count});
}
