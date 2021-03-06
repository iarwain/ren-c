//
//  File: %sys-string.h
//  Summary: {Definitions for REBSTR (e.g. WORD!) and REBUNI (e.g. STRING!)}
//  Project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  Homepage: https://github.com/metaeducation/ren-c/
//
//=////////////////////////////////////////////////////////////////////////=//
//
// Copyright 2012 REBOL Technologies
// Copyright 2012-2017 Rebol Open Source Contributors
// REBOL is a trademark of REBOL Technologies
//
// See README.md and CREDITS.md for more information
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//=////////////////////////////////////////////////////////////////////////=//
//
// !!! R3-Alpha and Red would work with strings in their decoded form, in
// series of varying widths.  Ren-C's goal is to replace this with the idea
// of "UTF-8 everywhere", working with the strings as UTF-8 and only
// converting if the platform requires it for I/O (e.g. Windows):
//
// http://utf8everywhere.org/
//
// As a first step toward this goal, one place where strings were kept in
// UTF-8 form has been converted into series...the word table.  So for now,
// all REBSTR instances are for ANY-WORD!.
//
//=////////////////////////////////////////////////////////////////////////=//
//
// The *current* implementation of Rebol's ANY-STRING! type has two different
// series widths that are used.  One is the BYTE_SIZED() series which encodes
// ASCII in the low bits, and Latin-1 extensions in the range 0x80 - 0xFF.
// So long as a codepoint can fit in this range, the string can be stored in
// single bytes:
//
// https://en.wikipedia.org/wiki/Latin-1_Supplement_(Unicode_block)
//
// (Note: This is not to be confused with the other "byte-width" encoding,
// which is UTF-8.  Rebol series routines are not set up to handle insertions
// or manipulations of UTF-8 encoded data in a Reb_Any_String payload at
// this time...it is a format used only in I/O.)
//
// The second format that is used puts codepoints into a 16-bit REBUNI-sized
// element.  If an insertion of a string or character into a byte sized
// string cannot be represented in 0xFF or lower, then the target string will
// be "widened"--doubling the storage space taken and requiring updating of
// the character data in memory.  At this time there are no "in-place"
// cases where a string is reduced from REBUNI to byte sized, but operations
// like Copy_String_Slimming() will scan a source string to see if a byte-size
// copy can be made from a REBUNI-sized one without loss of information.
//
// Byte-sized series are also used by the BINARY! datatype.  There is no
// technical difference between such series used as strings or used as binary,
// the difference comes from being marked REB_BINARY or REB_STRING in the
// header of the value carrying the series.
//
// For easier type-correctness, the series macros are given with names BIN_XXX
// and UNI_XXX.  There aren't distinct data types for the series themselves,
// just REBSER* is used.  Hence BIN_LEN() and UNI_LEN() aren't needed as you
// could just use SER_LEN(), but it helps a bit for readability...and an
// assert is included to ensure the size matches up.
//


//=////////////////////////////////////////////////////////////////////////=//
//
//  REBSTR series for UTF-8 strings
//
//=////////////////////////////////////////////////////////////////////////=//
//
// The concept is that a SYM refers to one of the built-in words and can
// be used in C switch statements.  A canon STR is used to identify
// everything else.
//

inline static const REBYTE *STR_HEAD(REBSTR *str) {
    return BIN_HEAD(str);
}

inline static REBSTR *STR_CANON(REBSTR *str) {
    if (GET_SER_INFO(str, STRING_INFO_CANON))
        return str;
    return str->misc.canon;
}

inline static OPT_REBSYM STR_SYMBOL(REBSTR *str) {
    REBUPT sym = RIGHT_16_BITS(str->header.bits);
    assert(RIGHT_16_BITS(STR_CANON(str)->header.bits) == sym);
    return cast(REBSYM, sym);
}

inline static REBCNT STR_NUM_BYTES(REBSTR *str) {
    return SER_LEN(str); // number of bytes in seris is series length, ATM
}

inline static REBSTR *Canon(REBSYM sym) {
    assert(cast(REBCNT, sym) != 0);
    assert(cast(REBCNT, sym) < SER_LEN(PG_Symbol_Canons));
    return *SER_AT(REBSTR*, PG_Symbol_Canons, cast(REBCNT, sym));
}

inline static REBOOL SAME_STR(REBSTR *s1, REBSTR *s2) {
    if (s1 == s2) return TRUE; // !!! does this check speed things up or not?
    return LOGICAL(STR_CANON(s1) == STR_CANON(s2)); // canon check, quite fast
}



//
// !!! UNI_XXX: Unicode string series macros !!! - Becoming Deprecated
//

inline static REBCNT UNI_LEN(REBSER *s) {
    assert(SER_WIDE(s) == sizeof(REBUNI));
    return SER_LEN(s);
}

inline static void SET_UNI_LEN(REBSER *s, REBCNT len) {
    assert(SER_WIDE(s) == sizeof(REBUNI));
    SET_SERIES_LEN(s, len);
}

#define UNI_AT(s,n) \
    SER_AT(REBUNI, (s), (n))

#define UNI_HEAD(s) \
    SER_HEAD(REBUNI, (s))

#define UNI_TAIL(s) \
    SER_TAIL(REBUNI, (s))

#define UNI_LAST(s) \
    SER_LAST(REBUNI, (s))

inline static void TERM_UNI(REBSER *s) {
    UNI_HEAD(s)[SER_LEN(s)] = 0;
}

inline static void TERM_UNI_LEN(REBSER *s, REBCNT len) {
    SET_SERIES_LEN(s, len);
    UNI_HEAD(s)[len] = 0;
}


//
// Get a char, from either byte or unicode string:
//

inline static REBUNI GET_ANY_CHAR(REBSER *s, REBCNT n) {
    return BYTE_SIZE(s) ? BIN_HEAD(s)[n] : UNI_HEAD(s)[n];
}

inline static void SET_ANY_CHAR(REBSER *s, REBCNT n, REBYTE c) {
    if (BYTE_SIZE(s))
        BIN_HEAD(s)[n] = c;
    else
        UNI_HEAD(s)[n] = c;
}



//=////////////////////////////////////////////////////////////////////////=//
//
//  ANY-STRING! (uses `struct Reb_Any_Series`)
//
//=////////////////////////////////////////////////////////////////////////=//

#define Init_String(v,s) \
    Init_Any_Series((v), REB_STRING, (s))

#define Init_File(v,s) \
    Init_Any_Series((v), REB_FILE, (s))

#define Init_Email(v,s) \
    Init_Any_Series((v), REB_EMAIL, (s))

#define Init_Tag(v,s) \
    Init_Any_Series((v), REB_TAG, (s))

#define Init_Url(v,s) \
    Init_Any_Series((v), REB_URL, (s))

#define VAL_UNI(v) \
    UNI_HEAD(VAL_SERIES(v))

#define VAL_UNI_HEAD(v) \
    UNI_HEAD(VAL_SERIES(v))

#define VAL_UNI_AT(v) \
    UNI_AT(VAL_SERIES(v), VAL_INDEX(v))

#define VAL_ANY_CHAR(v) \
    GET_ANY_CHAR(VAL_SERIES(v), VAL_INDEX(v))


// Basic string initialization from UTF8.
//
inline static REBSER *Make_UTF8_May_Fail(const char *utf8)
{
    return Append_UTF8_May_Fail(
        NULL,
        cast(const REBYTE*, utf8),
        LEN_BYTES(cast(const REBYTE*, utf8))
    );
}


// Basic string initialization from UTF16.  Note: This just assumes two byte
// characters with no decoding at the moment.
//
inline static REBSER *Make_UTF16_May_Fail(REBUNI *utf16)
{
    REBCNT len = Strlen_Uni(utf16);
    REBSER *s = Make_Unicode(len);
    Append_Uni_Uni(s, utf16, len);
    return s;
}