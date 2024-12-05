(require '[clojure.string :as str])

(defn parse-input []
  (->> (line-seq (java.io.BufferedReader. *in*))
       (str/join "")))

(defn eval-mul-clause [[_ & groups]]
  (apply * (map Integer/parseInt groups)))

(defn part1 [prog]
  (->> (re-seq #"mul\((\d+)\s*,\s*(\d+)\)" prog)
       (map eval-mul-clause)
       (reduce +)))

(defn update-state [{enabled :enabled, :as state} [op & _ :as clause]]
  (if (str/starts-with? op "mul")
    (update state :sum #(if enabled (+ % (eval-mul-clause clause)) %))
    (assoc state :enabled (= op "do()"))))

(defn part2 [prog]
  (->> (re-seq #"do\(\)|don't\(\)|mul\((\d+)\s*,\s*(\d+)\)" prog)
       (reduce update-state {:enabled true :sum 0})
       (:sum)))

(let [prog (parse-input)]
  (println (part1 prog))
  (println (part2 prog)))
