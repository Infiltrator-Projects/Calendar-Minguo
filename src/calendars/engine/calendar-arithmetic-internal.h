/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 1993-2026 Shannon Smith
 *
 * Private family contracts for the compiled arithmetic calendar engine.
 *
 * These declarations are translation-unit boundaries only. The public
 * arithmetic API remains calendar-arithmetic.h; chronology-specific state
 * and algorithms stay owned by their compiled family modules. Native resource
 * calendars own their private chronology inside their executable module.
 */

#ifndef CALENDAR_CALENDAR_ARITHMETIC_INTERNAL_H
#define CALENDAR_CALENDAR_ARITHMETIC_INTERNAL_H

#include "calendar-engine-types.h"

G_BEGIN_DECLS

gint64 calendar_arithmetic_japanese_signed_year_from_fields(
    const CalendarCalendarFields *fields);
void calendar_arithmetic_japanese_set_year_for_gregorian(
    gint64 gregorian_year,
    CalendarCalendarFields *fields);
void calendar_arithmetic_japanese_fields_from_jdn(
    gint64 jdn,
    CalendarCalendarFields *fields);

gint64 calendar_arithmetic_coptic_signed_year_from_fields(
    CalendarCalendarMode mode,
    const CalendarCalendarFields *fields);
void calendar_arithmetic_coptic_set_signed_year(
    CalendarCalendarMode mode,
    gint64 signed_year,
    CalendarCalendarFields *fields);
gint calendar_arithmetic_coptic_month_length(gint64 year, gint month);
gint64 calendar_arithmetic_coptic_fields_to_jdn(
    CalendarCalendarMode mode, gint64 year, gint month, gint day);
gboolean calendar_arithmetic_coptic_fields_from_jdn(
    CalendarCalendarMode mode,
    gint64 jdn,
    CalendarCalendarFields *fields);

gint calendar_arithmetic_indian_month_length(gint64 year, gint month);
gint64 calendar_arithmetic_indian_fields_to_jdn(
    gint64 year, gint month, gint day);
void calendar_arithmetic_indian_fields_from_jdn(
    gint64 jdn,
    CalendarCalendarFields *fields);

gboolean calendar_arithmetic_islamic_fields_from_jdn(
    CalendarCalendarMode mode,
    gint64 jdn,
    CalendarCalendarFields *fields);
gint64 calendar_arithmetic_islamic_fields_to_jdn(
    CalendarCalendarMode mode,
    gint64 year,
    gint month,
    gint day);
gint calendar_arithmetic_islamic_month_length(
    CalendarCalendarMode mode,
    gint64 year,
    gint month);
gboolean calendar_arithmetic_islamic_year_supported(
    CalendarCalendarMode mode,
    gint64 year);

gint calendar_arithmetic_persian_month_length(gint64 year,
                                                gint month);
gint64 calendar_arithmetic_persian_fields_to_jdn(gint64 year,
                                                   gint month,
                                                   gint day);
gboolean calendar_arithmetic_persian_fields_from_jdn(
    gint64 jdn,
    CalendarCalendarFields *fields);

G_END_DECLS

#endif
