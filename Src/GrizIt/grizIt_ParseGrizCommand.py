#
#
############################################################################
# grizIt_ParseGrizCommand.py - Parses all Griz CLI input and calls the
# Grizit Python module to implement the appropriate Visit functions.
#
# Ivan R. Corey
# Lawrence Livermore National Laboratory
# May 07, 2010
#
#
############################################################################
# Modifications:
# I. R. Corey - May 07, 2010: Created.
#
############################################################################
#
import pdb;

import os, sys, re, bisect, math

from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *

from visit import *

import grizIt_Session as GS
import traceback
import grizIt_VisitInfo

from PySide.QtCore import *

# Note from Sara:
#   To keep track of which functions come from which files, I will import
#   the whole module so that I have to explicitly call the function. This
#   will make it easier to organize for debugging purposes.

import grizIt_GuiMain as GUI

import grizIt_GrizColormapCommands as CMAP
import grizIt_GrizExecCommands as EC
import grizIt_GrizHelpCommands as HELP
import grizIt_GrizInteractionCommands as IC
import grizIt_GrizIndepSurfaceCommands as SURF
import grizIt_GrizMtlManagerCommands as MTL
import grizIt_GrizMeshControlCommands as MC
import grizIt_GrizMiscCommands as MISC
import grizIt_GrizMousePickCommands as MPICK
import grizIt_GrizOptRenderingCommands as OR #that's an 'oh,' not a zero
import grizIt_GrizOutputCommands as OC
import grizIt_GrizParticleCommands as PART
import grizIt_GrizQueryCommands as QC
import grizIt_GrizResultsCommands as RC
import grizIt_GrizStateTimeCommands as ST
import grizIt_GrizTHCommands as TH
import grizIt_GrizTractionCommands as TC
import grizIt_GrizAddtlCommands as AC

import grizIt_GrizViewCommands as VIEW
import grizIt_GrizVideoCommands as VID
import grizIt_GrizVisCommands as VIS
import grizIt_GrizViewingModeCommands as VM
import grizIt_GuiMain
import grizIt_TestSuite as TS
import grizIt_VisitCommands as VISIT
import grizIt_VisitInfo as VISITINFO

import grizIt_CommandFactory as CF
import grizIt_GrizOnOffFactory as OOF
import grizIt_GrizSwitchFactory as SF
import grizIt_GrizSurfaceFactory as SUF
import grizIt_GrizSetColorFactory as SCF

 
session = GS.session
env     = GS.env
log     = GS.CmdLog

#***********************************************************************************************************************
class GRIZ_null(object):
    def __init__(self):
        pass
    def do(self):
         return
    #end def
#**********************************************************************


VAR = 100 # the max number of arguments. For functions with variable number of args.


GrizCommands = {

# View Commands (File=grizIt_GrizViewCommands.py)
#   Keyword               Function                   MinArgs MaxArgs   Help Text
    "rx":                (VIEW.rx,                   1,      1,        "Rotate-x: rx angle"),
    "ry":   	         (VIEW.ry,                   1,      1,        "Rotate-y: ry angle"),
    "rz":   	         (VIEW.rz,                   1,      1,        "Rotate-z: rz angle"),
    "tx":		 (VIEW.tx,                   1,      1,        "Translate-x: tx distance"),
    "ty":   	         (VIEW.ty,                   1,      1,        "Translate-y: ty distancd"),
    "tz":   	         (VIEW.tz,                   1,      1,        "Translate-z: tz distance"),

    "scale":   	         (VIEW.scale,                1,      1,        "Scale view: scale f"),
    "scalax":   	 (VIEW.scalax,               3,      3,        "Scale axis independently: scalax x y z"),

    "rview":   	         (VIEW.rview,                0,      0,        "Reset view: rview"),
    "rnf":   	         (VIEW.rnf,                  0,      0,        "Reset near far planes: rnf"),
    "vcent":   	         (VIEW.vcent,                1,      3,        "Center view: vcent <x> <y> <z> | vcent n <node number> | vcent hi | vcent {on | off"),
    "zf":		 (VIEW.zf,                   1,      1,        "Zoom forward: zf scale"),
    "zb":		 (VIEW.zb,                   1,      1,        "Zoom back: zb scale "),


# State and Time Commands (File=grizIt_GrizStateTimeCommands.py)
    "minst":   	         (ST.minst,                1,      1,         "Ignore states before n: minst n"),
    "maxst":   	         (ST.maxst,                1,      1,         "Ignore states after n: maxst n"),
    "stride":   	 (ST.stride,               1,      1,         "Set state stride to n: stride n"),
    "state":             (ST.state,                1,      1,         "Go to state n: state n"),
    "f":   	         (ST.f,                    0,      0,         "Go to first state: f"),
    "p":   	         (ST.p,                    0,      0,         "Go to previous state: p"),
    "l":   	         (ST.l,                    0,      0,         "Go to last state: l"),
    "lts":   	         (ST.lts,                  0,      0,         "List state times: lts"),
    "n":                 (ST.n,                    0,      0,         "Go to the next state"),
    "time":   	         (ST.time,                 1,      1,         "Go to time n"),
    "anim":   	         (ST.anim,                 0,      3,         "Animate states in database: anim | anim frames | anim frames time-start time-end"),
    "stopan":   	 (ST.stopan,               0,      0,         "Stop animation: stopan"),
    "animc":   	         (ST.animc,                0,      0,         "Continue animation: animc"),

    "resetimg":   	 (ST.resetimg,             0,      0,         "Specify auto-image rootfile and type: resetimg"),
    
# Executive Commands (File=grizIt_GrizExecCommands.py)
    "load":		 (EC.load,                 1,      1,         "Load a new database: load database"),
    "exit":		 (EC.quit,                 0,      0,         "Exit GrizIt: exit"),
    "quit":		 (EC.quit,                 0,      0,         "Exit GrizIt: quit"),
    "on":		 (OOF.on,                  1,      1,         "Turn on the specified flag: on flag"),
    "off":		 (OOF.off,                 1,      1,         "Turn off the specified flag: off flag"),
    "info":		 (EC.info,                 0,      0,         "Show state, mesh, and view info: info"),
    "r":		 (EC.r,                    0,      0,         "Repeat last command: r | repeat"),
    "repeat":		 (EC.r,                    0,      0,         "Repeat last command: r | repeat"),
    "switch":            (SF.switch,               1,      1,         "Switch Factory"),
    "sw":                (SF.switch,               1,      1,         "Switch Factory"),
    "alias":		 (EC.alias,                2,      2,         "Create alias for command-string: alias newcom command-string"),
    "exec":		 (EC.Exec,                 1,      VAR,       "Execute Unix command: exec command"),
    "commands":          (EC.commands,             0,      0,         "Prints all implemented commands with help text"),
    "test":              (EC.run_tests,            0,      0,         "Runs tests from test file"),

# Results Commands (File=grizIt_GrizResultsCommands.py)
    "show":              (RC.show,                 1,      1,         "Display a result variable: show result"),
    "rzero":             (RC.rzero,                1,      1,         "Set result zero tolerance: rzero value"),
    "rmin":              (RC.rmin,                 1,      1,         "Set result min value: rmin value"),
    "rmax":              (RC.rmax,                 1,      1,         "Set result max value: rmax value"),
    "clrthr":            (RC.clrthr,               0,      0,         "Clear rzero, rmin, and rmax thresholds: clrthr"),
    "refstate":          (RC.refstate,             1,      1,         "Set nodal displacement reference state: refstate n"),
    "dirv":		 (RC.dirv, 		   3,	   3,	      "Specify a global direction vector"),
    "dir3n":		 (RC.dir3n,		   3,	   3,	      "Specify a global direction vector via three nodes"),
    "dir3p":		 (RC.dir3p,		   9,	   9,	      "Specify a global direction vector via three points"),
    "globmm":		 (RC.globmm,		   0,	   0,	      "Find global min/max for current result"),
    "resetmm":		 (RC.resetmm,		   0,	   0,	      "Reset global min/max for current result and clear cache"),
 
    "clrconv":		 (RC.clrconv,		   0,	   0,	      "Turn off units conversion; reset parameters to defaults"),
    "pref":		 (RC.pref,		   1,	   1,	      "Set reference pressure for 'pint' result calculations"),
    "dia":		 (RC.dia,		   1,	   1,	      "Set beam diameter for beam strain calculation"),
    "ym":		 (RC.ym,		   1,	   1,	      "Set Young's Modulus for beam strain calculation"),
    
# Meshless Particle Commands (File=grizIt_GrizParticleCommands.py)
    "pres":              (PART.pres,               1,      1,         "Set particle render res."),
    "pscale":            (PART.pscale,             1,      1,         "Set particle size"),
    "fres":		 (PART.fres,	   	   1,      1,	      "Set the rendering resolution for free nodes. Enter a value between 2 and 10"),
    "fscale":	 	 (PART.fscale,		   1,	   1,	      "Set the rendering size for free nodes. Enter a value between 1.0 and 10.0"),


# Optional Rendered Info Commands *imported as OR
    # (there are none here because they are all on/off commands,
    # so they are in the on/off factory: grizIt_GrizOnOffFactory.py)


# Mesh Control Commands (File=grizIt_GrizMeshControlCommands.py)
    "hilite":           (MC.hilite,              2,      2,      "Toggle hilite on specified object: hilite class_name n"),
    "clrhil":           (MC.clrhil,              0,      0,      "Clear hilite: clrhil"),
    "select":		(MC.select,	         2,	VAR,	"Toggle selection state of specified objects"),
    "clrsel":		(MC.clrsel,	         0,	VAR,	"Clear object selection(s)"),
    "exclude":		(MC.exclude,	         1,	VAR,	"Combined 'invis' and 'disable' on specified materials"),
    "include":		(MC.include,	         1,	VAR,	"Combined 'vis' and 'enable' on specified materials"),
    "disable":		(MC.disable,	         1,	VAR,	"Disable result display on specified materials"),
    "enable":		(MC.enable,	         1,	VAR,	"Enable result display on specified materials"),
    "invis":		(MC.invis,	         1,	VAR,	"Make specified materials invisible"),
    "vis":		(MC.vis,	         1,	VAR,	"Make specified materials visible"),
    "dscal":		(MC.dscal,	         1,	1,	"Scale nodal displacements"),
    "descalx":		(MC.dscalx,	         1,	1,	"Scale nodal displacements along axis directions"),
    "dscaly":		(MC.dscaly,	         1,	1,	"Scale nodal displacements along axis directions"),
    "dscalz":		(MC.dscalz,	         1,	1,	"Scale nodal displacements along axis directions"),
    
  
# Time History (TH) Commands (File=grizIt_GrizTHCommands.py)
   "plot":		(TH.plot,		0,	VAR,	"Plot time series curves"),
   "oplot":		(TH.oplot,		0,	VAR,	"Plot operation time series curves"),
   "outth":		(TH.outth,		1,	VAR,	"Write 'plot' or 'plot' time series data to text file"),
   "gather":		(TH.gather,		0,	VAR,	"Gather 'plot' or 'plot' time series into memory"),
   "delth":		(TH.delth,		0,	VAR,	"Delete time series"),
   "glyphqty":		(TH.glyphqty,		1,	1,	"Set approximate per-curve glyph qty"),
   "glyphscl":		(TH.glyphscl,		1,	1,	"Set size scale coefficient for glyphs"),
   "mmloc":		(TH.mmloc,		1,	1,	"Set plot box ymin/ymax location: mmloc {ul | ll | lr | ur}"),
   "timhis":		(TH.timhis,		0,	0,	"Plot time series curves using defaults"),

# Output Commands (File=grizIt_GrizOutputCommands.py)
    "outps":            (OC.outps,            1,      1,        "Output render window in Postscript format: outps filename"),
    "outjpeg":          (OC.outjpeg,          1,      1,        "Output render window in JPEG format: outjpeg filename"),
    "outrgb":           (OC.outrgb,           1,      1,        "Output render window in RGB format: outrgb filename"),
    "jpegqual":         (OC.jpegqual,         1,      1,        "Set JPEG image quality: jpegqual n (1 <= n <= 100"),
    "outrgba":		(OC.outrgba,          1,      1,	"Save rendered image to SGI Image file with alpha data"),
    "outpng":		(OC.outpng,	      1,      1,	"Save rendered image to PNG file"),
    "outmm":		(OC.outmm,	      1,      VAR,	"Create text report of result min/max values per state, broken out by material"),
    "outobj":		(OC.outobj,	      1,      1,	"Save current mesh and result data to Wavefront file"),
    "outview":		(OC.outview,	      1,      1,	"Save current view transformation as GRIZ command file"),
    "outhid":		(OC.outhid,           1,      1,	"Save current mesh polygon data to HIDDEN file"),
    "outth":		(OC.outth,	      1,      1,	"Save time history as text"),
    "outvec":		(OC.outvec,           1,      1,	"Save vector components as text"),
    "outpt":		(OC.outpt,	      1,      1,	"Save particle traces as text"),

# Query Commands (File=grizIt_GrizQueryCommands.py)
    "tell":             (QC.tell,             1,      4,        "General information query interface: tell report..."),
    "info":		(QC.info,	      0,      0,	"Alias for 'tell info class view'"),
    "tellmm":		(QC.tellmm,	      0,      VAR,      "Alias for 'tell mm...'"),
    "telliso":		(QC.telliso,          0,      0,	"Alias for 'tell iso'"),
    "tellpos":		(QC.tellpos,	      2,      2,	"Alias for 'tell pos'"),
    "tellem":		(QC.tellem,	      0,      0,	"Alias for 'tell em'"),


# Traction Commands (File=grizIt_GrizTractionCommands.py)
    "surface":	        (SUF.surface,	      6,	6,	"Surface Factroy"),
    "traction":	        (TC.traction,	      1,	1,	"Compute traction force and moment vectors"),

# Visualization Commands (File=grizIt_GrizVisCommands.py)
   "cutpln":		(VIS.cutpln,	      6,	7,	"Define a cutting plane"),# I put 7...but it should just be 6. Perhaps the specifications are wrong in the parsecommand function?
   "cutrpln":		(VIS.cutrpln,	      6,	6,	"Define a relative cutting plane"),
   "clrcut":		(VIS.clrcut,	      0,	0,	"Clear all cut planes"),
   "conwid":		(VIS.conwid,	      1,	1,	"Set isocontour line width in pixels"),
   "ison":		(VIS.ison,	      1,	1,	"Create ne evenly-spaced isovalues"),
   "isop":		(VIS.isop,            1,	1,	"Add an isovalue at v percent fo result min/max delta"),
   "isov":		(VIS.isov,	      1,	1,	"Add an isovalue at result value v"),
   "clriso":		(VIS.clriso,	      0,	0,	"Clear all isovalues"),
   "vgrid1":		(VIS.vgrid1,	      7,	7,	"Add vector grid points along a line"),
   "vgrid2":		(VIS.vgrid2,	      8,	8,	"Add vector grid points on axis-aligned plane"),
   "vgrid3":		(VIS.vgrid3,	      9,	9,	"Add vector grid points on axis-aligned volume"),
   "clrvgr":		(VIS.clrvgr,	      0,	0,	"Clear all vector grid points"),
   "veccm":		(VIS.veccm,	      0,	0,	"Colormap vectors by magnitude"),
   "vecscl":		(VIS.vecscl,	      1,	1,	"Scale vector lengths"),
   "vhdscl":		(VIS.vhdscl,	      1,	1,	"Scale 2D vector arrowheads"),
   "outvec":		(VIS.outvec,          1,	1,	"Save vector positions and components as text"),
   "ptrace":		(VIS.ptrace,	      0,	VAR,	"Create particle traces, replacing existing traces"),
   "aptrace":		(VIS.aptrace,	      0,	VAR,	"Create particle traces, augmenting existing traces"),
   "ptstat":		(VIS.ptstat,	      0,	VAR,	"Create instantaneous particle traces at time t_0 or state = n"),
   "prake":		(VIS.prake,	      1,	VAR,	"Create particle rake"),
   "clrpar":		(VIS.clrpar,	      0,	0,	"Create particle rake(s) from specifications in file: prake n p_1x p_1y p_1z p_2y p_2x p_2z [r g b] | Clear particle trace positions: prake filename"),
   "ptlim":		(VIS.ptlim,	      1,	1,	"Limit length of particle traces to n most recent steps"),
   "ptwid":		(VIS.ptwid,	      1,	1,	"Set particle trace width"),
   "ptdis":		(VIS.ptdis,	      1,	VAR,	"Disable particle traces in specified materials"),
   "outpt":		(VIS.outpt,	      1,	1,	"Save particle traces as text"),
   "inpt":		(VIS.inpt,	      1,	1,	"Read in particle traces from file"),


# Independent Surface commands (File=grizIt_GrizIndepSurfaceCommands.py)
   "inref":		(SURF.inref,	      1,	1,	"Read reference surface definition file"),
   "outref":		(SURF.outref,	      1,	VAR,	"Write current external hex faces as reference surface file"),
   "clrref":		(SURF.clrref,	      0,	0,	"Clear reference surfaces"),
   "inslp":		(SURF.inslp,	      1,	1,	"Read SLP-format external surface definition file"),


# Interaction History Commands (File=grizIt_GrizInteractionCommands.py)
   "savhis":		(IC.savhis,		1,	1,	"Begin saving commands to file"),
   "endhis":		(IC.endhis,		0,	0,	"Close command history file"),
   "rdhis":		(IC.rdhis,		1,	1,	"Read command history file and execute commands"),
   "pause":		(IC.pause,		1,	1,	"Pause for approximately  seconds"),
   "echo":		(IC.echo,		1,	VAR,	"Print string to history"),
   "loop":		(IC.loop,		1,	1,	"Loop endlessly over commands in file"),
   "savtxt":		(IC.savtxt,		1,	1,	"Begin saving feedback window text to file"),
   "endtxt":		(IC.endtxt,		0,	0,	"Stop saving feedback text and close file"),
   "savhislog":         (IC.savhislog,          1,      1,      "Save the entire history up to that point to a file"),
   "savesg":            (IC.savesg,             1,      1,      "Save global session data to a file"),
   "savesl":            (IC.savesl,             1,      1,      "Save local (plot) session data to a file"),
   "loadsg":            (IC.loadsg,             1,      1,      "Load global session data from a file"),
   "loadsl":            (IC.loadsl,             1,      1,      "Load local (plot) session data from a file"),

# Colormap Commands (File=grizIt_GrizColormapCommands,py)
   "ldmap":             (CMAP.ldmap,            1,      VAR,    "Load a Griz Colormap from a file"),
   "posmap":            (CMAP.posmap,           1,      4,      "Position colormap in the display"),
   "hotmap":            (CMAP.hotmap,           1,      1,      "Load default hot-cold colormap"),
   "grmap":             (CMAP.grmap,            0,      0,      "Load greyscale colormap"),
   "igrmap":            (CMAP.igmap,            0,      0,      "Load inverse greyscale colormap"),
   "invmap":            (CMAP.invmap,           0,      0,      "Load inverse colormap"),
   "conmap":            (CMAP.conmap,           1,      1,      "Contour colormap with n equal bands"),
   "chmap":		(CMAP.chmap,    	1,	1,	"Create contoured hot-cold colormap"),
   "cgmap":		(CMAP.cgmap,    	1,	1,	"Create contoured greyscale colormap"),



# Additional View, Rendering, and Mesh Control Commands
   "clrsym":		(MISC.clrsym,	0,	0,	"Clear (delete) symmetry planes"),
   "tmx":		(MISC.tmx,	2,	2,	"Translate material n distance v along axis"),
   "tmy":		(MISC.tmy,	2,	2,	"Translate material n distance v along axis"),
   "tmz":		(MISC.tmz,	2,	2,	"Translate material n distance v along axis"),
   "clrtm":		(MISC.clrtm,	0,	0,	"Clear all material translations"),

   "partrad":		(MISC.partrad,	1,	1,	"Set particle radius"),
   "em":		(MISC.em,	1,	VAR,    "Explode materials by absolute distance"),
   "emsc":		(MISC.emsc,	1,	VAR,    "Explode materials by scaled distance"),
   "emsph":		(MISC.emcsph,	3,	VAR,    "Associate materials for spherical exploded views"),
   "emcyl":		(MISC.emcyl,    6,	VAR,    "Associate materials for cylindrical exploded views"),
   "emax":		(MISC.emax,	6,	VAR,    "Associate materials for axial exploded views"),
   "emrm":		(MISC.emrm,	1,	1,	"Remove exploded view associates"),
   "clrem":		(MISC.clrem,	0,	0,	"Clear all material translations"),
   "tellem":		(MISC.tellem,	0,	0,	"Alias for 'tell em'"),
   "crease":		(MISC.crease,	1,	1,	"Set edge detection angle"),
   "getedg":		(MISC.getedg,	0,	0,	"Re-calculate mesh edges"),
   "getedge":		(MISC.getedg,	0,	0,	"Re-calculate mesh edges"),
   "edgwid":		(OR.edgwid,	1,	1,	"Set line width (pixels) of edges"),
   "edgbias":		(MISC.edgbias,	1,	1,	"Set the depth bias applied to edge line segments"),
   "setcol":    	(SCF.setcol,	3,	3,	"Set text color switch"),
   "inrgb":		(MISC.inrgb,	1,	1,	"Load RGB image file"),
   "mat":		(MISC.mat,	1,	VAR,	"Set material properties"),
   "light":		(MISC.light,	5,	VAR,	"Set/modify light properties"),
   "tlx":		(MISC.tlx,	2,	2,	"Translate light n distance v along axis"),
   "tly":		(MISC.tly,	2,	2,	"Translate light n distance v along axis"),
   "tlz":		(MISC.tlz,	2,	2,	"Translate light n distance v along axis"),
   "dellit":		(MISC.dellet,	0,	0,	"Delete all lights"),
   "camang":		(MISC.camang,	1,	1,	"Set perspective camera angle"),
   "lookfr":		(MISC.lookfr,	3,	3,	"Set location of look-from point"),
   "lookat":		(MISC.lookat,	3,	3,	"Set location of look-at point"),
   "lookup":		(MISC.lookup,	3,	3,	"Set look-up vector"),
   "tfx":		(MISC.tfx,	1,	1,	"Translate look-from point distance v along axis"),
   "tfy":		(MISC.tfy,	1,	1,	"Translate look-from point distance v along axis"),
   "tfz":		(MISC.tfz,	1,	1,	"Translate look-from point distance v along axis"),
   "tax":		(MISC.tax,	1,	1,	"Translate look-at point distance v along axis"),
   "tay":		(MISC.tay,	1,	1,	"Translate look-at point distance v along axis"),
   "taz":		(MISC.taz,	1,	1,	"Translate look-at point distance v along axis"),
   "near":		(MISC.near,	1,	1,	"Set near plane to position v"),
   "far":		(MISC.far,	1,	1,	"Set far plane to position v"),
   "fracsz":		(MISC.fracsz,	1,	1,	"Set number of digits rendered in number fractions"),
   "fracsize":		(MISC.fracsz,	1,	1,	"Set number of digits rendered in number fractions"),
   "bbsrc":		(MISC.bbsrc,	1,	1,	"Set bounding box source object class(es)"),
   "bbox":		(MISC.bbox,	0,	VAR,	"Recalculate or set bounding box"),
   "hidwid":		(MISC.hidwid,	1,	1,	"Set line width (pixels) of mesh lines"),
   "bufqty":		(MISC.bufqty,	1,	VAR,	"Set quantity of I/O buffers used"),

# Misc Commands
    "copyrt":		(MISC.copyrt,      	0,	1,	"Display GrizIt copyright screen"),

# Mouse Picking Commands
   "setpick":		(MPICK.setpick,		2,	2,	"Set mesh object pick class associated with mouse buttons"),
   "minmov":		(MPICK.minmove,		1,	1,	"Set cursor motion threshold (pixels)"),


# Material Manager Commands (imported from MTL)

    "mtl":              (MTL.mtl,               2,      4,      "Various material commands: amb for ambient light, diff for diffuse light, spec for specular light, emis for emissive light, shine for shininess, alpha for opacity, and gslevel for gray scale level"),

# Video Commands

   "vidti":             ( VID.vidti,     2,      2, "Set video title line"),
   "vidttl":            ( VID.vidttl,    0,      0, "Draw video title screen"),

# New Visit Commands

   "opengui":           ( VISIT.opengui, 2,      2, "Open up a Visit GUI"),


# Help Commands
#   "help <s>":		        ( GRIZ_help,
#   "man":		        ( GRIZ_man,
#   "list <s> <s>":		( GRIZ_list,
#   "visitmessages <s>":	( GRIZ_visitmessages
#    "help":			(1, 1, "help commands"),
#    "list":			(1, 2, "list {meshes | materialsets | results [primal | derived]}"),
#    "man":			(0, 0, "display Griz Online Manual"),
#    "visit":			(0, 0, "visit mode"),
#    "visitmessages":		(1, 1, "visitmessages {on | off}")


# For Debugging
    "debug":            ( TS.ExtraTests,      0,   0, "For debugging, use this command to run the extra tests in the test suite"),
    "variables":        ( TS.VariableChecker, 0,   0, "Prints out selected environment variables")
}


#***********************************************************************************************************************

def GrizIt_help_helpsubject(helpsubject):
    if (helpsubject != "commands") and (helpsubject != "Commands"):
	w = GS.WarningMsg("Unrecognized help subject name following '-help'.")
    else:
	m1 = GS.Message("\nGrizIt currently handles the following GRIZ commands:\n")
        m1.display()
    
	commandInfoValues = GrizCommandInfo.values()
	commandList = []
    
	for commandInfoValue in commandInfoValues:
	    bisect.insort(commandList, commandInfoValue[2])
	#end for
    
	for command in commandList:
	    m = GS.Message( "    %s" % (command))
            m.display()
	#end for

	m2 = GS.Message("\nThe following commands are commands to GrizIt itself:\n")
        m2.display()
    
	commandInfoValues = GrizItCommandInfo.values()
	commandList = []
    
	for commandInfoValue in commandInfoValues:
	    bisect.insort(commandList, commandInfoValue[2])
	#end for
    
	for command in commandList:
            m = GS.Message( "    %s" % (command))
            m.display()
	#end for
    
	m3 = GS.Message( "" )
        m3.display()
    #end else
#end def

def GrizIt_list_itemtype_subtype(itemtype, subtype):
    if CurDatabase == "":
	message =  GS.Message("\n\nNo database is currently loaded.", 1)
        message.display()
    elif itemtype == "results":
	if subtype == "primal":
	    resultsVarList = DBPrimalVars
	elif subtype == "derived":
	    resultsVarList = DBDerivedVars
	else:
	    resultsVarList = None
	#end else
	
	if resultsVarList == None:
	    m = GS.Message("Unrecognized result type following '-list results'.", 1)
            m.setColor("red")
            m.display()
	else:
	    listHeading = "The following %s results are available in the currently loaded database:" % (subtype)
	    Print_Columnated_Name_List(resultsVarList, listHeading)
	#end else
    else:
	m = GS.Message("Unrecognized item type following '-list'.", 1)
        m.setColor("red")
        m.display()
    #end else
#end for


def GrizIt_list_itemtype(itemtype):
    if CurDatabase == "":
	message = "\n\nNo database is currently loaded."
        env.mainWin.MsgText( message, 1)
    elif itemtype == "meshes":
	Print_Columnated_Name_List(DBMeshNames, "The following meshes are available in the currently loaded database:")
    elif itemtype == "materialsets":
	for matSetNum in range(DBMatSetCount):
	    listHeading = "Material set %s consists of the following materials:" % (DBMatSetNames[matSetNum])
	    Print_Columnated_Name_List(DBMaterialSets[matSetNum], listHeading)
	#end for
    elif itemtype == "results":
	GrizIt_list_itemtype_subtype("results", "primal")
	GrizIt_list_itemtype_subtype("results", "derived")
    else:
	m = GS.Message("Unrecognized item type following '-list'.", 1)
        m.setColor("red")
        m.display()
    #end else
#end for


#***********************************************************************************************************************

def GrizIt_help_helpsubject(helpsubject):
    if (helpsubject != "commands") and (helpsubject != "Commands"):
	m = GS.Message("Unrecognized help subject name following '-help'.", 1)
        m.setColor("red")
        m.display()
    else:
	m = GS.Message("\nGrizIt currently handles the following GRIZ commands:\n", 1)
        m.display()
    
	commandInfoValues = GrizCommandInfo.values()
	commandList = []
    
	for commandInfoValue in commandInfoValues:
	    bisect.insort(commandList, commandInfoValue[2])
	#end for
    
	for command in commandList:
	    message = GS.Message("    %s" % (command), 1)
            message.display()
	#end for

	m1 = GS.Message("\nThe following commands are commands to GrizIt itself:\n", 1)
        m1.display()
    
	commandInfoValues = GrizItCommandInfo.values()
	commandList = []
    
	for commandInfoValue in commandInfoValues:
	    bisect.insort(commandList, commandInfoValue[2])
	#end for
    
	for command in commandList:
	    m2 = GS.Message("    %s" % (command), 1)
            m2.display()
	#end for
    
	
    #end else
#end def


def GrizIt_list_itemtype_subtype(itemtype, subtype):
    if CurDatabase == "":
	m = GS.Message("No database is currently loaded.", 1)
        m.setColor("red")
        m.display()
    elif itemtype == "results":
	if subtype == "primal":
	    resultsVarList = DBPrimalVars
	elif subtype == "derived":
	    resultsVarList = DBDerivedVars
	else:
	    resultsVarList = None
	#end else
	
	if resultsVarList == None:
	    w = GS.WarningMsg("Unrecognized result type following '-list results'.")
	else:
	    listHeading = "The following %s results are available in the currently loaded database:" % (subtype)
	    Print_Columnated_Name_List(resultsVarList, listHeading)
	#end else
    else:
	m = GS.Message("Unrecognized item type following '-list'.", 1)
        m.setColor("red")
        m.display()
    #end else
#end for


def GrizIt_list_itemtype(itemtype):
    if CurDatabase == "":
	m = GS.Message("No database is currently loaded.", 1)
        m.setColor("red")
        m.display()
    elif itemtype == "meshes":
	Print_Columnated_Name_List(DBMeshNames, "The following meshes are available in the currently loaded database:")
    elif itemtype == "materialsets":
	for matSetNum in range(DBMatSetCount):
	    listHeading = "Material set %s consists of the following materials:" % (DBMatSetNames[matSetNum])
	    Print_Columnated_Name_List(DBMaterialSets[matSetNum], listHeading)
	#end for
    elif itemtype == "results":
	GrizIt_list_itemtype_subtype("results", "primal")
	GrizIt_list_itemtype_subtype("results", "derived")
    else:
	m = GS.WarningMsg("Unrecognized item type following '-list'.")
    #end else
#end for


#-----------------------------------------------------------------------------------------------------------------------

def ParseCommand(command):
    commands = command.split(";")
    for cmd in commands:
        ParseSingleCommand(cmd)
#end def

def ParseSingleCommand(command):
    #
    # Cleanup PopUp message windows
    #
    logCommand=True
    
    if ( env.msg != None ):
         env.msg.hide()
         env.msg.destroy()
         env.msg=None
    #end if
    
    if not command:
        return
    else:
        # Save a copy of command in session
        GS.lastCommand = command
        if command[0]=="-":
           command = command.replace("-","")
           logCommand=False

	commandTokens = re.compile(r'\S+').findall(command) # Could probably just use split

	if len(commandTokens) == 0:
           return
	#end if
  
	commandKey = commandTokens[0]

	commandArgCount = len(commandTokens) - 1
        commandDict	= GrizCommands

        if commandDict.has_key(commandKey):
           commandInfo = commandDict[commandKey]
        else:
           color="red"
           env.mainWin.MsgFont( "Courior", color, 10, False, True, False, Qt.AlignLeft )
           #                                       ital   bold  uline
           message = "\n\nThis is not a currently recognized Griz/GrizIt or VisIt command: " + commandKey
           env.mainWin.MsgText( message, 1 )
           return
        #end else
        
        commandInfo = commandDict[commandKey]  # Note* This is already performed above. Change?     
         
        # Check argument count
        if commandArgCount < commandInfo[1]:
           w = GS.WarningMsg("Too few arguments specified in '%s' command." % (commandKey))
           w = GS.WarningMsg("%s takes %i argument(s)." % (commandKey, commandInfo[2]))
           return
       
        if commandArgCount > commandInfo[2]:
           w = GS.WarningMsg("Too many arguments specified in '%s' command." % (commandKey))
           w = GS.WarningMsg("%s takes %i argument(s)" % (commandKey, commandInfo[2]))
           return

        if (commandInfo[2] == 0 and commandArgCount>0):
            w = GS.WarningMsg("No arguments needed in '%s' command." % (commandKey))

        del commandTokens[0]

        functionClass = CF.GrizCommand(commandKey, commandTokens)

        try:
            functionClass.do()
            #
            # Update the command window
            #
            if ( logCommand and not env.batch_mode ):
                 env.mainWin.MsgFont( "Courior", "green", 12, False, False, False, Qt.AlignLeft ) 
                 env.mainWin.MsgText( command, 2 )
                 log = GS.CmdLog()
                 log.add( command )
            #
            # Update history file
            #
            if logCommand and GS.session.saveHistory == True and command[:5] != 'savhis':
               GS.session.historyFile.write(command+'\n')

        except:
            w = GS.WarningMsg("The following Error Occurred when calling {0} {1}:".format(commandKey, " ".join(commandTokens)))
            w = GS.WarningMsg(traceback.format_exc())
         
    #end else
#end def
