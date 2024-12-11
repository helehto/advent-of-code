(require '[clojure.string :as str])

(defn blink-step [freq [n cnt]]
  (let [update* (fn [m k] (update m k #(+ (or % 0) cnt)))]
    (if (zero? n)
      (update* freq 1)
      (let [s (str n)]
        (if (even? (count s))
          (let [[a b] (->> (split-at (quot (count s) 2) s)
                           (map #(Long/parseLong (str/join "" %))))]
            (-> freq (update* a) (update* b)))
          (update* freq (* n 2024)))))))

(defn blink [freq]
  (reduce blink-step {} freq))

(defn parse-input []
  (->> (re-seq #"\d+" (read-line))
       (map Long/parseLong)))

(defn count-from-freqs [freqs]
  (reduce + (vals freqs)))

(let [blinks-seq (->> (parse-input) frequencies (iterate blink))]
  (println (count-from-freqs (nth blinks-seq 25)))
  (println (count-from-freqs (nth blinks-seq 75))))
