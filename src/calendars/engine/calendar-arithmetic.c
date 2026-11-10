// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith
/*
 * Native Minguo chronology. The provider contains only one calendar mode;
 * its Gregorian month rules and era conversion share the same arithmetic.
 */
#include "calendar-arithmetic.h"
#include "calendar-arithmetic-internal.h"
#include "julian-day.h"

static gboolean supported(CalendarCalendarMode mode)
{
    return mode == CALENDAR_CALENDAR_MODE_MINGUO;
}

static gint64 signed_year(const CalendarCalendarFields *fields)
{
    return fields->auxiliary == 0
        ? calendar_i64_subtract_saturating(1912, fields->year)
        : calendar_i64_add_saturating(fields->year, 1911);
}

static void set_year(gint64 year, CalendarCalendarFields *fields)
{
    if (year >= 1912) {
        fields->year = calendar_i64_subtract_saturating(year, 1911);
        fields->auxiliary = 1;
    } else {
        fields->year = calendar_i64_subtract_saturating(1912, year);
        fields->auxiliary = 0;
    }
}

static gint64 to_jdn(const CalendarCalendarFields *fields)
{
    return calendar_gregorian_to_jdn(
        signed_year(fields), fields->month, fields->day);
}

gboolean calendar_arithmetic_fields_from_jdn(
    CalendarCalendarMode mode, gint64 jdn, CalendarCalendarFields *fields)
{
    gint year, month, day;
    g_return_val_if_fail(fields != NULL, FALSE);
    *fields = (CalendarCalendarFields){ 0 };
    if (!supported(mode)) return FALSE;
    calendar_jdn_to_gregorian(jdn, &year, &month, &day);
    fields->month = month;
    fields->day = day;
    fields->special = FALSE;
    set_year(year, fields);
    return TRUE;
}

gint64 calendar_arithmetic_month_start(CalendarCalendarMode mode, gint64 jdn)
{
    CalendarCalendarFields fields;
    if (!calendar_arithmetic_fields_from_jdn(mode, jdn, &fields))
        return G_MAXINT64;
    fields.day = 1;
    return to_jdn(&fields);
}

gint64 calendar_arithmetic_add_months(
    CalendarCalendarMode mode, gint64 jdn, gint amount)
{
    CalendarCalendarFields fields;
    gint64 serial, year;
    if (!calendar_arithmetic_fields_from_jdn(mode, jdn, &fields))
        return G_MAXINT64;
    year = signed_year(&fields);
    serial = calendar_i64_add_saturating(
        calendar_i64_add_saturating(
            calendar_i64_multiply_saturating(year, 12),
            fields.month - 1), amount);
    year = calendar_floor_divide(serial, 12);
    const gint month = (gint)calendar_positive_modulo(serial, 12) + 1;
    set_year(year, &fields);
    fields.month = month;
    fields.day = MIN(
        fields.day, calendar_gregorian_month_length(year, fields.month));
    return to_jdn(&fields);
}

gint64 calendar_arithmetic_add_years(
    CalendarCalendarMode mode, gint64 jdn, gint amount)
{
    CalendarCalendarFields fields;
    if (!calendar_arithmetic_fields_from_jdn(mode, jdn, &fields))
        return G_MAXINT64;
    const gint64 year = calendar_i64_add_saturating(signed_year(&fields), amount);
    set_year(year, &fields);
    fields.day = MIN(
        fields.day, calendar_gregorian_month_length(year, fields.month));
    return to_jdn(&fields);
}
