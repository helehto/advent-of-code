(require '[clojure.string :as str])

(defn parse-input []
  (->> (java.io.BufferedReader. *in*)
       line-seq
       (map #(str/split % #"\s+"))
       (map #(map Integer/parseInt %))
       (apply map vector))) ; transpose

(defn part1 [input]
  (->> input
       (map sort)
       (apply map vector)
       (map #(abs (reduce - %)))
       (reduce +)))

; Builds a map analogously to Python's Counter type.
(defn- count-elements [coll]
  (->> coll
       (map #(hash-map % 1))
       (apply merge-with +)))

(defn part2 [[a b]]
  (let [c (count-elements b)]
    (->> a
         (map #(* % (get c % 0)))
         (reduce +))))

(let [input (parse-input)]
  (println (part1 input))
  (println (part2 input)))
