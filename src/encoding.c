/* Copyright 2018-2024, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of XmlMan:
 *
 *   http://www.stevefryatt.org.uk/risc-os
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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

#include "encoding.h"
#include "string.h"

#include "msg.h"

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
	{160,	'\xa0', "&nbsp;"},	/* No-Break Space				*/
	{161,	'\xa1',	"&iexcl;"},	/* Inverted Exclamation Mark			*/
	{162,	'\xa2',	"&cent;"},	/* Cent Sign					*/
	{163,	'\xa3',	"&pound;"},	/* Pound Sign					*/
	{164,	'\xa4',	"&curren;"},	/* Currency Sign				*/
	{165,	'\xa5',	"&yen;"},	/* Yen Sign					*/
	{166,	'\xa6',	"&brvbar;"},	/* Broken Bar					*/
	{167,	'\xa7',	"&sect;"},	/* Section Sign					*/
	{168,	'\xa8',	"&uml;"},	/* Diaeresis					*/
	{169,	'\xa9',	"&copy;"},	/* Copyright Sign				*/
	{170,	'\xaa',	"&ordf;"},	/* Feminine Ordinal Indicator			*/
	{171,	'\xab',	"&laquo;"},	/* Left-Pointing Double Angle Quotation Mark	*/
	{172,	'\xac',	"&not;"},	/* Not Sign					*/
	{173,	'\xad',	"&shy;"},	/* Soft Hyphen					*/
	{174,	'\xae',	"&reg;"},	/* Registered Sign				*/
	{175,	'\xaf',	"&macr;"},	/* Macron					*/
	{176,	'\xb0',	"&deg;"},	/* Degree Sign					*/
	{177,	'\xb1',	"&plusmn;"},	/* Plus-Minus Sign				*/
	{178,	'\xb2',	"&sup2;"},	/* Superscript Two				*/
	{179,	'\xb3',	"&sup3;"},	/* Superscript Three				*/
	{180,	'\xb4',	"&acute;"},	/* Acute Accent					*/
	{181,	'\xb5',	"&micro;"},	/* Micro Sign					*/
	{182,	'\xb6',	"&para;"},	/* Pilcrow Sign					*/
	{183,	'\xb7',	"&middot;"},	/* Middle Dot					*/
	{184,	'\xb8',	"&cedil;"},	/* Cedilla					*/
	{185,	'\xb9',	"&sup1;"},	/* Superscript One				*/
	{186,	'\xba',	"&ordm;"},	/* Masculine Ordinal Indicator			*/
	{187,	'\xbb',	"&raquo;"},	/* Right-Pointing Double Angle Quotation Mark	*/
	{188,	'\xbc',	"&frac14;"},	/* Vulgar Fraction One Quarter			*/
	{189,	'\xbd',	"&frac12;"},	/* Vulgar Fraction One Half			*/
	{190,	'\xbe',	"&frac34;"},	/* Vulgar Fraction Three Quarters		*/
	{191,	'\xbf',	"&iquest;"},	/* Inverted Question Mark			*/
	{192,	'\xc0',	"&Agrave;"},	/* Latin Capital Letter A With Grave		*/
	{193,	'\xc1',	"&Aacute;"},	/* Latin Capital Letter A With Acute		*/
	{194,	'\xc2',	"&Acirc;"},	/* Latin Capital Letter A With Circumflex	*/
	{195,	'\xc3',	"&Atilde;"},	/* Latin Capital Letter A With Tilde		*/
	{196,	'\xc4',	"&Auml;"},	/* Latin Capital Letter A With Diaeresis	*/
	{197,	'\xc5',	"&Aring;"},	/* Latin Capital Letter A With Ring Above	*/
	{198,	'\xc6',	"&AElig;"},	/* Latin Capital Letter AE			*/
	{199,	'\xc7',	"&Ccedil;"},	/* Latin Capital Letter C With Cedilla		*/
	{200,	'\xc8',	"&Egrave;"},	/* Latin Capital Letter E With Grave		*/
	{201,	'\xc9',	"&Eacute;"},	/* Latin Capital Letter E With Acute		*/
	{202,	'\xca',	"&Ecirc;"},	/* Latin Capital Letter E With Circumflex	*/
	{203,	'\xcb',	"&Euml;"},	/* Latin Capital Letter E With Diaeresis	*/
	{204,	'\xcc',	"&Igrave;"},	/* Latin Capital Letter I With Grave		*/
	{205,	'\xcd',	"&Iacute;"},	/* Latin Capital Letter I With Acute		*/
	{206,	'\xce',	"&Icirc;"},	/* Latin Capital Letter I With Circumflex	*/
	{207,	'\xcf',	"&Iuml;"},	/* Latin Capital Letter I With Diaeresis	*/
	{208,	'\xd0',	"&ETH;"},	/* Latin Capital Letter Eth			*/
	{209,	'\xd1',	"&Ntilde;"},	/* Latin Capital Letter N With Tilde		*/
	{210,	'\xd2',	"&Ograve;"},	/* Latin Capital Letter O With Grave		*/
	{211,	'\xd3',	"&Oacute;"},	/* Latin Capital Letter O With Acute		*/
	{212,	'\xd4',	"&Ocirc;"},	/* Latin Capital Letter O With Circumflex	*/
	{213,	'\xd5',	"&Otilde;"},	/* Latin Capital Letter O With Tilde		*/
	{214,	'\xd6',	"&Ouml;"},	/* Latin Capital Letter O With Diaeresis	*/
	{215,	'\xd7',	"&times;"},	/* Multiplication Sign				*/
	{216,	'\xd8',	"&Oslash;"},	/* Latin Capital Letter O With Stroke		*/
	{217,	'\xd9',	"&Ugrave;"},	/* Latin Capital Letter U With Grave		*/
	{218,	'\xda',	"&Uacute;"},	/* Latin Capital Letter U With Acute		*/
	{219,	'\xdb',	"&Ucirc;"},	/* Latin Capital Letter U With Circumflex	*/
	{220,	'\xdc',	"&Uuml;"},	/* Latin Capital Letter U With Diaeresis	*/
	{221,	'\xdd',	"&Yacute;"},	/* Latin Capital Letter Y With Acute		*/
	{222,	'\xde',	"&THORN;"},	/* Latin Capital Letter Thorn			*/
	{223,	'\xdf',	"&szlig;"},	/* Latin Small Letter Sharp S			*/
	{224,	'\xe0',	"&agrave;"},	/* Latin Small Letter A With Grave		*/
	{225,	'\xe1',	"&aacute;"},	/* Latin Small Letter A With Acute		*/
	{226,	'\xe2',	"&acirc;"},	/* Latin Small Letter A With Circumflex		*/
	{227,	'\xe3',	"&atilde;"},	/* Latin Small Letter A With Tilde		*/
	{228,	'\xe4',	"&auml;"},	/* Latin Small Letter A With Diaeresis		*/
	{229,	'\xe5',	"&aring;"},	/* Latin Small Letter A With Ring Above		*/
	{230,	'\xe6',	"&aelig;"},	/* Latin Small Letter AE			*/
	{231,	'\xe7',	"&ccedil;"},	/* Latin Small Letter C With Cedilla		*/
	{232,	'\xe8',	"&egrave;"},	/* Latin Small Letter E With Grave		*/
	{233,	'\xe9',	"&eacute;"},	/* Latin Small Letter E With Acute		*/
	{234,	'\xea',	"&ecirc;"},	/* Latin Small Letter E With Circumflex		*/
	{235,	'\xeb',	"&euml;"},	/* Latin Small Letter E With Diaeresis		*/
	{236,	'\xec',	"&igrave;"},	/* Latin Small Letter I With Grave		*/
	{237,	'\xed',	"&iacute;"},	/* Latin Small Letter I With Acute		*/
	{238,	'\xee',	"&icirc;"},	/* Latin Small Letter I With Circumflex		*/
	{239,	'\xef',	"&iuml;"},	/* Latin Small Letter I With Diaeresis		*/
	{240,	'\xf0',	"&eth;"},	/* Latin Small Letter Eth			*/
	{241,	'\xf1',	"&ntilde;"},	/* Latin Small Letter N With Tilde		*/
	{242,	'\xf2',	"&ograve;"},	/* Latin Small Letter O With Grave		*/
	{243,	'\xf3',	"&oacute;"},	/* Latin Small Letter O With Acute		*/
	{244,	'\xf4',	"&ocirc;"},	/* Latin Small Letter O With Circumflex		*/
	{245,	'\xf5',	"&otilde;"},	/* Latin Small Letter O With Tilde		*/
	{246,	'\xf6',	"&ouml;"},	/* Latin Small Letter O With Diaeresis		*/
	{247,	'\xf7',	"&divide;"},	/* Division Sign				*/
	{248,	'\xf8',	"&oslash;"},	/* Latin Small Letter O With Stroke		*/
	{249,	'\xf9',	"&ugrave;"},	/* Latin Small Letter U With Grave		*/
	{250,	'\xfa',	"&uacute;"},	/* Latin Small Letter U With Acute		*/
	{251,	'\xfb',	"&ucirc;"},	/* Latin Small Letter U With Circumflex		*/
	{252,	'\xfc',	"&uuml;"},	/* Latin Small Letter U With Diaeresis		*/
	{253,	'\xfd',	"&yacute;"},	/* Latin Small Letter Y With Acute		*/
	{254,	'\xfe',	"&thorn;"},	/* Latin Small Letter Thorn			*/
	{255,	'\xff',	"&yuml;"},	/* Latin Small Letter Y With Diaeresis		*/
	{338,	'\x9a',	"&OElig;"},	/* Latin Capital Ligature OE			*/
	{339,	'\x9b',	"&oelig;"},	/* Latin Small Ligature OE			*/
	{372,	'\x81',	"&Wcirc;"},	/* Latin Capital Letter W With Circumflex	*/
	{373,	'\x82',	"&wcirc;"},	/* Latin Small Letter W With Circumflex		*/
	{374,	'\x85',	"&Ycirc;"},	/* Latin Capital Letter Y With Circumflex	*/
	{375,	'\x86',	"&ycirc;"},	/* Latin Small Letter Y With Circumflex		*/
	{8211,	'\x97',	"&ndash;"},	/* En Dash					*/
	{8212,	'\x98',	"&mdash;"},	/* Em Dash					*/
	{8216,	'\x90',	"&lsquo;"},	/* Left Single Quotation Mark			*/
	{8217,	'\x91',	"&rsquo;"},	/* Right Single Quotation Mark			*/
	{8220,	'\x94',	"&ldquo;"},	/* Left Double Quotation Mark			*/
	{8221,	'\x95',	"&rdquo;"},	/* Right Double Quotation Mark			*/
	{8222,	'\x96',	"&bdquo;"},	/* Double Low-9 Quotation Mark			*/
	{8224,	'\x9c',	"&dagger;"},	/* Dagger					*/
	{8225,	'\x9d',	"&Dagger;"},	/* Double Dagger				*/
	{8226,	'\x8f',	"&bull;"},	/* Bullet					*/
	{8230,	'\x8c',	"&hellip;"},	/* Horizontal Ellipsis				*/
	{8240,	'\x8e',	"&permil;"},	/* Per Mille Sign				*/
	{8249,	'\x92',	"&lsaquo;"},	/* Single Left-Pointing Angle Quotation Mark	*/
	{8250,	'\x93',	"&rsaquo;"},	/* Single Right-Pointing Angle Quotation Mark	*/
	{8482,	'\x8d',	"&trade;"},	/* Trade Mark Sign				*/
	{8722,	'\x99',	"&minus;"},	/* Minus Sign					*/
	{64257,	'\x9e',	"&filig;"},	/* Latin Small Ligature Fi			*/
	{64258,	'\x9f',	"&fllig;"},	/* Latin Small Ligature Fl			*/
	{0,	'\0',	NULL}		/* End of Table					*/
};

/**
 * UTF8 to RISC OS Latin 2
 */

static struct encoding_map encoding_acorn_latin2[] = {
	{160,	'\xa0', "&nbsp;"},	/* No-Break Space				*/
	{164,	'\xa4',	"&curren;"},	/* Currency Sign				*/
	{167,	'\xa7',	"&sect;"},	/* Section Sign					*/
	{168,	'\xa8',	"&uml;"},	/* Diaeresis					*/
	{173,	'\xad',	"&shy;"},	/* Soft Hyphen					*/
	{176,	'\xb0',	"&deg;"},	/* Degree Sign					*/
	{180,	'\xb4',	"&acute;"},	/* Acute Accent					*/
	{184,	'\xb8',	"&cedil;"},	/* Cedilla					*/
	{193,	'\xc1',	"&Aacute;"},	/* Latin Capital Letter A With Acute		*/
	{194,	'\xc2',	"&Acirc;"},	/* Latin Capital Letter A With Circumflex	*/
	{196,	'\xc4',	"&Auml;"},	/* Latin Capital Letter A With Diaeresis	*/
	{199,	'\xc7',	"&Ccedil;"},	/* Latin Capital Letter C With Cedilla		*/
	{201,	'\xc9',	"&Eacute;"},	/* Latin Capital Letter E With Acute		*/
	{203,	'\xcb',	"&Euml;"},	/* Latin Capital Letter E With Diaeresis	*/
	{205,	'\xcd',	"&Iacute;"},	/* Latin Capital Letter I With Acute		*/
	{206,	'\xce',	"&Icirc;"},	/* Latin Capital Letter I With Circumflex	*/
	{211,	'\xd3',	"&Oacute;"},	/* Latin Capital Letter O With Acute		*/
	{212,	'\xd4',	"&Ocirc;"},	/* Latin Capital Letter O With Circumflex	*/
	{214,	'\xd6',	"&Ouml;"},	/* Latin Capital Letter O With Diaeresis	*/
	{215,	'\xd7',	"&times;"},	/* Multiplication Sign				*/
	{218,	'\xda',	"&Uacute;"},	/* Latin Capital Letter U With Acute		*/
	{220,	'\xdc',	"&Uuml;"},	/* Latin Capital Letter U With Diaeresis	*/
	{221,	'\xdd',	"&Yacute;"},	/* Latin Capital Letter Y With Acute		*/
	{223,	'\xdf',	"&szlig;"},	/* Latin Small Letter Sharp S			*/
	{225,	'\xe1',	"&aacute;"},	/* Latin Small Letter A With Acute		*/
	{226,	'\xe2',	"&acirc;"},	/* Latin Small Letter A With Circumflex		*/
	{228,	'\xe4',	"&auml;"},	/* Latin Small Letter A With Diaeresis		*/
	{231,	'\xe7',	"&ccedil;"},	/* Latin Small Letter C With Cedilla		*/
	{233,	'\xe9',	"&eacute;"},	/* Latin Small Letter E With Acute		*/
	{235,	'\xeb',	"&euml;"},	/* Latin Small Letter E With Diaeresis		*/
	{237,	'\xed',	"&iacute;"},	/* Latin Small Letter I With Acute		*/
	{238,	'\xee',	"&icirc;"},	/* Latin Small Letter I With Circumflex		*/
	{243,	'\xf3',	"&oacute;"},	/* Latin Small Letter O With Acute		*/
	{244,	'\xf4',	"&ocirc;"},	/* Latin Small Letter O With Circumflex		*/
	{246,	'\xf6',	"&ouml;"},	/* Latin Small Letter O With Diaeresis		*/
	{247,	'\xf7',	"&divide;"},	/* Division Sign				*/
	{250,	'\xfa',	"&uacute;"},	/* Latin Small Letter U With Acute		*/
	{252,	'\xfc',	"&uuml;"},	/* Latin Small Letter U With Diaeresis		*/
	{253,	'\xfd',	"&yacute;"},	/* Latin Small Letter Y With Acute		*/
	{258,	'\xc3',	"&Abreve;"},	/* Latin Capital Letter A With Breve		*/
	{259,	'\xe3',	"&abreve;"},	/* Latin Small Letter A With Breve		*/
	{260,	'\xa1',	"&Aogon;"},	/* Latin Capital Letter A With Ogonek		*/
	{261,	'\xb1',	"&aogon;"},	/* Latin Small Letter A With Ogonek		*/
	{262,	'\xc6',	"&Cacute;"},	/* Latin Capital Letter C With Acute		*/
	{263,	'\xe6',	"&cacute;"},	/* Latin Small Letter C With Acute		*/
	{268,	'\xc8',	"&Ccaron;"},	/* Latin Capital Letter C With Caron		*/
	{269,	'\xe8',	"&ccaron;"},	/* Latin Small Letter C With Caron		*/
	{270,	'\xcf',	"&Dcaron;"},	/* Latin Capital Letter D With Caron		*/
	{271,	'\xef',	"&dcaron;"},	/* Latin Small Letter D With Caron		*/
	{272,	'\xd0',	"&Dstrok;"},	/* Latin Capital Letter D With Stroke		*/
	{273,	'\xf0',	"&dstrok;"},	/* Latin Small Letter D With Stroke		*/
	{280,	'\xca',	"&Eogon;"},	/* Latin Capital Letter E With Ogonek		*/
	{281,	'\xea',	"&eogon;"},	/* Latin Small Letter E With Ogonek		*/
	{282,	'\xcc',	"&Ecaron;"},	/* Latin Capital Letter E With Caron		*/
	{283,	'\xec',	"&ecaron;"},	/* Latin Small Letter E With Caron		*/
	{313,	'\xc5',	"&Lacute;"},	/* Latin Capital Letter L With Acute		*/
	{314,	'\xe5',	"&lacute;"},	/* Latin Small Letter L With Acute		*/
	{317,	'\xa5',	"&Lcaron;"},	/* Latin Capital Letter L With Caron		*/
	{318,	'\xb5',	"&lcaron;"},	/* Latin Small Letter L With Caron		*/
	{321,	'\xa3',	"&Lstrok;"},	/* Latin Capital Letter L With Stroke		*/
	{322,	'\xb3',	"&lstrok;"},	/* Latin Small Letter L With Stroke		*/
	{323,	'\xd1',	"&Nacute;"},	/* Latin Capital Letter N With Acute		*/
	{324,	'\xf1',	"&nacute;"},	/* Latin Small Letter N With Acute		*/
	{327,	'\xd2',	"&Ncaron;"},	/* Latin Capital Letter N With Caron		*/
	{328,	'\xf2',	"&ncaron;"},	/* Latin Small Letter N With Caron		*/
	{336,	'\xd5',	"&Odblac;"},	/* Latin Capital Letter O With Double Acute	*/
	{337,	'\xf5',	"&odblac;"},	/* Latin Small Letter O With Double Acute	*/
	{338,	'\x9a',	"&OElig;"},	/* Latin Capital Ligature OE			*/
	{339,	'\x9b',	"&oelig;"},	/* Latin Small Ligature OE			*/
	{340,	'\xc0',	"&Racute;"},	/* Latin Capital Letter R With Acute		*/
	{341,	'\xe0',	"&racute;"},	/* Latin Small Letter R With Acute		*/
	{344,	'\xd8',	"&Rcaron;"},	/* Latin Capital Letter R With Caron		*/
	{345,	'\xf8',	"&rcaron;"},	/* Latin Small Letter R With Caron		*/
	{346,	'\xa6',	"&Sacute;"},	/* Latin Capital Letter S With Acute		*/
	{347,	'\xb6',	"&sacute;"},	/* Latin Small Letter S With Acute		*/
	{350,	'\xaa',	"&Scedil;"},	/* Latin Capital Letter S With Cedilla		*/
	{351,	'\xba',	"&scedil;"},	/* Latin Small Letter S With Cedilla		*/
	{352,	'\xa9',	"&Scaron;"},	/* Latin Capital Letter S With Caron		*/
	{353,	'\xb9',	"&scaron;"},	/* Latin Small Letter S With Caron		*/
	{354,	'\xde',	"&Tcedil;"},	/* Latin Capital Letter T With Cedilla		*/
	{355,	'\xfe',	"&tcedil;"},	/* Latin Small Letter T With Cedilla		*/
	{356,	'\xab',	"&Tcaron;"},	/* Latin Capital Letter T With Caron		*/
	{357,	'\xbb',	"&tcaron;"},	/* Latin Small Letter T With Caron		*/
	{366,	'\xd9',	"&Uring;"},	/* Latin Capital Letter U With Ring Above	*/
	{367,	'\xf9',	"&uring;"},	/* Latin Small Letter U With Ring Above		*/
	{368,	'\xdb',	"&Udblac;"},	/* Latin Capital Letter U With Double Acute	*/
	{369,	'\xfb',	"&udblac;"},	/* Latin Small Letter U With Double Acute	*/
	{377,	'\xac',	"&Zacute;"},	/* Latin Capital Letter Z With Acute		*/
	{378,	'\xbc',	"&zacute;"},	/* Latin Small Letter Z With Acute		*/
	{379,	'\xaf',	"&Zdot;"},	/* Latin Capital Letter Z With Dot Above	*/
	{380,	'\xbf',	"&zdot;"},	/* Latin Small Letter Z With Dot Above		*/
	{381,	'\xae',	"&Zcaron;"},	/* Latin Capital Letter Z With Caron		*/
	{382,	'\xbe',	"&zcaron;"},	/* Latin Small Letter Z With Caron		*/
	{774,	'\xa2',	NULL},		/* Breve					*/
	{775,	'\xff',	NULL},	 	/* Dot Above					*/
	{779,	'\xbd', NULL},		/* Double Acute Accent				*/
	{780,	'\xb7',	NULL},		/* Caron					*/
	{808,	'\xb2',	NULL},		/* Ogonek					*/
	{8211,	'\x97',	"&ndash;"},	/* En Dash					*/
	{8212,	'\x98',	"&mdash;"},	/* Em Dash					*/
	{8216,	'\x90',	"&lsquo;"},	/* Left Single Quotation Mark			*/
	{8217,	'\x91',	"&rsquo;"},	/* Right Single Quotation Mark			*/
	{8220,	'\x94',	"&ldquo;"},	/* Left Double Quotation Mark			*/
	{8221,	'\x95',	"&rdquo;"},	/* Right Double Quotation Mark			*/
	{8222,	'\x96',	"&bdquo;"},	/* Double Low-9 Quotation Mark			*/
	{8224,	'\x9c',	"&dagger;"},	/* Dagger					*/
	{8225,	'\x9d',	"&Dagger;"},	/* Double Dagger				*/
	{8226,	'\x8f',	"&bull;"},	/* Bullet					*/
	{8230,	'\x8c',	"&hellip;"},	/* Horizontal Ellipsis				*/
	{8240,	'\x8e',	"&permil;"},	/* Per Mille Sign				*/
	{8249,	'\x92',	"&lsaquo;"},	/* Single Left-Pointing Angle Quotation Mark	*/
	{8250,	'\x93',	"&rsaquo;"},	/* Single Right-Pointing Angle Quotation Mark	*/
	{8482,	'\x8d',	"&trade;"},	/* Trade Mark Sign				*/
	{8722,	'\x99',	"&minus;"},	/* Minus Sign					*/
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
	const char		*name;
	const char		*label;
};

/**
 * The available encoding tables.
 */

static struct encoding_table encoding_list[] = {
	{ENCODING_TARGET_UTF8,		NULL,			"UTF8",		"utf-8"},
	{ENCODING_TARGET_7BIT,		NULL,			"7Bit",		"utf-8"},
	{ENCODING_TARGET_ACORN_LATIN1,	encoding_acorn_latin1,	"AcornL1",	"iso-8859-1"},
	{ENCODING_TARGET_ACORN_LATIN2,	encoding_acorn_latin2,	"AcornL2",	"iso-8859-2"}
};

/**
 * An entry in the line endings table.
 */

struct encoding_line_end_table {
	enum encoding_line_end	line_end;
	const char		*sequence;
	const char		*name;
};

/**
 * The available line endings.
 */

static struct encoding_line_end_table encoding_line_end_list[] = {
	{ENCODING_LINE_END_CR,		"\r",		"CR"},
	{ENCODING_LINE_END_LF,		"\n",		"LF"},
	{ENCODING_LINE_END_CRLF,	"\r\n",		"CRLF"},
	{ENCODING_LINE_END_LFCR,	"\n\r",		"LFCR"}
};

/**
 * The currently selected encoding target.
 */

static enum encoding_target encoding_current_target = ENCODING_TARGET_UTF8;

/**
 * The active encoding map, or NULL to pass out UTF8.
 */

static struct encoding_map *encoding_current_map = NULL;

/**
 * The number of entries in the current map.
 */

static size_t encoding_current_map_size = 0;

/**
 * The current line end selection.
 */

static int encoding_current_line_end = -1;

/* Static Function Prototypes. */

static bool encoding_find_mapped_character(int unicode, char *c);

/**
 * Find an encoding type based on a textual name.
 *
 * \param *name			The encoding name to match.
 * \return			The encoding type, or
 *				ENCODING_TARGET_NONE.
 */

enum encoding_target encoding_find_target(char *name)
{
	int i;

	for (i = 0; i < ENCODING_TARGET_MAX; i++) {
		if (string_nocase_strcmp(name, encoding_list[i].name) == 0)
			return encoding_list[i].encoding;
	}

	return ENCODING_TARGET_NONE;
}

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
	encoding_current_map_size = 0;

	/* Check that the requested map actually exists. */

	if (target < 0 || target >= ENCODING_TARGET_MAX)
		return false;

	/* Does the encoding's table entry make sense? */

	if (encoding_list[target].encoding != target)
		return false;

	/* Set the current encoding. */

	encoding_current_target = target;

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
			msg_report(MSG_ENC_OUT_OF_SEQ, encoding_current_map[i].utf8, i);

		if (encoding_current_map[i].target < 0 || encoding_current_map[i].target >= 256)
			msg_report(MSG_ENC_OUT_OF_RANGE, encoding_current_map[i].utf8, i);

		if (map[encoding_current_map[i].target] == true)
			msg_report(MSG_ENC_DUPLICATE, encoding_current_map[i].utf8, encoding_current_map[i].target, i);

		map[encoding_current_map[i].target] = true;

		current_code = encoding_current_map[i].utf8;
	}

	encoding_current_map_size = i;

	for (i = 128; i < 256; i++) {
		if (map[i] == false)
			msg_report(MSG_ENC_NO_MAP, i, i);
	}

	return true;
}

/**
 * Return the name of the current encding, in the standard
 * form recognised in HTML documents.
 * 
 * \return			Pointer to the name.
 */

const char *encoding_get_current_label(void)
{
	return encoding_list[encoding_current_target].label;
}

/**
 * Find a line ending type based on a textual name.
 *
 * \param *name			The line ending name to match.
 * \return			The line ending type, or
 *				ENCODING_LINE_END_NONE.
 */

enum encoding_line_end encoding_find_line_end(char *name)
{
	int i;

	for (i = 0; i < ENCODING_LINE_END_MAX; i++) {
		if (strcmp(name, encoding_line_end_list[i].name) == 0)
			return encoding_line_end_list[i].line_end;
	}

	return ENCODING_LINE_END_NONE;
}

/**
 * Select a type of line ending.
 *
 * \param type		The line ending type to use.
 */

bool encoding_select_line_end(enum encoding_line_end type)
{
	/* Reset the current line end selection */

	encoding_current_line_end = -1;

	/* Check that the requested line end actually exists. */

	if (type < 0 || type >= ENCODING_LINE_END_MAX)
		return false;

	/* Does the line end's table entry make sense? */

	if (encoding_line_end_list[type].line_end != type)
		return false;

	/* Set the current map table. */

	encoding_current_line_end = type;

	return true;
}

/**
 * Parse a UTF-8 string and return its visible length, in characters.
 * This should be a constant in all encodings.
 * 
 * \param *text			Pointer to the the UTF8 string to parse.
 * \return			The number of characters in the string.
 */

int encoding_get_utf8_string_length(char *text)
{
	int length = 0;

	while (encoding_parse_utf8_string(&text) != 0)
		length++;

	return length;
}

/**
 * Parse a UTF8 string, returning the individual characters in Unicode.
 * The supplied string pointer is updated on return, to  point to the
 * next character to be processed (but stops on the zero terminator).
 *
 * \param **text		Pointer to the the UTF8 string to parse.
 * \return			The next character in the text.
 */

int encoding_parse_utf8_string(char **text)
{
	int	current_char = 0, bytes_remaining = 0;

	/* There's no buffer, or we're at the end of the string. */

	if (text == NULL || *text == NULL || **text == '\0')
		return 0;

	/* We're not currently in a UTF8 byte sequence. */

	if ((bytes_remaining == 0) && ((**text & 0x80) == 0)) {
		return *(*text)++;
	} else if ((bytes_remaining == 0) && ((**text & 0xe0) == 0xc0)) {
		current_char = (*(*text)++ & 0x1f) << 6;
		bytes_remaining = 1;
	} else if ((bytes_remaining == 0) && ((**text & 0xf0) == 0xe0)) {
		current_char = (*(*text)++ & 0x0f) << 12;
		bytes_remaining = 2;
	} else if ((bytes_remaining == 0) && ((**text & 0xf8) == 0xf0)) {
		current_char = (*(*text)++ & 0x07) << 18;
		bytes_remaining = 3;
	} else {
		msg_report(MSG_ENC_BAD_UTF8);
		return 0;
	}

	/* Process any additional UTF8 bytes. */

	while (bytes_remaining > 0) {
		if ((**text == 0) || ((**text & 0xc0) != 0x80)) {
			msg_report(MSG_ENC_BAD_UTF8);
			return 0;
		}

		current_char += (*(*text)++ & 0x3f) << (6 * --bytes_remaining);
	}

	return current_char;
}

/**
 * Write a unicode character to a buffer in the current encoding,
 * followed by a zero terminator.
 *
 * \param *buffer		Pointer to the buffer to write to.
 * \param length		The length of the supplied buffer.
 * \param unicode		The unicode character to write.
 * \return			True if the requested character could
 *				be encoded; otherwise false.
 */

bool encoding_write_unicode_char(char *buffer, size_t length, int unicode)
{
	bool success = false;

	if (buffer == NULL)
		return false;

	/* There's an encoding selected, so convert the character. */

	if (encoding_current_map != NULL) {
		if (length < 2)
			return false;

		buffer[1] = '\0';
		return encoding_find_mapped_character(unicode, buffer);
	}

	/* It's 7-bit encoding, so reject anything that falls out of range. */

	if (encoding_current_target == ENCODING_TARGET_7BIT && unicode > 127) {
		buffer[0] = '?';
		buffer[1] = '\0';
		return false;
	}

	/* There's no encoding, so convert to UTF8. */

	return (encoding_write_utf8_character(buffer, length, unicode) > 0) ? true : false;
}

/**
 * Write a unicode character to a buffer in UTF-8.
 *
 * \param *buffer		Pointer to the buffer to write to.
 * \param length		The length of the supplied buffer.
 * \param unicode		The unicode character to write.
 * \return			The number of bytes written to the buffer.
 */

int encoding_write_utf8_character(char *buffer, size_t length, int unicode)
{
	if (unicode >= 0x00 && unicode <= 0x7f) {
		if (length < 2)
			return 0;

		buffer[0] = unicode;
		buffer[1] = '\0';
		return 1;
	} else if (unicode >= 0x80 && unicode <= 0x7ff) {
		if (length < 3)
			return 0;

		buffer[0] = 0xc0 | ((unicode >> 6) & 0x1f);
		buffer[1] = 0x80 | (unicode & 0x3f);
		buffer[2] = '\0';
		return 2;
	} else if (unicode >= 0x800 && unicode <= 0xffff) {
		if (length < 4)
			return 0;

		buffer[0] = 0xe0 | ((unicode >> 12) & 0x0f);
		buffer[1] = 0x80 | ((unicode >> 6) & 0x3f);
		buffer[2] = 0x80 | (unicode & 0x3f);
		buffer[3] = '\0';
		return 3;
	} else if (unicode >= 0x10000 && unicode <= 0x10ffff) {
		if (length < 5)
			return 0;

		buffer[0] = 0xf0 | ((unicode >> 16) & 0x07);
		buffer[1] = 0x80 | ((unicode >> 12) & 0x3f);
		buffer[2] = 0x80 | ((unicode >> 6) & 0x3f);
		buffer[3] = 0x80 | (unicode & 0x3f);
		buffer[4] = '\0';
		return 4;
	} else {
		return 0;
	}
}

/**
 * Convert a unicode character into the appropriate code in the current
 * encoding. Characters which can't be mapped are returned as '?'.
 *
 * \param unicode		The unicode character to convert.
 * \param *c			Pointer to a character in which to
 *				return the encoding (or '?').
 * \return			True if an encoding was found; else False.
 */

static bool encoding_find_mapped_character(int unicode, char *c)
{
	int first = 0, last = encoding_current_map_size, middle;

	if (c == NULL)
		return false;

	/* The byte is the same in unicode or ASCII. */

	if (unicode >= 0 && unicode < 128) {
		*c = unicode;
		return true;
	}

	/* There's no encoding selected, so output straight unicode. */

	if (encoding_current_map == NULL) {
		*c = unicode;
		return true;
	}

	/* Find the character in the current encoding. */

	while (first <= last) {
		middle = (first + last) / 2;

		if (encoding_current_map[middle].utf8 == unicode) {
			*c = encoding_current_map[middle].target;
			return true;
		} else if (encoding_current_map[middle].utf8 < unicode) {
			first = middle + 1;
		} else {
			last = middle - 1;
		}
	}

	msg_report(MSG_ENC_NO_OUTPUT, unicode, unicode);

	*c = '?';

	return false;
}

/**
 * Return a pointer to the currently selected line end sequence.
 *
 * \return			A pointer to the sequence, or NULL.
 */

const char *encoding_get_newline(void)
{
	if (encoding_current_line_end < 0 || encoding_current_line_end >= ENCODING_LINE_END_MAX)
		return NULL;

	return encoding_line_end_list[encoding_current_line_end].sequence;
}

/**
 * Flatten down the white space in a text string, so that multiple spaces
 * and newlines become a single ASCII space. The supplied buffer is
 * assumed to be zero terminated, and its contents will be updated.
 *
 * \param *text			The text to be flattened.
 */

void encoding_flatten_whitespace(char *text)
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

