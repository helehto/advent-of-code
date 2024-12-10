(require '[clojure.string :as str])

(defn parse-input []
  (for [line (line-seq (java.io.BufferedReader. *in*))]
    (->> (re-seq #"\d+" line)
         (map Long/parseLong))))

(defn eqn-true-1? [goal a [b & others]]
  (if (nil? b)
    (= a goal)
    (or (eqn-true-1? goal (+ a b) others)
        (eqn-true-1? goal (* a b) others))))

(defn eqn-true-2? [goal a [b & others]]
  (if (nil? b)
    (= a goal)
    (or (eqn-true-2? goal (+ a b) others)
        (eqn-true-2? goal (* a b) others)
        (eqn-true-2? goal (Long/parseLong (str a b)) others))))

(defn solve [predicate input-lines]
  (let [predicate* (fn [[goal & nums]] (predicate goal 0 nums))]
    (->> (filter predicate* input-lines)
         (map first)
         (reduce +))))

(let [input (parse-input)]
  (println (solve eqn-true-1? input))
  (println (solve eqn-true-2? input)))
