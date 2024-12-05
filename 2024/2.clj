(require '[clojure.string :as str])

(defn parse-input []
  (for [line (line-seq (java.io.BufferedReader. *in*))]
    (->> (str/split line #"\s+")
         (map Integer/parseInt))))

(defn ordered-by? [cmp report]
  (->> (map vector report (drop 1 report) (drop 2 report))
       (every? #(apply cmp %))))

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
  (keep-indexed #(if (not= i %1) %2 nil) report))

(defn safe-with-removal? [report]
  (->> (map-indexed (fn [i _] (drop-nth i report)) report)
       (some safe?)))

(defn solve [pred reports]
  (count (filter pred reports)))

(let [reports (parse-input)]
  (println (solve safe? reports))
  (println (solve safe-with-removal? reports)))
