##############################################################################
# Application: GrizIt - Griz CLI interface to the VisIt rendering
#
# grizIt_Env.py - Classes for creating and operating on Meshes.
#
#      
#      Lawrence Livermore National Laboratory
#      Jan 2, 1992
#
#############################################################################
# Modifications:
#
#  I. R. Corey - March 25, 2010: Created.
#
#############################################################################

import pdb, datetime
import os, sys, re, bisect, math
import grizIt_VisitInfo as VI

from array import *
sys.path.append(VI.visitPath)

import visit
import fileinput

from grizIt_Utils import *

import grizIt_Session as S

grizItVersion="GrizIt 10.1 011011"

#############################################################################
# Enums
#############################################################################
class view_mode:
    RENDER_FILLED, RENDER_HIDDEN, RENDER_WIREFRAME, RENDER_WIREFRAMETRANS, RENDER_GS, RENDER_POINT_CLOUD, RENDER_NONEINVALID = range(7)

class mouse_mode:
    MOUSE_HILITE, MOUSE_SELECT = range(2)
    
class filter:
    BOX_FILTER, TRIANGLE_FILTER, SYNC_FILTER = range(3)

class autoimg_format:
    IMAGE_FORMAT_RGB, IMAGE_FORMAT_JPEG, IMAGE_FORMAT_PNG = range(3)

class plot_op:
      OP_NULL, OP_DIFFERENCE, OP_SUM, OP_PRODUCT, OP_QUOTIENT = range(5)
      
class exploded_view:
     SPHERICAL, CYLINDRICL, AXIAL = range(3)
     
class interp_mode:
      NO_INTERP, REG_INTERP, GOOD_INTERP = range(3)
      
class strain_type:
     INFINITESIMAL, GREEN_LAGRANGE, ALMANSI, RATE = range(4)

class ref_frame_type:
     GLOBAL, LOCAL = range(2)

class ref_surface_type:
     MIDDLE, INNER, OUTER = range(3)
     

#############################################################################
# Version Information Classes
#############################################################################
class VersionInfo:
    def __init__(self):
        self.version = 1.0
        self.buildDate = "August 10, 2010"


#############################################################################
# Env Classes
#############################################################################
class Env:
    def __init__(self):
        self.input_filename = ""
        self.partition_filename = ""
        self.host = ""
        self.arch = ""
        self.systype = ""
        self.machtype = ""
        self.time_used = 0.0
        self.user_name = ""
        self.command_line = ""
        self.grizdir = ""
        self.datestr = ""
        self.pid = 0
        self.gid = 0
        self.nprocs = 1

        self.batch_mode = False
        self.batch_filename = ""
        self.show_man = False
        self.show_version = False
        self.winsize = [0,0]
        self.debug_mode = False
        self.datetime_now = ""
        self.datestr = ""
        self.part_file=""
        self.exit = False
        self.tests_file=""
        self.CLImode=False
        self.debugMode=False # Prints out the debug messages in the GUI
        self.CR=None
        self.show_util_panel=False
        self.show_mat_panel=False
        self.show_gui=True
        
        # Visit variables
        self.visitGUI=False
        self.visitBeta=False
        self.visitVersion=None
        self.visitSerial = False

        VisitPlots=None
        VisitWindows=None
        
        # Window manager variables
        self.app=None
        self.CRWin=None
        self.msgWin=None
        self.dialogWin=None
        self.mtlWin=None
        self.mainWin=None        
        self.utilWin=None
        self.msg=None
        self.cmdSelected=False
        self.selectedCmd=""
        self.returnPressed=False
    #end def

    def setenv(self):
        self.systype  = os.getenv( "SYS_TYPE" )
        self.host     = os.getenv( "HOST" )
        self.machtype = os.getenv( "MACHTYPE" )
        self.grizdir  = os.getenv( "GRIZDIR" )
        self.datetime_now = datetime.datetime.now()
        self.datestr      = self.datetime_now.strftime("%B %d, %Y (%H:%M)")
        self.CLImode=False
    #end def    

    def updateVar(self, var, value):
        self.var = value
        d = S.DebugMsg("Variable Set: {0} to {1}. exit: {2}".format(var,value,self.exit))
    #end def

    def populateVars(self):
        """ This function helps populate variables and is called by load"""
        pass # for now, all population happens elsewhere, but this fcn is still called by load

#############################################################################

class Analy:
    def __init__(self):
        self.miliDB=None
        self.db_ident = 0
        self.root_name = ""
        self.mili_version = ""
        self.mili_arch = ""
        self.mili_timestamp = ""
        self.mili_host = ""
        self.xmilics_version = ""
        self.title = ""
        self.min_state = 1
        self.max_state = 0
        self.state_count=0 
        self.cur_state = 100
        self.previous_state = 0
        self.reference_state = 0
        self.mesh_count = 1
        self.cur_mesh_id = 0
        self.dimension = 0
        self.vis_mat_bbox = True
        self.keep_max_bbox_extent = True
        self.render_mode = 0
        self.normals_smooth = 0
        self.hex_overlap = 0
        self.interp_mode = 0
        
        self.state_times = array("d", [0.0])
        self.CurDatabase=""
        self.cur_state = 1

        self.curstride = 1
        
        self.ViewCenteringOn  = False
	self.GenericCentering = False

        # Results Variables
        self.cur_result = ""
        self.rzero=0.0
        self.rzero_set=False
        self.rmin=0.0
        self.rmin_set=False
        self.rmax=0.0
        self.rmax_set=False
        self.refState=-1
        self.globalMM=[sys.float_info.min, sys.float_info.max]
        self.coordxf_nodes=[0,0,0]
        self.coordxf_points=[0.0,0.0,0.0,  0.0,0.0,0.0,  0.0,0.0,0.0]
        self.dirv=[0.0,0.0,0.0]
        
        # Database variables
        self.DBPrimalVarCount = 0
        self.DBPrimalVarsMili = {}
        self.DBPrimalVars = {}
        
        self.DBDerivedVarCount = 0
 	self.DBDerivedVars  = {}       

        self.DBMatSetCount  = 0
        self.DBMeshNames    = []
        self.DBMatSetNames  = []
        self.DBMaterialSets = []
        self.DBMatMeshNames = []

        self.DBMeshes = []
        self.DBMaterials    = []
        self.DBMaterialsVisible = []
        self.DBMaterialsResult  = []

        self.PrimalList = []
        self.DerivedList = []

        self.MiliClassNames = []
        self.MiliClassIds = []
        
        # Output Options
        self.jpeg_quality = 75
        
        # Visit Options
        self.VisItMessagesOn = True

        #######################################
        # Command State and ControlVariables
        #  - Arranged by sections as per Griz
        #    Command Quick Reference
        #######################################

        
        # Viewing Mode Variables
        self.vcentPoint = [0.,0.,0.]
        self.vcentNode  = 0
        self.OnOff_edges = False
        self.OnOff_vcent = False
        self.presp = True
        self.ortho = True
        
        self.OnOff_ei_mode = False # Flag Only. No feedback code
        self.OnOff_gs_mode = False # Flag Only. No feedback code
        self.OnOff_pn_mode = False # Flag Only. No feedback code
        self.OnOff_fn_mode = False # Flag Only. No feedback code

        # State and Time Variables
        self.OnOff_autoimg = False
        self.state = 1
        self.numStates = 1
        self.minst  = 0
        self.maxst  = 1
        self.curst  = 1

        # Animation variables
        self.animMinst = 0
        self.animMaxst = 0

        self.stride = 1

        self.currentAnimThread = None
        
        # Executive Control Variables
        self.refresh   = False
        self.aliasCmds = {}
                                
        # MeshControl Variables
        self.materials         = [] #['mat1','mat2'...]
        self.materialNums      = [] #[1,2,3...]
        self.materials_visible = {} # Dictionary -> {material:True if visible, False if invisible}
        self.materials_result  = []
        self.mat_mode_on = False
        self.true_colors = {} # a dictionary of the original colors
        self.enabled_mats = {} # {materials: True if enabled, False if disabled # This needs to be established as all mats in the beginning!

        # Mouse Pick Classes
        self.pickMode = "pichil"
        self.btn1Pick = None
        self.btn2Pick = None
        self.btn3Pick = None
        self.minmove  = 0
    
        # Interaction History Variables
        self.historyFile = ""
        self.saveHistory = False
        self.log = []
        self.logIndex=0

        # Visualization Variables
        self.OnOffCutDisplay = False # This default maybe should be true...
        self.OnOffRoughDisplay = False
        self.cutPlanes = []

         
        # Results Commands Variables
        self.OnOff_derived = False
        self.OnOff_primal  = False
        self.OnOff_conv    = False

        # Time History (TH) Commands Variables
        self.OnOff_glyphs = False
        self.OnOff_glyphcol  = False
        self.OnOff_conv    = False

        # Plot info/legend control variables
        self.show_all = True
        self.show_bbox = True
        self.show_coord = True
        self.show_time = True
        self.show_triad = True
        self.show_safe = True
        self.show_scale = True
        self.show_title = True
        self.show_legend = True
        self.show_user = True
        self.show_cmap = True
        self.show_minmax = True
         
        self.animStop = False
        
    def checkVar(self, var):
        # For debugging purposes - but something is wrong. it's not printing correctly
        d = S.DebugMsg("\n** \n Checking Var \n** \n The value of"+self.var+"is"+var)
        
    #end def
    def updateVar(self, var, value):
        self.var = value
        d = S.DebugMsg("Variable Set: {0} to {1}. exit: {2}".format(var,value,self.exit))

