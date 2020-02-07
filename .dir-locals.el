((nil . ((eval . (set (make-local-variable 'my-project-path)
                      (file-name-directory
                       (let ((d (dir-locals-find-file ".")))
                         (if (stringp d) d (car d))))))
         (eval . (when (not (boundp 'ccls-args))
                   (setq ccls-args (list))))
         (eval . (setq ccls-args (append ccls-args `("--init" ,(format "{\"compilationDatabaseDirectory\": \"%s/build\"}" my-project-path))))))))
