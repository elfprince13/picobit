#!/usr/bin/env racket
#lang racket

(define (usage)
  (printf "usage: p [-L|-B] path/to/file.scm\n")
  (exit 1))


; TODO: DE-DUPLICATE THIS CODE WITH RUN-TESTS
(define little-endian? (make-parameter #f))

(define (invoke-compiler file-str)
  (system* "./picobit" (if little-endian? "-L" "-B") file-str))

(define (invoke-vm hex)
  (system* (string-append "./vm/picobit-vm" (if little-endian? "-le" "-be")) hex))
; END TODO

(command-line
 #:once-any
 [("-L" "--little-endian")
  "Generate little-endian variant byte code"
  (little-endian? #t)]
 [("-B" "--big-endian")
  "Generate big-endian variant byte code"
  (little-endian? #f)]
 #:args (file)
 (void (and (invoke-compiler (path-replace-suffix file ".scm"))
            (invoke-vm (path-replace-suffix file ".hex")))))
