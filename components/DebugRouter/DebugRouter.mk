#
# Copyright 2017, DornerWorks
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DORNERWORKS_BSD)
#

CURRENT_DIR := $(dir $(abspath $(lastword ${MAKEFILE_LIST})))

DebugRouter_CFILES := $(wildcard ${CURRENT_DIR}/src/*.c)
DebugRouter_HFILES := $(wildcard ${CURRENT_DIR}/include/*.h)
DebugRouter_LIBS := sel4camkes ethdrivers lwip sel4vspace
