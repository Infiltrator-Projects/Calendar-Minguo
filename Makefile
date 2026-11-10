# SPDX-License-Identifier: GPL-3.0-or-later
# Clock-specific logic stays local; Common owns generic code and ABI.
.DEFAULT_GOAL := all
CC ?= cc
PKG_CONFIG ?= pkg-config
BUILD_DIR ?= build
RESOURCE_NAME := Republic of China Minguo Calendar
COMMON_DIR := common
COMMON_REVISION := a99ef676b0e43c7e645c81756e3fd38be0a54406
COMMON_VERSION := 1.19.81
COMMON_BUILD_DIR := $(abspath $(BUILD_DIR))/common
COMMON_ARCHIVE := $(COMMON_BUILD_DIR)/libinfiltratr-common.a
DEPENDENCIES := glib-2.0 icu-i18n
SOURCES := $(shell find src -type f -name '*.c' | LC_ALL=C sort)
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/obj/%.o,$(SOURCES))
TARGET := $(BUILD_DIR)/$(RESOURCE_NAME).so
TARGET_STAMP := $(BUILD_DIR)/.provider-built
ifneq ($(strip $(DEPENDENCIES)),)
DEP_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(DEPENDENCIES))
endif
ifneq ($(filter glib-2.0,$(DEPENDENCIES)),)
DEP_LIBS := $(shell $(PKG_CONFIG) --libs glib-2.0)
endif
CPPFLAGS += -I$(COMMON_DIR)/include -Iinclude -Isrc -Isrc/support -Isrc/calendars -Isrc/calendars/engine -Isrc/clocks \
    -include include/provider-config.h -DGETTEXT_PACKAGE=\"infiltrator-calendar-minguo\" \
    -DU_DISABLE_RENAMING=1 -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_74 \
    -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_74
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections \
    -Wall -Wextra -Wpedantic -Werror -Wformat=2 -Wshadow -Wstrict-prototypes \
    -Wmissing-prototypes -fstack-protector-strong -fno-common \
    -ffile-prefix-map=$(CURDIR)=. -fdebug-prefix-map=$(CURDIR)=.
LDLIBS += $(DEP_LIBS) -lm -ldl -pthread
.PHONY: common-check
common-check:
	@if test ! -f common/VERSION; then git submodule update --init --depth 1 -- common; fi
	@grep -qx "$(COMMON_VERSION)" common/VERSION
	@if test -e common/.git; then git -C common rev-parse HEAD | grep -qx "$(COMMON_REVISION)"; fi
	@test ! -d include/infiltratr || { echo "Copied Common headers forbidden" >&2; exit 1; }
	@test ! -f src/arithmetic.c -a ! -f src/core.c -a ! -f src/dynlib.c -a ! -f src/temporal.c || { echo "Private Common sources forbidden" >&2; exit 1; }
$(COMMON_ARCHIVE): Makefile | common-check
	@mkdir -p "$(COMMON_BUILD_DIR)"
	$(MAKE) -C common CC="$(CC)" BUILD_DIR="$(COMMON_BUILD_DIR)" CFLAGS="$(CFLAGS) -Wno-missing-profile" all
$(BUILD_DIR)/obj/%.o: src/%.c Makefile | common-check
	@mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEP_CFLAGS) -MMD -MP -c "$<" -o "$@"
.PHONY: all check audit-authorities clean
all: $(TARGET_STAMP)
$(TARGET_STAMP): $(OBJECTS) $(COMMON_ARCHIVE) exports.map Makefile
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEP_CFLAGS) -shared $(OBJECTS) $(COMMON_ARCHIVE) \
	    -Wl,--version-script=exports.map -Wl,-z,defs -Wl,--gc-sections \
	    $(LDFLAGS) -o "$(TARGET)" $(LDLIBS)
	@touch "$@"
check: $(TARGET_STAMP)
	bash common/calendar-tools/check.sh "$(TARGET)"
audit-authorities: $(TARGET_STAMP)
	bash common/calendar-tools/check.sh "$(TARGET)" --authorities
-include $(OBJECTS:.o=.d)
clean:
	rm -rf "$(BUILD_DIR)"

.PHONY: release deb native-deb
release:
	bash common/calendar-tools/build.sh release
deb:
	bash common/calendar-tools/build.sh deb --profile portable
native-deb:
	bash common/calendar-tools/build.sh deb --profile aggressive
