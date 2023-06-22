/*
 Copyright (c) 2016, Lawrence Livermore National Security, LLC.
 Produced at the Lawrence Livermore National Laboratory. Written
 by Kevin Durrenberger: durrenberger1@llnl.gov. CODE-OCEC-16-056.
 All rights reserved.

 This file is part of Mili. For details, see <URL describing code
 and how to download source>.

 Please also read this link-- Our Notice and GNU Lesser General
 Public License.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License (as published by
 the Free Software Foundation) version 2.1 dated February 1999.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms
 and conditions of the GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

 * eprtf - Extended printf library.
 *
 *         Douglas E. Speck
 *         Lawrence Livermore National Laboratory
 *         Jan 23 1997
 *
 *         Eprtf consists of printf() family derivatives which support
 *         traditional formats plus the following:
 *
 *              %<col>t     Begin subsequent output at the column number
 *              %*t         specified by <col>, or the next argument
 *                          (which must be an integer) if * specifier used.
 *                          The specified column must be a number on [1, CMAX]
 *                          and, when output is to a stream, is counted RELATIVE
 *                          TO THE END OF THE LAST OUTPUT ON THE STREAM.  This
 *                          means that the column number will only be absolute
 *                          if the last character output on a stream prior to
 *                          the eprintf() or efprintf() call was a newline.
 *                          If the column number lies beyond the end of
 *                          previously translated text, the output is blank
 *                          filled up to the new output column prior to any
 *                          subsequent output.  Previously translated text can
 *                          be overwritten by appropriately specifying the
 *                          output column, but if subsequently translated text
 *                          is shorter than existing text, the line is NOT
 *                          truncated to the end of the new text.
 *
 *         Forms available are:
 *              eprintf() - Translate text to standard output.
 *              efprintf() - Translate text to specified stream.
 *              esprintf() - Translate text to memory.
 *
 * NOTES: With the following exceptions, the eprtf functions should behave
 *        just like their standard I/O library progenitors:
 *
 *        Only tab formats in the required initial format string argument will
 *        be interpreted.  Recursive evaluations of additional string arguments
 *        to find embedded tab-formats are not performed.
 *
 *        Translated output is limited to CMAX (eprtf.h) characters.
 *
 *        Without going into the nitty-gritty, an incorrect tab format may
 *        produce unintended output that is inconsistent with the unintended
 *        output produced by an erroneous regular format, but who really cares?

 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
 *                with Visit.
 *                See SCR #587.
 ************************************************************************
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <regex.h>
#else
#include "win32-regex.h"
#endif

#include "mili_enum.h"
#include "eprtf.h"

static char destbuf[CMAX];
static char *p_cur;
static int cur_len;
static va_list val;
#ifdef HAVE_EPRINT
static char *t_pattern = "%([0-9]+|[*])t";
static regex_t all_re;
static char *all_pattern = "%[0 -+#]*([0-9]*|[*])([.]([0-9]*|[*]))?[hlL]?[dioxXucsfeEgGpn%]";

#endif
static regex_t t_re;
static regmatch_t t_match[1];

#ifdef NOOPTERON
static regmatch_t all_match[1];
#endif

/*****************************************************************
 * TAG( translate_segment )
 *
 * Translate a segment of text using vsprintf.
 */
static void translate_segment(char *fmt)
{
    char *p_dest, *p_end;
    char sav[CMAX];
    int trans_len;

    /* Sanity check. */
    if ( *fmt == '\0' )
    {
        return;
    }

    /* Blank fill up to current output location if necessary. */
    if ( (int)(p_cur - destbuf) > cur_len )
    {
        for ( p_dest = destbuf + cur_len; p_dest < p_cur; *p_dest++ = ' ' )
            ;
        cur_len = (int)(p_cur - destbuf);
    }

    /* Save a copy of current translated text that may be overwritten. */
    for ( p_end = p_cur, p_dest = sav; p_end < destbuf + cur_len; *p_dest++ = *p_end++ )
        ;

    /* Translate text preceding tab format. */
    vsprintf(p_cur, fmt, val);

    /*
     * If translation inserted a NULL before previous end-of-string,
     * overwrite NULL with correct character from saved copy.
     */
    trans_len = strlen(destbuf);
    if ( trans_len < cur_len )
    {
        destbuf[trans_len] = sav[trans_len - ((int)(p_cur - destbuf))];
    }
    else
    {
        cur_len = trans_len;
    }
}

#ifdef NOOPTERON
/*****************************************************************
 * TAG( advance_va_list )
 *
 * Scan a format string and advance the variable argument list
 * appropriately for each item found.
 */
static void advance_va_list(char *fmt)
{
    char *p_src;
    char *p_argtype;
    int int_arg;
    long long_arg;
    double double_arg;
    void *ptr_arg;
    char *string_arg;

    p_src = fmt;
    while ( regexec(&all_re, p_src, (LONGLONG)1, all_match, 0) == 0 )
    {
        p_argtype = p_src + all_match[0].rm_eo - 1;

        p_src += all_match[0].rm_eo;

        switch ( *p_argtype )
        {
            case 's':
            case 'p':
            case 'n':
                ptr_arg = va_arg(val, void *);
                break;

            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
            case 'u':
            case 'c':
                if ( *(p_argtype - 1) != 'l' )
                {
                    int_arg = va_arg(val, int);
                }
                else
                {
                    long_arg = va_arg(val, long);
                }
                break;

            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                double_arg = va_arg(val, double);
                break;

            case '%':
                /* No conversion for percent. */
                break;

            default:
                fprintf(stderr, "libeprtf: error matching conversion expression!\n");
                return;
        }
    }

    return;
}
#endif

#ifdef HAVE_EPRINT
/*****************************************************************
 * TAG( translate_string ) LOCAL
 *
 * Translate to a string the arguments in a variable argument list
 * by subdividing the list into non-tab-format segments (processed
 * by vsprintf) and tab-formats (processed locally).
 */
static int translate_string(char *fmt)
{
    char tmp[CMAX], colbuf[8];
    char *p_src, *p_last, *p_dest, *p_copy, *bound, *p_tabfmt, *p_test;
    int col;
    int no_tab;

    /* Search for a tab format in the input format string. */
    p_src = p_last = fmt;
    while ( regexec(&t_re, p_src, (LONGLONG)1, t_match, 0) == 0 )
    {
        /* Locate the first character ("%") of the tab format. */
        p_tabfmt = p_src + t_match[0].rm_so;

        /* If there are multiple %'s, it may not really be a tab format. */
        if ( p_tabfmt > fmt && *(p_tabfmt - 1) == '%' )
        {
            for ( p_test = p_tabfmt - 2, no_tab = 1; *p_test == '%'; no_tab ^= 0x1, p_test-- )
                ;
            if ( no_tab )
            {
                p_src += t_match[0].rm_eo;
                continue;
            }
        }

        /* Copy text preceding tab format and terminate string. */
        for ( p_dest = tmp, p_copy = p_last; p_copy < p_tabfmt; *p_dest++ = *p_copy++ )
            ;
        *p_dest = '\0';

        /*
         * Process the segment.
         */

        translate_segment(tmp);
#ifdef NOOPTERON
        advance_va_list(tmp);
#endif
        /*
         * Process the tab format.
         */

        /* Get new output column. */
        if ( p_copy[1] == '*' )
        {
            /*
             * Variable format.  Extract the column number from the next
             * parameter and advance the variable arg list.
             */
            col = va_arg(val, int);
        }
        else
        {
            /* Explicit format.  Copy numeric text and convert to int. */
            for ( p_dest = colbuf, p_copy++, bound = p_src + t_match[0].rm_eo - 1; p_copy < bound;
                  *p_dest++ = *p_copy++ )
                ;
            *p_dest = '\0';
            col = atoi(colbuf);
        }

        /*
         * Update index pointers.
         */

        if ( col <= 0 || col > CMAX )
        {
            return -1;
        }
        p_cur = destbuf + col - 1;
        p_src += t_match[0].rm_eo;
        p_last = p_src;
    }

    /* No (remaining) tab formats.  Process remaining text, if any. */
    translate_segment(p_last);

    return cur_len;
}

#endif

/*****************************************************************
 * TAG( eprintf )
 *
 * Extended printf.
 */
Return_value eprintf(int *num_written, char *format_spec, ...)
{
#ifdef HAVE_EPRINT
    int status;

    *num_written = 0;

    /* Sanity check. */
    if ( format_spec == NULL || *format_spec == '\0' )
    {
        return OK;
    }

    /* Initialize output buffer and access pointers. */
    memset(destbuf, (int)'\0', CMAX);
    p_cur = destbuf;
    cur_len = 0;

    /* Compile the regular expressions for the tab and regular formats. */
    status = regcomp(&t_re, t_pattern, REG_EXTENDED);
    if ( status != 0 )
    {
        return EPRINTF_FAILED;
    }
    status = regcomp(&all_re, all_pattern, REG_EXTENDED);
    if ( status != 0 )
    {
        return EPRINTF_FAILED;
    }

    /* Initialize argument list. */
    va_start(val, format_spec);

    /* Do the work. */
    status = translate_string(format_spec);

    va_end(val);

    regfree(&t_re);

    if ( status < 0 )
    {
        return EPRINTF_FAILED;
    }

    *num_written = (int)fwrite(destbuf, 1, (LONGLONG)status, stdout);
    if ( *num_written != status )
    {
        return EPRINTF_FAILED;
    }
    else
    {
        return OK;
    }
#else
    return OK;
#endif
}

/*****************************************************************
 * TAG( efprintf )
 *
 * Extended fprintf.
 */
Return_value efprintf(FILE *p_f, char *format_spec, ...)
{
#ifdef HAVE_EPRINT
    int status;

    /* Sanity check. */
    if ( format_spec == NULL || *format_spec == '\0' )
    {
        return OK;
    }

    /* Initialize output buffer and access pointers. */
    memset(destbuf, (int)'\0', CMAX);
    p_cur = destbuf;
    cur_len = 0;

    /* Compile the regular expressions for the tab and regular formats. */
    status = regcomp(&t_re, t_pattern, REG_EXTENDED);
    if ( status != 0 )
    {
        return EFPRINTF_FAILED;
    }
    status = regcomp(&all_re, all_pattern, REG_EXTENDED);
    if ( status != 0 )
    {
        return EFPRINTF_FAILED;
    }

    /* Initialize argument list. */
    va_start(val, format_spec);

    /* Do the work. */
    status = translate_string(format_spec);

    va_end(val);

    regfree(&t_re);

    if ( status < 0 )
    {
        return EFPRINTF_FAILED;
    }

    if ( fwrite(destbuf, 1, (LONGLONG)status, p_f) != (LONGLONG)status )
    {
        return EFPRINTF_FAILED;
    }
    else
    {
        return OK;
    }
#else
    return OK;
#endif
}

/*****************************************************************
 * TAG( esprintf )
 *
 * Extended sprintf.
 */
Return_value esprintf(char *p_output, char *format_spec, ...)
{
#ifdef HAVE_EPRINT
    int status;

    /* Sanity check. */
    if ( format_spec == NULL || *format_spec == '\0' )
    {
        return OK;
    }

    /* Initialize output buffer and access pointers. */
    memset(destbuf, (int)'\0', CMAX);
    p_cur = destbuf;
    cur_len = 0;

    /* Compile the regular expressions for the tab and regular formats. */
    status = regcomp(&t_re, t_pattern, REG_EXTENDED);
    if ( status != 0 )
    {
        return ESPRINTF_FAILED;
    }
    status = regcomp(&all_re, all_pattern, REG_EXTENDED);
    if ( status != 0 )
    {
        return ESPRINTF_FAILED;
    }

    /* Initialize argument list. */
    va_start(val, format_spec);

    /* Do the work. */
    status = translate_string(format_spec);

    va_end(val);

    regfree(&t_re);

    if ( status < 0 )
    {
        return ESPRINTF_FAILED;
    }

    memcpy(p_output, destbuf, status);
    p_output[status] = '\0';

    return OK;
#else
    return OK;
#endif
}
