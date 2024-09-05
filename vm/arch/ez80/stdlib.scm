(define putchar #%putchar)

(define getchar #%getchar)

(define sleep
  (lambda (duration)
    (#%sleep-aux (#%+ (clock) duration))))

(define #%sleep-aux
  (lambda (wake-up)
    (if (< (clock) wake-up)
        (#%sleep-aux wake-up)
        #f)))

(define display
  (lambda (x)
    (if (string? x)
      (let ([upper-bound (string-length x)])
        (let loop ([i 0])
          (if (< i upper-bound)
            (begin 
              (#%putchar (string-ref x i))
              (loop (+ 1 i)))
            #f)))
        (write x))))

(define (newline) (#%putchar #\newline))

(define (displayln x) (display x) (newline))

(define write
  (lambda (x)
    (cond ((string? x)
	   (begin 
		  (#%putchar #\")
		  (display x)
		  (#%putchar #\")))
	  ((number? x)
	   (display (number->string x)))
	  ((pair? x)
	   (begin (#%putchar #\()
                  (write (car x))
                  (#%write-list (cdr x))))
	  ((symbol? x)
           (begin
	     (display "'")
             (display (symbol->string x))))
	  ((boolean? x)
	   (display (if x "#t" "#f")))
	  (else
	   (display "#<object>")))))
;; TODO have vectors and co ?

(define #%write-list
  (lambda (lst)
    (cond ((null? lst)
	   (#%putchar #\)))
	  ((pair? lst)
	   (begin (#%putchar #\space)
		  (write (car lst))
		  (#%write-list (cdr lst))))
	  (else
	   (begin (display " . ")
		  (write lst)
		  (#%putchar #\)))))))

(define pp
  (lambda (x)
    (write x)
    (#%putchar #\newline)))

(define current-time clock)
(define time->seconds (lambda (t) (quotient t 100)))
