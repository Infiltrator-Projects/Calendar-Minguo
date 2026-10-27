// SPDX-License-Identifier: GPL-3.0-or-later
/* ABI adapter only: Common owns every civil coordinate calculation. */
#include "julian-day.h"
#include <infiltratr/temporal.h>

gint calendar_gregorian_month_length(gint64 y, gint m) { return infiltratr_temporal_gregorian_month_length(y, m); }

gint64 calendar_gregorian_to_jdn(gint64 y, gint m, gint d) { return infiltratr_temporal_gregorian_to_jdn(y, m, d); }
void calendar_jdn_to_gregorian(gint64 jdn, gint *y, gint *m, gint *d) { infiltratr_temporal_jdn_to_gregorian(jdn, y, m, d); }


