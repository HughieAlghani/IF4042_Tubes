;GIL-2
(
(
	:type                   :menu
	:name                   quit_menu
	:help                   ""
	:columns                1
	:label                  ""
	:label-type             :string
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Quit"
	:menu-item-labels       ("quit" "xit" )
	:menu-item-label-types  (:string :string )
	:menu-item-defaults     (t nil )
	:menu-item-handlers     (x_quit x_xit )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               t
	:user-data              ()
)
(
	:type                   :menu
	:name                   save_menu
	:help                   ""
	:columns                1
	:label                  ""
	:label-type             :string
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Save"
	:menu-item-labels       ("Save last command" "        Save all" )
	:menu-item-label-types  (:string :string )
	:menu-item-defaults     (t nil )
	:menu-item-handlers     (savelast_popup saveall_popup )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               t
	:user-data              ()
)
(
	:type                   :menu
	:name                   advanced_menu
	:help                   ""
	:columns                1
	:label                  ""
	:label-type             :string
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "advanced functions"
	:menu-item-labels       ("Raw_doc" "Top_ranked" "Location" "Spec_file" "Dtext" "Dvec" "Drun" "Dsim" "Dmatch" "Qtext" "Qvec" "Qsim" "Qmatch" )
	:menu-item-label-types  (:string :string :string :string :string :string :string :string :string :string :string :string :string )
	:menu-item-defaults     (nil nil nil nil nil nil nil nil nil t nil nil nil )
	:menu-item-handlers     (x_raw_doc x_top_ranked x_location x_spec_file x_dtext x_dvec x_drun x_dsim x_dmatch x_qtext x_qvec x_qsim x_qmatch )
	:menu-item-menus        (nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" "" "" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
)
(
	:type                   :base-window
	:name                   basewindow
	:owner                  nil
	:width                  643
	:height                 683
	:background-color       ""
	:foreground-color       ""
	:label                  "Smart Text Retrieval System"
	:label-type             :string
	:mapped                 t
	:show-footer            t
	:resizable              t
	:icon-file              ""
	:icon-mask-file         ""
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :control-area
	:name                   main_control
	:owner                  basewindow
	:help                   ""
	:x                      0
	:y                      0
	:width                  640
	:height                 96
	:background-color       ""
	:foreground-color       ""
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   retrieve
	:owner                  main_control
	:help                   "retrieve:
retrieve new query from the text pane. Retrieve
only after a new query had typed in."
	:x                      32
	:y                      16
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  " Retrieve"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          x_retrieve
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   more
	:owner                  main_control
	:help                   "more:
retrieve more documents for this query, using only original query."
	:x                      144
	:y                      16
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "   More"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          x_more_documents
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   keywords
	:owner                  main_control
	:help                   "keywords:
text searching using keywords."
	:x                      256
	:y                      16
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "Keywords"
	:label-type             :string
	:menu                   nil
	:notify-handler         x_keywords
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   advanced
	:owner                  main_control
	:help                   "advanced:
Display help message describingother advanced options. "
	:x                      371
	:y                      16
	:constant-width         t
	:button-type            :normal
	:width                  85
	:height                 19
	:foreground-color       ""
	:label                  "Advanced"
	:label-type             :string
	:menu                   advanced_menu
	:notify-handler         nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   quit
	:owner                  main_control
	:help                   "quit:
quit Smart.
"
	:x                      496
	:y                      16
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "   Quit"
	:label-type             :string
	:menu                   quit_menu
	:notify-handler         nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   feedback
	:owner                  main_control
	:help                   "feedback <usefuldoc1> <usefuldoc2> ...:
Retrieve more documents for this query, using original query plus the given list of useful documents."
	:x                      32
	:y                      48
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "Feedback"
	:label-type             :string
	:menu                   nil
	:notify-handler         x_feedback
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   next
	:owner                  main_control
	:help                   "next:
show the next ranked document."
	:x                      144
	:y                      48
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "   Next"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          x_next_document
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   clear
	:owner                  main_control
	:help                   "clear:
Clears the user query text pane for new query."
	:x                      256
	:y                      48
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "   Clear"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          clear_usertext
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   help
	:owner                  main_control
	:help                   "help:
displays all the functions of smart."
	:x                      371
	:y                      48
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "   Help"
	:label-type             :string
	:menu                   nil
	:notify-handler         x_help
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   save
	:owner                  main_control
	:help                   "save:
save_last_command:  append copy of output of last command to the named file.

save_all: append all transactions to the named file."
	:x                      496
	:y                      48
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "   Save"
	:label-type             :string
	:menu                   save_menu
	:notify-handler         nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :message
	:name                   general_msg
	:owner                  main_control
	:help                   "contains user messages from the program."
	:x                      8
	:y                      72
	:width                  465
	:height                 13
	:foreground-color       ""
	:label                  "Enter natural language query below and press the Retrieve Button.  "
	:label-type             :string
	:label-bold             t
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :text-pane
	:name                   user_textpane
	:owner                  basewindow
	:help                   ""
	:x                      0
	:y                      92
	:width                  640
	:height                 152
	:background-color       ""
	:foreground-color       ""
	:show-border            t
	:read-only              nil
	:event-handler          x_hilite_button
	:events                 ()
	:user-data              ()
)
(
	:type                   :control-area
	:name                   main_control2
	:owner                  basewindow
	:help                   ""
	:x                      0
	:y                      249
	:width                  640
	:height                 500
	:background-color       ""
	:foreground-color       ""
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :scrolling-list
	:name                   title_list
	:owner                  main_control2
	:help                   ""
	:x                      0
	:y                      5
	:width                  620
	:height                 415
	:foreground-color       ""
	:label                  "  Num Action      Sim      Title"
	:label-type             :string
	:layout-type            :vertical
	:rows                   21
	:read-only              t
	:multiple-selections    t
	:selection-required     t
	:menu                   nil
	:notify-handler         show_document
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
)
