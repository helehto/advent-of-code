(require '[clojure.string :as str])

(defn parse-input []
  (->> (java.io.BufferedReader. *in*)
       line-seq
       (mapv #(apply vector %))))

(defn walk [grid [x y] [Δx Δy] length]
  (->> (range length)
       (map #(get-in grid [(+ y (* % Δy)), (+ x (* % Δx))]))
       (clojure.string/join "")))

(defn index-seq [grid]
  (for [y (range (count grid))
        x (range (count (first grid)))]
    [x y]))

(defn part1 [grid]
  (->> (for [p (index-seq grid), Δy [-1 0 1], Δx [-1 0 1]]
         (walk grid p [Δx Δy] 4))
       (filter #(= "XMAS" %))
       count))

(defn x-mas-at? [grid [x y]]
  (and (.contains ["SAM" "MAS"] (walk grid [x y] [1 1] 3))
       (.contains ["SAM" "MAS"] (walk grid [(+ x 2) y] [-1 1] 3))))

(defn part2 [grid]
  (->> (index-seq grid)
       (filter #(x-mas-at? grid %))
       count))

(let [grid (parse-input)]
  (println (part1 grid))
  (println (part2 grid)))
