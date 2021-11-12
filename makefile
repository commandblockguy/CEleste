# ----------------------------
# Makefile Options
# ----------------------------

PATH := /home/john/CEdev/bin:$(PATH)
SHELL := env PATH=$(PATH) /bin/bash

NAME = CELESTE
ICON = icon.png
DESCRIPTION = "Celeste Classic"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
