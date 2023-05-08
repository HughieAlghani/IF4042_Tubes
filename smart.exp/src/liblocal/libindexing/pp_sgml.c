#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libindexing/pp_sgml.c,v 11.0 1992/07/21 18:22:07 chrisb Exp $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/******************************************************************
 *
 * Definitions of tables of constants used in the SGML parsing
 * (e.g., for the F&W encyclopedia).
 *
 * sgml_codes[]
 *	internal code and SGML string for various important SGML
 *	tags.
 *
 ******************************************************************/

#include "fw.h"

SGML_CODE sgml_codes[] =
{
/***** NOTE: Use alpha order by the words (binary search) *****/
    SYM_BDY,            "bdy",
    SYM_BIO,		"bio",
    SYM_CATEGORY,	"category",
    DATESYM_DAY,        "day",
    SYM_FW_ART,         "fw.art",
    SYM_MDT,            "mdt",
    SYM_LISTELT,	"li",
    SYM_LIST,		"list",
    DATESYM_MONTH,      "mo",
    SYM_NAME,		"name",
    SYM_NORMNAME,	"normname",
    SYM_OBSCURITY,	"obs",
    SYM_PAGE,           "page",
    SYM_PUBFM,          "pubfm",
    SYM_P,              "p",
    SYM_SEC,            "sec",
    SYM_SECA,           "seca",
    SYM_SECB,           "secb",
    SYM_SECC,           "secc",
    SYM_SUBCATEGORY,	"subcat",
    SYM_SUBH,		"subh",
    SYM_TI,             "ti",
    SYM_VOL,            "vol",
    SYM_XREF_ID,        "x",
    SYM_XREF,           "xr",
    DATESYM_YEAR,       "yr"
};

int sgml_COUNT = sizeof(sgml_codes) / sizeof(sgml_codes[0]);

/******************************************************************
 *
 * trans[]
 *	Gives a special character string (&xxx;) and a value for
 *	translating it into.  Requires the catchall at the end.
 *
 ******************************************************************/
SGML_CHAR trans[] = {
    { "&dollar;", "$"},
    { "&lsquo;", "`"},
    { "&rsquo;", "'"},
    { "&ldquo;", "\""},
    { "&rdquo;", "\""},
    { "&rdquo,;", "\","},
    { "&laquo;", "\""},
    { "&raquo;", "\""},


    { "&aacute;", "a"},         { "&Aacute;", "A"},
    { "&agrave;", "a"},         { "&Agrave;", "A"},  
    { "&acedil;", "a"},         { "&Acedil;", "A"},   
    { "&acirc;", "a"},          { "&Acirc;", "A"},   
    { "&auml;", "a"},           { "&Auml;", "A"},    
    { "&atilde;", "a"},         { "&Atilde;", "A"},  
    { "&abreve;", "a"},         { "&Abreve;", "A"},  
    { "&aring;", "a"},          { "&Aring;", "A"},   
    { "&amacr;", "a"},          { "&Amacr;", "A"},   
    { "&aogon;", "a"},          { "&Aogon;", "A"},   
    { "&acaron;", "a"},         { "&Acaron;", "A"},   
    { "&aelig;", "ae"},         { "&AElig;", "Ae"},  

    { "&Bmacr;", "B"},

    { "&cacute;", "c"},         { "&Cacute;", "C"},
    { "&cbreve;", "g"},		{ "&Cbreve;", "G"},
    { "&ccirc;", "c"}, 		{ "&Ccirc;", "C"}, 
    { "&ccaron;", "c"},		{ "&Ccaron;", "C"},
    { "&cdot;", "c"},  		{ "&Cdot;", "C"},  
    { "&ccedil;", "c"},		{ "&Ccedil;", "C"},
    { "&cgrave;", "c"},		{ "&Cgrave;", "C"},
    { "&Cmacr;", "C"},

    { "&dcaron;", "d"},         { "&Dcaron;", "D"},
    { "&dstrok;", "d"},		{ "&Dstrok;", "D"},
    { "&eth;", "d"},   		{ "&Eth;", "D"},   

    { "&eacute;", "e"}, 	{ "&Eacute;", "E"}, 
    { "&egrave;", "e"}, 	{ "&Egrave;", "E"}, 
    { "&ecedil;", "e"},  	{ "&Ecedil;", "E"},  
    { "&ecirc;", "e"},  	{ "&Ecirc;", "E"},  
    { "&euml;", "e"},   	{ "&Euml;", "E"},   
    { "&ecaron;", "e"}, 	{ "&Ecaron;", "E"}, 
    { "&edot;", "e"},   	{ "&Edot;", "E"},   
    { "&emacr;", "e"},  	{ "&Emacr;", "E"},  
    { "&eogon;", "e"},  	{ "&Eogon;", "E"},  

    { "&gacute;", "g"},
    { "&gcirc;", "g"},		{ "&Gcirc;", "G"},
    { "&gbreve;", "g"},		{ "&Gbreve;", "G"},
    { "&gdot;", "g"},  		{ "&Gdot;", "G"},  
    				{ "&Gcedil;", "G"},

    { "&hcirc;", "h"}, 		{ "&Hcirc;", "H"}, 
    { "&hstrok;", "h"},		{ "&Hstrok;", "H"},

    { "&iacute;", "i"},         { "&Iacute;", "I"},  
    { "&ibreve;", "i"},  	{ "&Ibreve;", "I"},  
    { "&igrave;", "i"},  	{ "&Igrave;", "I"},  
    { "&icirc;", "i"},   	{ "&Icirc;", "I"},   
    { "&iuml;", "i"},    	{ "&Iuml;", "I"},    
    { "&itilde;", "i"},  	{ "&Itilde;", "I"},  
    { "&inodot;", "i"},  	{ "&Idot;", "I"},  
    { "&imacr;", "i"},   	{ "&Imacr;", "I"},   
    { "&iogon;", "i"},   	{ "&Iogon;", "I"},   
    { "&ijlig;", "ij"},  	{ "&IJlig;", "IJ"},

    { "&jcirc;", "j"},		{ "*Jcirc;", "J"},

    { "&kcedil;", "k"},		{ "&Kcedil;", "K"},
    { "&kgreen;", "k"},

    { "&lacute;", "l"},		{ "&Lacute;", "L"},
    { "&lcaron;", "l"},		{ "&Lcaron;", "L"},
    { "&lcedil;", "l"},		{ "&Lcedil;", "L"},
    { "&lstrok;", "l"},		{ "&Lstrok;", "L"},
    { "&lmidot;", "l"},		{ "&Lmidot;", "L"},

    { "&nacute;", "n"},		{ "&Nacute;", "N"},
    { "&ntilde;", "n"},		{ "&Ntilde;", "N"},
    { "&ncaron;", "n"},		{ "&Ncaron;", "N"},
    { "&ncedil;", "n"},		{ "&Ncedil;", "N"},
    { "&eng;", "n"},   		{ "&ENG;", "N"},   
    { "&napos;", "n"},

    { "&oacute;", "o"},		{ "&Oacute;", "O"},
    { "&obreve;", "o"},		{ "&Obreve;", "O"},
    { "&ograve;", "o"},		{ "&Ograve;", "O"},
    { "&ocaron;", "o"},		{ "&Ocaron;", "O"}, 
    { "&ocirc;", "o"}, 		{ "&Ocirc;", "O"}, 
    { "&ouml;", "o"},  		{ "&Ouml;", "O"},  
    { "&otilde;", "o"},		{ "&Otilde;", "O"},
    { "&odblac;", "o"},		{ "&Odblac;", "O"},
    { "&omacr;", "o"}, 		{ "&Omacr;", "O"}, 
    { "&oelig;", "oe"},		{ "&OElig;", "Oe"},
    { "&oslash;", "o"},		{ "&Oslash;", "O"},

    { "&racute;", "r"},		{ "&Racute;", "R"},
    { "&rcaron;", "r"},		{ "&Rcaron;", "R"},
    { "&rcedil;", "r"},		{ "&Rcedil;", "R"},

    { "&sacute;", "s"},		{ "&Sacute;", "S"},
    { "&scirc;", "s"}, 		{ "&Scirc;", "S"}, 
    { "&scaron;", "s"},		{ "&Scaron;", "S"},
    { "&scedil;", "s"},		{ "&Scedil;", "S"},
    { "&szlig;", "s"},

    { "&tcaron;", "t"}, 	{ "&Tcaron;", "T"}, 
    { "&tcedil;", "t"}, 	{ "&Tcedil;", "T"}, 
    { "&tstrok;", "t"}, 	{ "&Tstrok;", "T"}, 
    { "&thorn;", "t"},  	{ "&THORN;", "T"},  

    { "&uacute;", "u"}, 	{ "&Uacute;", "U"}, 
    { "&ugrave;", "u"}, 	{ "&Ugrave;", "U"}, 
    { "&ucirc;", "u"},  	{ "&Ucirc;", "U"},  
    { "&uuml;", "u"},   	{ "&Uuml;", "U"},   
    { "&utilde;", "u"}, 	{ "&Utilde;", "U"}, 
    { "&ubreve;", "u"}, 	{ "&Ubreve;", "U"}, 
    { "&udblac;", "u"}, 	{ "&Udblac;", "U"}, 
    { "&uring;", "u"},  	{ "&Uring;", "U"},  
    { "&umacr;", "u"},  	{ "&Umacr;", "U"},  
    { "&uogon;", "u"},  	{ "&Uogon;", "U"},  

    { "&vacute;", "v"},

    { "&wcirc;", "w"},		{ "&Wcirc;", "W"},

    { "&xmacr;", "x"},		{ "&Xmacr;", "X"},

    { "&yacute;", "y"},		{ "&Yacute;", "Y"},
    { "&ycirc;", "y"}, 		{ "&Ycirc;", "Y"}, 
    { "&yuml;", "y"},  		{ "&Yuml;", "Y"},  
                       		                   
    { "&zacute;", "z"},		{ "&Zacute;", "Z"},
    { "&zcaron;", "z"},		{ "&Zcaron;", "Z"},
    { "&zdot;", "z"},  		{ "&Zdot;", "Z"},  

    /*
     * Some special characters...
     */
    { "&emdash;", "--"},
    { "&mdash;", "--"},  	{ "&ndash;", "-"},
    { "&lang;", "<"},  		{ "&rang;", ">"},
    { "&verbar;", "|"},		{ "&lt;", "<"},
    { "&gt;", ">"},		{ "&amp;", "&"},
    { "&ge;", ">="},		{ "&le;", "<="},
    { "&minus;", "-"},		{ "&copy;", "(c)"},
    { "&times;", "x"},		{ "&half;", "1/2"},
    { "&divide;", "/"},		{ "&plus;", "+"},
    { "&hellip;", "..."},
    { "&rarr;", "->"},		{ "&larr;", "<-"},
    { "&cent;", "cents"},	{ "&pound;", "L"},
    { "&prime;", "'"},		{ "&Prime;", "''"},
    { "&ne;", "!="},		{ "&sdot;", "."},
    { "&pi;", "pi"},
    { "&frac1.2;", "-1/2"},
    { "&frac1.4;", "-1/4"},
    { "&frac3.8;", "-3/8"},

    {"&;", "&"},  /* because they're sometimes careless. */

    /*
     * Some accent marks.
     */
    { "&tilde;", "~" },

    /*
     * A spacing character, I think.
     */
    { "&emsp;", " "},
    { "&mspace;", " "},
    { "&nbsp;", ""},  /* no blank space */
    /*
     * Some shapes which we'll approximate.
     */
    { "&rlarr2;", "<=>"},
    { "&z.1horbar;", "-"},
    { "&z.2horbar;", "="},
    { "&z.3horbar;", "="},
    { "&z.Ashape;", "A"},
    { "&z.Sshape;", "S"},
    { "&z.Ushape;", "U"},
    { "&z.Vshape;", "V"},

    /*
     * Ones that we know we don't want, but which appear frequently,
     * so we'll include here to get rid of the error message.
     */
    { "&deg;", ""},
    { "&Upsi;", ""},

    /*
     * The catchall.  Anything else is NOT translated, but omitted.
     */
    { "", ""}
};
