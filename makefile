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

CFLAGS = -Wall -Wextra -O3
CXXFLAGS = -Wall -Wextra -O3

# ----------------------------

include $(shell cedev-config --makefile)
