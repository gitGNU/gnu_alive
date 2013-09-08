#!@GUILE@ -s
;;; alive --- periodically ping some hosts

;; Copyright (C) 2012 Thien-Thi Nguyen
;;
;; This file is part of GNU Alive.
;;
;; GNU Alive is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; GNU Alive is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GNU Alive.  If not, see <http://www.gnu.org/licenses/>.
!#
;;; Commentary:

;; GNU Alive documentation is available from:
;;
;; - the command-line
;;   $ info alive
;;
;; - Emacs
;;   evaluate this form w/ ‘M-x eval-last-sexp’: (info "(alive)")
;;   or type ‘C-h i d m alive RET’.
;;
;; - possibly other places (search for alive.html or alive.pdf)
;;
;; Report bugs to <@PACKAGE_BUGREPORT@>.

;;; Code:

(define ARGV (list->vector (command-line)))
(define ARGC (vector-length ARGV))

(define (argv n)
  (vector-ref ARGV n))

(define (whoami)
  (basename (argv 0)))

(and (= 2 ARGC)
     (let ((me (whoami)))
       (define (finish . ls)
         (for-each display ls)
         (newline)
         (exit #t))
       (case (string->symbol (argv 1))
         ((--version)
          (finish me " (@PACKAGE_NAME@) @PACKAGE_VERSION@
Copyright (C) 2012 Thien-Thi Nguyen
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law."))
         ((--help)
          (finish "Usage: " me " [option]

Options:
     --help             Display this message.
     --version          Display version and license info.

Report bugs to: <@PACKAGE_BUGREPORT@>
Home page: <http://www.gnu.org/software/alive/>
General help using GNU software: <http://www.gnu.org/gethelp/>")))))

(or (= 1 ARGC)
    (let ((me (whoami)))
      (for-each display (list me ": Unexpected argument (try --help)"))
      (newline)
      (exit #f)))

(use-modules
 ((srfi srfi-1) #:select (circular-list
                          length+
                          car+cdr))
 ((srfi srfi-11) #:select (let-values))
 ((srfi srfi-13) #:select (string-index
                           substring/shared
                           string-concatenate-reverse))
 ((srfi srfi-14) #:select (char-set-complement
                           char-set-union
                           char-set:letter+digit
                           char-set))
 ((ice-9 popen) #:select (open-input-pipe
                          close-pipe))
 ((ice-9 rdelim) #:select (read-line))
 ((ice-9 regex) #:select (match:suffix)))

(define (fs s . args)
  (apply simple-format #f s args))

(define (fso s . args)
  (apply simple-format #t s args))

(define (at moment)
  ;; ‘%F’ is C99, probably will be OK in a few years.
  ;; For now, stick w/ the classic.
  ;; TODO: Make format string a configuration item.
  (strftime "%Y-%m-%d %T" (localtime (or moment (current-time)))))

(define (ok-dir dir)
  (and dir
       (file-exists? dir)
       dir))

(define (config-dir-a-la-XDG)
  (false-if-exception
   (assq-ref (read (open-input-pipe "xdgdirs alive"))
             'config-home)))

(define config-item
  (let ((dir (or (ok-dir (in-vicinity (getenv "HOME") ".alive.d"))
                 (ok-dir (config-dir-a-la-XDG))
                 *null-device*)))
    ;; config-item
    (lambda (nick)
      (let ((filename (in-vicinity dir nick))
            (mtime #f))

        (define (forms)
          (false-if-exception
           (call-with-input-file filename
             (lambda (port)
               (let loop ((acc '()))
                 (let ((form (read port)))
                   (if (eof-object? form)
                       (reverse! acc)
                       (loop (cons form acc)))))))))

        (define (probe)
          (define (simply x)
            (values x #f))
          (cond ((and (file-exists? filename)
                      (stat:mtime (stat filename)))
                 => (lambda (new-mtime)
                      (cond ((eqv? mtime new-mtime)
                             (simply 'no-change))
                            (else
                             (set! mtime new-mtime)
                             (values mtime (forms))))))
                ((eqv? 0 mtime)
                 (simply 'still-unspecified))
                (else
                 (set! mtime 0)
                 (simply 'unspecified))))

        (define (nb! moment s . args)
          (fso "(~A ~A) " (at moment) nick)
          (apply fso s args)
          (newline))

        ;; rv
        (lambda (command)
          (case command
            ((nb!) nb!)
            (else (call-with-values probe command))))))))

(define next-host
  (let* ((ci (config-item "hosts"))
         (nb! (ci 'nb!))
         (hosts (cons #f #f)))

    (define (replace! . ls)
      ;; We can't resist a little coddling.
      (set-cdr! hosts #f)
      (set! hosts (apply circular-list ls)))

    (define (lonely! moment reason)
      (let ((lh "localhost"))
        (nb! moment (fs "~A, falling back to ~A" reason lh))
        (replace! lh)))

    (define (re-scan mtime hosts)
      (case mtime
        ((still-unspecified no-change)
         ;; do nothing
         #f)
        ((unspecified)
         (lonely! #f "unspecified"))
        (else
         (cond ((string? hosts)
                (lonely! mtime hosts))
               ((null? hosts)
                (lonely! mtime "no hosts"))
               ((and (pair? hosts)
                     (let ((count (length+ hosts)))
                       (and (integer? count)
                            (and-map (lambda (x)
                                       (or (symbol? x)
                                           (string? x)))
                                     hosts)
                            (fs "~A hosts" count))))
                => (lambda (blurb)
                     (nb! mtime blurb)
                     (apply replace! hosts)))
               (else
                (lonely! mtime "invalid 'hosts' spec"))))))

    ;; next-host
    (lambda ()
      (ci re-scan)
      (let ((one (car hosts)))
        (set! hosts (cdr hosts))
        one))))

(define some-seconds
  (let* ((ci (config-item "period"))
         (nb! (ci 'nb!))
         (period #f))

    (define (random! moment reason)
      (nb! moment (fs "~A, using random value" reason)))

    (define (range! lo hi)
      (set! period (cons lo hi)))

    (define (standard-range!)
      (range! 42 420))

    (define (re-scan mtime spec)
      (define (well-formed? len)
        (and (pair? spec)
             (integer? (length+ spec))
             (= len (length spec))
             (and-map integer? spec)
             (and-map positive? spec)))
      (case mtime
        ((still-unspecified no-change)
         ;; do nothing
         #f)
        ((unspecified)
         (random! #f mtime)
         (standard-range!))
        (else
         (cond ((string? spec)
                (random! mtime spec)
                (standard-range!))
               ((well-formed? 1)
                (set! period (car spec))
                (nb! mtime "~A seconds" period))
               ((and (well-formed? 2)
                     ;; low first, high after
                     (apply <= spec))
                (apply nb! mtime "random in range [~A, ~A]" spec)
                (range! (car spec) (cadr spec)))
               (else
                (random! mtime "invalid 'period' spec")
                (standard-range!))))))

    ;; some-seconds
    (lambda ()
      (ci re-scan)
      (if (integer? period)
          period
          (let-values (((lo hi) (car+cdr period)))
            ;; Widen by one for a doubly-inclusive range.
            (+ lo (random (- hi lo -1))))))))

(define ping!
  (let ((rx (make-regexp "^.* from ")))

    (define shell-quote-argument
      (let ((funky (char-set-complement
                    (char-set-union char-set:letter+digit
                                    (char-set #\@ #\/ #\:
                                              #\. #\- #\_
                                              ;; Add chars here.
                                              )))))

        (define (funkiness start string)
          (string-index string funky start))

        ;; shell-quote-argument
        (lambda (arg)
          (let ((string (if (symbol? arg)
                            (symbol->string arg)
                            arg)))
            (let loop ((start 0) (acc '()))
              (cond ((funkiness start string)
                     => (lambda (pos)

                          (define (subs beg end)
                            (substring/shared string beg end))

                          (loop (1+ pos)
                                (cons* (subs pos (1+ pos))
                                       "\\"
                                       (subs start pos)
                                       acc))))
                    (else
                     (string-concatenate-reverse
                      acc (substring/shared string start)))))))))

    ;; ping!
    (lambda (host)
      (let* ((port (open-input-pipe
                    ;; TODO: Make ping program (and its output parsing)
                    ;;       a configuration item.
                    (fs "@PING@ -n -c 1 ~A 2>&1"
                        (shell-quote-argument host))))
             ;; first two lines of output
             (one (read-line port))
             (two (read-line port)))
        (close-pipe port)
        (fso "~A | ~A~%"
             host (cond
                   ;; error from ping program
                   ((eof-object? two) one)
                   ;; decrufted
                   ((regexp-exec rx two) => match:suffix)
                   ;; raw
                   (else two)))))))

(define (nb! s . args)
  (fso "~A: ~A " (whoami) (at (current-time)))
  (apply fso s args)
  (newline))

(set! *random-state* (let ((now (gettimeofday)))
                       (seed->random-state (* (car now)
                                              (cdr now)))))

;; install signal handlers
(let ((numeric-to-symbolic-map
       ;; FIXME: Guile should provide this!
       `((,SIGHUP . SIGHUP)
         (,SIGINT . SIGINT)
         (,SIGQUIT . SIGQUIT)
         (,SIGALRM . SIGALRM)
         (,SIGTERM . SIGTERM)
         (,SIGUSR1 . SIGUSR1))))

  (define (got-signal signo)
    ;; FIXME: Guile should provide numeric to named map.
    (let ((named (assq-ref numeric-to-symbolic-map signo)))
      (nb! "received signal ~A~A" signo (if named
                                            (fs " (~A)" named)
                                            ""))
      named))

  (define (sigactions handler . ls)
    (for-each (lambda (signo)
                (sigaction signo handler))
              ls))

  (sigactions got-signal
    SIGALRM)

  (sigactions (lambda (signo)           ; restart
                (got-signal signo)
                (apply execlp (argv 0) (vector->list ARGV)))
    SIGHUP
    SIGUSR1)

  (sigactions (lambda (signo)           ; exit
                (got-signal signo)
                (nb! "exiting")
                (exit #t))
    SIGINT
    SIGQUIT
    SIGTERM))

(nb! "restart (pid ~A)~%" (getpid))

(let loop ()
  (let* ((secs (some-seconds))
         (bef (current-time))
         (aft (+ bef secs)))
    (ping! (next-host))
    ;; TODO: Make "verbosity" a configuration item.
    (fso "(~A)\t~A~%\t~A~%~%" secs (at bef) (at aft))
    (sleep secs)
    (loop)))

;;; Local variables:
;;; eval: (put 'sigactions 'scheme-indent-function 1)
;;; End:

;;; alive ends here
