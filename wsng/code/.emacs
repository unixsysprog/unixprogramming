;; Here's a sample .emacs file that might help you along the way.  Just
;; copy this region and paste it into your .emacs file.  You may want to
;; change some of the actual values.

      (defconst my-c-style
        '((c-tab-always-indent        . t)
          (c-comment-only-line-offset . 4)
          (c-hanging-braces-alist     . ((substatement-open after)
                                         (brace-list-open)))
          (c-hanging-colons-alist     . ((member-init-intro before)
                                         (inher-intro)
                                         (case-label after)
                                         (label after)
                                         (access-label after)))
          (c-cleanup-list             . (scope-operator
                                         empty-defun-braces
                                         defun-close-semi))
          (c-offsets-alist            . ((arglist-close . c-lineup-arglist)
                                         (substatement-open . 0)
                                         (case-label        . 4)
                                         (block-open        . 0)
					(defun-block-intro . 0)
                                         (statement-block-intro . 8)
                                         (substatement . 8)
                                         (knr-argdecl-intro . -)))
          (c-echo-syntactic-information-p . t)
          )
        "My C Programming Style")

      ;; Customizations for all of c-mode, c++-mode, and objc-mode
      (defun my-c-mode-common-hook ()
        ;; add my personal style and set it for the current buffer
        (c-add-style "PERSONAL" my-c-style t)
        ;; offset customizations not in my-c-style
        (c-set-offset 'defun-block-intro' +)
        ;; other customizations
        (setq tab-width 8
              ;; this will make sure spaces are used instead of tabs
              indent-tabs-mode nil)
        ;; we like auto-newline and hungry-delete
        (c-toggle-auto-hungry-state 1)
        ;; keybindings for all supported languages.  We can put these in
        ;; c-mode-base-map because c-mode-map, c++-mode-map, objc-mode-map,
        ;; java-mode-map, and idl-mode-map inherit from it.
        (define-key c-mode-base-map "\C-m" 'newline-and-indent)
        )

      (add-hook 'c-mode-common-hook 'my-c-mode-common-hook)

