############################################################################
# grizIt_GrizOnOffFactory.py - Function Factory for On/Off Functions
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      
#
# 
############################################################################
# Modifications:
#  S. E. Drakeley - July 2010: Created
#
############################################################################

## For on/off factories:
import grizIt_GrizAddtlCommands as AC
import grizIt_GrizViewingModeCommands as VM
import grizIt_GrizStateTimeCommands as ST
import grizIt_GrizExecCommands as EC
import grizIt_GrizMiscCommands as MISC
import grizIt_GrizResultsCommands as RC
import grizIt_GrizOptRenderingCommands as OR
import grizIt_GrizTHCommands as TH
import grizIt_GrizVisCommands as VC
import grizIt_GrizIndepSurfaceCommands as IS
import grizIt_GrizVideoCommands as VID
import grizIt_Session as S

GRIZ_null = None
VAR = 100

# The on/off dictionary:
on_off_commands = {
#   Keyword             Function           MinArgs  MaxArgs   Help Text
# Viewing Mode Commands
    "ei":	        (VM.OnOffEi,     	    1, 	    1, "Turn on (off) Error Indicator (EI)"),
    "gs":		(VM.OnOffGs, 		    1, 	    1, "Turn on (off) greyscale rendering of disabled materials"),
    "pn":		(VM.OnOffPn, 		    1,	    1, "Turn on (off) particle rendering mode"),
    "fn":		(VM.OnOffFn, 		    1,	    1, "Turn on (off) free node rendering mode"),

    "all":		(OR.OnOffAll,		    1,	    1,	"Turn on (off) 'coord', 'time', 'title', 'cmap', 'minmax', and 'dscal'"),
# State Time Commands
    "autoimg":   	(ST.OnOffAutoimg,           1,      2,  "Specify auto-image rootfile and type: autoimg rootname [jpeg | png]"),

# Exec Commands
    "refresh":		(EC.refresh,	            1,	     1,         "Turn on (off) rendering window update"),
    "su":		(EC.OnOffSu,		    1,	     1,	        "Suppress updates: Turn on (off) rendering window updates even if window is resized"),

# Results Commands
    "coordxf":          (RC.coordxf,                1,      9,         "Specify alternate coord system via 3 nodes or 3 points: coordxf node1 node2 node3 | coordxf x1y1z1 x2y2z2 x3y3z3 | Turn on (off) stress/strain coordinate transformations"),
    "derived":		(RC.OnOffDerived,	    1,      1,         "Turn on (off) searches of the derived result table"),
    "primal":		(RC.OnOffPrimal,	    1,	    1,	       "Turn on (off) searches of the primal result table"),
    "conv":		(RC.OnOffConv,	            1,	    1,	       "Turn on (off) result units conversion: conv {on | off} | Set units conversion parameters: conv scale offset"),

# Optional Rendered Info Commands
    "all":		(OR.OnOffAll,		1,	1,	"Turn on (off) 'coord', 'time', 'title', 'cmap', 'minmax', and 'dscal'"),
    "bgimage":		(OR.OnOffBgimage,	1,	1,	"Turn on (off) display of loaded image as background"),
    "box":		(OR.OnOffBox,		1,	1,	"Turn on (off) mesh bounding box"),
    "cmap":		(OR.OnOffCmap,    	1,	1,	"Turn on (off) colormap and associated indicators"),
    "coord":		(OR.OnOffCoord,      	1,	1,	"Turn on (off) coordinate axes"),
    "cscale":		(OR.OnOffCscale,	1,	1,	"Turn on (off) colormap title and value scale"),
    "dscal":		(OR.OnOffDscal,		1,	1,	"Turn on (off) display of nodal displacement scale factors"),
    "dscale":		(OR.OnOffDscal,		1, 	1,	"Turn on (off) display of nodal displacement scale factors"),
    "edges":		(OR.OnOffEdges,		1,	1,	"Turn on (off) mesh edges"),
    "locref":		(OR.OnOffLocref,	1,	1,	"Turn on (off) display of local coords on 'select'ed shell and hex elements"),
    "minmax":           (OR.OnOffMinmax,	1,	1,	"Turn on (off) result min/max"),
    "num":		(OR.OnOffNum,		1,	1,	"Turn on (off) node/element numbering"),
    "time":		(OR.OnOffTime,		1,	1,	"Turn on (off) current time"),
    "title":		(OR.OnOffTitle,		1,	1,	"Turn on (off) mesh title: title {on | off} | Update data title: title 'Title String'"),

# Time History Commands
    "glyphs":		(TH.OnOffGlyphs,	1,	1,	"Turn on (off) display of geometric glyphs on plot curves"),
    "glyphcol":		(TH.OnOffGlyphcol,	1,	1,	"Turn on (off) rendering of glyphs in curve colors"),
    "minmax":		(TH.OnOffMinmax,       	1,	1,	"Turn on (off) display of ymin/ymax values"),
    "plotcoords":	(TH.OnOffPlotcoords,	1,	1,	"Turn on (off) cursor coordinates display"),
    "plotcol":		(TH.OnOffPlotcol,	1,	1,	"Turn on (off) rendering of plot curves in individual colors"),
    "plotgrid":		(TH.OnOffPlotgrid,	1,	1,	"Turn on (off) background grid in plot box"),
    "leglines":		(TH.OnOffLeglines,	1,	1,	"Turn on (off) inclusion of curve samples in legend"),
    
# Visualization Commands
    "cut":		(VC.OnOffCut,		1,	1,	"Turn on (off) cutting planes"),
    "rough":		(VC.OnOffRough,		1,	1,	"Turn on (off) rough cutting planes"),
    "con":		(VC.OnOffCon,		1,	1,	"Turn on (off) contours"),
    "iso":		(VC.OnOffIso,		1,	1,	"Turn on (off) isosurfaces"),
    "vec":		(VC.OnOffVec,		1,	VAR,	"Turn on (off) vector field display: vec {on | off} | Define vector result: vec rnx [rny [rnz]]"),
    "sphere":		(VC.OnOffSphere,	1,	1,	"Turn on (off) spheres in 3D vector bases"),
    "pn":		(VC.OnOffPn,		1,	1,	"Turn on (off) meshless particles"),
    "fn":		(VC.OnOffFn,		1,	1,	"Turn on (off) free nodes"),

# Indpendent Surface Commands
    "refsrf":		(IS.OnOffRefsrf,	1,	1,	"Turn on (off) reference surfaces"),
    "refres":		(IS.OnOffRefres,	1,	1,	"Turn on (off) result display on reference surfaces"),
    "extsrf":		(IS.OnOffExtsrf,        1,	1,	"Turn on (off) external surfaces"),

# Additional View, Rendering, and Mesh Control Commands
    "sym":		(AC.OnOffSym,		1,	6,	"Turn on (off) reflections about symmetry planes: sym {on | off} | Add symmetry plane: sym p_x p_y p_z n_x n_y n_z"),
    "particles":	(AC.OnOffParticles,	1,	1,	"Turn on (off) display of particles from a database"),
    "lighting":		(AC.OnOffLighting,	1,	1,	"Turn on (off) lighting contribution to shading"),
    "autosz":		(AC.OnOffAutosz,	1,	1,	"Turn on (off) automatic sizing of plot axis numbers"),
    "bbmax":		(AC.OnOffBbmax,		1,	1,	"Turn on (off) accumulation of max bounding box extents"),
    "shrfac":		(AC.OnOffShrfac,	1,	1,	"Turn on (off) Fix coincident polygon rendering bug"),
    "hexoverlap":       (AC.OnOffHexoverlap,	1,	1,	"Turn on (off) generation of redundant faces from coincident hexahedral elements of the same element class but different materials"),
    "zlines":		(MISC.noOp,    	        1,	1,	"Turn on (off) depth-buffering of anti-aliased lines with respect to each other"),

# Video Commands
    "safe":             (VID.OnOffSafe,         1,    1, "Show 'safe action' area border")
    }

############################################################################################

class OnOffFactory(object):
    """ This is the on/off factory for the classes listed in the command dict """
    def __init__(self, flag, command_tokens):
        """ Creates the appropriate class and performs the appropriate do function.

            command_tokens consists of class_key and args
        """
        self.class_key = command_tokens[0]
        self.flag = flag
        self.args = command_tokens[1:]
        self.command_dict = on_off_commands

    def do(self):
        PyCommand = self.command_dict[self.class_key][0]
        try:
            newInstance = PyCommand(self.flag)
            newInstance.do() # This will default to the OnOffFunction do, found in grizIt_Session.py

        except TypeError:
            d = S.DebugMsg(type(PyCommand)+ "this is the type of the pycommand \n@@")
            d = S.DebugMsg(PyCommand+"is the pycommand")
            d = S.DebugMsg(self.command_dict.keys()+ "are the keys in the dictionary")
            d = S.DebugMsg(self.command_dict[self.class_key]+ "is the class key list")
            d = S.DebugMsg(self.class_key+ "is the class key")


class on(S.GrizFunction):
    def __init__(self, command_tokens):
        self.onClass = OnOffFactory('on', command_tokens)
    def do(self):
        self.onClass.do()

class off(S.GrizFunction):
    def __init__(self, command_tokens):
        self.offClass = OnOffFactory('off', command_tokens)
    def do(self):
        self.offClass.do()

