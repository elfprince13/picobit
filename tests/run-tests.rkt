#lang racket

(require rackunit rackunit/text-ui)

(define little-endian? (make-parameter #f))

(define (invoke-compiler file-str)
  (system* "./picobit" (if little-endian? "-L" "-B") file-str))

(define (invoke-vm hex)
  (system* (string-append "./vm/picobit-vm" (if little-endian? "-le" "-be")) hex))

;; This tests whether the programs produce the expected output.
;; This is a pretty weak equivalence, and doesn't catch optimization
;; regressions. Checking for bytecode equality would be too strong an
;; equivalence.
;; This should be fixed.

(define (test-file? file)
  (and (regexp-match? #rx"[.]scm$" file)
       ;; skip emacs temp unsaved file backups
       (not (regexp-match "^\\.#" file))))

(define (run-one file f)
  (when (test-file? file)
    (let* ([file-str (path->string file)]
           [hex (path-replace-suffix file ".hex")]
           [expected (path-replace-suffix file ".expected")]
           [input    (path-replace-suffix file ".input")])
      (test-suite
       file-str
       (begin
         ;; (displayln file)
         (test-case "no expected file"
                    (check-true (file-exists? expected)))
         (when (file-exists? expected)
           (f file-str hex expected input))
         (when (file-exists? hex)
           (delete-file hex)))))))

(define (run-succeed file)
  (run-one
   file
   (lambda (file-str hex expected input)
     (invoke-compiler file-str)
     (test-case "compilation" (check-true (file-exists? hex)))
     (when (file-exists? hex)
       (define out (open-output-string))
       (parameterize ([current-input-port  (if (file-exists? input)
                                               (open-input-file input)
                                               (open-input-string ""))]
                      [current-output-port out]
                      [current-error-port  out]) ; run-fail-execute needs that
         (invoke-vm hex))
       (test-case "execution"
                  (check-equal? (get-output-string out)
                                (file->string expected)))))))

(define (run-fail-compile file)
  (run-one
   file
   (lambda (file-str hex expected input)
     (define err (open-output-string))
     (parameterize ([current-error-port err])
       (invoke-compiler file-str))
     (test-case "compilation error"
                (check-false (file-exists? hex))
                (check-equal? (get-output-string err)
                              (file->string expected))))))

(define (run-fail-execute file) (run-succeed file))

(define (run-single file)
  (let*-values ([(path p b) (split-path file)]
                [(dir)      (path->string path)])
    (cond [(equal? dir "tests/succeed/")
           (run-succeed file)]
          [(equal? dir "tests/fail/compile/")
           (run-fail-compile file)]
          [(equal? dir "tests/fail/execute/")
           (run-fail-execute file)])))

(command-line
 #:once-any
 [("-L" "--little-endian")
  "Generate little-endian variant byte code"
  (little-endian? #t)]
 [("-B" "--big-endian")
  "Generate big-endian variant byte code"
  (little-endian? #f)]
 #:args args
 (void
  (run-tests
   (cond [(not (null? args)) ; run one
          (run-single (string->path (car args)))]
         [else ; run all
          (make-test-suite
           "PICOBIT tests"
           (filter (lambda (x) (not (void? x)))
                   (append
                    (for/list ([file (in-directory "./tests/succeed/")])
                      (run-succeed file))
                    (for/list ([file (in-directory "./tests/fail/compile/")])
                      (run-fail-compile file))
                    (for/list ([file (in-directory "./tests/fail/execute/")])
                      (run-fail-execute file)))))]))))
