import qualified Data.HashSet as HashSet

type Move = (Char, Int)

type Point = (Int, Int)

newtype Rope =
  Rope [Point]

adjustKnot :: Point -> Point -> Point
adjustKnot (y0, x0) from@(y1, x1)
  | abs dy <= 1 && abs dx <= 1 = from
  | dx == 0 && dy == 0 = from
  | otherwise = (y1 - signum dy, x1 - signum dx)
  where
    dy = y1 - y0
    dx = x1 - x0

-- Given a list of knot positions, fix up the tail of the list such that the
-- invariant of each knot being at most 1 tile away holds. This assumes that
-- the head hasn't moved too far.
adjustKnots :: [Point] -> [Point]
adjustKnots (head:knots) = head : move' head knots
  where
    move' _ [] = []
    move' head (k:knots) =
      let k' = adjustKnot head k
       in k' : move' k' knots

-- Move an entire rope.
moveRopeRelative :: Point -> Rope -> Rope
moveRopeRelative (dx, dy) (Rope ((hx, hy):tail)) =
  Rope (adjustKnots $ (hx + dx, hy + dy) : tail)

-- Apply a repeated move, e.g. ('R', 4), to a rope. The resulting tuple
-- consists of the end state of the rope, along with all of the tail positions
-- from each intermediate move.
applyMove1 :: Move -> Rope -> (Rope, [Point])
applyMove1 (d, n) rope =
  let delta =
        case d of
          'U' -> (-1, 0)
          'D' -> (1, 0)
          'L' -> (0, -1)
          'R' -> (0, 1)
      ropes = take (n + 1) $ iterate (moveRopeRelative delta) rope
      tailPositions = map (\(Rope xs) -> last xs) ropes
   in (last ropes, tailPositions)

-- Apply a sequence of repeated moves to a rope.
applyMoves :: [Move] -> Rope -> (Rope, [Point])
applyMoves moves rope = applyMoves' moves rope []
    -- The accumulator may include duplicates, we count uniques using a hash
    -- set later.
  where
    applyMoves' [] rope acc = (rope, acc)
    applyMoves' (m:ms) rope acc =
      let (newRope, tailPositions) = applyMove1 m rope
       in applyMoves' ms newRope (acc ++ tailPositions)

-- Given a sequence of moves and a rope, count the unique number of positions
-- that the tail of the rope visits.
countUniqueTailPositions :: [Move] -> Rope -> Int
countUniqueTailPositions moves =
  HashSet.size . HashSet.fromList . snd . applyMoves moves

-- Parses a line from the input into a move.
parseMove :: String -> Move
parseMove s =
  let [dir, n] = words s
   in (head dir, read n)

main = do
  moves <- fmap (map parseMove . lines) getContents
  print . countUniqueTailPositions moves . Rope $ replicate 2 (4, 0)
  print . countUniqueTailPositions moves . Rope $ replicate 10 (4, 0)
