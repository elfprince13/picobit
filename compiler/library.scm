(define null '())

(define +
  (lambda (x . rest)
    (if (pair? rest)
        (#%+-aux x rest)
        x)))

(define #%+-aux
  (lambda (x rest)
    (if (pair? rest)
        (#%+-aux (#%+ x (car rest)) (cdr rest))
        x)))

(define neg
  (lambda (x)
    (- 0 x)))

(define -
  (lambda (x . rest)
    (if (pair? rest)
        (#%--aux x rest)
        (neg x))))

(define #%--aux
  (lambda (x rest)
    (if (pair? rest)
        (#%--aux (#%- x (car rest)) (cdr rest))
        x)))

(define *
  (lambda (x . rest)
    (if (pair? rest)
        (#%*-aux x rest)
        x)))

(define #%*-aux
  (lambda (x rest)
    (if (pair? rest)
        (#%*-aux (#%mul x (car rest)) (cdr rest))
        x)))

(define #%mul
  (lambda (x y)
    (let* ((x-neg? (< x 0))
           (y-neg? (< y 0))
           (x      (if x-neg? (neg x) x))
           (y      (if y-neg? (neg y) y)))
      (let ((prod   (#%mul-non-neg x y)))
        (cond ((and x-neg? y-neg?)
               prod)
              ((or x-neg? y-neg?)
               (neg prod))
              (else
               prod))))))

(define quotient ;; TODO similar to #%mul, abstract ?
  (lambda (x y)
    (let* ((x-neg? (< x 0))
           (y-neg? (< y 0))
           (x      (if x-neg? (neg x) x))
           (y      (if y-neg? (neg y) y)))
      (let ((quot   (#%div-non-neg x y)))
        (cond ((and x-neg? y-neg?)
               quot)
              ((or x-neg? y-neg?)
               (neg quot))
              (else
               quot))))))

(define / quotient)


(define <=
  (lambda (x y)
    (or (< x y) (= x y))))

(define >=
  (lambda (x y)
    (or (> x y) (= x y))))

(define list
  (lambda lst lst))

(define length
  (lambda (lst)
    (#%length-aux lst 0)))

(define #%length-aux
  (lambda (lst n)
    (if (pair? lst)
        (#%length-aux (cdr lst) (#%+ n 1))
        n)))

(define append
  (lambda (lst1 lst2)
    (if (pair? lst1)
        (cons (car lst1) (append (cdr lst1) lst2))
        lst2)))

(define reverse
  (lambda (lst)
    (#%reverse-aux lst '())))

(define #%reverse-aux
  (lambda (lst rev)
    (if (pair? lst)
        (#%reverse-aux (cdr lst) (cons (car lst) rev))
        rev)))

(define list-ref
  (lambda (lst i)
    (if (= i 0)
        (car lst)
        (list-ref (cdr lst) (#%- i 1)))))

(define list-set!
  (lambda (lst i x)
    (if (= i 0)
        (set-car! lst x)
        (list-set! (cdr lst) (#%- i 1) x))))

(define max
  (lambda (x y)
    (if (> x y) x y)))

(define min
  (lambda (x y)
    (if (< x y) x y)))

(define abs
  (lambda (x)
    (if (< x 0) (neg x) x)))

(define remainder #%rem-non-neg)
(define modulo    #%rem-non-neg)

(define #%box (lambda (a) (cons a '())))

(define #%unbox car)

(define #%box-set! set-car!)

(define make-string
  (lambda (n . c)
    (#%make-string n (if (pair? c) (car c) 0))))

(define string
  (lambda chars
    (list->string chars)))

(define string-append
  (lambda (str1 str2)
    (let* ((len1 (string-length str1))
           (len2 (string-length str2))
           (len3 (+ len1 len2))
           (str3 (make-string len3 0)))
     (letrec
         ((copy1
           (lambda (i)
             (if (< i len1)
                 (begin
                   (string-set! str3 i (string-ref str1 i))
                   (copy1 (#%+ 1 i)))
                 (copy2 i 0))))
          (copy2
           (lambda (i j)
             (if (< j len2)
                 (begin
                   (string-set! str3 i (string-ref str2 j))
                   (copy2 (#%+ 1 i) (#%+ 1 j)))
                 str3))))
       (copy1 0)))))

(define substring
  (lambda (str start end)
    (let* ((len (#%- end start))
           (result (make-string len 0)))
      (letrec
          ((copy
            (lambda (i j)
              (if (< i len)
                  (begin
                    (string-set! result i (string-ref str j))
                    (copy (#%+ 1 i) (#%+ 1 j)))
                  result))))
        (copy 0 start)))))

(define symbol->string
  (lambda (symbol)
    (let ([str (symbol->immutable-string symbol)])
      (substring str 0 (string-length str)))))

(define map
  (lambda (f lst)
    (if (pair? lst)
        (cons (f (car lst))
              (map f (cdr lst)))
        '())))

(define for-each
  (lambda (f lst)
    (if (pair? lst)
        (begin
          (f (car lst))
          (for-each f (cdr lst)))
        #f)))

(define call/cc
  (lambda (receiver)
    (let ((k (get-cont)))
      (receiver
       (lambda (r)
         (return-to-cont k r))))))

(define root-k #f)
(define readyq #f)

(define start-first-process
  (lambda (thunk)
    ;; rest of the program, after call to start-first-process
    (set! root-k (get-cont))
    (set! readyq (cons #f #f))
    ;; initialize thread queue, which is a circular list of continuations
    (set-cdr! readyq readyq)
    (thunk)))

(define spawn
  (lambda (thunk)
    (let* ((k (get-cont))
           (next (cons k (cdr readyq))))
      ;; add a new link to the circular list
      (set-cdr! readyq next)
      ;; Run thunk with root-k as cont.
      (graft-to-cont root-k (lambda () (thunk) (exit))))))

(define exit
  (lambda ()
    (let ((next (cdr readyq)))
      (if (eq? next readyq) ; queue is empty
          #f
          (begin
            ;; step once on the circular list
            (set-cdr! readyq (cdr next))
            ;; invoke next thread
            (return-to-cont (car next) #f))))))

(define yield
  (lambda ()
    (let ((k (get-cont)))
      ;; save the current continuation
      (set-car! readyq k)
      ;; step once on the circular list
      (set! readyq (cdr readyq))
      ;; run the next thread
      (let ((next-k (car readyq)))
        (set-car! readyq #f)
        (return-to-cont next-k #f)))))

(define number->string
  (lambda (n)
    (list->string
     (if (< n 0)
         (cons #\- (#%number->string-aux (neg n) '()))
         (#%number->string-aux n '())))))

(define #%number->string-aux
  (lambda (n lst)
    (let ((rest (cons (#%+ #\0 (remainder n 10)) lst)))
      (if (< n 10)
          rest
          (#%number->string-aux (quotient n 10) rest)))))

(define caar
  (lambda (p)
    (car (car p))))
(define cadr
  (lambda (p)
    (car (cdr p))))
(define cdar
  (lambda (p)
    (cdr (car p))))
(define cddr
  (lambda (p)
    (cdr (cdr p))))
(define caaar
  (lambda (p)
    (car (car (car p)))))
(define caadr
  (lambda (p)
    (car (car (cdr p)))))
(define cadar
  (lambda (p)
    (car (cdr (car p)))))
(define caddr
  (lambda (p)
    (car (cdr (cdr p)))))
(define cdaar
  (lambda (p)
    (cdr (car (car p)))))
(define cdadr
  (lambda (p)
    (cdr (car (cdr p)))))
(define cddar
  (lambda (p)
    (cdr (cdr (car p)))))
(define cdddr
  (lambda (p)
    (cdr (cdr (cdr p)))))

(define equal?
  (lambda (x y)
    (cond ((eq? x y)
	   #t)
	  ((and (pair? x) (pair? y))
	   (and (equal? (car x) (car y))
		(equal? (cdr x) (cdr y))))
	  ((and (u8vector? x) (u8vector? y))
	   (u8vector-equal? x y))
	  (else
	   #f))))

(define u8vector-equal?
  (lambda (x y)
    (let ((lx (u8vector-length x)))
      (if (= lx (u8vector-length y))
	  (#%u8vector-equal?-loop x y (- lx 1))
	  #f))))
(define #%u8vector-equal?-loop
  (lambda (x y l)
    (if (= l 0)
	#t
	(and (= (u8vector-ref x l) (u8vector-ref y l))
	     (#%u8vector-equal?-loop x y (#%- l 1))))))

(define assoc
  (lambda (t l)
    (cond ((null? l)
	   #f)
	  ((equal? t (caar l))
	   (car l))
	  (else
	   (assoc t (cdr l))))))

(define memq
  (lambda (t l)
    (cond ((null? l)
	   #f)
	  ((eq? (car l) t)
	   l)
	  (else
	   (memq t (cdr l))))))

(define vector
  (lambda x
    (list->vector x)))
(define list->vector
  (lambda (x)
    (let* ((n (length x))
	   (v (#%make-vector n #f)))
      (#%list->vector-loop v 0 x)
      v)))
(define #%list->vector-loop
  (lambda (v n x)
    (vector-set! v n (car x))
    (if (not (null? (cdr x)))
	(#%list->vector-loop v (#%+ n 1) (cdr x)))))
(define make-vector
  (lambda (n . x)
    (#%make-vector n (if (pair? x) (car x) 0))))
(define vector-copy!
  (lambda (source source-start target target-start n)
    (if (> n 0)
        (begin (vector-set! target target-start
                              (vector-ref source source-start))
               (vector-copy! source (+ source-start 1)
                               target (+ target-start 1)
                               (- n 1))))))
(define vector->list
  (lambda (v)
    (let loop ([res '()]
               [i (- (vector-length v) 1)])
      (let ([next-res (cons (vector-ref v i) res)])
        (if (eq? i 0)
            next-res
            (loop next-res (- i 1)))))))

(define u8vector
  (lambda x
    (list->u8vector x)))
(define list->u8vector
  (lambda (x)
    (let* ((n (length x))
	   (v (#%make-u8vector n 0)))
      (#%list->u8vector-loop v 0 x)
      v)))
(define #%list->u8vector-loop
  (lambda (v n x)
    (u8vector-set! v n (car x))
    (if (not (null? (cdr x)))
	(#%list->u8vector-loop v (#%+ n 1) (cdr x)))))
(define make-u8vector
  (lambda (n . x)
    (#%make-u8vector n (if (pair? x) (car x) 0))))
(define u8vector-copy!
  (lambda (source source-start target target-start n)
    (if (> n 0)
        (begin (u8vector-set! target target-start
                              (u8vector-ref source source-start))
               (u8vector-copy! source (+ source-start 1)
                               target (+ target-start 1)
                               (- n 1))))))

(define (char-whitespace? c)
  (or (and (>= c #\tab) (<= c #\return)) (eq? c #\space)))

(define (char-numeric? c)
  (and (>= c #\u30) (<= c #\u39)))

(define (char-alphabetic? c)
  (or (and (>= c #\A) (<= c #\Z)) (and (>= c #\a) (<= c #\z))))

(define (char-extended-alphabetic? c)
  (or (char-alphabetic? c)
      (eq? c #\u5B) ;; Greek theta
      (and (>= c #\u8A) (<= c #\uB5)) ;; accented
      (and (>= c #\uBB) (<= c #\uC0)) ;; Greek alpha - epsilon
      (and (>= c #\uC2) (<= c #\uCC)) ;; Greek lambda - omega + x mean / y mean
      (and (>= c #\uD7) (<= c #\uDD)) ;; imaginary i - finance N
      (eq? c #\uF4) ;; sharp s
      ))

(define (error message)
  (print message)
  (#%halt))

(define #%make-reader
  (let*
      ([eof-token '(-1)]
       [eof-char? (lambda (char) (< char 0))]
       [eof-token? (lambda (tok) (eq? tok eof-token))]
       [left-paren-token '(#\()]
       [right-paren-token '(#\))]
       [left-bracket-token '(#\uC1)] ; ti-ascii displaces [ for theta
       [right-bracket-token '(#\])]
       [left-opener?
        (lambda (tok)
          (or (eq? tok left-paren-token)
              (eq? tok left-bracket-token)))]
       [right-opener?
        (lambda (tok)
          (or (eq? tok right-paren-token)
              (eq? tok right-bracket-token)))]
       [matched-openers?
        (lambda (left-tok right-tok)
          (or (and (eq? left-tok left-paren-token)
                   (eq? right-tok right-paren-token))
              (and (eq? left-tok left-bracket-token)
                   (eq? right-tok right-bracket-token))))]
       [mismatched-openers?
        (lambda (left-tok right-tok)
          (or (and (eq? left-tok left-paren-token)
                   (eq? right-tok right-bracket-token))
              (and (eq? left-tok left-bracket-token)
                   (eq? right-tok right-paren-token))))]
       [special-initial?
        (lambda (c)
          (or
           (eq? c #\!)
           (eq? c #\uF2) ; ti-ascii displaces $ for fourth power
           (eq? c #\%)
           (eq? c #\&)
           (eq? c #\*)
           (eq? c #\/)
           (eq? c #\:)
           (and (>= c #\<) (<= c #\?))
           (and (>= c #\u17) (<= c #\u19)) ; lte, neq, gte
           (eq? c #\^)
           (eq? c #\_)
           (eq? c #\~)
           ))]
       [initial?
        (lambda (c)
          (or (char-extended-alphabetic? c) (special-initial? c)))]
       [special-subsequent?
        (lambda (c)
          (or (eq? c #\+) (eq? c #\-) (eq? c #\.) (eq? c #\@)))]
       [subsequent?
        (lambda (c)
          (or (initial? c) (char-numeric? c) (special-subsequent? c)))]
       [peculiar-identifier?
        (lambda (c)
          (or
           (eq? c #\+)
           (eq? c #\-)
           (eq? c #\uCE) ; ellipsis
           ))]
       [convert-digit
        (lambda (d) (- d #\u30))])
    ;; split the bindings because
    ;; vm only supports up to 16 arguments / call
      
    (lambda (#%read-char)
      (lambda ()
        (letrec
            ([peek-char (#%read-char)]
             [read-char
              (lambda ()
                (let ([result peek-char])
                  (if (not (eof-char? result))
                    (set! peek-char (#%read-char)))
                  result))]
             [char-ends-token?
              (lambda (c)
                (or (char-whitespace? c)
                    (eq? c #\()
                    (eq? c #\uC1)
                    (eq? c #\))
                    (eq? c #\])))]
             [read-identifier
              (lambda (head)
                (let read-remainder
                  ([tail head][size 1])
                  (cond
                    [(char-ends-token? peek-char)
                     ; precalculate length to avoid
                     ; extra pass over data
                     (let ([buffer (make-string size)])
                       (let loop ([data head][i 0])
                         (if (null? data)
                             (string->symbol buffer)
                             (begin
                               (string-set! buffer i (car data))
                               (loop (cdr data) (+ 1 i))))))]
                    [(subsequent? peek-char)
                     (let ([new-tail (cons (read-char) null)])
                       (set-cdr! tail new-tail)
                       (read-remainder new-tail (+ 1 size)))]
                    [else (error (string-append "invalid identifier character:"
                                                (make-string 1 peek-char)))])))]
             [read-number
              (lambda (n)
                ;(displayln "reading number")
                (cond
                  [(char-ends-token? peek-char) n]
                  [(char-numeric? peek-char)
                   (read-number (+ (* 10 n) (convert-digit (read-char))))]
                  [else (error (string-append "invalid digit character: "
                                              (make-string 1 peek-char)))]))]
             [read-token
              (lambda ()
                (let ([first-char (read-char)])
                  (cond ;; if first-char is a space or line break, just skip it
                    ;; and loop to try again by calling self recursively
                    [(char-whitespace? first-char)
                     (read-token)]
                    ;; else if it's a left paren char, return the special
                    ;; object that we use to represent left parenthesis tokens.
                    [(eq? first-char #\( )
                     left-paren-token]
                    [(eq? first-char #\uC1 )
                     left-bracket-token]
                    ;; likewise for right parens
                    [(eq? first-char #\) )
                     right-paren-token]
                    [(eq? first-char #\] )
                     right-bracket-token]
                    ;; else if it's a letter, we assume it's the first char
                    ;; of a symbol and call read-identifier to read the rest of
                    ;; of the chars in the identifier and return a symbol object
                    [(initial? first-char)
                     (read-identifier (cons first-char null))]
                    ;; else if it's a digit, we assume it's the first digit
                    ;; of a number and call read-number to read the rest of
                    ;; the number and return a number object
                    [(char-numeric? first-char)
                     (read-number (convert-digit first-char))]
                    [(peculiar-identifier? first-char)
                     (cond
                       [(char-ends-token? peek-char)
                        (string->symbol (make-string 1 first-char))]
                       [(char-numeric? peek-char)
                        (*
                         (cond
                           [(eq? first-char #\+) 1]
                           [(eq? first-char #\-) -1]
                           [else (error "illegal numeric sign character")])
                         (read-number (convert-digit (read-char))))]
                       [else
                        (error
                         (string-append
                          (string-append "illegal lexical syntax: " (make-string 1 first-char))
                          (string-append " followed by " (make-string 1 peek-char))))])]
                    ; end of file / data stream
                    [(eof-char? first-char) eof-token]
                    ;; else it's something this little reader can't handle,
                    ;; so signal an error
                    [else
                     (error (string-append "illegal lexical syntax: "
                                           (make-string 1 first-char)))])))]
             [read-list
              ; avoid a lot of code duplication at the cost of one extra
              ; temp allocation per list read
              (lambda (opener head-minus-1)
                (let read-remainder ([tail head-minus-1]
                                     [next-token (read-token)]
                                     [dot-context #f])
                  (cond
                    [(right-opener? next-token)
                     (if (matched-openers? opener next-token)
                         (cdr head-minus-1)
                         (error
                          (string-append 
                          (string-append "Mismatched parens and braces: " (make-string 1 (car opener)))
                          (string-append " and " (make-string 1 (car next-token))))))]
                    [(eof-token? next-token) (error "Unexpected end of file")]
                    [else
                     (let
                         ([new-tail
                           (cons
                            (if (left-opener? next-token)
                                (read-list next-token (cons #f null))
                                next-token) null)])
                       (set-cdr! tail new-tail)
                       (read-remainder new-tail (read-token) dot-context))])))])
          (let ([token (read-token)])
            (cond
              [(left-opener? token)
               (read-list token (cons #f null))]
              [else token])))))))

(define (read-string str)
  ((#%make-reader
   (let ([n (string-length str)]
         [i 0])
     (lambda ()
       (if (< i n)
           (let ([result (string-ref str i)])
             (set! i (+ i 1))
             result)
           -1))))))
       