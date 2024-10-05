#lang racket

(require srfi/4) ; u8vector stuff
(require "utilities.rkt" "asm.rkt" "primitives.rkt" "gen.config.rkt")

(provide assemble)

;; These definitions must match those in the VM (in picobit-vm.h).
(define min-fixnum-encoding 3)
(define min-fixnum -1)
(define max-fixnum 256)
(define min-rom-encoding (+ min-fixnum-encoding (- max-fixnum min-fixnum) 1))
(define min-ram-encoding 1280)

(define (predef-constants) (list))

(define (predef-globals) (list))

(define (encode-direct obj)
  (cond [(eq? obj #f)  0]
        [(eq? obj #t)  1]
        [(eq? obj '()) 2]
        [(and (exact-integer? obj)
              (<= min-fixnum obj max-fixnum))
         (+ obj (- min-fixnum-encoding min-fixnum))]
        [else #f])) ; can't encode directly

(define (translate-constant obj)
  (if (char? obj)
      (char->integer obj)
      obj))

(define (encode-constant obj constants)
  (let* ([o (translate-constant obj)]
         [e (encode-direct o)])
    (cond [e e] ; can be encoded directly
          [(assoc o constants)
           => (lambda (x) (vector-ref (cdr x) 0))]
          [else (compiler-error "unknown object" obj)])))

;; TODO actually, seem to be in a pair, scheme object in car, vector in cdr
;; constant objects are represented by vectors
;; 0 : encoding (ROM address) TODO really the ROM address ?
;; 1 : TODO asm label constant ?
;; 2 : number of occurences of this constant in the code
;; 3 : pointer to content, used at encoding time
(define (add-constant obj constants from-code?)
  (define o (translate-constant obj))
  (define e (encode-direct o))
  (cond [e constants] ; can be encoded directly
        [(dict-ref constants o #f) ; did we encode this already?
         =>
         (lambda (x)
           (when from-code? ; increment its reference counter
             (vector-set! x 2 (+ (vector-ref x 2) 1)))
           constants)]
        [else
         (define descr
           (vector #f
                   (asm-make-label 'constant)
                   (if from-code? 1 0)
                   #f))
         (define new-constants (dict-set constants o descr))
         (cond [(pair? o)
                ;; encode both parts as well
                (add-constants (list (car o) (cdr o)) new-constants)]
               [(symbol? o)
                (let ([text (symbol->string o)])
                  (vector-set! descr 3 text) ; intern the string representation
                  (add-constant text new-constants #f))]
               [(string? o)
                ;; TODO? encode each character as well
                (vector-set! descr 3 (asm-make-label 'string-store))
                new-constants]
               [(vector? o)
                  (vector-set! descr 3 (asm-make-label 'vector-store))
                  (add-constants (vector->list o) new-constants)]
               [(u8vector? o)
                (vector-set! descr 3 (asm-make-label 'u8vector-store))
                new-constants]
               [(exact-integer? o)
                (let ([hi (arithmetic-shift o -16)])
                  (vector-set! descr 3 hi)
                  ;; Recursion will stop once we reach 0 or -1 as the
                  ;; high part, which will be matched by encode-direct.
                  ;; Only the high part needs to be registered as a new
                  ;; constant. The low part will be filled in at
                  ;; encoding time.
                  (add-constant hi new-constants #f))]
               (else
                new-constants))]))

(define (add-constants objs constants)
  (for/fold ([constants constants])
      ([o objs])
    (add-constant o constants #f)))

(define (add-global var globals)
  (cond [(dict-ref globals var #f)
         =>
         (lambda (x)
           ;; increment reference counter
           (vector-set! x 1 (+ (vector-ref x 1) 1))
           globals)]
        [else (dict-set globals var (vector (length globals) 1))]))

(define (sort-constants constants)
  (let ([csts (sort constants > #:key (lambda (x) (vector-ref (cdr x) 2)))])
    (for ([i   (in-naturals min-rom-encoding)]
          [cst (in-list csts)])
      (when (>= i min-ram-encoding)
        (compiler-error (~a "too many constants: ROM encoding " i " >= " min-ram-encoding " (min-ram-encoding)")))
      (vector-set! (cdr cst) 0 i))
    csts))

(define (sort-globals globals)
  (let ([glbs (sort globals > #:key (lambda (x) (vector-ref (cdr x) 1)))])
    (for ([i (in-naturals)]
          [g (in-list glbs)])
      (when (> i 256) ;; the number of globals is encoded in a byte
        (compiler-error "too many global variables"))
      (vector-set! (cdr g) 0 i))
    glbs))


;;-----------------------------------------------------------------------------

(define instr-table (make-hash))
(define (inc-instr-count! k)
  (hash-update! instr-table k add1 (lambda () 0)))

(define (label-instr label
                     opcode-rel4 opcode-rel8 opcode-rel12 opcode-abs16
                     opcode-sym)
  (asm-at-assembly
   ;; Args are procedures that go 2 by 2.
   ;; The first one of each pair checks if a given strategy is applicable.
   ;; If so, it returns the size of the code to be generated, and the
   ;; second procedure of the pair generates code. Otherwise, the assembler
   ;; tries the next pair, and so on.

   ;; target is less than 16 bytecodes ahead:
   ;; 4 bit opcode, 4 bit operand
   (lambda (self)
     (let ([dist (- (asm-label-pos label) (+ self 1))])
       (and opcode-rel4 (<= 0 dist 15) 1))) ; size 1 byte total
   (lambda (self)
     (let ([dist (- (asm-label-pos label) (+ self 1))])
       (inc-instr-count! (list '---rel-4bit opcode-sym))
       (asm-8 (+ opcode-rel4 dist))))

   ;; distance is less than 128 in either direction:
   ;; 1 byte opcode, 1 byte operand
   (lambda (self)
     (let ([dist (+ 128 (- (asm-label-pos label) (+ self 2)))])
       (and opcode-rel8 (<= 0 dist 255) 2))) ; size 2 bytes total
   (lambda (self)
     (let ([dist (+ 128 (- (asm-label-pos label) (+ self 2)))])
       (inc-instr-count! (list '---rel-8bit opcode-sym))
       (asm-8 opcode-rel8)
       (asm-8 dist)))

   ;; distance is less than 2048 in either direction:
   ;; 4 bit opcode, 12 bit operand
   (lambda (self)
     (let ([dist (+ 2048 (- (asm-label-pos label) (+ self 2)))])
       (and opcode-rel12 (<= 0 dist 4095) 2))) ; size 2 bytes total
   (lambda (self)
     (let ([dist (+ 2048 (- (asm-label-pos label) (+ self 2)))])
       (inc-instr-count! (list '---rel-12bit opcode-sym))
       (asm-16 (+ (* opcode-rel12 256) dist))))

   ;; target is too far, fallback on absolute jump:
   ;; 1 byte opcode, 2 bytes operand
   (lambda (self)
     3) ; size 3 bytes total
   (lambda (self)
     (let ([pos (- (asm-label-pos label) code-start)])
       (inc-instr-count! (list '---abs-16bit opcode-sym))
       (asm-8 opcode-abs16)
       (asm-16 pos)))))

(define (push-constant n)
  (cond [(<= n 31) ; 3 bit opcode, 5 bit operand. first 32 constants.
         (inc-instr-count! '---push-constant-1byte)
         (asm-8 (+ #x00 n))]
        [else ; 4 bit opcode, 12 bits operand.
         (inc-instr-count! '---push-constant-2bytes)
         (cond [asm-big-endian?
                (asm-16 (+ #xa000 n))]
               [else
                (asm-8 (+ #xa0 (remainder n 16)))
                (asm-8 (+ #x00 (quotient n 16)))]
              )]))

(define (push-stack n)
  (if (> n 31) ; 3 bit opcode, 5 bits operand
      (if (> n 255) ; 8 bit opcode, 8 bit operand
          (compiler-error (~a "stack depth " n " is too deep"))
          (begin
            (compiler-warning (~a "stack depth " n " is very deep - in a hot codepath this may cause performance issues"))
            (asm-8 #xbd)
            (asm-8 n)))
      (asm-8 (+ #x20 n))))

(define (push-global n)
  (cond [(<= n 15) ; 4 bit opcode, 4 bit operand. first 16 globals.
         (inc-instr-count! '---push-global-1byte)
         (asm-8 (+ #x40 n))]
        [else ; 8 bit opcode, 8 bit operand. 256 globals max.
         (inc-instr-count! '---push-global-2bytes)
         (asm-8 #x8e)
         (asm-8 n)]))

(define (set-global n)
  (cond [(<= n 15) ; 4 bit opcode, 4 bit operand. first 16 globals.
         (inc-instr-count! '---set-global-1byte)
         (asm-8 (+ #x50 n))]
        [else ; 8 bit opcode, 8 bit operand. 256 globals max.
         (inc-instr-count! '---set-global-2bytes)
         (asm-8 #x8f)
         (asm-8 n)]))

(define (call n)
  (if (> n 15) ; 4 bit opcode, 4 bit argument (n of args to the call)
      (compiler-error "call has too many arguments")
      (asm-8 (+ #x60 n))))

(define (jump n)
  (if (> n 15) ; 4 bit opcode, 4 bit argument (n of args to the call)
      (compiler-error "call has too many arguments")
      (asm-8 (+ #x70 n))))

(define (call-toplevel label)
  (label-instr label #f   #xb5 #f #xb0 'call-toplevel))
(define (jump-toplevel label)
  (label-instr label #x80 #xb6 #f #xb1 'jump-toplevel))
(define (goto label)
  (label-instr label #f   #xb7 #f #xb2 'goto))
(define (goto-if-false label)
  (label-instr label #x90 #xb8 #f #xb3 'goto-if-false))
(define (closure label)
  (label-instr label #f   #xb9 #f #xb4 'closure))

(define (prim n) (asm-8 (+ #xc0 n)))

;;-----------------------------------------------------------------------------

;; like asm-at-assembly but the first strategy always succeeds.
;; code doesn't know it's own position
(define-syntax (asm-defer stx)
  (syntax-case stx ()
    [(asm-defer (size) exprs ...)
     #'(asm-at-assembly
         (lambda (x) size)
         (lambda (x) exprs ...))]))

;;-----------------------------------------------------------------------------
(define (assemble-constant x constants)
  (match x
    [`(,obj . ,(and descr `#(,_ ,label ,_ ,d3)))
     (asm-label label)
     ;; see the vm source for a description of encodings
     (cond [(exact-integer? obj)
            (let ([hi (encode-constant d3 constants)])
              (asm-32 (+
                       (* #x10000 hi) ; pointer to hi
                       (modulo obj #x10000))))]    ; bits 0-15
           [(pair? obj)
            (let ([obj-car (encode-constant (car obj) constants)]
                  [obj-cdr (encode-constant (cdr obj) constants)])
              (asm-32 (+
                       (* #x10000 (+ #x8000 obj-car))
                       (+ #x0000 obj-cdr))))]
           [(symbol? obj)
            (let ([text-rep (encode-constant d3 constants)])
              (asm-32 (+ #x80002000 (* #x10000 text-rep))))]
           [(string? obj)
            (asm-defer
             (4)
             (let ([data-store (asm-label-pos d3)]
                   [l (string-length obj)])
               ;; length is stored raw, not encoded as an object
               (asm-32
                (+ (* #x10000 (+ #x8000 l))
                   (+ #x4000
                      ; sort of invert C macro OBJ_TO_ROM_ADDR
                      ; (skip MIN_ROM_ENCODING and field offset)
                      (quotient 
                       (- data-store
                          (+ code-start 4)) ; subtract header
                       4))))))]
           [(vector? obj) ; ordinary vectors are stored as lists
            (asm-defer
             (4)
             (let ([data-store (asm-label-pos d3)]
                   [l (vector-length obj)])
               ;; length is stored raw, not encoded as an object
               (asm-32
                (+
                 (* #x10000 (+ #x8000 l))
                 (+ #xA000
                    ; sort of invert C macro OBJ_TO_ROM_ADDR
                    ; (skip MIN_ROM_ENCODING and field offset)
                    (quotient 
                     (- data-store
                        (+ code-start 4)) ; subtract header
                     4))))))]
           [(u8vector? obj)
            (asm-defer
             (4)
             (let ([data-store (asm-label-pos d3)]
                   [l       (u8vector-length obj)])
               ;; length is stored raw, not encoded as an object
               (asm-32
                (+
                 (* #x10000 (+ #x8000 l))
                 (+ #x6000
                    ; sort of invert C macro OBJ_TO_ROM_ADDR
                    ; (skip MIN_ROM_ENCODING and field offset)
                    (quotient 
                     (- data-store
                        (+ code-start 4)) ; subtract header
                     4))))))]
           [else
            (compiler-error "unknown object type" obj)])]))

(define (assemble-byte-buf for-const constants)
  (match for-const
    [`(,obj . ,(and descr `#(,_ ,label ,_ ,d3)))
     (cond
       [(string? obj)
        (asm-align 4 0)
        ;; header: leading 2 bits unused by RAM vectors in original
        ;; implementation but we want to distinguish mutable / immutable
        ;; data so we will set leading bit to 1 for immutable
        (asm-defer (4)
         ;;;;(displayln (~a "Storing " (~v obj) " to backing store, object header at " (asm-label-pos label) ", vector data starts at " (asm-label-pos d3)))
         (asm-32
          (+ (*
              #x10000
              (+ #x8000
                 (+ 1 ; this header block
                    (quotient
                     (+ 4 (string-length obj)) ; +1 for nul-termination (to be C-friendly) and +3 for "ceil" in flooring division by 4
                     4))))
             (asm-label-pos label)))) ; pointer to normal object header
        (asm-label d3)
        (asm-string obj)] ; doesn't handle multi-byte encodings nicely
       [(u8vector? obj)
        (asm-align 4 0)
        ;; header: leading 2 bits unused by RAM vectors in original
        ;; implementation but we want to distinguish mutable / immutable
        ;; data so we will set leading bit to 1 for immutable
        (asm-defer (4)
         ;;;;(displayln (~a "Storing " (~v obj) " to backing store, object header at " (asm-label-pos label) ", vector data starts at " (asm-label-pos d3)))
         (asm-32
          (+ (*
              #x10000
              (+ #x8000
                 (+ 1 ; this header block
                    (quotient
                     (+ 3 (u8vector-length obj)) ;+3 for "ceil" in flooring division by 4
                     4))))
             (asm-label-pos label)))) ; pointer to normal object header
        (asm-label d3) ; normal object header points to contents.
        (asm-bytes obj)] ; doesn't handle multi-byte encodings nicely
       [(vector? obj)
        (asm-align 4 0)
        ;; header: leading 2 bits unused by RAM vectors in original
        ;; implementation but we want to distinguish mutable / immutable
        ;; data so we will set leading bit to 1 for immutable
        (let ([n-obj (vector-length obj)])
         (asm-defer (4)
           ;;;;(displayln (~a "Storing " (~v obj) " to backing store, object header at " (asm-label-pos label) ", vector data starts at " (asm-label-pos d3)))
           (asm-32
            (+ (*
                #x10000
                (+ #x8000
                   (+ 1 ; this header block
                      (quotient
                       (+ 1 n-obj) ;+1 for "ceil" in flooring division by 2
                       2))))
               (asm-label-pos label)))) ; pointer to normal object header
         (asm-label d3) ; normal object header points to contents.
         (let ([asm-sub-obj
                (lambda (sub-obj i)
                  (asm-16
                   (+
                    (encode-constant sub-obj constants)
                    ; we want our objects in order so that ref/set! are
                    ; easy to implement, but we also want it to look like a pair
                    ; in memory for convenience so we set the pair bit in whichever
                    ; reference will be car depending on endianness
                    (if (or
                         (and (even? i) asm-big-endian?)
                         (and (odd? i) (not asm-big-endian?))) #x8000 #x0000))))])
           (for ([sub-obj obj]
                 [i (in-range n-obj)])
             (asm-sub-obj sub-obj i))
           (when (odd? n-obj)
             (asm-sub-obj (encode-direct #f) n-obj))))] 
       [else
        (void)])]))


(define (assemble code port big-endian?)

  ;; Collect constants and globals
  (define-values
    (constants globals labels)
    (for/fold ([constants (predef-constants)]
               [globals   (predef-globals)]
               [labels    '()])
        ([instr code])
      (match instr
        [(? number? instr)
         (values constants
                 globals
                 (dict-set labels instr (asm-make-label 'label)))]
        [`(push-constant ,arg)
         (values (add-constant arg constants #t)
                 globals
                 labels)]
        [`(,(or 'push-global 'set-global) ,arg)
         (values constants
                 (add-global arg globals)
                 labels)]
        [_
         (values constants globals labels)])))

  ;;;;(display (~v constants)) (newline)
  ;;;;(display (~v globals)) (newline)
  ;; Constants and globals are sorted by frequency of reference.
  ;; That way, the most often referred to constants and globals get
  ;; the lowest encodings. Low encodings mean that they can be
  ;; pushed/set with short instructions, reducing overall code size.
  (let* ((constants (sort-constants constants))
         (globals   (sort-globals   globals))
         (num-constants (length constants)))

    (asm-begin! code-start big-endian?)

    ;; Header.
    (asm-16 (+ #xfbd8 (quotient num-constants 256)))
    (asm-8 (remainder num-constants 256))
    (asm-8 (length globals))

    ;; Constants.
    ;(displayln (~a "encoding " num-constants " constants"))
    (for ([c (in-list constants)])
      (assemble-constant c constants))

    ;; Program.
    (for ([instr (in-list code)])
      (when (not (number? instr))
        (inc-instr-count! (car instr)))
      (match instr
        [(? number? instr)
         (asm-label (dict-ref labels instr))]
        [`(entry ,np ,rest?)
         (asm-8 (if rest? (- np) np))]
        [`(push-constant ,n)
         (push-constant (encode-constant n constants))]
        [`(push-stack ,arg)
         (push-stack arg)]
        [`(push-global ,arg)
         (push-global (vector-ref (dict-ref globals arg) 0))]
        [`(set-global ,arg)
         (set-global  (vector-ref (dict-ref globals arg) 0))]
        [`(call ,arg)
         (call arg)]
        [`(jump ,arg)
         (jump arg)]
        [`(call-toplevel ,arg)
         (call-toplevel (dict-ref labels arg))]
        [`(jump-toplevel ,arg)
         (jump-toplevel (dict-ref labels arg))]
        [`(goto          ,arg)
         (goto          (dict-ref labels arg))]
        [`(goto-if-false ,arg)
         (goto-if-false (dict-ref labels arg))]
        [`(closure       ,arg)
         (closure       (dict-ref labels arg))]
        [`(prim ,p)
         (prim (dict-ref primitive-encodings p
                         (lambda ()
                           (compiler-error "unknown primitive" p))))]
        ['(return)
         (prim (dict-ref primitive-encodings 'return))]
        ['(pop)
         (prim (dict-ref primitive-encodings 'pop))]
        [_
         (compiler-error "unknown instruction" instr)]))
    ;; text + byte data
    (for ([c (in-list constants)])
      (assemble-byte-buf c constants))

    (asm-assemble)

    (when (stats?)
      (pretty-print
       (sort (hash->list instr-table) > #:key cdr)))

    (begin0 (asm-write-hex port)
      (asm-end!))))
