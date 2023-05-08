#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/dict.c,v 11.0 1992/07/21 18:20:47 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <stdio.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "rel_header.h"
#include "dict.h"
#include "io.h"
#include "smart_error.h"

/* Utility programs for accessing dictionary relations in unix files */
/*    The dictionary relations contain information about each term int the*/
/*    collection that has been indexed.*/
/**/
/**/
/*Procedures include:*/
/*  int seek_dict (file_index, dict_entry);*/
/*          int file_index, DICT_ENTRY *dict_entry;*/
/*      Returns 1 if a dict_entry is found which matches the given information*/
/*      Returns 0 otherwise. If found, position in dictionary set to*/
/*      that entry for future read or write. If not found, position set to*/
/*      entry for insertion (write) of dict_entry with that information.*/
/*      <file_index> must have been returned by open_dict.*/
/*      <dict_entry.con> must be UNDEF or known concept number.*/
/*      <dict_entry.token> must be NULL, "" or known character string */
/*              (max length is MAX_TOKEN_LEN, currently 1024)*/
/*      Either <con> or <token> should be known, If not,*/
/*      UNDEF is returned and position is undefined. */
/*      If dict_entry is NULL, a seek is performed to the beginning of the*/
/*      file and 1 is returned.*/
/**/
/*  int read_dict (file_index, dict_entry);*/
/*          int file_index, DICT_ENTRY *dict_entry;*/
/*      Returns the number of entries actually put into <dict_entry> (1 or 0).*/
/*      Reads previously opened dictionary at current position,*/
/*      putting entries read in <dict_entry>.*/
/*      <file_index> must have been returned by open_dict.*/
/**/
/*  int write_dict(file_index, dict_entry);*/
/*          int file_index, DICT_ENTRY *dict_entry;*/
/*      Returns actual number of dict_entry's written (1 or 0).*/
/*      <file_index> must have been returned by open_dict with mode "a"*/
/*      close_dict must be called to finally commit all written entries.*/
/**/
/*      close_dict(file_index);*/
/*      <file_index> must have been returned by open_dict.*/
/*      Closes all UNIX files, writing out those which were kept in core.*/
/**/
/*Interior forms (from dict.h) :*/
/*       typedef struct {*/
/*           char  *token;       actual string */
/*           long info;          Usage dependant value */
/*           long  con;          unique index for this token ctype pair */
/*       } DICT_ENTRY;*/
/*    */
/**/
/*File format:*/
/*    Dictionary implemented by a hash table. See the actual routines for*/
/*    those implementation details. There are is one+ physical files*/
/*    associated with the present implementation.*/
/*          <file_name>    The hash table implementation of main dictionary.*/
/*          <file_name>    (second part) The actual strings of entries.*/
/*          <file_name>.?? Overflow entries (used only when <file_name>*/
/*                          is full).*/
/*    The present format of a hash entry (from "dict.h")*/
/*       typedef struct hash_entry {*/
/*           short collision_ptr;      offset of next word hashed here   */
/*                                     If IN_OVF_TABLE, then need to go to*/
/*                                     (next) overflow hash table */
/*           char prefix[2];           First two bytes of string */
/*           long str_tab_off;         position of string */
/*           long info;                Usage dependent filed */
/*       } HASH_ENTRY;*/
/*Notes:*/
/*    close_dict must be used in order to write out a file that was opened*/
/*    with mode SWRONLY or SRDWR*/


_SDICT _Sdict[MAX_NUM_DICT];


