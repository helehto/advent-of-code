(defn parse-input []
  (apply vector
         (for [line (line-seq (java.io.BufferedReader. *in*))]
           (mapv #(Character/digit % 10) line))))

(defn indices [grid]
  (for [y (range (count grid))
        x (range (count (first grid)))]
    [y x]))

(defn neighbors-of [grid u]
  (for [d [[-1 0] [1 0] [0 -1] [0 1]]
        :let [v (mapv + u d), e (get-in grid v)]
        :when (some? e)]
    [v e]))

(defn trail-neighbors-of [grid u]
  (->> (neighbors-of grid u)
       (filter (fn [[v e]] (= 1 (- e (get-in grid u)))))
       (map first)))

(defn trailhead-score [grid head allow-revisit]
  (loop [queue [head] visited #{} score 0]
    (let [u (first queue)]
      (if (nil? u)
        score
        (if (and (not allow-revisit)
                 (visited u))
          (recur (subvec queue 1)
                 visited
                 score)
          (recur (->> (trail-neighbors-of grid u)
                      (into (subvec queue 1)))
                 (conj visited u)
                 (if (= (get-in grid u) 9)
                   (inc score)
                   score)))))))

(let [grid (parse-input)
      starts (filter #(zero? (get-in grid %)) (indices grid))]
  (println (reduce + (map #(trailhead-score grid % false) starts)))
  (println (reduce + (map #(trailhead-score grid % true) starts))))
