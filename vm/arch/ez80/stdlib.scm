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
        (for-each putchar (string->list x))
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
	   (display "#<symbol>"))
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
