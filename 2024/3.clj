(require '[clojure.string :as str])

(defn parse-input []
  (->> (java.io.BufferedReader. *in*)
       line-seq
       (str/join "")))

(defn make-reducer [always-enabled]
  (fn [{enabled :enabled sum :sum, :as state} match]
    (conj state
          (if (str/starts-with? (match 0) "mul")
            [:sum (let [product (* (Integer/parseInt (match 1))
                                   (Integer/parseInt (match 2)))]
                    (+ sum (if enabled product 0)))]
            [:enabled (or always-enabled (= (match 0) "do()"))]))))

(defn solve [prog always-enabled]
  (->> (re-seq #"do\(\)|don't\(\)|mul\((\d+)\s*,\s*(\d+)\)" prog)
       (reduce (make-reducer always-enabled) {:enabled true :sum 0})
       :sum))

(let [prog (parse-input)]
  (println (solve prog true))
  (println (solve prog false)))
