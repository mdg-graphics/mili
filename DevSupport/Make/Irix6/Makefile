#
# $Id$
#
# Makefile - This is the makefile for GRIZ
#
# Author:	Don Dovey
#               Lawrence Livermore National Laboratory
#               Jan 3 1992
#
# V2.0 update:	Doug Speck
#               Lawrence Livermore National Laboratory
#               March 11, 1996
#
# This version for CVS Griz devlopement module on SGI platforms
#
# Copyright (c) 1992, Regents of the University of California
# Copyright (c) 1996, Regents of the University of California
#
SHELL = /bin/sh
#

# Set name of griz binary appropriately. The name "griz" is used
# for the Griz shell script make target, below.
GRIZBIN = griz.sgi

# Set to location of GRIZ source. Use an absolute path if GRIZHOME
# is used to define GRIZBINLOC and HFONTLOC, below.
GRIZHOME = .

# Set to intended installed locations of GRIZ binary and futura.l 
# Hershey font. GRIZBINLOC defines the location for both the GRIZ
# binary and the "griz_start_text" announcements file. 
# The installation is not performed by this Makefile, rather these 
# these symbols are used to pass the appropriate paths into the
# GRIZ shell script.
GRIZBINLOC = ${GRIZHOME}
HFONTLOC = ${GRIZHOME}/Font

# SGI compile flags; add -mips2 if running on R4000, etc.
CFLAGS = -cckr -g -mips2 -32

# Minimal library set (not complete! See below.)
LIBS = -lXm -lXt -lX11 -lGLw -lGLU -lGL -lm -lPW

OURLIBS = \
	${GRIZHOME}/Lib/libhershey.a ${GRIZHOME}/Lib/libimage.a

OBJS = \
	viewer.o faces.o geometric.o mdg_mem.o mdg_in.o misc.o \
	interpret.o time.o show.o draw.o results.o iso_surface.o \
	gui.o contour.o video.o stress.o strain.o node.o \
	frame.o time_hist.o shape.o flow.o poly.o explode.o

all:		${GRIZBIN} griz

${GRIZBIN}:	${OBJS}
		${CC} ${CFLAGS} -o $@ ${OBJS} ${OURLIBS} ${LIBS}

#
# The griz shell script will always rebuild when the "griz" make
# target is evaluated.  It's cheap, and any changes to GRIZBINLOC,
# HFONTLOC, or GRIZBIN will then always be reflected in the script. 
# To eliminate this behavior, delete "force_rebuild" from the "griz"
# dependencies.
#
griz:	griz.in force_rebuild
	sed '/_grizhome_/ s/_grizhome_/'`echo ${GRIZBINLOC} | sed 's/\\//\\\\\//g'`'/' \
		griz.in \
	| sed '/_hfontlib_/ s/_hfontlib_/'`echo ${HFONTLOC} | sed 's/\\//\\\\\//g'`'/' \
	| sed '/_grizbin_/ s/_grizbin_/${GRIZBIN}/' \
	> $@;
	chmod 755 $@;

force_rebuild:

viewer.o:	viewer.h misc.h list.h mdg.h results.h geometric.h
faces.o:	viewer.h misc.h list.h mdg.h results.h geometric.h
interpret.o:	viewer.h misc.h list.h mdg.h results.h geometric.h draw.h
time.o:		viewer.h misc.h list.h mdg.h results.h geometric.h
show.o:		viewer.h misc.h list.h mdg.h results.h geometric.h
draw.o:		viewer.h misc.h list.h mdg.h results.h geometric.h draw.h image.h
geometric.o:	misc.h geometric.h
mdg_in.o:	misc.h mdg.h list.h results.h
mdg_mem.o:	misc.h mdg.h results.h
misc.o:		misc.h
results.o:	viewer.h misc.h list.h mdg.h results.h geometric.h
iso_surface.o:	viewer.h misc.h list.h mdg.h results.h geometric.h cell_cases.h
gui.o:		viewer.h misc.h list.h mdg.h results.h geometric.h draw.h
contour.o:	viewer.h misc.h list.h mdg.h results.h geometric.h
video.o:	
strain.o:       viewer.h misc.h list.h mdg.h results.h geometric.h
stress.o:       viewer.h misc.h list.h mdg.h results.h geometric.h
node.o:       	viewer.h misc.h list.h mdg.h results.h geometric.h
frame.o:       	viewer.h misc.h list.h mdg.h results.h geometric.h
time_hist.o:	viewer.h misc.h list.h mdg.h results.h geometric.h draw.h
shape.o:       	viewer.h misc.h list.h mdg.h results.h geometric.h
flow.o:       	viewer.h misc.h list.h mdg.h results.h geometric.h draw.h
poly.o:		viewer.h misc.h list.h mdg.h results.h geometric.h draw.h
explode.o:	viewer.h misc.h list.h mdg.h results.h geometric.h

image.h:
		ln -s Include/image.h image.h

clean:
	rm *.o
