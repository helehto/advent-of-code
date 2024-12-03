(require '[clojure.string :as str])

(defn parse-input []
  (->> (java.io.BufferedReader. *in*)
       line-seq
       (filter seq)
       (map #(str/split % #"\s+"))
       (map #(map Integer/parseInt %))))

(defn ordered-by? [cmp report]
  (->> (map vector report (drop 1 report) (drop 2 report))
       (every? (partial apply cmp))))

(defn monotonic? [report]
  (or (ordered-by? < report)
      (ordered-by? > report)))

(defn pairwise-differences [report]
  (->> (map vector report (rest report))
       (map (fn [[a b]] (abs (- a b))))))

(defn safe? [report]
  (and (monotonic? report)
       (every? #(<= 1 % 3) (pairwise-differences report))))

(defn drop-nth [i report]
  (concat (take i report)
          (drop (+ i 1) report)))

(defn safe-with-removal? [report]
  (->> (range (count report))
       (map #(drop-nth % report))
       (filter safe?)
       seq
       some?))

(defn part1 [reports]
  (count (filter safe? reports)))

(defn part2 [reports]
  (count (filter safe-with-removal? reports)))

(let [reports (parse-input)]
  (println (part1 reports))
  (println (part2 reports)))
