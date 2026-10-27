/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 1993-2026 Shannon Smith
 *
 * ICU/CLDR calendar adapter.
 *
 * ICU owns the remaining delegated calendar conversion, era/lunisolar data
 * and locale-sensitive formatting. Common opens those calendars
 * in UTC and places the JDN at UTC noon.  Noon avoids any midnight
 * ambiguity while UTC ensures that host timezone and daylight-saving changes
 * cannot alter a date-only conversion.
 */


#include "icu-calendar.h"

#include <infiltratr/arithmetic.h>
#include <infiltratr/core.h>

#include "julian-day.h"

#include <math.h>

#include <unicode/ucal.h>
#include <unicode/udat.h>
#include <unicode/uloc.h>
#include <unicode/ustring.h>

#define MILLISECONDS_PER_DAY 86400000.0
/*
 * ICU's Gregorian calendar is hybrid by default: Julian before the 1582
 * cutover and Gregorian afterwards.  the "gregorian" provider is
 * explicitly proleptic, so place the ICU cutover far before the supported
 * civil range.  -8.64e15 ms is 100,000,000 days before the Unix epoch.
 */
#define PROLEPTIC_GREGORIAN_CUTOVER (-8.64e15)

static const UChar utc_zone[] = { 0x0055, 0x0054, 0x0043, 0 };

static gboolean
make_icu_locale(const gchar *calendar_keyword,
                gchar *locale,
                gint32 locale_capacity)
{
    UErrorCode status = U_ZERO_ERROR;

    if (calendar_keyword == NULL || *calendar_keyword == '\0' ||
        locale_capacity <= 0)
        return FALSE;

    /* Date and clock wording share the desktop's UI language, including a
     * LANGUAGE preference that differs from LC_TIME/ICU's process default. */
    const gchar *language = g_get_language_names()[0];
    if (infiltratr_string_equal(language, "C") ||
        infiltratr_string_equal(language, "POSIX"))
        language = "en_US";
    infiltratr_copy_string(locale, (size_t)locale_capacity, language);
    uloc_setKeywordValue("calendar",
                         calendar_keyword,
                         locale,
                         locale_capacity,
                         &status);
    return U_SUCCESS(status);
}

static UCalendar *
open_icu_calendar(const gchar *calendar_keyword,
                  UErrorCode *status)
{
    gchar locale[ULOC_FULLNAME_CAPACITY];

    if (!make_icu_locale(calendar_keyword, locale, G_N_ELEMENTS(locale)))
    {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    UCalendar *calendar =
        ucal_open(utc_zone, 3, locale, UCAL_DEFAULT, status);

    if (U_FAILURE(*status) || calendar == NULL)
        return NULL;

    if (infiltratr_string_equal(calendar_keyword, "gregorian"))
    {
        ucal_setGregorianChange(calendar,
                                PROLEPTIC_GREGORIAN_CUTOVER,
                                status);
        if (U_FAILURE(*status))
        {
            ucal_close(calendar);
            return NULL;
        }
    }

    return calendar;
}

/*
 * ICU dates span ±100 million days as milliseconds since Unix epoch.
 * Store the civil JDN at UTC noon: the upper allowed JDN is one day before
 * the maximum. Check before subtracting, casting, or calling ICU.
 */
#define ICU_MIN_DAY_OFFSET (-G_GINT64_CONSTANT(100000000))
#define ICU_MAX_DAY_OFFSET G_GINT64_CONSTANT(99999999)
#define ICU_MILLIS_ABSOLUTE_LIMIT 8640000000000000.0
static gboolean jdn_to_udate(gint64 jdn, UDate *out)
{
    if (!out ||
        jdn < CALENDAR_UNIX_EPOCH_JDN + ICU_MIN_DAY_OFFSET ||
        jdn > CALENDAR_UNIX_EPOCH_JDN + ICU_MAX_DAY_OFFSET)
        return FALSE;
    const gint64 days = jdn - CALENDAR_UNIX_EPOCH_JDN;
    *out = (UDate)days * MILLISECONDS_PER_DAY +
           MILLISECONDS_PER_DAY / 2.0;
    return TRUE;
}


static gchar *
utf16_to_utf8(const UChar *source,
              gint32 source_length)
{
    UErrorCode status = U_ZERO_ERROR;
    gint32 required = 0;
    gchar *result;

    u_strToUTF8(NULL, 0, &required, source, source_length, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status))
        return g_strdup("");

    status = U_ZERO_ERROR;
    size_t capacity = 0U;
    if (required < 0 ||
        !infiltratr_size_add_checked((size_t)required, 1U, &capacity) ||
        capacity > (size_t)G_MAXINT32)
    {
        return g_strdup("");
    }

    result = g_malloc(capacity);
    u_strToUTF8(result,
                (gint32)capacity,
                NULL,
                source,
                source_length,
                &status);
    if (U_FAILURE(status))
    {
        g_free(result);
        return g_strdup("");
    }

    return result;
}

static const gchar *
pattern_for_part(CalendarCalendarMode mode,
                 CalendarDatePart part)
{
    (void)mode;
    const gboolean era_calendar =
        TRUE;
    /* These lunisolar calendars name years in their native sixty-year cycle.
     * `r` would inject a Gregorian reference year; `y` alone is only an
     * unexplained 1..60 counter. ICU's `U` supplies the localized year name. */
    const gboolean cyclic_year =
        FALSE;
    const gboolean omit_era =
        FALSE;

    if (part == CALENDAR_DATE_PART_DAY)
        return "d";
    if (part == CALENDAR_DATE_PART_MONTH)
        return "LLLL";
    if (part == CALENDAR_DATE_PART_YEAR)
    {
        if (cyclic_year)
            return "U";
        return era_calendar ? "G y" : "y";
    }
    if (part == CALENDAR_DATE_PART_FULL)
    {
        if (cyclic_year)
            return "EEEE, d MMMM U";
        if (era_calendar)
            return "EEEE, d MMMM G y";
        return omit_era ? "EEEE, d MMMM y" : "EEEE, d MMMM y G";
    }

    if (cyclic_year)
        return "d MMMM U";
    if (era_calendar)
        return "d MMMM G y";
    return omit_era ? "d MMMM y" : "d MMMM y G";
}

gchar *
calendar_icu_format(CalendarCalendarMode format_profile,
                         const gchar *calendar_keyword,
                         gint64 jdn,
                         CalendarDatePart part)
{
    (void)format_profile;
    UErrorCode status = U_ZERO_ERROR;
    gchar locale[ULOC_FULLNAME_CAPACITY];
    UChar pattern_utf16[128];
    gint32 pattern_length;
    UDateFormat *formatter;
    UCalendar *calendar;
    UChar stack_buffer[256];
    UChar *output = stack_buffer;
    gint32 capacity = G_N_ELEMENTS(stack_buffer);
    gint32 length;
    gchar *result;
    const gchar *pattern = pattern_for_part(format_profile, part);
    UDate utc_noon;

    if (!jdn_to_udate(jdn, &utc_noon)) return g_strdup("");
    if (!make_icu_locale(calendar_keyword, locale, G_N_ELEMENTS(locale)))
        return g_strdup("");

    u_strFromUTF8(pattern_utf16,
                  G_N_ELEMENTS(pattern_utf16),
                  &pattern_length,
                  pattern,
                  -1,
                  &status);
    if (U_FAILURE(status))
        return g_strdup("");

    formatter = udat_open(UDAT_PATTERN,
                          UDAT_PATTERN,
                          locale,
                          utc_zone,
                          3,
                          pattern_utf16,
                          pattern_length,
                          &status);
    if (U_FAILURE(status) || formatter == NULL)
        return g_strdup("");

    /*
     * UDateFormat creates its own calendar, which would otherwise restore the
     * default 1582 Gregorian cutover.  Give formatting the exact same calendar
     * policy as field conversion and navigation.
     */
    calendar = open_icu_calendar(calendar_keyword, &status);
    if (U_FAILURE(status) || calendar == NULL)
    {
        udat_close(formatter);
        return g_strdup("");
    }
    udat_setCalendar(formatter, calendar);
    ucal_close(calendar);

    length = udat_format(formatter,
                         utc_noon,
                         output,
                         capacity,
                         NULL,
                         &status);
    if (status == U_BUFFER_OVERFLOW_ERROR)
    {
        size_t element_count = 0U;
        size_t allocation_bytes = 0U;

        status = U_ZERO_ERROR;
        if (length < 0 ||
            !infiltratr_size_add_checked(
                (size_t)length, 1U, &element_count) ||
            element_count > (size_t)G_MAXINT32 ||
            !infiltratr_size_multiply_checked(
                element_count, sizeof(UChar), &allocation_bytes))
        {
            udat_close(formatter);
            return g_strdup("");
        }

        capacity = (gint32)element_count;
        output = g_malloc(allocation_bytes);
        length = udat_format(formatter,
                             utc_noon,
                             output,
                             capacity,
                             NULL,
                             &status);
    }

    result = U_FAILURE(status) ? g_strdup("") :
             utf16_to_utf8(output, length);
    if (output != stack_buffer)
        g_free(output);
    udat_close(formatter);
    return result;
}


