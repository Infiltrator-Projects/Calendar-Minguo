// SPDX-License-Identifier: GPL-3.0-or-later
/* Independently installed calendar. No Calendar application ABI is exported. */
#include <infiltratr/core.h>
#include <infiltratr/temporal_calendar.h>
#include "calendar-engine-types.h"
#include "calendar-arithmetic.h"
#include "calendar-custom.h"
#include "calendar-historical.h"
#include "calendar-swedish.h"
#include "icu-calendar.h"
#include "catalogue.h"
#include "julian-day.h"
#include <string.h>

typedef struct ResourceCalendar ResourceCalendar;
typedef gint64 (*ResourceNavigateFunc)(const ResourceCalendar *, gint64, gint);
struct ResourceCalendar {
    CalendarCalendarMode mode;
    const gchar *icu_keyword;
    gboolean (*fields_from_jdn)(const ResourceCalendar *,gint64,CalendarCalendarFields *);
    gchar *(*format)(const ResourceCalendar *,gint64,CalendarDatePart);
    gint64 (*period_start)(const ResourceCalendar *,gint64);
    ResourceNavigateFunc add_periods;
    ResourceNavigateFunc add_years;
};
static gchar *
icu_format(const ResourceCalendar *provider,
           gint64 jdn,
           CalendarDatePart part)
{
    const ResourceCalendar *builtin = provider;

    return calendar_icu_format(
        builtin->mode, builtin->icu_keyword, jdn, part);
}

static gboolean
arithmetic_fields(const ResourceCalendar *provider,
                  gint64 jdn,
                  CalendarCalendarFields *fields)
{
    const ResourceCalendar *builtin = provider;

    return calendar_arithmetic_fields_from_jdn(
        builtin->mode, jdn, fields);
}

static gint64
arithmetic_period_start(const ResourceCalendar *provider,
                        gint64 jdn)
{
    const ResourceCalendar *builtin = provider;

    return calendar_arithmetic_month_start(builtin->mode, jdn);
}

static gint64
arithmetic_add_periods(const ResourceCalendar *provider,
                       gint64 jdn,
                       gint amount)
{
    const ResourceCalendar *builtin = provider;

    return calendar_arithmetic_add_months(
        builtin->mode, jdn, amount);
}

static gint64
arithmetic_add_years(const ResourceCalendar *provider,
                     gint64 jdn,
                     gint amount)
{
    const ResourceCalendar *builtin = provider;

    return calendar_arithmetic_add_years(
        builtin->mode, jdn, amount);
}

#define RESOURCE_OPERATIONS arithmetic_fields, icu_format, arithmetic_period_start, arithmetic_add_periods, arithmetic_add_years
static const ResourceCalendar native = {
    RESOURCE_MODE, RESOURCE_KEYWORD, RESOURCE_OPERATIONS
};
/* The ICU-backed formatter and the arithmetic callbacks share this
 * UTC-noon domain. Reject before narrowing extreme JDNs into civil fields. */
static bool provider_jdn_supported(int64_t jdn)
{
    return jdn >= INT64_C(2440588) - INT64_C(100000000) &&
           jdn <= INT64_C(2440588) + INT64_C(99999999);
}

static bool fields(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn, InfiltratrTemporalCalendarFields *result)
{
    CalendarCalendarFields value;
    (void)self;
    if (result == NULL || !provider_jdn_supported(jdn) ||
        !native.fields_from_jdn(&native,jdn,&value))
        return false;
    *result = (InfiltratrTemporalCalendarFields){
        value.year, value.month, value.day, value.auxiliary, value.special != FALSE };
    return true;
}
static bool format(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn, InfiltratrTemporalCalendarDatePart part,
    char *buffer, size_t capacity, size_t *length)
{
    gchar *text;
    CalendarCalendarFields validated_fields;
    (void)self;
    if (buffer == NULL || capacity == 0U || !provider_jdn_supported(jdn) ||
        part < INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_DAY ||
        part > INFILTRATR_TEMPORAL_CALENDAR_DATE_PART_FULL ||
        !native.fields_from_jdn(&native, jdn, &validated_fields))
        return false;
    /* Field conversion owns supported chronology. Formatting must not ask
     * ICU to invent dates outside a bounded arithmetic provider's range. */
    text = native.format(&native, jdn, (CalendarDatePart)part);
    if (text == NULL || *text == '\0') { g_free(text); return false; }
    const size_t size = strlen(text);
    if (size >= capacity) { g_free(text); return false; }
    memcpy(buffer,text,size+1U);
    if (length != NULL) *length=size;
    g_free(text);
    return true;
}
static bool start(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn, int64_t *result)
{
    CalendarCalendarFields value;
    if (result == NULL || !fields(self,jdn,&(InfiltratrTemporalCalendarFields){0}))
        return false;
    const int64_t candidate=native.period_start(&native,jdn);
    if (!provider_jdn_supported(candidate) ||
        !native.fields_from_jdn(&native,candidate,&value)) return false;
    *result=candidate;
    return true;
}
static bool navigate(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn,int32_t amount,int64_t *result,ResourceNavigateFunc operation)
{
    CalendarCalendarFields value;
    if (result == NULL || !fields(self,jdn,&(InfiltratrTemporalCalendarFields){0}))
        return false;
    const int64_t candidate=operation(&native,jdn,amount);
    if (!provider_jdn_supported(candidate) ||
        !native.fields_from_jdn(&native,candidate,&value)) return false;
    *result=candidate;
    return true;
}
static bool periods(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn,int32_t amount,int64_t *result)
{ return navigate(self,jdn,amount,result,native.add_periods); }
static bool years(const InfiltratrTemporalCalendarProvider *self,
    int64_t jdn,int32_t amount,int64_t *result)
{ return navigate(self,jdn,amount,result,native.add_years); }
static const InfiltratrTemporalResourceMetadata metadata = {
    sizeof(InfiltratrTemporalResourceMetadata),
    INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    INFILTRATR_TEMPORAL_RESOURCE_CALENDAR,RESOURCE_ID,INFILTRATR_TEMPORAL_DISPLAY_NAME,false,false,false
};
static const InfiltratrTemporalCalendarProvider provider = {
    sizeof(InfiltratrTemporalCalendarProvider),
    INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI,RESOURCE_ID,
    fields,format,start,periods,years
};
#if defined(_WIN32)
#define RESOURCE_EXPORT __declspec(dllexport)
#else
#define RESOURCE_EXPORT __attribute__((visibility("default")))
#endif
RESOURCE_EXPORT const InfiltratrTemporalResourceMetadata *infiltratr_temporal_resource_metadata_v1(void);
RESOURCE_EXPORT const InfiltratrTemporalCalendarProvider *infiltratr_temporal_calendar_provider_v1(void);
RESOURCE_EXPORT const InfiltratrTemporalResourceMetadata *infiltratr_temporal_resource_metadata_v1(void) { return &metadata; }
RESOURCE_EXPORT const InfiltratrTemporalCalendarProvider *infiltratr_temporal_calendar_provider_v1(void) { return &provider; }
