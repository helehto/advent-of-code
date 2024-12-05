(require '[clojure.string :as str])

(defn parse-input []
  (->> (line-seq (java.io.BufferedReader. *in*))
       (mapv #(apply vector %))))

(defn walk [grid [x y] [dx dy] length]
  (->> (range length)
       (map #(get-in grid [(+ y (* % dy)), (+ x (* % dx))]))
       (str/join "")))

(defn index-seq [grid]
  (for [y (range (count grid))
        x (range (count (first grid)))]
    [x y]))

(defn part1 [grid]
  (->> (for [p (index-seq grid), dy [-1 0 1], dx [-1 0 1]]
         (walk grid p [dx dy] 4))
       (filter #(= "XMAS" %))
       (count)))

(defn x-mas-at? [grid [x y]]
  (and (#{"SAM" "MAS"} (walk grid [x y] [1 1] 3))
       (#{"SAM" "MAS"} (walk grid [(+ x 2) y] [-1 1] 3))))

(defn part2 [grid]
  (->> (index-seq grid)
       (filter #(x-mas-at? grid %))
       (count)))

(let [grid (parse-input)]
  (println (part1 grid))
  (println (part2 grid)))
