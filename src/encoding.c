/* Copyright 2018-2025, Stephen Fryatt (info@stevefryatt.org.uk)
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
};

/**
 * UTF8 to RISC OS Latin 1
 */

static struct encoding_map encoding_acorn_latin1[] = {
	{160,	'\xa0'},	// No-Break Space
	{161,	'\xa1'},	// Inverted Exclamation Mark
	{162,	'\xa2'},	// Cent Sign
	{163,	'\xa3'},	// Pound Sign
	{164,	'\xa4'},	// Currency Sign
	{165,	'\xa5'},	// Yen Sign
	{166,	'\xa6'},	// Broken Bar
	{167,	'\xa7'},	// Section Sign
	{168,	'\xa8'},	// Diaeresis
	{169,	'\xa9'},	// Copyright Sign
	{170,	'\xaa'},	// Feminine Ordinal Indicator
	{171,	'\xab'},	// Left-Pointing Double Angle Quotation Mark
	{172,	'\xac'},	// Not Sign
	{173,	'\xad'},	// Soft Hyphen
	{174,	'\xae'},	// Registered Sign
	{175,	'\xaf'},	// Macron
	{176,	'\xb0'},	// Degree Sign
	{177,	'\xb1'},	// Plus-Minus Sign
	{178,	'\xb2'},	// Superscript Two
	{179,	'\xb3'},	// Superscript Three
	{180,	'\xb4'},	// Acute Accent
	{181,	'\xb5'},	// Micro Sign
	{182,	'\xb6'},	// Pilcrow Sign
	{183,	'\xb7'},	// Middle Dot
	{184,	'\xb8'},	// Cedilla
	{185,	'\xb9'},	// Superscript One
	{186,	'\xba'},	// Masculine Ordinal Indicator
	{187,	'\xbb'},	// Right-Pointing Double Angle Quotation Mark
	{188,	'\xbc'},	// Vulgar Fraction One Quarter
	{189,	'\xbd'},	// Vulgar Fraction One Half
	{190,	'\xbe'},	// Vulgar Fraction Three Quarters
	{191,	'\xbf'},	// Inverted Question Mark
	{192,	'\xc0'},	// Latin Capital Letter A With Grave
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{195,	'\xc3'},	// Latin Capital Letter A With Tilde
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{197,	'\xc5'},	// Latin Capital Letter A With Ring Above
	{198,	'\xc6'},	// Latin Capital Letter AE
	{199,	'\xc7'},	// Latin Capital Letter C With Cedilla
	{200,	'\xc8'},	// Latin Capital Letter E With Grave
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{202,	'\xca'},	// Latin Capital Letter E With Circumflex
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{204,	'\xcc'},	// Latin Capital Letter I With Grave
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{207,	'\xcf'},	// Latin Capital Letter I With Diaeresis
	{208,	'\xd0'},	// Latin Capital Letter Eth
	{209,	'\xd1'},	// Latin Capital Letter N With Tilde
	{210,	'\xd2'},	// Latin Capital Letter O With Grave
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{213,	'\xd5'},	// Latin Capital Letter O With Tilde
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{215,	'\xd7'},	// Multiplication Sign
	{216,	'\xd8'},	// Latin Capital Letter O With Stroke
	{217,	'\xd9'},	// Latin Capital Letter U With Grave
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{221,	'\xdd'},	// Latin Capital Letter Y With Acute
	{222,	'\xde'},	// Latin Capital Letter Thorn
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{224,	'\xe0'},	// Latin Small Letter A With Grave
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{227,	'\xe3'},	// Latin Small Letter A With Tilde
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{229,	'\xe5'},	// Latin Small Letter A With Ring Above
	{230,	'\xe6'},	// Latin Small Letter AE
	{231,	'\xe7'},	// Latin Small Letter C With Cedilla
	{232,	'\xe8'},	// Latin Small Letter E With Grave
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{234,	'\xea'},	// Latin Small Letter E With Circumflex
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{236,	'\xec'},	// Latin Small Letter I With Grave
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{239,	'\xef'},	// Latin Small Letter I With Diaeresis
	{240,	'\xf0'},	// Latin Small Letter Eth
	{241,	'\xf1'},	// Latin Small Letter N With Tilde
	{242,	'\xf2'},	// Latin Small Letter O With Grave
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{245,	'\xf5'},	// Latin Small Letter O With Tilde
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{247,	'\xf7'},	// Division Sign
	{248,	'\xf8'},	// Latin Small Letter O With Stroke
	{249,	'\xf9'},	// Latin Small Letter U With Grave
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{253,	'\xfd'},	// Latin Small Letter Y With Acute
	{254,	'\xfe'},	// Latin Small Letter Thorn
	{255,	'\xff'},	// Latin Small Letter Y With Diaeresis
	{338,	'\x9a'},	// Latin Capital Ligature OE
	{339,	'\x9b'},	// Latin Small Ligature OE
	{372,	'\x81'},	// Latin Capital Letter W With Circumflex
	{373,	'\x82'},	// Latin Small Letter W With Circumflex
	{374,	'\x85'},	// Latin Capital Letter Y With Circumflex
	{375,	'\x86'},	// Latin Small Letter Y With Circumflex
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 2
 */

static struct encoding_map encoding_acorn_latin2[] = {
	{160,	'\xa0'},	// No-Break Space
	{164,	'\xa4'},	// Currency Sign
	{167,	'\xa7'},	// Section Sign
	{168,	'\xa8'},	// Diaeresis
	{173,	'\xad'},	// Soft Hyphen
	{176,	'\xb0'},	// Degree Sign
	{180,	'\xb4'},	// Acute Accent
	{184,	'\xb8'},	// Cedilla
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{199,	'\xc7'},	// Latin Capital Letter C With Cedilla
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{215,	'\xd7'},	// Multiplication Sign
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{221,	'\xdd'},	// Latin Capital Letter Y With Acute
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{231,	'\xe7'},	// Latin Small Letter C With Cedilla
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{247,	'\xf7'},	// Division Sign
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{253,	'\xfd'},	// Latin Small Letter Y With Acute
	{258,	'\xc3'},	// Latin Capital Letter A With Breve
	{259,	'\xe3'},	// Latin Small Letter A With Breve
	{260,	'\xa1'},	// Latin Capital Letter A With Ogonek
	{261,	'\xb1'},	// Latin Small Letter A With Ogonek
	{262,	'\xc6'},	// Latin Capital Letter C With Acute
	{263,	'\xe6'},	// Latin Small Letter C With Acute
	{268,	'\xc8'},	// Latin Capital Letter C With Caron
	{269,	'\xe8'},	// Latin Small Letter C With Caron
	{270,	'\xcf'},	// Latin Capital Letter D With Caron
	{271,	'\xef'},	// Latin Small Letter D With Caron
	{272,	'\xd0'},	// Latin Capital Letter D With Stroke
	{273,	'\xf0'},	// Latin Small Letter D With Stroke
	{280,	'\xca'},	// Latin Capital Letter E With Ogonek
	{281,	'\xea'},	// Latin Small Letter E With Ogonek
	{282,	'\xcc'},	// Latin Capital Letter E With Caron
	{283,	'\xec'},	// Latin Small Letter E With Caron
	{313,	'\xc5'},	// Latin Capital Letter L With Acute
	{314,	'\xe5'},	// Latin Small Letter L With Acute
	{317,	'\xa5'},	// Latin Capital Letter L With Caron
	{318,	'\xb5'},	// Latin Small Letter L With Caron
	{321,	'\xa3'},	// Latin Capital Letter L With Stroke
	{322,	'\xb3'},	// Latin Small Letter L With Stroke
	{323,	'\xd1'},	// Latin Capital Letter N With Acute
	{324,	'\xf1'},	// Latin Small Letter N With Acute
	{327,	'\xd2'},	// Latin Capital Letter N With Caron
	{328,	'\xf2'},	// Latin Small Letter N With Caron
	{336,	'\xd5'},	// Latin Capital Letter O With Double Acute
	{337,	'\xf5'},	// Latin Small Letter O With Double Acute
	{338,	'\x9a'},	// Latin Capital Ligature OE
	{339,	'\x9b'},	// Latin Small Ligature OE
	{340,	'\xc0'},	// Latin Capital Letter R With Acute
	{341,	'\xe0'},	// Latin Small Letter R With Acute
	{344,	'\xd8'},	// Latin Capital Letter R With Caron
	{345,	'\xf8'},	// Latin Small Letter R With Caron
	{346,	'\xa6'},	// Latin Capital Letter S With Acute
	{347,	'\xb6'},	// Latin Small Letter S With Acute
	{350,	'\xaa'},	// Latin Capital Letter S With Cedilla
	{351,	'\xba'},	// Latin Small Letter S With Cedilla
	{352,	'\xa9'},	// Latin Capital Letter S With Caron
	{353,	'\xb9'},	// Latin Small Letter S With Caron
	{354,	'\xde'},	// Latin Capital Letter T With Cedilla
	{355,	'\xfe'},	// Latin Small Letter T With Cedilla
	{356,	'\xab'},	// Latin Capital Letter T With Caron
	{357,	'\xbb'},	// Latin Small Letter T With Caron
	{366,	'\xd9'},	// Latin Capital Letter U With Ring Above
	{367,	'\xf9'},	// Latin Small Letter U With Ring Above
	{368,	'\xdb'},	// Latin Capital Letter U With Double Acute
	{369,	'\xfb'},	// Latin Small Letter U With Double Acute
	{377,	'\xac'},	// Latin Capital Letter Z With Acute
	{378,	'\xbc'},	// Latin Small Letter Z With Acute
	{379,	'\xaf'},	// Latin Capital Letter Z With Dot Above
	{380,	'\xbf'},	// Latin Small Letter Z With Dot Above
	{381,	'\xae'},	// Latin Capital Letter Z With Caron
	{382,	'\xbe'},	// Latin Small Letter Z With Caron
	{711,	'\xb7'},	// Caron
	{728,	'\xa2'},	// Breve
	{729,	'\xff'},	// Dot Above
	{731,	'\xb2'},	// Ogonek
	{733,	'\xbd'},	// Double Acute Accent
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 3
 */

static struct encoding_map encoding_acorn_latin3[] = {
	{160,	'\xa0'},	// No-Break Space
	{163,	'\xa3'},	// Pound Sign
	{164,	'\xa4'},	// Currency Sign
	{167,	'\xa7'},	// Section Sign
	{168,	'\xa8'},	// Diaeresis
	{173,	'\xad'},	// Soft Hyphen
	{176,	'\xb0'},	// Degree Sign
	{178,	'\xb2'},	// Superscript Two
	{179,	'\xb3'},	// Superscript Three
	{180,	'\xb4'},	// Acute Accent
	{181,	'\xb5'},	// Micro Sign
	{183,	'\xb7'},	// Middle Dot
	{184,	'\xb8'},	// Cedilla
	{189,	'\xbd'},	// Vulgar Fraction One Half
	{192,	'\xc0'},	// Latin Capital Letter A With Grave
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{199,	'\xc7'},	// Latin Capital Letter C With Cedilla
	{200,	'\xc8'},	// Latin Capital Letter E With Grave
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{202,	'\xca'},	// Latin Capital Letter E With Circumflex
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{204,	'\xcc'},	// Latin Capital Letter I With Grave
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{207,	'\xcf'},	// Latin Capital Letter I With Diaeresis
	{209,	'\xd1'},	// Latin Capital Letter N With Tilde
	{210,	'\xd2'},	// Latin Capital Letter O With Grave
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{215,	'\xd7'},	// Multiplication Sign
	{217,	'\xd9'},	// Latin Capital Letter U With Grave
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{224,	'\xe0'},	// Latin Small Letter A With Grave
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{231,	'\xe7'},	// Latin Small Letter C With Cedilla
	{232,	'\xe8'},	// Latin Small Letter E With Grave
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{234,	'\xea'},	// Latin Small Letter E With Circumflex
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{236,	'\xec'},	// Latin Small Letter I With Grave
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{239,	'\xef'},	// Latin Small Letter I With Diaeresis
	{241,	'\xf1'},	// Latin Small Letter N With Tilde
	{242,	'\xf2'},	// Latin Small Letter O With Grave
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{247,	'\xf7'},	// Division Sign
	{249,	'\xf9'},	// Latin Small Letter U With Grave
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{264,	'\xc6'},	// Latin Capital Letter C With Circumflex
	{265,	'\xe6'},	// Latin Small Letter C With Circumflex
	{266,	'\xc5'},	// Latin Capital Letter C With Dot Above
	{267,	'\xe5'},	// Latin Small Letter C With Dot Above
	{284,	'\xd8'},	// Latin Capital Letter G With Circumflex
	{285,	'\xf8'},	// Latin Small Letter G With Circumflex
	{286,	'\xab'},	// Latin Capital Letter G With Breve
	{287,	'\xbb'},	// Latin Small Letter G With Breve
	{288,	'\xd5'},	// Latin Capital Letter G With Dot Above
	{289,	'\xf5'},	// Latin Small Letter G With Dot Above
	{292,	'\xa6'},	// Latin Capital Letter H With Circumflex
	{293,	'\xb6'},	// Latin Small Letter H With Circumflex
	{294,	'\xa1'},	// Latin Capital Letter H With Stroke
	{295,	'\xb1'},	// Latin Small Letter H With Stroke
	{304,	'\xa9'},	// Latin Capital Letter I With Dot Above
	{305,	'\xb9'},	// Latin Small Letter Dotless I
	{308,	'\xac'},	// Latin Capital Letter J With Circumflex
	{309,	'\xbc'},	// Latin Small Letter J With Circumflex
	{338,	'\x9a'},	// Latin Capital Ligature Oe
	{339,	'\x9b'},	// Latin Small Ligature Oe
	{348,	'\xde'},	// Latin Capital Letter S With Circumflex
	{349,	'\xfe'},	// Latin Small Letter S With Circumflex
	{350,	'\xaa'},	// Latin Capital Letter S With Cedilla
	{351,	'\xba'},	// Latin Small Letter S With Cedilla
	{364,	'\xdd'},	// Latin Capital Letter U With Breve
	{365,	'\xfd'},	// Latin Small Letter U With Breve
	{379,	'\xaf'},	// Latin Capital Letter Z With Dot Above
	{380,	'\xbf'},	// Latin Small Letter Z With Dot Above
	{728,	'\xa2'},	// Breve
	{729,	'\xff'},	// Dot Above
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 4
 */

static struct encoding_map encoding_acorn_latin4[] = {
	{160,	'\xa0'},	// No-Break Space
	{164,	'\xa4'},	// Currency Sign
	{167,	'\xa7'},	// Section Sign
	{168,	'\xa8'},	// Diaeresis
	{173,	'\xad'},	// Soft Hyphen
	{175,	'\xaf'},	// Macron
	{176,	'\xb0'},	// Degree Sign
	{180,	'\xb4'},	// Acute Accent
	{184,	'\xb8'},	// Cedilla
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{195,	'\xc3'},	// Latin Capital Letter A With Tilde
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{197,	'\xc5'},	// Latin Capital Letter A With Ring Above
	{198,	'\xc6'},	// Latin Capital Letter AE
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{213,	'\xd5'},	// Latin Capital Letter O With Tilde
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{215,	'\xd7'},	// Multiplication Sign
	{216,	'\xd8'},	// Latin Capital Letter O With Stroke
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{227,	'\xe3'},	// Latin Small Letter A With Tilde
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{229,	'\xe5'},	// Latin Small Letter A With Ring Above
	{230,	'\xe6'},	// Latin Small Letter AE
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{245,	'\xf5'},	// Latin Small Letter O With Tilde
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{247,	'\xf7'},	// Division Sign
	{248,	'\xf8'},	// Latin Small Letter O With Stroke
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{256,	'\xc0'},	// Latin Capital Letter A With Macron
	{257,	'\xe0'},	// Latin Small Letter A With Macron
	{260,	'\xa1'},	// Latin Capital Letter A With Ogonek
	{261,	'\xb1'},	// Latin Small Letter A With Ogonek
	{268,	'\xc8'},	// Latin Capital Letter C With Caron
	{269,	'\xe8'},	// Latin Small Letter C With Caron
	{272,	'\xd0'},	// Latin Capital Letter D With Stroke
	{273,	'\xf0'},	// Latin Small Letter D With Stroke
	{274,	'\xaa'},	// Latin Capital Letter E With Macron
	{275,	'\xba'},	// Latin Small Letter E With Macron
	{278,	'\xcc'},	// Latin Capital Letter E With Dot Above
	{279,	'\xec'},	// Latin Small Letter E With Dot Above
	{280,	'\xca'},	// Latin Capital Letter E With Ogonek
	{281,	'\xea'},	// Latin Small Letter E With Ogonek
	{290,	'\xab'},	// Latin Capital Letter G With Cedilla
	{291,	'\xbb'},	// Latin Small Letter G With Cedilla
	{296,	'\xa5'},	// Latin Capital Letter I With Tilde
	{297,	'\xb5'},	// Latin Small Letter I With Tilde
	{298,	'\xcf'},	// Latin Capital Letter I With Macron
	{299,	'\xef'},	// Latin Small Letter I With Macron
	{302,	'\xc7'},	// Latin Capital Letter I With Ogonek
	{303,	'\xe7'},	// Latin Small Letter I With Ogonek
	{310,	'\xd3'},	// Latin Capital Letter K With Cedilla
	{311,	'\xf3'},	// Latin Small Letter K With Cedilla
	{312,	'\xa2'},	// Latin Small Letter Kra
	{315,	'\xa6'},	// Latin Capital Letter L With Cedilla
	{316,	'\xb6'},	// Latin Small Letter L With Cedilla
	{325,	'\xd1'},	// Latin Capital Letter N With Cedilla
	{326,	'\xf1'},	// Latin Small Letter N With Cedilla
	{330,	'\xbd'},	// Latin Capital Letter Eng
	{331,	'\xbf'},	// Latin Small Letter Eng
	{332,	'\xd2'},	// Latin Capital Letter O With Macron
	{333,	'\xf2'},	// Latin Small Letter O With Macron
	{338,	'\x9a'},	// Latin Capital Ligature OE
	{339,	'\x9b'},	// Latin Small Ligature OE
	{342,	'\xa3'},	// Latin Capital Letter R With Cedilla
	{343,	'\xb3'},	// Latin Small Letter R With Cedilla
	{352,	'\xa9'},	// Latin Capital Letter S With Caron
	{353,	'\xb9'},	// Latin Small Letter S With Caron
	{358,	'\xac'},	// Latin Capital Letter T With Stroke
	{359,	'\xbc'},	// Latin Small Letter T With Stroke
	{360,	'\xdd'},	// Latin Capital Letter U With Tilde
	{361,	'\xfd'},	// Latin Small Letter U With Tilde
	{362,	'\xde'},	// Latin Capital Letter U With Macron
	{363,	'\xfe'},	// Latin Small Letter U With Macron
	{370,	'\xd9'},	// Latin Capital Letter U With Ogonek
	{371,	'\xf9'},	// Latin Small Letter U With Ogonek
	{381,	'\xae'},	// Latin Capital Letter Z With Caron
	{382,	'\xbe'},	// Latin Small Letter Z With Caron
	{711,	'\xb7'},	// Caron
	{729,	'\xff'},	// Dot Above
	{731,	'\xb2'},	// Ogonek
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 5
 */

static struct encoding_map encoding_acorn_latin5[] = {
	{160,	'\xa0'},	// No-Break Space
	{161,	'\xa1'},	// Inverted Exclamation Mark
	{162,	'\xa2'},	// Cent Sign
	{163,	'\xa3'},	// Pound Sign
	{164,	'\xa4'},	// Currency Sign
	{165,	'\xa5'},	// Yen Sign
	{166,	'\xa6'},	// Broken Bar
	{167,	'\xa7'},	// Section Sign
	{168,	'\xa8'},	// Diaeresis
	{169,	'\xa9'},	// Copyright Sign
	{170,	'\xaa'},	// Feminine Ordinal Indicator
	{171,	'\xab'},	// Left-Pointing Double Angle Quotation Mark
	{172,	'\xac'},	// Not Sign
	{173,	'\xad'},	// Soft Hyphen
	{174,	'\xae'},	// Registered Sign
	{175,	'\xaf'},	// Macron
	{176,	'\xb0'},	// Degree Sign
	{177,	'\xb1'},	// Plus-Minus Sign
	{178,	'\xb2'},	// Superscript Two
	{179,	'\xb3'},	// Superscript Three
	{180,	'\xb4'},	// Acute Accent
	{181,	'\xb5'},	// Micro Sign
	{182,	'\xb6'},	// Pilcrow Sign
	{183,	'\xb7'},	// Middle Dot
	{184,	'\xb8'},	// Cedilla
	{185,	'\xb9'},	// Superscript One
	{186,	'\xba'},	// Masculine Ordinal Indicator
	{187,	'\xbb'},	// Right-Pointing Double Angle Quotation Mark
	{188,	'\xbc'},	// Vulgar Fraction One Quarter
	{189,	'\xbd'},	// Vulgar Fraction One Half
	{190,	'\xbe'},	// Vulgar Fraction Three Quarters
	{191,	'\xbf'},	// Inverted Question Mark
	{192,	'\xc0'},	// Latin Capital Letter A With Grave
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{195,	'\xc3'},	// Latin Capital Letter A With Tilde
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{197,	'\xc5'},	// Latin Capital Letter A With Ring Above
	{198,	'\xc6'},	// Latin Capital Letter AE
	{199,	'\xc7'},	// Latin Capital Letter C With Cedilla
	{200,	'\xc8'},	// Latin Capital Letter E With Grave
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{202,	'\xca'},	// Latin Capital Letter E With Circumflex
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{204,	'\xcc'},	// Latin Capital Letter I With Grave
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{207,	'\xcf'},	// Latin Capital Letter I With Diaeresis
	{209,	'\xd1'},	// Latin Capital Letter N With Tilde
	{210,	'\xd2'},	// Latin Capital Letter O With Grave
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{213,	'\xd5'},	// Latin Capital Letter O With Tilde
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{215,	'\xd7'},	// Multiplication Sign
	{216,	'\xd8'},	// Latin Capital Letter O With Stroke
	{217,	'\xd9'},	// Latin Capital Letter U With Grave
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{224,	'\xe0'},	// Latin Small Letter A With Grave
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{227,	'\xe3'},	// Latin Small Letter A With Tilde
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{229,	'\xe5'},	// Latin Small Letter A With Ring Above
	{230,	'\xe6'},	// Latin Small Letter AE
	{231,	'\xe7'},	// Latin Small Letter C With Cedilla
	{232,	'\xe8'},	// Latin Small Letter E With Grave
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{234,	'\xea'},	// Latin Small Letter E With Circumflex
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{236,	'\xec'},	// Latin Small Letter I With Grave
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{239,	'\xef'},	// Latin Small Letter I With Diaeresis
	{241,	'\xf1'},	// Latin Small Letter N With Tilde
	{242,	'\xf2'},	// Latin Small Letter O With Grave
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{245,	'\xf5'},	// Latin Small Letter O With Tilde
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{247,	'\xf7'},	// Division Sign
	{248,	'\xf8'},	// Latin Small Letter O With Stroke
	{249,	'\xf9'},	// Latin Small Letter U With Grave
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{255,	'\xff'},	// Latin Small Letter Y With Diaeresis
	{286,	'\xd0'},	// Latin Capital Letter G With Breve
	{287,	'\xf0'},	// Latin Small Letter G With Breve
	{304,	'\xdd'},	// Latin Capital Letter I With Dot Above
	{305,	'\xfd'},	// Latin Small Letter Dotless I
	{338,	'\x9a'},	// Latin Capital Ligature OE
	{339,	'\x9b'},	// Latin Small Ligature OE
	{350,	'\xde'},	// Latin Capital Letter S With Cedilla
	{351,	'\xfe'},	// Latin Small Letter S With Cedilla
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 6
 */

static struct encoding_map encoding_acorn_latin6[] = {
	{160,	'\xa0'},	// No-Break Space
	{167,	'\xa7'},	// Section Sign
	{173,	'\xad'},	// Soft Hyphen
	{176,	'\xb0'},	// Degree Sign
	{183,	'\xb7'},	// Middle Dot
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{195,	'\xc3'},	// Latin Capital Letter A With Tilde
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{197,	'\xc5'},	// Latin Capital Letter A With Ring Above
	{198,	'\xc6'},	// Latin Capital Letter AE
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{207,	'\xcf'},	// Latin Capital Letter I With Diaeresis
	{208,	'\xd0'},	// Latin Capital Letter Eth
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{213,	'\xd5'},	// Latin Capital Letter O With Tilde
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{216,	'\xd8'},	// Latin Capital Letter O With Stroke
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{221,	'\xdd'},	// Latin Capital Letter Y With Acute
	{222,	'\xde'},	// Latin Capital Letter Thorn
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{227,	'\xe3'},	// Latin Small Letter A With Tilde
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{229,	'\xe5'},	// Latin Small Letter A With Ring Above
	{230,	'\xe6'},	// Latin Small Letter AE
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{239,	'\xef'},	// Latin Small Letter I With Diaeresis
	{240,	'\xf0'},	// Latin Small Letter Eth
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{245,	'\xf5'},	// Latin Small Letter O With Tilde
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{248,	'\xf8'},	// Latin Small Letter O With Stroke
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{253,	'\xfd'},	// Latin Small Letter Y With Acute
	{254,	'\xfe'},	// Latin Small Letter Thorn
	{256,	'\xc0'},	// Latin Capital Letter A With Macron
	{257,	'\xe0'},	// Latin Small Letter A With Macron
	{260,	'\xa1'},	// Latin Capital Letter A With Ogonek
	{261,	'\xb1'},	// Latin Small Letter A With Ogonek
	{268,	'\xc8'},	// Latin Capital Letter C With Caron
	{269,	'\xe8'},	// Latin Small Letter C With Caron
	{272,	'\xa9'},	// Latin Capital Letter D With Stroke
	{273,	'\xb9'},	// Latin Small Letter D With Stroke
	{274,	'\xa2'},	// Latin Capital Letter E With Macron
	{275,	'\xb2'},	// Latin Small Letter E With Macron
	{278,	'\xcc'},	// Latin Capital Letter E With Dot Above
	{279,	'\xec'},	// Latin Small Letter E With Dot Above
	{280,	'\xca'},	// Latin Capital Letter E With Ogonek
	{281,	'\xea'},	// Latin Small Letter E With Ogonek
	{290,	'\xa3'},	// Latin Capital Letter G With Cedilla
	{291,	'\xb3'},	// Latin Small Letter G With Cedilla
	{296,	'\xa5'},	// Latin Capital Letter I With Tilde
	{297,	'\xb5'},	// Latin Small Letter I With Tilde
	{298,	'\xa4'},	// Latin Capital Letter I With Macron
	{299,	'\xb4'},	// Latin Small Letter I With Macron
	{302,	'\xc7'},	// Latin Capital Letter I With Ogonek
	{303,	'\xe7'},	// Latin Small Letter I With Ogonek
	{310,	'\xa6'},	// Latin Capital Letter K With Cedilla
	{311,	'\xb6'},	// Latin Small Letter K With Cedilla
	{312,	'\xff'},	// Latin Small Letter Kra
	{315,	'\xa8'},	// Latin Capital Letter L With Cedilla
	{316,	'\xb8'},	// Latin Small Letter L With Cedilla
	{325,	'\xd1'},	// Latin Capital Letter N With Cedilla
	{326,	'\xf1'},	// Latin Small Letter N With Cedilla
	{330,	'\xaf'},	// Latin Capital Letter Eng
	{331,	'\xbf'},	// Latin Small Letter Eng
	{332,	'\xd2'},	// Latin Capital Letter O With Macron
	{333,	'\xf2'},	// Latin Small Letter O With Macron
	{338,	'\x9a'},	// Latin Capital Ligature OE
	{339,	'\x9b'},	// Latin Small Ligature OE
	{352,	'\xaa'},	// Latin Capital Letter S With Caron
	{353,	'\xba'},	// Latin Small Letter S With Caron
	{358,	'\xab'},	// Latin Capital Letter T With Stroke
	{359,	'\xbb'},	// Latin Small Letter T With Stroke
	{360,	'\xd7'},	// Latin Capital Letter U With Tilde
	{361,	'\xf7'},	// Latin Small Letter U With Tilde
	{362,	'\xae'},	// Latin Capital Letter U With Macron
	{363,	'\xbe'},	// Latin Small Letter U With Macron
	{370,	'\xd9'},	// Latin Capital Letter U With Ogonek
	{371,	'\xf9'},	// Latin Small Letter U With Ogonek
	{381,	'\xac'},	// Latin Capital Letter Z With Caron
	{382,	'\xbc'},	// Latin Small Letter Z With Caron
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8213,	'\xbd'},	// Horizontal Bar
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 7
 */

static struct encoding_map encoding_acorn_latin7[] = {
	{160,	'\xa0'},	// No-Break Space
	{162,	'\xa2'},	// Cent Sign
	{163,	'\xa3'},	// Pound Sign
	{164,	'\xa4'},	// Currency Sign
	{166,	'\xa6'},	// Broken Bar
	{167,	'\xa7'},	// Section Sign
	{169,	'\xa9'},	// Copyright Sign
	{171,	'\xab'},	// Left-Pointing Double Angle Quotation Mark
	{172,	'\xac'},	// Not Sign
	{173,	'\xad'},	// Soft Hyphen
	{174,	'\xae'},	// Registered Sign
	{176,	'\xb0'},	// Degree Sign
	{177,	'\xb1'},	// Plus-Minus Sign
	{178,	'\xb2'},	// Superscript Two
	{179,	'\xb3'},	// Superscript Three
	{181,	'\xb5'},	// Micro Sign
	{182,	'\xb6'},	// Pilcrow Sign
	{183,	'\xb7'},	// Middle Dot
	{185,	'\xb9'},	// Superscript One
	{187,	'\xbb'},	// Right-Pointing Double Angle Quotation Mark
	{188,	'\xbc'},	// Vulgar Fraction One Quarter
	{189,	'\xbd'},	// Vulgar Fraction One Half
	{190,	'\xbe'},	// Vulgar Fraction Three Quarters
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{197,	'\xc5'},	// Latin Capital Letter A With Ring Above
	{198,	'\xaf'},	// Latin Capital Letter AE
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{213,	'\xd5'},	// Latin Capital Letter O With Tilde
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{215,	'\xd7'},	// Multiplication Sign
	{216,	'\xa8'},	// Latin Capital Letter O With Stroke
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{229,	'\xe5'},	// Latin Small Letter A With Ring Above
	{230,	'\xbf'},	// Latin Small Letter AE
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{245,	'\xf5'},	// Latin Small Letter O With Tilde
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{247,	'\xf7'},	// Division Sign
	{248,	'\xb8'},	// Latin Small Letter O With Stroke
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{256,	'\xc2'},	// Latin Capital Letter A With Macron
	{257,	'\xe2'},	// Latin Small Letter A With Macron
	{260,	'\xc0'},	// Latin Capital Letter A With Ogonek
	{261,	'\xe0'},	// Latin Small Letter A With Ogonek
	{262,	'\xc3'},	// Latin Capital Letter C With Acute
	{263,	'\xe3'},	// Latin Small Letter C With Acute
	{268,	'\xc8'},	// Latin Capital Letter C With Caron
	{269,	'\xe8'},	// Latin Small Letter C With Caron
	{274,	'\xc7'},	// Latin Capital Letter E With Macron
	{275,	'\xe7'},	// Latin Small Letter E With Macron
	{278,	'\xcb'},	// Latin Capital Letter E With Dot Above
	{279,	'\xeb'},	// Latin Small Letter E With Dot Above
	{280,	'\xc6'},	// Latin Capital Letter E With Ogonek
	{281,	'\xe6'},	// Latin Small Letter E With Ogonek
	{290,	'\xcc'},	// Latin Capital Letter G With Cedilla
	{291,	'\xec'},	// Latin Small Letter G With Cedilla
	{298,	'\xce'},	// Latin Capital Letter I With Macron
	{299,	'\xee'},	// Latin Small Letter I With Macron
	{302,	'\xc1'},	// Latin Capital Letter I With Ogonek
	{303,	'\xe1'},	// Latin Small Letter I With Ogonek
	{310,	'\xcd'},	// Latin Capital Letter K With Cedilla
	{311,	'\xed'},	// Latin Small Letter K With Cedilla
	{315,	'\xcf'},	// Latin Capital Letter L With Cedilla
	{316,	'\xef'},	// Latin Small Letter L With Cedilla
	{321,	'\xd9'},	// Latin Capital Letter L With Stroke
	{322,	'\xf9'},	// Latin Small Letter L With Stroke
	{323,	'\xd1'},	// Latin Capital Letter N With Acute
	{324,	'\xf1'},	// Latin Small Letter N With Acute
	{325,	'\xd2'},	// Latin Capital Letter N With Cedilla
	{326,	'\xf2'},	// Latin Small Letter N With Cedilla
	{332,	'\xd4'},	// Latin Capital Letter O With Macron
	{333,	'\xf4'},	// Latin Small Letter O With Macron
	{338,	'\x9a'},	// Latin Capital Ligature OE
	{339,	'\x9b'},	// Latin Small Ligature OE
	{342,	'\xaa'},	// Latin Capital Letter R With Cedilla
	{343,	'\xba'},	// Latin Small Letter R With Cedilla
	{346,	'\xda'},	// Latin Capital Letter S With Acute
	{347,	'\xfa'},	// Latin Small Letter S With Acute
	{352,	'\xd0'},	// Latin Capital Letter S With Caron
	{353,	'\xf0'},	// Latin Small Letter S With Caron
	{362,	'\xdb'},	// Latin Capital Letter U With Macron
	{363,	'\xfb'},	// Latin Small Letter U With Macron
	{370,	'\xd8'},	// Latin Capital Letter U With Ogonek
	{371,	'\xf8'},	// Latin Small Letter U With Ogonek
	{377,	'\xca'},	// Latin Capital Letter Z With Acute
	{378,	'\xea'},	// Latin Small Letter Z With Acute
	{379,	'\xdd'},	// Latin Capital Letter Z With Dot Above
	{380,	'\xfd'},	// Latin Small Letter Z With Dot Above
	{381,	'\xde'},	// Latin Capital Letter Z With Caron
	{382,	'\xfe'},	// Latin Small Letter Z With Caron
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\xff'},	// Right Single Quotation Mark
	{8220,	'\xb4'},	// Left Double Quotation Mark
	{8221,	'\xa1'},	// Right Double Quotation Mark
	{8222,	'\xa5'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 8
 */

static struct encoding_map encoding_acorn_latin8[] = {
	{160,	'\xa0'},	// No-Break Space
	{163,	'\xa3'},	// Pound Sign
	{167,	'\xa7'},	// Section Sign
	{169,	'\xa9'},	// Copyright Sign
	{173,	'\xad'},	// Soft Hyphen
	{174,	'\xae'},	// Registered Sign
	{182,	'\xb6'},	// Pilcrow Sign
	{192,	'\xc0'},	// Latin Capital Letter A With Grave
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{195,	'\xc3'},	// Latin Capital Letter A With Tilde
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{197,	'\xc5'},	// Latin Capital Letter A With Ring Above
	{198,	'\xc6'},	// Latin Capital Letter AE
	{199,	'\xc7'},	// Latin Capital Letter C With Cedilla
	{200,	'\xc8'},	// Latin Capital Letter E With Grave
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{202,	'\xca'},	// Latin Capital Letter E With Circumflex
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{204,	'\xcc'},	// Latin Capital Letter I With Grave
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{207,	'\xcf'},	// Latin Capital Letter I With Diaeresis
	{209,	'\xd1'},	// Latin Capital Letter N With Tilde
	{210,	'\xd2'},	// Latin Capital Letter O With Grave
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{213,	'\xd5'},	// Latin Capital Letter O With Tilde
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{216,	'\xd8'},	// Latin Capital Letter O With Stroke
	{217,	'\xd9'},	// Latin Capital Letter U With Grave
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{221,	'\xdd'},	// Latin Capital Letter Y With Acute
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{224,	'\xe0'},	// Latin Small Letter A With Grave
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{227,	'\xe3'},	// Latin Small Letter A With Tilde
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{229,	'\xe5'},	// Latin Small Letter A With Ring Above
	{230,	'\xe6'},	// Latin Small Letter AE
	{231,	'\xe7'},	// Latin Small Letter C With Cedilla
	{232,	'\xe8'},	// Latin Small Letter E With Grave
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{234,	'\xea'},	// Latin Small Letter E With Circumflex
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{236,	'\xec'},	// Latin Small Letter I With Grave
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{239,	'\xef'},	// Latin Small Letter I With Diaeresis
	{241,	'\xf1'},	// Latin Small Letter N With Tilde
	{242,	'\xf2'},	// Latin Small Letter O With Grave
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{245,	'\xf5'},	// Latin Small Letter O With Tilde
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{248,	'\xf8'},	// Latin Small Letter O With Stroke
	{249,	'\xf9'},	// Latin Small Letter U With Grave
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{253,	'\xfd'},	// Latin Small Letter Y With Acute
	{255,	'\xff'},	// Latin Small Letter Y With Diaeresis
	{266,	'\xa4'},	// Latin Capital Letter C With Dot Above
	{267,	'\xa5'},	// Latin Small Letter C With Dot Above
	{288,	'\xb2'},	// Latin Capital Letter G With Dot Above
	{289,	'\xb3'},	// Latin Small Letter G With Dot Above
	{338,	'\x9a'},	// Latin Capital Ligature OE
	{339,	'\x9b'},	// Latin Small Ligature OE
	{372,	'\xd0'},	// Latin Capital Letter W With Circumflex
	{373,	'\xf0'},	// Latin Small Letter W With Circumflex
	{374,	'\xde'},	// Latin Capital Letter Y With Circumflex
	{375,	'\xfe'},	// Latin Small Letter Y With Circumflex
	{376,	'\xaf'},	// Latin Capital Letter Y With Diaeresis
	{7682,	'\xa1'},	// Latin Capital Letter B With Dot Above
	{7683,	'\xa2'},	// Latin Small Letter B With Dot Above
	{7690,	'\xa6'},	// Latin Capital Letter D With Dot Above
	{7691,	'\xab'},	// Latin Small Letter D With Dot Above
	{7710,	'\xb0'},	// Latin Capital Letter F With Dot Above
	{7711,	'\xb1'},	// Latin Small Letter F With Dot Above
	{7744,	'\xb4'},	// Latin Capital Letter M With Dot Above
	{7745,	'\xb5'},	// Latin Small Letter M With Dot Above
	{7766,	'\xb7'},	// Latin Capital Letter P With Dot Above
	{7767,	'\xb9'},	// Latin Small Letter P With Dot Above
	{7776,	'\xbb'},	// Latin Capital Letter S With Dot Above
	{7777,	'\xbf'},	// Latin Small Letter S With Dot Above
	{7786,	'\xd7'},	// Latin Capital Letter T With Dot Above
	{7787,	'\xf7'},	// Latin Small Letter T With Dot Above
	{7808,	'\xa8'},	// Latin Capital Letter W With Grave
	{7809,	'\xb8'},	// Latin Small Letter W With Grave
	{7810,	'\xaa'},	// Latin Capital Letter W With Acute
	{7811,	'\xba'},	// Latin Small Letter W With Acute
	{7812,	'\xbd'},	// Latin Capital Letter W With Diaeresis
	{7813,	'\xbe'},	// Latin Small Letter W With Diaeresis
	{7922,	'\xac'},	// Latin Capital Letter Y With Grave
	{7923,	'\xbc'},	// Latin Small Letter Y With Grave
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 9
 */

static struct encoding_map encoding_acorn_latin9[] = {
	{160,	'\xa0'},	// No-Break Space
	{161,	'\xa1'},	// Inverted Exclamation Mark
	{162,	'\xa2'},	// Cent Sign
	{163,	'\xa3'},	// Pound Sign
	{165,	'\xa5'},	// Yen Sign
	{167,	'\xa7'},	// Section Sign
	{169,	'\xa9'},	// Copyright Sign
	{170,	'\xaa'},	// Feminine Ordinal Indicator
	{171,	'\xab'},	// Left-Pointing Double Angle Quotation Mark
	{172,	'\xac'},	// Not Sign
	{173,	'\xad'},	// Soft Hyphen
	{174,	'\xae'},	// Registered Sign
	{175,	'\xaf'},	// Macron
	{176,	'\xb0'},	// Degree Sign
	{177,	'\xb1'},	// Plus-Minus Sign
	{178,	'\xb2'},	// Superscript Two
	{179,	'\xb3'},	// Superscript Three
	{181,	'\xb5'},	// Micro Sign
	{182,	'\xb6'},	// Pilcrow Sign
	{183,	'\xb7'},	// Middle Dot
	{185,	'\xb9'},	// Superscript One
	{186,	'\xba'},	// Masculine Ordinal Indicator
	{187,	'\xbb'},	// Right-Pointing Double Angle Quotation Mark
	{191,	'\xbf'},	// Inverted Question Mark
	{192,	'\xc0'},	// Latin Capital Letter A With Grave
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{195,	'\xc3'},	// Latin Capital Letter A With Tilde
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{197,	'\xc5'},	// Latin Capital Letter A With Ring Above
	{198,	'\xc6'},	// Latin Capital Letter AE
	{199,	'\xc7'},	// Latin Capital Letter C With Cedilla
	{200,	'\xc8'},	// Latin Capital Letter E With Grave
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{202,	'\xca'},	// Latin Capital Letter E With Circumflex
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{204,	'\xcc'},	// Latin Capital Letter I With Grave
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{207,	'\xcf'},	// Latin Capital Letter I With Diaeresis
	{208,	'\xd0'},	// Latin Capital Letter Eth
	{209,	'\xd1'},	// Latin Capital Letter N With Tilde
	{210,	'\xd2'},	// Latin Capital Letter O With Grave
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{213,	'\xd5'},	// Latin Capital Letter O With Tilde
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{215,	'\xd7'},	// Multiplication Sign
	{216,	'\xd8'},	// Latin Capital Letter O With Stroke
	{217,	'\xd9'},	// Latin Capital Letter U With Grave
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{221,	'\xdd'},	// Latin Capital Letter Y With Acute
	{222,	'\xde'},	// Latin Capital Letter Thorn
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{224,	'\xe0'},	// Latin Small Letter A With Grave
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{227,	'\xe3'},	// Latin Small Letter A With Tilde
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{229,	'\xe5'},	// Latin Small Letter A With Ring Above
	{230,	'\xe6'},	// Latin Small Letter AE
	{231,	'\xe7'},	// Latin Small Letter C With Cedilla
	{232,	'\xe8'},	// Latin Small Letter E With Grave
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{234,	'\xea'},	// Latin Small Letter E With Circumflex
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{236,	'\xec'},	// Latin Small Letter I With Grave
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{239,	'\xef'},	// Latin Small Letter I With Diaeresis
	{240,	'\xf0'},	// Latin Small Letter Eth
	{241,	'\xf1'},	// Latin Small Letter N With Tilde
	{242,	'\xf2'},	// Latin Small Letter O With Grave
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{245,	'\xf5'},	// Latin Small Letter O With Tilde
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{247,	'\xf7'},	// Division Sign
	{248,	'\xf8'},	// Latin Small Letter O With Stroke
	{249,	'\xf9'},	// Latin Small Letter U With Grave
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{253,	'\xfd'},	// Latin Small Letter Y With Acute
	{254,	'\xfe'},	// Latin Small Letter Thorn
	{255,	'\xff'},	// Latin Small Letter Y With Diaeresis
	{338,	'\xbc'},	// Latin Capital Ligature OE
	{339,	'\xbd'},	// Latin Small Ligature OE
	{352,	'\xa6'},	// Latin Capital Letter S With Caron
	{353,	'\xa8'},	// Latin Small Letter S With Caron
	{372,	'\x81'},	// Latin Capital Letter W With Circumflex
	{373,	'\x82'},	// Latin Small Letter W With Circumflex
	{374,	'\x85'},	// Latin Capital Letter Y With Circumflex
	{375,	'\x86'},	// Latin Small Letter Y With Circumflex
	{376,	'\xbe'},	// Latin Capital Letter Y With Diaeresis
	{381,	'\xb4'},	// Latin Capital Letter Z With Caron
	{382,	'\xb8'},	// Latin Small Letter Z With Caron
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\xa4'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Latin 10
 */

static struct encoding_map encoding_acorn_latin10[] = {
	{160,	'\xa0'},	// No-Break Space
	{167,	'\xa7'},	// Section Sign
	{169,	'\xa9'},	// Copyright Sign
	{171,	'\xab'},	// Left-Pointing Double Angle Quotation Mark
	{173,	'\xad'},	// Soft Hyphen
	{176,	'\xb0'},	// Degree Sign
	{177,	'\xb1'},	// Plus-Minus Sign
	{182,	'\xb6'},	// Pilcrow Sign
	{183,	'\xb7'},	// Middle Dot
	{187,	'\xbb'},	// Right-Pointing Double Angle Quotation Mark
	{192,	'\xc0'},	// Latin Capital Letter A With Grave
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{198,	'\xc6'},	// Latin Capital Letter AE
	{199,	'\xc7'},	// Latin Capital Letter C With Cedilla
	{200,	'\xc8'},	// Latin Capital Letter E With Grave
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{202,	'\xca'},	// Latin Capital Letter E With Circumflex
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{204,	'\xcc'},	// Latin Capital Letter I With Grave
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{207,	'\xcf'},	// Latin Capital Letter I With Diaeresis
	{210,	'\xd2'},	// Latin Capital Letter O With Grave
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{217,	'\xd9'},	// Latin Capital Letter U With Grave
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{224,	'\xe0'},	// Latin Small Letter A With Grave
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{230,	'\xe6'},	// Latin Small Letter AE
	{231,	'\xe7'},	// Latin Small Letter C With Cedilla
	{232,	'\xe8'},	// Latin Small Letter E With Grave
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{234,	'\xea'},	// Latin Small Letter E With Circumflex
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{236,	'\xec'},	// Latin Small Letter I With Grave
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{239,	'\xef'},	// Latin Small Letter I With Diaeresis
	{242,	'\xf2'},	// Latin Small Letter O With Grave
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{249,	'\xf9'},	// Latin Small Letter U With Grave
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{255,	'\xff'},	// Latin Small Letter Y With Diaeresis
	{258,	'\xc3'},	// Latin Capital Letter A With Breve
	{259,	'\xe3'},	// Latin Small Letter A With Breve
	{260,	'\xa1'},	// Latin Capital Letter A With Ogonek
	{261,	'\xa2'},	// Latin Small Letter A With Ogonek
	{262,	'\xc5'},	// Latin Capital Letter C With Acute
	{263,	'\xe5'},	// Latin Small Letter C With Acute
	{268,	'\xb2'},	// Latin Capital Letter C With Caron
	{269,	'\xb9'},	// Latin Small Letter C With Caron
	{272,	'\xd0'},	// Latin Capital Letter D With Stroke
	{273,	'\xf0'},	// Latin Small Letter D With Stroke
	{280,	'\xdd'},	// Latin Capital Letter E With Ogonek
	{281,	'\xfd'},	// Latin Small Letter E With Ogonek
	{321,	'\xa3'},	// Latin Capital Letter L With Stroke
	{322,	'\xb3'},	// Latin Small Letter L With Stroke
	{323,	'\xd1'},	// Latin Capital Letter N With Acute
	{324,	'\xf1'},	// Latin Small Letter N With Acute
	{336,	'\xd5'},	// Latin Capital Letter O With Double Acute
	{337,	'\xf5'},	// Latin Small Letter O With Double Acute
	{338,	'\xbc'},	// Latin Capital Ligature OE
	{339,	'\xbd'},	// Latin Small Ligature OE
	{346,	'\xd7'},	// Latin Capital Letter S With Acute
	{347,	'\xf7'},	// Latin Small Letter S With Acute
	{352,	'\xa6'},	// Latin Capital Letter S With Caron
	{353,	'\xa8'},	// Latin Small Letter S With Caron
	{368,	'\xd8'},	// Latin Capital Letter U With Double Acute
	{369,	'\xf8'},	// Latin Small Letter U With Double Acute
	{376,	'\xbe'},	// Latin Capital Letter Y With Diaeresis
	{377,	'\xac'},	// Latin Capital Letter Z With Acute
	{378,	'\xae'},	// Latin Small Letter Z With Acute
	{379,	'\xaf'},	// Latin Capital Letter Z With Dot Above
	{380,	'\xbf'},	// Latin Small Letter Z With Dot Above
	{381,	'\xb4'},	// Latin Capital Letter Z With Caron
	{382,	'\xb8'},	// Latin Small Letter Z With Caron
	{536,	'\xaa'},	// Latin Capital Letter S With Comma Below
	{537,	'\xba'},	// Latin Small Letter S With Comma Below
	{538,	'\xde'},	// Latin Capital Letter T With Comma Below
	{539,	'\xfe'},	// Latin Small Letter T With Comma Below
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\xb5'},	// Right Double Quotation Mark
	{8222,	'\xa5'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\xa4'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Cyrillic
 */

static struct encoding_map encoding_acorn_cyrillic[] = {
	{160,	'\xa0'},	// No-Break Space
	{167,	'\xfd'},	// Section Sign
	{173,	'\xad'},	// Soft Hyphen
	{1025,	'\xa1'},	// Cyrillic Capital Letter Io
	{1026,	'\xa2'},	// Cyrillic Capital Letter Dje
	{1027,	'\xa3'},	// Cyrillic Capital Letter Gje
	{1028,	'\xa4'},	// Cyrillic Capital Letter Ukrainian Ie
	{1029,	'\xa5'},	// Cyrillic Capital Letter Dze
	{1030,	'\xa6'},	// Cyrillic Capital Letter Byelorussian-Ukrainian I
	{1031,	'\xa7'},	// Cyrillic Capital Letter Yi
	{1032,	'\xa8'},	// Cyrillic Capital Letter Je
	{1033,	'\xa9'},	// Cyrillic Capital Letter Lje
	{1034,	'\xaa'},	// Cyrillic Capital Letter Nje
	{1035,	'\xab'},	// Cyrillic Capital Letter Tshe
	{1036,	'\xac'},	// Cyrillic Capital Letter Kje
	{1038,	'\xae'},	// Cyrillic Capital Letter Short U
	{1039,	'\xaf'},	// Cyrillic Capital Letter Dzhe
	{1040,	'\xb0'},	// Cyrillic Capital Letter A
	{1041,	'\xb1'},	// Cyrillic Capital Letter Be
	{1042,	'\xb2'},	// Cyrillic Capital Letter Ve
	{1043,	'\xb3'},	// Cyrillic Capital Letter Ghe
	{1044,	'\xb4'},	// Cyrillic Capital Letter De
	{1045,	'\xb5'},	// Cyrillic Capital Letter Ie
	{1046,	'\xb6'},	// Cyrillic Capital Letter Zhe
	{1047,	'\xb7'},	// Cyrillic Capital Letter Ze
	{1048,	'\xb8'},	// Cyrillic Capital Letter I
	{1049,	'\xb9'},	// Cyrillic Capital Letter Short I
	{1050,	'\xba'},	// Cyrillic Capital Letter Ka
	{1051,	'\xbb'},	// Cyrillic Capital Letter El
	{1052,	'\xbc'},	// Cyrillic Capital Letter Em
	{1053,	'\xbd'},	// Cyrillic Capital Letter En
	{1054,	'\xbe'},	// Cyrillic Capital Letter O
	{1055,	'\xbf'},	// Cyrillic Capital Letter Pe
	{1056,	'\xc0'},	// Cyrillic Capital Letter Er
	{1057,	'\xc1'},	// Cyrillic Capital Letter Es
	{1058,	'\xc2'},	// Cyrillic Capital Letter Te
	{1059,	'\xc3'},	// Cyrillic Capital Letter U
	{1060,	'\xc4'},	// Cyrillic Capital Letter Ef
	{1061,	'\xc5'},	// Cyrillic Capital Letter Ha
	{1062,	'\xc6'},	// Cyrillic Capital Letter Tse
	{1063,	'\xc7'},	// Cyrillic Capital Letter Che
	{1064,	'\xc8'},	// Cyrillic Capital Letter Sha
	{1065,	'\xc9'},	// Cyrillic Capital Letter Shcha
	{1066,	'\xca'},	// Cyrillic Capital Letter Hard Sign
	{1067,	'\xcb'},	// Cyrillic Capital Letter Yeru
	{1068,	'\xcc'},	// Cyrillic Capital Letter Soft Sign
	{1069,	'\xcd'},	// Cyrillic Capital Letter E
	{1070,	'\xce'},	// Cyrillic Capital Letter Yu
	{1071,	'\xcf'},	// Cyrillic Capital Letter Ya
	{1072,	'\xd0'},	// Cyrillic Small Letter A
	{1073,	'\xd1'},	// Cyrillic Small Letter Be
	{1074,	'\xd2'},	// Cyrillic Small Letter Ve
	{1075,	'\xd3'},	// Cyrillic Small Letter Ghe
	{1076,	'\xd4'},	// Cyrillic Small Letter De
	{1077,	'\xd5'},	// Cyrillic Small Letter Ie
	{1078,	'\xd6'},	// Cyrillic Small Letter Zhe
	{1079,	'\xd7'},	// Cyrillic Small Letter Ze
	{1080,	'\xd8'},	// Cyrillic Small Letter I
	{1081,	'\xd9'},	// Cyrillic Small Letter Short I
	{1082,	'\xda'},	// Cyrillic Small Letter Ka
	{1083,	'\xdb'},	// Cyrillic Small Letter El
	{1084,	'\xdc'},	// Cyrillic Small Letter Em
	{1085,	'\xdd'},	// Cyrillic Small Letter En
	{1086,	'\xde'},	// Cyrillic Small Letter O
	{1087,	'\xdf'},	// Cyrillic Small Letter Pe
	{1088,	'\xe0'},	// Cyrillic Small Letter Er
	{1089,	'\xe1'},	// Cyrillic Small Letter Es
	{1090,	'\xe2'},	// Cyrillic Small Letter Te
	{1091,	'\xe3'},	// Cyrillic Small Letter U
	{1092,	'\xe4'},	// Cyrillic Small Letter Ef
	{1093,	'\xe5'},	// Cyrillic Small Letter Ha
	{1094,	'\xe6'},	// Cyrillic Small Letter Tse
	{1095,	'\xe7'},	// Cyrillic Small Letter Che
	{1096,	'\xe8'},	// Cyrillic Small Letter Sha
	{1097,	'\xe9'},	// Cyrillic Small Letter Shcha
	{1098,	'\xea'},	// Cyrillic Small Letter Hard Sign
	{1099,	'\xeb'},	// Cyrillic Small Letter Yeru
	{1100,	'\xec'},	// Cyrillic Small Letter Soft Sign
	{1101,	'\xed'},	// Cyrillic Small Letter E
	{1102,	'\xee'},	// Cyrillic Small Letter Yu
	{1103,	'\xef'},	// Cyrillic Small Letter Ya
	{1105,	'\xf1'},	// Cyrillic Small Letter Io
	{1106,	'\xf2'},	// Cyrillic Small Letter Dje
	{1107,	'\xf3'},	// Cyrillic Small Letter Gje
	{1108,	'\xf4'},	// Cyrillic Small Letter Ukrainian Ie
	{1109,	'\xf5'},	// Cyrillic Small Letter Dze
	{1110,	'\xf6'},	// Cyrillic Small Letter Byelorussian-Ukrainian I
	{1111,	'\xf7'},	// Cyrillic Small Letter Yi
	{1112,	'\xf8'},	// Cyrillic Small Letter Je
	{1113,	'\xf9'},	// Cyrillic Small Letter Lje
	{1114,	'\xfa'},	// Cyrillic Small Letter Nje
	{1115,	'\xfb'},	// Cyrillic Small Letter Tshe
	{1116,	'\xfc'},	// Cyrillic Small Letter Kje
	{1118,	'\xfe'},	// Cyrillic Small Letter Short U
	{1119,	'\xff'},	// Cyrillic Small Letter Dzhe
	{8470,	'\xf0'},	// Numero Sign
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Welsh
 */

static struct encoding_map encoding_acorn_welsh[] = {
	{160,	'\xa0'},	// No-Break Space
	{161,	'\xa1'},	// Inverted Exclamation Mark
	{162,	'\xa2'},	// Cent Sign
	{163,	'\xa3'},	// Pound Sign
	{164,	'\xa4'},	// Currency Sign
	{165,	'\xa5'},	// Yen Sign
	{166,	'\xa6'},	// Broken Bar
	{167,	'\xa7'},	// Section Sign
	{169,	'\xa9'},	// Copyright Sign
	{171,	'\xab'},	// Left-Pointing Double Angle Quotation Mark
	{173,	'\xad'},	// Soft Hyphen
	{174,	'\xae'},	// Registered Sign
	{176,	'\xb0'},	// Degree Sign
	{177,	'\xb1'},	// Plus-Minus Sign
	{178,	'\xb2'},	// Superscript Two
	{179,	'\xb3'},	// Superscript Three
	{180,	'\xb4'},	// Acute Accent
	{181,	'\xb5'},	// Micro Sign
	{182,	'\xb6'},	// Pilcrow Sign
	{183,	'\xb7'},	// Middle Dot
	{185,	'\xb9'},	// Superscript One
	{187,	'\xbb'},	// Right-Pointing Double Angle Quotation Mark
	{191,	'\xbf'},	// Inverted Question Mark
	{192,	'\xc0'},	// Latin Capital Letter A With Grave
	{193,	'\xc1'},	// Latin Capital Letter A With Acute
	{194,	'\xc2'},	// Latin Capital Letter A With Circumflex
	{195,	'\xc3'},	// Latin Capital Letter A With Tilde
	{196,	'\xc4'},	// Latin Capital Letter A With Diaeresis
	{197,	'\xc5'},	// Latin Capital Letter A With Ring Above
	{198,	'\xc6'},	// Latin Capital Letter AE
	{199,	'\xc7'},	// Latin Capital Letter C With Cedilla
	{200,	'\xc8'},	// Latin Capital Letter E With Grave
	{201,	'\xc9'},	// Latin Capital Letter E With Acute
	{202,	'\xca'},	// Latin Capital Letter E With Circumflex
	{203,	'\xcb'},	// Latin Capital Letter E With Diaeresis
	{204,	'\xcc'},	// Latin Capital Letter I With Grave
	{205,	'\xcd'},	// Latin Capital Letter I With Acute
	{206,	'\xce'},	// Latin Capital Letter I With Circumflex
	{207,	'\xcf'},	// Latin Capital Letter I With Diaeresis
	{209,	'\xd1'},	// Latin Capital Letter N With Tilde
	{210,	'\xd2'},	// Latin Capital Letter O With Grave
	{211,	'\xd3'},	// Latin Capital Letter O With Acute
	{212,	'\xd4'},	// Latin Capital Letter O With Circumflex
	{213,	'\xd5'},	// Latin Capital Letter O With Tilde
	{214,	'\xd6'},	// Latin Capital Letter O With Diaeresis
	{215,	'\xd7'},	// Multiplication Sign
	{216,	'\xd8'},	// Latin Capital Letter O With Stroke
	{217,	'\xd9'},	// Latin Capital Letter U With Grave
	{218,	'\xda'},	// Latin Capital Letter U With Acute
	{219,	'\xdb'},	// Latin Capital Letter U With Circumflex
	{220,	'\xdc'},	// Latin Capital Letter U With Diaeresis
	{221,	'\xdd'},	// Latin Capital Letter Y With Acute
	{223,	'\xdf'},	// Latin Small Letter Sharp S
	{224,	'\xe0'},	// Latin Small Letter A With Grave
	{225,	'\xe1'},	// Latin Small Letter A With Acute
	{226,	'\xe2'},	// Latin Small Letter A With Circumflex
	{227,	'\xe3'},	// Latin Small Letter A With Tilde
	{228,	'\xe4'},	// Latin Small Letter A With Diaeresis
	{229,	'\xe5'},	// Latin Small Letter A With Ring Above
	{230,	'\xe6'},	// Latin Small Letter AE
	{231,	'\xe7'},	// Latin Small Letter C With Cedilla
	{232,	'\xe8'},	// Latin Small Letter E With Grave
	{233,	'\xe9'},	// Latin Small Letter E With Acute
	{234,	'\xea'},	// Latin Small Letter E With Circumflex
	{235,	'\xeb'},	// Latin Small Letter E With Diaeresis
	{236,	'\xec'},	// Latin Small Letter I With Grave
	{237,	'\xed'},	// Latin Small Letter I With Acute
	{238,	'\xee'},	// Latin Small Letter I With Circumflex
	{239,	'\xef'},	// Latin Small Letter I With Diaeresis
	{241,	'\xf1'},	// Latin Small Letter N With Tilde
	{242,	'\xf2'},	// Latin Small Letter O With Grave
	{243,	'\xf3'},	// Latin Small Letter O With Acute
	{244,	'\xf4'},	// Latin Small Letter O With Circumflex
	{245,	'\xf5'},	// Latin Small Letter O With Tilde
	{246,	'\xf6'},	// Latin Small Letter O With Diaeresis
	{247,	'\xf7'},	// Division Sign
	{248,	'\xf8'},	// Latin Small Letter O With Stroke
	{249,	'\xf9'},	// Latin Small Letter U With Grave
	{250,	'\xfa'},	// Latin Small Letter U With Acute
	{251,	'\xfb'},	// Latin Small Letter U With Circumflex
	{252,	'\xfc'},	// Latin Small Letter U With Diaeresis
	{253,	'\xfd'},	// Latin Small Letter Y With Acute
	{255,	'\xff'},	// Latin Small Letter Y With Diaeresis
	{338,	'\x9a'},	// Latin Capital Ligature OE
	{339,	'\x9b'},	// Latin Small Ligature OE
	{372,	'\xd0'},	// Latin Capital Letter W With Circumflex
	{373,	'\xf0'},	// Latin Small Letter W With Circumflex
	{374,	'\xde'},	// Latin Capital Letter Y With Circumflex
	{375,	'\xfe'},	// Latin Small Letter Y With Circumflex
	{376,	'\xaf'},	// Latin Capital Letter Y With Diaeresis
	{7808,	'\xa8'},	// Latin Capital Letter W With Grave
	{7809,	'\xb8'},	// Latin Small Letter W With Grave
	{7810,	'\xaa'},	// Latin Capital Letter W With Acute
	{7811,	'\xba'},	// Latin Small Letter W With Acute
	{7812,	'\xbd'},	// Latin Capital Letter W With Diaeresis
	{7813,	'\xbe'},	// Latin Small Letter W With Diaeresis
	{7922,	'\xac'},	// Latin Capital Letter Y With Grave
	{7923,	'\xbc'},	// Latin Small Letter Y With Grave
	{8211,	'\x97'},	// En Dash
	{8212,	'\x98'},	// Em Dash
	{8216,	'\x90'},	// Left Single Quotation Mark
	{8217,	'\x91'},	// Right Single Quotation Mark
	{8220,	'\x94'},	// Left Double Quotation Mark
	{8221,	'\x95'},	// Right Double Quotation Mark
	{8222,	'\x96'},	// Double Low-9 Quotation Mark
	{8224,	'\x9c'},	// Dagger
	{8225,	'\x9d'},	// Double Dagger
	{8226,	'\x8f'},	// Bullet
	{8230,	'\x8c'},	// Horizontal Ellipsis
	{8240,	'\x8e'},	// Per Mille Sign
	{8249,	'\x92'},	// Single Left-Pointing Angle Quotation Mark
	{8250,	'\x93'},	// Single Right-Pointing Angle Quotation Mark
	{8364,	'\x80'},	// Euro Sign
	{8482,	'\x8d'},	// Trade Mark Sign
	{8722,	'\x99'},	// Minus Sign
	{64257,	'\x9e'},	// Latin Small Ligature Fi
	{64258,	'\x9f'},	// Latin Small Ligature Fl
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Greek
 */

static struct encoding_map encoding_acorn_greek[] = {
	{160,	'\xa0'},	// No-Break Space
	{163,	'\xa3'},	// Pound Sign
	{166,	'\xa6'},	// Broken Bar
	{167,	'\xa7'},	// Section Sign
	{168,	'\xa8'},	// Diaeresis
	{169,	'\xa9'},	// Copyright Sign
	{171,	'\xab'},	// Left-Pointing Double Angle Quotation Mark
	{172,	'\xac'},	// Not Sign
	{173,	'\xad'},	// Soft Hyphen
	{176,	'\xb0'},	// Degree Sign
	{177,	'\xb1'},	// Plus-Minus Sign
	{178,	'\xb2'},	// Superscript Two
	{179,	'\xb3'},	// Superscript Three
	{187,	'\xbb'},	// Right-Pointing Double Angle Quotation Mark
	{189,	'\xbd'},	// Vulgar Fraction One Half
	{890,	'\xaa'},	// Greek Ypogegrammeni
	{894,	'\xae'},	// Greek Question Mark
	{900,	'\xb4'},	// Greek Tonos
	{901,	'\xb5'},	// Greek Dialytika Tonos
	{902,	'\xb6'},	// Greek Capital Letter Alpha With Tonos
	{903,	'\xb7'},	// Greek Ano Teleia
	{904,	'\xb8'},	// Greek Capital Letter Epsilon With Tonos
	{905,	'\xb9'},	// Greek Capital Letter Eta With Tonos
	{906,	'\xba'},	// Greek Capital Letter Iota With Tonos
	{908,	'\xbc'},	// Greek Capital Letter Omicron With Tonos
	{910,	'\xbe'},	// Greek Capital Letter Upsilon With Tonos
	{911,	'\xbf'},	// Greek Capital Letter Omega With Tonos
	{912,	'\xc0'},	// Greek Small Letter Iota With Dialytika And Tonos
	{913,	'\xc1'},	// Greek Capital Letter Alpha
	{914,	'\xc2'},	// Greek Capital Letter Beta
	{915,	'\xc3'},	// Greek Capital Letter Gamma
	{916,	'\xc4'},	// Greek Capital Letter Delta
	{917,	'\xc5'},	// Greek Capital Letter Epsilon
	{918,	'\xc6'},	// Greek Capital Letter Zeta
	{919,	'\xc7'},	// Greek Capital Letter Eta
	{920,	'\xc8'},	// Greek Capital Letter Theta
	{921,	'\xc9'},	// Greek Capital Letter Iota
	{922,	'\xca'},	// Greek Capital Letter Kappa
	{923,	'\xcb'},	// Greek Capital Letter Lamda
	{924,	'\xcc'},	// Greek Capital Letter Mu
	{925,	'\xcd'},	// Greek Capital Letter Nu
	{926,	'\xce'},	// Greek Capital Letter Xi
	{927,	'\xcf'},	// Greek Capital Letter Omicron
	{928,	'\xd0'},	// Greek Capital Letter Pi
	{929,	'\xd1'},	// Greek Capital Letter Rho
	{931,	'\xd3'},	// Greek Capital Letter Sigma
	{932,	'\xd4'},	// Greek Capital Letter Tau
	{933,	'\xd5'},	// Greek Capital Letter Upsilon
	{934,	'\xd6'},	// Greek Capital Letter Phi
	{935,	'\xd7'},	// Greek Capital Letter Chi
	{936,	'\xd8'},	// Greek Capital Letter Psi
	{937,	'\xd9'},	// Greek Capital Letter Omega
	{938,	'\xda'},	// Greek Capital Letter Iota With Dialytika
	{939,	'\xdb'},	// Greek Capital Letter Upsilon With Dialytika
	{940,	'\xdc'},	// Greek Small Letter Alpha With Tonos
	{941,	'\xdd'},	// Greek Small Letter Epsilon With Tonos
	{942,	'\xde'},	// Greek Small Letter Eta With Tonos
	{943,	'\xdf'},	// Greek Small Letter Iota With Tonos
	{944,	'\xe0'},	// Greek Small Letter Upsilon With Dialytika And Tonos
	{945,	'\xe1'},	// Greek Small Letter Alpha
	{946,	'\xe2'},	// Greek Small Letter Beta
	{947,	'\xe3'},	// Greek Small Letter Gamma
	{948,	'\xe4'},	// Greek Small Letter Delta
	{949,	'\xe5'},	// Greek Small Letter Epsilon
	{950,	'\xe6'},	// Greek Small Letter Zeta
	{951,	'\xe7'},	// Greek Small Letter Eta
	{952,	'\xe8'},	// Greek Small Letter Theta
	{953,	'\xe9'},	// Greek Small Letter Iota
	{954,	'\xea'},	// Greek Small Letter Kappa
	{955,	'\xeb'},	// Greek Small Letter Lamda
	{956,	'\xec'},	// Greek Small Letter Mu
	{957,	'\xed'},	// Greek Small Letter Nu
	{958,	'\xee'},	// Greek Small Letter Xi
	{959,	'\xef'},	// Greek Small Letter Omicron
	{960,	'\xf0'},	// Greek Small Letter Pi
	{961,	'\xf1'},	// Greek Small Letter Rho
	{962,	'\xf2'},	// Greek Small Letter Final Sigma
	{963,	'\xf3'},	// Greek Small Letter Sigma
	{964,	'\xf4'},	// Greek Small Letter Tau
	{965,	'\xf5'},	// Greek Small Letter Upsilon
	{966,	'\xf6'},	// Greek Small Letter Phi
	{967,	'\xf7'},	// Greek Small Letter Chi
	{968,	'\xf8'},	// Greek Small Letter Psi
	{969,	'\xf9'},	// Greek Small Letter Omega
	{970,	'\xfa'},	// Greek Small Letter Iota With Dialytika
	{971,	'\xfb'},	// Greek Small Letter Upsilon With Dialytika
	{972,	'\xfc'},	// Greek Small Letter Omicron With Tonos
	{973,	'\xfd'},	// Greek Small Letter Upsilon With Tonos
	{974,	'\xfe'},	// Greek Small Letter Omega With Tonos
	{8213,	'\xaf'},	// Horizontal Bar
	{8216,	'\xa1'},	// Left Single Quotation Mark
	{8217,	'\xa2'},	// Right Single Quotation Mark
	{8364,	'\xa4'},	// Euro Sign
	{8367,	'\xa5'},	// Drachma Sign
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Hebrew
 */

static struct encoding_map encoding_acorn_hebrew[] = {
	{160,	'\xa0'},	// No-Break Space
	{162,	'\xa2'},	// Cent Sign
	{163,	'\xa3'},	// Pound Sign
	{164,	'\xa4'},	// Currency Sign
	{165,	'\xa5'},	// Yen Sign
	{166,	'\xa6'},	// Broken Bar
	{167,	'\xa7'},	// Section Sign
	{168,	'\xa8'},	// Diaeresis
	{169,	'\xa9'},	// Copyright Sign
	{171,	'\xab'},	// Left-Pointing Double Angle Quotation Mark
	{172,	'\xac'},	// Not Sign
	{173,	'\xad'},	// Soft Hyphen
	{174,	'\xae'},	// Registered Sign
	{176,	'\xb0'},	// Degree Sign
	{177,	'\xb1'},	// Plus-Minus Sign
	{178,	'\xb2'},	// Superscript Two
	{179,	'\xb3'},	// Superscript Three
	{180,	'\xb4'},	// Acute Accent
	{181,	'\xb5'},	// Micro Sign
	{182,	'\xb6'},	// Pilcrow Sign
	{183,	'\xb7'},	// Middle Dot
	{184,	'\xb8'},	// Cedilla
	{185,	'\xb9'},	// Superscript One
	{187,	'\xbb'},	// Right-Pointing Double Angle Quotation Mark
	{188,	'\xbc'},	// Vulgar Fraction One Quarter
	{189,	'\xbd'},	// Vulgar Fraction One Half
	{190,	'\xbe'},	// Vulgar Fraction Three Quarters
	{215,	'\xaa'},	// Multiplication Sign
	{247,	'\xba'},	// Division Sign
	{1488,	'\xe0'},	// Hebrew Letter Alef
	{1489,	'\xe1'},	// Hebrew Letter Bet
	{1490,	'\xe2'},	// Hebrew Letter Gimel
	{1491,	'\xe3'},	// Hebrew Letter Dalet
	{1492,	'\xe4'},	// Hebrew Letter He
	{1493,	'\xe5'},	// Hebrew Letter Vav
	{1494,	'\xe6'},	// Hebrew Letter Zayin
	{1495,	'\xe7'},	// Hebrew Letter Het
	{1496,	'\xe8'},	// Hebrew Letter Tet
	{1497,	'\xe9'},	// Hebrew Letter Yod
	{1498,	'\xea'},	// Hebrew Letter Final Kaf
	{1499,	'\xeb'},	// Hebrew Letter Kaf
	{1500,	'\xec'},	// Hebrew Letter Lamed
	{1501,	'\xed'},	// Hebrew Letter Final Mem
	{1502,	'\xee'},	// Hebrew Letter Mem
	{1503,	'\xef'},	// Hebrew Letter Final Nun
	{1504,	'\xf0'},	// Hebrew Letter Nun
	{1505,	'\xf1'},	// Hebrew Letter Samekh
	{1506,	'\xf2'},	// Hebrew Letter Ayin
	{1507,	'\xf3'},	// Hebrew Letter Final Pe
	{1508,	'\xf4'},	// Hebrew Letter Pe
	{1509,	'\xf5'},	// Hebrew Letter Final Tsadi
	{1510,	'\xf6'},	// Hebrew Letter Tsadi
	{1511,	'\xf7'},	// Hebrew Letter Qof
	{1512,	'\xf8'},	// Hebrew Letter Resh
	{1513,	'\xf9'},	// Hebrew Letter Shin
	{1514,	'\xfa'},	// Hebrew Letter Tav
	{8206,	'\xfd'},	// Left-To-Right Mark
	{8207,	'\xfe'},	// Right-To-Left Mark
	{8215,	'\xdf'},	// Double Low Line
	{8254,	'\xaf'},	// Overline
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS Cyrillic 2
 */

static struct encoding_map encoding_acorn_cyrillic2[] = {
	{1040,	'\x80'},	// Cyrillic Capital Letter A
	{1041,	'\x81'},	// Cyrillic Capital Letter Be
	{1042,	'\x82'},	// Cyrillic Capital Letter Ve
	{1043,	'\x83'},	// Cyrillic Capital Letter Ghe
	{1044,	'\x84'},	// Cyrillic Capital Letter De
	{1045,	'\x85'},	// Cyrillic Capital Letter Ie
	{1046,	'\x86'},	// Cyrillic Capital Letter Zhe
	{1047,	'\x87'},	// Cyrillic Capital Letter Ze
	{1048,	'\x88'},	// Cyrillic Capital Letter I
	{1049,	'\x89'},	// Cyrillic Capital Letter Short I
	{1050,	'\x8a'},	// Cyrillic Capital Letter Ka
	{1051,	'\x8b'},	// Cyrillic Capital Letter El
	{1052,	'\x8c'},	// Cyrillic Capital Letter Em
	{1053,	'\x8d'},	// Cyrillic Capital Letter En
	{1054,	'\x8e'},	// Cyrillic Capital Letter O
	{1055,	'\x8f'},	// Cyrillic Capital Letter Pe
	{1056,	'\x90'},	// Cyrillic Capital Letter Er
	{1057,	'\x91'},	// Cyrillic Capital Letter Es
	{1058,	'\x92'},	// Cyrillic Capital Letter Te
	{1059,	'\x93'},	// Cyrillic Capital Letter U
	{1060,	'\x94'},	// Cyrillic Capital Letter Ef
	{1061,	'\x95'},	// Cyrillic Capital Letter Ha
	{1062,	'\x96'},	// Cyrillic Capital Letter Tse
	{1063,	'\x97'},	// Cyrillic Capital Letter Che
	{1064,	'\x98'},	// Cyrillic Capital Letter Sha
	{1065,	'\x99'},	// Cyrillic Capital Letter Shcha
	{1066,	'\x9a'},	// Cyrillic Capital Letter Hard Sign
	{1067,	'\x9b'},	// Cyrillic Capital Letter Yeru
	{1068,	'\x9c'},	// Cyrillic Capital Letter Soft Sign
	{1069,	'\x9d'},	// Cyrillic Capital Letter E
	{1070,	'\x9e'},	// Cyrillic Capital Letter Yu
	{1071,	'\x9f'},	// Cyrillic Capital Letter Ya
	{1072,	'\xa0'},	// Cyrillic Small Letter A
	{1073,	'\xa1'},	// Cyrillic Small Letter Be
	{1074,	'\xa2'},	// Cyrillic Small Letter Ve
	{1075,	'\xa3'},	// Cyrillic Small Letter Ghe
	{1076,	'\xa4'},	// Cyrillic Small Letter De
	{1077,	'\xa5'},	// Cyrillic Small Letter Ie
	{1078,	'\xa6'},	// Cyrillic Small Letter Zhe
	{1079,	'\xa7'},	// Cyrillic Small Letter Ze
	{1080,	'\xa8'},	// Cyrillic Small Letter I
	{1081,	'\xa9'},	// Cyrillic Small Letter Short I
	{1082,	'\xaa'},	// Cyrillic Small Letter Ka
	{1083,	'\xab'},	// Cyrillic Small Letter El
	{1084,	'\xac'},	// Cyrillic Small Letter Em
	{1085,	'\xad'},	// Cyrillic Small Letter En
	{1086,	'\xae'},	// Cyrillic Small Letter O
	{1087,	'\xaf'},	// Cyrillic Small Letter Pe
	{1088,	'\xe0'},	// Cyrillic Small Letter Er
	{1089,	'\xe1'},	// Cyrillic Small Letter Es
	{1090,	'\xe2'},	// Cyrillic Small Letter Te
	{1091,	'\xe3'},	// Cyrillic Small Letter U
	{1092,	'\xe4'},	// Cyrillic Small Letter Ef
	{1093,	'\xe5'},	// Cyrillic Small Letter Ha
	{1094,	'\xe6'},	// Cyrillic Small Letter Tse
	{1095,	'\xe7'},	// Cyrillic Small Letter Che
	{1096,	'\xe8'},	// Cyrillic Small Letter Sha
	{1097,	'\xe9'},	// Cyrillic Small Letter Shcha
	{1098,	'\xea'},	// Cyrillic Small Letter Hard Sign
	{1099,	'\xeb'},	// Cyrillic Small Letter Yeru
	{1100,	'\xec'},	// Cyrillic Small Letter Soft Sign
	{1101,	'\xed'},	// Cyrillic Small Letter E
	{1102,	'\xee'},	// Cyrillic Small Letter Yu
	{1103,	'\xef'},	// Cyrillic Small Letter Ya
	{0,	'\0'}		// End of Table
};

/**
 * UTF8 to RISC OS BFont
 */

static struct encoding_map encoding_acorn_bfont[] = {
	{96,	'\xbb'},	// Grave Accent
	{124,	'\xdc'},	// Vertical Line
	{156,	'\xb8'},	// <Control>
	{157,	'\xb9'},	// <Control>
	{161,	'\xb5'},	// Inverted Exclamation Mark
	{164,	'\x9e'},	// Currency Sign
	{167,	'\x9f'},	// Section Sign
	{169,	'\x87'},	// Copyright Sign
	{176,	'\xa0'},	// Degree Sign
	{177,	'\xda'},	// Plus-Minus Sign
	{182,	'\xbc'},	// Pilcrow Sign
	{183,	'\xbd'},	// Middle Dot
	{191,	'\xb4'},	// Inverted Question Mark
	{196,	'\x80'},	// Latin Capital Letter A With Diaeresis
	{197,	'\x81'},	// Latin Capital Letter A With Ring Above
	{198,	'\x82'},	// Latin Capital Letter Ae
	{209,	'\xb6'},	// Latin Capital Letter N With Tilde
	{214,	'\x85'},	// Latin Capital Letter O With Diaeresis
	{216,	'\xc0'},	// Latin Capital Letter O With Stroke
	{220,	'\x86'},	// Latin Capital Letter U With Diaeresis
	{224,	'\x8c'},	// Latin Small Letter A With Grave
	{228,	'\x91'},	// Latin Small Letter A With Diaeresis
	{229,	'\x92'},	// Latin Small Letter A With Ring Above
	{230,	'\x93'},	// Latin Small Letter Ae
	{231,	'\x94'},	// Latin Small Letter C With Cedilla
	{232,	'\x8d'},	// Latin Small Letter E With Grave
	{233,	'\x95'},	// Latin Small Letter E With Acute
	{234,	'\x8f'},	// Latin Small Letter E With Circumflex
	{234,	'\x90'},	// Latin Small Letter E With Circumflex
	{235,	'\x8e'},	// Latin Small Letter E With Diaeresis
	{236,	'\x98'},	// Latin Small Letter I With Grave
	{238,	'\x99'},	// Latin Small Letter I With Circumflex
	{241,	'\xb7'},	// Latin Small Letter N With Tilde
	{242,	'\x9a'},	// Latin Small Letter O With Grave
	{246,	'\x96'},	// Latin Small Letter O With Diaeresis
	{248,	'\xe0'},	// Latin Small Letter O With Stroke
	{249,	'\x9b'},	// Latin Small Letter U With Grave
	{251,	'\x9c'},	// Latin Small Letter U With Circumflex
	{252,	'\x97'},	// Latin Small Letter U With Diaeresis
	{255,	'\x9d'},	// Latin Small Letter Y With Diaeresis
	{913,	'\xc1'},	// Greek Capital Letter Alpha
	{914,	'\xc2'},	// Greek Capital Letter Beta
	{915,	'\xc3'},	// Greek Capital Letter Gamma
	{916,	'\xc4'},	// Greek Capital Letter Delta
	{917,	'\xc5'},	// Greek Capital Letter Epsilon
	{918,	'\xc6'},	// Greek Capital Letter Zeta
	{919,	'\xc7'},	// Greek Capital Letter Eta
	{920,	'\xc8'},	// Greek Capital Letter Theta
	{921,	'\xc9'},	// Greek Capital Letter Iota
	{922,	'\xca'},	// Greek Capital Letter Kappa
	{923,	'\xcb'},	// Greek Capital Letter Lamda
	{924,	'\xcc'},	// Greek Capital Letter Mu
	{925,	'\xcd'},	// Greek Capital Letter Nu
	{926,	'\xce'},	// Greek Capital Letter Xi
	{927,	'\xcf'},	// Greek Capital Letter Omicron
	{928,	'\xd0'},	// Greek Capital Letter Pi
	{929,	'\xd1'},	// Greek Capital Letter Rho
	{931,	'\xd2'},	// Greek Capital Letter Sigma
	{932,	'\xd3'},	// Greek Capital Letter Tau
	{933,	'\xd4'},	// Greek Capital Letter Upsilon
	{934,	'\xd5'},	// Greek Capital Letter Phi
	{935,	'\xd6'},	// Greek Capital Letter Chi
	{936,	'\xd7'},	// Greek Capital Letter Psi
	{937,	'\xd8'},	// Greek Capital Letter Omega
	{945,	'\xe1'},	// Greek Small Letter Alpha
	{946,	'\xe2'},	// Greek Small Letter Beta
	{947,	'\xe3'},	// Greek Small Letter Gamma
	{948,	'\xe4'},	// Greek Small Letter Delta
	{949,	'\xe5'},	// Greek Small Letter Epsilon
	{950,	'\xe6'},	// Greek Small Letter Zeta
	{951,	'\xe7'},	// Greek Small Letter Eta
	{952,	'\xe8'},	// Greek Small Letter Theta
	{953,	'\xe9'},	// Greek Small Letter Iota
	{954,	'\xea'},	// Greek Small Letter Kappa
	{955,	'\xeb'},	// Greek Small Letter Lamda
	{956,	'\xec'},	// Greek Small Letter Mu
	{957,	'\xed'},	// Greek Small Letter Nu
	{958,	'\xee'},	// Greek Small Letter Xi
	{959,	'\xef'},	// Greek Small Letter Omicron
	{960,	'\xf0'},	// Greek Small Letter Pi
	{961,	'\xf1'},	// Greek Small Letter Rho
	{963,	'\xf2'},	// Greek Small Letter Sigma
	{964,	'\xf3'},	// Greek Small Letter Tau
	{965,	'\xf4'},	// Greek Small Letter Upsilon
	{966,	'\xf5'},	// Greek Small Letter Phi
	{967,	'\xf6'},	// Greek Small Letter Chi
	{968,	'\xf7'},	// Greek Small Letter Psi
	{969,	'\xf8'},	// Greek Small Letter Omega
	{8214,	'\xdd'},	// Double Vertical Line
	{8706,	'\xf9'},	// Partial Differential
	{8711,	'\xd9'},	// Nabla
	{8723,	'\xdb'},	// Minus-Or-Plus Sign
	{8730,	'\xbe'},	// Square Root
	{8745,	'\xdf'},	// Intersection
	{8746,	'\xde'},	// Union
	{8771,	'\xfa'},	// Asymptotically Equal To
	{8800,	'\xfd'},	// Not Equal To
	{8801,	'\xfb'},	// Identical To
	{8804,	'\xfc'},	// Less-Than Or Equal To
	{8805,	'\xfe'},	// Greater-Than Or Equal To
	{0,	'\0'}		// End of Table
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
	{ENCODING_TARGET_UTF8,			NULL,				"UTF8",		"utf-8"},
	{ENCODING_TARGET_7BIT,			NULL,				"7Bit",		"utf-8"},
	{ENCODING_TARGET_ACORN_LATIN1,		encoding_acorn_latin1,		"AcornL1",	"iso-8859-1"},
	{ENCODING_TARGET_ACORN_LATIN2,		encoding_acorn_latin2,		"AcornL2",	"iso-8859-2"},
	{ENCODING_TARGET_ACORN_LATIN3,		encoding_acorn_latin3,		"AcornL3",	"iso-8859-3"},
	{ENCODING_TARGET_ACORN_LATIN4,		encoding_acorn_latin4,		"AcornL4",	"iso-8859-4"},
	{ENCODING_TARGET_ACORN_LATIN5,		encoding_acorn_latin5,		"AcornL5",	"iso-8859-9"},
	{ENCODING_TARGET_ACORN_LATIN6,		encoding_acorn_latin6,		"AcornL6",	"iso-8859-10"},
	{ENCODING_TARGET_ACORN_LATIN7,		encoding_acorn_latin7,		"AcornL7",	"iso-8859-13"},
	{ENCODING_TARGET_ACORN_LATIN8,		encoding_acorn_latin8,		"AcornL8",	"iso-8859-14"},
	{ENCODING_TARGET_ACORN_LATIN9,		encoding_acorn_latin9,		"AcornL9",	"iso-8859-15"},
	{ENCODING_TARGET_ACORN_LATIN10,		encoding_acorn_latin10,		"AcornL10",	"iso-8859-16"},
	{ENCODING_TARGET_ACORN_CYRILLIC,	encoding_acorn_cyrillic,	"Cyrillic",	"iso-8859-5"},
	{ENCODING_TARGET_ACORN_CYRILLIC2,	encoding_acorn_cyrillic2,	"Cyrillic2",	NULL},
	{ENCODING_TARGET_ACORN_GREEK,		encoding_acorn_greek,		"Greek",	"iso-8859-7"},
	{ENCODING_TARGET_ACORN_HEBREW,		encoding_acorn_hebrew,		"Hebrew",	"iso-8859-8"},
	{ENCODING_TARGET_ACORN_WELSH,		encoding_acorn_welsh,		"Welsh",	NULL},
	{ENCODING_TARGET_ACORN_BFONT,		encoding_acorn_bfont,		"BFont",	NULL}
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
		if (string_nocase_strcmp(name, (char *) encoding_list[i].name) == 0)
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
