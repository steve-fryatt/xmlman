/* Copyright 2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of XmlMan:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file encoding.c
 *
 * Text Encloding Support, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/xmlreader.h>

#include "encoding.h"

/**
 * An entry in a character encoding table.
 */

struct encoding_map {
	int		utf8;
	unsigned char	target;
	char		*entity;
};

/**
 * UTF8 to RISC OS Latin 1
 */

static struct encoding_map encoding_acorn_latin1[] = {
	{160,	'\xa0', "&nbsp;"},	/* NO-BREAK SPACE				*/
	{161,	'\xa1',	"&iexcl;"},	/* INVERTED EXCLAMATION MARK			*/
	{162,	'\xa2',	"&cent;"},	/* CENT SIGN					*/
	{163,	'\xa3',	"&pound;"},	/* POUND SIGN					*/
	{164,	'\xa4',	"&curren;"},	/* CURRENCY SIGN				*/
	{165,	'\xa5',	"&yen;"},	/* YEN SIGN					*/
	{166,	'\xa6',	"&brvbar;"},	/* BROKEN BAR					*/
	{167,	'\xa7',	"&sect;"},	/* SECTION SIGN					*/
	{168,	'\xa8',	"&uml;"},	/* DIAERESIS					*/
	{169,	'\xa9',	"&copy;"},	/* COPYRIGHT SIGN				*/
	{170,	'\xaa',	"&ordf;"},	/* FEMININE ORDINAL INDICATOR			*/
	{171,	'\xab',	"&laquo;"},	/* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK	*/
	{172,	'\xac',	"&not;"},	/* NOT SIGN					*/
	{173,	'\xad',	"&shy;"},	/* SOFT HYPHEN					*/
	{174,	'\xae',	"&reg;"},	/* REGISTERED SIGN				*/
	{175,	'\xaf',	"&macr;"},	/* MACRON					*/
	{176,	'\xb0',	"&deg;"},	/* DEGREE SIGN					*/
	{177,	'\xb1',	"&plusmn;"},	/* PLUS-MINUS SIGN				*/
	{178,	'\xb2',	"&sup2;"},	/* SUPERSCRIPT TWO				*/
	{179,	'\xb3',	"&sup3;"},	/* SUPERSCRIPT THREE				*/
	{180,	'\xb4',	"&acute;"},	/* ACUTE ACCENT					*/
	{181,	'\xb5',	"&micro;"},	/* MICRO SIGN					*/
	{182,	'\xb6',	"&para;"},	/* PILCROW SIGN					*/
	{183,	'\xb7',	"&middot;"},	/* MIDDLE DOT					*/
	{184,	'\xb8',	"&cedil;"},	/* CEDILLA					*/
	{185,	'\xb9',	"&sup1;"},	/* SUPERSCRIPT ONE				*/
	{186,	'\xba',	"&ordm;"},	/* MASCULINE ORDINAL INDICATOR			*/
	{187,	'\xbb',	"&raquo;"},	/* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK	*/
	{188,	'\xbc',	"&frac14;"},	/* VULGAR FRACTION ONE QUARTER			*/
	{189,	'\xbd',	"&frac12;"},	/* VULGAR FRACTION ONE HALF			*/
	{190,	'\xbe',	"&frac34;"},	/* VULGAR FRACTION THREE QUARTERS		*/
	{191,	'\xbf',	"&iquest;"},	/* INVERTED QUESTION MARK			*/
	{192,	'\xc0',	"&Agrave;"},	/* LATIN CAPITAL LETTER A WITH GRAVE		*/
	{193,	'\xc1',	"&Aacute;"},	/* LATIN CAPITAL LETTER A WITH ACUTE		*/
	{194,	'\xc2',	"&Acirc;"},	/* LATIN CAPITAL LETTER A WITH CIRCUMFLEX	*/
	{195,	'\xc3',	"&Atilde;"},	/* LATIN CAPITAL LETTER A WITH TILDE		*/
	{196,	'\xc4',	"&Auml;"},	/* LATIN CAPITAL LETTER A WITH DIAERESIS	*/
	{197,	'\xc5',	"&Aring;"},	/* LATIN CAPITAL LETTER A WITH RING ABOVE	*/
	{198,	'\xc6',	"&AElig;"},	/* LATIN CAPITAL LETTER AE			*/
	{199,	'\xc7',	"&Ccedil;"},	/* LATIN CAPITAL LETTER C WITH CEDILLA		*/
	{200,	'\xc8',	"&Egrave;"},	/* LATIN CAPITAL LETTER E WITH GRAVE		*/
	{201,	'\xc9',	"&Eacute;"},	/* LATIN CAPITAL LETTER E WITH ACUTE		*/
	{202,	'\xca',	"&Ecirc;"},	/* LATIN CAPITAL LETTER E WITH CIRCUMFLEX	*/
	{203,	'\xcb',	"&Euml;"},	/* LATIN CAPITAL LETTER E WITH DIAERESIS	*/
	{204,	'\xcc',	"&Igrave;"},	/* LATIN CAPITAL LETTER I WITH GRAVE		*/
	{205,	'\xcd',	"&Iacute;"},	/* LATIN CAPITAL LETTER I WITH ACUTE		*/
	{206,	'\xce',	"&Icirc;"},	/* LATIN CAPITAL LETTER I WITH CIRCUMFLEX	*/
	{207,	'\xcf',	"&Iuml;"},	/* LATIN CAPITAL LETTER I WITH DIAERESIS	*/
	{208,	'\xd0',	"&ETH;"},	/* LATIN CAPITAL LETTER ETH			*/
	{209,	'\xd1',	"&Ntilde;"},	/* LATIN CAPITAL LETTER N WITH TILDE		*/
	{210,	'\xd2',	"&Ograve;"},	/* LATIN CAPITAL LETTER O WITH GRAVE		*/
	{211,	'\xd3',	"&Oacute;"},	/* LATIN CAPITAL LETTER O WITH ACUTE		*/
	{212,	'\xd4',	"&Ocirc;"},	/* LATIN CAPITAL LETTER O WITH CIRCUMFLEX	*/
	{213,	'\xd5',	"&Otilde;"},	/* LATIN CAPITAL LETTER O WITH TILDE		*/
	{214,	'\xd6',	"&Ouml;"},	/* LATIN CAPITAL LETTER O WITH DIAERESIS	*/
	{215,	'\xd7',	"&times;"},	/* MULTIPLICATION SIGN				*/
	{216,	'\xd8',	"&Oslash;"},	/* LATIN CAPITAL LETTER O WITH STROKE		*/
	{217,	'\xd9',	"&Ugrave;"},	/* LATIN CAPITAL LETTER U WITH GRAVE		*/
	{218,	'\xda',	"&Uacute;"},	/* LATIN CAPITAL LETTER U WITH ACUTE		*/
	{219,	'\xdb',	"&Ucirc;"},	/* LATIN CAPITAL LETTER U WITH CIRCUMFLEX	*/
	{220,	'\xdc',	"&Uuml;"},	/* LATIN CAPITAL LETTER U WITH DIAERESIS	*/
	{221,	'\xdd',	"&Yacute;"},	/* LATIN CAPITAL LETTER Y WITH ACUTE		*/
	{222,	'\xde',	"&THORN;"},	/* LATIN CAPITAL LETTER THORN			*/
	{223,	'\xdf',	"&szlig;"},	/* LATIN SMALL LETTER SHARP S			*/
	{224,	'\xe0',	"&agrave;"},	/* LATIN SMALL LETTER A WITH GRAVE		*/
	{225,	'\xe1',	"&aacute;"},	/* LATIN SMALL LETTER A WITH ACUTE		*/
	{226,	'\xe2',	"&acirc;"},	/* LATIN SMALL LETTER A WITH CIRCUMFLEX		*/
	{227,	'\xe3',	"&atilde;"},	/* LATIN SMALL LETTER A WITH TILDE		*/
	{228,	'\xe4',	"&auml;"},	/* LATIN SMALL LETTER A WITH DIAERESIS		*/
	{229,	'\xe5',	"&aring;"},	/* LATIN SMALL LETTER A WITH RING ABOVE		*/
	{230,	'\xe6',	"&aelig;"},	/* LATIN SMALL LETTER AE			*/
	{231,	'\xe7',	"&ccedil;"},	/* LATIN SMALL LETTER C WITH CEDILLA		*/
	{232,	'\xe8',	"&egrave;"},	/* LATIN SMALL LETTER E WITH GRAVE		*/
	{233,	'\xe9',	"&eacute;"},	/* LATIN SMALL LETTER E WITH ACUTE		*/
	{234,	'\xea',	"&ecirc;"},	/* LATIN SMALL LETTER E WITH CIRCUMFLEX		*/
	{235,	'\xeb',	"&euml;"},	/* LATIN SMALL LETTER E WITH DIAERESIS		*/
	{236,	'\xec',	"&igrave;"},	/* LATIN SMALL LETTER I WITH GRAVE		*/
	{237,	'\xed',	"&iacute;"},	/* LATIN SMALL LETTER I WITH ACUTE		*/
	{238,	'\xee',	"&icirc;"},	/* LATIN SMALL LETTER I WITH CIRCUMFLEX		*/
	{239,	'\xef',	"&iuml;"},	/* LATIN SMALL LETTER I WITH DIAERESIS		*/
	{240,	'\xf0',	"&eth;"},	/* LATIN SMALL LETTER ETH			*/
	{241,	'\xf1',	"&ntilde;"},	/* LATIN SMALL LETTER N WITH TILDE		*/
	{242,	'\xf2',	"&ograve;"},	/* LATIN SMALL LETTER O WITH GRAVE		*/
	{243,	'\xf3',	"&oacute;"},	/* LATIN SMALL LETTER O WITH ACUTE		*/
	{244,	'\xf4',	"&ocirc;"},	/* LATIN SMALL LETTER O WITH CIRCUMFLEX		*/
	{245,	'\xf5',	"&otilde;"},	/* LATIN SMALL LETTER O WITH TILDE		*/
	{246,	'\xf6',	"&ouml;"},	/* LATIN SMALL LETTER O WITH DIAERESIS		*/
	{247,	'\xf7',	"&divide;"},	/* DIVISION SIGN				*/
	{248,	'\xf8',	"&oslash;"},	/* LATIN SMALL LETTER O WITH STROKE		*/
	{249,	'\xf9',	"&ugrave;"},	/* LATIN SMALL LETTER U WITH GRAVE		*/
	{250,	'\xfa',	"&uacute;"},	/* LATIN SMALL LETTER U WITH ACUTE		*/
	{251,	'\xfb',	"&ucirc;"},	/* LATIN SMALL LETTER U WITH CIRCUMFLEX		*/
	{252,	'\xfc',	"&uuml;"},	/* LATIN SMALL LETTER U WITH DIAERESIS		*/
	{253,	'\xfd',	"&yacute;"},	/* LATIN SMALL LETTER Y WITH ACUTE		*/
	{254,	'\xfe',	"&thorn;"},	/* LATIN SMALL LETTER THORN			*/
	{255,	'\xff',	"&yuml;"},	/* LATIN SMALL LETTER Y WITH DIAERESIS		*/
	{338,	'\x9a',	"&OElig;"},	/* LATIN CAPITAL LIGATURE OE			*/
	{339,	'\x9b',	"&oelig;"},	/* LATIN SMALL LIGATURE OE			*/
	{372,	'\x81',	"&Wcirc;"},	/* LATIN CAPITAL LETTER W WITH CIRCUMFLEX	*/
	{373,	'\x82',	"&wcirc;"},	/* LATIN SMALL LETTER W WITH CIRCUMFLEX		*/
	{374,	'\x85',	"&Ycirc;"},	/* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX	*/
	{375,	'\x86',	"&ycirc;"},	/* LATIN SMALL LETTER Y WITH CIRCUMFLEX		*/
	{8211,	'\x97',	"&ndash;"},	/* EN DASH					*/
	{8212,	'\x98',	"&mdash;"},	/* EM DASH					*/
	{8216,	'\x90',	"&lsquo;"},	/* LEFT SINGLE QUOTATION MARK			*/
	{8217,	'\x91',	"&rsquo;"},	/* RIGHT SINGLE QUOTATION MARK			*/
	{8220,	'\x94',	"&ldquo;"},	/* LEFT DOUBLE QUOTATION MARK			*/
	{8221,	'\x95',	"&rdquo;"},	/* RIGHT DOUBLE QUOTATION MARK			*/
	{8222,	'\x96',	"&bdquo;"},	/* DOUBLE LOW-9 QUOTATION MARK			*/
	{8224,	'\x9c',	"&dagger;"},	/* DAGGER					*/
	{8225,	'\x9d',	"&Dagger;"},	/* DOUBLE DAGGER				*/
	{8226,	'\x8f',	"&bull;"},	/* BULLET					*/
	{8230,	'\x8c',	"&hellip;"},	/* HORIZONTAL ELLIPSIS				*/
	{8240,	'\x8e',	"&permil;"},	/* PER MILLE SIGN				*/
	{8249,	'\x92',	"&lsaquo;"},	/* SINGLE LEFT-POINTING ANGLE QUOTATION MARK	*/
	{8250,	'\x93',	"&rsaquo;"},	/* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK	*/
	{8482,	'\x8d',	"&trade;"},	/* TRADE MARK SIGN				*/
	{8722,	'\x99',	"&minus;"},	/* MINUS SIGN					*/
	{64257,	'\x9e',	"&filig;"},	/* Latin Small Ligature Fi			*/
	{64258,	'\x9f',	"&fllig;"},	/* Latin Small Ligature Fl			*/
	{0,	'\0',	NULL}		/* End of Table					*/
};

/**
 * UTF8 to RISC OS Latin 2
 */

static struct encoding_map encoding_acorn_latin2[] = {
	{160,	'\xa0', "&nbsp;"},	/* NO-BREAK SPACE				*/
	{164,	'\xa4',	"&curren;"},	/* CURRENCY SIGN				*/
	{167,	'\xa7',	"&sect;"},	/* SECTION SIGN					*/
	{168,	'\xa8',	"&uml;"},	/* DIAERESIS					*/
	{173,	'\xad',	"&shy;"},	/* SOFT HYPHEN					*/
	{176,	'\xb0',	"&deg;"},	/* DEGREE SIGN					*/
	{180,	'\xb4',	"&acute;"},	/* ACUTE ACCENT					*/
	{184,	'\xb8',	"&cedil;"},	/* CEDILLA					*/
	{193,	'\xc1',	"&Aacute;"},	/* LATIN CAPITAL LETTER A WITH ACUTE		*/
	{194,	'\xc2',	"&Acirc;"},	/* LATIN CAPITAL LETTER A WITH CIRCUMFLEX	*/
	{196,	'\xc4',	"&Auml;"},	/* LATIN CAPITAL LETTER A WITH DIAERESIS	*/
	{199,	'\xc7',	"&Ccedil;"},	/* LATIN CAPITAL LETTER C WITH CEDILLA		*/
	{201,	'\xc9',	"&Eacute;"},	/* LATIN CAPITAL LETTER E WITH ACUTE		*/
	{203,	'\xcb',	"&Euml;"},	/* LATIN CAPITAL LETTER E WITH DIAERESIS	*/
	{205,	'\xcd',	"&Iacute;"},	/* LATIN CAPITAL LETTER I WITH ACUTE		*/
	{206,	'\xce',	"&Icirc;"},	/* LATIN CAPITAL LETTER I WITH CIRCUMFLEX	*/
	{211,	'\xd3',	"&Oacute;"},	/* LATIN CAPITAL LETTER O WITH ACUTE		*/
	{212,	'\xd4',	"&Ocirc;"},	/* LATIN CAPITAL LETTER O WITH CIRCUMFLEX	*/
	{214,	'\xd6',	"&Ouml;"},	/* LATIN CAPITAL LETTER O WITH DIAERESIS	*/
	{215,	'\xd7',	"&times;"},	/* MULTIPLICATION SIGN				*/
	{218,	'\xda',	"&Uacute;"},	/* LATIN CAPITAL LETTER U WITH ACUTE		*/
	{220,	'\xdc',	"&Uuml;"},	/* LATIN CAPITAL LETTER U WITH DIAERESIS	*/
	{221,	'\xdd',	"&Yacute;"},	/* LATIN CAPITAL LETTER Y WITH ACUTE		*/
	{223,	'\xdf',	"&szlig;"},	/* LATIN SMALL LETTER SHARP S			*/
	{225,	'\xe1',	"&aacute;"},	/* LATIN SMALL LETTER A WITH ACUTE		*/
	{226,	'\xe2',	"&acirc;"},	/* LATIN SMALL LETTER A WITH CIRCUMFLEX		*/
	{228,	'\xe4',	"&auml;"},	/* LATIN SMALL LETTER A WITH DIAERESIS		*/
	{231,	'\xe7',	"&ccedil;"},	/* LATIN SMALL LETTER C WITH CEDILLA		*/
	{233,	'\xe9',	"&eacute;"},	/* LATIN SMALL LETTER E WITH ACUTE		*/
	{235,	'\xeb',	"&euml;"},	/* LATIN SMALL LETTER E WITH DIAERESIS		*/
	{237,	'\xed',	"&iacute;"},	/* LATIN SMALL LETTER I WITH ACUTE		*/
	{238,	'\xee',	"&icirc;"},	/* LATIN SMALL LETTER I WITH CIRCUMFLEX		*/
	{243,	'\xf3',	"&oacute;"},	/* LATIN SMALL LETTER O WITH ACUTE		*/
	{244,	'\xf4',	"&ocirc;"},	/* LATIN SMALL LETTER O WITH CIRCUMFLEX		*/
	{246,	'\xf6',	"&ouml;"},	/* LATIN SMALL LETTER O WITH DIAERESIS		*/
	{247,	'\xf7',	"&divide;"},	/* DIVISION SIGN				*/
	{250,	'\xfa',	"&uacute;"},	/* LATIN SMALL LETTER U WITH ACUTE		*/
	{252,	'\xfc',	"&uuml;"},	/* LATIN SMALL LETTER U WITH DIAERESIS		*/
	{253,	'\xfd',	"&yacute;"},	/* LATIN SMALL LETTER Y WITH ACUTE		*/
	{258,	'\xc3',	"&Abreve;"},	/* LATIN CAPITAL LETTER A WITH BREVE		*/
	{259,	'\xe3',	"&abreve;"},	/* LATIN SMALL LETTER A WITH BREVE		*/
	{260,	'\xa1',	"&Aogon;"},	/* LATIN CAPITAL LETTER A WITH OGONEK		*/
	{261,	'\xb1',	"&aogon;"},	/* LATIN SMALL LETTER A WITH OGONEK		*/
	{262,	'\xc6',	"&Cacute;"},	/* LATIN CAPITAL LETTER C WITH ACUTE		*/
	{263,	'\xe6',	"&cacute;"},	/* LATIN SMALL LETTER C WITH ACUTE		*/
	{268,	'\xc8',	"&Ccaron;"},	/* LATIN CAPITAL LETTER C WITH CARON		*/
	{269,	'\xe8',	"&ccaron;"},	/* LATIN SMALL LETTER C WITH CARON		*/
	{270,	'\xcf',	"&Dcaron;"},	/* LATIN CAPITAL LETTER D WITH CARON		*/
	{271,	'\xef',	"&dcaron;"},	/* LATIN SMALL LETTER D WITH CARON		*/
	{272,	'\xd0',	"&Dstrok;"},	/* LATIN CAPITAL LETTER D WITH STROKE		*/
	{273,	'\xf0',	"&dstrok;"},	/* LATIN SMALL LETTER D WITH STROKE		*/
	{280,	'\xca',	"&Eogon;"},	/* LATIN CAPITAL LETTER E WITH OGONEK		*/
	{281,	'\xea',	"&eogon;"},	/* LATIN SMALL LETTER E WITH OGONEK		*/
	{282,	'\xcc',	"&Ecaron;"},	/* LATIN CAPITAL LETTER E WITH CARON		*/
	{283,	'\xec',	"&ecaron;"},	/* LATIN SMALL LETTER E WITH CARON		*/
	{313,	'\xc5',	"&Lacute;"},	/* LATIN CAPITAL LETTER L WITH ACUTE		*/
	{314,	'\xe5',	"&lacute;"},	/* LATIN SMALL LETTER L WITH ACUTE		*/
	{317,	'\xa5',	"&Lcaron;"},	/* LATIN CAPITAL LETTER L WITH CARON		*/
	{318,	'\xb5',	"&lcaron;"},	/* LATIN SMALL LETTER L WITH CARON		*/
	{321,	'\xa3',	"&Lstrok;"},	/* LATIN CAPITAL LETTER L WITH STROKE		*/
	{322,	'\xb3',	"&lstrok;"},	/* LATIN SMALL LETTER L WITH STROKE		*/
	{323,	'\xd1',	"&Nacute;"},	/* LATIN CAPITAL LETTER N WITH ACUTE		*/
	{324,	'\xf1',	"&nacute;"},	/* LATIN SMALL LETTER N WITH ACUTE		*/
	{327,	'\xd2',	"&Ncaron;"},	/* LATIN CAPITAL LETTER N WITH CARON		*/
	{328,	'\xf2',	"&ncaron;"},	/* LATIN SMALL LETTER N WITH CARON		*/
	{336,	'\xd5',	"&Odblac;"},	/* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE	*/
	{337,	'\xf5',	"&odblac;"},	/* LATIN SMALL LETTER O WITH DOUBLE ACUTE	*/
	{338,	'\x9a',	"&OElig;"},	/* LATIN CAPITAL LIGATURE OE			*/
	{339,	'\x9b',	"&oelig;"},	/* LATIN SMALL LIGATURE OE			*/
	{340,	'\xc0',	"&Racute;"},	/* LATIN CAPITAL LETTER R WITH ACUTE		*/
	{341,	'\xe0',	"&racute;"},	/* LATIN SMALL LETTER R WITH ACUTE		*/
	{344,	'\xd8',	"&Rcaron;"},	/* LATIN CAPITAL LETTER R WITH CARON		*/
	{345,	'\xf8',	"&rcaron;"},	/* LATIN SMALL LETTER R WITH CARON		*/
	{346,	'\xa6',	"&Sacute;"},	/* LATIN CAPITAL LETTER S WITH ACUTE		*/
	{347,	'\xb6',	"&sacute;"},	/* LATIN SMALL LETTER S WITH ACUTE		*/
	{350,	'\xaa',	"&Scedil;"},	/* LATIN CAPITAL LETTER S WITH CEDILLA		*/
	{351,	'\xba',	"&scedil;"},	/* LATIN SMALL LETTER S WITH CEDILLA		*/
	{352,	'\xa9',	"&Scaron;"},	/* LATIN CAPITAL LETTER S WITH CARON		*/
	{353,	'\xb9',	"&scaron;"},	/* LATIN SMALL LETTER S WITH CARON		*/
	{354,	'\xde',	"&Tcedil;"},	/* LATIN CAPITAL LETTER T WITH CEDILLA		*/
	{355,	'\xfe',	"&tcedil;"},	/* LATIN SMALL LETTER T WITH CEDILLA		*/
	{356,	'\xab',	"&Tcaron;"},	/* LATIN CAPITAL LETTER T WITH CARON		*/
	{357,	'\xbb',	"&tcaron;"},	/* LATIN SMALL LETTER T WITH CARON		*/
	{366,	'\xd9',	"&Uring;"},	/* LATIN CAPITAL LETTER U WITH RING ABOVE	*/
	{367,	'\xf9',	"&uring;"},	/* LATIN SMALL LETTER U WITH RING ABOVE		*/
	{368,	'\xdb',	"&Udblac;"},	/* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE	*/
	{369,	'\xfb',	"&udblac;"},	/* LATIN SMALL LETTER U WITH DOUBLE ACUTE	*/
	{377,	'\xac',	"&Zacute;"},	/* LATIN CAPITAL LETTER Z WITH ACUTE		*/
	{378,	'\xbc',	"&zacute;"},	/* LATIN SMALL LETTER Z WITH ACUTE		*/
	{379,	'\xaf',	"&Zdot;"},	/* LATIN CAPITAL LETTER Z WITH DOT ABOVE	*/
	{380,	'\xbf',	"&zdot;"},	/* LATIN SMALL LETTER Z WITH DOT ABOVE		*/
	{381,	'\xae',	"&Zcaron;"},	/* LATIN CAPITAL LETTER Z WITH CARON		*/
	{382,	'\xbe',	"&zcaron;"},	/* LATIN SMALL LETTER Z WITH CARON		*/
	{774,	'\xa2',	NULL},		/* BREVE					*/
	{775,	'\xff',	NULL},	 	/* DOT ABOVE					*/
	{779,	'\xbd', NULL},		/* DOUBLE ACUTE ACCENT				*/
	{780,	'\xb7',	NULL},		/* CARON					*/
	{808,	'\xb2',	NULL},		/* OGONEK					*/
	{8211,	'\x97',	"&ndash;"},	/* EN DASH					*/
	{8212,	'\x98',	"&mdash;"},	/* EM DASH					*/
	{8216,	'\x90',	"&lsquo;"},	/* LEFT SINGLE QUOTATION MARK			*/
	{8217,	'\x91',	"&rsquo;"},	/* RIGHT SINGLE QUOTATION MARK			*/
	{8220,	'\x94',	"&ldquo;"},	/* LEFT DOUBLE QUOTATION MARK			*/
	{8221,	'\x95',	"&rdquo;"},	/* RIGHT DOUBLE QUOTATION MARK			*/
	{8222,	'\x96',	"&bdquo;"},	/* DOUBLE LOW-9 QUOTATION MARK			*/
	{8224,	'\x9c',	"&dagger;"},	/* DAGGER					*/
	{8225,	'\x9d',	"&Dagger;"},	/* DOUBLE DAGGER				*/
	{8226,	'\x8f',	"&bull;"},	/* BULLET					*/
	{8230,	'\x8c',	"&hellip;"},	/* HORIZONTAL ELLIPSIS				*/
	{8240,	'\x8e',	"&permil;"},	/* PER MILLE SIGN				*/
	{8249,	'\x92',	"&lsaquo;"},	/* SINGLE LEFT-POINTING ANGLE QUOTATION MARK	*/
	{8250,	'\x93',	"&rsaquo;"},	/* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK	*/
	{8482,	'\x8d',	"&trade;"},	/* TRADE MARK SIGN				*/
	{8722,	'\x99',	"&minus;"},	/* MINUS SIGN					*/
	{64257,	'\x9e',	"&filig;"},	/* Latin Small Ligature Fi			*/
	{64258,	'\x9f',	"&fllig;"},	/* Latin Small Ligature Fl			*/
	{0,	'\0',	NULL}		/* End of Table					*/
};

/**
 * An entry in the table of encoding tables.
 */

struct encoding_table {
	enum encoding_target	encoding;
	struct encoding_map	*table;
};

/**
 * The available encoding tables.
 */

static struct encoding_table encoding_list[] = {
	{ENCODING_TARGET_UTF8,		NULL},
	{ENCODING_TARGET_ACORN_LATIN1,	encoding_acorn_latin1},
	{ENCODING_TARGET_ACORN_LATIN2,	encoding_acorn_latin2}
};

/**
 * The active encoding map, or NULL to pass out UTF8.
 */

static struct encoding_map *encoding_current_map = NULL;

/* Static Function Prototypes. */

static int encoding_find_mapped_character(int utf8);


/**
 * Select an encoding table.
 *
 * \param target		The encoding target to use.
 */

bool encoding_select_table(enum encoding_target target)
{
	int	i = 0, current_code = 0;
	bool	map[256];

	/* Reset the current map selection. */

	encoding_current_map = NULL;

	/* Check that the requested map actually exists. */

	if (target < 0 || target >= ENCODING_TARGET_MAX)
		return false;

	/* Does the encoding's table entry make sense? */

	if (encoding_list[target].encoding != target)
		return false;

	/* Set the current map table. */

	encoding_current_map = encoding_list[target].table;

	/* If the table isn't allocated, there's nothing else to check. */

	if (encoding_current_map == NULL)
		return true;

	/* Reset the map target flags. */

	for (i = 0; i < 256; i++)
		map[i] = false;

	/* Scan the table, looking for out of sequence UTF8 codes and duplicate
	 * map targets.
	 */

	for (i = 0; encoding_current_map[i].utf8 != 0; i++) {
		if (encoding_current_map[i].utf8 <= current_code)
			fprintf(stderr, "Encoding %d is out of sequence at line %d of table.\n", encoding_current_map[i].utf8, i);

		if (encoding_current_map[i].target < 0 || encoding_current_map[i].target >= 256)
			fprintf(stderr, "Encoding %d has target out of range at line %d of table.\n", encoding_current_map[i].utf8, i);

		if (map[encoding_current_map[i].target] == true)
			fprintf(stderr, "Encoding %d has duplicate target %d at line %d of table.\n", encoding_current_map[i].utf8, encoding_current_map[i].target, i);

		map[encoding_current_map[i].target] = true;

		current_code = encoding_current_map[i].utf8;
	}

	for (i = 128; i < 256; i++) {
		if (map[i] == false)
			printf("Character %d (0x%x) is not mapped to UTF8\n", i, i);
	}

	return true;
}


/**
 * Parse a UTF8 string, returning the individual characters in the current
 * target encoding. State is held across calls, so that a string is returned
 * character by character.
 *
 * \param *text			The UTF8 string to parse, or NULL to continue
 *				with the current string.
 * \return			The next character in the text.
 */

int encoding_parse_utf8_string(xmlChar *text)
{
	static xmlChar	*current_text = NULL;
	int		current_char = 0, bytes_remaining = 0;

	if (text != NULL) {
		printf("Setting text to %s\n", text);
		current_text = text;
	}

	if (current_text == NULL || *current_text == '\0') {
		printf("No string, or end of string.\n");
		return 0;
	}

	if ((bytes_remaining == 0) && ((*current_text & 0x80) == 0)) {
		printf("Standard ASCII: %d\n", *current_text);
		return *current_text++;
	} else if ((bytes_remaining == 0) && ((*current_text & 0xe0) == 0xc0)) {
		printf("Opening of two-byte Unicode\n");
		current_char = (*current_text++ & 0x1f) << 6;
		bytes_remaining = 1;
	} else if ((bytes_remaining == 0) && ((*current_text & 0xf0) == 0xe0)) {
		printf("Opening of three-byte Unicode\n");
		current_char = (*current_text++ & 0x0f) << 12;
		bytes_remaining = 2;
	} else if ((bytes_remaining == 0) && ((*current_text & 0xf8) == 0xf0)) {
		printf("Opening of four-byte Unicode\n");
		current_char = (*current_text++ & 0x07) << 18;
		bytes_remaining = 3;
	} else {
		fprintf(stderr, "Unexpected UTF8 sequence.\n");
		return 0;
	}

	while (bytes_remaining > 0) {
		if ((*current_text == 0) || ((*current_text & 0xc0) != 0x80)) {
			fprintf(stderr, "Unexpected UTF8 sequence.\n");
			return 0;
		}

		printf("Additional UTF8 byte code.\n");
		current_char += (*current_text++ & 0x3f) << (6 * --bytes_remaining);
	}

	printf("UTF8: %d\n", current_char);

	if (current_char >= 0 && current_char < 128)
		return current_char;

	return encoding_find_mapped_character(current_char);
}

/**
 * Convert a UTF8 character into the appropriate code in the current encoding.
 * Characters which can't be mapped are returned as '?'.
 *
 * \param utf8			The UTF8 character to convert.
 * \return			The encoded character, or '?'.
 */

static int encoding_find_mapped_character(int utf8)
{
	int i;

	if (encoding_current_map == NULL) {
		printf("No mapping is in force.\n");
		return utf8;
	}

	for (i = 0; encoding_current_map[i].utf8 != 0; i++) {
		if (encoding_current_map[i].utf8 == utf8) {
			printf("Found mapping to %d.\n", encoding_current_map[i].target);
			return encoding_current_map[i].target;
		}
	}

	printf("No mapping found in current encoding.\n");

	return '?';
}

/**
 * Flatten down the white space in a text string, so that multiple spaces
 * and newlines become a single ASCII space. The supplied buffer is
 * assumed to be zero terminated, and its contents will be updated.
 *
 * \param *text			The text to be flattened.
 */

void encoding_flatten_whitespace(xmlChar *text)
{
	unsigned char	*head, *tail;
	bool		space = false, whitespace = false;

	head = (unsigned char*) text;
	tail = head;

	while (*head != '\0') {
		space = (*head == '\t' || *head == '\r' || *head == '\n' || *head == ' ');

		if (space && whitespace) {
			head++;
		} else if (space == true) {
			*tail++ = ' ';
			head++;
			whitespace = space;
		} else {
			*tail++ = *head++;
			whitespace = space;
		}
	}

	*tail = '\0';
}

