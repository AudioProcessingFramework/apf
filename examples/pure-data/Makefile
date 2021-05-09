# Makefile to generate externals for Puredata

cflags += \
       -DFLEXT_INLINE \
       -Iflext/source \
       -I.. \
       -I../.. \
       -std=c++14 \

class.sources = simpleprocessor~.cpp

datafiles = $(wildcard *-help.pd)

include pd-lib-builder/Makefile.pdlibbuilder
