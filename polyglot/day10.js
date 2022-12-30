const fs = require("fs");
const text = fs.readFileSync(process.stdin.fd).toString();
const lines = text.split(/\n/).filter(x => x != '');

const timings = {
    noop: 0,
    addx: 1,
};

var X = 1;
var cycle = 1;
var pending_instr = undefined;
var signal_strength = 0;
var crt_rows = [];

while (lines.length > 0) {
    // Start of the cycle. If nothing is executing, fetch an instruction.
    if (pending_instr === undefined) {
        const line = lines.shift();
        const [op, ...args] = line.split(/\s+/);
        pending_instr = [cycle + timings[op], op, ...args.map(x => parseInt(x))];
    }

    // Get signal strength for part 1:
    if ([20, 60, 100, 140, 180, 220].includes(cycle))
        signal_strength += cycle * X;

    // Draw to CRT for part 2:
    const col = (cycle - 1) % 40;
    if (col == 0) {
        // Push a new scanline.
        crt_rows.push("");
    }
    crt_rows[crt_rows.length - 1] +=
        (X - 1 <= col && col <= X + 1) ? "#" : " ";

    // End of the cycle.
    if (pending_instr[0] == cycle) {
        // Current instruction is due; finish it.
        if (pending_instr[1] == "addx") {
            X += pending_instr[2];
        }
        pending_instr = undefined;
    }

    cycle += 1;
}

console.log(signal_strength);
console.log(crt_rows.join("\n"));
