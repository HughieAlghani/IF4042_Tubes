;GIL-2
(
(
	:type                   :popup-window
	:name                   document_popup
	:owner                  nil
	:width                  639
	:height                 449
	:background-color       ""
	:foreground-color       ""
	:label                  "Document"
	:label-type             :string
	:mapped                 nil
	:show-footer            t
	:resizable              t
	:pinned                 nil
	:done-handler           nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :control-area
	:name                   doc_popup_control
	:owner                  document_popup
	:help                   ""
	:x                      0
	:y                      0
	:width                  639
	:height                 64
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
	:name                   docnext_button
	:owner                  doc_popup_control
	:help                   ""
	:x                      20
	:y                      12
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "   Next"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          x_docnext_document
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   docdone_button
	:owner                  doc_popup_control
	:help                   ""
	:x                      112
	:y                      12
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "    Done"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          close_doc_popup
	:events                 ()
	:user-data              ()
)
(
	:type                   :text-pane
	:name                   doc_textpane
	:owner                  document_popup
	:help                   ""
	:x                      0
	:y                      64
	:width                  639
	:height                 385
	:background-color       ""
	:foreground-color       ""
	:show-border            t
	:read-only              t
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :popup-window
	:name                   help_popup
	:owner                  nil
	:width                  479
	:height                 447
	:background-color       ""
	:foreground-color       ""
	:label                  "    Help "
	:label-type             :string
	:mapped                 nil
	:show-footer            t
	:resizable              t
	:pinned                 nil
	:done-handler           nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :control-area
	:name                   help_popup_conrol
	:owner                  help_popup
	:help                   ""
	:x                      0
	:y                      0
	:width                  479
	:height                 56
	:background-color       ""
	:foreground-color       ""
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :message
	:name                   helppopup_msg
	:owner                  help_popup_conrol
	:help                   ""
	:x                      4
	:y                      12
	:width                  128
	:height                 13
	:foreground-color       ""
	:label                  "SMART Commands"
	:label-type             :string
	:label-bold             t
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   helpdone_button
	:owner                  help_popup_conrol
	:help                   ""
	:x                      196
	:y                      24
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "   Done"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          close_help_popup
	:events                 ()
	:user-data              ()
)
(
	:type                   :text-pane
	:name                   help_textpane
	:owner                  help_popup
	:help                   ""
	:x                      0
	:y                      52
	:width                  479
	:height                 395
	:background-color       ""
	:foreground-color       ""
	:show-border            t
	:read-only              t
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :popup-window
	:name                   saveall_popup
	:owner                  nil
	:width                  400
	:height                 150
	:background-color       ""
	:foreground-color       ""
	:label                  "Saveall"
	:label-type             :string
	:mapped                 nil
	:show-footer            t
	:resizable              t
	:pinned                 nil
	:done-handler           nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :control-area
	:name                   saveall_popup_control
	:owner                  saveall_popup
	:help                   ""
	:x                      0
	:y                      0
	:width                  400
	:height                 150
	:background-color       ""
	:foreground-color       ""
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :text-field
	:name                   saveall_dirtext
	:owner                  saveall_popup_control
	:help                   ""
	:x                      16
	:y                      24
	:width                  315
	:height                 15
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Directory:"
	:label-type             :string
	:value-x                91
	:value-y                24
	:layout-type            :horizontal
	:value-length           30
	:stored-length          30
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :text-field
	:name                   saveall_filetext
	:owner                  saveall_popup_control
	:help                   ""
	:x                      16
	:y                      64
	:width                  315
	:height                 15
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Filename:"
	:label-type             :string
	:value-x                91
	:value-y                64
	:layout-type            :horizontal
	:value-length           30
	:stored-length          30
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   saveall_ok_button
	:owner                  saveall_popup_control
	:help                   ""
	:x                      152
	:y                      96
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "    OK"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          x_saveall
	:events                 ()
	:user-data              ()
)
(
	:type                   :popup-window
	:name                   savelast_popup
	:owner                  nil
	:width                  400
	:height                 150
	:background-color       ""
	:foreground-color       ""
	:label                  "Save Last Command"
	:label-type             :string
	:mapped                 nil
	:show-footer            t
	:resizable              t
	:pinned                 nil
	:done-handler           nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :control-area
	:name                   savelast_popup_control
	:owner                  savelast_popup
	:help                   ""
	:x                      0
	:y                      0
	:width                  400
	:height                 150
	:background-color       ""
	:foreground-color       ""
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :text-field
	:name                   savelast_dirtext
	:owner                  savelast_popup_control
	:help                   ""
	:x                      24
	:y                      32
	:width                  315
	:height                 15
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Directory:"
	:label-type             :string
	:value-x                99
	:value-y                32
	:layout-type            :horizontal
	:value-length           30
	:stored-length          30
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :text-field
	:name                   savelast_filetext
	:owner                  savelast_popup_control
	:help                   ""
	:x                      24
	:y                      72
	:width                  315
	:height                 15
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Filename:"
	:label-type             :string
	:value-x                99
	:value-y                72
	:layout-type            :horizontal
	:value-length           30
	:stored-length          30
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :button
	:name                   savelast_ok_button
	:owner                  savelast_popup_control
	:help                   ""
	:x                      152
	:y                      104
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "    OK"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          x_savelast
	:events                 ()
	:user-data              ()
)
(
	:type                   :popup-window
	:name                   advance_popup
	:owner                  nil
	:width                  605
	:height                 648
	:background-color       ""
	:foreground-color       ""
	:label                  "Advanced Functions"
	:label-type             :string
	:mapped                 nil
	:show-footer            t
	:resizable              t
	:pinned                 nil
	:done-handler           nil
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
(
	:type                   :control-area
	:name                   adv_popup_control
	:owner                  advance_popup
	:help                   ""
	:x                      0
	:y                      0
	:width                  605
	:height                 40
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
	:name                   advdone_button
	:owner                  adv_popup_control
	:help                   ""
	:x                      256
	:y                      12
	:constant-width         t
	:button-type            :normal
	:width                  77
	:height                 19
	:foreground-color       ""
	:label                  "    Done"
	:label-type             :string
	:menu                   nil
	:notify-handler         nil
	:event-handler          close_adv_popup
	:events                 ()
	:user-data              ()
)
(
	:type                   :text-pane
	:name                   doc_textpane1
	:owner                  advance_popup
	:help                   ""
	:x                      0
	:y                      40
	:width                  605
	:height                 608
	:background-color       ""
	:foreground-color       ""
	:show-border            t
	:read-only              t
	:event-handler          nil
	:events                 ()
	:user-data              ()
)
)
