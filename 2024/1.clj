(require '[clojure.string :as str])

(defn parse-line [line]
  (->> (str/split line #"\s+")
       (map Integer/parseInt)))

(defn parse-input []
  (->> (line-seq (java.io.BufferedReader. *in*))
       (map parse-line)
       (apply map vector))) ; transpose

(defn part1 [input]
  (->> (map sort input)
       (apply map (comp abs -))
       (reduce +)))

(defn part2 [[a b]]
  (let [freqs (frequencies b)
        score #(* % (freqs % 0))]
    (reduce + (map score a))))

(let [input (parse-input)]
  (println (part1 input))
  (println (part2 input)))
