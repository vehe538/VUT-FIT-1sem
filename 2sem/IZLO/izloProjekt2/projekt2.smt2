(set-logic ALL)
(set-option :produce-models true)
(set-option :incremental true)

(declare-fun A () Int)
(declare-fun B () Int)
(declare-fun C () Int)
(declare-fun D () Int)
(declare-fun E () Int)


(define-fun x ((A Int) (B Int)) Int
  (* 2 A B)
)

(define-fun y ((A Int) (B Int) (C Int) (E Int) (X Int)) Int

  (ite
    (< X E)
    (+ X (* 5 B))
    (- X C)
  )
)

(define-fun z ((A Int) (B Int) (C Int) (D Int) (E Int) (X Int) (Y Int)) Int
  (ite
      (< Y (- D 2))
      (- (* X A) (* B Y))
      (+ (* Y A) (* B X))
  )
)


(assert
  (and
    (and ( > D 0 ) ( > E 0 ))
    (< (z A B C D E (x A B) (y A B C E (x A B))) (+ E D))  
  )
)

(assert
  (not
    (exists ((g Int) (h Int))
      (and        
          (> g 0)
          (> h 0)
          (< (z A B C g h (x A B) (y A B C g (x A B))) (+ g h))
          (< (+ g h) (+ E D))
      )
    )
  )
)



(echo "Test 1 - vstup A=1, B=1, C=3")
(echo "a) Ocekavany vystup je sat a D+E se rovna 2")
(check-sat-assuming (
  (= A 1) (= B 1) (= C 3)
))
(get-value (D E (+ D E)))

(echo "b) Neexistuje jine reseni nez 2, ocekavany vystup je unsat")
(check-sat-assuming (
  (= A 1) (= B 1) (= C 3) (distinct (+ D E) 2)
))


(echo "Test 2 - vstup A=5, B=2, C=5")
(echo "a) Ocekavany vystup je sat a D+E se rovna 54")
(check-sat-assuming (
  (= A 5) (= B 2) (= C 5)
))
(get-value (D E (+ D E)))

(echo "b) Neexistuje jine reseni nez 54, ocekavany vystup je unsat")
(check-sat-assuming (
  (= A 5) (= B 2) (= C 5) (distinct (+ D E) 54)
))

(echo "Test 3 - vstup A=100, B=15, C=1")
(echo "a) Ocekavany vystup je sat a D+E se rovna 253876")
(check-sat-assuming (
  (= A 100) (= B 15) (= C 1)
))
(get-value (D E (+ D E)))

(echo "b) Neexistuje jine reseni nez 253876, ocekavany vystup je unsat")
(check-sat-assuming (
  (= A 100) (= B 15) (= C 1) (distinct (+ D E) 253876)
))

(echo "Test 4 - vstup A=5, B=5, C=3")
(echo "a) Ocekavany vystup je sat a D+E se rovna 51")
(check-sat-assuming (
  (= A 5) (= B 5) (= C 3)
))
(get-value (D E (+ D E)))

(echo "b) Neexistuje jine reseni nez 51, ocekavany vystup je unsat")
(check-sat-assuming (
  (= A 5) (= B 5) (= C 3) (distinct (+ D E) 51)
))

(echo "Test 5 - vstup A=2, B=1, C=2")
(echo "a) Ocekavany vystup je sat a D+E se rovna 7")
(check-sat-assuming (
  (= A 2) (= B 1) (= C 2)
))
(get-value (D E (+ D E)))

(echo "b) Neexistuje jine reseni nez 7, ocekavany vystup je unsat")
(check-sat-assuming (
  (= A 2) (= B 1) (= C 2) (distinct (+ D E) 7)
))
