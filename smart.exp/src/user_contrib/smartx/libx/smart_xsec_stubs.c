/*
 * smart_xsec_stubs.c - Notify and event callback function stubs.
 * This file was generated by `gxv' from `smart_xsec.G'.
 * DO NOT EDIT BY HAND.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <gdd.h>
#include "smart_xsec_ui.h"

#ifdef MAIN

/*
 * Instance XV_KEY_DATA key.  An instance is a set of related
 * user interface objects.  A pointer to an object's instance
 * is stored under this key in every object.  This must be a
 * global variable.
 */
Attr_attribute	INSTANCE;

void
main(argc, argv)
	int		argc;
	char		**argv;
{
	smart_xsec_document_popup_objects	*smart_xsec_document_popup;
	smart_xsec_help_popup_objects	*smart_xsec_help_popup;
	smart_xsec_saveall_popup_objects	*smart_xsec_saveall_popup;
	smart_xsec_savelast_popup_objects	*smart_xsec_savelast_popup;
	smart_xsec_advance_popup_objects	*smart_xsec_advance_popup;
	
	/*
	 * Initialize XView.
	 */
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 0);
	INSTANCE = xv_unique_key();
	
	/*
	 * Initialize user interface components.
	 */
	smart_xsec_document_popup = smart_xsec_document_popup_objects_initialize(NULL, NULL);
	smart_xsec_help_popup = smart_xsec_help_popup_objects_initialize(NULL, NULL);
	smart_xsec_saveall_popup = smart_xsec_saveall_popup_objects_initialize(NULL, NULL);
	smart_xsec_savelast_popup = smart_xsec_savelast_popup_objects_initialize(NULL, NULL);
	smart_xsec_advance_popup = smart_xsec_advance_popup_objects_initialize(NULL, NULL);
	
	/*
	 * Turn control over to XView.
	 */
	xv_main_loop(smart_xsec_document_popup->document_popup);
	exit(0);
}

#endif

/*
 * Event callback function for `docnext_button'.
 */
void
x_docnext_document(item, event)
	Panel_item	item;
	Event		*event;
{
	smart_xsec_document_popup_objects	*ip = (smart_xsec_document_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	fprintf(stderr, "smart_xsec: x_docnext_document: event %d\n", event_id(event));
	panel_default_handle_event(item, event);
}

/*
 * Event callback function for `docdone_button'.
 */
void
close_doc_popup(item, event)
	Panel_item	item;
	Event		*event;
{
	smart_xsec_document_popup_objects	*ip = (smart_xsec_document_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	fprintf(stderr, "smart_xsec: close_doc_popup: event %d\n", event_id(event));
	panel_default_handle_event(item, event);
}

/*
 * Event callback function for `helpdone_button'.
 */
void
close_help_popup(item, event)
	Panel_item	item;
	Event		*event;
{
	smart_xsec_help_popup_objects	*ip = (smart_xsec_help_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	fprintf(stderr, "smart_xsec: close_help_popup: event %d\n", event_id(event));
	panel_default_handle_event(item, event);
}

/*
 * Event callback function for `saveall_ok_button'.
 */
void
x_saveall(item, event)
	Panel_item	item;
	Event		*event;
{
	smart_xsec_saveall_popup_objects	*ip = (smart_xsec_saveall_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	fprintf(stderr, "smart_xsec: x_saveall: event %d\n", event_id(event));
	panel_default_handle_event(item, event);
}

/*
 * Event callback function for `savelast_ok_button'.
 */
void
x_savelast(item, event)
	Panel_item	item;
	Event		*event;
{
	smart_xsec_savelast_popup_objects	*ip = (smart_xsec_savelast_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	fprintf(stderr, "smart_xsec: x_savelast: event %d\n", event_id(event));
	panel_default_handle_event(item, event);
}

/*
 * Event callback function for `advdone_button'.
 */
void
close_adv_popup(item, event)
	Panel_item	item;
	Event		*event;
{
	smart_xsec_advance_popup_objects	*ip = (smart_xsec_advance_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	fprintf(stderr, "smart_xsec: close_adv_popup: event %d\n", event_id(event));
	panel_default_handle_event(item, event);
}

