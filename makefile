# ----------------------------
# Makefile Options
# ----------------------------

NAME = CELESTE
ICON = icon.png
DESCRIPTION = "Celeste Classic"
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
