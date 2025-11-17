/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2022 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_fits_card.h"
#include "cpl_propertylist_impl.h"
#include "cpl_memory.h"

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_fits_card   FITS card related basic routines
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_fits_card.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                             Macro definitions
 -----------------------------------------------------------------------------*/

/* Space permitting non-string values are right justified to column 30,
   per ESO-044156 (DICD), Ver. 6, p. 20/73. */
#define CPL_ALIGN_POS 30

/* The FITS card value indicator is initialized via (a part of) this string,
   which has padding sufficient for the right justification, see CPL_ALIGN_POS.
   Its length is such that when the equal sign is on position 9 it extends to
   one character short of the alignment position.
   It can be used in four different ways, with or without its initial blank
   and with or without padding for right column alignment. */
#define VALINDSTR (const char *)" =                    "

#define BLANK80                                \
    "                                        " \
    "                                        "

/* The actual FITS card length does not include the null-byte */
#define FITS_CARD_LEN (FLEN_CARD - 1)

/*-----------------------------------------------------------------------------
                        Private type definitions
 -----------------------------------------------------------------------------*/

typedef struct cpl_fits_value
{
    union
    {
        long long i;      /* Numerical, integer */
        double f;         /* Numerical, double */
        double complex x; /* Numerical, complex */
        char l;           /* Boolean: 1 for true ('T') or 0 for false ('F') */
        const char *c;    /* String, any escaped quotes ('') are decoded   */
    } val;
    int nmemb;  /* For string data: number of bytes in buffer
                  For no-value, undefined value and error: 0
                  For one of the other values: 1
               */
    char tcode; /* Type code: FITS code: 'C', 'L', 'F', 'I', 'X', or
                   'U' (undefined), 'N' (none) or 0 (unparsable card) */
    /* String buffer for decoded string value with at least one internal quote
       - can at most use 67 chars */
    char unquote[FLEN_VALUE];
} cpl_fits_value;

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static const char *
cpl_fits_set_value_(char *, int *, int, const cpl_property *) CPL_ATTR_NONNULL;

static const char *
cpl_fits_get_comment(const char *, int, int *) CPL_ATTR_NONNULL;

static int
cpl_property_is_float(const cpl_cstr *) CPL_ATTR_NONNULL CPL_ATTR_PURE;

static char
cpl_fits_get_value(cpl_fits_value *, const char *, int, const cpl_cstr *, int *)
    CPL_ATTR_NONNULL;

static int cpl_fits_get_number(const char *, int, long long *, double *, int *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1, 4, 5)))
#endif
    ;

static const char *
cpl_fits_get_key(const char *, int *, int *) CPL_ATTR_NONNULL;

static void cpl_fits_txtcpy(char *, const char *, int) CPL_ATTR_NONNULL;

static char
cpl_fits_check_key(const char *, int) CPL_ATTR_NONNULL CPL_ATTR_PURE;

static int cpl_fits_set_key(char *,
                            int *,
                            const cpl_cstr *,
                            cpl_property_sorttype) CPL_ATTR_NONNULL;

static cpl_error_code
cpl_fits_set_value(char *, int *, const cpl_property *) CPL_ATTR_NONNULL;

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/**
 * @internal
 * @brief
 *   Check whether the substring matches the provided names
 *
 * @param self      The substring (of e.g. FITS card) to compare
 * @param nstart    Number of start keys to select against
 * @param startkey  Keys starting with this name are matched
 * @return TRUE iff the substring matches
 * @see cpl_property_check_regexp, cpl_cstr_key_memcmp
 *
 */
cxbool
cpl_fits_card_check_memcmp(const cpl_cstr *self,
                           cpl_size nstart,
                           const cpl_cstr *startkey[])
{
    const cpl_memcmp filter = { nstart, startkey, 0, NULL, CPL_FALSE };

    return cpl_cstr_check_memcmp(self, (cxcptr)&filter);
}

/*----------------------------------------------------------------------------*/
/*
 * @internal
 * @brief Convert card to a property
 * @param self    The propertylist to append to
 * @param cardi   A NULL-terminated string with a FITS card, 81 bytes
 * @param names   An optional list of names w. invert flag for filtering cards
 * @param regexp  An optional regexp w. invert flag for filtering cards
 * @return CPL_ERROR_NONE, or the relevant CPL error on failure
 * @see _cpl_propertylist_fill_from_fits()
 * @note While a newly created property has a NULL-comment, a property
 *       appended here has a non-NULL (but possibly zero-length) comment
 *
 */
/*----------------------------------------------------------------------------*/
cpl_error_code
cpl_propertylist_append_from_string(cpl_propertylist *self,
                                    const char *cardi,
                                    const cpl_memcmp *names,
                                    const cpl_regexp *regexp)
{
    char keystr[FLEN_KEYWORD];
    cpl_property *myprop;
    const char *commentmem = NULL;
    cpl_boolean get_comment = CPL_TRUE;
    cpl_fits_value parseval;
    int compos = 0;
    int comlen;
    char type;

    int keylen = 0;   /* Length excl. terminating null byte */
    int valinlen = 0; /* Length to value indicator */
    const char *keymem = cpl_fits_get_key(cardi, &keylen, &valinlen);
    const cpl_cstr *keywlen = CXSTR(keymem, keylen);

    if (keylen + 1 >= FLEN_KEYWORD) {
        return cpl_error_set_message_(CPL_ERROR_BAD_FILE_FORMAT,
                                      "FITS Card has bad key (len=%d)", keylen);
    }

    if (names != NULL) {
        if (cpl_cstr_check_memcmp(keywlen, names) == FALSE) {
            /* Card is filtered out */
            return CPL_ERROR_NONE;
        }
    }
    else if (regexp != NULL) {
        /* The regexp parser requires a null-terminated key */
        (void)memcpy(keystr, keymem, keylen);
        keystr[keylen] = '\0';

        if (cpl_cstr_check_regexp(CXSTR(keystr, keylen), regexp) == FALSE) {
            /* Card is filtered out */
            return CPL_ERROR_NONE;
        }
    }

    type = cpl_fits_get_value(&parseval, cardi, valinlen, keywlen, &compos);

    /*
     * Create the property from the parsed FITS card.
     */

    switch (type) {
        case 'L':
            myprop = cpl_property_new_cx(keywlen, CPL_TYPE_BOOL);
            cpl_property_set_bool(myprop, parseval.val.l);

            break;

        case 'I':

            /* Certain (WCS) keywords must be floating point, even if
           their FITS encoding is a valid integer. */

            if (cpl_property_is_float(keywlen)) {
                /* The different types share a union, so use temp var */
                const long long ival = parseval.val.i;

                parseval.val.f = (double)ival;
            }
            else if ((long long)((int)parseval.val.i) == parseval.val.i) {
                /* Using an 'int' since the integer property fits */

                myprop = cpl_property_new_cx(keywlen, CPL_TYPE_INT);
                cpl_property_set_int(myprop, (int)parseval.val.i);
                break;
            }
            else {
                myprop = cpl_property_new_cx(keywlen, CPL_TYPE_LONG_LONG);
                cpl_property_set_long_long(myprop, parseval.val.i);
                break;
            }

            CPL_ATTR_FALLTRHU; /* fall through */
        case 'F':

            myprop = cpl_property_new_cx(keywlen, CPL_TYPE_DOUBLE);
            cpl_property_set_double(myprop, parseval.val.f);

            break;

        case 'U': /* Undefined value fall-through to no-value */
        case 'N':

            /* Strip any trailing blanks */
            for (comlen = FITS_CARD_LEN - compos; comlen > 0; comlen--) {
                if (cardi[compos + comlen - 1] != ' ')
                    break;
            }

            /* Skip totally empty records */
            if (keylen == 0 && comlen == 0)
                return CPL_ERROR_NONE;

            /*
         * FITS standard: blank keywords may be followed by
         * any ASCII text as it is for COMMENT and HISTORY.
         *
         * In order to preserve this header record it is
         * changed into COMMENT record, so that it can be
         * stored in the property list.
         */

            /* For a value-less card, a string value is made from the comment
           which becomes empty */

            parseval.val.c = cardi + compos;
            parseval.nmemb = comlen;
            comlen = 0;

            get_comment = CPL_FALSE;

            CPL_ATTR_FALLTRHU; /* fall through */
        case 'C':

            /* For the above fall through, a blank key becomes a comment key */
            myprop =
                cpl_property_new_cx(keylen == 0 ? CXSTR("COMMENT", 7) : keywlen,
                                    CPL_TYPE_STRING);

            cpl_property_set_string_cx(myprop,
                                       CXSTR(parseval.nmemb > 0 ? parseval.val.c
                                                                : "",
                                             parseval.nmemb));

            break;

        case 'X':

            myprop = cpl_property_new_cx(keywlen, CPL_TYPE_DOUBLE_COMPLEX);
            cpl_property_set_double_complex(myprop, parseval.val.x);

            break;

        default: {
            /* A card with an invalid value will go here */
            const char badchar[2] = { cardi[compos], '\0' };

            (void)memcpy(keystr, keymem, keylen);
            keystr[keylen] = '\0';
            return cpl_error_set_message_(
                CPL_ERROR_BAD_FILE_FORMAT,
                "Bad value in FITS card with key='%*s', bytepos(=): %d, "
                "bad-byte=0x%02x (\"%s\") at pos: %d",
                keylen, keystr, valinlen, (int)*badchar, badchar, compos);
        }
    }

    if (get_comment) {
        commentmem = cpl_fits_get_comment(cardi, compos, &comlen);
    }

    /* While for the cpl_property a NULL comment is the default,
       here an empty comment is set as such */
    cpl_property_set_comment_cx(myprop,
                                CXSTR(comlen > 0 ? commentmem : "", comlen));

    (void)cpl_propertylist_set_property(self, myprop);

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Fill a FITS card
  @param card     A buffer of 80 (or more) characters
  @param self    The property to fill from
  @return CPL_ERROR_NONE, or the relevant CPL error on failure
  @note (Almost) no input validation in this internal function
  @see fits_make_key(), ffmkky(), fftkey()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code
cpl_fits_fill_card(char *card, const cpl_property *self)
{
    const cpl_type type = cpl_property_get_type_(self);
    const cxchar *name = cpl_property_get_name_(self);
    const cxsize namelen = cpl_property_get_size_name(self);
    const cpl_cstr *name_ = CXSTR(name, namelen);
    const cpl_size commlen = (cpl_size)cpl_property_get_size_comment(self);
    int wlen = 0;
    const cpl_property_sorttype ksort = cpl_property_get_sortkey_(self);
    const int keytype = cpl_fits_set_key(card, &wlen, name_, ksort);
    /* Whether to write the value indicator */
    const cpl_boolean dovalin =
        (keytype == 0 || type != CPL_TYPE_STRING) ? CPL_TRUE : CPL_FALSE;

    if (CPL_UNLIKELY(keytype < 0))
        return cpl_error_set_where_();

    if (!dovalin) {
        /* Commentary card (COMMENT/HISTORY/blank key) */
        const int mysz =
            CX_MIN(FITS_CARD_LEN - 8,
                   CX_MAX(0, (int)cpl_property_get_size_(self) - 1));
        const char *strcpy = mysz == 0 ? "" : cpl_property_get_string_(self);

        cpl_fits_txtcpy(card + wlen, strcpy, mysz);
        wlen += mysz;
    }
    else if (CPL_UNLIKELY(cpl_fits_set_value(card, &wlen, self)))
        return cpl_error_set_where_();

    if (wlen + 3 < FITS_CARD_LEN && commlen > 0) {
        const cxchar *comment = cpl_property_get_comment_(self);
        const int mysz = CX_MIN(FITS_CARD_LEN - (wlen + 3), commlen);

        (void)memcpy(card + wlen, " / ", 3);
        wlen += 3;

        cpl_fits_txtcpy(card + wlen, comment, mysz);
        wlen += mysz;
    }

    if (wlen < FITS_CARD_LEN) /* Space pad remainder of card */
        (void)memset(card + wlen, ' ', FITS_CARD_LEN - wlen);

#ifdef CPL_PROPERTYLIST_DEBUG
    cpl_msg_warning(cpl_func, "FITSFILL(%d:%d:%d:%d:%s): %s", (int)namelen,
                    (int)cpl_property_get_size_(self), (int)commlen, wlen, name,
                    card);
#endif

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Convert text to restricted FITS, replacing non-printables with blanks
  @param dest    A buffer of characters to write to
  @param src     A buffer of characters to read from
  @param sz      The number of characters to convert
  @return void
  @note (Almost) no input validation in this internal function
  @see memcpy()

  In FITS text is restricted to the ASCII range 32 to 126, any other is replaced
  by a blank.

 */
/*----------------------------------------------------------------------------*/
inline static void
cpl_fits_txtcpy(char *dest, const char *src, int sz)
{
#ifdef CPL_USE_CFITSIO_STRICT
    (void)memcpy(dest, src, sz);
#else
    /* The lookup table of the 256 8-bit characters, mapping each
       restricted character (0..31, 127..255) to a blank.
       Each produced character is (naturally) printable, still to produce
       a regularly formatted map each character is coded in hexadecimal. */

    const char *lookup =
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
        "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
        "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"
        "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
        "\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
        "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20";


    for (int i = 0; i < sz; i++) {
        dest[i] = lookup[(const unsigned char)src[i]];
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Check that the key is OK
  @param key     The key to check
  @param sz      The length of the key
  @return Zero on OK, negative on invalid, positive on lower-case
  @note (Almost) no input validation in this internal function

  The return values are:
  Zero: Valid FITS key, i.e. [A-Z0-9_-] and the blank.
  Positive: Lower case letter (a-z), each ASCII value binary or'ed together
  Negative: Any other, each ASCII value or'ed together with most significant bit

 */
/*----------------------------------------------------------------------------*/
inline static char
cpl_fits_check_key(const char *key, int sz)
{
    /* The below initializer was generated with:
       perl -le 'for ($n=0; $n < 256; $n+=16) {printf("        \"");' \
       -e 'grep(printf("\\x%02x", (97 <= $_ && $_ <= 122) ? $_ : '    \
       -e '($_ == 32 || $_ == 45 || 48 <= $_ && $_ <= 57 || '         \
       -e '65 <= $_ && $_ <= 90 || $_ == 95)'                         \
       -e ' ? 0 : ($_ | 128)), ($n)..($n+15));print "\"";};'
     */
    const char *checkmap =
        "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
        "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
        "\x00\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\x00\xae\xaf"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xba\xbb\xbc\xbd\xbe\xbf"
        "\xc0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xdb\xdc\xdd\xde\x00"
        "\xe0\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
        "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\xfb\xfc\xfd\xfe\xff"
        "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
        "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
        "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
        "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
        "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
        "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
        "\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
        "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff";
    char checksum = 0;

    while (sz)
        checksum |= checkmap[(const unsigned char)key[--sz]];

    return checksum;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Convert integer of unspecified type to its string representation
  @param VALUE  Integer L-value to convert, will be modified
  @param CARD   Character array to write to from the end, points to result
  @return Nothing
  @note Card must be long enough! No error checking in this internal function!

  The right alignment avoids copying/reversing the string.

*/
/*----------------------------------------------------------------------------*/
#define CPL_ITOA_RIGHTALIGN(VALUE, CARD)              \
    do {                                              \
        if (VALUE < 0) {                              \
            while (VALUE <= -10) {                    \
                *--(CARD) = (char)('0' - VALUE % 10); \
                VALUE /= 10;                          \
            }                                         \
            *--(CARD) = (char)('0' - VALUE);          \
            *--(CARD) = '-';                          \
        }                                             \
        else {                                        \
            while (VALUE >= 10) {                     \
                *--(CARD) = (char)('0' + VALUE % 10); \
                VALUE /= 10;                          \
            }                                         \
            *--(CARD) = (char)('0' + VALUE);          \
        }                                             \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Convert the value of a property to a string
  @param card    A buffer of FLEN_VALUE+1 (or more) characters
  @param plen    The number of converted characters, or undefined on error
  @param maxlen  The maximum number of characters allowed, at least 8
  @param self    The property to fill from
  @return A pointer to the converted characters, may be without null-terminator
          or NULL on error
  @note (Almost) no input validation in this internal function
  @see cpl_fits_set_value()

  The string representation of the value is first written into a separate
  buffer which is then copied into the FITS card, this is done for several
  reasons:
  1) The actual position in the card where the first character of the converted
     string is to be written depends on the length of the converted string...
  2) A very fast conversion of an integer value can start from the least
     significant byte, but this creates a string where the position of the
     first character is only known when the conversion is complete.
     In this case the filling of the card buffer starts at position FLEN_VALUE.
     In case of a conversion error, the card buffer needs an extra character to
     NULL-terminate the string for when it goes into the CPL error message.
  3) memcpy() is very fast so writing the string representation of the
     value directly to the card and then sometimes having to move it to its
     right position is slower and also more complex.

 */
/*----------------------------------------------------------------------------*/
inline static const char *
cpl_fits_set_value_(char *strval,
                    int *plen,
                    int maxlen,
                    const cpl_property *self)
{
    char *retval = strval; /* Default is to fill from the start */
    const cpl_type type = cpl_property_get_type_(self);

    switch (type) {
        case CPL_TYPE_BOOL: {
            const int b = cpl_property_get_bool_(self);

            retval[0] = b == TRUE ? 'T' : 'F';
            *plen = 1;
        } break;

        case CPL_TYPE_INT: {
            int value = cpl_property_get_int_(self);

            /* Start the conversion at the end of the array,
               working backwards, avoiding extra copying */

            retval += FLEN_VALUE;
            CPL_ITOA_RIGHTALIGN(value, retval);
            *plen = strval + FLEN_VALUE - retval;

        } break;

        case CPL_TYPE_LONG: {
            long value = cpl_property_get_long_(self);

            retval += FLEN_VALUE;
            CPL_ITOA_RIGHTALIGN(value, retval);
            *plen = strval + FLEN_VALUE - retval;

        } break;

        case CPL_TYPE_LONG_LONG: {
            long long value = cpl_property_get_long_long_(self);

            retval += FLEN_VALUE;
            CPL_ITOA_RIGHTALIGN(value, retval);
            *plen = strval + FLEN_VALUE - retval;

        } break;

        case CPL_TYPE_FLOAT: {
            const float value = cpl_property_get_float_(self);

            /* Default CFITSIO float precision: 7, ffr2e() */
            *plen = sprintf(retval, "%.7G", value);

            cx_assert(*plen > 0);
            cx_assert(*plen < FLEN_VALUE);

            /* Check whether the number is special - or just integer */
            if (memchr(retval, '.', *plen) == NULL &&
                memchr(retval, 'E', *plen) == NULL) {
                const int fptype = fpclassify(value);

                if (fptype == FP_NAN || fptype == FP_INFINITE) {
                    (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                                 "FITS does not allow special "
                                                 "float: %s",
                                                 retval);
                    return NULL;
                }
                else {
                    /* Force decimal point onto integer */
                    retval[(*plen)++] = '.';
                }
            }
        } break;

        case CPL_TYPE_DOUBLE: {
            const double value = cpl_property_get_double_(self);

            /* Default CFITSIO double precision: 15, ffd2e() */
            *plen = sprintf(retval, "%.15G", value);

            cx_assert(*plen > 0);
            cx_assert(*plen < FLEN_VALUE);

            /* Check whether the number is special - or just integer */
            if (memchr(retval, '.', *plen) == NULL &&
                memchr(retval, 'E', *plen) == NULL) {
                const int fptype = fpclassify(value);

                if (fptype == FP_NAN || fptype == FP_INFINITE) {
                    (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                                 "FITS does not allow special "
                                                 "double: %s",
                                                 retval);
                    return NULL;
                }
                else {
                    /* Force decimal point onto integer */
                    retval[(*plen)++] = '.';
                }
            }
        } break;

        case CPL_TYPE_FLOAT_COMPLEX: {
            const float complex value = cpl_property_get_float_complex_(self);
            const float val2[2] = { crealf(value), cimagf(value) };
            const int fptype0 = fpclassify(val2[0]);
            const int fptype1 = fpclassify(val2[1]);

            /* Default CFITSIO float precision: 7 */
            *plen = sprintf(retval, "(%.7G, %.7G)", val2[0], val2[1]);

            cx_assert(*plen > 0);
            cx_assert(*plen < FLEN_VALUE);

            /* Check whether the number is special */
            if (fptype0 == FP_NAN || fptype0 == FP_INFINITE ||
                fptype1 == FP_NAN || fptype1 == FP_INFINITE) {
                (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                             "FITS does not allow special "
                                             "float complex: %s",
                                             retval);
                return NULL;
            }
        } break;

        case CPL_TYPE_DOUBLE_COMPLEX: {
            const double complex value = cpl_property_get_double_complex_(self);
            const double val2[2] = { creal(value), cimag(value) };
            const int fptype0 = fpclassify(val2[0]);
            const int fptype1 = fpclassify(val2[1]);

            /* Default CFITSIO double precision: 15 */
            *plen = sprintf(retval, "(%.15G, %.15G)", val2[0], val2[1]);

            cx_assert(*plen > 0);
            cx_assert(*plen < FLEN_VALUE);

            /* Check whether the number is special */
            if (fptype0 == FP_NAN || fptype0 == FP_INFINITE ||
                fptype1 == FP_NAN || fptype1 == FP_INFINITE) {
                (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                             "FITS does not allow special "
                                             "double complex: %s",
                                             retval);
                return NULL;
            }
        } break;

        case CPL_TYPE_CHAR: {
            const char c = cpl_property_get_char_(self);

            /*
             * Character properties are represented as a single
             * character string, not as its numerical equivalent.
             */

            /* Space pad like CFITSIO */
            /* If the character is a quote, it must be escaped by a second one.
               With a default quote in the string literal, only one single
               character has to be written in each case */
            (void)memcpy(retval, "''       '", 10);
            cpl_fits_txtcpy(retval + (c == '\'' ? 2 : 1), &c, 1);
            if (CPL_UNLIKELY(maxlen < 10)) {
                cx_assert(maxlen > 3);
                retval[maxlen - 1] = '\'';
                *plen = maxlen;
            }
            else {
                *plen = 10;
            }
        } break;

        case CPL_TYPE_STRING: {
            /* The size is the string length incl. the null-byte */
            const int valuesize = (int)cpl_property_get_size_(self);
            int mysz = 0;
            int npad;

            retval[mysz++] = '\'';

            if (CPL_LIKELY(valuesize > 1)) {
                const char *value = cpl_property_get_string_(self);
                /* Need space also for enclosing quotes */
                int remsize = CX_MIN(valuesize - 1, maxlen - 2);
                int rpos = 0;
                const char *qpos;

                while (CPL_LIKELY(remsize > 0) &&
                       CPL_UNLIKELY((qpos = memchr(value + rpos, '\'',
                                                   remsize)) != NULL)) {
                    const int prelen = qpos - (value + rpos); /* Pre-quote */

                    cx_assert(qpos >= value + rpos);

                    cpl_fits_txtcpy(retval + mysz, value + rpos, prelen);

                    mysz += prelen;
                    rpos += prelen + 1; /* Includes quote to be escaped */
                    /* Remaining space needs one extra quote */
                    remsize = CX_MIN(valuesize - rpos - 1, maxlen - mysz - 3);

                    if (remsize >= 0) {
                        retval[mysz++] = '\''; /* Escape the quote */
                        retval[mysz++] = '\''; /* Escape the quote */
                    }
                    else {
                        remsize = 0;
                    }
                }

                cpl_fits_txtcpy(retval + mysz, value + rpos, remsize);
                mysz += remsize;
            }

            npad = CX_MIN(9, maxlen - 2) - mysz;
            if (npad > 0) {
                /* Space pad like CFITSIO */
                (void)memset(retval + mysz, ' ', npad);
                mysz += npad;
            }

            retval[mysz++] = '\'';

            *plen = mysz;

            break;
        }

        default:
            (void)cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                         "type=%d ('%s')", type,
                                         cpl_type_get_name(type));
            return NULL;
    }

    if (*plen > maxlen) {
        cx_assert(type != CPL_TYPE_STRING);
        retval[*plen] = '\0'; /* Cannot overflow string buffer */
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                     "Key + '%s'-value too long for FITS card: "
                                     "%d < %d (%*s)",
                                     cpl_type_get_name(type), *plen, maxlen,
                                     *plen, retval);
        return NULL;
    }

    return retval;
}
/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Set the value of a FITS card
  @param card    A buffer of 80 (or more) characters
  @param plen    The number of characters in card, or undefined on error
  @param self    The property to fill from
  @return CPL_ERROR_NONE, or the relevant CPL error on failure
  @note (Almost) no input validation in this internal function
  @see cpl_fits_fill_card(), fits_make_key(), ffmkky(), fftkey()

 */
/*----------------------------------------------------------------------------*/
inline static cpl_error_code
cpl_fits_set_value(char *card, int *plen, const cpl_property *self)
{
    char strval[FLEN_VALUE + 1];
    const int maxlen = FLEN_VALUE - CX_MAX(1, *plen - 7);
    int mysz = 0; /* The length of the value as a string */

    /* First write the value to a temp buffer and then copy it */
    const char *myval = cpl_fits_set_value_(strval, &mysz, maxlen, self);

    if (CPL_UNLIKELY(myval == NULL))
        return cpl_error_set_where_();

    cx_assert(mysz <= maxlen);

    if (*myval == '\'') {
        if (*plen > 8 && *plen + mysz + 2 < FITS_CARD_LEN) {
            (void)memcpy(card + *plen, VALINDSTR, 3);
            *plen += 3;
        }
        else {
            (void)memcpy(card + *plen, VALINDSTR + 1, 2);
            *plen += 2;
        }

        (void)memcpy(card + *plen, myval, mysz);
        *plen += mysz;

        if (*plen < CPL_ALIGN_POS) {
            (void)memset(card + *plen, ' ', CPL_ALIGN_POS - *plen);
            *plen = CPL_ALIGN_POS;
        }
    }
    else {
        if (*plen + mysz + 2 < CPL_ALIGN_POS) {
            (void)memcpy(card + *plen, *plen > 8 ? VALINDSTR : VALINDSTR + 1,
                         CPL_ALIGN_POS - *plen - mysz);
            *plen = CPL_ALIGN_POS - mysz;
        }
        else if (*plen > 8 && *plen + mysz + 2 < FITS_CARD_LEN) {
            (void)memcpy(card + *plen, VALINDSTR, 3);
            *plen += 3;
        }
        else {
            (void)memcpy(card + *plen, VALINDSTR + 1, 2);
            *plen += 2;
        }
        (void)memcpy(card + *plen, myval, mysz);
        *plen += mysz;
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Determine whether the key is commentary
   @param keywlen  The name w. length of the property
   @return Zero on commentary, non-zero otherwise
   @note (Almost) no input validation in this internal function
   @see fits_set_key()

*/
/*----------------------------------------------------------------------------*/
inline int
cpl_fits_key_is_comment(const cpl_cstr *keywlen)
{
    const char *key = cx_string_get_(keywlen);
    cxsize wlen = cx_string_size_(keywlen);


    /* Leading and trailing spaces are ignored, if nothing remains the key is
       blank. */
    while (!memcmp(key, BLANK80, CX_MIN(FITS_CARD_LEN, wlen))) {
        if (CPL_LIKELY(wlen <= FITS_CARD_LEN)) {
            return 0; /* Blank key: Commentary card */
        }
        wlen -= FITS_CARD_LEN;
        key += FITS_CARD_LEN;
    }

    /* Key is not blank, skip (remaining) initial+trailing blanks */
    while (CPL_UNLIKELY(*key == ' ')) {
        key++; /* Skip initial blanks */
        wlen--;
    }
    while (CPL_UNLIKELY(key[wlen - 1] == ' '))
        wlen--; /* Skip trailing blanks */

    cx_assert(*key != ' ');
    cx_assert(wlen > 0);

    return wlen == 7 &&
                   (!memcmp(key, "COMMENT", 7) || !memcmp(key, "HISTORY", 7))
               ? 0
               : 1;
}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Set the key
   @param card     A buffer of 80 (or more) characters
   @param plen     The number of characters in card, or undefined on error
   @param type     The type of the value, needed for commentary cards
   @param keywlen  The name w. length of the property
   @param ksort    The sort key of the property
   @return Zero on normal card, positive on commentary, negative on error
   @note (Almost) no input validation in this internal function
   @see cpl_property_set_sortkey(), fits_make_key(), ffmkky(), fftkey()

  A lower case key character [a-z] does not cause an error, but is silently
  changed to upper case, like CFITSIO does.

*/
/*----------------------------------------------------------------------------*/
inline static int
cpl_fits_set_key(char *card,
                 int *plen,
                 const cpl_cstr *keywlen,
                 cpl_property_sorttype ksort)
{
    const char *key = cx_string_get_(keywlen);
    cxsize wlen = cx_string_size_(keywlen); /* Default write length */
    int keytype = 0;
    char checksum = 0;

    /* Determine the type of key in order of typical frequency */

    /* Since leading and trailing blanks need to be ignored and since an
       all blank card can occur (albeit not very often), we use the very
       fast memcmp() to detect a blank card. After that we know that the
       key has a non-blank and we will assume that leading and trailing
       blanks are rare, since few use them. */

    while (!memcmp(key, BLANK80, CX_MIN(FITS_CARD_LEN, wlen))) {
        if (CPL_LIKELY(wlen <= FITS_CARD_LEN)) {
            (void)memcpy(card, BLANK80, 8);
            *plen = 8;
            return 1; /* Blank key: Commentary card */
        }
        wlen -= FITS_CARD_LEN; /* Using so many blanks is really expensive... */
        key += FITS_CARD_LEN;
    }

    /* Key is not blank, skip (remaining) initial+trailing blanks */
    while (CPL_UNLIKELY(*key == ' ')) {
        key++; /* Skip initial blanks */
        wlen--;
    }
    while (CPL_UNLIKELY(key[wlen - 1] == ' '))
        wlen--; /* Skip trailing blanks */

    cx_assert(*key != ' ');
    cx_assert(wlen > 0);

    if (wlen > 8) {
        keytype = 'H'; /* ESO HIERARCH key, assumed to need HIERARCH prolog */

        if (ksort & CPL_DICB_HIERARCH_XYZ) {
            /* Need to verify all but the first 7 key characters
               pre-verified as "ESO XYZ" */
            checksum = cpl_fits_check_key(key + 7, wlen - 7);
        }
        else if (!memcmp(key, "ESO ", 4)) {
            /* HIERARCH key extension came from ESO so is its most likely use */

            checksum = cpl_fits_check_key(key + 4, wlen - 4);
        }
        else if (CPL_UNLIKELY(memcmp(key, "HIERARCH ", 9) != 0)) {
            /* Not HIERARCH */

            checksum = cpl_fits_check_key(key, wlen);
        }
        else if (CPL_LIKELY(wlen < FLEN_KEYWORD - 1)) {
            /* Long w. HIERARCH, so "Standard" copy of key */

            keytype = 'S';

            /* Skip verification of already present "HIERARCH " */
            checksum = cpl_fits_check_key(key + 9, wlen - 9);
        }
        else {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                         "FITS HIERARCH key too long: %d > %d",
                                         (int)wlen, FLEN_KEYWORD - 2);
            return -1;
        }
    }
    else if (wlen == 7 &&
             (!memcmp(key, "COMMENT", 7) || !memcmp(key, "HISTORY", 7))) {
        keytype = 'C'; /* Commentary key */
    }
    else if (CPL_DICB_UNDEF < ksort && ksort < CPL_DICB_PRIMARY) {
        keytype = 'S'; /* Pre-verified Standard key */
    }
    else {
        keytype = CPL_LIKELY(memchr(key, ' ', wlen) == NULL)
                      ? 'S'  /* Standard key */
                      : 'H'; /* No known HIERARCH key of at most 8 characters */

        /* Need to verify all the (up to 8) key characters */
        checksum = cpl_fits_check_key(key, wlen);
    }

    if (keytype != 'H') {
        (void)memcpy(card, key, wlen);
        if (wlen < 8) {
            (void)memset(card + wlen, ' ', 8 - wlen);
            wlen = 8;
        }
        *plen = wlen;
    }
    else if (CPL_LIKELY(wlen < FLEN_KEYWORD - 10)) {
        (void)memcpy(card, "HIERARCH ", 9);
        (void)memcpy(card + 9, key, wlen);
        *plen = 9 + wlen;
    }
    else {
        (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                     "FITS HIERARCH key too long: %d + 9 > %d",
                                     (int)wlen, FLEN_KEYWORD - 2);
        return -1;
    }

    if (CPL_UNLIKELY(checksum)) {
        if (checksum < 0) {
            const char checkchar[2] = { checksum & 0x7f, '\0' };
            (void)cpl_error_set_message_(
                CPL_ERROR_ILLEGAL_INPUT,
                "Bad %d-character FITS key (\"%*s\") w. mask: 0x%02x (\"%s\")",
                (int)wlen, (int)wlen, key, (int)*checkchar, checkchar);
            return -1;
        }
        else {
            for (int i = 0; i < *plen; i++) {
                card[i] = toupper(card[i]);
            }
        }
    }

    return keytype == 'C' ? 1 : 0;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Get the length of the key of a FITS card
  @param card     A FITS card (80 bytes or more) may not have null terminator
  @param plen     The length, zero for a key-less card, undefined on error
  @param piparsed The index to the last parsed character
  @return On success, points to start of key (skips "HIERARCH "), else NULL
  @note (Almost) no input validation in this internal function
  @see fits_get_keyname(), ffgknm()

  On success *piparsed is the index to the last parsed character, for a
  ESO HIERARCH card *piparsed is the index of the value indicator ('='), for a
  non ESO HIERARCH card *piparsed is 8 (whether or not that byte has a
  value indicator).

 */
/*----------------------------------------------------------------------------*/
inline static const char *
cpl_fits_get_key(const char *card, int *plen, int *piparsed)
{
    const char *kstart = card;

    if (!memcmp(card, "HIERARCH ", 9)) {
        /* A malformed card could be missing its value indicator (and could
           instead have an '=' sign in its comment) or the key could be invalid.
           The purpose of this library is not to process invalid FITS data, so
           in such cases the card may be dropped silently or a property with an
           unusual key may be created. */
        const char *eqpos = memchr(card + 9, '=', FITS_CARD_LEN - 9);

        if (CPL_LIKELY(eqpos != NULL)) {
            *piparsed = (int)(eqpos - card);

            /* If available memrchr() would not help much here, unlikely
               to find a whole (properly aligned) word of ' ' */
            while (*--eqpos == ' ')
                ;

            if (CPL_LIKELY(eqpos > card + 7)) {
                kstart += 9;
                *plen = (int)(1 + eqpos - kstart);
            }
            else {
                /* A non-standard HIERARCH card with a value indicator */
                *plen = 8;
            }
        }
        else {
            /* A non-standard HIERARCH card without a value indicator */
            *plen = 8;
            *piparsed = 8;
        }
    }
    else {
        /* The standard allows for non-HIERARCH keys with up to 8 characters
           and any trailing blanks are not counted as part of the key. */
        const char *spcpos = memchr(card, ' ', 8);

        *plen = spcpos ? (int)(spcpos - card) : 8;

        *piparsed = 8;
    }

    return kstart;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get a numerical value from a FITS card
  @param card      A FITS card (80 bytes), must have null terminator
  @param iparsed   The value from cpl_fits_get_key() or just before the number
  @param plval     On positive return, *plval is the integer, unless NULL
  @param pdval     On negative return, *pdval is the double
  @param pjparsed  The index to the last parsed character
  @return Positive for int, negative for floating point, zero for NaN (error)
  @note (Almost) no input validation in this internal function!
  @see cpl_fits_get_key()

  On error, *pjparsed is undefined.

  Parsing a string for a number is non-trivial, so use strtoll() + strtod().

  If plval is non-NULL, the resulting number is either integer or float,
  as indicatd by the return value.

  If the string has a decimal point or an exponent part, the resulting
  number is a float even if its fractional part is zero.

  If the string has neither but the conversion to a signed long long int
  causes an overflow, the resulting number is deemed float.

  The use of strtod() is lenient to silently allow certain strings that
  are not strictly FITS to still be accepted with the value returned by
  strtod() as long as the value is normal or zero, e.g. 42e1.

 */
/*----------------------------------------------------------------------------*/
inline static int
cpl_fits_get_number(const char *card,
                    int iparsed,
                    long long *plval,
                    double *pdval,
                    int *pjparsed)
{
    char *endptr;

    if (plval != NULL) {
        /* Since strtoll() is several times faster than strtod() and since
           after strtoll() it is trivial to detect a non-integer, strtoll()
           is called first.
           If necessary, strtod() is then called on the same address. */

        errno = 0; /* Integer parsing can result in overflow, ERANGE */
        *plval = strtoll(card + iparsed, &endptr, 10);

        if ((*endptr == ' ' || *endptr == '\0' ||
             /* FITS std. 4.1.2.3: "A space between the value and
                the slash is strongly recommended. */
             CPL_UNLIKELY(*endptr == '/')) &&
            CPL_LIKELY(errno == 0)) {
            *pjparsed = (int)(endptr - card);
            return 1; /* Done: It is an integer fitting a long long */
        }
        /* The string value may be valid but is not an integer that fits a
           long long int. */
    }

    errno = 0; /* Floating-point parsing can result in overflow, ERANGE */
    *pdval = strtod(card + iparsed, &endptr);

    /* The FITS standard (4.2.4) allows for a floating-point constant with a 'D'
       starting the exponent part. This format cannot by handled by strtod().
       (Coincidentally, in FORTRAN a double precision constant uses a 'D').
       Since this format is quite rare we don't try to avoid calling strtod()
       twice on e.g. 0D0. */

    if (CPL_UNLIKELY(*endptr == 'D')) {
        /* Deal with the FORTRAN-format by replacing the 'D'
           with an 'E' in a copy */

        char numparse[FLEN_VALUE];

        cx_assert(FITS_CARD_LEN - iparsed + 1 <= FLEN_VALUE);

        /* Card has null terminator, copy it as well */
        (void)memcpy(numparse, card + iparsed, FITS_CARD_LEN - iparsed + 1);

        numparse[endptr - (card + iparsed)] = 'E';

        errno = 0; /* Floating-point parsing can result in overflow, ERANGE */
        *pdval = strtod(numparse, &endptr);

        /* Transform back to card base */
        endptr += card + iparsed - numparse;
    }

    *pjparsed = (int)(endptr - card);

    /* While strtod() converts the various lower and upper case variations of
       the strings 'NaN' and '+/-/Inf' (that end with a letter) these strings
       are not valid FITS. */
    return CPL_UNLIKELY(errno != 0) || CPL_UNLIKELY(isalpha(*(endptr - 1)))
               ? 0
               : -1;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the parsed value and the comment location of a FITS card
  @param pparseval On success, the parsed value and its type
  @param card      A FITS card (80 bytes), must have null terminator
  @param iparsed   The value from cpl_fits_get_key()
  @param keywlen   The name w. length of the property
  @param pjparsed  The index to the last parsed character (comment separator, /)
  @return The FITS type code, or zero on error
  @note (Almost) no input validation in this internal function!
  @see fits_parse_value(), ffpsvc(), ffdtyp()

  The possible return values are these FITS type codes:
    'C', 'L', 'F', 'I', 'X' - as well as
    'U' (undefined), 'N' (none) or 0 (unparsable value in card)

  If the card has no value indicator, everything after its expected location
  is deemed a comment, per FITS standard sections 4.1.2.2 and 4.1.2.3.

  If the value is of type string, the enclosing quotes are left out. If the
  string itself contains (FITS-encoded, per 4.2.1 1) quotes, each is converted.
  If the string itself contains no quotes, the pointer to its value points
  to the relevant byte in the input card, so that string value is only valid
  while the card itself is.

  On error, *pjparsed is undefined.

 */
/*----------------------------------------------------------------------------*/
inline static char
cpl_fits_get_value(cpl_fits_value *pparseval,
                   const char *card,
                   int iparsed,
                   const cpl_cstr *keywlen,
                   int *pjparsed)
{
    if (card[iparsed] != '=' || cx_string_size_(keywlen) == 0 ||
        (cx_string_size_(keywlen) == 7 &&
         (!memcmp("COMMENT", cx_string_get_(keywlen), 7) ||
          !memcmp("HISTORY", cx_string_get_(keywlen), 7)))) {
        /* Card is commentary, i.e. it has no value indicator or
           it is a COMMENT/HISTORY or blank card, see FITS std. 4.4.2.4.
           Everything after the key is a comment */

        pparseval->tcode = 'N';
        *pjparsed = cx_string_size_(keywlen) ? 8 : 0; /* Card may be blank */
    }
    else {
        /* Value indicator is present */

        /* - skip it and any leading spaces */
        while (card[++iparsed] == ' ')
            ; /* Rely on null terminator */

        if (iparsed < FITS_CARD_LEN) {
            /* card[iparsed] now points to the first value byte */

            /* Assume failure */
            pparseval->nmemb = 0;
            pparseval->tcode = 0;

            switch (card[iparsed]) {
                case '\'': { /* character string starts with a quote */
                    const char *nextquote;
                    int vallen = 0; /* The number of value characters */

                    /* - need to increase iparsed to point past current quote
                   when looking for the next one */
                    iparsed++;
                    while ((nextquote = memchr(card + iparsed, '\'',
                                               FITS_CARD_LEN - iparsed)) != NULL
                           /* We silently ignore the error where there is 1 byte
                          after the ending quote and before the end-of-record
                          and that last byte equals a quote */
                           && nextquote + 1 < card + FITS_CARD_LEN &&
                           nextquote[1] == '\'') {
                        /* O''HARA -> O'HARA */

                        /* The parsed string differs from the FITS-encoded,
                       so we need to copy it (including the found quote) */
                        (void)memcpy(pparseval->unquote + vallen,
                                     card + iparsed,
                                     (int)(nextquote + 1 - card - iparsed));

                        vallen += (int)(nextquote + 1 - card - iparsed);

                        /* iparsed must be updated to point to the 2nd quote */
                        iparsed = 2 + nextquote - card;
                    }

                    if (nextquote != NULL) {
                        /* Found the ending quote (none would be a format error) */
                        pparseval->tcode = 'C';

                        if (vallen > 0) {
                            /* Wrote to decoded string buffer */
                            pparseval->val.c = pparseval->unquote;

                            /* Copy part of string following encoded quote */
                            (void)memcpy(pparseval->unquote + vallen,
                                         card + iparsed,
                                         (int)(nextquote - card - iparsed));
                        }
                        else {
                            /* Reference to original, quote-free string */
                            pparseval->val.c = card + iparsed;
                        }

                        vallen += (int)(nextquote - card - iparsed);

                        /* FITS standard 4.2.1 1:
                                          Given the example
                      "KEYWORD2= '     ' / empty string keyword"
                                         and the statement
                      "the value of the KEYWORD2 is an empty string (nominally "
                      "a single space character because the first space in the "
                      " string is significant, but trailing spaces are not)."
                      a string consisting solely of spaces is deemed to be empty
                    */
                        while (vallen > 0 &&
                               pparseval->val.c[vallen - 1] == ' ') {
                            vallen--;
                        }

                        /* Update iparsed to point to first byte after value */
                        iparsed = 1 + (int)(nextquote - card);
                    }

                    pparseval->nmemb = vallen;

                    break;
                }
                case 'T':
                    pparseval->val.l = 1;
                    pparseval->nmemb = 1;
                    pparseval->tcode = 'L'; /* logical True ('T' character) */
                    /* Update iparsed to point to first byte after value */
                    iparsed++;
                    break;
                case 'F':
                    pparseval->val.l = 0;
                    pparseval->nmemb = 1;
                    pparseval->tcode = 'L'; /* logical: False ('F' character) */
                    /* Update iparsed to point to first byte after value */
                    iparsed++;
                    break;
                case '(': {
                    double dval[2];
                    const char dsep[2] = { ',', ')' };
                    int i;

                    iparsed++; /* Skip '(' */
                    for (i = 0; i < 2; i++) {
                        /* FITS std. 4.2.5: Integer allowed, but parse it as double
                       since that is all we can store */
                        const int ntype =
                            cpl_fits_get_number(card, iparsed, NULL, dval + i,
                                                &iparsed);

                        if (ntype != -1)
                            break;

                        /* FITS std. 4.2.5/6: Trailing blanks are allowed */
                        while (card[iparsed] ==
                               ' ') { /* Rely on null terminator */
                            iparsed++;
                        }
                        if (!(iparsed < FITS_CARD_LEN) ||
                            card[iparsed] != dsep[i])
                            break;
                        iparsed++;
                    }

                    if (i < 2) {
                        pparseval->nmemb = 0;
                        pparseval->tcode = 0;
                        *pjparsed =
                            iparsed; /* Invalid character - for error msg */
                    }
                    else {
                        pparseval->val.x = dval[0] + dval[1] * _Complex_I;
                        pparseval->nmemb = 1;
                        pparseval->tcode =
                            'X'; /* complex datatype "(1.2, -3.4)" */
                    }
                    break;
                }
                case '/': /* Undefined value - with a comment */
                    pparseval->nmemb = 0;
                    pparseval->tcode = 'U';
                    break; /* No value found, so do not advance iparsed */

                default: { /* Numerical type */
                    /* In determining whether it is int or float, we might as
                   well parse it. */
                    double dvalue = 0.0;
                    long long lvalue = 0;

#ifdef CPL_PROPERTYLIST_DEBUG
                    const int iiparsed = iparsed;
#endif
                    const int ntype =
                        cpl_fits_get_number(card, iparsed, &lvalue, &dvalue,
                                            &iparsed);

#ifdef CPL_PROPERTYLIST_DEBUG
                    cpl_msg_warning(cpl_func,
                                    "FITSNUM(%d:%d<=%d): %lld %g (%c) %s",
                                    ntype, iiparsed, iparsed, lvalue, dvalue,
                                    card[iparsed], card + iparsed);
#endif

                    if (CPL_LIKELY(ntype != 0)) {
                        if (ntype < 0) {
                            pparseval->tcode = 'F';
                            pparseval->val.f = dvalue;
                        }
                        else {
                            pparseval->tcode = 'I';
                            pparseval->val.i = lvalue;
                        }
                        pparseval->nmemb = 1;
                    }
                }
            }

            /* Now iparsed points to first byte after value */

            if (CPL_LIKELY(pparseval->tcode != 0)) {
                /* Found value. Look for comment */

                const char *comchar;
                /* Cast to avoid gcc-warning: Wstringop-overread */
                const size_t remsize = FITS_CARD_LEN - (size_t)iparsed;

                cx_assert(iparsed >= 0);
                cx_assert(iparsed <= FITS_CARD_LEN);

                comchar = memchr(card + iparsed, '/', remsize);

                *pjparsed = CPL_LIKELY(comchar != NULL)
                                ? (int)(comchar - card) /* Start of comment */
                                : FITS_CARD_LEN;        /* Nothing left */

                if (CPL_LIKELY(*pjparsed > iparsed) &&
                    CPL_UNLIKELY(card[iparsed] != ' ')) {
                    /* A value was found but the subsequent byte is invalid */
                    /* In principle a malformed FITS card could have subsequent
                       non-space byte(s) before the comment/EOL. An actual FITS
                       verification could use memcmp() against that. */

                    pparseval->tcode = 0;
                    pparseval->nmemb = 0;
                    *pjparsed = iparsed; /* Invalid character - for error msg */
                }
            }

            return pparseval->tcode;
        }
        /* The value and its type is undefined,
           both value and comment have zero length*/

        pparseval->tcode = 'U';
        *pjparsed = FITS_CARD_LEN; /* Nothing left */
    }

    /* No value */
    pparseval->nmemb = 0;
    return pparseval->tcode;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Determine whether a given property must be floating point
  @param    keywlen  The name w. length of the property, must be non-NULL
  @return   Non-zero iff the property must be floating point
  @note This internal function has no error checking

  The function does not validate its input according to the FITS standard,
  it merely determines whether the given key must be loaded as a floating
  point type, even if its actual value can be represented as an integer.

  https://fits.gsfc.nasa.gov/standard40/fits_standard40aa-le.pdf

  Per this paper coauthored by M. R. Calabretta:
  https://www.aanda.org/articles/aa/full/2002/45/aah3859/aah3859.html

  these are numerical keys of a floating point type, where the axis is 1-99
  and where the total key length cannot exceed 8:

    CRPIX[0-9]+
    CRVAL[0-9]+
    CDELT[0-9]+
    CRDER[0-9]+
    CSYER[0-9]+
    PC[0-9]+_[0-9]+
    PV[0-9]+_[0-9]+
    CD[0-9]+_[0-9]+
    EQUINOX
    EPOCH
    MJD-OBS
    LONGPOLE
    LATPOLE

 Yes - and partly for historical reasons this will also promote to float keys
 such as CRPIX0, CRPIX001, CRPIX111, PC0_1, etc.

 */
/*----------------------------------------------------------------------------*/
inline static int
cpl_property_is_float(const cpl_cstr *keywlen)
{
    const char *key = cx_string_get_(keywlen);
    const cxsize keylen = cx_string_size_(keywlen);
    int isfloat = 0; /* Default is integer type */
    /* The state of multi-digit parsing:
       0: Nothing,
       1: A sequence of digits,
       2: Above, followed by a '_'
    */
    int parsestate = 0;

    /* Switch on the length, i.e. the number of input characters */

    /* First matching the length of the key to the length of the string literal
       allows for a static-length memcmp() which should be inline-able. */

    /* NB: The indentation that aligns the calls to memcmp() helps to
       ensure that strings of identical length share the correct branch */

    switch (keylen) {
        case 8:
            if (isdigit(key[7])) {
                parsestate = 1; /* Fall through */
            }
            else if (!memcmp(key, "LONGPOLE", 8)) {
                isfloat = 1;
                break;
            }
            else {
                break;
            }

            CPL_ATTR_FALLTRHU; /* fall through */

        case 7:
            if (parsestate) {
                if (key[6] == '_') {
                    parsestate = 2; /* Fall through */
                }
                else if (isdigit(key[6])) {
                    /* Fall through */
                }
                else {
                    break;
                }
            }
            else if (isdigit(key[6])) {
                parsestate = 1; /* Fall through */
            }
            else if (!memcmp(key, "MJD-OBS", 7) || !memcmp(key, "EQUINOX", 7) ||
                     !memcmp(key, "LATPOLE", 7)) {
                isfloat = 1;
                break;
            }
            else {
                break;
            }

            CPL_ATTR_FALLTRHU; /* fall through */

        case 6:
            if (parsestate <= 1) {
                if (isdigit(key[5])) {
                    if (!memcmp(key, "CRPIX", 5) || !memcmp(key, "CRVAL", 5) ||
                        !memcmp(key, "CDELT", 5) || !memcmp(key, "CRDER", 5) ||
                        !memcmp(key, "CSYER", 5)) {
                        isfloat = 1;
                        break;
                    }
                    else {
                        parsestate = 1; /* Fall through */
                    }
                }
                else if (CPL_UNLIKELY(parsestate && key[5] == '_')) {
                    parsestate = 2; /* Fall through */
                }
                else {
                    break;
                }
            }
            else if (CPL_UNLIKELY(isdigit(key[5]))) {
                /* Fall through */
            }
            else {
                break;
            }

            CPL_ATTR_FALLTRHU; /* fall through */

        case 5:
            if (parsestate <= 1) {
                if (!parsestate && !memcmp(key, "EPOCH", 5)) {
                    isfloat = 1;
                    break;
                }
                else if (key[3] == '_' && isdigit(key[4])) {
                    /* parsestate = 2; */
                }
                else if (key[4] == '_' && isdigit(key[3])) {
                    /* parsestate = 2; */
                }
                else {
                    break;
                }
            }
            else if (CPL_UNLIKELY(isdigit(key[4]) && isdigit(key[3]))) {
                /* parsestate = 2; */
            }
            else {
                break;
            }

            if (isdigit(key[2]) &&
                (!memcmp(key, "PC", 2) || !memcmp(key, "PV", 2) ||
                 !memcmp(key, "CD", 2))) {
                isfloat = 1;
            }
            break;

        default:
            break;
    }

#ifdef CPL_PROPERTYLIST_DEBUG
    if (isfloat) {
        char keystr[FLEN_KEYWORD]; /* Needed for %s */
        (void)memcpy(keystr, key, keylen);
        keystr[keylen] = '\0';

        cpl_msg_warning(cpl_func, "Floating point key(%zd): %s", keylen,
                        keystr);
    }
#endif

    return isfloat;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Get the comment of a FITS card
  @param card     A FITS card (80 bytes or more) may not have null terminator
  @param jparsed  The value from cpl_fits_get_value()
  @param plen     The length when present, otherwise zero
  @return Points to start of comment, or NULL if no comment present
  @note (Almost) no input validation in this internal function
  @see fits_get_keyname(), ffgknm()

  A value of 80 (or more) for jparsed is allowed and interpreted as no comment
  available.

 */
/*----------------------------------------------------------------------------*/
inline static const char *
cpl_fits_get_comment(const char *card, int jparsed, int *plen)
{
    /* The comment is not a comment if it is empty */
    if (jparsed + 1 < FITS_CARD_LEN && card[jparsed++] == '/') {
        /* Since for some comments it is recommended that a space follows the
           comment byte (/) (FITS standard 4.3.2), such a space is not
           considered part of the commentary text */

        if (card[jparsed] == ' ')
            jparsed++;

        if (jparsed < FITS_CARD_LEN) {
            *plen = FITS_CARD_LEN - jparsed;
            /* Drop trailing spaces */
            while (card[jparsed + *plen - 1] == ' ' && *plen > 0) {
                *plen -= 1;
            }
            return *plen > 0 ? card + jparsed : NULL;
        }
    }

    *plen = 0;
    return NULL;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Deallocate memory used for checking key uniqueness
  @param putkey   Array of keys arrays already written, ordered after length
  @return Nothing
  @note (Almost) no input validation in this internal function

 */
/*----------------------------------------------------------------------------*/
static void
cpl_fits_key_free_unique(const char **putkey[])
{
#ifdef CPL_PROPERTYLIST_DEBUG
    cpl_size maxpos = 0;
    cpl_size maxlen = 0;
    cpl_size nonull = 0;
#endif
    for (cpl_size i = 0; i < FLEN_KEYWORD; i++) {
        if (putkey[i]) {
#ifdef CPL_PROPERTYLIST_DEBUG
            cpl_size k = 0;

            while (putkey[i][k++] != NULL)
                ;

            nonull++;
            if (k > maxlen) {
                maxlen = k;
                maxpos = i;
            }
            cpl_msg_info(cpl_func, "UNIQ-check(%d<%d): %d", (int)i,
                         FLEN_KEYWORD, (int)k);
#endif
            cpl_free(putkey[i]);
        }
    }

#ifdef CPL_PROPERTYLIST_DEBUG
    cpl_msg_warning(cpl_func, "UNIQ-CHECK(%d:%d<%d): %d", (int)nonull,
                    (int)maxpos, FLEN_KEYWORD, (int)maxlen);
#endif
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Reset memory used for checking key uniqueness
  @param putkey   Array of keys arrays already written, ordered after length
  @return Nothing
  @note (Almost) no input validation in this internal function

 */
/*----------------------------------------------------------------------------*/
static void
cpl_fits_key_reset_unique(const char **putkey[])
{
    for (cpl_size i = 0; i < FLEN_KEYWORD; i++) {
        if (putkey[i]) {
            putkey[i][0] = NULL;
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Uniqueness check of a key with the given length
  @param KLEN The key length
  @return Nothing
  @note (Almost) no input validation in this internal macro

*/
/*----------------------------------------------------------------------------*/
#define CPL_FITS_IS_UNIQUE_ONE(KLEN)                                    \
    do {                                                                \
        cpl_size i = 0;                                                 \
        if (putkey[KLEN] != NULL) {                                     \
            while (putkey[KLEN][i] != NULL) {                           \
                if (!memcmp(putkey[KLEN][i], keyname, KLEN))            \
                    return 1;                                           \
                i++;                                                    \
            };                                                          \
        }                                                               \
        else {                                                          \
            /* One extra for the NULL-terminator */                     \
            putkey[KLEN] = cpl_malloc((ntocheck + 1) * sizeof(char *)); \
        }                                                               \
                                                                        \
        putkey[KLEN][i] = keyname;                                      \
        putkey[KLEN][i + 1] = NULL;                                     \
                                                                        \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief Uniqueness check of a key with the given length, larger than 8
   @param KLEN The key length
   @return Nothing
   @note Longer keys tend to differ only at the end, call memcmp() differently
   @see CPL_FITS_IS_UNIQUE_ONE()

*/
/*----------------------------------------------------------------------------*/
#define CPL_FITS_IS_UNIQUE_TWO(KLEN)                                        \
    do {                                                                    \
        cpl_size i = 0;                                                     \
        if (putkey[KLEN] != NULL) {                                         \
            while (putkey[KLEN][i] != NULL) {                               \
                if (!memcmp(putkey[KLEN][i] + KLEN - 8, keyname + KLEN - 8, \
                            8) &&                                           \
                    !memcmp(putkey[KLEN][i], keyname, KLEN - 8))            \
                    return 1;                                               \
                i++;                                                        \
            };                                                              \
        }                                                                   \
        else {                                                              \
            /* One extra for the NULL-terminator */                         \
            putkey[KLEN] = cpl_malloc((ntocheck + 1) * sizeof(char *));     \
        }                                                                   \
                                                                            \
        putkey[KLEN][i] = keyname;                                          \
        putkey[KLEN][i + 1] = NULL;                                         \
                                                                            \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Check if a FITS card has already been written
  @param putkey   Array of keys arrays already written, ordered after length
  @param key      The key to check
  @param ntocheck Number of keys to check after this one
  @return Zero if the card has not (yet) been written, negative if exempt
  @note (Almost) no input validation in this internal function

  As a poor-man's hash this check first groups written keys according to their
  length, so the actual string comparison is done only on keys with matching
  lengths, reducing the number of memcmp() calls.

 */
/*----------------------------------------------------------------------------*/
inline static int
cpl_fits_key_is_unique(const char **putkey[],
                       const cpl_cstr *key,
                       cpl_size ntocheck)
{
    cxsize keylen = cx_string_size_(key);
    const char *keyname = cx_string_get_(key);

    /* Trailing blanks in the key should not occur.
       Regardless (and since for a normal key it is only one check), make
       sure to remove any since they would be ignored in a FITS card. */
    while (CPL_LIKELY(keylen > 0) && CPL_UNLIKELY(keyname[keylen - 1] == ' ')) {
        keylen--;
    }

    /* Use this cumbersome switch so each call to memcmp() can be inline'd */
    switch (keylen) {
        case 0:
            return -1; /* Zero length may come from all spaces */
        case 1:
            CPL_FITS_IS_UNIQUE_ONE(1);
            break;
        case 2:
            CPL_FITS_IS_UNIQUE_ONE(2);
            break;
        case 3:
            CPL_FITS_IS_UNIQUE_ONE(3);
            break;
        case 4:
            CPL_FITS_IS_UNIQUE_ONE(4);
            break;
        case 5:
            CPL_FITS_IS_UNIQUE_ONE(5);
            break;
        case 6:
            CPL_FITS_IS_UNIQUE_ONE(6);
            break;
        case 7:
            if (!memcmp("COMMENT", keyname, 7) ||
                !memcmp("HISTORY", keyname, 7))
                return -1;
            CPL_FITS_IS_UNIQUE_ONE(7);
            break;
        case 8:
            CPL_FITS_IS_UNIQUE_ONE(8);
            break;
        case 9:
            CPL_FITS_IS_UNIQUE_TWO(9);
            break;
        case 10:
            CPL_FITS_IS_UNIQUE_TWO(10);
            break;
        case 11:
            CPL_FITS_IS_UNIQUE_TWO(11);
            break;
        case 12:
            CPL_FITS_IS_UNIQUE_TWO(12);
            break;
        case 13:
            CPL_FITS_IS_UNIQUE_TWO(13);
            break;
        case 14:
            CPL_FITS_IS_UNIQUE_TWO(14);
            break;
        case 15:
            CPL_FITS_IS_UNIQUE_TWO(15);
            break;
        case 16:
            CPL_FITS_IS_UNIQUE_TWO(16);
            break;
        case 17:
            CPL_FITS_IS_UNIQUE_TWO(17);
            break;
        case 18:
            CPL_FITS_IS_UNIQUE_TWO(18);
            break;
        case 19:
            CPL_FITS_IS_UNIQUE_TWO(19);
            break;
        case 20:
            CPL_FITS_IS_UNIQUE_TWO(20);
            break;
        case 21:
            CPL_FITS_IS_UNIQUE_TWO(21);
            break;
        case 22:
            CPL_FITS_IS_UNIQUE_TWO(22);
            break;
        case 23:
            CPL_FITS_IS_UNIQUE_TWO(23);
            break;
        case 24:
            CPL_FITS_IS_UNIQUE_TWO(24);
            break;
        case 25:
            CPL_FITS_IS_UNIQUE_TWO(25);
            break;
        case 26:
            CPL_FITS_IS_UNIQUE_TWO(26);
            break;
        case 27:
            CPL_FITS_IS_UNIQUE_TWO(27);
            break;
        case 28:
            CPL_FITS_IS_UNIQUE_TWO(28);
            break;
        case 29:
            CPL_FITS_IS_UNIQUE_TWO(29);
            break;
        case 30:
            CPL_FITS_IS_UNIQUE_TWO(30);
            break;
        case 31:
            CPL_FITS_IS_UNIQUE_TWO(31);
            break;
        case 32:
            CPL_FITS_IS_UNIQUE_TWO(32);
            break;
        case 33:
            CPL_FITS_IS_UNIQUE_TWO(33);
            break;
        case 34:
            CPL_FITS_IS_UNIQUE_TWO(34);
            break;
        case 35:
            CPL_FITS_IS_UNIQUE_TWO(35);
            break;
        case 36:
            CPL_FITS_IS_UNIQUE_TWO(36);
            break;
        case 37:
            CPL_FITS_IS_UNIQUE_TWO(37);
            break;
        case 38:
            CPL_FITS_IS_UNIQUE_TWO(38);
            break;
        case 39:
            CPL_FITS_IS_UNIQUE_TWO(39);
            break;
        case 40:
            CPL_FITS_IS_UNIQUE_TWO(40);
            break;
        case 41:
            CPL_FITS_IS_UNIQUE_TWO(41);
            break;
        case 42:
            CPL_FITS_IS_UNIQUE_TWO(42);
            break;
        case 43:
            CPL_FITS_IS_UNIQUE_TWO(43);
            break;
        case 44:
            CPL_FITS_IS_UNIQUE_TWO(44);
            break;
        case 45:
            CPL_FITS_IS_UNIQUE_TWO(45);
            break;
        case 46:
            CPL_FITS_IS_UNIQUE_TWO(46);
            break;
        case 47:
            CPL_FITS_IS_UNIQUE_TWO(47);
            break;
        case 48:
            CPL_FITS_IS_UNIQUE_TWO(48);
            break;
        case 49:
            CPL_FITS_IS_UNIQUE_TWO(49);
            break;
        case 50:
            CPL_FITS_IS_UNIQUE_TWO(50);
            break;
        case 51:
            CPL_FITS_IS_UNIQUE_TWO(51);
            break;
        case 52:
            CPL_FITS_IS_UNIQUE_TWO(52);
            break;
        case 53:
            CPL_FITS_IS_UNIQUE_TWO(53);
            break;
        case 54:
            CPL_FITS_IS_UNIQUE_TWO(54);
            break;
        case 55:
            CPL_FITS_IS_UNIQUE_TWO(55);
            break;
        case 56:
            CPL_FITS_IS_UNIQUE_TWO(56);
            break;
        case 57:
            CPL_FITS_IS_UNIQUE_TWO(57);
            break;
        case 58:
            CPL_FITS_IS_UNIQUE_TWO(58);
            break;
        case 59:
            CPL_FITS_IS_UNIQUE_TWO(59);
            break;
        case 60:
            CPL_FITS_IS_UNIQUE_TWO(60);
            break;
        default: /* Don't create extra code for exceedingly rare, long keys */
            /* No point in distinguishing among keys too long to convert to FITS */
            CPL_FITS_IS_UNIQUE_TWO(CX_MIN(keylen, FLEN_KEYWORD - 1));
            break;
    }

    return 0;
}

/**@}*/
