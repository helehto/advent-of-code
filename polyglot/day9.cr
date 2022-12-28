def move_knot(dest, src)
  src = src.dup
  dy = dest[0] - src[0]
  dx = dest[1] - src[1]

  if dx.abs > 1 || dy.abs > 1
    src[0] += dy <=> 0
    src[1] += dx <=> 0
  end

  src
end

def move(pos, tail_positions, dy, dx, n)
  n.times do
    # Move the head
    pos[0] = [pos[0][0] + dy, pos[0][1] + dx]

    # Update tail positions
    (1 .. pos.size - 1).each do |i|
      pos[i] = move_knot(pos[i - 1], pos[i])
    end

    tail_positions << pos[-1]
  end
end


def solve(lines, n)
  pos = n.times.map {[30, 30]}.to_a
  tail_positions = Set{pos[-1]}

  lines.map {|l| l.split}.each do |fields|
    dir, n = fields
    case dir
    when "U"
      move pos, tail_positions, -1, 0, n.to_i
    when "D"
      move pos, tail_positions, 1, 0, n.to_i
    when "L"
      move pos, tail_positions, 0, -1, n.to_i
    when "R"
      move pos, tail_positions, 0, 1, n.to_i
    end
  end

  tail_positions.size
end

lines = STDIN.each_line.to_a
puts solve lines, 2
puts solve lines, 10
