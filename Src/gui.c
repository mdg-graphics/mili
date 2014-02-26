/* $Id$ */
/*
 *
 * gui.c - Graphical user interface routines.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Apr 27 1992
 *
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Oct 1, 2004:Changed reference to env variable GRIZ4HOME
 *  to GRIZHOME.
 *
 *  I. R. Corey - Jan 5, 2005: Added new option to allow for selecting
 *                a beta version of the code. This option (-beta) is
 *                also parsed by the griz4s shell script.
 *                See SCR #298.
 *
 *  I. R. Corey - Mar 29, 2005: Print list of in objects state records.
 *
 *  I. R. Corey - Apr 11, 2005: Increased length of cnt to 100 in
 *                function wrt_standard_db_text.
 *
 *  I. R. Corey - May 19, 2005: Added new option to allow for selecting
 *                a alpha version of the code. This option (-alpha) is
 *                also parsed by the griz4s shell script.
 *                See SCR #328.
 *
 *  I. R. Corey - Aug 26, 2005: Add option to hide result from pull-down
 *                list using new variable 'hide_in_menu'.
 *                See SRC# 339.
 *
 *  I. R. Corey - Jan 17, 2006: Add Rubberband zoom capability. Activated
 *                using control key with left mouse button.
 *                See SRC#354 .
 *
 *  I. R. Corey - Feb 02, 2006: Add a new capability to display meshless,
 *                particle-based results.
 *                See SRC#367.
 *
 *  I. R. Corey - October 24, 2006: Add a class selector to all vis/invis
 *                and enable/disable commands.
 *                See SRC#421.
 *
 *  I. R. Corey - Dec 16, 2006: Added full path name to plot window and remove
 *                from titlebar.
 *                See SRC#431.
 *
 *  I. R. Corey - Feb 12, 2007: Added wireframe viewing capability and added
 *                logic to fix degenerate polygon faces.
 *                See SRC#437.
 *
 *  I. R. Corey - Apr 25, 2007:	Added check to make sure we really had a
 *                button press.
 *
 *  I. R. Corey - Apr 27, 2007:	Added a new menu item and button for Help
 *                to display the PDF manual.
 *
 *  I. R. Corey - May 11, 2007:	Raise Utility and Control windows to the
 *                top.
 *                See SRC#456.
 *
 *  I. R. Corey - May 15, 2007:	Save/Restore window attributes.
 *                See SRC#458.
 *
 *  I. R. Corey - Aug 15, 2007:	Added menus for displaying TI results.
 *                See SRC#480.
 *
 *  I. R. Corey - Aug 24, 2007:	Made a minor adjustment to the RB zoom
 *                factor, set to 1.28 - RE Mark Gracia.
 *                See SRC#482.
 *
 *  I. R. Corey - Aug 24, 2007:	Added a new help button to display cur-
 *                rent release notes.
 *                See SRC#483.
 *
 *  I. R. Corey - Oct 04, 2007: Increase size of history buffer in control
 *                window to 25.
 *                See SRC#493.
 *
 *  I. R. Corey - Jan 09, 2008: Added button to utility menu for Greyscale
 *                              mode.
 *                See SRC#476.
 *
 *  I. R. Corey - Feb 12, 2008: Added a condition compile option for IRIX.
 *
 *  K. Durrenberger - April 29, 2008: added a define for the glwMDrawingAreaWidgetClass
 *                    to be compiled as glwDrawingAreaWidgetClass except on the
 *		      sun and irix systems.
 *
 *  I. R. Corey - Feb 12, 2008: Added a conditional compile option for
 *                              IRIX.
 *
 *  I. R. Corey - May 5, 2008: Added support for code usage tracking using
 *                the AX tracker tool.
 *                See SRC#533.
 *
 *  I. R. Corey - Aug 12, 2009: Rewrote the code to get blocking and class
 *                info for the wrt_standard_db_text() function.
 *                See SRC#621.
 *
 *  I. R. Corey - Nov 09, 2009: Added enhancements to better support
 *                running multiple sessions of Griz.
 *                See SRC#638.
 *
 *  I. R. Corey - Sept 14, 2010: Added support for tear-off menus.
 *                See SRC#686
 *
 *  I. R. Corey - June 1, 2011: Added support for long result menus using
 *                a multi-column layout.
 *
 *  I. R. Corey - April 8th, 2012: Completed development of surface class
 *                based on new requirements.
 *                See TeamForge#17795
 *
 *  I. R. Corey - May 2nd, 2012: Added path to top of window panes.
 *                See TeamForge#17900
 *
 *  I. R. Corey - July 26th, 2012: Added capability to plot a Modal
 *                database from Diablo.
 *                See TeamForge#18395 & 18396
 *
 *  I. R. Corey - October 18th, 2012: Fixed problem with output of blocking
 *                info.
 *
 *  I. R. Corey - November 14th, 2012: Removed code name & date from top
 *                of all windows and moved to Control Window text box.
 *
 *  I. R. Corey - March 20th, 2013:
 *                of all windows and moved to Control Window text box.
 *                See TeamForge#18395 & 18396
 *
 ************************************************************************
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include <math.h>

#include "griz_config.h"

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/CascadeB.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/Command.h>
#include <Xm/PanedW.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/RowColumn.h>
#include <Xm/BulletinB.h>
#include <Xm/Scale.h>
#include <Xm/Frame.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/MessageB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>
#include <Xm/ArrowB.h>
#include <Xm/Protocols.h>
#ifdef __alpha
#include "GLwMDrawA.h"
#define GLWM 1
#else
#ifdef sun
#include "GLwMDrawA.h"
#define GLWM 1
#else
#include "GL/GLwDrawA.h"
#undef GLWM
#endif
#endif

#include "GL/gl.h"
#include "GL/glx.h"
#include "image.h"
#include "viewer.h"
#include "draw.h"
#include "results.h"
/* Bitmaps. */
#include "Bitmaps/GrizCheck"
#include "Bitmaps/GrizStart"
#include "Bitmaps/GrizStop"
#include "Bitmaps/GrizLeftStop"
#include "Bitmaps/GrizLeft"
#include "Bitmaps/GrizRight"
#include "Bitmaps/GrizRightStop"

#ifdef xSERIAL_BATCHx
#include <GL/gl_mangle.h>
#endif

#define GRIZ_VERSION "                      WELCOME TO GRIZ " PACKAGE_VERSION "\n"
#define GRIZ_DATE    "                        Updated: " PACKAGE_DATE "\n"

#define ABS(x) ((x) < 0 ? -(x) : (x))

/*
 * Global boolean to allow utility and material panels to be part of control window.
 * Set indirectly from command line.
 */
Bool_type include_util_panel    = FALSE;
Bool_type include_mtl_panel     = FALSE;

/* Griz name & version for window titles. */
static char *griz_name=NULL;
static char path_string[256];

/* Set to TRUE if the material Color-Mode is active */
Bool_type mtl_color_active=FALSE;

/* Time series plotting window bounds. */
static float win_xmin;
static float win_ymin;
static float win_xman;
static float win_ymax;
static float win_xspan;
static float win_yspan;
static float data_xmin;
static float data_ymax;
static float data_xspan;
static float data_yspan;

extern Session *session;

extern Bool_type history_inputCB_cmd;

/* Flag to preclude redraws for resize, expose events. */
static Bool_type suppress_updates   = FALSE;
static Bool_type resize_in_progress = FALSE;


/*
 * Menu button IDs.
 */
typedef enum
{
    BTN_COPYRIGHT,
    BTN_MTL_MGR,
    BTN_SURF_MGR,
    BTN_UTIL_PANEL,
    BTN_SAVE_SESSION_GLOBAL,
    BTN_SAVE_SESSION_PLOT,
    BTN_LOAD_SESSION_GLOBAL,
    BTN_LOAD_SESSION_PLOT,
    BTN_QUIT,

    BTN_DRAWFILLED,
    BTN_DRAWHIDDEN,
    BTN_DRAWWIREFRAME,
    BTN_DRAWWIREFRAMETRANS,
    BTN_DRAWGS,
    BTN_COORDSYS,
    BTN_TITLE,
    BTN_TIME,
    BTN_COLORMAP,
    BTN_MINMAX,
    BTN_SCALE,
    BTN_ALLON,
    BTN_ALLOFF,
    BTN_BBOX,
    BTN_EDGES,
    BTN_PERSPECTIVE,
    BTN_ORTHOGRAPHIC,
    BTN_ADJUSTNF,
    BTN_RESETVIEW,
    BTN_SU,

    BTN_HILITE,
    BTN_SELECT,
    BTN_CLEARHILITE,
    BTN_CLEARSELECT,
    BTN_CLEARALL,
    BTN_PICSHELL,
    BTN_PICBEAM,
    BTN_CENTERON,
    BTN_CENTEROFF,

    BTN_INFINITESIMAL,
    BTN_ALMANSI,
    BTN_GREEN,
    BTN_RATE,
    BTN_LOCAL,
    BTN_GLOBAL,
    BTN_MIDDLE,
    BTN_INNER,
    BTN_OUTER,

    BTN_NEXTSTATE,
    BTN_PREVSTATE,
    BTN_FIRSTSTATE,
    BTN_LASTSTATE,
    BTN_ANIMATE,
    BTN_STOPANIMATE,
    BTN_CONTANIMATE,

    BTN_TIMEPLOT,
    BTN_EI,

    BTN_FN,  /* Free nodes     */
    BTN_PN,  /* Particle nodes */

    BTN_HELP,
    BTN_RELNOTES,
    BTN_GS
} Btn_type;


/* Local routines. */
static void init_gui( void );
#ifdef USE_OLD_CALLBACKS
static void expose_CB( Widget w, XtPointer client_data, XtPointer call_data );
static void resize_CB( Widget w, XtPointer client_data, XtPointer call_data );
#else
static void expose_resize_CB( Widget w, XtPointer client_data,
                              XtPointer call_data );
static Bool_type get_last_renderable_event( Window win, XEvent *p_xe,
        Bool_type *p_resize,
        Dimension *p_width,
        Dimension *p_height );
#endif
static void res_menu_CB( Widget w, XtPointer client_data, XtPointer call_data );
static void menu_CB( Widget w, XtPointer client_data, XtPointer call_data );
static void input_CB( Widget w, XtPointer client_data, XtPointer call_data );
static void process_keyboard_input( XKeyEvent *p_xke );
static void install_plot_coords_display( void );
static void remove_plot_coords_display( void );
static void set_plot_cursor_display ( int cursor_x, int cursor_y );
#ifdef WANT_PLOT_CALLBACK
static void plot_input_CB( Widget w, XtPointer client_data,
                           XtPointer call_data );
#endif
static void parse_CB( Widget w, XtPointer client_data, XtPointer call_data );
static void exit_CB( Widget, XtPointer, XtPointer );

static void destroy_mtl_mgr_CB( Widget w, XtPointer client_data,
                                XtPointer call_data );
void destroy_mtl_mgr( void );
static void destroy_surf_mgr_CB( Widget w, XtPointer client_data,
                                 XtPointer call_data );
void destroy_surf_mgr( void );
static void destroy_util_panel_CB( Widget w, XtPointer client_data,
                                   XtPointer call_data );

/*     Material Manager Functions */
static void mtl_func_select_CB( Widget w, XtPointer client_data,
                                XtPointer call_data );
static void mtl_quick_select_CB( Widget w, XtPointer client_data,
                                 XtPointer call_data );
static void mtl_select_CB( Widget w, XtPointer client_data,
                           XtPointer call_data );
static void mtl_func_operate_CB( Widget w, XtPointer client_data,
                                 XtPointer call_data );
static void col_comp_disarm_CB( Widget w, XtPointer client_data,
                                XtPointer call_data );
static void col_ed_scale_CB( Widget w, XtPointer client_data,
                             XtPointer call_data );
static void col_ed_scale_update_CB( Widget w, XtPointer client_data,
                                    XtPointer call_data );
static void expose_swatch_CB( Widget w, XtPointer client_data,
                              XtPointer call_data );
static void init_swatch_CB( Widget w, XtPointer client_data,
                            XtPointer call_data );

/*     Surface Manager Functions */
static void surf_func_select_CB( Widget w, XtPointer client_data,
                                 XtPointer call_data );
static void surf_quick_select_CB( Widget w, XtPointer client_data,
                                  XtPointer call_data );
static void surf_select_CB( Widget w, XtPointer client_data,
                            XtPointer call_data );
static void surf_func_operate_CB( Widget w, XtPointer client_data,
                                  XtPointer call_data );

static void util_render_CB( Widget w, XtPointer client_data,
                            XtPointer call_data );
static void step_CB( Widget w, XtPointer client_data, XtPointer call_data );
static void stride_CB( Widget w, XtPointer client_data, XtPointer call_data );
static void menu_setpick_CB( Widget w, XtPointer client_data,
                             XtPointer call_data );
static void menu_setcolormap_CB( Widget w, XtPointer client_data, XtPointer call_data );
static void plot_cursor_EH( Widget w, XtPointer client_data,
                            XEvent *event, Boolean *continue_dispatch );
static void enter_render_EH( Widget w, XtPointer client_data, XEvent *event,
                             Boolean *continue_dispatch );
static void stack_init_EH( Widget w, XtPointer client_data, XEvent *event,
                           Boolean *continue_dispatch );
static void stack_window_EH( Widget w, XtPointer client_data, XEvent *event,
                             Boolean *continue_dispatch );
static void find_ancestral_root_child( Widget widg, Window *root_child );
static void create_app_widg( Btn_type btn );
static void create_result_menus( Widget parent );
static void create_derived_res_menu( Widget parent );
static void create_primal_res_menu( Widget parent );
static void create_ti_res_menu( Widget parent );
static void add_primal_result_button( Widget parent, Primal_result * );
static void add_ti_result_button( Widget parent, Primal_result * );
static void get_primal_submenu_name( Analysis *, Primal_result *, char * );
static Bool_type find_labelled_child( Widget, char *, Widget *, int * );
static void add_derived_result_button( Derived_result * );
static void get_result_submenu_name( Derived_result *, char * );
static Widget create_menu_bar( Widget parent, Analysis * );
static Widget create_mtl_manager( Widget main_widg );
static Widget create_surf_manager( Widget main_widg );
static Widget create_free_util_panel( Widget main_widg );
static Widget create_utility_panel( Widget main_widg );
static Widget create_pick_menu( Widget parent, Util_panel_button_type btn_type,
                                char *cascade_name );
static Widget create_pick_submenu( Widget parent,
                                   Util_panel_button_type btn_type,
                                   char *cascade_name,
                                   Widget *p_initial_button );
static void get_pick_superclass( Util_panel_button_type, int * );
static void send_mtl_cmd( char *cmd, int tok_qty );
static size_t load_mtl_mgr_funcs( char *p_buf, int *p_token_cnt );
static size_t load_selected_mtls( char *p_buf, int *p_tok_cnt );
static size_t load_mtl_properties( char *p_buf, int *p_tok_cnt );
static void select_mtl_mgr_mtl( int mtl );
static void send_surf_cmd( char *cmd, int tok_qty );
static size_t load_surf_mgr_funcs( char *p_buf, int *p_token_cnt );
static size_t load_selected_surfs( char *p_buf, int *p_tok_cnt );
static void select_surf_mgr_surf( int surf );
static void set_rgb_scales( GLfloat col[4] );
static void set_shininess_scale( GLfloat shine );
static void set_scales_to_mtl( void );
static void update_actions_sens( void );
static void update_surf_actions_sens( void );
static void update_swatch_label( void );
static void action_quit( Widget w, XEvent *event, String params[], int *qty );
static void action_translate_command( Widget w, XEvent *event, String params[],
                                      int *qty );
static Bool_type mtl_func_active( void );
static Bool_type surf_func_active( void );

static void action_create_app_widg( Widget w, XEvent *event, String params[],
                                    int *qty );
static void resize_mtl_scrollwin( Widget w, XEvent *event, String params[],
                                  int qty );
static void resize_surf_scrollwin( Widget w, XEvent *event, String params[],
                                   int qty );
static void gress_mtl_mgr_EH( Widget w, XtPointer client_data, XEvent *event,
                              Boolean *continue_dispatch );
static void gress_surf_mgr_EH( Widget w, XtPointer client_data, XEvent *event,
                               Boolean *continue_dispatch );
static void string_convert( XmString str, char *buf );
static Boolean animate_workproc_CB();
static void remove_widget_CB( Widget, XtPointer, XtPointer );
static void init_alt_cursors( void );

void get_window_attributes( void );
void put_window_attributes( void );
void defineDialogColor( Display* dpy );
void pushpop_window( PushPop_type direction );

MO_class_data *
assemble_blocking( Analysis *analy, int sclass, char *class_name,
                   int *qty_objects, int *label_block_qty,
                   int *num_blocks, int *blocks, int *blocks_labels );

MO_class_data *
get_blocking_info( Analysis *analy,  char *class_name, int superclass,
                   int *qty_objects,
                   int *num_blocks,  int **blocks );

int
assemble_compare_blocks( int *blockl1, int *block2 );

void defineBorderColor( Display* dpy );

static Widget
create_colormap_menu( Widget parent, Util_panel_button_type btn_type,
                      char *cascade_name );

static Widget
create_colormap_submenu( Widget parent, colormap_type btn_type,
                         Widget *p_initial_button );


void x11_signal( int code )
{
    exit( code );
}

/*
 * This resource list provides defaults for settable values in the
 * interface widgets.
 */
String fallback_resources[] =
{
    "*fontList: -misc-fixed-medium-r-normal-*-14-*-*-*-*-*-*-*",
    "*background: grey",
    "*monitor*height: 130",
    "*monitor*columns: 60",
    "*foreground: Black",
    NULL
};

static void load_colormap( Analysis *analy, char *colormap );

#ifdef OLD_MENU_CODE
/*
 * Menu buttons in results sub-menus.
 */

static int share_btns[] =
{
    VAL_SHARE_SIGX,
    VAL_SHARE_SIGY,
    VAL_SHARE_SIGZ,
    VAL_SHARE_SIGXY,
    VAL_SHARE_SIGYZ,
    VAL_SHARE_SIGZX,
    VAL_SHARE_SIG_EFF,
    VAL_SHARE_SIG_PD1,       /* 1st prinicipal deviatoric stress */
    VAL_SHARE_SIG_PD2,       /* 2nd prinicipal deviatoric stress */
    VAL_SHARE_SIG_PD3,       /* 3rd prinicipal deviatoric stress */
    VAL_SHARE_SIG_MAX_SHEAR, /* Maximum shear stress */
    VAL_SHARE_SIG_P1,        /* 1st principle stress */
    VAL_SHARE_SIG_P2,        /* 2nd principle stress */
    VAL_SHARE_SIG_P3,        /* 3rd principle stress */
    VAL_SHARE_EPSX,
    VAL_SHARE_EPSY,
    VAL_SHARE_EPSZ,
    VAL_SHARE_EPSXY,
    VAL_SHARE_EPSYZ,
    VAL_SHARE_EPSZX,
    VAL_SHARE_EPS_EFF,
    VAL_SHARE_PRESS,
    VAL_ALL_END
};

static int hex_btns[] =
{
    VAL_HEX_RELVOL,        /* Relative volume */
    VAL_HEX_SIG_P3,        /* 3rd principle stress */
    VAL_HEX_EPS_PD1,       /* 1st prinicipal deviatoric strain */
    VAL_HEX_EPS_PD2,       /* 2nd prinicipal deviatoric strain */
    VAL_HEX_EPS_PD3,       /* 3rd prinicipal deviatoric strain */
    VAL_HEX_EPS_MAX_SHEAR, /* Maximum shear strain */
    VAL_HEX_EPS_P1,        /* 1st principle strain */
    VAL_HEX_EPS_P2,        /* 2nd principle strain */
    VAL_HEX_EPS_P3,        /* 3rd principle strain */
    VAL_ALL_END
};

static int shell_btns[] =
{
    VAL_SHELL_RES1,        /* M_xx bending resultant */
    VAL_SHELL_RES2,        /* M_yy bending resultant */
    VAL_SHELL_RES3,        /* M_zz bending resultant */
    VAL_SHELL_RES4,        /* Q_xx shear resultant */
    VAL_SHELL_RES5,        /* Q_yy shear resultant */
    VAL_SHELL_RES6,        /* N_xx normal resultant */
    VAL_SHELL_RES7,        /* N_yy normal resultant */
    VAL_SHELL_RES8,        /* N_zz normal resultant */
    VAL_SHELL_THICKNESS,   /* Thickness */
    VAL_SHELL_INT_ENG,     /* Internal energy */
    VAL_SHELL_SURF1,       /* Surface stress ( taurus 34 ) */
    VAL_SHELL_SURF2,       /* Surface stress ( taurus 35 ) */
    VAL_SHELL_SURF3,       /* Surface stress ( taurus 36 ) */
    VAL_SHELL_SURF4,       /* Surface stress ( taurus 37 ) */
    VAL_SHELL_SURF5,       /* Surface stress ( taurus 38 ) */
    VAL_SHELL_SURF6,       /* Surface stress ( taurus 39 ) */
    VAL_SHELL_EFF1,        /* Effective upper surface stress */
    VAL_SHELL_EFF2,        /* Effective lower surface stress */
    VAL_SHELL_EFF3,        /* Maximum effective surface stress */
    VAL_ALL_END
};

static int beam_btns[] =
{
    VAL_BEAM_AX_FORCE,     /* Axial force */
    VAL_BEAM_S_SHEAR,      /* S shear resultant */
    VAL_BEAM_T_SHEAR,      /* T shear resultant */
    VAL_BEAM_S_MOMENT,     /* S moment */
    VAL_BEAM_T_MOMENT,     /* T moment */
    VAL_BEAM_TOR_MOMENT,   /* Torsional resultant */
    VAL_BEAM_S_AX_STRN_P,  /* S axial strain (+) */
    VAL_BEAM_S_AX_STRN_M,  /* S axial strain (-) */
    VAL_BEAM_T_AX_STRN_P,  /* T axial strain (+) */
    VAL_BEAM_T_AX_STRN_M,  /* T axial strain (-) */
    VAL_ALL_END
};

static int node_btns[] =
{
    VAL_NODE_VELX,         /* Nodal X velocity */
    VAL_NODE_VELY,         /* Nodal Y velocity */
    VAL_NODE_VELZ,         /* Nodal Z velocity */
    VAL_NODE_VELMAG,       /* Nodal velocity magnitude */
    VAL_NODE_ACCX,         /* Nodal X acceleration */
    VAL_NODE_ACCY,         /* Nodal Y acceleration */
    VAL_NODE_ACCZ,         /* Nodal Z acceleration */
    VAL_NODE_ACCMAG,       /* Nodal acceleration magnitude */
    VAL_NODE_TEMP,         /* Nodal temperature */
    VAL_NODE_DISPX,        /* Nodal X displacement */
    VAL_NODE_DISPY,        /* Nodal Y displacement */
    VAL_NODE_DISPZ,        /* Nodal Z displacement */
    VAL_NODE_DISPMAG,      /* Nodal displacement magnitude */
    VAL_NODE_PINTENSE,     /* Nodal pressure intensity */
    VAL_NODE_HELICITY,     /* Nodal helicity */
    VAL_NODE_ENSTROPHY,    /* Nodal enstrophy */
    VAL_NODE_K,            /* Nodal k */
    VAL_NODE_EPSILON,      /* Nodal epsilon */
    VAL_NODE_A2,           /* Nodal a2 */
    VAL_PROJECTED_VEC,     /* Projected vector magnitude */
    VAL_ALL_END
};
#endif


/* Material manager function button values. */
typedef enum
{
    VIS,
    INVIS,
    ENABLE,
    DISABLE,
    COLOR,
    MTL_FUNC_QTY
} Mtl_mgr_func_type;

/* Material manager global material selection button values. */
typedef enum
{
    ALL_MTL,
    NONE,
    INVERT,
    BY_FUNC
} Mtl_mgr_glo_sel_type;

/* Material manager function operation button values. */
typedef enum
{
    OP_PREVIEW,
    OP_CANCEL,
    OP_APPLY,
    OP_DEFAULT,
    MTL_OP_QTY
} Mtl_mgr_op_type;

/* Material manager color scales. */
typedef enum
{
    RED_SCALE,
    GREEN_SCALE,
    BLUE_SCALE,
    SHININESS_SCALE
} Color_editor_scale_type;

/* Surface manager function button values. */
typedef enum
{
    VIS_SURF,
    INVIS_SURF,
    ENABLE_SURF,
    DISABLE_SURF,
    SURF_FUNC_QTY
} Surf_mgr_func_type;

/* Surface manager global selection button values. */
typedef enum
{
    ALL_SURF,
    NONE_SURF,
    INVERT_SURF,
    BY_FUNC_SURF
} Surf_mgr_glo_sel_type;

/* Surface manager function operation button values. */
typedef enum
{
    SURF_OP_APPLY,
    SURF_OP_DEFAULT,
    SURF_OP_QTY
} Surf_mgr_op_type;


/*****************************************************************
 * TAG( Widget_list_obj )
 *
 * Node for a linked list of widget handles.
 */
typedef struct _Widget_list_obj
{
    struct _Widget_list_obj *next;
    struct _Widget_list_obj *prev;
    Widget handle;
} Widget_list_obj;

/*****************************************************************
 * TAG( Material_list_obj )
 *
 * Node for a linked list of material numbers.
 */
typedef struct _Material_list_obj
{
    struct _Material_list_obj *next;
    struct _Material_list_obj *prev;
    int mtl;
} Material_list_obj;

/*****************************************************************
 * TAG( Surface_list_obj )
 *
 * Node for a linked list of material numbers.
 */
typedef struct _Surface_list_obj
{
    struct _Surface_list_obj *next;
    struct _Surface_list_obj *prev;
    int surf;
} Surface_list_obj;


#define OK 0


/* Mouse modes. */
#define MOUSE_STATIC 0
#define MOUSE_MOVE 1

/* Some handles that need to stay around. */
static XtAppContext app_context;
static Display *dpy;
Display *dpy_copy;
static Widget ctl_shell_widg = NULL;
static Widget rendershell_widg = NULL;
static Widget command_widg = NULL;
static Widget render_form_widg = NULL;
static Widget plot_coord_widg = NULL;
static Widget x_coord_widg = NULL;
static Widget y_coord_widg = NULL;
static Widget menu_widg = NULL;
static Widget ctl_menu_pane = NULL;
static Widget derived_menu_widg = NULL;
static Widget primal_menu_widg = NULL;
static Widget ti_menu_widg = NULL;
static Widget monitor_widg = NULL;
static Widget mtl_mgr_widg = NULL;
static Widget mtl_mgr_button = NULL;
static Widget mtl_base = NULL;
static Widget surf_mgr_widg = NULL;
static Widget surf_mgr_button = NULL;
static Widget surf_base = NULL;
static Widget util_button = NULL;
static Widget quit_button = NULL;
static Widget *mtl_mgr_func_toggles = NULL;
static Widget surf_mgr_func_toggles[SURF_FUNC_QTY];
static Widget util_panel_widg = NULL;
static Widget mtl_row_col = NULL;
static Widget surf_row_col = NULL;
static Widget mtl_quick_select = NULL;
static Widget surf_quick_select = NULL;
static Widget color_editor = NULL;
/*static Widget color_comps[EMISSIVE + 1];*/
static Widget color_comps[MTL_PROP_QTY];
static Widget prop_checks[MTL_PROP_QTY];
static Widget col_ed_scales[2][4];
static Widget swatch_label = NULL;
static Widget *op_buttons = NULL;
static Widget surf_op_buttons[SURF_OP_QTY];
static Widget swatch_frame = NULL;
static Widget util_panel_main = NULL;
static Widget util_render_ctl = NULL;
static Widget util_state_ctl = NULL;
static Widget *util_render_btns = NULL;
static Widget stride_label = NULL;
static Widget setpick_menu1_widg = NULL;
static Widget setpick_menu2_widg = NULL;
static Widget setpick_menu3_widg = NULL;
static Widget colormap_menu_widg = NULL;

typedef enum _shell_win_type
{
    CONTROL_SHELL_WIN,
    RENDER_SHELL_WIN,
    UTIL_PANEL_SHELL_WIN,
    MTL_MGR_SHELL_WIN,
    SURF_MGR_SHELL_WIN
} Shell_win_type;

static Window ctl_top_win = 0;
static Window render_top_win = 0;
static Window mtl_mgr_top_win = 0;
static Window surf_mgr_top_win = 0;
static Window util_panel_top_win = 0;

/* Griz Window attributes */
static XWindowAttributes ctl_attrib, render_attrib, mtl_atrib, surf_attrib, util_attrib;

/* Parameterized names for derived and primal result menus. */
static char *derived_menu_name = "Derived";
static char *primal_menu_name  = "Primal";
static char *ti_menu_name      = "Time-Indep";

/* Translation specification for "global" translations. */
static char action_spec[512];

/* OpenGL drawables. */
static Widget ogl_widg[NO_OGL_WIN] = { NULL };

/* Graphics contexts for the OpenGL drawable windows. */
static GLXContext render_ctx;
static GLXContext swatch_ctx;

/* Visual info. */
static XVisualInfo *vi;

/* Current rendering window. */
static OpenGL_win cur_opengl_win = NO_OGL_WIN;

/* Current mtl manager color editor color and shininess. */
static GLfloat cur_color[4];

/* Current mtl manager color editor color component. */
static Material_property_type cur_mtl_comp;

/* Booleans to indicate if a material property value has changed. */
static Bool_type prop_val_changed[MTL_PROP_QTY];

/* Booleans to indicate if a material property change is being previewed. */
static Bool_type preview_set = FALSE;

/* Storage for new values of material properties. */
static GLfloat *property_vals[MTL_PROP_QTY];

/* Material manager command buffer. */
char *mtl_mgr_cmd;

/* Surface manager command buffer. */
char *surf_mgr_cmd;

/* Pixmaps. */
static Pixmap mtl_check, surf_check,
       pixmap_start, pixmap_stop, pixmap_leftstop,
       pixmap_left, pixmap_right, pixmap_rightstop;

/* Lists for select and deselected materials for material manager. */
static Material_list_obj *mtl_select_list = NULL;
static Material_list_obj *mtl_deselect_list = NULL;

/* Lists for select and deselected surfaces for surface manager. */
static Surface_list_obj *surf_select_list = NULL;
static Surface_list_obj *surf_deselect_list = NULL;

/* Stride value for state stepping buttons of Utility panel. */
static int step_stride = 1;

/* Indicator of Utility Panel "Edges Only" callback activity. */
static Bool_type util_panel_CB_active = FALSE;

/* User-selected classes for mouse picks - save for new utility panels. */
static MO_class_data *btn1_mo_class = NULL;
static MO_class_data *btn2_mo_class = NULL;
static MO_class_data *btn3_mo_class = NULL;

/* An ordered list of superclass types used in pick-class selections. */
/**/
/* NOTE - G_UNIT should be replaced with G_PARTICLE when/if available!!! */
static int pick_sclasses[] =
{
    G_NODE, G_UNIT, G_TRUSS, G_BEAM, G_TRI, G_QUAD, G_TET, G_PYRAMID, G_WEDGE,
    G_HEX, G_SURFACE, G_PARTICLE
};

/* List for warning dialog handles. */
static Widget_list_obj *popup_dialog_list = NULL;
static int popup_dialog_count = 0;
#define MAX_WARNING_DIALOGS 10

/* Rendering window size. */
static Dimension window_width = DEFAULT_WIDTH;
static Dimension window_height = DEFAULT_HEIGHT;

/* Keep track of text position in monitor window. */
static XmTextPosition wpr_position;

/* Flag that GUI widgets are up and can be sent text. */
static Bool_type gui_up = FALSE;

/* Text history file stuff. */
FILE *text_history_file = NULL;
Bool_type text_history_invoked = FALSE;

/* Non-default cursors. */
static Cursor alt_cursors[CURSOR_QTY];

/* Mouse motion/pick threshold. */
static float motion_threshold = 0.0;

/*
 * Resource settings for GL choose visual.  Demand full 24 bit
 * color -- otherwise you can get yucky dithering.
 */
/*
static int single_buf[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16,
                            GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
                            GLX_BLUE_SIZE, 8, GLX_STENCIL_SIZE, 1, None };
static int double_buf[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER,
                            GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4,
                            GLX_BLUE_SIZE, 4, GLX_STENCIL_SIZE, 1, None };
static int single_buf_no_z[] = { GLX_RGBA, GLX_RED_SIZE, 8,
                                 GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
                                 GLX_STENCIL_SIZE, 1, None };
static int double_buf_no_z[] = { GLX_RGBA, GLX_RED_SIZE, 4,
                                 GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4,
                                 GLX_STENCIL_SIZE, 1, GLX_DOUBLEBUFFER, None };
*/
/*
 * glXChooseVisual() should return the maximum buffers supported on the
 * machine.  Apparently, this was buggy in IRIX 5.1.
 */
static int single_buf[] = { GLX_RGBA, GLX_DEPTH_SIZE, 1,
                            GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1,
                            GLX_BLUE_SIZE, 1, GLX_STENCIL_SIZE, 1, None
                          };
static int double_buf[] = { GLX_RGBA, GLX_DEPTH_SIZE, 1, GLX_DOUBLEBUFFER,
                            GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1,
                            GLX_BLUE_SIZE, 1, GLX_STENCIL_SIZE, 1, None
                          };
static int single_buf_no_z[] = { GLX_RGBA, GLX_RED_SIZE, 1,
                                 GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
                                 GLX_STENCIL_SIZE, 1, None
                               };
static int double_buf_no_z[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1,
                                 GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
                                 GLX_STENCIL_SIZE, 1, None
                               };

/*
 * ID for the animation workproc.
 */
static XtWorkProcId anim_work_proc_id = 0;
static Boolean stop_animate;

test()
{
    dpy = XtDisplay( ctl_shell_widg );
    vi = glXChooseVisual( dpy, DefaultScreen( dpy ), single_buf );
    exit(1);
}
/*
 *  Rubberband Zoom Variables
 *   Added January 17, 2006: IRC
 */

#define RECTANGLE   1
#define RB_MOVE    10
#define RB_STATIC  11

#define RB_MAX_HIST 20

static GC gc_rubber;
static XGCValues xgcvalues;
static Pixel p1;
static XColor    rb_color, rb_dummy;
static Colormap  rb_cmap;
static Status    rb_status;

static XColor border_color, border_dummy;

static int line_style = LineSolid;  /* LineSolid */
static int cap_style  = CapButt;  /* CapRound */
static int join_style = 0;  /* JoinRound */
static int line_width = 1; /* Pixels */

static int screen_num;
static int obj_kind = RECTANGLE;
static int startX, startY, lastX, lastY;

static float rb_zoom_hist[RB_MAX_HIST][3];
static int   rb_node_hist[RB_MAX_HIST];
static int   rb_hist_index = 0;
static int   rb_node_num;

static Position  ctl_x, ctl_y;
static Dimension ctl_width, ctl_height;
float rb_dx, rb_dy, temp_x, temp_y;

/*****************************************************************
 * TAG( gui_start )
 *
 * Open the gui main windows and initialize everything, then start
 * the gui event loop.
 */
void
gui_start( int argc, char **argv , Analysis *analy )
{
    Widget mainwin_widg, pane, x_label_widg, y_label_widg;
    Drawable d1;
    XmString command_label;
    Arg args[20];
    char title[512], path[256]="";
    int n;
    XtActionsRec global_actions[3];
    Atom  WM_DELETE_WINDOW;
    int gid=0;
    int status;

    init_griz_name( analy );

    /* Read in the session initialization data */

    /*
     * Create an application shell to hold all of the interface except
     * the rendering window.
     */

    if ( !analy->path_found )
        strcpy( path, analy->path_name );

    if ( env.bname )
        sprintf( title, "Control:  %s%s", path_string, env.bname );
    else
        sprintf( title, "Control:  %s%s", path_string, env.plotfile_name );

    n = 0;
    XtSetArg( args[n], XmNiconic, FALSE );
    n++;
    XtSetArg( args[n], XmNiconName, "GRIZ" );
    n++;
    XtSetArg( args[n], XmNtitle, title );
    n++;
    XtSetArg( args[n], XmNallowShellResize, TRUE );
    n++;

    /*
    The XtAppInitialize function calls XtToolkitInitialize followed by XtCreateApplicationContext, then calls XtOpenDisplay with display_string NULL and application_name NULL, and finally calls XtAppCreateShell with application_name NULL, widget_class applicationShellWidgetClass, and the specified args and num_args and returns the created shell. The modified argc and argv returned by XtDisplayInitialize are returned in argc_in_out and argv_in_out. If app_context_return is not NULL, the created application context is also returned. If the display specified by the command line cannot be opened, an error message is issued and XtAppInitialize terminates the application. If fallback_resources is non-NULL, XtAppSetFallbackResources is called with the value prior to calling XtOpenDisplay.

    */
    ctl_shell_widg = XtAppInitialize( &app_context, "GRIZ",
                                      (XrmOptionDescList)NULL , 0,
                                      &argc,
                                      (String*)argv,
                                      fallback_resources,
                                      args, n );

    /* Record the display for use elsewhere. */
    dpy = XtDisplay( ctl_shell_widg );
    dpy_copy = dpy;
    defineDialogColor( dpy );

    if ( env.griz_id>0 )
        defineBorderColor( dpy );

    /*
     * Link the window manager "close" function to the exit callback so
     * it will be called if the user kills GRIZ via the window manager.
     */
    WM_DELETE_WINDOW = XmInternAtom( dpy, "WM_DELETE_WINDOW", False );
    XmAddWMProtocolCallback( ctl_shell_widg, WM_DELETE_WINDOW,
                             (XtCallbackProc) exit_CB, NULL );

    n = 0;
    mainwin_widg = XtCreateManagedWidget( "mainw",
                                          xmMainWindowWidgetClass,
                                          ctl_shell_widg, args, n );

    menu_widg = create_menu_bar( mainwin_widg, analy );

    /*
     * Everything else for the control window goes onto a pane.
     */

    n = 0;
    if ( env.griz_id>0 )
    {
        gid = env.griz_id;
        XtSetArg( args[n], XmNallowResize, TRUE );
        n++;
        XtSetArg( args[n], XmNmarginWidth, 2 );
        n++;
        XtSetArg( args[n], XmNmarginHeight, 2 );
        n++;
        XtSetArg( args[n],  XmNbackground,  env.border_colors[gid-1] );
        n++;
    }

    pane = XmCreatePanedWindow( mainwin_widg, "pane", args, n );

    n = 0;
    XtSetArg( args[n], XmNeditable, FALSE );
    n++;
    XtSetArg( args[n], XmNeditMode, XmMULTI_LINE_EDIT );
    n++;
    XtSetArg( args[n], XmNautoShowCursorPosition, FALSE );
    n++;
    XtSetArg( args[n], XmNscrollingPolicy, XmAUTOMATIC );
    n++;
    monitor_widg = XmCreateScrolledText( pane, "monitor", args, n );
    XtManageChild( monitor_widg );

    /* Include other panels in Control Panel if requested on command line */

    if ( include_util_panel )
        util_panel_widg = create_utility_panel( pane );

    if ( include_mtl_panel )
        mtl_mgr_widg = create_mtl_manager( pane );

    command_label = XmStringCreateLocalized( "Command:" );
    n = 0;
    /* XtSetArg( args[n], XmNheight, 150 ); n++; */

    /* History display is set to 25 */
    XtSetArg( args[n], XmNhistoryVisibleItemCount, 25 );
    n++;
    XtSetArg( args[n], XmNselectionLabelString, command_label );
    n++;
    XtSetArg( args[n], XmNskipAdjust, True );
    n++;

    command_widg = XmCreateCommand( pane, "command", args, n );
    XtAddCallback( command_widg, XmNcommandEnteredCallback, parse_CB, NULL );
    XtManageChild( command_widg );
    XtManageChild( pane );

    /* Set initial keyboard focus to the command widget. */
    XtVaSetValues( pane, XmNinitialFocus, command_widg, NULL );

    XmMainWindowSetAreas( mainwin_widg, menu_widg, NULL, NULL, NULL, pane );

    /* The control widget gets realized down below. */

    /* Start-up notices go into the monitor window. */
    write_start_text();

    /*
     * Have the control window, now build the rendering window.
     */

    /* Create a topLevelShell for the rendering window. */

    if ( env.bname )
    {
        sprintf( title, "Render:  %s%s", path_string, env.bname );
    }
    else
        sprintf( title, "Render:  %s%s", path_string, env.plotfile_name );

    n = 0;
    XtSetArg( args[n], XtNtitle, title );
    n++;
    XtSetArg( args[n], XmNiconic, FALSE );
    n++;
    XtSetArg( args[n], XmNwindowGroup, XtWindow( mainwin_widg ) );
    n++;
    XtSetArg( args[n], XmNdeleteResponse, XmDO_NOTHING );
    n++;
    XtSetArg( args[n], XmNbackground, XBlackPixel( dpy, DefaultScreen( dpy ) ));
    n++;

    rendershell_widg = XtAppCreateShell( NULL, "rendershell",
                                         topLevelShellWidgetClass,
                                         XtDisplay( mainwin_widg ), args, n );

    /* Find an OpenGL-capable RGB visual with depth buffer. */

    vi = NULL;
    if ( env.curr_analy->dimension == 3 )
    {
        if ( !env.single_buffer )
            vi = glXChooseVisual( dpy, DefaultScreen( dpy ), double_buf );
        if ( vi == NULL )
            vi = glXChooseVisual( dpy, DefaultScreen( dpy ), single_buf );
        if ( vi == NULL )
            popup_fatal( "No RGB visual with depth buffer.\n" );
    }
    else  /* Don't use Z buffer for 2D drawing. */
    {
        if ( !env.single_buffer )
            vi = glXChooseVisual( dpy, DefaultScreen( dpy ), double_buf_no_z );
        if ( vi == NULL )
            vi = glXChooseVisual( dpy, DefaultScreen( dpy ), single_buf_no_z );
        if ( vi == NULL )
            popup_fatal( "No RGB visual with depth buffer.\n" );
    }

    /* Create an OpenGL rendering context. */
    render_ctx = glXCreateContext( dpy, vi, None, GL_TRUE );

    if ( render_ctx == NULL )
        popup_fatal( "Could not create rendering context.\n" );

    if ( env.griz_id>0 )
        render_form_widg = XtVaCreateManagedWidget(
                               "renderform", xmFormWidgetClass, rendershell_widg,
                               XmNborderWidth, 20,
                               XmNmarginWidth, 4,
                               XmNmarginHeight, 4,
                               XmNbackground, env.border_colors[gid-1],
                               NULL );
    else
        render_form_widg = XtVaCreateManagedWidget(
                               "renderform", xmFormWidgetClass, rendershell_widg,
                               NULL );

    /* Create a RowColumn for plot cursor coordinates display. */
    plot_coord_widg = XtVaCreateWidget(
                          "plotcoords", xmRowColumnWidgetClass, render_form_widg,
                          XmNorientation, XmHORIZONTAL,
                          XmNtopAttachment, XmATTACH_NONE,
                          XmNleftAttachment, XmATTACH_FORM,
                          XmNrightAttachment, XmATTACH_FORM,
                          XmNbottomAttachment, XmATTACH_FORM,
                          NULL );

    /*"Values at cursor:   X", xmLabelGadgetClass, plot_coord_widg, */
    x_label_widg = XtVaCreateManagedWidget(
                       "Cursor X value: ", xmLabelGadgetClass, plot_coord_widg,
                       NULL );
    x_coord_widg = XtVaCreateManagedWidget(
                       "xplotcoord", xmTextFieldWidgetClass, plot_coord_widg,
                       XmNcolumns, 12,
                       XmNeditable, False,
                       XmNcursorPositionVisible, False,
                       XmNtraversalOn, False,
                       XmNbackground, XWhitePixel( dpy, DefaultScreen( dpy ) ),
                       NULL );
    y_label_widg = XtVaCreateManagedWidget(
                       " Cursor Y value:", xmLabelGadgetClass, plot_coord_widg,
                       NULL );
    y_coord_widg = XtVaCreateManagedWidget(
                       "yplotcoord", xmTextFieldWidgetClass, plot_coord_widg,
                       XmNcolumns, 12,
                       XmNeditable, False,
                       XmNcursorPositionVisible, False,
                       XmNtraversalOn, False,
                       XmNbackground, XWhitePixel( dpy, DefaultScreen( dpy ) ),
                       NULL );

    /* Create the OpenGL drawing widget. */
    ogl_widg[MESH_VIEW] = XtVaCreateManagedWidget(
                              "glwidget",
#ifdef GLWM
                              glwMDrawingAreaWidgetClass,
#else
                              glwDrawingAreaWidgetClass,
#endif
                              render_form_widg,
                              XmNtopAttachment, XmATTACH_FORM,
                              XmNleftAttachment, XmATTACH_FORM,
                              XmNrightAttachment, XmATTACH_FORM,
                              XmNbottomAttachment, XmATTACH_FORM,
                              GLwNvisualInfo, vi,
                              XmNwidth, window_width, XmNheight, window_height,
                              NULL );
#ifdef USE_OLD_CALLBACKS
    XtAddCallback( ogl_widg[MESH_VIEW], GLwNexposeCallback, expose_CB, 0 );
    XtAddCallback( ogl_widg[MESH_VIEW], GLwNresizeCallback, resize_CB, 0 );
#else
    XtAddCallback( ogl_widg[MESH_VIEW], GLwNexposeCallback,
                   expose_resize_CB, 0 );
    XtAddCallback( ogl_widg[MESH_VIEW], GLwNresizeCallback,
                   expose_resize_CB, 0 );
#endif
    XtAddCallback( ogl_widg[MESH_VIEW], GLwNinputCallback, input_CB, 0 );
    XtManageChild( ogl_widg[MESH_VIEW] );

    XtAddEventHandler( ogl_widg[MESH_VIEW], ExposureMask, False,
                       stack_init_EH, (XtPointer) RENDER_SHELL_WIN );

    XtAddEventHandler( ogl_widg[MESH_VIEW],
                       EnterWindowMask | LeaveWindowMask, False,
                       enter_render_EH, NULL );

    /*
     * Define translations for creation of Material Manager and
     * Utility Panel (if not part of Control window) and keyboard
     * input to the command widget from other widgets.
     */
    global_actions[0].string = "action_create_app_widg";
    global_actions[0].proc = (XtActionProc) action_create_app_widg;
    global_actions[1].string = "action_translate_command";
    global_actions[1].proc = (XtActionProc) action_translate_command;
    global_actions[2].string = "action_quit";
    global_actions[2].proc = (XtActionProc) action_quit;
    XtAppAddActions( app_context, global_actions, 3 );

    if ( !include_util_panel )
    {
        sprintf( action_spec,
                 "Ctrl<Key>m: action_create_app_widg( %d ) \n ",
                 BTN_MTL_MGR );
        sprintf( action_spec + strlen( action_spec ),
                 "Ctrl<Key>s: action_create_app_widg( %d ) \n ",
                 BTN_SURF_MGR );
        sprintf( action_spec + strlen( action_spec ),
                 "Ctrl<Key>u: action_create_app_widg( %d ) \n ",
                 BTN_UTIL_PANEL );
        sprintf( action_spec + strlen( action_spec ),
                 "Ctrl<Key>q: action_quit() \n " );
        sprintf( action_spec + strlen( action_spec ),
                 "~Ctrl <Key>: action_translate_command()" );
    }
    else
    {
        sprintf( action_spec,
                 "Ctrl<Key>m: action_create_app_widg( %d ) \n ",
                 BTN_MTL_MGR );
        sprintf( action_spec + strlen( action_spec ),
                 "Ctrl<Key>s: action_create_app_widg( %d ) \n ",
                 BTN_SURF_MGR );
        sprintf( action_spec + strlen( action_spec ),
                 "Ctrl<Key>q: action_quit() \n " );
        sprintf( action_spec + strlen( action_spec ),
                 "~Ctrl <Key>: action_translate_command()" );
    }

    /* Add the translations to the rendering window. */
    XtOverrideTranslations( ogl_widg[MESH_VIEW],
                            XtParseTranslationTable( action_spec ) );

    /* Bring up the control window last. */
    XtPopup( rendershell_widg, XtGrabNone );
    XtRealizeWidget( ctl_shell_widg );

    /* Rubber Band Zoom */
    screen_num = XDefaultScreen(dpy);
    d1         = XtWindow(ogl_widg[MESH_VIEW]);

    XAllocNamedColor(dpy, XDefaultColormap(dpy, screen_num), "black", &rb_color, &rb_dummy);
    xgcvalues.background = WhitePixel(dpy, screen_num) ;
    xgcvalues.foreground = BlackPixel(dpy, screen_num) ;

    gc_rubber = XDefaultGC(dpy, screen_num);

    XtVaGetValues ( ogl_widg[MESH_VIEW],
                    XmNforeground, &xgcvalues.foreground,
                    XmNbackground, &xgcvalues.background,
                    NULL );

    /* Set the rubber band gc to use XOR mode and draw a solid line. */

    xgcvalues.line_style = LineOnOffDash;
    xgcvalues.foreground = xgcvalues.foreground ^ xgcvalues.background;
    xgcvalues.line_width = 1; /* pixels */
    xgcvalues.function   = GXxor;

    gc_rubber = XtGetGC ( ogl_widg[MESH_VIEW], GCForeground | GCBackground |
                          GCFunction | GCLineStyle | GCLineWidth,
                          &xgcvalues );

    /* Rubber Band Zoom */

    gui_up = TRUE;

    /*
     * Bind the mesh rendering context and window.
     */
    sleep(1);
    switch_opengl_win( MESH_VIEW );
    init_gui();

    init_alt_cursors();

    init_btn_pick_classes();


    /* Bring in other Apps if they were defaulted tp come up
     * in the session file.
     */
    if ( !include_util_panel )
    {

        if ( session->win_util_active )
            create_app_widg( BTN_UTIL_PANEL );
    }

    if ( session->win_mtl_active )
        create_app_widg( BTN_MTL_MGR );

    /* Read in Global preferences */
    status = read_griz_session_file( session, ".griz_session",
                                     env.griz_id, TRUE );

    if ( status==OK )
    {
        /* Update the window attributes */
        put_window_attributes() ;
        put_griz_session( env.curr_analy, session );
    }

    /* Start event processing. */
    XtAppMainLoop( app_context );
}


/*****************************************************************
 * TAG( find_ancestral_root_child )
 *
 * Follow the ancestral line of parent windows for a widget to
 * find the window which is the child of the display's root
 * window.
 */
static void
find_ancestral_root_child( Widget widg, Window *root_child )
{
    Window root_win, root_return, parent_win, win;
    Window *children;
    unsigned int qty_children;
    Status stat;

    root_win = RootWindow( dpy, DefaultScreen( dpy ) );

    win = XtWindow( widg );
    stat = XQueryTree( dpy, win, &root_return, &parent_win, &children,
                       &qty_children );
    if ( stat == 0 )
        return;

    XFree( children );

    while ( parent_win != root_win )
    {
        win = parent_win;
        stat = XQueryTree( dpy, win, &root_return, &parent_win, &children,
                           &qty_children );
        if ( stat == 0 )
            return;

        XFree( children );
    }

    *root_child = win;
}


/*****************************************************************
 * TAG( move_bottom )
 *
 * Adjust the offset of the OpenGL drawing widget from the bottom
 * of the Form that manages it.
 */
void
move_bottom( int new_offset )
{
    XtVaSetValues( ogl_widg[MESH_VIEW], XmNbottomOffset, new_offset, NULL );
}


#ifdef SERIAL_BATCH
/*****************************************************************
 * TAG( init_app_context_serial_batch )
 *
 * Initialize application context for use with "anim" command
 * in serial, batch mode
 */
void
init_app_context_serial_batch( int argc, char *argv[] )
{
    ctl_shell_widg = XtAppInitialize(
                         &app_context,
                         "GRIZ",
                         (XrmOptionDescList) NULL,
                         0,
                         &argc,
                         (String*) argv,
                         fallback_resources,
                         (ArgList) NULL,
                         0
                     );
}
#endif


/*****************************************************************
 * TAG( gui_swap_buffers )
 *
 * Swap buffers in the current OpenGL window.
 */
void
gui_swap_buffers( void )
{
#ifdef SERIAL_BATCH
#else

    GLwDrawingAreaSwapBuffers( ogl_widg[cur_opengl_win] );

#endif /* SERIAL_BATCH */
}


/*****************************************************************
 * TAG( create_menu_bar )
 *
 * Create the menu bar for the main window.
 */
static Widget
create_menu_bar( Widget parent, Analysis *analy )
{
    Widget menu_bar;
    Widget cascade;
    Widget menu_pane;
    Widget button;
    Widget colormap_menu;
    Arg args[10];
    int n;
    XmString accel_str;

    n = 0;
    XtSetArg( args[n], XmNx, 0 );
    n++;
    XtSetArg( args[n], XmNx, 0 );
    n++;
    menu_bar = XmCreateMenuBar( parent, "menu_bar", args, n );
    XtManageChild( menu_bar );

    /****************/
    /* Control menu.*/
    /****************/
    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    ctl_menu_pane = XmCreatePulldownMenu( menu_bar, "ctl_menu_pane", args, n );

    button = XmCreatePushButtonGadget( ctl_menu_pane, "Copyright", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_COPYRIGHT );

    /* If utility panel not part of control window, allow it from pulldown. */
    if ( !include_util_panel )
    {
        accel_str = XmStringCreateSimple( "Ctrl+u" );
        n = 0;
        XtSetArg( args[n], XmNaccelerator, "Ctrl<Key>u" );
        n++;
        XtSetArg( args[n], XmNacceleratorText, accel_str );
        n++;
        util_button = XmCreatePushButtonGadget( ctl_menu_pane, "Utility Panel",
                                                args, n );
        XmStringFree( accel_str );
        XtManageChild( util_button );
        XtAddCallback( util_button, XmNactivateCallback, menu_CB,
                       (XtPointer) BTN_UTIL_PANEL );
    }

    accel_str = XmStringCreateSimple( "Ctrl+m" );
    n = 0;
    XtSetArg( args[n], XmNaccelerator, "Ctrl<Key>m" );
    n++;
    XtSetArg( args[n], XmNacceleratorText, accel_str );
    n++;
    mtl_mgr_button = XmCreatePushButtonGadget( ctl_menu_pane, "Material Mgr",
                     args, n );
    XmStringFree( accel_str );
    XtManageChild( mtl_mgr_button );
    XtAddCallback( mtl_mgr_button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_MTL_MGR );

    if( env.curr_analy->mesh_table[0].surface_qty > 0 )
    {
        accel_str = XmStringCreateSimple( "Ctrl+s" );
        n = 0;
        XtSetArg( args[n], XmNaccelerator, "Ctrl<Key>s" );
        n++;
        XtSetArg( args[n], XmNacceleratorText, accel_str );
        n++;
        surf_mgr_button = XmCreatePushButtonGadget( ctl_menu_pane, "Surface Mgr",
                          args, n );
        XmStringFree( accel_str );
        XtManageChild( surf_mgr_button );
        XtAddCallback( surf_mgr_button, XmNactivateCallback, menu_CB,
                       (XtPointer) BTN_SURF_MGR );
    }

    n = 0;
    button = XmCreateSeparatorGadget( ctl_menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( ctl_menu_pane, "Save Session - Global", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_SAVE_SESSION_GLOBAL );

    button = XmCreatePushButtonGadget( ctl_menu_pane, "Save Session - Plotfile", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_SAVE_SESSION_PLOT );


    button = XmCreateSeparatorGadget( ctl_menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( ctl_menu_pane, "Load Session - Global", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_LOAD_SESSION_GLOBAL );

    button = XmCreatePushButtonGadget( ctl_menu_pane, "Load Session - Plotfile", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_LOAD_SESSION_PLOT );

    button = XmCreateSeparatorGadget( ctl_menu_pane, "separator", args, n );
    XtManageChild( button );

    accel_str = XmStringCreateSimple( "Ctrl+q" );
    n = 0;
    XtSetArg( args[n], XmNaccelerator, "Ctrl<Key>q" );
    n++;
    XtSetArg( args[n], XmNacceleratorText, accel_str );
    n++;
    quit_button = XmCreatePushButtonGadget( ctl_menu_pane, "Quit", args, n );
    XmStringFree( accel_str );
    XtManageChild( quit_button );
    XtAddCallback( quit_button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_QUIT );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, ctl_menu_pane );
    n++;
    cascade = XmCreateCascadeButton( menu_bar, "Control", args, n );
    XtManageChild( cascade );

    /******************/
    /* Rendering menu.*/
    /******************/
    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Draw Solid", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_DRAWFILLED );

    button = XmCreatePushButtonGadget( menu_pane, "Draw Hidden", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_DRAWHIDDEN );

    button = XmCreatePushButtonGadget( menu_pane, "Draw Wireframe", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_DRAWWIREFRAME );

    button = XmCreatePushButtonGadget( menu_pane, "Draw Wireframe Transparent", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_DRAWWIREFRAMETRANS );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Coord Sys On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_COORDSYS );

    button = XmCreatePushButtonGadget( menu_pane, "Title On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_TITLE );

    button = XmCreatePushButtonGadget( menu_pane, "Time On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_TIME );

    button = XmCreatePushButtonGadget( menu_pane, "Colormap On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_COLORMAP );

    button = XmCreatePushButtonGadget( menu_pane, "Min/max On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_MINMAX );

    button = XmCreatePushButtonGadget( menu_pane, "Disp Scale On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_SCALE );

    button = XmCreatePushButtonGadget( menu_pane, "Error Indicator On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_EI );

    button = XmCreatePushButtonGadget( menu_pane, "All On", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_ALLON );

    button = XmCreatePushButtonGadget( menu_pane, "All Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_ALLOFF );

    button = XmCreatePushButtonGadget( menu_pane, "Bound Box On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_BBOX );

    button = XmCreatePushButtonGadget( menu_pane, "Edges On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_EDGES );

    button = XmCreatePushButtonGadget( menu_pane, "Greyscale On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_GS );


    if ( analy->free_nodes_found || analy->particle_nodes_found )
    {
        button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
        XtManageChild( button );

        if ( analy->free_nodes_found )
        {
            button = XmCreatePushButtonGadget( menu_pane, "Free Nodes On/Off", args, n );
            XtManageChild( button );
            XtAddCallback( button, XmNactivateCallback, menu_CB,
                           (XtPointer) BTN_FN );
        }

        if ( analy->particle_nodes_found )
        {
            button = XmCreatePushButtonGadget( menu_pane, "Particle Nodes On/Off", args, n );
            XtManageChild( button );
            XtAddCallback( button, XmNactivateCallback, menu_CB,
                           (XtPointer) BTN_PN );
        }
    }

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Perspective", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_PERSPECTIVE );

    button = XmCreatePushButtonGadget( menu_pane, "Orthographic", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_ORTHOGRAPHIC );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Adjust Near/Far", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_ADJUSTNF );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Reset View", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_RESETVIEW );

    button = XmCreatePushButtonGadget( menu_pane, "Supress Screen Refresh", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_SU );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    colormap_menu_widg = create_colormap_menu( menu_pane, BTN_CM_PICK,
                         "Set Colormap" );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane );
    n++;
    cascade = XmCreateCascadeButton( menu_bar, "Rendering", args, n );
    XtManageChild( cascade );

    /****************/
    /* Picking menu.*/
    /****************/
    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Mouse Hilite", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_HILITE );

    button = XmCreatePushButtonGadget( menu_pane, "Mouse Select", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_SELECT );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Clear Hilite", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_CLEARHILITE );

    button = XmCreatePushButtonGadget( menu_pane, "Clear Select", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_CLEARSELECT );

    button = XmCreatePushButtonGadget( menu_pane, "Clear All", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_CLEARALL );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    setpick_menu1_widg = create_pick_menu( menu_pane, BTN1_PICK,
                                           "Set Btn 1 Pick" );
    setpick_menu2_widg = create_pick_menu( menu_pane, BTN2_PICK,
                                           "Set Btn 2 Pick" );
    setpick_menu3_widg = create_pick_menu( menu_pane, BTN3_PICK,
                                           "Set Btn 3 Pick" );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Center Hilite On", args, n);
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_CENTERON );

    button = XmCreatePushButtonGadget( menu_pane, "Center Hilite Off", args, n);
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_CENTEROFF );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane );
    n++;
    cascade = XmCreateCascadeButton( menu_bar, "Picking", args, n );
    XtManageChild( cascade );

    /************************************/
    /* Build db-sensitive result menus. */
    /************************************/
    create_result_menus( menu_bar );

    /* Time menu. */
    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Next State", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_NEXTSTATE );

    button = XmCreatePushButtonGadget( menu_pane, "Prev State", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_PREVSTATE );

    button = XmCreatePushButtonGadget( menu_pane, "First State", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_FIRSTSTATE );

    button = XmCreatePushButtonGadget( menu_pane, "Last State", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_LASTSTATE );

    button = XmCreatePushButtonGadget( menu_pane, "Animate States", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_ANIMATE );

    button = XmCreatePushButtonGadget( menu_pane, "Stop Animate", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_STOPANIMATE );

    button = XmCreatePushButtonGadget( menu_pane, "Continue Animate", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_CONTANIMATE );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane );
    n++;
    cascade = XmCreateCascadeButton( menu_bar, "Time", args, n );
    XtManageChild( cascade );

    /**************/
    /* Plot menu. */
    /**************/
    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Time Hist Plot", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_TIMEPLOT );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane );
    n++;
    cascade = XmCreateCascadeButton( menu_bar, "Plot", args, n );
    XtManageChild( cascade );

    /**************/
    /* Help menu. */
    /**************/
    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Display Griz Manual", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_HELP );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Display Release Notes", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_RELNOTES );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane );
    n++;
    cascade = XmCreateCascadeButton( menu_bar, "Help", args, n );

    XtManageChild( cascade );

    return( menu_bar );
}


/*****************************************************************
 * TAG( create_result_menus )
 *
 * Create the results menu(s) for the Control window menu bar.
 */
static void
create_result_menus( Widget parent )
{
    int n;
    Arg args[10];
    Widget cascade;
    Analysis *analy;

    analy = env.curr_analy;

    /* Create new result menus. */
    create_derived_res_menu( parent );
    create_primal_res_menu( parent );

    /* Bring up TI menus if TI data is found */
    if (  analy->ti_data_found )
        create_ti_res_menu( parent );

    /* Create cascade buttons for the new menus. */
    n = 0;
    XtSetArg( args[n], XmNsubMenuId, derived_menu_widg );
    n++;
    cascade = XmCreateCascadeButton( parent, derived_menu_name,
                                     args, n );
    XtManageChild( cascade );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, primal_menu_widg );
    n++;
    cascade = XmCreateCascadeButton( parent, primal_menu_name, args, n );
    XtManageChild( cascade );

    /* Create cascade buttons for TI menus */
#ifdef TIGUI
    if (  analy->ti_data_found )
    {
        n = 0;
        XtSetArg( args[n], XmNsubMenuId, ti_menu_widg );
        n++;
        cascade = XmCreateCascadeButton( parent, ti_menu_name,
                                         args, n );
        XtManageChild( cascade );
    }
#endif
}


/*****************************************************************
 * TAG( add_primal_result_button )
 *
 * Add a single primal result button to the menu.
 */
static void
add_primal_result_button( Widget parent, Primal_result *p_pr )
{
    Widget submenu_cascade, submenu, result_menu;
    Widget button, cascade, result_submenu, subsubmenu_cascade;
    int position, vec_size;
    int i, j, n;
    char cbuf[M_MAX_NAME_LEN];
    Bool_type make_submenu;
    Arg args[10];
    char **comps, **p_specs;
    int spec_qty;
    Analysis *analy;
    State_variable *comp_svar;
    static char *cell_nums[] =
    {
        "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]", "[10]",
        "[11]", "[12]", "[13]", "[14]", "[15]", "[16]", "[17]", "[18]", "[19]",
        "[20]"
    };
    int qty_cell_nums;
    Htable_entry *p_hte;

    analy = env.curr_analy;
    qty_cell_nums = sizeof( cell_nums ) / sizeof( cell_nums[0] );

    /* Arrays and Vector Arrays that are too big don't go in menu. */
    if ( ( p_pr->var->agg_type == ARRAY
            && ( p_pr->var->rank > 2
                 || p_pr->var->dims[0] > qty_cell_nums
                 || ( p_pr->var->rank == 2
                      && p_pr->var->dims[1] > qty_cell_nums
                    )
               )
         )

            ||

            ( p_pr->var->agg_type == VEC_ARRAY
              && ( p_pr->var->rank > 1
                   || p_pr->var->dims[0] > qty_cell_nums
                 )
            )
       )
    {
        popup_dialog( INFO_POPUP, "Non-scalar Variable \"%s\" has too many\n%s",
                      p_pr->long_name,
                      "entries for inclusion in pulldown menu." );
        return;
    }

    /* Determine correct submenu name ("Shared" or class name). */
    get_primal_submenu_name( analy, p_pr, cbuf );

    /* See if submenu exists. */
    make_submenu = !find_labelled_child( primal_menu_widg, cbuf,
                                         &submenu_cascade, &position );

    if ( make_submenu )
    {
        n = 0;
        XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
        n++;
        XtSetArg( args[n], XmNscrollingPolicy, XmAUTOMATIC );
        n++;
        XtSetArg( args[n], XmNnumColumns, 40 );
        n++;
        XtSetArg( args[n], XmNorientation, XmHORIZONTAL );
        n++;
        XtSetArg( args[n], XmNpacking, XmPACK_COLUMN );
        n++;
        submenu = XmCreatePulldownMenu( primal_menu_widg, "submenu_pane", args,
                                        n );

        n = 0;
        XtSetArg( args[n], XmNsubMenuId, submenu );
        n++;
        submenu_cascade = XmCreateCascadeButton( primal_menu_widg, cbuf, args,
                          n );
        XtManageChild( submenu_cascade );
    }
    else
        /* Submenu exists; get the pane from the cascade button. */
        XtVaGetValues( submenu_cascade, XmNsubMenuId, &submenu, NULL );

    /* Now add the new primal result button. */
    if ( p_pr->var->agg_type != SCALAR )
    {
        /* Non-scalar types require another submenu level. */
        n = 0;
        XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
        n++;
        result_menu = XmCreatePulldownMenu( submenu, "submenu_pane", args, n );

        comps = p_pr->var->components;
        p_specs = analy->component_menu_specs;
        spec_qty = analy->component_spec_qty;

        if ( p_pr->var->agg_type == VECTOR )
        {
            for ( i = 0; i < p_pr->var->vec_size; i++ )
            {
                /* Find State_variable to provide component long name. */
                htable_search( analy->st_var_table, comps[i], FIND_ENTRY,
                               &p_hte );
                comp_svar = (State_variable *) p_hte->data;

                /* Build/save complete result specification string. */
                if( p_specs == NULL )
                    p_specs = NEW( char *, "New menu specs" );
                else
                    p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
                                       "Extend menu specs" );
                sprintf( cbuf, "%s[%s]", p_pr->short_name,
                         comp_svar->short_name );
                griz_str_dup( p_specs + spec_qty, cbuf );

                /* Create button. */
                sprintf( cbuf, "%s (%s)", comp_svar->long_name,
                         comp_svar->short_name );
                n = 0;
                button = XmCreatePushButtonGadget( result_menu, cbuf, args,
                                                   n );
                XtManageChild( button );
                XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                               p_specs[spec_qty] );

                spec_qty++;
            }
        }
        else if ( p_pr->var->agg_type == ARRAY )
        {
            if ( p_pr->var->rank == 1 )
            {
                for ( i = 0; i < p_pr->var->dims[0]; i++ )
                {
                    /* Build/save complete result specification string. */
                    p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
                                       "Extend menu specs" );
                    sprintf( cbuf, "%s[%d]", p_pr->short_name, i + 1 );
                    griz_str_dup( p_specs + spec_qty, cbuf );

                    /* Create button. */
                    n = 0;
                    button = XmCreatePushButtonGadget( result_menu,
                                                       cell_nums[i], args, n );
                    XtManageChild( button );
                    XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                                   p_specs[spec_qty] );

                    spec_qty++;
                }
            }
            else /* rank is 2 */
            {
                for ( i = 0; i < p_pr->var->dims[1]; i++ )
                {
                    /* Create button. */
                    n = 0;
                    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
                    n++;
                    result_submenu = XmCreatePulldownMenu( result_menu,
                                                           "subsubmenu_pane",
                                                           args, n );

                    n = 0;
                    XtSetArg( args[n], XmNsubMenuId, result_submenu );
                    n++;
                    subsubmenu_cascade = XmCreateCascadeButton( result_menu,
                                         cell_nums[i],
                                         args, n );
                    XtManageChild( subsubmenu_cascade );

                    for ( j = 0; j < p_pr->var->dims[0]; j++ )
                    {
                        /* Build/save complete result specification string. */
                        p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
                                           "Extend menu specs" );

                        sprintf( cbuf, "%s[%d,%d]", p_pr->short_name, i + 1,
                                 j + 1 );
                        griz_str_dup( p_specs + spec_qty, cbuf );

                        /* Create button. */
                        n = 0;
                        button = XmCreatePushButtonGadget( result_submenu,
                                                           cell_nums[j],
                                                           args, n );
                        XtManageChild( button );
                        XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                                       p_specs[spec_qty] );

                        spec_qty++;
                    }
                }
            }
        }
        else
        {
            for ( i = 0; i < p_pr->var->dims[0]; i++ )
            {
                /* Create button. */
                n = 0;
                XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
                n++;
                result_submenu = XmCreatePulldownMenu( result_menu,
                                                       "subsubmenu_pane",
                                                       args, n );

                n = 0;
                XtSetArg( args[n], XmNsubMenuId, result_submenu );
                n++;
                XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
                n++;

                /*
                 * This is WIP for the new Element Set menus.
                 */

                if ( analy->es_cnt>0 && strstr(p_pr->short_name, "es_" ) )
                {
                    int es_id=0, es_index=0, es_label=-1;
                    char *es_ptr=NULL, es_title[64], surface_title[16]="";
                    int integration_pt=0;
                    es_ptr   = strstr( p_pr->short_name, "es_" );
                    es_id    = get_element_set_id( es_ptr );
                    es_index = get_element_set_index( analy, es_id );

                    if ( es_id>=0 && i<= analy->es_intpoints[es_index].labels_cnt )
                    {
                        es_label = analy->es_intpoints[es_index].labels[i];
                        if ( analy->es_intpoints[es_index].in_mid_out_set[0]==es_label )
                            strcpy( surface_title, "(Default In)" );
                        else if ( analy->es_intpoints[es_index].in_mid_out_set[1]==es_label )
                            strcpy( surface_title, "(Default Mid)" );
                        else if ( analy->es_intpoints[es_index].in_mid_out_set[2]==es_label )
                            strcpy( surface_title, "(Default Out)" );
                        sprintf( es_title, "Int Pt %d (Label %d) %s", i+1, analy->es_intpoints[es_index].labels[i],
                                 surface_title );
                    }
                    else sprintf(es_title, "[%d]", i);
                    subsubmenu_cascade = XmCreateCascadeButton( result_menu,
                                         es_title,
                                         args, n );
                }
                else
                    subsubmenu_cascade = XmCreateCascadeButton( result_menu,
                                         cell_nums[i],
                                         args, n );
                XtManageChild( subsubmenu_cascade );

                vec_size = p_pr->var->vec_size;

                for ( j = 0; j < vec_size; j++ )
                {
                    /* Find State_variable to provide component long name. */
                    htable_search( analy->st_var_table, comps[j], FIND_ENTRY,
                                   &p_hte );
                    comp_svar = (State_variable *) p_hte->data;

                    /* Build/save complete result specification string. */
                    p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
                                       "Extend menu specs" );

                    sprintf( cbuf, "%s[%d,%s]", p_pr->short_name, i + 1,
                             comp_svar->short_name );

                    griz_str_dup( p_specs + spec_qty, cbuf );

                    /* Create button. */
                    sprintf( cbuf, "%s (%s)", comp_svar->long_name,
                             comp_svar->short_name );
                    n = 0;
                    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
                    n++;
                    button = XmCreatePushButtonGadget( result_submenu, cbuf,
                                                       args, n );
                    XtManageChild( button );
                    XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                                   p_specs[spec_qty] );

                    spec_qty++;
                }
            }
        }

        sprintf( cbuf, "%s (%s)", p_pr->long_name, p_pr->short_name );
        n = 0;
        XtSetArg( args[n], XmNsubMenuId, result_menu );
        n++;
        cascade = XmCreateCascadeButton( submenu, cbuf, args, n );
        XtManageChild( cascade );

        /* Update analy ("p_specs" could have been re-located). */
        analy->component_menu_specs = p_specs;
        analy->component_spec_qty = spec_qty;
    }
    else if ( p_pr->var->agg_type == SCALAR )
    {
        sprintf( cbuf, "%s (%s)", p_pr->long_name, p_pr->short_name );
        n = 0;
        button = XmCreatePushButtonGadget( submenu, cbuf, args, n );
        XtManageChild( button );
        XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                       p_pr->short_name );
    }
    else
        popup_dialog( WARNING_POPUP, "Variable of unknown agg type \"%s\"\n%s",
                      p_pr->long_name, "not included in pulldown menu." );
}


/*****************************************************************
 * TAG( add_ti_result_button )
 *
 * Add a single primal result button to the menu.
 */
static void
add_ti_result_button( Widget parent, Primal_result *p_pr )
{
    Widget submenu_cascade, submenu, result_menu;
    Widget button, cascade, result_submenu, subsubmenu_cascade;
    int position, vec_size;
    int i, j, n;
    char cbuf[M_MAX_NAME_LEN];
    Bool_type make_submenu;
    Arg args[10];
    char **comps, **p_specs;
    int spec_qty;
    Analysis *analy;
    State_variable *comp_svar;
    static char *cell_nums[] =
    {
        "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]", "[10]",
        "[11]", "[12]", "[13]", "[14]", "[15]", "[16]", "[17]", "[18]", "[19]",
        "[20]"
    };
    int qty_cell_nums;
    Htable_entry *p_hte;

    Bool_type found_hex=FALSE,
              found_shell=FALSE,
              found_beam=FALSE,
              shared_found=FALSE;

    analy = env.curr_analy;

    /* First make a tally of the TI objects that we have */
    for (i=0;
            i<analy->ti_var_count;
            i++)
    {
        if ( analy->ti_vars[i].superclass==M_HEX )
            found_hex = TRUE;
        if ( analy->ti_vars[i].superclass==M_QUAD)
            found_shell = TRUE;
        if ( analy->ti_vars[i].superclass==M_BEAM)
            found_beam = TRUE;

    }

    /* Add the first-level sub-menus */

    if ( found_hex )
    {
        strcpy(cbuf, "Bricks");
        n = 0;
        submenu = XmCreatePulldownMenu( ti_menu_widg, "submenu_pane", args,
                                        n );
        n = 0;
        XtSetArg( args[n], XmNsubMenuId, submenu );
        n++;
        submenu_cascade = XmCreateCascadeButton( ti_menu_widg, cbuf, args,
                          n );
        XtManageChild( submenu_cascade );
    }



#ifdef AAA
    /* Arrays and Vector Arrays that are too big don't go in menu. */
    if ( ( p_pr->var->agg_type == ARRAY
            && ( p_pr->var->rank > 2
                 || p_pr->var->dims[0] > qty_cell_nums
                 || ( p_pr->var->rank == 2
                      && p_pr->var->dims[1] > qty_cell_nums
                    )
               )
         )

            ||

            ( p_pr->var->agg_type == VEC_ARRAY
              && ( p_pr->var->rank > 1
                   || p_pr->var->dims[0] > qty_cell_nums
                 )
            )
       )
    {
        popup_dialog( INFO_POPUP, "Non-scalar Variable \"%s\" has too many\n%s",
                      p_pr->long_name,
                      "entries for inclusion in pulldown menu." );
        return;
    }

    /* Determine correct submenu name ("Shared" or class name). */
    get_primal_submenu_name( analy, p_pr, cbuf );

    /* See if submenu exists. */
    make_submenu = !find_labelled_child( primal_menu_widg, cbuf,
                                         &submenu_cascade, &position );

    if ( make_submenu )
    {
        n = 0;
        submenu = XmCreatePulldownMenu( primal_menu_widg, "submenu_pane", args,
                                        n );

        n = 0;
        XtSetArg( args[n], XmNsubMenuId, submenu );
        n++;
        submenu_cascade = XmCreateCascadeButton( primal_menu_widg, cbuf, args,
                          n );
        XtManageChild( submenu_cascade );
    }
    else
        /* Submenu exists; get the pane from the cascade button. */
        XtVaGetValues( submenu_cascade, XmNsubMenuId, &submenu, NULL );

    /* Now add the new primal result button. */
    if ( p_pr->var->agg_type != SCALAR )
    {
        /* Non-scalar types require another submenu level. */
        n = 0;
        result_menu = XmCreatePulldownMenu( submenu, "submenu_pane", args, n );

        comps = p_pr->var->components;
        p_specs = analy->component_menu_specs;
        spec_qty = analy->component_spec_qty;

        if ( p_pr->var->agg_type == VECTOR )
        {
            for ( i = 0; i < p_pr->var->vec_size; i++ )
            {
                /* Find State_variable to provide component long name. */
                htable_search( analy->st_var_table, comps[i], FIND_ENTRY,
                               &p_hte );
                comp_svar = (State_variable *) p_hte->data;

                /* Build/save complete result specification string. */
                if( p_specs == NULL )
                    p_specs = NEW( char *, "New menu specs" );
                else
                    p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
                                       "Extend menu specs" );
                sprintf( cbuf, "%s[%s]", p_pr->short_name,
                         comp_svar->short_name );
                griz_str_dup( p_specs + spec_qty, cbuf );

                /* Create button. */
                sprintf( cbuf, "%s (%s)", comp_svar->long_name,
                         comp_svar->short_name );
                n = 0;
                button = XmCreatePushButtonGadget( result_menu, cbuf, args,
                                                   n );
                XtManageChild( button );
                XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                               p_specs[spec_qty] );

                spec_qty++;
            }
        }
        else if ( p_pr->var->agg_type == ARRAY )
        {
            if ( p_pr->var->rank == 1 )
            {
                for ( i = 0; i < p_pr->var->dims[0]; i++ )
                {
                    /* Build/save complete result specification string. */
                    p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
                                       "Extend menu specs" );
                    sprintf( cbuf, "%s[%d]", p_pr->short_name, i + 1 );
                    griz_str_dup( p_specs + spec_qty, cbuf );

                    /* Create button. */
                    n = 0;
                    button = XmCreatePushButtonGadget( result_menu,
                                                       cell_nums[i], args, n );
                    XtManageChild( button );
                    XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                                   p_specs[spec_qty] );

                    spec_qty++;
                }
            }
            else /* rank is 2 */
            {
                for ( i = 0; i < p_pr->var->dims[1]; i++ )
                {
                    /* Create button. */
                    n = 0;
                    result_submenu = XmCreatePulldownMenu( result_menu,
                                                           "subsubmenu_pane",
                                                           args, n );

                    n = 0;
                    XtSetArg( args[n], XmNsubMenuId, result_submenu );
                    n++;
                    subsubmenu_cascade = XmCreateCascadeButton( result_menu,
                                         cell_nums[i],
                                         args, n );
                    XtManageChild( subsubmenu_cascade );

                    for ( j = 0; j < p_pr->var->dims[0]; j++ )
                    {
                        /* Build/save complete result specification string. */
                        p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
                                           "Extend menu specs" );

                        sprintf( cbuf, "%s[%d,%d]", p_pr->short_name, i + 1,
                                 j + 1 );
                        griz_str_dup( p_specs + spec_qty, cbuf );

                        /* Create button. */
                        n = 0;
                        button = XmCreatePushButtonGadget( result_submenu,
                                                           cell_nums[j],
                                                           args, n );
                        XtManageChild( button );
                        XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                                       p_specs[spec_qty] );

                        spec_qty++;
                    }
                }
            }
        }
        else
        {
            for ( i = 0; i < p_pr->var->dims[0]; i++ )
            {
                /* Create button. */
                n = 0;
                result_submenu = XmCreatePulldownMenu( result_menu,
                                                       "subsubmenu_pane",
                                                       args, n );

                n = 0;
                XtSetArg( args[n], XmNsubMenuId, result_submenu );
                n++;
                subsubmenu_cascade = XmCreateCascadeButton( result_menu,
                                     cell_nums[i],
                                     args, n );
                XtManageChild( subsubmenu_cascade );

                vec_size = p_pr->var->vec_size;

                for ( j = 0; j < vec_size; j++ )
                {
                    /* Find State_variable to provide component long name. */
                    htable_search( analy->st_var_table, comps[j], FIND_ENTRY,
                                   &p_hte );
                    comp_svar = (State_variable *) p_hte->data;

                    /* Build/save complete result specification string. */
                    p_specs = RENEW_N( char *, p_specs, spec_qty, 1,
                                       "Extend menu specs" );

                    sprintf( cbuf, "%s[%d,%s]", p_pr->short_name, i + 1,
                             comp_svar->short_name );
                    griz_str_dup( p_specs + spec_qty, cbuf );

                    /* Create button. */
                    sprintf( cbuf, "%s (%s)", comp_svar->long_name,
                             comp_svar->short_name );
                    n = 0;
                    button = XmCreatePushButtonGadget( result_submenu, cbuf,
                                                       args, n );
                    XtManageChild( button );
                    XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                                   p_specs[spec_qty] );

                    spec_qty++;
                }
            }
        }

        sprintf( cbuf, "%s (%s)", p_pr->long_name, p_pr->short_name );
        n = 0;
        XtSetArg( args[n], XmNsubMenuId, result_menu );
        n++;
        cascade = XmCreateCascadeButton( submenu, cbuf, args, n );
        XtManageChild( cascade );

        /* Update analy ("p_specs" could have been re-located). */
        analy->component_menu_specs = p_specs;
        analy->component_spec_qty = spec_qty;
    }
    else if ( p_pr->var->agg_type == SCALAR )
    {
        sprintf( cbuf, "%s (%s)", p_pr->long_name, p_pr->short_name );
        n = 0;
        button = XmCreatePushButtonGadget( submenu, cbuf, args, n );
        XtManageChild( button );
        XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                       p_pr->short_name );
    }
    else
        popup_dialog( WARNING_POPUP, "Variable of unknown agg type \"%s\"\n%s",
                      p_pr->long_name, "not included in pulldown menu." );
#endif
}


/*****************************************************************
 * TAG( get_primal_submenu_name )
 *
 * Determine the proper submenu name for a primal result by
 * searching through the object classes that support the result.
 */
static void
get_primal_submenu_name( Analysis *analy, Primal_result *p_pr, char *name )
{
    Subrecord *p_subrec;
    char **class_names;
    int qty, subr_qty;
    int *subr_indxs;
    int i, j, k;
    Bool_type done;
    Htable_entry *p_hte;
    MO_class_data *p_mo_class;

    qty = 0;
    done = FALSE;
    class_names = NULL;
    for ( i = 0; i < analy->qty_srec_fmts && !done; i++ )
    {
        subr_qty = p_pr->srec_map[i].qty;

        if ( subr_qty == 0 )
            continue;

        subr_indxs = (int *) p_pr->srec_map[i].list;

        for ( j = 0; j < p_pr->srec_map[i].qty; j++ )
        {
            p_subrec = &analy->srec_tree[i].subrecs[ subr_indxs[j] ].subrec;

            /* Search through existing class names to see if this is new. */
            for ( k = 0; k < qty; k++ )
                if ( strcmp( p_subrec->class_name, class_names[k] ) == 0 )
                    break;

            if ( k == qty )
            {
                /* New class name; add it to list. */
                class_names = RENEW_N( char *, class_names, qty, 1,
                                       "Extend class name list" );
                griz_str_dup( class_names + qty, p_subrec->class_name );
                qty++;

                /* If we see more than one class name, menu is "Shared". */
                if ( qty > 1 )
                {
                    done = TRUE;
                    break;
                }
            }
        }
    }

    if ( qty == 1 )
    {
        /* Only saw one class, so submenu name is class name. */

        /* Need to look up the long name. */
        htable_search( MESH_P( analy )->class_table, class_names[0], FIND_ENTRY,
                       &p_hte );
        p_mo_class = (MO_class_data *) p_hte->data;
        sprintf( name, "%s (%s)", p_mo_class->long_name, class_names[0] );
    }
    else
        /* Saw multiple classes, menu is "Shared". */
        strcpy( name, "Shared" );

    /* Clean-up. */
    for ( i = 0; i < qty; i++ )
        free( class_names[i] );
    free( class_names );
}


/*****************************************************************
 * TAG( find_labelled_child )
 *
 * Attempt match a name among a widgets children.
 */
static Bool_type
find_labelled_child( Widget parent, char *name, Widget *child, int *index )
{
    WidgetList children;
    XmString label;
    int i;
    int qty;
    char cbuf[M_MAX_NAME_LEN];

    /* Get list of parent's children widgets. */
    XtVaGetValues( parent, XmNnumChildren, &qty, XmNchildren, &children,
                   NULL );

    /* Search children for name match. */
    for ( i = qty - 1; i > -1; i-- )
    {
        XtVaGetValues( children[i], XmNlabelString, &label, NULL );
        if ( label != NULL )
            string_convert( label, cbuf );
        else
            continue;

        if ( strcmp( cbuf, name ) == 0 )
        {
            *child = children[i];
            *index = i;
            return TRUE;
        }
    }

    return FALSE;
}

/*****************************************************************
 * TAG( add_derived_result_button )
 *
 * Add a single derived result button to the menu.
 */
static void
add_derived_result_button( Derived_result *p_dr )
{
    Widget submenu_cascade, submenu, subsubmenu, cascade, sub_cascade, button;
    int i, j, k, n;
    int idx, position;
    char cbuf[M_MAX_NAME_LEN], nambuf[M_MAX_NAME_LEN];
    Bool_type make_submenu;
    Arg args[10];
    Analysis *analy;
    Subrecord_result *p_subr_res;

    /* Vars added to add element set derived results */
    int es_id=0;
    char submenu_name[64], *cb_name=NULL, cb_name_temp[64];
    Bool_type es_found=FALSE;
    State_variable sv;
    Subrecord subrec;
    Hash_table *p_pr_ht;
    Htable_entry *p_hte;
    Primal_result *p_pr;
    int dbid=0, srec_qty=0;
    int subrec_qty=0;
    int rval=0;

    analy = env.curr_analy;

    /* See if correct submenu exists. */

    /* Determine correct submenu name ("Shared" or class name). */
    get_result_submenu_name( p_dr, cbuf );
    strcpy( submenu_name, cbuf );

    /* See if submenu exists. */
    make_submenu = !find_labelled_child( derived_menu_widg, cbuf,
                                         &submenu_cascade, &position );

    if ( make_submenu )
    {
        n = 0;
        XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
        n++;
        submenu = XmCreatePulldownMenu( derived_menu_widg, "submenu_pane",
                                        args, n );

        n = 0;
        XtSetArg( args[n], XmNsubMenuId, submenu );
        n++;
        cascade = XmCreateCascadeButton( derived_menu_widg, cbuf, args, n );
        XtManageChild( cascade );
    }
    else
        /* Submenu exists; get the pane from the cascade button. */
        XtVaGetValues( submenu_cascade, XmNsubMenuId, &submenu, NULL );

    for ( i = 0; i < analy->qty_srec_fmts; i++ )
        if ( p_dr->srec_map[i].qty > 0 )
            break;
    p_subr_res = (Subrecord_result *) p_dr->srec_map[i].list;
    idx = p_subr_res->index;

    /* If we have element set derived results then add them now */

    p_pr_ht = env.curr_analy->primal_results;
    if ( analy->es_cnt>0 && p_pr_ht != NULL )
    {

        /* Get state record format count for this database. */
        dbid = env.curr_analy->db_ident;
        srec_qty = 0;
        env.curr_analy->db_query( dbid, QRY_QTY_SREC_FMTS, NULL, NULL,
                                  (void *) &srec_qty );

        /* Loop over srecs */
        for ( i = 0;
                i < srec_qty;
                i++ )
        {
            /* Get subrecord count for this state record. */
            rval = env.curr_analy->db_query( dbid, QRY_QTY_SUBRECS, (void *) &i,
                                             NULL, (void *) &subrec_qty );
            if ( rval != OK )
                continue;

            /* Loop over subrecs */
            for ( j = 0;
                    j < subrec_qty;
                    j++ )
            {
                /* Get binding */
                rval = env.curr_analy->db_get_subrec_def( dbid, i, j, &subrec );
                if ( rval != OK )
                    continue;

                /* Look for element set svars to add to sub-menu */

                if ( subrec.qty_svars==1 && strstr( submenu_name, subrec.class_name)
                        && !strncmp( subrec.svar_names[0], "es_", 3 ) )
                {
                    es_id = get_element_set_id( subrec.svar_names[0] );
                    sprintf( nambuf, "Element Set %d (%s)", es_id,
                             subrec.svar_names[0] );

                    /* See if submenu exists. */
                    make_submenu = !find_labelled_child( submenu, nambuf,
                                                         &submenu_cascade, &position );

                    /* Use bound state vars as keys into Primal_result hash table.
                     */
                    rval = htable_search( p_pr_ht, subrec.svar_names[0], FIND_ENTRY, &p_hte );
                    if ( rval != OK )
                        continue;

                    p_pr = (Primal_result *) p_hte->data;

                    if ( make_submenu )
                    {
                        n = 0;
                        XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
                        n++;
                        subsubmenu = XmCreatePulldownMenu( submenu, "es_submenu_pane",
                                                           args, n );

                        n = 0;
                        XtSetArg( args[n], XmNsubMenuId, subsubmenu );
                        n++;
                        sub_cascade = XmCreateCascadeButton( submenu, nambuf, args, n );
                        XtManageChild( sub_cascade );
                    }
                    else
                        /* Submenu exists; get the pane from the cascade button. */
                        XtVaGetValues( submenu_cascade, XmNsubMenuId, &subsubmenu, NULL );

                    if ( p_pr->var->num_type == M_STRING )
                        continue;

                    rval = mc_get_svar_def( dbid, subrec.svar_names[0], &sv );
                    if ( rval == 0 )
                    {
                        for ( k=0;
                                k<sv.vec_size;
                                k++ )
                            if ( !strcmp( sv.components[k], p_subr_res->candidate->short_names[idx] ) )
                            {
                                /* Add derived element set result button. */
                                for ( i = 0; i < analy->qty_srec_fmts; i++ )
                                    if ( p_dr->srec_map[i].qty > 0 )
                                        break;
                                p_subr_res = (Subrecord_result *) p_dr->srec_map[i].list;

                                sprintf( nambuf, "%s (%s)", p_subr_res->candidate->long_names[idx],
                                         p_subr_res->candidate->short_names[idx] );
                                n = 0;
                                button = XmCreatePushButtonGadget( subsubmenu, nambuf, args, n );
                                XtManageChild( button );
                                sprintf( cb_name_temp, "%s[%s]", subrec.svar_names[0], p_subr_res->candidate->short_names[idx] );
                                cb_name = NEW_N( char, strlen(cb_name_temp), "Static cb name" );
                                strcpy( cb_name, cb_name_temp );
                                XtAddCallback( button, XmNactivateCallback, res_menu_CB, cb_name );
                            }
                    }
                } /* es variable test */

                /* Don't care about return value for this. */
                env.curr_analy->db_cleanse_subrec( &subrec );
            } /* for on j */
        }
    }

    /* Add derived result button. */

    sprintf( nambuf, "%s (%s)", p_subr_res->candidate->long_names[idx],
             p_subr_res->candidate->short_names[idx] );
    n = 0;
    button = XmCreatePushButtonGadget( submenu, nambuf, args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                   p_subr_res->candidate->short_names[idx] );

}

/*****************************************************************
 * TAG( get_result_submenu_name )
 *
 * Determine the proper submenu name for a derived result by
 * determining if multiple classes support the result.  Because
 * a result can be derived for one class from data in another
 * class (for ex., brick strains calculated from nodal positions),
 * we can't rely on simple examinations of the class names for
 * the subrecords to identify the classes actually supporting the
 * result derivation or to provide the menu name of a non-shared
 * result.
 */
static void
get_result_submenu_name( Derived_result *p_dr, char *name )
{
    int i, j, k;
    int qty_fmts, sr_qty, class_qty;
    Subrec_obj *subrecs;
    int subr_id;
    Subrecord_result *p_sr;
    int candidate_sclass, subrecord_sclass;
    MO_class_data *p_mo_class;
    Mesh_data *p_mesh;
    char *shared = "Shared";
    char *subrecord_class;

    qty_fmts = env.curr_analy->qty_srec_fmts;

    /*
     * Search all state record formats to get a menu name from the
     * first format that supports the derived result.
     */
    for ( i = 0; i < qty_fmts; i++ )
    {
        /*
         * Compare data for the first class supporting the result
         * with the rest of the classes supporting the result to
         * determine if the submenu name should be "Shared" or a
         * class name.
         */
        sr_qty = p_dr->srec_map[i].qty;

        if ( sr_qty == 0 )
            continue;

        p_sr = (Subrecord_result *) p_dr->srec_map[i].list;
        subrecs = env.curr_analy->srec_tree[i].subrecs;

        /* Parameterize data for the first class. */
        candidate_sclass = p_sr[0].candidate->superclass;
        subr_id = p_sr[0].subrec_id;
        subrecord_sclass = subrecs[subr_id].p_object_class->superclass;
        subrecord_class = subrecs[subr_id].p_object_class->short_name;

        /*
         * Loop over the subrecords supporting the result derivation
         * to determine if the result is shared.
         */
        for ( j = 1; j < sr_qty; j++ )
        {
            subr_id = p_sr[j].subrec_id;

            if ( candidate_sclass != p_sr[j].candidate->superclass )
            {
                /*
                 * The superclasses from originating Result_candidate's
                 * differ; go no farther, it's a shared result.
                 */
                strcpy( name, shared );
                break;
            }
            else if ( subrecord_sclass
                      != subrecs[subr_id].p_object_class->superclass )
            {
                /*
                 * The originating superclasses are the same but the
                 * supporting subrecords bind data from classes which
                 * themselves have different superclasses, so it's a shared
                 * result (maybe, for example, nodal data supporting
                 * brick strain calculations and thick shell data which
                 * includes a strain tensor explicitly - bricks and
                 * thick shells would both be G_HEX, but the supporting
                 * subrecords would have superclasses G_NODE and G_HEX).
                 */
                strcpy( name, shared );
                break;
            }
            else if ( strcmp( subrecord_class,
                              subrecs[subr_id].p_object_class->short_name )
                      != 0 )
            {
                /*
                 * The Result_candidate and subrecord superclasses match,
                 * but the subrecord classes differ - almost certainly a
                 * shared result (for example, a stress derived result
                 * from a brick subrecord containing a stress tensor and
                 * a thick shell subrecord containing a stress tensor).
                 * An exception might be if for some reason there were two
                 * nodal classes in the mesh (not currently supported by
                 * Griz), and subrecords binding each of the nodal classes
                 * independently support the same result derivation on
                 * another class (i.e., brick strains from nodal positions).
                 * In that case, it shouldn't be treated as shared since
                 * the final result from both subrecords would be for the
                 * same class.  Assume this won't occur for now.
                 */
                strcpy( name, shared );
                break;
            }
        }

        if ( j == sr_qty && sr_qty != 0 )
        {
            /*
             * Might still be shared if the result is indirect and there are
             * multiple classes from the superclass of the derived result.
             * If it's indirect but there's only one class from that
             * superclass, use that class name.  Otherwise, use the class
             * name of the class bound to the subrecord.
             */

            p_mo_class = subrecs[p_sr[0].subrec_id].p_object_class;

            if ( candidate_sclass != p_mo_class->superclass )
            {
                /*
                 * The superclasses differ, so we have to find a suitable
                 * class name somewhere, but the Result_candidate doesn't
                 * know about classes.  Pick the first class of the correct
                 * superclass.
                 */
                for ( k = 0; k < env.curr_analy->mesh_qty; k++ )
                {
                    p_mesh = env.curr_analy->mesh_table + k;
                    class_qty = p_mesh->classes_by_sclass[candidate_sclass].qty;

                    if ( class_qty > 0 )
                    {
                        p_mo_class = ((MO_class_data **)
                                      p_mesh->classes_by_sclass[candidate_sclass].list)[0];

                        if ( class_qty > 1 )
                        {
                            strcpy( name, shared );
                            p_mo_class = NULL;
                        }
                        break;
                    }
                }
            }

            if ( p_mo_class != NULL )
                sprintf( name, "%s (%s)", p_mo_class->long_name,
                         p_mo_class->short_name );
        }
    }

    return;
}


/*****************************************************************
 * TAG( create_derived_res_menu )
 *
 * Create the derived results menu for the Control window menu bar.
 */
static void
create_derived_res_menu( Widget parent )
{
    Widget button;
    int i, j, n;
    Arg args[1];
    int qty_candidates;
    Result_candidate *p_rc;
    Hash_table *p_dr_ht;
    Htable_entry *p_hte;
    Derived_result *p_dr;
    int rval;
    Analysis *p_analy;
    p_analy = get_analy_ptr();

    /*
     * Traverse the possible_results[] array to get the names of derived
     * results in a reasonable order.  Ignore the ones that don't exist
     * since this database doesn't support them.
     */

    for ( qty_candidates = 0;
            possible_results[qty_candidates].superclass != QTY_SCLASS;
            qty_candidates++ );

    p_dr_ht = env.curr_analy->derived_results;
    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    derived_menu_widg = XmCreatePulldownMenu( parent, "derived_menu_pane",
                        args, n );

    /* Always add an entry to show materials. */
    n = 0;
    button = XmCreatePushButtonGadget( derived_menu_widg, "Result off",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, res_menu_CB, "mat" );

    /* Go no further if there aren't actually any derived results. */
    if ( p_dr_ht == NULL )
        return;

    /* Now build the result menu(s) */
    for ( i = 0; i < qty_candidates; i++ )
    {
        p_rc = &possible_results[i];

        for ( j = 0; p_rc->short_names[j] != NULL; j++ )
        {
            rval = htable_search( p_dr_ht, p_rc->short_names[j], FIND_ENTRY,
                                  &p_hte );

            if ( rval == OK )
            {
                p_dr = (Derived_result *) p_hte->data;

                if ( p_analy->analysis_type==MODAL ) /* May later check for
						      * p_rc->origin.is_node_result
						      */
                    if ( p_rc->short_names[0]!=NULL )
                        if ( strncmp(  p_rc->short_names[0], "evec_", 5) )
                            p_rc->hide_in_menu = TRUE;

                if ( !p_dr->in_menu && !p_rc->hide_in_menu)
                {
                    add_derived_result_button( p_dr );
                    p_dr->in_menu = TRUE;
                }
            }
        }
    }
}


/*****************************************************************
 * TAG( create_primal_res_menu )
 *
 * Create the primal results menu for the Control window menu bar.
 */
static void
create_primal_res_menu( Widget parent )
{
    int i, j, k, n;
    Arg args[1];
    Hash_table *p_pr_ht;
    Htable_entry *p_hte;
    Primal_result *p_pr;
    int rval;
    Widget button;
    int srec_qty, subrec_qty;
    char **svar_names;
    int dbid;
    Subrecord subrec;

    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    primal_menu_widg = XmCreatePulldownMenu( parent, "primal_menu_pane",
                       args, n );

    /* Always add an entry to show materials. */
    n = 0;
    button = XmCreatePushButtonGadget( primal_menu_widg, "Result off",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, res_menu_CB, "mat" );

    p_pr_ht = env.curr_analy->primal_results;

    /* Go no further if there aren't actually any primal results. */
    if ( p_pr_ht == NULL )
        return;

    /*
     * Traverse the state record formats and read each subrecord definition
     * to get (potential) Primal_results in a reasonable order.
     */

    /* Get state record format count for this database. */
    dbid = env.curr_analy->db_ident;
    srec_qty = 0;

    /* Don't care about return value for this. */
    env.curr_analy->db_query( dbid, QRY_QTY_SREC_FMTS, NULL, NULL,
                              (void *) &srec_qty );

    /* Loop over srecs */
    for ( i = 0; i < srec_qty; i++ )
    {
        /* Get subrecord count for this state record. */
        rval = env.curr_analy->db_query( dbid, QRY_QTY_SUBRECS, (void *) &i,
                                         NULL, (void *) &subrec_qty );
        if ( rval != OK )
            continue;

        /* Loop over subrecs */
        for ( j = 0; j < subrec_qty; j++ )
        {
            /* Get binding */
            rval = env.curr_analy->db_get_subrec_def( dbid, i, j, &subrec );
            if ( rval != OK )
                continue;

            /* Use bound state vars as keys into Primal_result hash table.
             */
            svar_names = subrec.svar_names;

            for ( k = 0; k < subrec.qty_svars; k++ )
            {
                rval = htable_search( p_pr_ht, svar_names[k], FIND_ENTRY,
                                      &p_hte );
                if ( rval == OK )
                {
                    p_pr = (Primal_result *) p_hte->data;

                    if ( p_pr->var->num_type == M_STRING )
                        continue;

                    if ( !p_pr->in_menu )
                    {
                        add_primal_result_button( parent, p_pr );
                        p_pr->in_menu = TRUE;
                    }
                }
            }

            /* Don't care about return value for this. */
            env.curr_analy->db_cleanse_subrec( &subrec );
        }
    }
}


/*****************************************************************
 * TAG( create_ti_res_menu )
 *
 * Create the TI results menu for the Control window menu bar.
 */
static void
create_ti_res_menu( Widget parent )
{
    int i, j, k, n;
    Arg args[1];
    Hash_table *p_pr_ht;
    Htable_entry *p_hte;
    Primal_result *p_pr;
    int rval;
    Widget button;
    int srec_qty, subrec_qty;
    char **svar_names;
    int dbid;
    Subrecord subrec;
    Analysis *analy;

    analy = env.curr_analy;
    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    ti_menu_widg = XmCreatePulldownMenu( parent, "ti_menu_pane",
                                         args, n );

    /* Always add an entry to show materials. */
    n = 0;
    button = XmCreatePushButtonGadget( ti_menu_widg, "Result off",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, res_menu_CB, "mat" );

    /*
     * Traverse the state record formats and read each subrecord definition
     * to get (potential) Ti_results in a reasonable order.
     */

    /* add_ti_result_button( parent, p_pr ); */
}



/*****************************************************************
 * TAG( regenerate_result_menus )
 *
 * Destroy existing derived and primal result menus and re-create
 * them according to current available results.
 */
void
regenerate_result_menus( void )
{
    Widget child;
    int position;
    Analysis *analy;

    analy = env.curr_analy;

    /* Destroy the existing pulldown menus. */
    XtDestroyWidget( derived_menu_widg );
    derived_menu_widg = NULL;
    XtDestroyWidget( primal_menu_widg );
    primal_menu_widg = NULL;

    if ( analy->ti_data_found && ti_menu_widg != NULL )
    {
        XtDestroyWidget( ti_menu_widg );
        ti_menu_widg = NULL;
    }

    /* Create new result menus. */
    create_derived_res_menu( menu_widg );
    create_primal_res_menu( menu_widg );

    /* Bring up TI menus if TI data is found */
    if ( analy->ti_data_found )
        create_ti_res_menu( menu_widg );

    /* Reference the new menus in the appropriate cascade buttons. */
    find_labelled_child( menu_widg, derived_menu_name, &child, &position );
    XtVaSetValues( child, XmNsubMenuId, derived_menu_widg, NULL );

    find_labelled_child( menu_widg, primal_menu_name, &child, &position );
    XtVaSetValues( child, XmNsubMenuId, primal_menu_widg, NULL );

    if ( analy->ti_data_found )
    {
        find_labelled_child(  menu_widg, ti_menu_name, &child, &position );
        /* XtVaSetValues( child, XmNsubMenuId, ti_menu_widg, NULL ); */
    }
}


/*****************************************************************
 * TAG( create_mtl_manager )
 *
 * Create the material manager widget.
 */
static Widget
create_mtl_manager( Widget main_widg )
{
    Widget mtl_shell, widg, func_select, scroll_win, sep1,
           sep2, sep3, vert_scroll, func_operate, mtl_label,
           frame, col_comp;
    Arg args[10];
    char win_title[256];
    char mtl_toggle_name[8];
    int n, i, mtl, qty_mtls;
    int gid=3;
    Dimension width, max_child_width, margin_width, spacing, scrollbar_width,
              name_len, col_ed_width, max_mgr_width, height, sw_height, fudge;
    short func_width, max_cols, rows;
    XtActionsRec rec;
    static Widget select_buttons[4];
    static Widget *ctl_buttons[2];
    XmString val_text;
    int scale_len, val_len, vert_space, vert_height,
        vert_pos, hori_offset, vert_offset, loc;
    char *func_names[] =
    {
        "Visible", "Invisible", "Enable", "Disable", "Color"
    };
    char *select_names[] =
    {
        "All", "None", "Invert", "By func", "Prev"
    };
    char *op_names[] = { "Preview", "Cancel", "Apply", "Default" };
    char *scale_names[] = { "Red", "Green", "Blue", "Shininess" };
    char *comp_names[] = { "Ambient", "Diffuse", "Specular", "Emissive" };
    Pixel fg, bg;
    String trans =
        "Ctrl<Key>q: action_quit() \n ~Ctrl <Key>: action_translate_command()";
    XtTranslations key_trans;
    Analysis *p_analy;

    key_trans = XtParseTranslationTable( trans );

    mtl_mgr_func_toggles = NEW_N( Widget, MTL_FUNC_QTY, "Mtl func toggle" );

    ctl_buttons[0] = mtl_mgr_func_toggles;
    ctl_buttons[1] = select_buttons;
    qty_mtls = env.curr_analy->mesh_table[0].material_qty;

    sprintf( win_title, "Material Manager %s", path_string );
    n = 0;
    XtSetArg( args[n], XtNtitle, win_title );
    n++;
    XtSetArg( args[n], XmNiconic, FALSE );
    n++;
    XtSetArg( args[n], XmNwindowGroup, XtWindow( main_widg ) );
    n++;

    mtl_shell = XtAppCreateShell( "GRIZ", "mtl_shell",
                                  topLevelShellWidgetClass,
                                  XtDisplay( main_widg ), args, n );

    /* XtAddCallback( mtl_shell, XmNdestroyCallback,
                   destroy_mtl_mgr_CB, (XtPointer) NULL ); */

    XtAddEventHandler( mtl_shell, EnterWindowMask | LeaveWindowMask, False,
                       gress_mtl_mgr_EH, NULL );

    /* Use a Form widget to manage everything else. */

    mtl_base = XtVaCreateManagedWidget(
                   "mtl_base", xmFormWidgetClass, mtl_shell,
                   NULL );

    XtAddCallback( mtl_base, XmNdestroyCallback,
                   destroy_mtl_mgr_CB, (XtPointer) NULL );

    if ( env.griz_id>0 )
    {
        gid = env.griz_id;
        widg = XtVaCreateManagedWidget(
                   "Function", xmLabelGadgetClass, mtl_base,
                   XmNalignment, XmALIGNMENT_CENTER,
                   XmNtopAttachment, XmATTACH_FORM,
                   XmNrightAttachment, XmATTACH_FORM,
                   XmNleftAttachment, XmATTACH_FORM,
                   XmNinitialResourcesPersistent, FALSE,
                   XmNbackground, env.border_colors[gid-1],
                   NULL );
    }
    else
        widg = XtVaCreateManagedWidget(
                   "Function", xmLabelGadgetClass, mtl_base,
                   XmNalignment, XmALIGNMENT_CENTER,
                   XmNtopAttachment, XmATTACH_FORM,
                   XmNrightAttachment, XmATTACH_FORM,
                   XmNleftAttachment, XmATTACH_FORM,
                   XmNinitialResourcesPersistent, FALSE,
                   NULL );

    /* Use a RowColumn to hold the function select toggles. */
    func_select = XtVaCreateManagedWidget(
                      "func_select", xmRowColumnWidgetClass, mtl_base,
                      XmNorientation, XmHORIZONTAL,
                      XmNpacking, XmPACK_COLUMN,
                      XmNtraversalOn, False,
                      XmNisAligned, True,
                      XmNentryAlignment, XmALIGNMENT_CENTER,
                      XmNtopAttachment, XmATTACH_WIDGET,
                      XmNtopWidget, widg,
                      XmNrightAttachment, XmATTACH_POSITION,
                      XmNrightPosition, 50,
                      NULL );

    XtOverrideTranslations( func_select, key_trans );

    /* Get these for geometry adjustment later. */
    XtVaGetValues( func_select,
                   XmNmarginWidth, &margin_width,
                   XmNspacing, &spacing,
                   NULL );

    max_mgr_width = 0;
    max_child_width = 0;

    for ( i = 0; i < 5; i++ )
    {
        mtl_mgr_func_toggles[i] = XtVaCreateManagedWidget(
                                      func_names[i], xmToggleButtonWidgetClass, func_select,
                                      XmNindicatorOn, False,
                                      XmNshadowThickness, 3,
                                      XmNfillOnSelect, True,
                                      NULL );

        XtOverrideTranslations( mtl_mgr_func_toggles[i], key_trans );

        XtAddCallback( mtl_mgr_func_toggles[i], XmNdisarmCallback,
                       mtl_func_select_CB, (XtPointer) mtl_mgr_func_toggles );

        XtVaGetValues( mtl_mgr_func_toggles[i], XmNwidth, &width, NULL );
        if ( width > max_child_width )
            max_child_width = width;
    }

    /*
     * Now that all function selects have been created and the total width
     * can be calculated, set the right offset so that the middle of the
     * RowColumn will be on the attach position.
     */
    func_width = 5 * max_child_width + 2 * margin_width + 4 * spacing;
    XtVaSetValues( func_select,
                   XmNrightOffset, -((int) (func_width / 2)), NULL );

    if ( func_width > max_mgr_width )
        max_mgr_width = func_width;

    sep1 = XtVaCreateManagedWidget(
               "sep1", xmSeparatorGadgetClass, mtl_base,
               XmNtopAttachment, XmATTACH_WIDGET,
               XmNtopWidget, func_select,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    mtl_label = XtVaCreateManagedWidget(
                    "Material", xmLabelGadgetClass, mtl_base,
                    XmNalignment, XmALIGNMENT_CENTER,
                    XmNtopAttachment, XmATTACH_WIDGET,
                    XmNtopWidget, sep1,
                    XmNrightAttachment, XmATTACH_FORM,
                    XmNleftAttachment, XmATTACH_FORM,
                    NULL );

    /*
     * Now build up from the bottom...
     */

    /* Use a BulletinBoard to hold the color editor. */
    col_ed_width = 465;

    if ( col_ed_width > max_mgr_width )
        max_mgr_width = col_ed_width;

    color_editor = XtVaCreateManagedWidget(
                       "color_editor", xmBulletinBoardWidgetClass, mtl_base,
                       XmNwidth, col_ed_width,
                       XmNheight, 225,
                       XmNmarginHeight, 0,
                       XmNmarginWidth, 0,
                       XmNtraversalOn, False,
                       XmNresizePolicy, XmRESIZE_NONE,
                       XmNtopAttachment, XmATTACH_NONE,
                       XmNrightAttachment, XmATTACH_POSITION,
                       XmNrightPosition, 50,
                       XmNrightOffset, -(col_ed_width / 2),
                       XmNbottomAttachment, XmATTACH_FORM,
                       NULL );

    XtOverrideTranslations( color_editor, key_trans );

    /* Use a RowColumn to hold the color property select buttons. */
    col_comp = XtVaCreateManagedWidget(
                   "col_comp", xmRowColumnWidgetClass, color_editor,
                   XmNx, 10,
                   XmNy, 30,
                   XmNorientation, XmHORIZONTAL,
                   XmNradioBehavior, True,
                   XmNradioAlwaysOne, False,
                   XmNisAligned, True,
                   XmNentryAlignment, XmALIGNMENT_CENTER,
                   XmNpacking, XmPACK_COLUMN,
                   NULL );

    XtOverrideTranslations( col_comp, key_trans );

    widg = XtVaCreateManagedWidget(
               " Property", xmLabelGadgetClass, col_comp,
               NULL );
    XtVaGetValues( widg, XmNwidth, &name_len, NULL );

    for ( i = 0; i < 4; i++ )
    {
        color_comps[i] = XtVaCreateManagedWidget(
                             comp_names[i], xmToggleButtonWidgetClass, col_comp,
                             XmNset, False,
                             XmNindicatorOn, False,
                             XmNshadowThickness, 3,
                             XmNfillOnSelect, True,
                             NULL );

        XtOverrideTranslations( color_comps[i], key_trans );

        XtAddCallback( color_comps[i], XmNdisarmCallback,
                       col_comp_disarm_CB, (XtPointer) i );

        prop_val_changed[i] = FALSE;
        property_vals[i] = NEW_N( GLfloat, 3, "Col comp val" );
    }

    XtVaGetValues( color_editor, XmNforeground, &fg, XmNbackground, &bg, NULL );
    mtl_check = XCreatePixmapFromBitmapData( dpy,
                RootWindow( dpy, DefaultScreen( dpy ) ), (char *) GrizCheck_bits,
                GrizCheck_width, GrizCheck_height, fg, bg,
                DefaultDepth( dpy, DefaultScreen( dpy ) ) );

    /* Use a RowColumn to hold the property-modify indicators. */
    widg = XtVaCreateManagedWidget(
               "col_comp_checks", xmRowColumnWidgetClass, color_editor,
               XmNx, (name_len + 10 + margin_width + spacing),
               XmNy, 5,
               XmNorientation, XmHORIZONTAL,
               XmNisAligned, True,
               XmNentryAlignment, XmALIGNMENT_CENTER,
               XmNpacking, XmPACK_COLUMN,
               NULL );

    XtOverrideTranslations( widg, key_trans );

    /* Use a pixmap of a check as the modify indicator. */
    for ( i = 0; i < (EMISSIVE + 1); i++ )
    {
        prop_checks[i] = XtVaCreateManagedWidget(
                             "check", xmLabelWidgetClass, widg,
                             XmNlabelType, XmPIXMAP,
                             XmNlabelPixmap, mtl_check,
                             XmNwidth, name_len,
                             XmNrecomputeSize, False,
                             XmNmappedWhenManaged, False,
                             XmNmarginWidth, 0,
                             XmNmarginHeight, 0,
                             XmNpositionIndex, ((short) i),
                             NULL );

        XtOverrideTranslations( prop_checks[i], key_trans );
    }

    /* These need to be #def'd when stable. */
    scale_len = 200;
    val_len = 50;
    vert_height = 25;
    vert_space = 8;
    hori_offset = 10 + 3;
    vert_offset = 75;

    /* Now add the modify indicator for the shininess scale. */
    prop_checks[SHININESS] = XtVaCreateManagedWidget(
                                 "check", xmLabelWidgetClass, color_editor,
                                 XmNx, (hori_offset + (name_len - GrizCheck_width) / 2),
                                 XmNy, (vert_offset + 3 * vert_height
                                        + 4 * vert_space + vert_height / 2 - GrizCheck_height),
                                 XmNlabelType, XmPIXMAP,
                                 XmNlabelPixmap, mtl_check,
                                 XmNmappedWhenManaged, False,
                                 XmNmarginWidth, 0,
                                 XmNmarginHeight, 0,
                                 NULL );

    XtOverrideTranslations( prop_checks[SHININESS], key_trans );

    val_text = XmStringCreateLocalized( "0.00" );

    for ( i = 0; i < 4; i++ )
    {
        vert_pos = vert_offset + i * vert_height + i * vert_space;
        if ( i == SHININESS_SCALE )
            vert_pos += vert_height / 2 + vert_space;

        widg = XtVaCreateManagedWidget(
                   scale_names[i], xmLabelGadgetClass, color_editor,
                   XmNx, hori_offset,
                   XmNy, vert_pos,
                   XmNwidth, name_len,
                   XmNheight, vert_height,
                   XmNalignment, XmALIGNMENT_END,
                   NULL );

        XtOverrideTranslations( widg, key_trans );

        col_ed_scales[0][i] = XtVaCreateManagedWidget(
                                  "scale", xmScaleWidgetClass, color_editor,
                                  XmNx, hori_offset + name_len + spacing,
                                  XmNy, vert_pos + (vert_height - 20) / 2,
                                  XmNwidth, scale_len,
                                  XmNheight, 20,
                                  XmNorientation, XmHORIZONTAL,
                                  XmNmaximum, (( i == SHININESS_SCALE ) ? 128 : 100 ),
                                  XmNvalue, 0,
                                  XmNsensitive, (( i == SHININESS_SCALE ) ? True : False ),
                                  NULL );

        XtOverrideTranslations( col_ed_scales[0][i], key_trans );

        frame = XtVaCreateManagedWidget(
                    "frame", xmFrameWidgetClass, color_editor,
                    XmNx, hori_offset + name_len + scale_len + spacing,
                    XmNy, vert_pos,
                    XmNheight, vert_height,
                    NULL );

        XtOverrideTranslations( frame, key_trans );

        col_ed_scales[1][i] = XtVaCreateManagedWidget(
                                  "scale_val", xmLabelGadgetClass, frame,
                                  XmNalignment, XmALIGNMENT_CENTER,
                                  XmNlabelString, val_text,
                                  NULL );

        XtAddCallback( col_ed_scales[0][i], XmNdragCallback,
                       col_ed_scale_CB, (XtPointer) i );
        XtAddCallback( col_ed_scales[0][i], XmNvalueChangedCallback,
                       col_ed_scale_CB, (XtPointer) i );
        XtAddCallback( col_ed_scales[0][i], XmNvalueChangedCallback,
                       col_ed_scale_update_CB, (XtPointer) i );
    }

    XmStringFree( val_text );
    /* Reset the shininess scale text to an integer rep. */
    val_text = XmStringCreateLocalized( "0" );
    XtVaSetValues( col_ed_scales[1][SHININESS_SCALE],
                   XmNlabelString, val_text,
                   NULL );
    XmStringFree( val_text );

    /* Create the color swatch in a frame. */
    loc = hori_offset + name_len + scale_len + spacing + val_len + 28;
    swatch_frame = XtVaCreateManagedWidget(
                       "frame", xmFrameWidgetClass, color_editor,
                       XmNx, loc,
                       XmNy, vert_offset,
                       XmNwidth, 60,
                       XmNheight, 60,
                       NULL );

    /* Create the OpenGL drawing window. */
    ogl_widg[SWATCH] = XtVaCreateManagedWidget(
                           "swatch",
#ifdef GLWM
                           glwMDrawingAreaWidgetClass,
#else
                           glwDrawingAreaWidgetClass,
#endif
                           swatch_frame,
                           GLwNvisualInfo, vi,
                           NULL );

    XtOverrideTranslations( ogl_widg[SWATCH], key_trans );

    XtAddCallback( ogl_widg[SWATCH], GLwNginitCallback,
                   init_swatch_CB, (XtPointer) main_widg );
    XtAddCallback( ogl_widg[SWATCH], GLwNexposeCallback,
                   expose_swatch_CB, (XtPointer) main_widg );

    /* Label to show current material in the swatch. */
    swatch_label = XtVaCreateManagedWidget(
                       "swatch_label", xmLabelWidgetClass, color_editor,
                       XmNalignment, XmALIGNMENT_CENTER,
                       XmNmappedWhenManaged, False,
                       XmNrecomputeSize, False,
                       XmNx, (loc - (104 - 60) / 2),
                       XmNy, vert_offset + 60 + 10,
                       XmNwidth, 104,
                       NULL );

    XtOverrideTranslations( swatch_label, key_trans );

    /* Init some globals associated with the color editor. */
    prop_val_changed[SHININESS] = FALSE;
    property_vals[SHININESS] = NEW( GLfloat, "Shine comp val" );
    cur_mtl_comp = MTL_PROP_QTY; /* Guaranteed to be a non-functional value. */
    cur_color[0] = 0.0;
    cur_color[1] = 0.0;
    cur_color[2] = 0.0;
    cur_color[3] = 0.0;

    /* Place a separator on top of the color editor. */
    sep3 = XtVaCreateManagedWidget(
               "sep3", xmSeparatorGadgetClass, mtl_base,
               XmNbottomAttachment, XmATTACH_WIDGET,
               XmNbottomWidget, color_editor,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    /* Use a RowColumn to hold the function operate buttons. */
    func_operate = XtVaCreateManagedWidget(
                       "func_operate", xmRowColumnWidgetClass, mtl_base,
                       XmNorientation, XmHORIZONTAL,
                       XmNpacking, XmPACK_COLUMN,
                       XmNtraversalOn, False,
                       XmNisAligned, True,
                       XmNentryAlignment, XmALIGNMENT_CENTER,
                       XmNtopAttachment, XmATTACH_NONE,
                       XmNrightAttachment, XmATTACH_POSITION,
                       XmNrightPosition, 50,
                       XmNbottomAttachment, XmATTACH_WIDGET,
                       XmNbottomWidget, sep3,
                       NULL );

    XtOverrideTranslations( func_operate, key_trans );

    max_child_width = 0;

    op_buttons = NEW_N( Widget, MTL_OP_QTY, "Mtl Op Btns" );

    for ( i = 0; i < sizeof( op_names ) / sizeof( op_names[0] ); i++ )
    {
        op_buttons[i] = XtVaCreateManagedWidget(
                            op_names[i], xmPushButtonGadgetClass, func_operate,
                            XmNsensitive, False,
                            NULL );

        XtOverrideTranslations( op_buttons[i], key_trans );

        XtAddCallback( op_buttons[i], XmNdisarmCallback, mtl_func_operate_CB,
                       (XtPointer) (OP_PREVIEW + i) );

        XtVaGetValues( op_buttons[i], XmNwidth, &width, NULL );
        if ( width > max_child_width )
            max_child_width = width;
    }

    /*
     * Now set the right offset so that the middle of the RowColumn
     * will be on the attach position.
     */
    func_width = 4 * max_child_width + 2 * margin_width + 3 * spacing;
    XtVaSetValues( func_operate,
                   XmNrightOffset, -((int) (func_width / 2)), NULL );

    if ( func_width > max_mgr_width )
        max_mgr_width = func_width;

    widg = XtVaCreateManagedWidget(
               "Action", xmLabelGadgetClass, mtl_base,
               XmNalignment, XmALIGNMENT_CENTER,
               XmNbottomAttachment, XmATTACH_WIDGET,
               XmNbottomWidget, func_operate,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    /* Place a separator on top of the operation buttons. */
    sep2 = XtVaCreateManagedWidget(
               "sep2", xmSeparatorGadgetClass, mtl_base,
               XmNbottomAttachment, XmATTACH_WIDGET,
               XmNbottomWidget, widg,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    /* Use a RowColumn to hold the global material select buttons. */
    mtl_quick_select = XtVaCreateManagedWidget(
                           "mtl_quick_select", xmRowColumnWidgetClass, mtl_base,
                           XmNorientation, XmHORIZONTAL,
                           XmNpacking, XmPACK_COLUMN,
                           XmNtraversalOn, False,
                           XmNisAligned, True,
                           XmNentryAlignment, XmALIGNMENT_CENTER,
                           XmNbottomAttachment, XmATTACH_WIDGET,
                           XmNbottomWidget, sep2,
                           XmNrightAttachment, XmATTACH_POSITION,
                           XmNrightPosition, 50,
                           NULL );

    XtOverrideTranslations( mtl_quick_select, key_trans );

    max_child_width = 0;

    for ( i = 0; i < 4; i++ )
    {
        select_buttons[i] = XtVaCreateManagedWidget(
                                select_names[i], xmPushButtonGadgetClass, mtl_quick_select,
                                XmNshadowThickness, 3,
                                NULL );

        XtOverrideTranslations( select_buttons[i], key_trans );

        XtAddCallback( select_buttons[i], XmNdisarmCallback,
                       mtl_quick_select_CB, (XtPointer) ctl_buttons );

        XtVaGetValues( select_buttons[i], XmNwidth, &width, NULL );
        if ( width > max_child_width )
            max_child_width = width;
    }

    /*
     * Now set the right offset so that the middle of the RowColumn
     * will be on the attach position.
     */
    func_width = 4 * max_child_width + 2 * margin_width + 3 * spacing;
    XtVaSetValues( mtl_quick_select,
                   XmNrightOffset, -((int) (func_width / 2)), NULL );

    if ( func_width > max_mgr_width )
        max_mgr_width = func_width;

    /*
     * Now insert a scrolled window to hold the material select
     * toggles and bridge the widgets attached from the top and
     * the bottom of the form.
     */

    scroll_win = XtVaCreateManagedWidget(
                     "mtl_scroll", xmScrolledWindowWidgetClass, mtl_base,
                     XmNscrollingPolicy, XmAUTOMATIC,
                     XmNtopAttachment, XmATTACH_WIDGET,
                     XmNtopWidget, mtl_label,
                     XmNrightAttachment, XmATTACH_FORM,
                     XmNleftAttachment, XmATTACH_FORM,
                     XmNbottomAttachment, XmATTACH_WIDGET,
                     XmNbottomWidget, mtl_quick_select,
                     NULL );

    XtVaGetValues( scroll_win, XmNverticalScrollBar, &vert_scroll, NULL );
    XtVaGetValues( vert_scroll, XmNwidth, &scrollbar_width, NULL );

    rec.string = "resize_mtl_scrollwin";
    rec.proc = (XtActionProc) resize_mtl_scrollwin;
    XtAppAddActions( app_context, &rec, 1 );
    XtOverrideTranslations( scroll_win,
                            XtParseTranslationTable( "<Configure>: resize_mtl_scrollwin()" ) );

    mtl_row_col = XtVaCreateManagedWidget(
                      "row_col", xmRowColumnWidgetClass, scroll_win,
                      XmNorientation, XmHORIZONTAL,
                      XmNpacking, XmPACK_COLUMN,
                      XmNrowColumnType, XmWORK_AREA,
                      XmNentryAlignment, XmALIGNMENT_CENTER,
                      XmNisAligned, True,
                      NULL );

    XtOverrideTranslations( mtl_row_col, key_trans );

    mtl_deselect_list = NULL;
    mtl_select_list = NULL;
    for ( i = 0; i < qty_mtls; i++ )
    {
        Material_list_obj *p_mtl;

        mtl = i + 1;
        if ( mtl < 10 )
            sprintf( mtl_toggle_name, " %d ", mtl );
        else
            sprintf( mtl_toggle_name, "%d", mtl );

        widg = XtVaCreateManagedWidget(
                   mtl_toggle_name, xmToggleButtonWidgetClass, mtl_row_col,
                   XmNtraversalOn, False,
                   XmNindicatorOn, False,
                   XmNshadowThickness, 3,
                   XmNfillOnSelect, True,
                   NULL );

        XtOverrideTranslations( widg, key_trans );

        p_mtl = NEW( Material_list_obj, "Init mtl list" );
        p_mtl->mtl = mtl;
        INSERT( p_mtl, mtl_deselect_list );

        XtAddCallback( widg, XmNdisarmCallback, mtl_select_CB,
                       (XtPointer) mtl );
    }

    /*
     * Limit number of columns to fit within the maximum of the
     * other manager widgets.
     */
    XtVaGetValues( widg, XmNwidth, &width, XmNheight, &height, NULL );
    /* "4" comes from default distance between scrollbar and work area. */
    max_cols = (max_mgr_width - 2.0 * margin_width - scrollbar_width
                - 4 + spacing)
               / (float) (width + spacing);
    rows = (short) ceil( ((double) qty_mtls) / max_cols );
    XtVaSetValues( mtl_row_col, XmNnumColumns, rows, NULL );

    /*
     * Limit the initial height of the scrolled window to lesser of
     * the height of the required rows of the RowColumn or the height
     * of three rows.
     */
    fudge = 8; /* Oh hell. */
    if ( rows > 3 ) /* Note: XmNmarginWidth = XmNmarginHeight */
        sw_height = 3 * height + 2 * (spacing + margin_width) + fudge;
    else
        sw_height = rows * height + (rows - 1) * spacing + 2 * margin_width
                    + fudge;
    XtVaSetValues( scroll_win, XmNheight, sw_height, NULL );

    /* Make color editor & children insensitive. */
    XtSetSensitive( color_editor, False );

    /* Buffer to hold commands for parser. */
    n = (int) (ceil( log10( (double) qty_mtls ) )) + 1;
    mtl_mgr_cmd = NEW_N( char, 128 + qty_mtls * n, "Mtl mgr cmd bufr" );

    XtOverrideTranslations( mtl_base, key_trans );

    /* All done, pop it up. */
    XtPopup( mtl_shell, XtGrabNone );

    /* Add handler to init stacking order control. */
    XtAddEventHandler( mtl_shell, StructureNotifyMask, False,
                       stack_init_EH, (XtPointer) MTL_MGR_SHELL_WIN );

    session->win_mtl_active = 1;

    return mtl_shell;
}


/*****************************************************************
 * TAG( create_surf_manager )
 *
 * Create the material manager widget.
 */
static Widget
create_surf_manager( Widget main_widg )
{
    Widget surf_shell, widg, func_select = NULL, scroll_win, sep1,
                             sep2, sep3, vert_scroll, func_operate, surf_label,
                             frame, col_comp;
    Arg args[10];
    char win_title[256];
    char surf_toggle_name[64];
    int n, i, surf, qty_surfs;
    Dimension width, max_child_width, margin_width, spacing, scrollbar_width,
              name_len, col_ed_width, max_mgr_width, height, sw_height, fudge;
    short func_width, max_cols, rows;
    XtActionsRec rec;
    static Widget select_buttons[4];
    static Widget *ctl_buttons[2];
    XmString val_text;
    int scale_len, val_len, vert_space, vert_height,
        vert_pos, hori_offset, vert_offset, loc;
    char *func_names[] =
    {
        "Visible", "Invisible", "Enable", "Disable"
    };
    char *select_names[] =
    {
        "All", "None", "Invert", "By func"
    };
    char *op_names[] = { "Apply", "Default" };
    Pixel fg, bg;
    String trans =
        "Ctrl<Key>q: action_quit() \n ~Ctrl <Key>: action_translate_command()";
    XtTranslations key_trans;
    int gid=0;

    key_trans = XtParseTranslationTable( trans );

    ctl_buttons[0] = surf_mgr_func_toggles;
    ctl_buttons[1] = select_buttons;
    qty_surfs = env.curr_analy->mesh_table[0].surface_qty;

    sprintf( win_title, "%s Surface Manager", griz_name );
    n = 0;
    XtSetArg( args[n], XtNtitle, win_title );
    n++;
    XtSetArg( args[n], XmNiconic, FALSE );
    n++;
    XtSetArg( args[n], XmNinitialResourcesPersistent, FALSE );
    n++;
    XtSetArg( args[n], XmNwindowGroup, XtWindow( main_widg ) );
    n++;
    surf_shell = XtAppCreateShell( "GRIZ", "surf_shell",
                                   topLevelShellWidgetClass,
                                   XtDisplay( main_widg ), args, n );

    XtAddCallback( surf_shell, XmNdestroyCallback,
                   destroy_surf_mgr_CB, (XtPointer) NULL );

    XtAddEventHandler( surf_shell, EnterWindowMask | LeaveWindowMask, False,
                       gress_surf_mgr_EH, NULL );

    /* Use a Form widget to manage everything else. */
    gid = env.griz_id;
    if ( env.griz_id>0 )
    {
        surf_base = XtVaCreateManagedWidget( "surf_base", xmFormWidgetClass, surf_shell,
                                             XmNmarginWidth, 2,
                                             XmNmarginHeight, 2,
                                             XmNbackground, env.border_colors[gid-1],
                                             NULL );
    }
    else
        surf_base = XtVaCreateManagedWidget( "surf_base", xmFormWidgetClass, surf_shell,
                                             NULL );


    XtOverrideTranslations( surf_base, key_trans );

    widg = XtVaCreateManagedWidget(
               "Function", xmLabelGadgetClass, surf_base,
               XmNalignment, XmALIGNMENT_CENTER,
               XmNtopAttachment, XmATTACH_FORM,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    /* Use a RowColumn to hold the function select toggles. */
    func_select = XtVaCreateManagedWidget(
                      "func_select", xmRowColumnWidgetClass, surf_base,
                      XmNorientation, XmHORIZONTAL,
                      XmNpacking, XmPACK_COLUMN,
                      XmNtraversalOn, False,
                      XmNisAligned, True,
                      XmNentryAlignment, XmALIGNMENT_CENTER,
                      XmNtopAttachment, XmATTACH_WIDGET,
                      XmNtopWidget, widg,
                      XmNrightAttachment, XmATTACH_POSITION,
                      XmNrightPosition, 50,
                      NULL );

    XtOverrideTranslations( func_select, key_trans );

    /* Get these for geometry adjustment later. */
    XtVaGetValues( func_select,
                   XmNmarginWidth, &margin_width,
                   XmNspacing, &spacing,
                   NULL );

    max_mgr_width = 0;
    max_child_width = 0;

    for ( i = 0; i < 4; i++ )
    {
        surf_mgr_func_toggles[i] = XtVaCreateManagedWidget(
                                       func_names[i], xmToggleButtonWidgetClass, func_select,
                                       XmNindicatorOn, False,
                                       XmNshadowThickness, 3,
                                       XmNfillOnSelect, True,
                                       NULL );

        XtOverrideTranslations( surf_mgr_func_toggles[i], key_trans );

        XtAddCallback( surf_mgr_func_toggles[i], XmNdisarmCallback,
                       surf_func_select_CB, (XtPointer) surf_mgr_func_toggles );

        XtVaGetValues( surf_mgr_func_toggles[i], XmNwidth, &width, NULL );
        if ( width > max_child_width )
            max_child_width = width;
    }

    /*
     * Now that all function selects have been created and the total width
     * can be calculated, set the right offset so that the middle of the
     * RowColumn will be on the attach position.
     */
    func_width = 4 * max_child_width + 2 * margin_width + 4 * spacing;
    XtVaSetValues( func_select,
                   XmNrightOffset, -((int) (func_width / 2)), NULL );

    if ( func_width > max_mgr_width )
        max_mgr_width = func_width;

    sep1 = XtVaCreateManagedWidget(
               "sep1", xmSeparatorGadgetClass, surf_base,
               XmNtopAttachment, XmATTACH_WIDGET,
               XmNtopWidget, func_select,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    surf_label = XtVaCreateManagedWidget(
                     "Surface Classes", xmLabelGadgetClass, surf_base,
                     XmNalignment, XmALIGNMENT_CENTER,
                     XmNtopAttachment, XmATTACH_WIDGET,
                     XmNtopWidget, sep1,
                     XmNrightAttachment, XmATTACH_FORM,
                     XmNleftAttachment, XmATTACH_FORM,
                     NULL );

    /*
     * Now build up from the bottom...
     */

    /* Use a RowColumn to hold the function operate buttons. */
    func_operate = XtVaCreateManagedWidget(
                       "func_operate", xmRowColumnWidgetClass, surf_base,
                       XmNorientation, XmHORIZONTAL,
                       XmNpacking, XmPACK_COLUMN,
                       XmNtraversalOn, False,
                       XmNisAligned, True,
                       XmNentryAlignment, XmALIGNMENT_CENTER,
                       XmNtopAttachment, XmATTACH_NONE,
                       XmNrightAttachment, XmATTACH_POSITION,
                       XmNrightPosition, 50,
                       XmNbottomAttachment, XmATTACH_WIDGET,
                       NULL );

    XtOverrideTranslations( func_operate, key_trans );

    max_child_width = 0;

    for ( i = 0; i < sizeof( op_names ) / sizeof( op_names[0] ); i++ )
    {
        surf_op_buttons[i] = XtVaCreateManagedWidget(
                                 op_names[i], xmPushButtonGadgetClass, func_operate,
                                 XmNsensitive, False,
                                 NULL );

        XtOverrideTranslations( surf_op_buttons[i], key_trans );

        XtAddCallback( surf_op_buttons[i], XmNdisarmCallback, surf_func_operate_CB,
                       (XtPointer) (i) );

        XtVaGetValues( surf_op_buttons[i], XmNwidth, &width, NULL );
        if ( width > max_child_width )
            max_child_width = width;
    }

    /*
     * Now set the right offset so that the middle of the RowColumn
     * will be on the attach position.
     */
    func_width = 2 * max_child_width + 2 * margin_width + 3 * spacing;
    XtVaSetValues( func_operate,
                   XmNrightOffset, -((int) (func_width / 2)), NULL );

    if ( func_width > max_mgr_width )
        max_mgr_width = func_width;

    widg = XtVaCreateManagedWidget(
               "Action", xmLabelGadgetClass, surf_base,
               XmNalignment, XmALIGNMENT_CENTER,
               XmNbottomAttachment, XmATTACH_WIDGET,
               XmNbottomWidget, func_operate,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    /* Place a separator on top of the operation buttons. */
    sep2 = XtVaCreateManagedWidget(
               "sep2", xmSeparatorGadgetClass, surf_base,
               XmNbottomAttachment, XmATTACH_WIDGET,
               XmNbottomWidget, widg,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    /* Use a RowColumn to hold the global surface select buttons. */
    surf_quick_select = XtVaCreateManagedWidget(
                            "surf_quick_select", xmRowColumnWidgetClass, surf_base,
                            XmNorientation, XmHORIZONTAL,
                            XmNpacking, XmPACK_COLUMN,
                            XmNtraversalOn, False,
                            XmNisAligned, True,
                            XmNentryAlignment, XmALIGNMENT_CENTER,
                            XmNbottomAttachment, XmATTACH_WIDGET,
                            XmNbottomWidget, sep2,
                            XmNrightAttachment, XmATTACH_POSITION,
                            XmNrightPosition, 50,
                            NULL );

    XtOverrideTranslations( surf_quick_select, key_trans );

    max_child_width = 0;

    for ( i = 0; i < 4; i++ )
    {
        select_buttons[i] = XtVaCreateManagedWidget(
                                select_names[i], xmPushButtonGadgetClass, surf_quick_select,
                                XmNshadowThickness, 3,
                                NULL );

        XtOverrideTranslations( select_buttons[i], key_trans );

        XtAddCallback( select_buttons[i], XmNdisarmCallback,
                       surf_quick_select_CB, (XtPointer) ctl_buttons );

        XtVaGetValues( select_buttons[i], XmNwidth, &width, NULL );
        if ( width > max_child_width )
            max_child_width = width;
    }

    /*
     * Now set the right offset so that the middle of the RowColumn
     * will be on the attach position.
     */
    func_width = 3 * max_child_width + 2 * margin_width + 3 * spacing;
    XtVaSetValues( surf_quick_select,
                   XmNrightOffset, -((int) (func_width / 2)), NULL );

    if ( func_width > max_mgr_width )
        max_mgr_width = func_width;

    /*
     * Now insert a scrolled window to hold the surface select
     * toggles and bridge the widgets attached from the top and
     * the bottom of the form.
     */

    scroll_win = XtVaCreateManagedWidget(
                     "surf_scroll", xmScrolledWindowWidgetClass, surf_base,
                     XmNscrollingPolicy, XmAUTOMATIC,
                     XmNtopAttachment, XmATTACH_WIDGET,
                     XmNtopWidget, surf_label,
                     XmNrightAttachment, XmATTACH_FORM,
                     XmNleftAttachment, XmATTACH_FORM,
                     XmNbottomAttachment, XmATTACH_WIDGET,
                     XmNbottomWidget, surf_quick_select,
                     NULL );

    XtVaGetValues( scroll_win, XmNverticalScrollBar, &vert_scroll, NULL );
    XtVaGetValues( vert_scroll, XmNwidth, &scrollbar_width, NULL );

    rec.string = "resize_surf_scrollwin";
    rec.proc = (XtActionProc) resize_surf_scrollwin;
    XtAppAddActions( app_context, &rec, 1 );
    XtOverrideTranslations( scroll_win,
                            XtParseTranslationTable( "<Configure>: resize_surf_scrollwin()" ) );

    surf_row_col = XtVaCreateManagedWidget(
                       "row_col", xmRowColumnWidgetClass, scroll_win,
                       XmNorientation, XmHORIZONTAL,
                       XmNpacking, XmPACK_COLUMN,
                       XmNrowColumnType, XmWORK_AREA,
                       XmNentryAlignment, XmALIGNMENT_CENTER,
                       XmNisAligned, True,
                       NULL );

    XtOverrideTranslations( surf_row_col, key_trans );

    surf_deselect_list = NULL;
    surf_select_list = NULL;
    for ( i = 0; i < qty_surfs; i++ )
    {
        Surface_list_obj *p_surf;

        surf = i + 1;
        if ( surf < 10 )
            sprintf( surf_toggle_name, " Name%d(master) ", surf );
        else
            sprintf( surf_toggle_name, "%d", surf );

        widg = XtVaCreateManagedWidget(
                   surf_toggle_name, xmToggleButtonWidgetClass, surf_row_col,
                   XmNtraversalOn, False,
                   XmNindicatorOn, False,
                   XmNshadowThickness, 3,
                   XmNfillOnSelect, True,
                   NULL );

        XtOverrideTranslations( widg, key_trans );

        p_surf = NEW( Surface_list_obj, "Init surf list" );
        p_surf->surf = surf;
        INSERT( p_surf, surf_deselect_list );

        XtAddCallback( widg, XmNdisarmCallback, surf_select_CB,
                       (XtPointer) surf );
    }

    /*
     * Limit number of columns to fit within the maximum of the
     * other manager widgets.
     */
    XtVaGetValues( widg, XmNwidth, &width, XmNheight, &height, NULL );
    /* "4" comes from default distance between scrollbar and work area. */
    max_cols = (max_mgr_width - 2.0 * margin_width - scrollbar_width
                - 4 + spacing)
               / (float) (width + spacing);
    rows = (short) ceil( ((double) qty_surfs) / max_cols );
    XtVaSetValues( surf_row_col, XmNnumColumns, rows, NULL );

    /*
     * Limit the initial height of the scrolled window to lesser of
     * the height of the required rows of the RowColumn or the height
     * of three rows.
     */
    fudge = 8; /* Oh hell. */
    if ( rows > 3 ) /* Note: XmNmarginWidth = XmNmarginHeight */
        sw_height = 3 * height + 2 * (spacing + margin_width) + fudge;
    else
        sw_height = rows * height + (rows - 1) * spacing + 2 * margin_width
                    + fudge;
    XtVaSetValues( scroll_win, XmNheight, sw_height, NULL );

    /* Buffer to hold commands for parser. */
    n = (int) (ceil( log10( (double) qty_surfs ) )) + 1;
    surf_mgr_cmd = NEW_N( char, 128 + qty_surfs * n, "Surface mgr cmd bufr" );

    /* All done, pop it up. */
    XtPopup( surf_shell, XtGrabNone );

    /* Add handler to init stacking order control. */
    XtAddEventHandler( surf_shell, StructureNotifyMask, False,
                       stack_init_EH, (XtPointer) SURF_MGR_SHELL_WIN );

    session->win_surf_active = 1;

    return surf_shell;
}


/*****************************************************************
 * TAG( create_free_util_panel )
 *
 * Create the utility panel widget.
 */
static Widget
create_free_util_panel( Widget main_widg )
{
    int n;
    char win_title[256];
    Arg args[10];
    Widget util_shell, util_panel;
    String trans =
        "Ctrl<Key>q: action_quit() \n ~Ctrl <Key>: action_translate_command()";

    sprintf( win_title, "Utility Panel %s", path_string );

    n = 0;
    XtSetArg( args[n], XtNtitle, win_title );
    n++;

    XtSetArg( args[n], XmNiconic, FALSE );
    n++;
    XtSetArg( args[n], XmNinitialResourcesPersistent, FALSE );
    n++;
    XtSetArg( args[n], XmNwindowGroup, XtWindow( main_widg ) );
    n++;
    util_shell = XtAppCreateShell( "GRIZ", "util_panel",
                                   topLevelShellWidgetClass,
                                   XtDisplay( main_widg ), args, n );

    util_panel = create_utility_panel( util_shell );

    XtOverrideTranslations( util_panel, XtParseTranslationTable( trans ) );

    /* Pop it up. */
    XtPopup( util_shell, XtGrabNone );

    /* XtAddEventHandler( util_shell, ExposureMask, False, stack_init_EH,  */
    XtAddEventHandler( util_shell, StructureNotifyMask, False, stack_init_EH,
                       (XtPointer) UTIL_PANEL_SHELL_WIN );

    session->win_util_active = 1;

    return util_shell;
}


/*****************************************************************
 * TAG( action_translate_command )
 *
 * Action routine to send text input to the command widget.
 */
/* ARGSUSED 2 */
static void
action_translate_command( Widget w, XEvent *event, String params[], int *qty )
{
    process_keyboard_input( (XKeyEvent *) event );
}

/*****************************************************************
 * TAG( create_utility_panel )
 *
 * Create the utility panel widget.
 */
static Widget
create_utility_panel( Widget main_widg )
{
    Widget widg, rend_child, sep1;
    Pixel fg, bg;
    Dimension width, rc_width, child_width, height,
              margin_width, spacing;
    char stride_text[5];
    XmString stride_str;
    Analysis *analy;
    String trans =
        "Ctrl<Key>q: action_quit() \n ~Ctrl <Key>: action_translate_command()";
    XtTranslations key_trans;
    int gid=0;

    key_trans = XtParseTranslationTable( trans );

    analy = env.curr_analy;

    /* Use a Form widget to manage everything else. */
    gid = env.griz_id;
    if ( env.griz_id>0 && !include_util_panel )
        util_panel_main = XtVaCreateManagedWidget(
                              "util_panel_main", xmFormWidgetClass, main_widg,
                              XmNallowResize, FALSE,
                              XmNmarginWidth, 2,
                              XmNmarginHeight, 2,
                              XmNbackground, env.border_colors[gid-1],
                              NULL );
    else
        util_panel_main = XtVaCreateManagedWidget(
                              "util_panel_main", xmFormWidgetClass, main_widg,
                              XmNallowResize, TRUE,
                              NULL );

    XtAddCallback( util_panel_main, XmNdestroyCallback,
                   destroy_util_panel_CB, (XtPointer) NULL );

    util_state_ctl = XtVaCreateManagedWidget(
                         "util_state_ctl", xmRowColumnWidgetClass, util_panel_main,
                         XmNorientation, XmHORIZONTAL,
                         XmNrowColumnType, XmWORK_AREA,
                         XmNtraversalOn, False,
                         XmNtopAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_POSITION,
                         XmNrightPosition, 50,
                         NULL );

    XtOverrideTranslations( util_state_ctl, key_trans );

    /* Get these for use later. */
    XtVaGetValues( util_state_ctl,
                   XmNmarginWidth, &margin_width,
                   XmNspacing, &spacing,
                   XmNforeground, &fg,
                   XmNbackground, &bg,
                   NULL );

    rc_width = 2 * margin_width + 12 * spacing;

    widg = XtVaCreateManagedWidget(
               "Step", xmLabelGadgetClass, util_state_ctl,
               NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    pixmap_leftstop = XCreatePixmapFromBitmapData( dpy,
                      RootWindow( dpy, DefaultScreen( dpy ) ), (char *)  GrizLeftStop_bits,
                      GrizLeftStop_width, GrizLeftStop_height, fg, bg,
                      DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget(
               "first_state", xmPushButtonGadgetClass, util_state_ctl,
               XmNlabelType, XmPIXMAP,
               XmNlabelPixmap, pixmap_leftstop,
               NULL );
    XtAddCallback( widg, XmNactivateCallback, step_CB, STEP_FIRST );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    pixmap_left = XCreatePixmapFromBitmapData( dpy,
                  RootWindow( dpy, DefaultScreen( dpy ) ), (char *) GrizLeft_bits,
                  GrizLeft_width, GrizLeft_height, fg, bg,
                  DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget(
               "prev_state", xmPushButtonGadgetClass, util_state_ctl,
               XmNlabelType, XmPIXMAP,
               XmNlabelPixmap, pixmap_left,
               NULL );
    XtAddCallback( widg, XmNactivateCallback, step_CB, (XtPointer) STEP_DOWN );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    pixmap_right = XCreatePixmapFromBitmapData( dpy,
                   RootWindow( dpy, DefaultScreen( dpy ) ), (char *) GrizRight_bits,
                   GrizRight_width, GrizRight_height, fg, bg,
                   DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget(
               "next_state", xmPushButtonGadgetClass, util_state_ctl,
               XmNlabelType, XmPIXMAP,
               XmNlabelPixmap, pixmap_right,
               NULL );
    XtAddCallback( widg, XmNactivateCallback, step_CB, (XtPointer) STEP_UP );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    pixmap_rightstop = XCreatePixmapFromBitmapData( dpy,
                       RootWindow( dpy, DefaultScreen( dpy ) ), (char *) GrizRightStop_bits,
                       GrizRightStop_width, GrizRightStop_height, fg, bg,
                       DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget(
               "last_state", xmPushButtonGadgetClass, util_state_ctl,
               XmNlabelType, XmPIXMAP,
               XmNlabelPixmap, pixmap_rightstop,
               NULL );
    XtAddCallback( widg, XmNactivateCallback, step_CB, (XtPointer) STEP_LAST );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    stride_str = XmStringCreateLtoR( "Stride",
                                     XmSTRING_DEFAULT_CHARSET );
    widg = XtVaCreateManagedWidget(
               "stride_label", xmLabelGadgetClass, util_state_ctl,
               XmNlabelString, stride_str,
               NULL );
    XmStringFree( stride_str );
    XtVaGetValues( widg, XmNwidth, &child_width, XmNheight, &height, NULL );
    rc_width += child_width;

    sprintf( stride_text, "%d", step_stride );
    stride_label = XtVaCreateManagedWidget(
                       "stride_text", xmTextFieldWidgetClass, util_state_ctl,
                       XmNcolumns, 3,
                       XmNcursorPositionVisible, True,
                       XmNeditable, True,
                       XmNresizeWidth, True,
                       XmNvalue, stride_text,
                       NULL );
    XtAddCallback( stride_label, XmNactivateCallback, stride_CB,
                   (XtPointer) STRIDE_EDIT );
    XtVaGetValues( stride_label, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    widg = XtVaCreateManagedWidget(
               "stride_incr", xmArrowButtonWidgetClass, util_state_ctl,
               NULL );
    XtOverrideTranslations( widg, key_trans );
    XtAddCallback( widg, XmNactivateCallback, stride_CB,
                   (XtPointer) STRIDE_INCREMENT );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    widg = XtVaCreateManagedWidget(
               "stride_decr", xmArrowButtonWidgetClass, util_state_ctl,
               XmNarrowDirection, XmARROW_DOWN,
               NULL );
    XtOverrideTranslations( widg, key_trans );
    XtAddCallback( widg, XmNactivateCallback, stride_CB,
                   (XtPointer) STRIDE_DECREMENT );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    widg = XtVaCreateManagedWidget(
               "  Animate", xmLabelGadgetClass, util_state_ctl,
               NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    pixmap_start = XCreatePixmapFromBitmapData( dpy,
                   RootWindow( dpy, DefaultScreen( dpy ) ), (char *) GrizStart_bits,
                   GrizStart_width, GrizStart_height, fg, bg,
                   DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget(
               "start_anim", xmPushButtonGadgetClass, util_state_ctl,
               XmNlabelType, XmPIXMAP,
               XmNlabelPixmap, pixmap_start,
               NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    XtAddCallback( widg, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_ANIMATE );

    pixmap_stop = XCreatePixmapFromBitmapData( dpy,
                  RootWindow( dpy, DefaultScreen( dpy ) ), (char *) GrizStop_bits,
                  GrizStop_width, GrizStop_height, fg, bg,
                  DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget(
               "stop_anim", xmPushButtonGadgetClass, util_state_ctl,
               XmNlabelType, XmPIXMAP,
               XmNlabelPixmap, pixmap_stop,
               NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    XtAddCallback( widg, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_STOPANIMATE );

    widg = XtVaCreateManagedWidget(
               "cont_anim", xmPushButtonGadgetClass, util_state_ctl,
               XmNlabelType, XmPIXMAP,
               XmNlabelPixmap, pixmap_right,
               NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    XtAddCallback( widg, XmNactivateCallback, menu_CB,
                   (XtPointer) BTN_CONTANIMATE );

    XtVaSetValues( util_state_ctl,
                   XmNrightOffset, -((int) (rc_width / 2)), NULL );

    sep1 = XtVaCreateManagedWidget(
               "sep1", xmSeparatorGadgetClass, util_panel_main,
               XmNtopAttachment, XmATTACH_WIDGET,
               XmNtopWidget, util_state_ctl,
               XmNrightAttachment, XmATTACH_FORM,
               XmNleftAttachment, XmATTACH_FORM,
               NULL );

    util_render_ctl = XtVaCreateManagedWidget(
                          "util_render_ctl", xmRowColumnWidgetClass, util_panel_main,
                          XmNtopAttachment, XmATTACH_WIDGET,
                          XmNtopWidget, sep1,
                          XmNrightAttachment, XmATTACH_POSITION,
                          XmNrightPosition, 50,
                          XmNorientation, XmHORIZONTAL,
                          XmNpacking, XmPACK_COLUMN,
                          XmNtraversalOn, False,
                          NULL );

    XtOverrideTranslations( util_render_ctl, key_trans );

    util_render_btns = NEW_N( Widget, UTIL_PANEL_BTN_QTY, "Util render btns" );

    rend_child = XtVaCreateManagedWidget(
                     "render_select", xmRowColumnWidgetClass, util_render_ctl,
                     XmNisAligned, True,
                     XmNentryAlignment, XmALIGNMENT_CENTER,
                     XmNorientation, XmVERTICAL,
                     XmNpacking, XmPACK_COLUMN,
                     NULL );

    XtOverrideTranslations( rend_child, key_trans );

    child_width = 0;

    /*
     * Mesh rendering style selection toggle buttons.
     */

    widg = XtVaCreateManagedWidget(
               "Mesh View", xmLabelGadgetClass, rend_child,
               NULL );
    XtVaGetValues( widg, XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;

    util_render_btns[VIEW_SOLID] = XtVaCreateManagedWidget(
                                       "Solid", xmToggleButtonWidgetClass, rend_child,
                                       XmNindicatorOn,  False,
                                       XmNset,  ( analy->mesh_view_mode == RENDER_FILLED ),
                                       XmNshadowThickness, 3,
                                       XmNfillOnSelect, True,
                                       NULL );
    XtVaGetValues( util_render_btns[VIEW_SOLID], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[VIEW_SOLID], XmNvalueChangedCallback,
                   util_render_CB, (XtPointer) VIEW_SOLID );

    XtOverrideTranslations( util_render_btns[VIEW_SOLID], key_trans );

    util_render_btns[VIEW_SOLID_MESH] = XtVaCreateManagedWidget(
                                            "Solid Mesh", xmToggleButtonWidgetClass, rend_child,
                                            XmNindicatorOn, False,
                                            XmNset, ( analy->mesh_view_mode == RENDER_HIDDEN ),
                                            XmNshadowThickness, 3,
                                            XmNfillOnSelect, True,
                                            NULL );
    XtVaGetValues( util_render_btns[VIEW_SOLID_MESH], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[VIEW_SOLID_MESH], XmNvalueChangedCallback,
                   util_render_CB, (XtPointer) VIEW_SOLID_MESH );

    XtOverrideTranslations( util_render_btns[VIEW_SOLID_MESH], key_trans );

    util_render_btns[VIEW_EDGES] = XtVaCreateManagedWidget(
                                       "Edges Only", xmToggleButtonWidgetClass, rend_child,
                                       XmNindicatorOn, False,
                                       XmNset, ( analy->show_edges
                                               && env.curr_analy->mesh_view_mode == RENDER_NONE ),
                                       XmNshadowThickness, 3,
                                       XmNfillOnSelect, True,
                                       NULL );
    XtVaGetValues( util_render_btns[VIEW_EDGES], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[VIEW_EDGES], XmNvalueChangedCallback,
                   util_render_CB, (XtPointer) VIEW_EDGES );

    XtOverrideTranslations( util_render_btns[VIEW_EDGES], key_trans );


    /* IRC: May 04, 2007 - Added buttons on utility panel for Wireframe render modes */

    util_render_btns[VIEW_WIREFRAME] = XtVaCreateManagedWidget(
                                           "Wireframe", xmToggleButtonWidgetClass, rend_child,
                                           XmNindicatorOn,  False,
                                           XmNset,  ( analy->mesh_view_mode == RENDER_WIREFRAME ),
                                           XmNshadowThickness, 3,
                                           XmNfillOnSelect, True,
                                           NULL );
    XtVaGetValues( util_render_btns[VIEW_WIREFRAME], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[VIEW_WIREFRAME], XmNvalueChangedCallback,
                   util_render_CB, (XtPointer) VIEW_WIREFRAME );

    XtOverrideTranslations( util_render_btns[VIEW_WIREFRAME], key_trans );

    /* IRC: Jan 09, 2008 - Added buttons on utility panel for Greyscale render modes */

    util_render_btns[VIEW_GS] = XtVaCreateManagedWidget(
                                    "Greyscale", xmToggleButtonWidgetClass, rend_child,
                                    XmNindicatorOn,  False,
                                    XmNset,  ( analy->material_greyscale ),
                                    XmNshadowThickness, 3,
                                    XmNfillOnSelect, True,
                                    NULL );
    XtVaGetValues( util_render_btns[VIEW_GS], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[VIEW_GS], XmNvalueChangedCallback,
                   util_render_CB, (XtPointer) VIEW_GS );

    XtOverrideTranslations( util_render_btns[VIEW_GS], key_trans );

    /*
     * Select/Hilite mode select radio buttons.
     */

    rend_child = XtVaCreateManagedWidget(
                     "render_pick", xmRowColumnWidgetClass, util_render_ctl,
                     XmNisAligned, True,
                     XmNentryAlignment, XmALIGNMENT_CENTER,
                     XmNradioBehavior, True,
                     XmNradioAlwaysOne, True,
                     XmNorientation, XmVERTICAL,
                     XmNpacking, XmPACK_COLUMN,
                     NULL );

    XtOverrideTranslations( rend_child, key_trans );

    widg = XtVaCreateManagedWidget(
               "Pick Mode", xmLabelGadgetClass, rend_child,
               NULL );
    XtVaGetValues( widg, XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    util_render_btns[PICK_MODE_SELECT] = XtVaCreateManagedWidget(
            "Select", xmToggleButtonWidgetClass, rend_child,
            XmNindicatorOn, False,
            XmNset, ( analy->mouse_mode == MOUSE_SELECT ),
            XmNshadowThickness, 3,
            XmNfillOnSelect, True,
            NULL );
    XtVaGetValues( util_render_btns[PICK_MODE_SELECT], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[PICK_MODE_SELECT], XmNvalueChangedCallback,
                   util_render_CB, (XtPointer) PICK_MODE_SELECT );

    XtOverrideTranslations( util_render_btns[PICK_MODE_SELECT], key_trans );

    util_render_btns[PICK_MODE_HILITE] = XtVaCreateManagedWidget(
            "Hilite", xmToggleButtonWidgetClass, rend_child,
            XmNindicatorOn, False,
            XmNset, ( analy->mouse_mode == MOUSE_HILITE ),
            XmNshadowThickness, 3,
            XmNfillOnSelect, True,
            NULL );
    XtVaGetValues( util_render_btns[PICK_MODE_HILITE], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[PICK_MODE_HILITE], XmNvalueChangedCallback,
                   util_render_CB, (XtPointer) PICK_MODE_HILITE );

    XtOverrideTranslations( util_render_btns[PICK_MODE_HILITE], key_trans );

    /*
     * Mouse-pick mesh object class selection buttons.
     */

    rend_child = XtVaCreateManagedWidget(
                     "bt2_pick", xmRowColumnWidgetClass, util_render_ctl,
                     XmNisAligned, True,
                     XmNentryAlignment, XmALIGNMENT_CENTER,
                     XmNorientation, XmVERTICAL,
                     XmNpacking, XmPACK_COLUMN,
                     NULL );

    XtOverrideTranslations( rend_child, key_trans );

    widg = XtVaCreateManagedWidget(
               "Btn Pick Class", xmLabelGadgetClass, rend_child,
               NULL );
    XtVaGetValues( widg, XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;

    util_render_btns[BTN1_PICK] = create_pick_menu( rend_child, BTN1_PICK,
                                  NULL );
    XtVaGetValues( util_render_btns[BTN1_PICK], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;

    XtOverrideTranslations( util_render_btns[BTN1_PICK], key_trans );

    util_render_btns[BTN2_PICK] = create_pick_menu( rend_child, BTN2_PICK,
                                  NULL );
    XtVaGetValues( util_render_btns[BTN2_PICK], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;

    XtOverrideTranslations( util_render_btns[BTN2_PICK], key_trans );

    util_render_btns[BTN3_PICK] = create_pick_menu( rend_child, BTN3_PICK,
                                  NULL );
    XtVaGetValues( util_render_btns[BTN3_PICK], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;

    XtOverrideTranslations( util_render_btns[BTN3_PICK], key_trans );

    /*
     * Mesh clean-up buttons.
     */

    rend_child = XtVaCreateManagedWidget(
                     "render_clean", xmRowColumnWidgetClass, util_render_ctl,
                     XmNisAligned, True,
                     XmNentryAlignment, XmALIGNMENT_CENTER,
                     XmNorientation, XmVERTICAL,
                     XmNpacking, XmPACK_COLUMN,
                     NULL );

    XtOverrideTranslations( rend_child, key_trans );

    widg = XtVaCreateManagedWidget(
               "Clean-up", xmLabelGadgetClass, rend_child,
               NULL );
    XtVaGetValues( widg, XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;

    util_render_btns[CLEAN_SELECT] = XtVaCreateManagedWidget(
                                         "Clear select", xmPushButtonGadgetClass, rend_child,
                                         NULL );
    XtVaGetValues( util_render_btns[CLEAN_SELECT], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[CLEAN_SELECT], XmNactivateCallback,
                   menu_CB, (XtPointer) BTN_CLEARSELECT );

    util_render_btns[CLEAN_HILITE] = XtVaCreateManagedWidget(
                                         "Clear hilite", xmPushButtonGadgetClass, rend_child,
                                         NULL );
    XtVaGetValues( util_render_btns[CLEAN_HILITE], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[CLEAN_HILITE], XmNactivateCallback,
                   menu_CB, (XtPointer) BTN_CLEARHILITE );

    util_render_btns[CLEAN_NEARFAR] = XtVaCreateManagedWidget(
                                          "Set near/far", xmPushButtonGadgetClass, rend_child,
                                          NULL );
    XtVaGetValues( util_render_btns[CLEAN_NEARFAR], XmNwidth, &width, NULL );
    if ( width > child_width )
        child_width = width;
    XtAddCallback( util_render_btns[CLEAN_NEARFAR], XmNactivateCallback,
                   menu_CB, (XtPointer) BTN_ADJUSTNF );

    /*
     * Finalize the column width for all the buttons.
     */

    rc_width = 10 * margin_width + 3 * spacing + 4 * child_width;

    XtVaSetValues( util_render_ctl,
                   XmNrightOffset, -((int) (rc_width / 2)), NULL );

    return util_panel_main;
}


/*****************************************************************
 * TAG( create_pick_menu )
 *
 * Create an option menu or a cascade menu to select mesh object
 * classes for pick actions.
 */
static Widget
create_pick_menu( Widget parent, Util_panel_button_type btn_type,
                  char *cascade_name )
{
    int n;
    Arg args[5];
    Widget pick_menu;
    Widget menu, initial_class_btn;

    pick_menu = create_pick_submenu( parent, btn_type, cascade_name,
                                     &initial_class_btn );

    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    XtSetArg( args[n], XmNsubMenuId, pick_menu );
    n++;

    if ( cascade_name == NULL )
    {
        /* Create an option menu. */
        XtSetArg( args[n], XmNmenuHistory, initial_class_btn );
        n++;
        XtSetArg( args[n], XmNmarginHeight, 0 );
        n++;
        XtSetArg( args[n], XmNmarginWidth, 0 );
        n++;
        menu = XmCreateOptionMenu( parent, "pick_option", args, n );
    }
    else
    {
        /* Create a cascade menu. */
        menu = XmCreateCascadeButton( parent, cascade_name, args, n );
    }

    XtManageChild( menu );

    return menu;
}


/*****************************************************************
 * TAG( create_pick_submenu )
 *
 * Create the submenu for an option menu or a cascade menu to
 * select mesh object classes for pick actions.
 */
static Widget
create_pick_submenu( Widget parent, Util_panel_button_type btn_type,
                     char *cascade_name, Widget *p_initial_button )
{
    int n, i, j;
    Arg args[5];
    Widget pick_submenu, button;
    Mesh_data *p_mesh;
    int qty;
    List_head *p_lh;
    int pref_sclass;
    MO_class_data **p_mo_classes;
    MO_class_data **pref_class;
    Widget init_class;

    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    pick_submenu = XmCreatePulldownMenu( parent, "pick_pane", args, n );

    p_mesh = MESH_P( env.curr_analy );

    switch ( btn_type )
    {
    case BTN1_PICK:
        pref_class = &btn1_mo_class;
        break;
    case BTN2_PICK:
        pref_class = &btn2_mo_class;
        break;
    case BTN3_PICK:
        pref_class = &btn3_mo_class;
        break;
    default:
        pref_class = NULL;
    }

    get_pick_superclass( btn_type, &pref_sclass );

    /*
     * Traverse the classes in the mesh and create PushButtonGadgets
     * for each class labeled with the class name.
     */

    /*
     * If a preferred class has already been defined for the button,
     * that class will be sought as the initial option for the button,
     * otherwise the first class of the preferred superclass will be the
     * initial menu item selected for the button.
     */

    qty = sizeof( pick_sclasses ) / sizeof( pick_sclasses[0] );
    init_class = NULL;
    for ( i = 0; i < qty; i++ )
    {
        p_lh = p_mesh->classes_by_sclass + pick_sclasses[i];

        if ( p_lh->qty != 0 )
        {
            p_mo_classes = (MO_class_data **) p_lh->list;

            n = 0;
            XtSetArg( args[n], XmNmarginHeight, 0 );
            n++;

            for ( j = 0; j < p_lh->qty; j++ )
            {
                /**/
                /* Change this when/if G_PARTICLE superclass is available. */
                /* For G_UNIT, only allow if it's the particle class. */
                if ( pick_sclasses[i] == G_UNIT
                        && strcmp( p_mo_classes[j]->short_name, particle_cname )
                        != 0 )
                    continue;

                button = XmCreatePushButtonGadget( pick_submenu,
                                                   p_mo_classes[j]->long_name,
                                                   args, n );
                XtManageChild( button );

                if ( cascade_name == NULL )
                    XtAddCallback( button, XmNactivateCallback, util_render_CB,
                                   (XtPointer) btn_type );
                else
                    XtAddCallback( button, XmNactivateCallback, menu_setpick_CB,
                                   (XtPointer) btn_type );

                /*
                 * For option menu, save button for user-specified class
                 * preference or first occurrence of preferred superclass.
                 */
                if ( cascade_name == NULL )
                {
                    if ( *pref_class != NULL )
                    {
                        if ( p_mo_classes[j] == *pref_class )
                            init_class = button;
                    }
                    else if ( init_class == NULL
                              && pick_sclasses[i] == pref_sclass
                              && j == 0 )
                    {
                        init_class = button;
                        *pref_class = p_mo_classes[j];
                    }
                }
            }
        }
    }

    if ( p_initial_button != NULL )
        *p_initial_button = init_class;

    return pick_submenu;
}

/*****************************************************************
 * TAG( create_colormap_menu )
 *
 * Create a cascade menu to select pre-defined colormap.
 */
static Widget
create_colormap_menu( Widget parent, Util_panel_button_type dummp,
                      char *cascade_name )
{
    int n;
    Arg args[5];
    Widget colormap_menu;
    Widget menu, p_initial_btn;
    colormap_type btn_type=CM_JET;

    /* Create a cascade menu. */
    colormap_menu = create_colormap_submenu( parent, btn_type,
                    &p_initial_btn );
    n = 0;
    XtSetArg( args[n], XmNsubMenuId, colormap_menu );
    n++;

    /* Create a cascade menu. */
    menu = XmCreateCascadeButton( parent, "Set Colormap", args, n );

    XtManageChild( menu );

    return menu;
}


/*****************************************************************
 * TAG( create_colormap_submenu )
 *
 * Create the submenu a cascade menu to select a pre-defined
 * colormap.
 */
static Widget
create_colormap_submenu( Widget parent, colormap_type btn_type,
                         Widget *p_initial_button )
{
    int n, i, j;
    Arg args[5];
    Widget pick_submenu, button;
    Mesh_data *p_mesh;
    Widget init_class;

    n = 0;
    XtSetArg( args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED );
    n++;
    pick_submenu = XmCreatePulldownMenu( parent, "colormap_pane", args, n );

    btn_type = CM_INVERSE;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "Inverse Colormap",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    button = XmCreateSeparatorGadget( pick_submenu, "separator", args, n );
    XtManageChild( button );

    btn_type = CM_COOL;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "Cool",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    btn_type = CM_DEFAULT;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "Hotmap(default)",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    btn_type = CM_GRAYSCALE;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "GrayScale",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    btn_type = CM_INVERSE_GRAYSCALE;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "Inverse GrayScale",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    btn_type = CM_HSV;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "HSV",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    btn_type = CM_JET;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "Jet",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    btn_type = CM_PRISM;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "Prism",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    btn_type = CM_WINTER;
    button = XmCreatePushButtonGadget( pick_submenu,
                                       "Winter",
                                       args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_setcolormap_CB,
                   (XtPointer) btn_type );

    return pick_submenu;
}


/*****************************************************************
 * TAG( init_btn_pick_classes )
 *
 * Initialize mesh object pick classes for each mouse button.
 */
void
init_btn_pick_classes( void )
{
    Mesh_data *p_mesh;
    int pref_sclass;

    p_mesh = MESH_P( env.curr_analy );

    get_pick_superclass( BTN1_PICK, &pref_sclass );
    btn1_mo_class =
        ((MO_class_data **) p_mesh->classes_by_sclass[pref_sclass].list)[0];

    get_pick_superclass( BTN2_PICK, &pref_sclass );
    btn2_mo_class =
        ((MO_class_data **) p_mesh->classes_by_sclass[pref_sclass].list)[0];

    get_pick_superclass( BTN3_PICK, &pref_sclass );
    btn3_mo_class =
        ((MO_class_data **) p_mesh->classes_by_sclass[pref_sclass].list)[0];
}


/*****************************************************************
 * TAG( get_pick_superclass )
 *
 * Determine a preferred mesh object superclass for association with a
 * mouse button for pick operations.
 */
static void
get_pick_superclass( Util_panel_button_type btn_type, int *p_superclass )
{
    int i, qty, pref_sclass;
    int *pref_order;
    Mesh_data *p_mesh;
    MO_class_data *p_mo_class;
    int *b1_pref_order = pick_sclasses; /* File-scope array. */
    /**/
    /* Replace all G_UNIT references to G_PARTICLE when/if available and remove
       special test in loop below
    */
    static int b2_pref_order[] =
    {
        G_QUAD, G_BEAM, G_UNIT, G_TET, G_TRI, G_TRUSS, G_WEDGE, G_PYRAMID,
        G_HEX, G_NODE, G_SURFACE, G_PARTICLE
    };
    static int b3_pref_order[] =
    {
        G_HEX, G_PYRAMID, G_WEDGE, G_TRUSS, G_TRI, G_TET, G_BEAM, G_QUAD,
        G_NODE, G_UNIT, G_SURFACE, G_PARTICLE
    };

    p_mesh = MESH_P( env.curr_analy );

    /* Initialize a preferred superclass based on which button is specified. */
    switch ( btn_type )
    {
    case BTN1_PICK:
        pref_order = b1_pref_order;
        break;
    case BTN2_PICK:
        pref_order = b2_pref_order;
        break;
    case BTN3_PICK:
        pref_order = b3_pref_order;
        break;
    default:
        pref_order = b1_pref_order;
    }

    qty = sizeof( pick_sclasses ) / sizeof( pick_sclasses[0] );

    for ( i = 0; i < qty ; i++ )
    {
        p_mo_class = (MO_class_data *)
                     p_mesh->classes_by_sclass[pref_order[i]].list;

        if ( p_mesh->classes_by_sclass[pref_order[i]].qty != 0 )
        {
            if ( p_mo_class != G_UNIT
                    || strcmp( p_mo_class->short_name, particle_cname ) == 0 )
            {
                pref_sclass = pref_order[i];
                break;
            }
        }
    }

    *p_superclass = pref_sclass;
}


/*****************************************************************
 * TAG( regenerate_pick_menus )
 *
 * Destroy existing button pick menus and re-create them for the
 * current database.
 */
extern void
regenerate_pick_menus( void )
{
    Widget menu, submenu, child;
    int position;

    /* Get the Picking menu, which is the parent of the setpick menus. */
    menu = NULL;
    find_labelled_child( menu_widg, "Picking", &child, &position );
    XtVaGetValues( child, XmNsubMenuId, &menu, NULL );

    /* Destroy the existing setpick1 menu. */
    XtVaGetValues( setpick_menu1_widg,
                   XmNsubMenuId, &submenu,
                   NULL );
    XtDestroyWidget( submenu );

    /* Create new setpick1 menu and assign it. */
    submenu = create_pick_submenu( menu, BTN1_PICK, "Set Btn 1 Pick", NULL );
    XtVaSetValues( setpick_menu1_widg, XmNsubMenuId, submenu, NULL );

    /* Destroy the existing setpick2 menu. */
    XtVaGetValues( setpick_menu2_widg,
                   XmNsubMenuId, &submenu,
                   NULL );
    XtDestroyWidget( submenu );

    /* Create new setpick2 menu and assign it. */
    submenu = create_pick_submenu( menu, BTN2_PICK, "Set Btn 2 Pick", NULL );
    XtVaSetValues( setpick_menu2_widg, XmNsubMenuId, submenu, NULL );

    /* Destroy the existing setpick3 menu. */
    XtVaGetValues( setpick_menu3_widg,
                   XmNsubMenuId, &submenu,
                   NULL );
    XtDestroyWidget( submenu );

    /* Create new setpick3 menu and assign it. */
    submenu = create_pick_submenu( menu, BTN3_PICK, "Set Btn 3 Pick", NULL );
    XtVaSetValues( setpick_menu3_widg, XmNsubMenuId, submenu, NULL );
}


/*****************************************************************
 * TAG( init_gui )
 *
 * This routine is called upon start-up of the gui.  It initializes
 * the GL window.
 */
static void
init_gui( void )
{
    FILE *test_file;
    char *init_file;
    char *home, home_path[512], home_hist[512];

    char init_cmd[100];
    if(MESH(env.curr_analy).material_qty == 0)
    {
        env.curr_analy->draw_wireframe = TRUE;
        env.curr_analy->mesh_view_mode = RENDER_WIREFRAME;
    }


    /*
        GLXwinset( XtDisplay(w), XtWindow(w) );
    */
    init_mesh_window( env.curr_analy );

    /* Set error handler so that rgb dump errors don't cause an exit. */
    i_seterror( do_nothing_stub );

    /*
     * Read in the start-up history file.  First check in current
     * directory, then in directory specified in environment path.
     */

    /* First read an init file from home directory */
    home = getenv( "HOME" );
    strcpy(home_hist, "h ");
    strcpy(home_path, home);
    strcat(home_path, "/grizinit");

    if ( ( test_file = fopen( home_path, "r" ) ) != NULL )
    {
        fclose( test_file );
        strcat(home_hist, home_path);
        parse_command( home_hist, env.curr_analy );
    }
    else
    {
        init_file = getenv( "GRIZINIT" );
        if ( init_file != NULL )
        {
            strcpy( init_cmd, "h " );
            strcat( init_cmd, init_file );
            if ( ( test_file = fopen( init_file, "r" ) ) != NULL )
            {
                fclose( test_file );
                parse_command( init_cmd, env.curr_analy );
            }
        }
    }

    /* Now read init file from local directory */
    if ( ( test_file = fopen( "grizinit", "r" ) ) != NULL )
    {
        fclose( test_file );
        parse_command( "h grizinit", env.curr_analy );
    }


    /* Now read from local directory - problem specific init file = grizinit.<plotfile_name> */
    strcpy(home_path, "grizinit.");
    strcat(home_path, env.plotfile_name);
    strcpy(home_hist, "h ");
    strcat(home_hist, home_path);

    if ( ( test_file = fopen( home_path, "r" ) ) != NULL )
    {
        fclose( test_file );
        parse_command( home_hist, env.curr_analy );
    }

}


/* ARGSUSED 3 */
/*****************************************************************
 * TAG( update_gui )
 *
 * Update gui callbacks and resources for render mode changes.
 */
void
update_gui( Analysis *analy, Render_mode_type new_rmode,
            Render_mode_type old_rmode )
{
    if ( new_rmode != RENDER_PLOT )
    {
        if ( old_rmode == RENDER_PLOT )
        {
#ifdef WANT_PLOT_CALLBACK
            XtRemoveCallback( ogl_widg[MESH_VIEW], GLwNinputCallback,
                              plot_input_CB, 0 );
#endif
            XtAddCallback( ogl_widg[MESH_VIEW], GLwNinputCallback, input_CB,
                           0 );

            manage_plot_cursor_display( analy->show_plot_coords, new_rmode,
                                        old_rmode );
        }
    }
    else
    {
        if ( old_rmode == RENDER_MESH_VIEW )
        {
            XtRemoveCallback( ogl_widg[MESH_VIEW], GLwNinputCallback,
                              input_CB, 0 );
#ifdef WANT_PLOT_CALLBACK
            XtAddCallback( ogl_widg[MESH_VIEW], GLwNinputCallback,
                           plot_input_CB, 0 );
#endif
            manage_plot_cursor_display( analy->show_plot_coords, new_rmode,
                                        old_rmode );
        }
    }
}


/* ARGSUSED 0 */
/*****************************************************************
 * TAG( remove_plot_coords_display )
 *
 * Turn the plot cursor coordinates display off.
 */
static void
remove_plot_coords_display( void )
{
    XtRemoveEventHandler( ogl_widg[MESH_VIEW], PointerMotionMask, False,
                          plot_cursor_EH, NULL );

    XtUnmanageChild( plot_coord_widg );

    /* Attach the bottom of the rendering window to the Form */
    XtVaSetValues( ogl_widg[MESH_VIEW],
                   XmNbottomOffset, 0,
                   NULL );
}


/* ARGSUSED 0 */
/*****************************************************************
 * TAG( install_plot_coords_display )
 *
 * Turn the plot cursor coordinates display on.
 */
static void
install_plot_coords_display( void )
{
    Dimension pc_height;

    pc_height = 0;
    XtVaGetValues( plot_coord_widg,
                   XmNheight, &pc_height,
                   NULL );

    /*
     * Attach the bottom of the rendering window to the top of the
     * plot coordinates display.
     */
    XtVaSetValues( ogl_widg[MESH_VIEW],
                   XmNbottomOffset, pc_height,
                   NULL );

    XtManageChild( plot_coord_widg );

    XtAddEventHandler( ogl_widg[MESH_VIEW], PointerMotionMask,
                       False, plot_cursor_EH, NULL );
}


/* ARGSUSED 3 */
/*****************************************************************
 * TAG( manage_plot_cursor_display )
 *
 * Turn the plot cursor coordinates display on or off.
 *
 * The given ordering of XtManageChild/XtUnManageChild and XtVaSetValues
 * calls incurs the fewest redraws (when used in conjunction with
 * the "suppress" flag and expose_resize_CB()).
 */
void
manage_plot_cursor_display( Bool_type turn_on, Render_mode_type new_rmode,
                            Render_mode_type old_rmode )
{
    Boolean managed;

    if ( old_rmode == RENDER_PLOT )
    {

        managed = XtIsManaged( plot_coord_widg );

        if ( new_rmode != RENDER_PLOT )
        {
            if ( managed )
                /* No plot display -> no plot coords display */
                remove_plot_coords_display();
        }
        else
        {
            /*
             * Plot render mode unchanged; consider current and requested
             * state of display.
             */

            if ( turn_on && !managed )
                install_plot_coords_display();
            else if ( !turn_on && managed )
                remove_plot_coords_display();
        }
    }
    else
    {
        /*
         * Existing mode is not RENDER_PLOT, so we can't need to turn
         * the plot coords display off; check if we need to turn it on.
         */

        if ( new_rmode == RENDER_PLOT )
        {
            if ( turn_on )
                install_plot_coords_display();
        }
    }
}


/* ARGSUSED 6 */
/*****************************************************************
 * TAG( set_plot_win_params )
 *
 * Save plotting window bounds.
 */
void
set_plot_win_params( float ll_x, float ll_y, float ur_x, float ur_y,
                     float *ax_mins, float *ax_maxs )
{
    win_xmin = ll_x;
    win_ymin = ll_y;
    win_xman = ur_x;
    win_ymax = ur_y;
    win_xspan = ur_x - ll_x;
    win_yspan = ur_y - ll_y;
    data_xmin = ax_mins[0];
    data_ymax = ax_maxs[1];
    data_xspan = ax_maxs[0] - data_xmin;
    data_yspan = data_ymax - ax_mins[1];
}


/* ARGSUSED 2 */
/*****************************************************************
 * TAG( set_plot_cursor_display )
 *
 * Update the plot cursor display widgets given cursor window coordinates.
 */
static void
set_plot_cursor_display ( int cursor_x, int cursor_y )
{
    char xstr[13], ystr[13];
    float cursor_x_f, cursor_y_f, cursor_data_x, cursor_data_y;

    cursor_x_f = (float) cursor_x;
    cursor_y_f = (float) cursor_y;

    cursor_data_x = data_xmin
                    + data_xspan * (cursor_x_f - win_xmin) / win_xspan;
    cursor_data_y = data_ymax
                    - data_yspan * (cursor_y_f - win_ymin) / win_yspan;

    if ( cursor_x_f >= win_xmin && cursor_x_f <= win_xman
            && cursor_y_f >= win_ymin && cursor_y_f <= win_ymax
            && env.curr_analy->current_plots != NULL )
    {
        sprintf( xstr, "%11.4e", cursor_data_x );
        sprintf( ystr, "%11.4e", cursor_data_y );
    }
    else
    {
        sprintf( xstr, " " );
        sprintf( ystr, " " );
    }

    XtVaSetValues( x_coord_widg, XmNvalue, xstr, NULL );
    XtVaSetValues( y_coord_widg, XmNvalue, ystr, NULL );
}


/*****************************************************************
 * TAG( update_cursor_vals )
 *
 * "Statically" update the displayed cursor coordinate values.
 */
void
update_cursor_vals( void )
{
    Window root_win, win, render_win;
    int root_x, root_y, top_win_x, top_win_y, win_x, win_y;
    unsigned int masks;
    Bool on_screen;

    /*
     * Ensure render_top_win is initialized - won't be if this is called
     * as a result of grizinit commands.
     */
    if ( rendershell_widg != NULL && render_top_win == 0 )
        find_ancestral_root_child( rendershell_widg, &render_top_win );

    /* Get cursor location. */
    on_screen = XQueryPointer( dpy, render_top_win, &root_win, &win,
                               &root_x, &root_y, &top_win_x, &top_win_y,
                               &masks );

    if ( win == None || !on_screen )
        /* Send values which will not be in the plot area. */
        set_plot_cursor_display( 0, 0 );
    else
    {
        win = XtWindow( ogl_widg[MESH_VIEW] );
        XTranslateCoordinates( dpy, render_top_win, win, top_win_x, top_win_y,
                               &win_x, &win_y, &render_win );
        set_plot_cursor_display( win_x, win_y );
    }
}


/* ARGSUSED 1 */
/*****************************************************************
 * TAG( suppress_display_updates )
 *
 * Set a flag to short circuit resize and expose callback redraws.
 */
void
suppress_display_updates( Bool_type suppress )
{
    suppress_updates = suppress;
}


#ifdef USE_OLD_CALLBACKS

/* ARGSUSED 0 */
/*****************************************************************
 * TAG( expose_CB )
 *
 * Called when expose events occur.  Redraws the mesh window.
 */
static void
expose_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    OpenGL_win save_win;

    save_win = cur_opengl_win;
    switch_opengl_win( MESH_VIEW );

    if ( !suppress_updates )
        env.curr_analy->update_display( env.curr_analy );
    if ( save_win != MESH_VIEW )
        switch_opengl_win( save_win );
}


/* ARGSUSED 1 */
/*****************************************************************
 * TAG( resize_CB )
 *
 * Called when resize events occur.  Resets the GL window size.
 */
static void
resize_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Dimension width, height;


    /* Don't use OpenGL until window is realized! */
    if ( XtIsRealized(w) )
    {
        XtVaGetValues( w, XmNwidth, &width, XmNheight, &height, NULL );

        /* Set viewport to fill the OpenGL window. */
        glViewport( 0, 0, (GLint)width, (GLint)height );
        set_mesh_view();
        resize_in_progress = TRUE;
        if ( !suppress_updates )
            env.curr_analy->update_display( env.curr_analy );
    }
}


#else


/* ARGSUSED 5 */
/*****************************************************************
 * TAG( get_last_renderable_event )
 *
 * Reads the event queue for a window to find the last of any
 * expose or resize events still in the queue.  Passes back
 * pertinent data from the last of any potential resize events
 * encountered.
 *
 * Parameter p_xe must point to the valid address of an XEvent structure.
 */
static Bool_type
get_last_renderable_event( Window win, XEvent *p_xe, Bool_type *p_resize,
                           Dimension *p_width, Dimension *p_height )
{
    long evt_mask;
    Bool_type found_either, found_resize;
    XEvent evt;

    found_either = found_resize = FALSE;
    evt_mask = ExposureMask | StructureNotifyMask;

    while ( XCheckWindowEvent( dpy, win, evt_mask, &evt ) )
    {
        if ( evt.type == ConfigureNotify )
        {
            *p_width = evt.xconfigure.width;
            *p_height = evt.xconfigure.height;

            found_resize = TRUE;
            found_either = TRUE;
        }
        else if ( evt.type == Expose )
            found_either = TRUE;
    }

    if ( found_either )
        *p_xe = evt;

    *p_resize = found_resize;

    return found_either;
}


/* ARGSUSED 3 */
/*****************************************************************
 * TAG( expose_resize_CB )
 *
 * Called when expose or resize events occur.  Looks ahead in the event
 * queue for additional events of either type to process them all in
 * one invocation of the callback with only one re-render.  Resets the
 * GL window size if any resize event is detected.
 */
static void
expose_resize_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Dimension width, height;
    XEvent last_event;
    Bool_type resize;
    OpenGL_win save_win;
    Window widg_win;


    save_win = cur_opengl_win;
    switch_opengl_win( MESH_VIEW );

    widg_win = XtWindow( w );

    if ( get_last_renderable_event( widg_win, &last_event, &resize, &width,
                                    &height ) )
    {
        if ( resize && XtIsRealized( w ) )
        {
            /*
             * There were additional events and at least one was a
             * resize event; update...
             */

            glViewport( 0, 0, (GLint)width, (GLint)height );
            set_mesh_view();
        }
        else
        {
            GLwDrawingAreaCallbackStruct *p_cbs
            = (GLwDrawingAreaCallbackStruct *) call_data;

            if ( p_cbs->reason == GLwCR_RESIZE && XtIsRealized( w ) )
            {
                /*
                 * There were additional events but not for resize.  However,
                 * the event prompting this callback was a resize; update...
                 */

                glViewport( 0, 0, (GLint) p_cbs->width, (GLint) p_cbs->height );
                set_mesh_view();
            }
        }
    }
    else
    {
        GLwDrawingAreaCallbackStruct *p_cbs
        = (GLwDrawingAreaCallbackStruct *) call_data;

        if ( p_cbs->reason == GLwCR_RESIZE && XtIsRealized( w ) )
        {
            /*
             * There were no additional events.  However, the event prompting
             * this callback was a resize; update...
             */
            glViewport( 0, 0, (GLint) p_cbs->width, (GLint) p_cbs->height );
            set_mesh_view();
        }
    }


    if ( !suppress_updates )
        env.curr_analy->update_display( env.curr_analy );

    if ( save_win != MESH_VIEW )
        switch_opengl_win( save_win );
}


#endif


/*****************************************************************
 * TAG( res_menu_CB )
 *
 * Handles menu button events for the result menu buttons.
 */
static void
res_menu_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    XmString text= NULL;
    Widget hist_list;
    char com_str[200];
    char *com;

    /* The result title is passed in the client_data. */
    com = (char *)client_data;
    sprintf( com_str, "show \"%s\"", com );
    text = XmStringCreateSimple( com_str );
    parse_command( com_str, env.curr_analy );
    if( text != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }
}


/*****************************************************************
 * TAG( menu_CB )
 *
 * Handles menu button events.
 *
 * NOTE:
 *      In the parallel griz case, there are some cases functions
 *      being disable because some information may not available
 *      on the client side (the master) such as the switch flag,
 *      env.curr_analy->show_tim etc.           Yuen L. Lee
 *
 *      id      : A request protocol, it services as a image refreshing
 *                request or a command request.  If id > 2 it services
 *                a command request and a message length as well.
 *
 *      cflag   : A toggle for determining whether the GuiMsgHandler
 *                should be called.
 *
 */
static void
menu_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Btn_type btn;
    XmString text=NULL;
    Widget hist_list;
    int status;
    btn = (Btn_type)client_data;

    switch( btn )
    {
    case BTN_COPYRIGHT:
        text = XmStringCreateSimple( "copyrt\0" );
        parse_command( "copyrt", env.curr_analy );
        break;
    case BTN_MTL_MGR:
        create_app_widg( BTN_MTL_MGR );
        break;
    case BTN_SURF_MGR:
        create_app_widg( BTN_SURF_MGR );
        break;
    case BTN_UTIL_PANEL:
        create_app_widg( BTN_UTIL_PANEL );
        break;

    case BTN_SAVE_SESSION_GLOBAL:
        write_griz_session_file( env.curr_analy, session, ".griz_session",
                                 env.griz_id, TRUE );
        break;

    case BTN_SAVE_SESSION_PLOT:
        update_session( PUT, NULL );

        write_griz_session_file( env.curr_analy, session, ".plot_session",
                                 env.griz_id, FALSE );
        break;

    case BTN_LOAD_SESSION_GLOBAL:
        status = read_griz_session_file( session, ".griz_session", env.griz_id,
                                         TRUE );

        if ( status==OK )
            put_griz_session( env.curr_analy, session );

        /* Update the window attributes */

        /*	    update_session( GET, NULL ); */

        put_window_attributes() ;
        break;

    case BTN_LOAD_SESSION_PLOT:
        status = read_griz_session_file( session, ".plot_session",
                                         env.griz_id, FALSE );
        break;

    case BTN_QUIT:
        parse_command( "quit", env.curr_analy );
        break;

    case BTN_DRAWFILLED:
        text = XmStringCreateSimple( "switch solid\0" );
        parse_command( "switch solid", env.curr_analy );
        break;
    case BTN_DRAWHIDDEN:
        text = XmStringCreateSimple( "switch hidden\0" );
        parse_command( "switch hidden", env.curr_analy );
        break;
    case BTN_DRAWWIREFRAME:
        text = XmStringCreateSimple( "switch wf\0" );
        parse_command( "switch wf", env.curr_analy );
        break;
    case BTN_DRAWWIREFRAMETRANS:
        text = XmStringCreateSimple( "switch wft\0" );
        parse_command( "switch wft", env.curr_analy );
        break;
    case BTN_COORDSYS:
        if ( env.curr_analy->show_coord )
        {
            text = XmStringCreateSimple( "off coord\0" );
            parse_command( "off coord", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on coord\0" );
            parse_command( "on coord", env.curr_analy );
        }
        break;
    case BTN_TITLE:
        if ( env.curr_analy->show_title )
        {
            text = XmStringCreateSimple( "off title\0" );
            parse_command( "off title", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on title\0" );
            parse_command( "on title", env.curr_analy );
        }
        break;
    case BTN_TIME:
        if ( env.curr_analy->show_time )
        {
            text = XmStringCreateSimple( "off time\0" );
            parse_command( "off time", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on time\0" );
            parse_command( "on time", env.curr_analy );
        }
        break;
    case BTN_COLORMAP:
        if ( env.curr_analy->show_colormap )
        {
            text = XmStringCreateSimple( "off cmap\0" );
            parse_command( "off cmap", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on cmap\0" );
            parse_command( "on cmap", env.curr_analy );
        }
        break;
    case BTN_MINMAX:
        if ( env.curr_analy->show_minmax )
        {
            text = XmStringCreateSimple( "off minmax\0" );
            parse_command( "off minmax", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on minmax\0" );
            parse_command( "on minmax", env.curr_analy );
        }
        break;
    case BTN_SCALE:
        if ( env.curr_analy->show_scale )
        {
            text = XmStringCreateSimple( "off scale\0" );
            parse_command( "off scale", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on scale\0" );
            parse_command( "on scale", env.curr_analy );
        }
        break;
    case BTN_EI:
        if ( env.curr_analy->ei_result )
        {
            text = XmStringCreateSimple( "off ei\0" );
            parse_command( "off ei", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on ei\0" );
            parse_command( "on ei", env.curr_analy );
        }
        break;
    case BTN_ALLON:
        text = XmStringCreateSimple( "on all\0" );
        parse_command( "on all", env.curr_analy );
        break;
    case BTN_ALLOFF:
        text = XmStringCreateSimple( "off all\0" );
        parse_command( "off all", env.curr_analy );
        break;
    case BTN_BBOX:
        if ( env.curr_analy->show_bbox )
        {
            text = XmStringCreateSimple( "off box\0" );
            parse_command( "off box", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on box\0" );
            parse_command( "on box", env.curr_analy );
        }
        break;
    case BTN_EDGES:
        if ( env.curr_analy->show_edges )
        {
            text = XmStringCreateSimple( "off edges\0" );
            parse_command( "off edges", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on edges\0" );
            parse_command( "on edges", env.curr_analy );
        }
        break;
    case BTN_GS:
        if ( env.curr_analy->material_greyscale )
        {
            text = XmStringCreateSimple( "off gs\0" );
            parse_command( "off gs", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on gs\0" );
            parse_command( "on gs", env.curr_analy );
        }
        break;
    case BTN_PERSPECTIVE:
        text = XmStringCreateSimple( "switch persp\0" );
        parse_command( "switch persp", env.curr_analy );
        break;
    case BTN_ORTHOGRAPHIC:
        text = XmStringCreateSimple( "switch ortho\0" );
        parse_command( "switch ortho", env.curr_analy );
        break;
    case BTN_ADJUSTNF:
        text = XmStringCreateSimple( "rnf\0" );
        parse_command( "rnf", env.curr_analy );
        break;
    case BTN_RESETVIEW:
        text = XmStringCreateSimple( "rview\0" );
        parse_command( "rview", env.curr_analy );
        break;
    case BTN_SU:
        if ( env.curr_analy->suppress_updates )
        {
            text = XmStringCreateSimple( "off su\0" );
            parse_command( "off su", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on su\0" );
            parse_command( "on su", env.curr_analy );
        }
        break;
    case BTN_HILITE:
        text = XmStringCreateSimple( "switch pichil\0" );
        parse_command( "switch pichil", env.curr_analy );
        break;
    case BTN_SELECT:
        text = XmStringCreateSimple( "switch picsel\0" );
        parse_command( "switch picsel", env.curr_analy );
        break;
    case BTN_CLEARHILITE:
        text = XmStringCreateSimple( "clrhil\0" );
        parse_command( "clrhil", env.curr_analy );
        break;
    case BTN_CLEARSELECT:
        text = XmStringCreateSimple( "clrsel\0" );
        parse_command( "clrsel", env.curr_analy );
        break;
    case BTN_CLEARALL:
        text = XmStringCreateSimple( "clrhil\0" );
        parse_command( "clrhil", env.curr_analy );
        parse_command( "clrsel", env.curr_analy );
        break;
    case BTN_PICSHELL:
        text = XmStringCreateSimple( "switch picsh\0" );
        parse_command( "switch picsh", env.curr_analy );
        break;
    case BTN_PICBEAM:
        text = XmStringCreateSimple( "switch picbm\0" );
        parse_command( "switch picbm", env.curr_analy );
        break;
    case BTN_CENTERON:
        text = XmStringCreateSimple( "vcent hi\0" );
        parse_command( "vcent hi", env.curr_analy );
        break;
    case BTN_CENTEROFF:
        text = XmStringCreateSimple( "vcent off\0" );
        parse_command( "vcent off", env.curr_analy );
        break;

    case BTN_INFINITESIMAL:
        text = XmStringCreateSimple( "switch infin\0" );
        parse_command( "switch infin", env.curr_analy );
        break;
    case BTN_ALMANSI:
        text = XmStringCreateSimple( "switch alman\0" );
        parse_command( "switch alman", env.curr_analy );
        break;
    case BTN_GREEN:
        text = XmStringCreateSimple( "switch grn\0" );
        parse_command( "switch grn", env.curr_analy );
        break;
    case BTN_RATE:
        text = XmStringCreateSimple( "switch rate\0" );
        parse_command( "switch rate", env.curr_analy );
        break;
    case BTN_GLOBAL:
        text = XmStringCreateSimple( "switch rglob\0" );
        parse_command( "switch rglob", env.curr_analy );
        break;
    case BTN_LOCAL:
        text = XmStringCreateSimple( "switch rloc\0" );
        parse_command( "switch rloc", env.curr_analy );
        break;
    case BTN_MIDDLE:
        text = XmStringCreateSimple( "switch middle\0" );
        parse_command( "switch middle", env.curr_analy );
        break;
    case BTN_INNER:
        text = XmStringCreateSimple( "switch inner\0" );
        parse_command( "switch inner", env.curr_analy );
        break;
    case BTN_OUTER:
        text = XmStringCreateSimple( "switch outer\0" );
        parse_command( "switch outer", env.curr_analy );
        break;

    case BTN_NEXTSTATE:
        text = XmStringCreateSimple( "n\0" );
        parse_command( "n", env.curr_analy );
        break;
    case BTN_PREVSTATE:
        text = XmStringCreateSimple( "p\0" );
        parse_command( "p", env.curr_analy );
        break;
    case BTN_FIRSTSTATE:
        text = XmStringCreateSimple( "f\0" );
        parse_command( "f", env.curr_analy );
        break;
    case BTN_LASTSTATE:
        text = XmStringCreateSimple( "l\0" );
        parse_command( "l", env.curr_analy );
        break;
    case BTN_ANIMATE:
        text = XmStringCreateSimple( "anim\0" );
        parse_command( "anim", env.curr_analy );
        break;
    case BTN_STOPANIMATE:
        text = XmStringCreateSimple( "stopan\0" );
        parse_command( "stopan", env.curr_analy );
        break;
    case BTN_CONTANIMATE:
        text = XmStringCreateSimple( "animc\0" );
        parse_command( "animc", env.curr_analy );
        break;

    case BTN_TIMEPLOT:
        text = XmStringCreateSimple( "timhis\0" );
        parse_command( "timhis", env.curr_analy );
        break;

    case BTN_HELP:
        text = XmStringCreateSimple( "help\0" );
        parse_command( "help", env.curr_analy );
        break;

    case BTN_RELNOTES:
        text = XmStringCreateSimple( "relnotes\0" );
        parse_command( "relnotes", env.curr_analy );
        break;

    case BTN_FN:
        if ( env.curr_analy->free_nodes_enabled )
        {
            text = XmStringCreateSimple( "off fn\0" );
            parse_command( "off fn", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on fn\0" );
            parse_command( "on fn", env.curr_analy );
        }
        break;

    case BTN_PN:
        if ( env.curr_analy->particle_nodes_enabled )
        {
            text = XmStringCreateSimple( "off pn\0" );
            parse_command( "off pn", env.curr_analy );
        }
        else
        {
            text = XmStringCreateSimple( "on pn\0" );
            parse_command( "on pn", env.curr_analy );
        }
        break;

    default:
        popup_dialog( INFO_POPUP, "Menu button %d is unrecognized", btn );
    }
    if( text != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }
}


/*****************************************************************
 * TAG( process_keyboard_input )
 *
 * Sends keyboard input from on to the Command widget.
 */
static void
process_keyboard_input( XKeyEvent *p_xke )
{
    KeySym keysym;
    char buffer[128];
    int klen;
    XEvent send_evt;

    /* No Control-modified key sequences are part of Griz commands. */
    /* Exception: Rubberband Zoom uses control sequences
     *            if ( p_xke->state & ControlMask )
     *	          return;
     */
    if ( p_xke->state & ControlMask )
        return;

    klen = XLookupString( p_xke, buffer, sizeof( buffer ), &keysym, NULL );
    buffer[klen] = '\0';

    send_evt = *((XEvent *) p_xke);
    send_evt.xkey.window = XtWindow( command_widg );
    send_evt.xkey.subwindow = None;
    XSendEvent( dpy, send_evt.xkey.window, True, KeyPressMask, &send_evt );
}


/*****************************************************************
 * TAG( input_CB )
 *
 * Callback for input events.
 */
static void
input_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    XmString text= NULL;
    Widget hist_list;
    static int posx, posy;
    int orig_posx, orig_posy;
    float angle, dx, dy, scale_fac, scale[3];
    char str[50];
    static int mode;
    int ident;
    Bool_type identify_only, yes_rx_ry;
    MO_class_data *p_class;
    Mesh_data *p_mesh;
    GLwDrawingAreaCallbackStruct *cb_data;
    float dist;
    double pixdx, pixdy;
    Analysis *analy;

    int save_center_view;

    XWindowChanges xwc;
    Bool_type popup_windows = FALSE;

    cb_data = (GLwDrawingAreaCallbackStruct *) call_data;

    analy = env.curr_analy;
    yes_rx_ry = !analy->limit_rotations;

    history_inputCB_cmd = TRUE;

    /* IRC: September 6, 2008. This code allows operations in the render window if we are using
     * the material manager color editor.
     */
    if (mtl_color_active)
    {
        mtl_color_active=FALSE;
        switch_opengl_win( MESH_VIEW );
        XtUnmanageChild( swatch_frame );
        XtUnmapWidget( swatch_label );
        XtSetSensitive( color_editor, False );
    }


    switch( cb_data->event->type )
    {
    case ButtonPress:

        popup_windows = TRUE;

        /* Rubberband Zoom */
        if (cb_data->event->xbutton.state & ControlMask)
        {
            switch ( cb_data->event->xbutton.button )
            {
            case Button1:
                startX = cb_data->event->xbutton.x;
                startY = cb_data->event->xbutton.y;
                lastX  = startX;
                lastY  = startY;

                obj_kind=RECTANGLE;
                mode = RB_MOVE;

                /* Determine the extents of the render window */
                XtVaGetValues( w,
                               XmNx, &ctl_x,
                               XmNy, &ctl_y,
                               XmNwidth, &ctl_width,
                               XmNheight, &ctl_height,
                               NULL );
                break;

            case Button2:
                mode = RB_STATIC;

                /* Set the view */

                rb_node_num = rb_node_hist[rb_hist_index];

                posx  = cb_data->event->xbutton.x;
                posy  = cb_data->event->xbutton.y;

                p_mesh = MESH_P( env.curr_analy );
                p_class =
                    ((MO_class_data **) p_mesh->classes_by_sclass[G_NODE].list)[0];

                identify_only = -1;
                rb_node_num = select_item( p_class, posx, posy, identify_only,
                                           analy );

                if (analy->rb_vcent_flag==FALSE)
                {
                    analy->rb_vcent_flag==TRUE;
                    sprintf( str, "vcent n %d", rb_node_num);
                    parse_command( str, analy);
                    history_command( str );
                }
                else
                {
                    analy->rb_vcent_flag==FALSE;
                    sprintf( str, "vcent off");
                    history_command( str );
                    parse_command( str, analy);
                }
                break;

            case Button3:

                rb_hist_index--;
                mode = RB_STATIC;

                if (rb_hist_index<0)
                {
                    rb_hist_index = 0;
                    break;
                }
                else
                {
                    scale[0] = rb_zoom_hist[rb_hist_index][0];
                    scale[1] = rb_zoom_hist[rb_hist_index][1];
                    scale[2] = rb_zoom_hist[rb_hist_index][2];
                }

                /* Disable vcent first */
                if ( analy->rb_vcent_flag )

                {
                    analy->rb_vcent_flag = FALSE;
                    sprintf( str, "vcent n %d", rb_node_num);
                    history_command( str );
                    parse_command( str, analy);
                }

                /* Perform the un-Zoom */

                if ( scale[0] == scale[1] && scale[1] == scale[2] )
                    sprintf( str, "scale %f", scale[0] );
                else
                    sprintf( str, "scalax %f %f %f", scale[0],
                             scale[1], scale[2] );
                history_command( str );

                set_mesh_scale( scale[0], scale[1], scale[2] );

                /* Set the view */

                rb_node_num = rb_node_hist[rb_hist_index];

                sprintf( str, "vcent n %d", rb_node_num);
                parse_command( str, analy);
                history_command( str );

                sprintf( str, "vcent off");
                history_command( str );
                parse_command( str, analy);

                /* Redraw the display */
                adjust_near_far( analy );
                analy->update_display( analy );
                break;
            }
        }
        else
        {
            posx = cb_data->event->xbutton.x;
            posy = cb_data->event->xbutton.y;
            mode = MOUSE_STATIC;
        }
        break;

    case ButtonRelease:

        /* Rubberband Zoom */
        if ( (cb_data->event->xbutton.state & ControlMask) &&
                mode == RB_MOVE )
        {
            int       rb_x, rb_y, rb_size;
            int       win_center_x, win_center_y;
            int       rb_center_x, rb_center_y;
            int       identify_only=-1;

            lastX  = cb_data->event->xbutton.x;
            lastY  = cb_data->event->xbutton.y;

            XDrawRectangle( XtDisplay ( w ), XtWindow ( w ), gc_rubber,
                            MIN(startX,lastX), MIN(startY,lastY),
                            ABS(lastX-startX), ABS(lastY-startY) );

            /* Determine the extents of the render window */
            XtVaGetValues( w,
                           XmNx, &ctl_x,
                           XmNy, &ctl_y,
                           XmNwidth, &ctl_width,
                           XmNheight, &ctl_height,
                           NULL );

            rb_x    = lastX - startX;
            rb_y    = lastY - startY;
            rb_size =  rb_x*rb_y;

            rb_size = ABS(rb_size);
            if (rb_size <1)
                rb_size = 1;

            if (rb_hist_index>=RB_MAX_HIST) rb_hist_index=0;

            /* Translate model so that zoom area is in the center of the window */
            /* Use the vcent n command */

            rb_center_x   = startX+((lastX-startX)/2.0);
            rb_center_y   = startY+((lastY-startY)/2.0);

            p_mesh = MESH_P( env.curr_analy );
            p_class =
                ((MO_class_data **) p_mesh->classes_by_sclass[G_NODE].list)[0];

            rb_node_num = select_item( p_class, rb_center_x, rb_center_y, identify_only,
                                       analy );

            /* Save the original scale setting in position 0 of the history variable */

            if (rb_hist_index==0)
            {
                get_mesh_scale( scale );
                rb_zoom_hist[rb_hist_index][0] = scale[0];
                rb_zoom_hist[rb_hist_index][1] = scale[1];
                rb_zoom_hist[rb_hist_index][2] = scale[2];
            }

            rb_node_hist[rb_hist_index] = rb_node_num;

            if (rb_hist_index<RB_MAX_HIST - 1)
                rb_hist_index++;


            /* Perform the Rubber Band Zoom  - do not zoom up to completely fill the
            *  window - fill it to within 90%
            */
            scale_fac = 1.28*log((double)((ctl_width*ctl_height)/rb_size));
            if (scale_fac<1.0) scale_fac = 1.0;

            get_mesh_scale( scale );

            VEC_ADDS( scale, scale_fac, scale, scale );
            if ( scale[0] == scale[1] && scale[1] == scale[2] )
                sprintf( str, "scale %f", scale[0] );
            else
                sprintf( str, "scalax %f %f %f", scale[0],
                         scale[1], scale[2] );
            history_command( str );

            if (scale[0]<=0) scale[0] = 1.0;
            if (scale[1]<=0) scale[1] = 1.0;
            if (scale[2]<=0) scale[2] = 1.0;

            rb_zoom_hist[rb_hist_index][0] = scale[0];
            rb_zoom_hist[rb_hist_index][1] = scale[1];
            rb_zoom_hist[rb_hist_index][2] = scale[2];

            set_mesh_scale( scale[0], scale[1], scale[2] );

            save_center_view = analy->center_view;
            sprintf( str, "vcent n %d", rb_node_num);
            history_command( str );
            parse_command( str, analy);
            analy->center_view = save_center_view;

            /* Redraw the display */
            adjust_near_far ( analy );
            analy->update_display( analy );

            analy->rb_vcent_flag = TRUE;
        }

        /* Check for completion of a resize event */
        if ( resize_in_progress )
        {
            analy->update_display( analy );
            resize_in_progress = FALSE;

        }

        /* End of Rubberband Zoom code */

        /* IRC: April 25, 2007 - Added check to make sure  we really had a button press */
        if ( mode == MOUSE_STATIC && (cb_data->event->xbutton.button==Button1  ||
                                      cb_data->event->xbutton.button==Button2  ||
                                      cb_data->event->xbutton.button==Button3))
        {
            identify_only = ( cb_data->event->xbutton.state & ShiftMask ) ;

            switch ( cb_data->event->xbutton.button )
            {
            case Button1:
                p_class = btn1_mo_class;
                break;
            case Button2:
                p_class = btn2_mo_class;
                break;
            case Button3:
                p_class = btn3_mo_class;
                break;
            }

            ident = select_item( p_class, posx, posy, identify_only,
                                 analy );

            if ( identify_only && ident > 0 )
                if ( cb_data->event->xbutton.state & ShiftMask )
                {
                    sprintf( str, "tellpos %s %d", p_class->short_name,
                             ident );
                    text = XmStringCreateSimple( str );
                    parse_command( str, analy );

                    ident--;

                    if ( IS_ELEMENT_SCLASS( p_class->superclass ) )
                        select_mtl_mgr_mtl(
                            p_class->objects.elems->mat[ident] );
                    else
                        popup_dialog( INFO_POPUP,
                                      "To select a material, please\n%s",
                                      "pick an element of the material." );
                }
        }
        else
        {
            unset_alt_cursor();
            analy->update_display( analy );
        }

        popup_windows = TRUE;
        break;

    case MotionNotify:
        orig_posx = posx;
        orig_posy = posy;

        /* Rubberband Zoom */

        if ( Button1Mask && mode == RB_MOVE)
        {
            /* Add depth bias. */
            popup_windows = FALSE;

            glDepthRange( 0, .1 );

            XDrawRectangle( XtDisplay ( w ), XtWindow ( w ), gc_rubber,
                            MIN(startX,lastX), MIN(startY,lastY),
                            ABS(lastX-startX), ABS(lastY-startY) );

            lastX  = cb_data->event->xbutton.x;
            lastY  = cb_data->event->xbutton.y;

            XDrawRectangle( XtDisplay ( w ), XtWindow ( w ), gc_rubber,
                            MIN(startX,lastX), MIN(startY,lastY),
                            ABS(lastX-startX), ABS(lastY-startY) );

            /* Remove depth bias. */
            glDepthRange( 0, 1 );

            break;
        }

        /* Rubberband Zoom */

        /* Cursor motion must exceed threshold to be a real move. */
        if ( mode == MOUSE_STATIC )
        {
            pixdx = (double) cb_data->event->xbutton.x - orig_posx;
            pixdy = (double) cb_data->event->xbutton.y - orig_posy;
            dist = (float) sqrt( pixdx * pixdx + pixdy * pixdy );
            if ( dist < motion_threshold )
                break;
            else
            {
                mode = MOUSE_MOVE;

                if ( cb_data->event->xmotion.state & Button2Mask
                        || cb_data->event->xmotion.state & Button3Mask
                        || yes_rx_ry )
                    set_alt_cursor( CURSOR_FLEUR );
            }
        }

        posx = cb_data->event->xbutton.x;
        posy = cb_data->event->xbutton.y;

        if ( cb_data->event->xmotion.state & Button1Mask && yes_rx_ry )
        {
            angle = (posx - orig_posx) / 10.0;
            sprintf( str, "ry %f", angle );
            history_command( str );
            inc_mesh_rot( 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
                          DEG_TO_RAD(angle) );

            angle = (posy - orig_posy) / 10.0;
            sprintf( str, "rx %f", angle );
            history_command( str );
            inc_mesh_rot( 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                          DEG_TO_RAD(angle) );

            /*
             * Put this call within each block so if we do have
             * Button 1 motion but out-of-plane rotations are turned off,
             * we don't extraneously get the edge-view anyway.
             */
            quick_draw_mesh_view( analy );
        }
        else if ( cb_data->event->xmotion.state & Button2Mask )
        {
            angle = - (posx - orig_posx) / 10.0;
            sprintf( str, "rz %f", angle );
            history_command( str );
            inc_mesh_rot( 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, DEG_TO_RAD(angle) );

            get_mesh_scale( scale );
            scale_fac = - (posy - orig_posy) * 0.0005;
            VEC_ADDS( scale, scale_fac, scale, scale );
            if ( scale[0] == scale[1] && scale[1] == scale[2] )
                sprintf( str, "scale %f", scale[0] );
            else
                sprintf( str, "scalax %f %f %f", scale[0],
                         scale[1], scale[2] );
            history_command( str );
            set_mesh_scale( scale[0], scale[1], scale[2] );

            quick_draw_mesh_view( analy );
        }
        else if ( cb_data->event->xmotion.state & Button3Mask )
        {
            if (analy->rb_vcent_flag)
            {
                analy->rb_vcent_flag==FALSE;
                sprintf( str, "vcent off");
                history_command( str );
                parse_command( str, analy);
            }

            dx = (posx - orig_posx) / 700.0;
            dy = - (posy - orig_posy) / 700.0;
            inc_mesh_trans( dx, dy, 0.0 );
            sprintf( str, "tx %f", dx );
            history_command( str );
            sprintf( str, "ty %f", dy );
            history_command( str );

            quick_draw_mesh_view( analy );
        }

        break;
    }
    if( text != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }

    if ( popup_windows )
        pushpop_window( PUSHPOP_ABOVE );
}


#ifdef WANT_PLOT_CALLBACK
/*****************************************************************
 * TAG( plot_input_CB )
 *
 * Callback for input events during curve-plotting render mode.
 */
static void
plot_input_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    static int posx, posy;
    static int mode;
    int orig_posx, orig_posy;
    GLwDrawingAreaCallbackStruct *cb_data;
    char xstr[13], ystr[13];

    cb_data = (GLwDrawingAreaCallbackStruct *) call_data;

    switch( cb_data->event->type )
    {
        /* This logic is for debugging. */
    case ButtonPress:
        mode = MOUSE_STATIC;
        break;

    case ButtonRelease:
        if ( mode == MOUSE_STATIC )
            popup_dialog( INFO_POPUP, "Plot win coords: %f %f %f %f",
                          win_xmin, win_ymin,
                          win_xman, win_ymax );
    }
}
#endif


/*****************************************************************
 * TAG( plot_cursor_EH )
 *
 * Event handler for updating the plot cursor values display.
 */
static void
plot_cursor_EH( Widget w, XtPointer client_data, XEvent *event,
                Boolean *continue_dispatch )
{
    set_plot_cursor_display( event->xmotion.x, event->xmotion.y );
}


/*****************************************************************
 * TAG( enter_render_EH )
 *
 * Event handler to set focus to Command widget upon entering the
 * rendering window.
 */
static void
enter_render_EH( Widget w, XtPointer client_data, XEvent *event,
                 Boolean *continue_dispatch )
{
    XmProcessTraversal( command_widg, XmTRAVERSE_CURRENT );
}


/*****************************************************************
 * TAG( stack_init_EH )
 *
 * Event handler which installs the stack_window_EH.  This
 * "logical indirection" is performed since the identified
 * "root child" ancestor window at the initial Expose event is not
 * correct.  Kind of a hack...
 */
static void
stack_init_EH( Widget w, XtPointer client_data, XEvent *event,
               Boolean *continue_dispatch )
{
    Shell_win_type swtype;

    swtype = (Shell_win_type) client_data;

    switch ( swtype )
    {
    case RENDER_SHELL_WIN:
        /* Remove self. */
        XtRemoveEventHandler( ogl_widg[MESH_VIEW], ExposureMask, False,
                              stack_init_EH, (XtPointer) RENDER_SHELL_WIN );

        /*
         * Now add the event handler on the top-level visible widgets
         * which are always available.
         */

        /* Rendering window widget. */
        XtAddEventHandler( ogl_widg[MESH_VIEW], ExposureMask, False,
                           stack_window_EH, (XtPointer) RENDER_SHELL_WIN );

        /* Control window widgets. */
        XtAddEventHandler( menu_widg, ExposureMask, False,
                           stack_window_EH, (XtPointer) CONTROL_SHELL_WIN );
        XtAddEventHandler( monitor_widg, ExposureMask, False,
                           stack_window_EH, (XtPointer) CONTROL_SHELL_WIN );
        XtAddEventHandler( command_widg, ExposureMask, False,
                           stack_window_EH, (XtPointer) CONTROL_SHELL_WIN );
        if ( util_panel_widg != NULL && include_util_panel )
        {
            XtAddEventHandler( util_panel_main, ExposureMask, False,
                               stack_window_EH,
                               (XtPointer) CONTROL_SHELL_WIN );
            XtAddEventHandler( util_render_ctl, ExposureMask, False,
                               stack_window_EH,
                               (XtPointer) UTIL_PANEL_SHELL_WIN );
            XtAddEventHandler( util_state_ctl, ExposureMask, False,
                               stack_window_EH,
                               (XtPointer) UTIL_PANEL_SHELL_WIN );
        }
        break;

    case UTIL_PANEL_SHELL_WIN:
        /* Will only get here with freestanding Utility Panel. */

        /*
         * Ensure the top window is unset.  It can be set incorrectly
         * if stack_window_EH is called as a result of creating the
         * Utility Panel from the pulldown, causing the UP top window to
         * be determined before (apparently) the window manager has
         * re-parented by adding its decorations.
         */
        util_panel_top_win = 0;

        /* Remove self. */
        XtRemoveEventHandler( util_panel_widg, StructureNotifyMask, False,
                              stack_init_EH,
                              (XtPointer) UTIL_PANEL_SHELL_WIN );

        /* Add handlers to cover the face of the Utility Panel. */
        XtAddEventHandler( util_panel_main, ExposureMask, False,
                           stack_window_EH,
                           (XtPointer) UTIL_PANEL_SHELL_WIN );
        XtAddEventHandler( util_render_ctl, ExposureMask, False,
                           stack_window_EH,
                           (XtPointer) UTIL_PANEL_SHELL_WIN );
        XtAddEventHandler( util_state_ctl, ExposureMask, False,
                           stack_window_EH,
                           (XtPointer) UTIL_PANEL_SHELL_WIN );
        break;

    case MTL_MGR_SHELL_WIN:
        /*
         * Ensure the top window is unset.  It can be set incorrectly
         * if stack_window_EH is called as a result of creating the
         * Material Manager from the pulldown, causing the MM top window to
         * be determined before (apparently) the window manager has
         * re-parented by adding its decorations.
         */
        mtl_mgr_top_win = 0;

        /* Remove self. */
        XtRemoveEventHandler( mtl_mgr_widg, StructureNotifyMask, False,
                              stack_init_EH,
                              (XtPointer) MTL_MGR_SHELL_WIN );

        /* Add handlers to cover the face of the Material Manager. */
        XtAddEventHandler( mtl_base, ExposureMask, False,
                           stack_window_EH,
                           (XtPointer) MTL_MGR_SHELL_WIN );
        XtAddEventHandler( color_editor, ExposureMask, False,
                           stack_window_EH,
                           (XtPointer) MTL_MGR_SHELL_WIN );
        break;

    case SURF_MGR_SHELL_WIN:

        surf_mgr_top_win = 0;

        /* Remove self. */
        XtRemoveEventHandler( surf_mgr_widg, StructureNotifyMask, False,
                              stack_init_EH,
                              (XtPointer) SURF_MGR_SHELL_WIN );

        /* Add handlers to cover the face of the Surface Manager. */
        XtAddEventHandler( surf_base, ExposureMask, False,
                           stack_window_EH,
                           (XtPointer) SURF_MGR_SHELL_WIN );
    }
}


/*****************************************************************
 * TAG( stack_window_EH )
 *
 * Event handler for updating the window stacking order.
 */
static void
stack_window_EH( Widget w, XtPointer client_data, XEvent *event,
                 Boolean *continue_dispatch )
{
    static Window win_list[5];
    Window wins[sizeof( win_list ) / sizeof( win_list[0] )];
    Window inrootwin, outrootwin, parentwin, tmpwin;
    Window *children;
    unsigned int child_qty;
    int i, j, index;
    int wincnt;
    Bool_type absolute_stack_move;
    XWindowChanges winvals =
    {
        0, 0, 0, 0, 0, 0, Below
    }
    ;
    /* Don't bother if there are more expose events still to be processed. */
    if ( event->type == Expose && event->xexpose.count != 0 )
        return;

    /* Get a list of the root window's children. */
    outrootwin = 0;
    parentwin = 0;
    inrootwin = RootWindow( dpy, DefaultScreen( dpy ) );
    XQueryTree( dpy, inrootwin, &outrootwin, &parentwin, &children,
                &child_qty );

    /* Make sure we have the child-of-root ancestor window for each widget. */
    if ( rendershell_widg != NULL && render_top_win == 0 )
        find_ancestral_root_child( rendershell_widg, &render_top_win );

    if ( ctl_shell_widg != NULL && ctl_top_win == 0 )
        find_ancestral_root_child( ctl_shell_widg, &ctl_top_win );

    if ( util_panel_widg != NULL && !include_util_panel
            && util_panel_top_win == 0 )
        find_ancestral_root_child( util_panel_widg, &util_panel_top_win );

    if ( mtl_mgr_widg != NULL && mtl_mgr_top_win == 0 )
        find_ancestral_root_child( mtl_mgr_widg, &mtl_mgr_top_win );

    if ( surf_mgr_widg != NULL && surf_mgr_top_win == 0 )
        find_ancestral_root_child( surf_mgr_widg, &surf_mgr_top_win );

    /* Init an array of currently existing windows. */
    wincnt = 0;
    wins[wincnt++] = render_top_win;
    wins[wincnt++] = ctl_top_win;
    if ( util_panel_widg != NULL && !include_util_panel )
        wins[wincnt++] = util_panel_top_win;
    if ( mtl_mgr_widg != NULL )
        wins[wincnt++] = mtl_mgr_top_win;
    if ( surf_mgr_widg != NULL )
        wins[wincnt++] = surf_mgr_top_win;

    /*
     * Sort extant windows by stacking order - top first.
     *
     * Search backwards through root's children because top windows are last
     * and Griz windows can be expected to be at or near the top.
     */
    index = 0;
    absolute_stack_move = FALSE;
    for ( i = child_qty - 1; i > -1; i-- )
    {
        for ( j = index; j < wincnt; j++ )
        {
            if ( children[i] == wins[j] )
            {
                SWAP( tmpwin, wins[j], wins[index] );
                if ( child_qty - 1 - index != i )
                    absolute_stack_move = TRUE;
                index++;
                break;
            }
        }

        if ( index == wincnt )
            break;
    }

    /* Don't need the kids anymore - bye darlings! */
    XFree( children );
    children = NULL;

    /*
     * If current relative order is same as last order and no one moved in the
     * stack, there's no need to call XConfigureWindow().
     */
    if ( memcmp( win_list, wins, wincnt * sizeof( win_list[0] ) ) )
    {
        /* New window stacking order - save. */
        for ( i = 0; i < wincnt; i++ )
            win_list[i] = wins[i];
    }
    else if ( !absolute_stack_move )
        return;

    /* Restack the windows at the top maintaining relative order. */
    for ( i = 1; i < wincnt; i++ )
    {
        winvals.sibling = win_list[i - 1];
        XConfigureWindow( dpy, win_list[i], CWStackMode | CWSibling, &winvals );
    }
}


/*****************************************************************
 * TAG( parse_CB )
 *
 * The callback for handling commands entered in the command dialog.
 */
static void
parse_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    char text[200];
    string_convert( ((XmCommandCallbackStruct *) call_data)->value, text );

    /* pushpop_window( PUSHPOP_ABOVE ); */

    /* Call the parsing guys. */
    parse_command( text, env.curr_analy );
}


/*****************************************************************
 * TAG( mtl_func_select_CB )
 *
 * The callback that manages material manager function selection.
 * It implements the natural logical exclusivity between the color
 * function, visibility/invisibility, and enable/disable.
 */
static void
mtl_func_select_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Widget *function_toggles;
    Boolean set, comp_set, color_set;
    int i;
    Mtl_mgr_func_type compliment;
    XmToggleButtonCallbackStruct *cb_data;

    function_toggles = (Widget *) client_data;
    cb_data = (XmToggleButtonCallbackStruct *) call_data;

    /* Find the index of the complimentary function to the widget executed. */
    for ( i = 0; i < 4; i++ )
        if ( w == function_toggles[i] )
            break;
    switch ( i )
    {
    case VIS:
        compliment = INVIS;
        break;
    case INVIS:
        compliment = VIS;
        break;
    case ENABLE:
        compliment = DISABLE;
        break;
    case DISABLE:
        compliment = ENABLE;
        break;
    }

    /* Color function toggle. */
    if ( w == function_toggles[COLOR] )
    {
        if ( cb_data->set )
        {
            mtl_color_active=TRUE;

            /* Exclude other functions while color is selected. */
            XtVaGetValues( function_toggles[VIS], XmNset, &set, NULL );
            XtVaGetValues( function_toggles[INVIS], XmNset, &comp_set, NULL );
            if ( set )
                XtVaSetValues( function_toggles[VIS], XmNset, False, NULL );
            else if ( comp_set )
                XtVaSetValues( function_toggles[INVIS], XmNset, False, NULL );

            XtVaGetValues( function_toggles[ENABLE], XmNset, &set, NULL );
            XtVaGetValues( function_toggles[DISABLE], XmNset, &comp_set, NULL );
            if ( set )
                XtVaSetValues( function_toggles[ENABLE], XmNset, False, NULL );
            else if ( comp_set )
                XtVaSetValues( function_toggles[DISABLE], XmNset, False, NULL );

            XtSetSensitive( color_editor, True );

            XtManageChild( swatch_frame );
            switch_opengl_win( SWATCH );
            update_swatch_label();
            XtMapWidget( swatch_label );

            /* Set color scales if a component is selected. */
            set_scales_to_mtl();
        }
        else
        {
            mtl_color_active=FALSE;

            switch_opengl_win( MESH_VIEW );
            XtUnmanageChild( swatch_frame );
            XtUnmapWidget( swatch_label );
            XtSetSensitive( color_editor, False );
        }
    }
    else
    {
        /* If color or invisible functions set, unset them. */
        XtVaGetValues( function_toggles[COLOR], XmNset, &color_set, NULL );
        XtVaGetValues( function_toggles[compliment], XmNset, &set, NULL );
        if ( color_set )
        {
            XtVaSetValues( function_toggles[COLOR], XmNset, False, NULL );

            switch_opengl_win( MESH_VIEW );
            XtUnmanageChild( swatch_frame );
            XtUnmapWidget( swatch_label );
            XtSetSensitive( color_editor, False );

            if ( preview_set )
            {
                /* Cancel the preview. */
                send_mtl_cmd( "mtl cancel", 2 );
                XtSetSensitive( mtl_row_col, True );
                XtSetSensitive( mtl_quick_select, True );
                preview_set = FALSE;
            }
        }
        else if ( set )
            XtVaSetValues( function_toggles[compliment], XmNset, False, NULL );
    }

    update_actions_sens();
}

/* ARGSUSED 2 */
/*****************************************************************
 * TAG( mtl_quick_select_CB )
 *
 * The callback for the material manager quick select buttons
 * which allow certain selections among the entire set of
 * materials in one step.
 */
static void
mtl_quick_select_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Widget **ctl_buttons;
    Boolean set, vis_set, invis_set, enable_set, disable_set;
    unsigned char *mtl_invis, *mtl_disable;
    int qty_mtls, i;
    WidgetList children;
    Material_list_obj *p_mtl;
    Mesh_data *p_mesh;

    p_mesh = env.curr_analy->mesh_table;
    qty_mtls = p_mesh->material_qty;
    mtl_invis = p_mesh->hide_material;
    mtl_disable = p_mesh->disable_material;
    ctl_buttons = (Widget **) client_data;
    XtVaGetValues( mtl_row_col, XmNchildren, &children, NULL );

    if ( w == ctl_buttons[1][ALL_MTL] )
    {
        /* Select all materials. */

        if ( mtl_deselect_list != NULL )
        {
            APPEND( mtl_deselect_list, mtl_select_list );
            mtl_deselect_list = NULL;
        }
        for ( i = qty_mtls - 1, p_mtl = mtl_select_list;
                i >= 0;
                p_mtl = p_mtl->next, i-- )
        {
            XtVaSetValues( children[i], XmNset, True, NULL );
            p_mtl->mtl = i + 1;
        }
    }
    else if ( w == ctl_buttons[1][NONE] )
    {
        /* Deselect all materials. */

        if ( mtl_select_list != NULL )
        {
            APPEND( mtl_select_list, mtl_deselect_list );
            mtl_select_list = NULL;
        }
        for ( i = 0; i < qty_mtls; i++ )
            XtVaSetValues( children[i], XmNset, False, NULL );
    }
    else if ( w == ctl_buttons[1][INVERT] )
    {
        /* Invert all selections. */

        /* Swap the lists. */
        p_mtl = mtl_deselect_list;
        mtl_deselect_list = mtl_select_list;
        mtl_select_list = p_mtl;

        /* Toggle the (now) selected toggles on, deselected off. */
        p_mtl = mtl_select_list;
        for ( i = qty_mtls - 1; i >= 0; i-- )
        {
            XtVaGetValues( children[i], XmNset, &set, NULL );
            if ( set )
                XtVaSetValues( children[i], XmNset, False, NULL );
            else
            {
                XtVaSetValues( children[i], XmNset, True, NULL );
                if ( p_mtl != NULL )
                {
                    p_mtl->mtl = i + 1;
                    p_mtl = p_mtl->next;
                }
                else
                    popup_dialog( WARNING_POPUP,
                                  "Material selection list does not match set state." );
            }
        }
    }
    else if ( w == ctl_buttons[1][BY_FUNC] )
    {
        /*
         * Select materials which are the intersection of the
         * selected functions.
         */

        /* First empty the material select list. */
        if ( mtl_select_list != NULL )
        {
            APPEND( mtl_select_list, mtl_deselect_list );
            mtl_select_list = NULL;
        }

        XtVaGetValues( ctl_buttons[0][VIS], XmNset, &vis_set, NULL );
        XtVaGetValues( ctl_buttons[0][INVIS], XmNset, &invis_set, NULL );
        XtVaGetValues( ctl_buttons[0][ENABLE], XmNset, &enable_set, NULL );
        XtVaGetValues( ctl_buttons[0][DISABLE], XmNset, &disable_set, NULL );

        if ( vis_set && enable_set )
        {
            for ( i = 0; i < qty_mtls; i++ )
                if ( !mtl_invis[i] && !mtl_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( vis_set && disable_set )
        {
            for ( i = 0; i < qty_mtls; i++ )
                if ( !mtl_invis[i] && mtl_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( invis_set && enable_set )
        {
            for ( i = 0; i < qty_mtls; i++ )
                if ( mtl_invis[i] && !mtl_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( invis_set && disable_set )
        {
            for ( i = 0; i < qty_mtls; i++ )
                if ( mtl_invis[i] && mtl_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( vis_set )
        {
            for ( i = 0; i < qty_mtls; i++ )
                if ( !mtl_invis[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( invis_set )
        {
            for ( i = 0; i < qty_mtls; i++ )
                if ( mtl_invis[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( enable_set )
        {
            for ( i = 0; i < qty_mtls; i++ )
                if ( !mtl_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( disable_set )
        {
            for ( i = 0; i < qty_mtls; i++ )
                if ( mtl_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else
        {
            /* Set all if color function selected, else unset all. */
            XtVaGetValues( ctl_buttons[0][COLOR], XmNset, &set, NULL );
            if ( set )
                for ( i = 0; i < qty_mtls; i++ )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_mtl = mtl_deselect_list;
                    UNLINK( p_mtl, mtl_deselect_list );
                    p_mtl->mtl = i + 1;
                    INSERT( p_mtl, mtl_select_list );
                }
            else
                for ( i = 0; i < qty_mtls; i++ )
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
    }

    /*
     * Reset all color component value change flags regardless of
     * current function.  Component changes are remembered only
     * for a constant set of materials.
     */
    prop_val_changed[AMBIENT] = FALSE;
    prop_val_changed[DIFFUSE] = FALSE;
    prop_val_changed[SPECULAR] = FALSE;
    prop_val_changed[EMISSIVE] = FALSE;
    prop_val_changed[SHININESS] = FALSE;

    /* If color function selected, update the rgb scales. */
    if ( XtIsSensitive( color_editor ) )
    {
        update_swatch_label();
        set_scales_to_mtl();
    }

    update_actions_sens();
}


/*****************************************************************
 * TAG( mtl_select_CB )
 *
 * The callback for the material manager material select toggles.
 * A linked list of selected materials is maintained solely to
 * know order of selection.
 */
static void
mtl_select_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    int mtl;
    Material_list_obj *p_mtl;
    XmToggleButtonCallbackStruct *cb_data;

    mtl = (int) client_data;
    cb_data = (XmToggleButtonCallbackStruct *) call_data;

    /*
     * Material list nodes are conserved between the select and deselect
     * lists and the two lists should reflect exactly which toggles are
     * set and unset.  However, very quick clicking on a toggle seems to
     * be able to cause, for example, two deselect requests on the same
     * toggle sequentially, which can cause an apparent inconsistency in
     * the lists.  The tests below for empty lists and unfound materials
     * are an attempt to avoid problems for "quick-clickers".
     */
    if ( cb_data->set )
    {
        if ( mtl_deselect_list != NULL )
        {
            p_mtl = mtl_deselect_list;
            UNLINK( p_mtl, mtl_deselect_list );
            p_mtl->mtl = mtl;
            INSERT( p_mtl, mtl_select_list );
        }
        else
            return;
    }
    else
    {
        for ( p_mtl = mtl_select_list; p_mtl != NULL; p_mtl = p_mtl->next )
            if ( p_mtl->mtl == mtl )
                break;
        if ( p_mtl != NULL )
        {
            UNLINK( p_mtl, mtl_select_list );
            INSERT( p_mtl, mtl_deselect_list );
        }
        else
            return;
    }

    /*
     * Reset all color component value change flags regardless of
     * current function.  Component changes are remembered only
     * for a constant set of materials.
     */
    prop_val_changed[AMBIENT] = FALSE;
    prop_val_changed[DIFFUSE] = FALSE;
    prop_val_changed[SPECULAR] = FALSE;
    prop_val_changed[EMISSIVE] = FALSE;
    prop_val_changed[SHININESS] = FALSE;

    /* If color function selected, update the rgb scales. */
    if ( XtIsSensitive( color_editor ) )
    {
        update_swatch_label();
        set_scales_to_mtl();
    }

    update_actions_sens();
}


/*****************************************************************
 * TAG( col_comp_disarm_CB )
 *
 * The callback for the color component selection toggles.
 */
static void
col_comp_disarm_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Material_property_type comp;
    int i;
    Material_property_type j;
    XmToggleButtonCallbackStruct *cb_data;

    comp = (Material_property_type) client_data;
    cb_data = (XmToggleButtonCallbackStruct *) call_data;

    /* Find which component has been set/unset. */
    for ( j = AMBIENT; j < MTL_PROP_QTY; j++ )
        if ( w == color_comps[j] )
            break;

    if ( cb_data->set )
    {
        cur_mtl_comp = comp;
        for ( i = 0; i < 3; i++ )
            if ( !XtIsSensitive( col_ed_scales[0][i] ) )
                XtSetSensitive( col_ed_scales[0][i], True );
        set_scales_to_mtl();
    }
    else
    {
        cur_mtl_comp = MTL_PROP_QTY;
        set_scales_to_mtl();
        for ( i = 0; i < 3; i++ )
            XtSetSensitive( col_ed_scales[0][i], False );
    }

    update_actions_sens();
}



/* ARGSUSED 0 */
/*****************************************************************
 * TAG( expose_swatch_CB )
 *
 * The callback for the material manager color editor which
 * redraws the swatch on expose.
 */
static void
expose_swatch_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    OpenGL_win save_win;

    save_win = cur_opengl_win;
    switch_opengl_win( SWATCH );
    draw_mtl_swatch( cur_color );
    if ( save_win != SWATCH )
        switch_opengl_win( save_win );
}

/*****************************************************************
 * TAG( init_swatch_CB )
 *
 * The callback for the material manager color editor which
 * initializes the swatch.
 */
static void
init_swatch_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Widget main_win;

    main_win = (Widget) client_data;

    /* Create an OpenGL rendering context. */
    swatch_ctx = glXCreateContext( XtDisplay(main_win), vi, None, GL_TRUE );
    if ( swatch_ctx == NULL )
        popup_fatal( "Could not create swatch rendering context.\n" );

    /* Bind the context to the swatch window. */
    switch_opengl_win( SWATCH );

    init_swatch();

    /*
     * Now that it's ready, get rid of it since the color editor is
     * insensitive to start with.
     */
    XtUnmanageChild( swatch_frame );

    switch_opengl_win( MESH_VIEW );
}


/*****************************************************************
 * TAG( col_ed_scale_CB )
 *
 * The callback for the material manager color editor color and
 * shininess scales which updates the numeric display.
 */
static void
col_ed_scale_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    XmString sval;
    char valbuf[5];
    Color_editor_scale_type scale;
    GLfloat fval;
    XmScaleCallbackStruct *cb_data;

    scale = (Color_editor_scale_type) client_data;
    cb_data = (XmScaleCallbackStruct *) call_data;

    if ( scale != SHININESS_SCALE )
    {
        fval = (GLfloat) cb_data->value / 100.0;
        sprintf( valbuf, "%4.2f", fval );
    }
    else
        sprintf( valbuf, "%d", (int) cb_data->value );

    sval = XmStringCreateLocalized( valbuf );
    XtVaSetValues( col_ed_scales[1][scale], XmNlabelString, sval, NULL );
    XmStringFree( sval );

    if ( scale != SHININESS_SCALE )
    {
        cur_color[scale] = fval;
        draw_mtl_swatch( cur_color );
    }
}


/*****************************************************************
 * TAG( col_ed_scale_update_CB )
 *
 * The callback for the material manager color editor color and
 * shininess scales which stores an entire component for which
 * a scale value has been updated.
 */
static void
col_ed_scale_update_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Color_editor_scale_type scale;
    int i, ival;
    XmScaleCallbackStruct *cb_data;

    scale = (Color_editor_scale_type) client_data;
    cb_data = (XmScaleCallbackStruct *) call_data;

    if ( scale == SHININESS_SCALE )
    {
        prop_val_changed[SHININESS] = TRUE;
        property_vals[SHININESS][0] = (GLfloat) cb_data->value;
    }
    else
    {
        /*
         * Regardless of which of r,g,b changed, save entire
         * triple for the current material component.
         */
        prop_val_changed[cur_mtl_comp] = TRUE;
        for ( i = 0; i < 3; i++ )
        {
            if ( i == scale )
                property_vals[cur_mtl_comp][i] = (GLfloat) cb_data->value
                                                 / 100.0;
            else
            {
                XtVaGetValues( col_ed_scales[0][i], XmNvalue, &ival, NULL );
                property_vals[cur_mtl_comp][i] = (GLfloat) ival / 100.0;
            }
        }
    }

    update_actions_sens();
}


/*****************************************************************
 * TAG( mtl_func_operate_CB )
 *
 * The callback that operates the material manager functions.
 */
static void
mtl_func_operate_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Mtl_mgr_op_type op;
    char *p_src, *p_dest;
    int t_cnt, token_cnt, i;
    size_t len;

    op = (Mtl_mgr_op_type) client_data;
    token_cnt = 0;

    switch_opengl_win( MESH_VIEW );

    /* memset( mtl_mgr_cmd, 0,
            128 + env.curr_analy->mesh_table[0].material_qty * 4 ); */
    memset(mtl_mgr_cmd, 0, sizeof(mtl_mgr_cmd));

    /* Command always starts with "mtl ". */
    for ( p_dest = mtl_mgr_cmd, p_src = "mtl "; *p_dest = *p_src;
            p_src++, p_dest++ );
    token_cnt++;

    switch( op )
    {
    case OP_PREVIEW:
        /* Preview is only a color function option. */
        for ( p_src = "preview mat "; *p_dest = *p_src; p_src++, p_dest++ );
        token_cnt += 2;

        len = load_selected_mtls( p_dest, &t_cnt );
        p_dest += len;
        token_cnt += t_cnt;

        len = load_mtl_properties( p_dest, &t_cnt );
        p_dest += len;
        token_cnt += t_cnt;
        if ( t_cnt == 0 )
            break;

        send_mtl_cmd( mtl_mgr_cmd, token_cnt );

        /* Prevent any material selection changes while in preview. */
        XtSetSensitive( mtl_quick_select, False );
        XtSetSensitive( mtl_row_col, False );

        preview_set = TRUE;
        break;

    case OP_CANCEL:
        for ( p_src = "cancel "; *p_dest = *p_src; p_src++, p_dest++ );
        token_cnt++;

        send_mtl_cmd( mtl_mgr_cmd, token_cnt );

        preview_set = FALSE;

        XtSetSensitive( mtl_quick_select, True );
        XtSetSensitive( mtl_row_col, True );

        /* Reset scales to reflect original color. */
        switch_opengl_win( SWATCH );

        /* Reset change flags. */
        for ( i = 0; i < MTL_PROP_QTY; i++ )
            prop_val_changed[i] = FALSE;

        set_scales_to_mtl();
        break;

    case OP_APPLY:
        if ( preview_set )
        {
            /* Simple command - just make preview permanent. */
            for ( p_src = "apply "; *p_dest = *p_src; p_src++, p_dest++ );
            token_cnt++;
        }
        else
        {
            /* Build whole new command. */
            len = load_mtl_mgr_funcs( p_dest, &t_cnt );
            if ( t_cnt == 0 )
                break;
            p_dest += len;
            token_cnt += t_cnt;

            len = load_selected_mtls( p_dest, &t_cnt );
            p_dest += len;
            token_cnt += t_cnt;

            /* Color function will take material properties. */
            if ( XtIsSensitive( color_editor ) )
            {
                len = load_mtl_properties( p_dest, &t_cnt );
                p_dest += len;
                token_cnt += t_cnt;

                if ( t_cnt == 0 )
                    break;
            }
        }

        send_mtl_cmd( mtl_mgr_cmd, token_cnt );

        for ( i = 0; i < MTL_PROP_QTY; i++ )
            prop_val_changed[i] = FALSE;

        if ( preview_set )
        {
            preview_set = FALSE;
            XtSetSensitive( mtl_quick_select, True );
            XtSetSensitive( mtl_row_col, True );
        }

        break;

    case OP_DEFAULT:
        for ( p_src = "default "; *p_dest = *p_src; p_src++, p_dest++ );
        token_cnt++;

        len = load_selected_mtls( p_dest, &t_cnt );
        p_dest += len;
        token_cnt += t_cnt;

        send_mtl_cmd( mtl_mgr_cmd, token_cnt );

        for ( i = 0; i < MTL_PROP_QTY; i++ )
            prop_val_changed[i] = FALSE;

        switch_opengl_win( SWATCH );
        set_scales_to_mtl();

        if ( preview_set )
        {
            preview_set = FALSE;
            XtSetSensitive( mtl_quick_select, True );
            XtSetSensitive( mtl_row_col, True );
        }
        break;

    default:
        popup_dialog( INFO_POPUP,
                      "Unknown Material Manager operation; ignored" );
    }

    update_actions_sens();

    if ( XtIsSensitive( color_editor ) )
        switch_opengl_win( SWATCH );
}


/* ARGSUSED 0 */
/*****************************************************************
 * TAG( destroy_mtl_mgr_CB )
 *
 * Callback to destroy allocated resources for the material
 * manager.
 */
static void
destroy_mtl_mgr_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Material_property_type i;
    /*return;*/
    switch_opengl_win( MESH_VIEW );

    if (!mtl_mgr_widg)
        return;

    if ( preview_set )
        send_mtl_cmd( "mtl cancel", 2 );

    for ( i = AMBIENT; i < MTL_PROP_QTY; i++ )
    {
        if (property_vals[i])
            free( property_vals[i] );
        property_vals[i] = NULL;
    }

    if (mtl_mgr_func_toggles)
        free( mtl_mgr_func_toggles );
    mtl_mgr_func_toggles = NULL;

    if ( op_buttons)
        free( op_buttons );
    op_buttons = NULL;

    if (mtl_select_list)
        DELETE_LIST( mtl_select_list );
    if (mtl_deselect_list)
        DELETE_LIST( mtl_deselect_list );
    mtl_select_list = NULL;
    mtl_deselect_list = NULL;

    if ( mtl_mgr_cmd)
        free( mtl_mgr_cmd );
    mtl_mgr_cmd = NULL;

    if ( mtl_check)
        XFreePixmap( dpy, mtl_check );
    mtl_check = NULL;

    mtl_mgr_top_win = 0;
    XtDestroyWidget( mtl_mgr_widg );
    mtl_mgr_widg = NULL;

    session->win_mtl_active = 0;

    destroy_mtl_mgr();

}

/*****************************************************************
 * TAG( surf_func_select_CB )
 *
 * The callback that manages surface  manager function selection.
 * It implements the natural logical exclusivity between
 * visibility/invisibility, and enable/disable.
 */
static void
surf_func_select_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Widget *function_toggles;
    Boolean set, comp_set, color_set;
    int i;
    Surf_mgr_func_type compliment;
    XmToggleButtonCallbackStruct *cb_data;

    function_toggles = (Widget *) client_data;
    cb_data = (XmToggleButtonCallbackStruct *) call_data;

    /* Find the index of the complimentary function to the widget executed. */
    for ( i = 0; i < 4; i++ )
        if ( w == function_toggles[i] )
            break;
    switch ( i )
    {
    case VIS:
        compliment = INVIS;
        break;
    case INVIS:
        compliment = VIS;
        break;
    case ENABLE:
        compliment = DISABLE;
        break;
    case DISABLE:
        compliment = ENABLE;
        break;
    }
    XtVaSetValues( function_toggles[compliment], XmNset, False, NULL );

    update_surf_actions_sens();
}


/*****************************************************************
 * TAG( surf_func_operate_CB )
 *
 * The callback that operates the material manager functions.
 */
static void
surf_func_operate_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Surf_mgr_op_type op;
    char *p_src, *p_dest;
    int t_cnt, token_cnt, i;
    size_t len;

    op = (Surf_mgr_op_type) client_data;
    token_cnt = 0;

    switch_opengl_win( MESH_VIEW );

    memset( surf_mgr_cmd, 0,
            128 + env.curr_analy->mesh_table[0].surface_qty * 4 );

    /* Command always starts with "surf ". */
    for ( p_dest = surf_mgr_cmd, p_src = "surf "; *p_dest = *p_src;
            p_src++, p_dest++ );
    token_cnt++;

    switch( op )
    {
    case SURF_OP_APPLY:
        /* Build whole new command. */
        len = load_surf_mgr_funcs( p_dest, &t_cnt );
        if ( t_cnt == 0 )
            break;
        p_dest += len;
        token_cnt += t_cnt;

        len = load_selected_surfs( p_dest, &t_cnt );
        p_dest += len;
        token_cnt += t_cnt;

        send_surf_cmd( surf_mgr_cmd, token_cnt );

        break;

    case SURF_OP_DEFAULT:
        for ( p_src = "default "; *p_dest = *p_src; p_src++, p_dest++ );
        token_cnt++;

        len = load_selected_surfs( p_dest, &t_cnt );
        p_dest += len;
        token_cnt += t_cnt;

        send_surf_cmd( surf_mgr_cmd, token_cnt );

        break;

    default:
        popup_dialog( INFO_POPUP,
                      "Unknown Surface Manager operation; ignored" );
    }

    update_surf_actions_sens();

}


/*****************************************************************
 * TAG( surf_quick_select_CB )
 *
 * The callback for the surface manager quick select buttons
 * which allow certain selections among the entire set of
 * surfaces in one step.
 */
static void
surf_quick_select_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    Widget **ctl_buttons;
    Boolean set, vis_set, invis_set, enable_set, disable_set;
    unsigned char *surf_invis, *surf_disable;
    int qty_surf, i;
    WidgetList children;
    Surface_list_obj *p_surf;
    Mesh_data *p_mesh;

    p_mesh = env.curr_analy->mesh_table;
    qty_surf = p_mesh->surface_qty;
    surf_invis = p_mesh->hide_surface;
    surf_disable = p_mesh->disable_surface;
    ctl_buttons = (Widget **) client_data;
    XtVaGetValues( surf_row_col, XmNchildren, &children, NULL );

    if ( w == ctl_buttons[1][ALL_SURF] )
    {
        /* Select all surfaces. */

        if ( surf_deselect_list != NULL )
        {
            APPEND( surf_deselect_list, surf_select_list );
            surf_deselect_list = NULL;
        }
        for ( i = qty_surf - 1, p_surf = surf_select_list;
                i >= 0;
                p_surf = p_surf->next, i-- )
        {
            XtVaSetValues( children[i], XmNset, True, NULL );
            p_surf->surf = i + 1;
        }
    }
    else if ( w == ctl_buttons[1][NONE] )
    {
        /* Deselect all surfaces. */

        if ( surf_select_list != NULL )
        {
            APPEND( surf_select_list, surf_deselect_list );
            surf_select_list = NULL;
        }
        for ( i = 0; i < qty_surf; i++ )
            XtVaSetValues( children[i], XmNset, False, NULL );
    }
    else if ( w == ctl_buttons[1][INVERT] )
    {
        /* Invert all selections. */

        /* Swap the lists. */
        p_surf = surf_deselect_list;
        surf_deselect_list = surf_select_list;
        surf_select_list = p_surf;

        /* Toggle the (now) selected toggles on, deselected off. */
        p_surf = surf_select_list;
        for ( i = qty_surf - 1; i >= 0; i-- )
        {
            XtVaGetValues( children[i], XmNset, &set, NULL );
            if ( set )
                XtVaSetValues( children[i], XmNset, False, NULL );
            else
            {
                XtVaSetValues( children[i], XmNset, True, NULL );
                if ( p_surf != NULL )
                {
                    p_surf->surf = i + 1;
                    p_surf = p_surf->next;
                }
                else
                    popup_dialog( WARNING_POPUP,
                                  "Surface selection list does not match set state." );
            }
        }
    }
    else if ( w == ctl_buttons[1][BY_FUNC_SURF] )
    {
        /*
         * Select surfaces which are the intersection of the
         * selected functions.
         */

        /* First empty the material select list. */
        if ( surf_select_list != NULL )
        {
            APPEND( surf_select_list, surf_deselect_list );
            surf_select_list = NULL;
        }

        XtVaGetValues( ctl_buttons[0][VIS], XmNset, &vis_set, NULL );
        XtVaGetValues( ctl_buttons[0][INVIS], XmNset, &invis_set, NULL );
        XtVaGetValues( ctl_buttons[0][ENABLE], XmNset, &enable_set, NULL );
        XtVaGetValues( ctl_buttons[0][DISABLE], XmNset, &disable_set, NULL );

        if ( vis_set && enable_set )
        {
            for ( i = 0; i < qty_surf; i++ )
                if ( !surf_invis[i] && !surf_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_surf = surf_deselect_list;
                    UNLINK( p_surf, surf_deselect_list );
                    p_surf->surf = i + 1;
                    INSERT( p_surf, surf_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( vis_set && disable_set )
        {
            for ( i = 0; i < qty_surf; i++ )
                if ( !surf_invis[i] && surf_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_surf = surf_deselect_list;
                    UNLINK( p_surf, surf_deselect_list );
                    p_surf->surf = i + 1;
                    INSERT( p_surf, surf_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( invis_set && enable_set )
        {
            for ( i = 0; i < qty_surf; i++ )
                if ( surf_invis[i] && !surf_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_surf = surf_deselect_list;
                    UNLINK( p_surf, surf_deselect_list );
                    p_surf->surf = i + 1;
                    INSERT( p_surf, surf_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( invis_set && disable_set )
        {
            for ( i = 0; i < qty_surf; i++ )
                if ( surf_invis[i] && surf_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_surf = surf_deselect_list;
                    UNLINK( p_surf, surf_deselect_list );
                    p_surf->surf = i + 1;
                    INSERT( p_surf, surf_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( vis_set )
        {
            for ( i = 0; i < qty_surf; i++ )
                if ( !surf_invis[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_surf = surf_deselect_list;
                    UNLINK( p_surf, surf_deselect_list );
                    p_surf->surf = i + 1;
                    INSERT( p_surf, surf_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( invis_set )
        {
            for ( i = 0; i < qty_surf; i++ )
                if ( surf_invis[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_surf = surf_deselect_list;
                    UNLINK( p_surf, surf_deselect_list );
                    p_surf->surf = i + 1;
                    INSERT( p_surf, surf_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( enable_set )
        {
            for ( i = 0; i < qty_surf; i++ )
                if ( !surf_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_surf = surf_deselect_list;
                    UNLINK( p_surf, surf_deselect_list );
                    p_surf->surf = i + 1;
                    INSERT( p_surf, surf_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
        else if ( disable_set )
        {
            for ( i = 0; i < qty_surf; i++ )
                if ( surf_disable[i] )
                {
                    XtVaSetValues( children[i], XmNset, True, NULL );
                    p_surf = surf_deselect_list;
                    UNLINK( p_surf, surf_deselect_list );
                    p_surf->surf = i + 1;
                    INSERT( p_surf, surf_select_list );
                }
                else
                    XtVaSetValues( children[i], XmNset, False, NULL );
        }
    }

    update_surf_actions_sens();
}


/*****************************************************************
 * TAG( surf_select_CB )
 *
 * The callback for the surface manager select toggles.
 * A linked list of selected surfaces is maintained solely to
 * know order of selection.
 */
static void
surf_select_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    int surf;
    Surface_list_obj *p_surf;
    XmToggleButtonCallbackStruct *cb_data;

    surf = (int) client_data;
    cb_data = (XmToggleButtonCallbackStruct *) call_data;

    /*
     * Surface list nodes are conserved between the select and deselect
     * lists and the two lists should reflect exactly which toggles are
     * set and unset.  However, very quick clicking on a toggle seems to
     * be able to cause, for example, two deselect requests on the same
     * toggle sequentially, which can cause an apparent inconsistency in
     * the lists.  The tests below for empty lists and unfound surfaces
     * are an attempt to avoid problems for "quick-clickers".
     */
    if ( cb_data->set )
    {
        if ( surf_deselect_list != NULL )
        {
            p_surf = surf_deselect_list;
            UNLINK( p_surf, surf_deselect_list );
            p_surf->surf = surf;
            INSERT( p_surf, surf_select_list );
        }
        else
            return;
    }
    else
    {
        for ( p_surf = surf_select_list; p_surf != NULL; p_surf = p_surf->next )
            if ( p_surf->surf == surf )
                break;
        if ( p_surf != NULL )
        {
            UNLINK( p_surf, surf_select_list );
            INSERT( p_surf, surf_deselect_list );
        }
        else
            return;
    }

    update_surf_actions_sens();
}

/* ARGSUSED 2 */
/*****************************************************************
 * TAG( destroy_surf_mgr_CB )
 *
 * Callback to destroy allocated resources for the surface
 * manager.
 */
static void
destroy_surf_mgr_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    switch_opengl_win( MESH_VIEW );

    DELETE_LIST( surf_select_list );
    DELETE_LIST( surf_deselect_list );
    surf_select_list = NULL;
    surf_deselect_list = NULL;

    free( surf_mgr_cmd );
    surf_mgr_cmd = NULL;

    surf_mgr_top_win = 0;

    session->win_surf_active = 0;

    destroy_surf_mgr();
}

/* ARGSUSED 0 */
/*****************************************************************
 * TAG( destroy_util_panel_CB )
 *
 * Callback to destroy allocated resources for the utility
 * panel.
 */
static void
destroy_util_panel_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    free( util_render_btns );
    util_render_btns = NULL;

    XFreePixmap( dpy, pixmap_start );
    pixmap_start = NULL;
    XFreePixmap( dpy, pixmap_stop );
    pixmap_stop = NULL;
    XFreePixmap( dpy, pixmap_leftstop );
    pixmap_leftstop = NULL;
    XFreePixmap( dpy, pixmap_left );
    pixmap_left = NULL;
    XFreePixmap( dpy, pixmap_right );
    pixmap_right = NULL;
    XFreePixmap( dpy, pixmap_rightstop );
    pixmap_rightstop = NULL;

    util_panel_top_win = 0;
    XtDestroyWidget( util_panel_widg );

    session->win_util_active = 0;

    util_panel_widg = NULL;
}


/*****************************************************************
 * TAG( util_render_CB )
 *
 * Callback for the utility rendering function toggles.
 */
static void
util_render_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    XmString text[5];
    Widget hist_list;
    Util_panel_button_type btn;
    XmString label;
    char cbuf[M_MAX_NAME_LEN], cmdbuf[M_MAX_NAME_LEN];
    unsigned char *hide_mtl;
    int qty_mtls, btn_num;
    int i, item_cnt;
    XmToggleButtonCallbackStruct *cb_data;
    MO_class_data *p_mo_class;

    cb_data = (XmToggleButtonCallbackStruct *) call_data;

    hide_mtl = env.curr_analy->mesh_table[0].hide_material;
    qty_mtls = env.curr_analy->mesh_table[0].material_qty;

    btn = (Util_panel_button_type) client_data;

    item_cnt = 0;
    switch( btn )
    {
    case VIEW_SOLID:
        if ( cb_data->set )
        {
            text[item_cnt] = XmStringCreateSimple( "sw solid" );
            item_cnt++;
            parse_command( "sw solid", env.curr_analy );
            if ( all_true_uc( hide_mtl, qty_mtls ) )
            {
                text[item_cnt] = XmStringCreateSimple( "mtl vis all" );
                item_cnt++;
                parse_command( "mtl vis all", env.curr_analy );
            }
        }
        else
        {
            /* Explicitly unsetting filled view. Do something... */
            text[item_cnt] = XmStringCreateSimple( "sw none" );
            item_cnt++;
            parse_command( "sw none", env.curr_analy );
        }
        break;
    case VIEW_SOLID_MESH:
        if ( cb_data->set )
        {
            text[item_cnt] = XmStringCreateSimple( "sw hidden" );
            item_cnt++;
            parse_command( "sw hidden", env.curr_analy );
            if ( all_true_uc( hide_mtl, qty_mtls ) )
            {
                text[item_cnt] = XmStringCreateSimple( "mtl vis all" );
                item_cnt++;
                parse_command( "mtl vis all", env.curr_analy );
            }
        }
        else
        {
            /* Explicitly unsetting mesh view. Do something... */
            text[item_cnt] = XmStringCreateSimple( "sw none" );
            item_cnt++;
            parse_command( "sw none", env.curr_analy );
        }
        break;
    case VIEW_EDGES:
        if ( cb_data->set )
        {
            util_panel_CB_active = TRUE;
            text[item_cnt] = XmStringCreateSimple( "sw none" );
            item_cnt++;
            parse_command( "sw none", env.curr_analy );
            util_panel_CB_active = FALSE;
            text[item_cnt] = XmStringCreateSimple( "on edges" );
            item_cnt++;
            parse_command( "on edges", env.curr_analy );
        }
        else
        {
            text[item_cnt] = XmStringCreateSimple( "off edges" );
            item_cnt++;
            parse_command( "off edges", env.curr_analy );
        }
        break;
    case VIEW_WIREFRAME:
        if ( !env.curr_analy->draw_wireframe )
        {
            text[item_cnt] = XmStringCreateSimple( "sw wf" );
            item_cnt++;
            parse_command( "sw wf", env.curr_analy );
            if ( all_true_uc( hide_mtl, qty_mtls ) )
            {
                text[item_cnt] = XmStringCreateSimple( "mtl vis all" );
                item_cnt++;
                parse_command( "mtl vis all", env.curr_analy );
            }
        }
        else
        {
            /* Explicitly unsetting filled view. Do something... */
            text[item_cnt] = XmStringCreateSimple( "sw solid" );
            item_cnt++;
            parse_command( "sw solid", env.curr_analy );
        }
        break;
    case VIEW_GS:
        if ( env.curr_analy->material_greyscale )
        {
            text[item_cnt] = XmStringCreateSimple( "off gs" );
            item_cnt++;
            parse_command( "off gs", env.curr_analy );
        }
        else
        {
            /* Explicitly unsetting greyscale view. Do something... */
            text[item_cnt] = XmStringCreateSimple( "off gs" );
            text[item_cnt] = XmStringCreateSimple( "on gs" );
            item_cnt++;
            parse_command( "on gs", env.curr_analy );
        }
        break;
    case PICK_MODE_SELECT:
        if ( cb_data->set )
        {
            text[item_cnt] = XmStringCreateSimple( "sw picsel" );
            item_cnt++;
            parse_command( "sw picsel", env.curr_analy );
        }
        break;
    case PICK_MODE_HILITE:
        if ( cb_data->set )
        {
            text[item_cnt] = XmStringCreateSimple( "sw pichil" );
            item_cnt++;
            parse_command( "sw pichil", env.curr_analy );
        }
        break;
        /*
                case BTN2_BEAM:
                    if ( cb_data->set )
                    {
                        text[item_cnt] = XmStringCreateSimple( "sw picbm" );
                        item_cnt++;
                        parse_command( "sw picbm", env.curr_analy );
                    }
                    break;
                case BTN2_SHELL:
                    if ( cb_data->set )
                    {
                        text[item_cnt] = XmStringCreateSimple( "sw picsh" );
                        item_cnt++;
                        parse_command( "sw picsh", env.curr_analy );
                    }
                    break;
        */
    case BTN1_PICK:
    case BTN2_PICK:
    case BTN3_PICK:
        XtVaGetValues( w, XmNlabelString, &label, NULL );
        string_convert( label, cbuf );
        p_mo_class = find_class_by_long_name( MESH_P( env.curr_analy ),
                                              cbuf );
        if ( p_mo_class != NULL )
        {
            btn_num = ( btn == BTN1_PICK ) ? 1 :
                      (( btn == BTN2_PICK ) ? 2 : 3);
            sprintf( cmdbuf, "setpick %d %s", btn_num,
                     p_mo_class->short_name );
            util_panel_CB_active = TRUE;
            text[item_cnt] = XmStringCreateSimple( cmdbuf );
            item_cnt++;
            parse_command( cmdbuf, env.curr_analy );
            util_panel_CB_active = FALSE;
        }
        else
            popup_dialog( WARNING_POPUP,
                          "Unable to find designated pick class." );
        break;
    }
    if( item_cnt > 0 )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItems( hist_list, text, item_cnt, 0 );
        XmListSetBottomPos( hist_list, 0 );
        for( i = 0; i < item_cnt; i++ )
            XmStringFree( text[i] );
    }
}


/*****************************************************************
 * TAG( step_CB )
 *
 * Callback for the utility panel state step buttons.
 */
static void
step_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    XmString text= NULL;
    Widget hist_list;
    Util_panel_button_type btn;
    Analysis *analy;
    int st_num;
    char cmd[16];

    btn = (Util_panel_button_type) client_data;
    analy = env.curr_analy;

    switch( btn )
    {
    case STEP_FIRST:
        text = XmStringCreateSimple( "f\0" );
        parse_command( "f", analy );
        break;
    case STEP_DOWN:
        st_num = analy->cur_state + 1 - step_stride;
        sprintf( cmd, "state %d", st_num );
        text = XmStringCreateSimple( cmd );
        parse_command( cmd, analy );
        break;
    case STEP_UP:
        st_num = analy->cur_state + 1 + step_stride;
        sprintf( cmd, "state %d", st_num );
        text = XmStringCreateSimple( cmd );
        parse_command( cmd, analy );
        break;
    case STEP_LAST:
        text = XmStringCreateSimple( "l\0" );
        parse_command( "l", analy );
        break;
    }

    if( text != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }
}


/*****************************************************************
 * TAG( stride_CB )
 *
 * Callback for the utility panel step stride display and controls.
 */
static void
stride_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    XmString htext= NULL;
    Widget hist_list;
    char stride_str[8];
    Util_panel_button_type stride_mod;
    int sval;
    char *text;
    int max_state;

    stride_mod = (Util_panel_button_type) client_data;

    /**/
    /* May prove too slow.  Could keep analy->num_states field for use in
     * cases like this, and have it updated by other activities that call
     * db_query() to get state quantity.
     */
    max_state = get_max_state( env.curr_analy );

    switch ( stride_mod )
    {
    case STRIDE_INCREMENT:
        if ( step_stride < max_state )
        {
            step_stride++;
            sprintf( stride_str, "%d", step_stride );
            XmTextSetString( stride_label, stride_str );
        }
        break;
    case STRIDE_DECREMENT:
        if ( step_stride > 1 )
        {
            step_stride--;
            sprintf( stride_str, "%d", step_stride );
            XmTextSetString( stride_label, stride_str );
        }
        break;
    case STRIDE_EDIT:
        text = XmTextGetString( stride_label );
        sval = atoi( text );
        if ( sval > max_state || sval < 1 )
        {
            sprintf( stride_str, "%d", step_stride );
            XmTextSetString( stride_label, stride_str );
        }
        else
            step_stride = sval;
    }
    if( htext != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, htext, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( htext );
    }
}


/*****************************************************************
 * TAG( menu_setpick_CB )
 *
 * Callback for setting pick class from the menu.
 */
static void
menu_setpick_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    XmString text= NULL;
    Widget hist_list;
    Util_panel_button_type btn;
    XmString label;
    char cbuf[M_MAX_NAME_LEN], cmdbuf[M_MAX_NAME_LEN];
    int btn_num;
    MO_class_data *p_mo_class;

    btn = (Util_panel_button_type) client_data;

    XtVaGetValues( w, XmNlabelString, &label, NULL );
    string_convert( label, cbuf );
    p_mo_class = find_class_by_long_name( MESH_P( env.curr_analy ),
                                          cbuf );
    if ( p_mo_class != NULL )
    {
        btn_num = ( btn == BTN1_PICK ) ? 1 :
                  (( btn == BTN2_PICK ) ? 2 : 3);
        sprintf( cmdbuf, "setpick %d %s", btn_num,
                 p_mo_class->short_name );
        text = XmStringCreateSimple( cmdbuf );
        parse_command( cmdbuf, env.curr_analy );
    }
    else
        popup_dialog( WARNING_POPUP,
                      "Unable to find designated pick class." );
    if( text != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }
}

/*****************************************************************
 * TAG( menu_setcolormap )
 *
 * Callback for setting the colormap from the menu.
 */
static void
menu_setcolormap_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
    XmString text=NULL;
    Widget hist_list;
    colormap_type btn;
    int btn_num;

    Analysis *analy;
    static char cmd[256];

    analy = env.curr_analy;

    btn = (Util_panel_button_type) client_data;

    switch ( btn )
    {
    case CM_INVERSE:
        parse_command( "invmap" ,analy );
        break;
    case CM_DEFAULT:
        parse_command( "hotmap", analy );
        strcpy( cmd, "hotmap" );
        break;
    case CM_COOL:
        load_colormap( analy, "cool.cm" );
        break;
    case CM_GRAYSCALE:
        parse_command( "grmap", analy );
        strcpy( cmd, "grmap" );
        break;
    case CM_INVERSE_GRAYSCALE:
        parse_command( "igrmap" ,analy );
        strcpy( cmd, "igrmap" );
        break;
    case CM_HSV:
        load_colormap( analy, "hsv.cm" );
        break;
    case CM_JET:
        load_colormap( analy, "jet.cm" );
        break;
    case CM_PRISM:
        load_colormap( analy, "prism.cm" );
        break;
    case CM_WINTER:
        load_colormap( analy, "winter.cm" );
        break;
    }

    if ( strlen(cmd) > 0 )
    {
        text = XmStringCreateSimple( cmd );
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }
}

/*****************************************************************
 * TAG( load_colormap )
 *
 *
 */
static void load_colormap( Analysis *analy, char *colormap )
{
    XmString text= NULL;
    char *griz_home, cm_filename[256], cm_cmd[256], file_spec[256];
    struct stat statbuf;
    Widget hist_list;
    Bool_type griz_home_set=FALSE;

    griz_home = getenv( "GRIZHOME" );
    if ( griz_home==NULL )
    {
        griz_home = strdup( "." );
        griz_home_set=TRUE;
    }

    if ( griz_home != NULL )
    {
        sprintf( cm_cmd, "ldmap %s/ColorMaps/%s", griz_home, colormap );

        if ( stat( cm_cmd, &statbuf ) != 0 )
        {
            parse_command( cm_cmd ,analy );
            text = XmStringCreateSimple( cm_cmd );
            hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
            XmListAddItem( hist_list, text, 0 );
            XmListSetBottomPos( hist_list, 0 );
            XmStringFree( text );
        }
        else
            popup_dialog( WARNING_POPUP,
                          "Unable to find designated colormap [%s].", file_spec );

        if ( griz_home_set )
            free( griz_home );
    }
}

/*****************************************************************
 * TAG( set_pick_class )
 *
 * Associates a mesh object class with a particular mouse button
 * for pick operations.
 */
void
set_pick_class( MO_class_data *p_mo_class, Util_panel_button_type btn_type )
{
    switch( btn_type )
    {
    case BTN1_PICK:
        btn1_mo_class = p_mo_class;
        break;
    case BTN2_PICK:
        btn2_mo_class = p_mo_class;
        break;
    case BTN3_PICK:
        btn3_mo_class = p_mo_class;
        break;
    }
}


/*****************************************************************
 * TAG( create_app_widg )
 *
 * Routine to create additional standalone interface widgets.
 */
static void
create_app_widg( Btn_type btn )
{
    Widget *app_widg;
    Widget (*create_func)();

    /* Assign parameters for creation. */
    if ( btn == BTN_MTL_MGR )
    {
        app_widg = &mtl_mgr_widg;
        create_func = create_mtl_manager;
    }
    else if ( btn == BTN_SURF_MGR )
    {
        app_widg = &surf_mgr_widg;
        create_func = create_surf_manager;
    }
    else if ( btn == BTN_UTIL_PANEL )
    {
        app_widg = &util_panel_widg;
        create_func = create_free_util_panel;
    }
    else
        return;

    /*
     * Create the application widget if it doesn't exist, else move
     * it to the top of the stack.
     */
    if ( *app_widg == NULL )
    {
        *app_widg = create_func( ctl_shell_widg );

        /* Add the creation translations to the app widget. */
        /*
                XtVaGetValues( *app_widg, XmNchildren, &children, NULL );
                XtOverrideTranslations( children[0],
                                        XtParseTranslationTable( action_spec ) );
        */
    }
    else
    {
        XWindowChanges xwc;
        xwc.stack_mode = Above;
        XConfigureWindow( dpy, XtWindow( *app_widg ), CWStackMode, &xwc );
    }
}


/*****************************************************************
 * TAG( update_util_panel )
 *
 * Routine to update the utility panel after a pertinent pulldown menu
 * or command line action.  Implements radio-button behavior for the
 * appropriate buttons on the utility panel.
 */
void
update_util_panel( Util_panel_button_type button, MO_class_data *p_mo_class )
{
    Boolean set;
    Bool_type ok;
    Widget pulldown, new_class;
    int position;
    char stride_str[8];

    /*
     * If utility panel not up or this called as a result of
     * button activity on utility panel, nothing to do.
     */
    if ( util_panel_widg == NULL || util_panel_CB_active )
        return;

    /* Don't try to set a button for modes that aren't implemented as such. */
    if ( button != VIEW_POINT_CLOUD && button != VIEW_NONE
            && button != STRIDE_EDIT )
    {
        XtVaGetValues( util_render_btns[button], XmNset, &set, NULL );
        if ( !set && button != VIEW_EDGES )
            XtVaSetValues( util_render_btns[button], XmNset, True, NULL );
    }

    switch( button )
    {
    case STRIDE_EDIT:
        sprintf( stride_str, "%d", step_stride );
        XmTextSetString( stride_label, stride_str );
        break;
    case VIEW_SOLID:
        XtVaGetValues( util_render_btns[VIEW_SOLID_MESH], XmNset, &set,
                       NULL );
        if ( set )
            XtVaSetValues( util_render_btns[VIEW_SOLID_MESH], XmNset, False,
                           NULL );
        else
        {
            XtVaGetValues( util_render_btns[VIEW_EDGES], XmNset, &set,
                           NULL );
            if ( set )
            {
                /*XtVaSetValues( util_render_btns[VIEW_EDGES], XmNset, False,
                               NULL );*/
                parse_command( "off edges", env.curr_analy );
            }
        }
        break;
    case VIEW_WIREFRAME:
        if ( env.curr_analy->draw_wireframe )
            XtVaSetValues( util_render_btns[VIEW_WIREFRAME], XmNset, True,
                           NULL );
        else
            XtVaSetValues( util_render_btns[VIEW_WIREFRAME], XmNset, False,
                           NULL );
        break;
    case VIEW_GS:
        if ( env.curr_analy->material_greyscale )
            XtVaSetValues( util_render_btns[VIEW_GS], XmNset, True,
                           NULL );
        else
            XtVaSetValues( util_render_btns[VIEW_GS], XmNset, False,
                           NULL );
        break;
    case VIEW_SOLID_MESH:
        XtVaGetValues( util_render_btns[VIEW_SOLID], XmNset, &set, NULL );
        if ( set )
            XtVaSetValues( util_render_btns[VIEW_SOLID], XmNset, False,
                           NULL );
        else
        {
            XtVaGetValues( util_render_btns[VIEW_EDGES], XmNset, &set,
                           NULL );
            if ( set )
            {
                /*XtVaSetValues( util_render_btns[VIEW_EDGES], XmNset, False,
                               NULL );*/
                parse_command( "off edges", env.curr_analy );
            }
        }
        break;
    case VIEW_EDGES:
        /*
         * This called when edge on/off is modified
         * to maintain consistency of "Edges Only" toggle with
         * the current state.
         */
        XtVaGetValues( util_render_btns[VIEW_EDGES], XmNset, &set, NULL );

        if ( env.curr_analy->mesh_view_mode == RENDER_NONE
                && env.curr_analy->show_edges )
        {
            if ( !set )
                XtVaSetValues( util_render_btns[VIEW_EDGES],
                               XmNset, True,
                               NULL );

            /* Unset "Solid" or "Solid Mesh" buttons if either set. */
            XtVaGetValues( util_render_btns[VIEW_SOLID], XmNset, &set, NULL );
            if ( set )
                XtVaSetValues( util_render_btns[VIEW_SOLID], XmNset, False,
                               NULL );
            else
            {
                XtVaGetValues( util_render_btns[VIEW_SOLID_MESH],
                               XmNset, &set,
                               NULL );
                if ( set )
                    XtVaSetValues( util_render_btns[VIEW_SOLID_MESH],
                                   XmNset, False,
                                   NULL );
            }
        }
        else
        {
            if ( set )
                XtVaSetValues( util_render_btns[VIEW_EDGES],
                               XmNset, False,
                               NULL );
        }
        break;
    case VIEW_POINT_CLOUD:
        XtVaGetValues( util_render_btns[VIEW_SOLID], XmNset, &set, NULL );
        if ( set )
            XtVaSetValues( util_render_btns[VIEW_SOLID], XmNset, False,
                           NULL );

        XtVaGetValues( util_render_btns[VIEW_SOLID_MESH], XmNset, &set,
                       NULL );
        if ( set )
            XtVaSetValues( util_render_btns[VIEW_SOLID_MESH], XmNset, False,
                           NULL );

        XtVaGetValues( util_render_btns[VIEW_EDGES], XmNset, &set, NULL );
        if ( set )
            XtVaSetValues( util_render_btns[VIEW_EDGES], XmNset, False,
                           NULL );
        break;
    case VIEW_NONE:
        XtVaGetValues( util_render_btns[VIEW_SOLID], XmNset, &set, NULL );
        if ( set )
            XtVaSetValues( util_render_btns[VIEW_SOLID], XmNset, False,
                           NULL );

        XtVaGetValues( util_render_btns[VIEW_SOLID_MESH], XmNset, &set,
                       NULL );
        if ( set )
            XtVaSetValues( util_render_btns[VIEW_SOLID_MESH], XmNset, False,
                           NULL );

        XtVaGetValues( util_render_btns[VIEW_EDGES], XmNset, &set, NULL );
        if ( set )
            XtVaSetValues( util_render_btns[VIEW_EDGES], XmNset, False,
                           NULL );

        /* Check for possible "Edges only" state. */
        update_util_panel( VIEW_EDGES, p_mo_class );
        break;
    case PICK_MODE_SELECT:
        XtVaGetValues( util_render_btns[PICK_MODE_HILITE], XmNset, &set,
                       NULL );
        if ( set )
            XtVaSetValues( util_render_btns[PICK_MODE_HILITE],
                           XmNset, False,
                           NULL );
        break;
    case PICK_MODE_HILITE:
        XtVaGetValues( util_render_btns[PICK_MODE_SELECT], XmNset, &set,
                       NULL );
        if ( set )
            XtVaSetValues( util_render_btns[PICK_MODE_SELECT],
                           XmNset, False,
                           NULL );
        break;
    case BTN1_PICK:
        pulldown = NULL;
        XtVaGetValues( util_render_btns[BTN1_PICK], XmNsubMenuId, &pulldown,
                       NULL );
        ok = find_labelled_child( pulldown, p_mo_class->long_name,
                                  &new_class, &position );
        if ( ok )
            XtVaSetValues( util_render_btns[BTN1_PICK], XmNmenuHistory,
                           new_class, NULL );
        else
            popup_dialog( WARNING_POPUP,
                          "Unable to update utility panel pick class." );
        break;
    case BTN2_PICK:
        pulldown = NULL;
        XtVaGetValues( util_render_btns[BTN2_PICK], XmNsubMenuId, &pulldown,
                       NULL );
        ok = find_labelled_child( pulldown, p_mo_class->long_name,
                                  &new_class, &position );
        if ( ok )
            XtVaSetValues( util_render_btns[BTN2_PICK], XmNmenuHistory,
                           new_class, NULL );
        else
            popup_dialog( WARNING_POPUP,
                          "Unable to update utility panel pick class." );
        break;
    case BTN3_PICK:
        pulldown = NULL;
        XtVaGetValues( util_render_btns[BTN3_PICK], XmNsubMenuId, &pulldown,
                       NULL );
        ok = find_labelled_child( pulldown, p_mo_class->long_name,
                                  &new_class, &position );
        if ( ok )
            XtVaSetValues( util_render_btns[BTN3_PICK], XmNmenuHistory,
                           new_class, NULL );
        else
            popup_dialog( WARNING_POPUP,
                          "Unable to update utility panel pick class." );
        break;
    }
}


/*****************************************************************
 * TAG( select_mtl_mgr_mtl )
 *
 * "Software" select of a Material Manager material.
 */
static void
select_mtl_mgr_mtl( int mtl )
{
    Boolean set;
    WidgetList children;
    Material_list_obj *p_mtl;

    /* Sanity check. */
    if ( mtl_mgr_widg == NULL )
    {
        popup_dialog( INFO_POPUP, "No Material Manager..." );
        return;
    }

    /* Get the material toggles. */
    XtVaGetValues( mtl_row_col, XmNchildren, &children, NULL );

    /* Get the current state of the specified material. */
    XtVaGetValues( children[mtl], XmNset, &set, NULL );
    if ( set )
    {
        /* Toggle from selected to deselected. */
        XtVaSetValues( children[mtl], XmNset, False, NULL );

        /* Find material's entry in the appropriate list. */
        for ( p_mtl = mtl_select_list; p_mtl != NULL; NEXT( p_mtl ) )
            if ( p_mtl->mtl == mtl + 1 )
                break;

        if ( p_mtl == NULL )
        {
            popup_dialog( WARNING_POPUP,
                          "Material toggle selection state inconsistent\n%s",
                          "with selection list." );
            return;
        }

        /* Remove from one list and put on other. */
        UNLINK( p_mtl, mtl_select_list );
        INSERT( p_mtl, mtl_deselect_list );
    }
    else
    {
        /* Toggle from deselected to selected. */
        XtVaSetValues( children[mtl], XmNset, True, NULL );

        /*
         * Pop a node off the deselect list, init it, and insert in the select
         * list.  Only the material numbers of nodes in the select list
         * matter (although the quantity of nodes in both lists must be
         * correct).
         */
        p_mtl = mtl_deselect_list;
        UNLINK( p_mtl, mtl_deselect_list );
        p_mtl->mtl = mtl + 1;
        INSERT( p_mtl, mtl_select_list );
    }

    /*
     * Reset all color component value change flags regardless of
     * current function.  Component changes are remembered only
     * for a constant set of materials.
     */
    prop_val_changed[AMBIENT] = FALSE;
    prop_val_changed[DIFFUSE] = FALSE;
    prop_val_changed[SPECULAR] = FALSE;
    prop_val_changed[EMISSIVE] = FALSE;
    prop_val_changed[SHININESS] = FALSE;

    /* If color function selected, update the rgb scales. */
    if ( XtIsSensitive( color_editor ) )
    {
        OpenGL_win save_win;

        /* Switch rendering context to allow swatch update. */
        save_win = cur_opengl_win;
        switch_opengl_win( SWATCH );

        update_swatch_label();
        set_scales_to_mtl();

        switch_opengl_win( save_win );
    }

    update_actions_sens();
}


/*****************************************************************
 * TAG( destroy_mtl_mgr )
 *
 * If the material manager exists, free all its resources and
 * destroy it.
 */
void
destroy_mtl_mgr( void )
{
    Window my_win;

    if ( mtl_base == NULL )
    {
        return;
    }

    my_win = XtWindow( mtl_base );
    XDestroyWindow( dpy, my_win );
    my_win = 0;

    XtDestroyWidget( mtl_base );
    mtl_base = NULL;

    mtl_mgr_top_win = 0;
    mtl_base = NULL;


}


/*****************************************************************
 * TAG( destroy_surf_mgr )
 *
 * If the surface manager exists, free all its resources and
 * destroy it.
 */
void
destroy_surf_mgr( void )
{

    if ( surf_mgr_widg == NULL )
        return;
    XtDestroyWidget( surf_mgr_widg );
    surf_mgr_widg = NULL;
}


/*****************************************************************
 * TAG( update_swatch_label )
 *
 * Update the material swatch label.
 */
static void
update_swatch_label( void )
{
    XmString sw_label;
    char cbuf[18];

    if ( mtl_select_list == NULL )
        sprintf( cbuf, "No mat"  );
    else if ( mtl_select_list->next == NULL )
        sprintf( cbuf, "Mat %d", mtl_select_list->mtl );
    else
        sprintf( cbuf, "Mat %d...", mtl_select_list->mtl );

    sw_label = XmStringCreateLocalized( cbuf );

    XtVaSetValues( swatch_label, XmNlabelString, sw_label, NULL );

    XmStringFree( sw_label );
}


/*****************************************************************
 * TAG( mtl_func_active )
 *
 * Returns TRUE if any of the material functions are active.
 */
static Bool_type
mtl_func_active( void )
{
    int i;

    for ( i = 0; i < MTL_FUNC_QTY; i++ )
        if ( XmToggleButtonGetState( mtl_mgr_func_toggles[i] ) )
            return TRUE;

    return FALSE;
}

/*****************************************************************
 * TAG( surf_func_active )
 *
 * Returns TRUE if any of the surface functions are active.
 */
static Bool_type
surf_func_active( void )
{
    int i;

    for ( i = 0; i < SURF_FUNC_QTY; i++ )
        if ( XmToggleButtonGetState( surf_mgr_func_toggles[i] ) )
            return TRUE;

    return FALSE;
}

/*****************************************************************
 * TAG( set_scales_to_mtl )
 *
 * Initialize the color editor scales to indicate the color and
 * shininess for the material at the head of the material select
 * list.
 */
static void
set_scales_to_mtl( void )
{
    int idx;
    static GLfloat black[] = { 0.0, 0.0, 0.0, 0.0 };
    Boolean rgb_sens;

    rgb_sens = XtIsSensitive( col_ed_scales[0][RED_SCALE] );

    /* If no materials, set everything to zero. */
    if ( mtl_select_list == NULL )
    {
        set_rgb_scales( black );
        set_shininess_scale( (GLfloat) 0.0 );
    }
    else
    {
        /* Get material index. */
        /* idx = (mtl_select_list->mtl - 1) % MAX_MATERIALS; */
        idx = (mtl_select_list->mtl - 1);

        /*
         * Set shininess according to value-changed flag.  If this routine
         * is called to initialize the rgb scales, we don't want an
         * adjusted shininess value to be overwritten with the current
         * material value.
         */
        if ( prop_val_changed[SHININESS] )
            set_shininess_scale( property_vals[SHININESS][0] );
        else
            set_shininess_scale( v_win->mesh_materials.shininess[idx] );

        /* Never update if scales are insensitive. */
        if ( rgb_sens )
        {
            if ( prop_val_changed[cur_mtl_comp] )
                set_rgb_scales( property_vals[cur_mtl_comp] );
            else
                switch( cur_mtl_comp )
                {
                case AMBIENT:
                    set_rgb_scales( v_win->mesh_materials.ambient[idx] );
                    break;
                case DIFFUSE:
                    set_rgb_scales( v_win->mesh_materials.diffuse[idx] );
                    break;
                case SPECULAR:
                    set_rgb_scales( v_win->mesh_materials.specular[idx] );
                    break;
                case EMISSIVE:
                    set_rgb_scales( v_win->mesh_materials.emission[idx] );
                    break;
                default:
                    set_rgb_scales( black );
                }
        }
    }
}


/*****************************************************************
 * TAG( set_rgb_scales )
 *
 * Initialize the color editor RGB scales to the passed values
 * and redraw the swatch.
 */
static void
set_rgb_scales( GLfloat col[4] )
{
    XmString sval;
    char valbuf[5];
    int i, ival, min, max;

    for ( i = 0; i < 3; i++ )
    {
        XtVaGetValues( col_ed_scales[0][i],
                       XmNminimum, &min,
                       XmNmaximum, &max,
                       NULL );
        sprintf( valbuf, "%4.2f", col[i] );
        sval = XmStringCreateLocalized( valbuf );
        ival = (int) ((float) min + col[i] * (max - min) + 0.5);
        XtVaSetValues( col_ed_scales[0][i], XmNvalue, ival, NULL );
        XtVaSetValues( col_ed_scales[1][i], XmNlabelString, sval, NULL );
        XmStringFree( sval );

        cur_color[i] = col[i];
    }
    draw_mtl_swatch( cur_color );
}


/*****************************************************************
 * TAG( set_shininess_scale )
 *
 * Initialize the color editor RGB scales to the passed values
 * and redraw the swatch.
 */
static void
set_shininess_scale( GLfloat shine )
{
    XmString sval;
    char valbuf[5];
    int min, max;

    XtVaGetValues( col_ed_scales[0][SHININESS_SCALE],
                   XmNminimum, &min,
                   XmNmaximum, &max,
                   NULL );
    sprintf( valbuf, "%d", (int) shine );
    sval = XmStringCreateLocalized( valbuf );
    XtVaSetValues( col_ed_scales[0][SHININESS_SCALE],
                   XmNvalue, (int) shine,
                   NULL );
    XtVaSetValues( col_ed_scales[1][SHININESS_SCALE],
                   XmNlabelString, sval,
                   NULL );
    XmStringFree( sval );
}


/*****************************************************************
 * TAG( send_mtl_cmd )
 *
 * Call parse_command on the material manager command string,
 * subdividing if necessary to avoid exceeding the maximum number
 * of tokens.
 */
static void
send_mtl_cmd( char *cmd, int tok_qty )
{
    XmString text= NULL;
    Widget hist_list;
    if ( tok_qty > MAXTOKENS )
    {
        char *cbuf, *p_c;
        int sent, max_tok, limit, i, j, eff_tok_qty;

        p_c = cmd + 4; /* Skip over "mtl " at beginning of command. */
        eff_tok_qty = tok_qty - 1; /* Disregard "mtl" in count as well. */
        sent = 0;
        max_tok = MAXTOKENS - 2; /* Allow for "mtl" and "continue" tokens. */
        while ( sent < eff_tok_qty )
        {
            /*
             * Set "limit" to pass the maximum number of tokens each time
             * parse_command is called.  On last pass, copy last token
             * "manually" as it is not terminated with a space character
             * and counting logic will fail.
             */
            limit = ( eff_tok_qty - sent > max_tok )
                    ? max_tok : eff_tok_qty - sent - 1;
            for ( i = 0, j = 0; i < limit; j++, i++ )
                for ( ; *(p_c + j) != ' '; j++ );

            /*
             * Allocate enough buffer for command tokens plus "mtl "
             * plus "continue" (not always present) and a null
             * terminator.
             */
            cbuf = NEW_N( char, j + 13, "Subdivided mtl cmd" );

            /* Copy the tokens for this pass, add "continue" if not last pass. */
            memcpy( cbuf, cmd, 4 ); /* "mtl " begins the command. */
            memcpy( cbuf + 4, p_c, j ); /* Add the current subdivision. */
            if ( limit == max_tok )
                memcpy( cbuf + 4 + j, "continue", 9 );
            else
            {
                /* Copy last token into buffer. */
                strcpy( cbuf + 4 + j, p_c + j );
                limit++; /* Want last token counted when "sent" is updated. */
            }

            text = XmStringCreateSimple( cbuf );
            parse_command( cbuf, env.curr_analy );

            free( cbuf );

            p_c += j;
            sent += limit;
        }
    }
    else
    {
        text = XmStringCreateSimple( cmd );
        parse_command( cmd, env.curr_analy );
    }

    if( text != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }
}



/*****************************************************************
 * TAG( send_surf_cmd )
 *
 * Call parse_command on the surface manager command string,
 * subdividing if necessary to avoid exceeding the maximum number
 * of tokens.
 */
static void
send_surf_cmd( char *cmd, int tok_qty )
{
    XmString text= NULL;
    Widget hist_list;
    if ( tok_qty > MAXTOKENS )
    {
        char *cbuf, *p_c;
        int sent, max_tok, limit, i, j, eff_tok_qty;

        p_c = cmd + 5; /* Skip over "surf " at beginning of command. */
        eff_tok_qty = tok_qty - 1; /* Disregard "surf" in count as well. */
        sent = 0;
        max_tok = MAXTOKENS - 2; /* Allow for "surf" and "continue" tokens. */
        while ( sent < eff_tok_qty )
        {
            /*
             * Set "limit" to pass the maximum number of tokens each time
             * parse_command is called.  On last pass, copy last token
             * "manually" as it is not terminated with a space character
             * and counting logic will fail.
             */
            limit = ( eff_tok_qty - sent > max_tok )
                    ? max_tok : eff_tok_qty - sent - 1;
            for ( i = 0, j = 0; i < limit; j++, i++ )
                for ( ; *(p_c + j) != ' '; j++ );

            /*
             * Allocate enough buffer for command tokens plus "surf "
             * plus "continue" (not always present) and a null
             * terminator.
             */
            cbuf = NEW_N( char, j + 14, "Subdivided surf cmd" );

            /* Copy the tokens for this pass, add "continue" if not last pass. */
            memcpy( cbuf, cmd, 5 ); /* "surf " begins the command. */
            memcpy( cbuf + 5, p_c, j ); /* Add the current subdivision. */
            if ( limit == max_tok )
                memcpy( cbuf + 5 + j, "continue", 9 );
            else
            {
                /* Copy last token into buffer. */
                strcpy( cbuf + 5 + j, p_c + j );
                limit++; /* Want last token counted when "sent" is updated. */
            }

            text = XmStringCreateSimple( cbuf );
            parse_command( cbuf, env.curr_analy );

            free( cbuf );

            p_c += j;
            sent += limit;
        }
    }
    else
    {
        text = XmStringCreateSimple( cmd );
        parse_command( cmd, env.curr_analy );
    }

    if( text != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }
}
/*****************************************************************
 * TAG( load_mtl_mgr_funcs )
 *
 * Copy material manager function command strings into buffer
 * for active functions.
 */
static size_t
load_mtl_mgr_funcs( char *p_buf, int *p_token_cnt )
{
    Boolean set;
    char *p_dest, *p_src;
    int i, t_cnt;

    p_dest = p_buf;
    i = 0;
    t_cnt = 0;

    XtVaGetValues( mtl_mgr_func_toggles[VIS], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "vis "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    XtVaGetValues( mtl_mgr_func_toggles[INVIS], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "invis "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    XtVaGetValues( mtl_mgr_func_toggles[ENABLE], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "enable "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    XtVaGetValues( mtl_mgr_func_toggles[DISABLE], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "disable "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    XtVaGetValues( mtl_mgr_func_toggles[COLOR], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "mat "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    *p_token_cnt = t_cnt;

    return i;
}

/*****************************************************************
 * TAG( load_surf_mgr_funcs )
 *
 * Copy surface manager function command strings into buffer
 * for active functions.
 */
static size_t
load_surf_mgr_funcs( char *p_buf, int *p_token_cnt )
{
    Boolean set;
    char *p_dest, *p_src;
    int i, t_cnt;

    p_dest = p_buf;
    i = 0;
    t_cnt = 0;

    XtVaGetValues( surf_mgr_func_toggles[VIS_SURF], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "vis "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    XtVaGetValues( surf_mgr_func_toggles[INVIS_SURF], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "invis "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    XtVaGetValues( surf_mgr_func_toggles[ENABLE_SURF], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "enable "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    XtVaGetValues( surf_mgr_func_toggles[DISABLE_SURF], XmNset, &set, NULL );
    if ( set )
    {
        t_cnt++;
        for ( p_src = "disable "; *p_dest = *p_src; i++, p_src++, p_dest++ );
    }

    *p_token_cnt = t_cnt;

    return i;
}

/*****************************************************************
 * TAG( load_selected_mtls )
 *
 * Copy material manager selected materials into buffer as text.
 */
static size_t
load_selected_mtls( char *p_buf, int *p_tok_cnt )
{
    char *p_dest, *p_init;
    Material_list_obj *p_mtl;
    int t_cnt;

    p_dest = p_buf;
    p_init = p_buf;
    p_mtl = mtl_select_list;
    t_cnt = 0;

    for ( p_mtl = mtl_select_list; p_mtl != NULL; p_mtl = p_mtl->next )
    {
        sprintf( p_dest, "%d ", p_mtl->mtl );
        p_dest += strlen( p_dest );
        t_cnt++;
    }

    *p_tok_cnt = t_cnt;

    return (p_dest - p_init);
}


/*****************************************************************
 * TAG( load_selected_surfs )
 *
 * Copy material manager selected materials into buffer as text.
 */
static size_t
load_selected_surfs( char *p_buf, int *p_tok_cnt )
{
    char *p_dest, *p_init;
    Surface_list_obj *p_surf;
    int t_cnt;

    p_dest = p_buf;
    p_init = p_buf;
    p_surf = surf_select_list;
    t_cnt = 0;

    for ( p_surf = surf_select_list; p_surf != NULL; p_surf = p_surf->next )
    {
        sprintf( p_dest, "%d ", p_surf->surf );
        p_dest += strlen( p_dest );
        t_cnt++;
    }

    *p_tok_cnt = t_cnt;

    return (p_dest - p_init);
}

/*****************************************************************
 * TAG( load_mtl_properties )
 *
 * Copy material manager color editor properties as text for
 * those material properties which user has edited.
 */
static size_t
load_mtl_properties( char *p_buf, int *p_tok_cnt )
{
    int i, t_cnt;
    size_t c_cnt, c_add;
    char *p_dest;
    static char *name[] = { "amb", "diff", "spec", "emis" };
    GLfloat *p_prop_val;

    /* Loop over the possible material properties. */
    t_cnt = 0;
    c_cnt = 0;
    p_dest = p_buf;
    for ( i = 0; i < MTL_PROP_QTY; i++ )
    {
        /*
         * Update any changed property plus the currently selected property
         * when there are multiple materials selected (this assumes that
         * the current property will be different for the multiple selected
         * materials so an update makes sense.  Conversely, if there were
         * only one material selected and the current property had not
         * changed there'd be no sense in updating it).
         */
        if ( prop_val_changed[i] || ( i == cur_mtl_comp
                                      && mtl_select_list->next != NULL ) )
        {
            if ( i == SHININESS )
            {
                /* Shininess is ONLY passed if it changed. */

                /* Shininess has only one value. */
                sprintf( p_dest, "shine %4.2f ", property_vals[i][0] );
                t_cnt += 2;
                c_cnt += 11;
                p_dest += 11;
            }
            else
            {
                /*
                 * If ambient, diffuse, specular, or emissive property
                 * is the currently selected property and hasn't changed,
                 * read it from the global cur_color array.
                 */
                if ( !prop_val_changed[i] )
                    p_prop_val = cur_color;
                else
                    p_prop_val = property_vals[i];

                /*
                 * These properties each have three values.
                 */
                sprintf( p_dest, "%s %4.2f %4.2f %4.2f ", name[i],
                         p_prop_val[0], p_prop_val[1],
                         p_prop_val[2] );
                t_cnt += 4;
                c_add = strlen( name[i] ) + 1 + 3 * 5;
                c_cnt += c_add;
                p_dest += c_add;
            }
        }
    }

    *p_tok_cnt = t_cnt;

    return c_cnt;
}


/*****************************************************************
 * TAG( update_actions_sens )
 *
 * Set sensitivity of the action pushbuttons dependent upon
 * the current state of the material manager.  Also manage the
 * mapping/unmapping of the property modify indicators when
 * the color function is selected.
 */
static void
update_actions_sens( void )
{
    int i;
    Boolean set;
    Bool_type actions_set, show_check[MTL_PROP_QTY];

    XtVaGetValues( mtl_mgr_func_toggles[COLOR], XmNset, &set, NULL );

    if ( !mtl_func_active() || mtl_select_list == NULL )
    {
        /*
         * At least one function and one material must be selected for
         * any action to be sensitive.
         */
        for ( i = 0; i < MTL_OP_QTY; i++ )
        {
            if ( XtIsSensitive( op_buttons[i] ) )
                XtSetSensitive( op_buttons[i], False );
            if ( set )
                XtUnmapWidget( prop_checks[i] );
        }
    }
    else if ( set )
    {
        /*
         * Default is always available when color selected and
         * material select list is not empty.
         */
        if ( !XtIsSensitive( op_buttons[OP_DEFAULT] ) )
            XtSetSensitive( op_buttons[OP_DEFAULT], True );
        /*
         * When color function selected, manage the property modify
         * indicators.  Actions are sensitive if any modify indicators
         * are shown (Preview/Cancel must be handled as well).
         */
        for ( i = 0; i < MTL_PROP_QTY; i++ )
        {
            show_check[i] = FALSE;
            if ( prop_val_changed[i] )
                show_check[i] = TRUE;
            else if ( i != SHININESS )
            {
                set = False;
                XtVaGetValues( color_comps[i], XmNset, &set, NULL );
                if ( set && mtl_select_list->next != NULL )
                    show_check[i] = TRUE;
            }
        }

        actions_set = FALSE;
        for ( i = 0; i < MTL_PROP_QTY; i++ )
        {
            if ( show_check[i] )
            {
                XtMapWidget( prop_checks[i] );

                if ( !actions_set )
                {
                    /*
                     * Apply always sensitive when a component is
                     * selected.
                     */
                    if ( !XtIsSensitive( op_buttons[OP_APPLY] ) )
                        XtSetSensitive( op_buttons[OP_APPLY], True );

                    /* Preview and Cancel are mutually exclusive. */
                    if ( preview_set )
                    {
                        XtSetSensitive( op_buttons[OP_CANCEL], True );
                        XtSetSensitive( op_buttons[OP_PREVIEW], False );
                    }
                    else
                    {
                        XtSetSensitive( op_buttons[OP_CANCEL], False );
                        XtSetSensitive( op_buttons[OP_PREVIEW], True );
                    }

                    actions_set = TRUE;
                }
            }
            else
                XtUnmapWidget( prop_checks[i] );
        }

        /* If no modify indicators were mapped, only Default available. */
        if ( !actions_set )
            for ( i = 0; i < OP_DEFAULT; i++ )
            {
                if ( XtIsSensitive( op_buttons[i] ) )
                    XtSetSensitive( op_buttons[i], False );
            }
    }
    else
    {
        /* For non-color functions, Apply is is only action. */
        if ( !XtIsSensitive( op_buttons[OP_APPLY] ) )
            XtSetSensitive( op_buttons[OP_APPLY], True );
        if ( XtIsSensitive( op_buttons[OP_PREVIEW] ) )
            XtSetSensitive( op_buttons[OP_PREVIEW], False );
        if ( XtIsSensitive( op_buttons[OP_CANCEL] ) )
            XtSetSensitive( op_buttons[OP_CANCEL], False );
        if ( XtIsSensitive( op_buttons[OP_DEFAULT] ) )
            XtSetSensitive( op_buttons[OP_DEFAULT], False );
    }
}

/*****************************************************************
 surf_* TAG( update_surf_actions_sens )
 *
 * Set sensitivity of the action pushbuttons dependent upon
 * the current state of the surface manager.
 */
static void
update_surf_actions_sens( void )
{
    int i;

    if ( !surf_func_active() || surf_select_list == NULL )
    {
        /*
         * At least one function and one surface must be selected for
         * any action to be sensitive.
         */
        for ( i = 0; i < SURF_OP_QTY; i++ )
        {
            if ( XtIsSensitive( surf_op_buttons[i] ) )
                XtSetSensitive( surf_op_buttons[i], False );
        }
    }
    else
    {
        if ( !XtIsSensitive( surf_op_buttons[SURF_OP_APPLY] ) )
            XtSetSensitive( surf_op_buttons[SURF_OP_APPLY], True );
        if ( XtIsSensitive( surf_op_buttons[SURF_OP_DEFAULT] ) )
            XtSetSensitive( surf_op_buttons[SURF_OP_DEFAULT], False );
    }
}

/*****************************************************************
 * TAG( gress_mtl_mgr_EH )
 *
 * Event handler for when pointer enters or leaves the material
 * manager to set the proper rendering context and window.
 */
static void
gress_mtl_mgr_EH( Widget w, XtPointer client_data, XEvent *event,
                  Boolean *continue_dispatch )
{
    if ( event->xcrossing.type == EnterNotify && XtIsSensitive( color_editor ) )
        switch_opengl_win( SWATCH );
    else
        switch_opengl_win( MESH_VIEW );
}

/*****************************************************************
 * TAG( gress_surf_mgr_EH )
 *
 * Event handler for when pointer enters or leaves the surface
 * manager to set the proper rendering context and window.
 */
static void
gress_surf_mgr_EH( Widget w, XtPointer client_data, XEvent *event,
                   Boolean *continue_dispatch )
{
    switch_opengl_win( MESH_VIEW );
}


/*****************************************************************
 * TAG( action_create_app_widg )
 *
 * Action routine for creating the Material Manager or Utility
 * Panel.
 */
static void
action_create_app_widg( Widget w, XEvent *event, String params[], int *qty )
{
    Btn_type btn;

    if ( *qty != 1 )
    {
        popup_dialog( WARNING_POPUP,
                      "Invalid args to action_create_app_widg()." );
        return;
    }
    else
        btn = (Btn_type) atoi( params[0] );

    create_app_widg( btn );
}


/* ARGSUSED 2 */
/*****************************************************************
 * TAG( resize_mtl_scrollwin )
 *
 * Resize action routine for the material manager ScrolledWindow.
 */
static void
resize_mtl_scrollwin( Widget w, XEvent *event, String params[], int qty )
{
    WidgetList children;
    Widget row_col, scroll;
    XConfigureEvent *cevent;
    int width;
    Dimension button_width, margin_width, spacing, scrollbar_width;
    short max_cols, rows;

    cevent = (XConfigureEvent *) event;
    width = cevent->width;

    XtVaGetValues( w,
                   XmNworkWindow, &row_col,
                   XmNverticalScrollBar, &scroll,
                   NULL );

    XtVaGetValues( row_col,
                   XmNchildren, &children,
                   XmNmarginWidth, &margin_width,
                   XmNspacing, &spacing,
                   NULL );

    XtVaGetValues( children[0],
                   XmNwidth, &button_width,
                   NULL );

    XtVaGetValues( scroll,
                   XmNwidth, &scrollbar_width,
                   NULL );

    max_cols = (width - 2.0 * margin_width - scrollbar_width - 4 + spacing)
               / (float) (button_width + spacing);
    rows = (short) ceil( (float) env.curr_analy->mesh_table[0].material_qty
                         / max_cols );
    XtVaSetValues( row_col, XmNnumColumns, rows, NULL );
}


/*****************************************************************
 * TAG( resize_surf_scrollwin )
 *
 * Resize action routine for the surface manager ScrolledWindow.
 */
static void
resize_surf_scrollwin( Widget w, XEvent *event, String params[], int qty )
{
    WidgetList children;
    Widget row_col, scroll;
    XConfigureEvent *cevent;
    int width;
    Dimension button_width, margin_width, spacing, scrollbar_width;
    short max_cols, rows;

    cevent = (XConfigureEvent *) event;
    width = cevent->width;

    XtVaGetValues( w,
                   XmNworkWindow, &row_col,
                   XmNverticalScrollBar, &scroll,
                   NULL );

    XtVaGetValues( row_col,
                   XmNchildren, &children,
                   XmNmarginWidth, &margin_width,
                   XmNspacing, &spacing,
                   NULL );

    XtVaGetValues( children[0],
                   XmNwidth, &button_width,
                   NULL );

    XtVaGetValues( scroll,
                   XmNwidth, &scrollbar_width,
                   NULL );

    max_cols = (width - 2.0 * margin_width - scrollbar_width - 4 + spacing)
               / (float) (button_width + spacing);
    rows = (short) ceil( (float) env.curr_analy->mesh_table[0].material_qty
                         / max_cols );
    XtVaSetValues( row_col, XmNnumColumns, rows, NULL );
}


/*****************************************************************
 * TAG( string_convert )
 *
 * Convert a compound Motif string to a normal "C" string.
 */
static void
string_convert( XmString str, char *buf )
{
    XmStringContext context;
    char *text, *p;
    XmStringCharSet charset;
    XmStringDirection direction;
    Boolean separator;
    int size = 0;

    if ( !XmStringInitContext( &context, str ) )
    {
        XtWarning( "Can't convert compound string." );
        return;
    }

    p = buf;

    while ( XmStringGetNextSegment( context, &text, &charset, &direction,
                                    &separator ) )
    {
        size += strlen(text);
        if(size < 198)
        {
            p += (strlen(strcpy(p, text)));
        }
        else
        {
            p += (strlen(strncpy(p, text, 197 - strlen(buf))));
            *p++ = '"';
            separator = TRUE;
        }

        if ( separator == TRUE )
        {
            /* Add newline and null-terminate. */
            *p++ = '\n';
            *p = 0;
        }

        XtFree( text );
    }


    XmStringFreeContext( context );
}


/* ARGSUSED 0 */
/*****************************************************************
 * TAG( exit_CB )
 *
 * The callback for the error dialog that kills GRIZ after the user
 * pushes the "Exit" (ok) button.
 */
static void
exit_CB( Widget w, XtPointer client_data, XtPointer reason )
{
    parse_command( "quit", env.curr_analy );

}


/*****************************************************************
 * TAG( init_animate_workproc )
 *
 * Start up the animation work proc.  This is used so that
 * view controls will stay active while the animation happens.
 */
void
init_animate_workproc( void )
{
    stop_animate = FALSE;

    /* Fire up the work proc. */
    anim_work_proc_id = XtAppAddWorkProc(app_context, animate_workproc_CB, 0);
}


/*****************************************************************
 * TAG( animate_workproc_CB )
 *
 * Move the current animation forward.
 */
static Boolean
animate_workproc_CB()
{
    Bool_type done;

    /* Verify that we haven't cleaned up somehow. */
    if ( !anim_work_proc_id )
        popup_dialog( WARNING_POPUP, "Anim work_proc around after cleanup." );

    /* Move the animation forward a step and return whether done. */
    done = step_animate( env.curr_analy );

    if ( done )
        env.animate_active = FALSE;
    else
        env.animate_active = TRUE;

    /* Returning true will kill the workproc. */
    return ( stop_animate ? TRUE : done );
}


/*****************************************************************
 * TAG( end_animate_workproc )
 *
 * Suspend the animation and remove the animation work proc (if
 * we need to.)
 */
void
end_animate_workproc( Bool_type remove_workproc )
{
    /* No animation active. */
    if ( !anim_work_proc_id )
        return;

    /* Cancel the work proc. */
    if ( remove_workproc )
    {
        XtRemoveWorkProc( anim_work_proc_id );
        stop_animate = TRUE;
    }

    anim_work_proc_id = 0;

    /* Call the cleanup routine. */
    end_animate( env.curr_analy );
}


/*****************************************************************
 * TAG( set_window_size )
 *
 * Perform non-default initialization of rendering window width and
 * height parameters.
 */
void
set_window_size( int w_width, int w_height )
{
    window_width = (Dimension) w_width;
    window_height = (Dimension) w_height;

    return;
}


/*****************************************************************
 * TAG( get_window_width )
 *
 * Return current rendering window width.
 */
int
get_window_width()
{
    return( (int)window_width );
}


/*****************************************************************
 * TAG( get_window_height )
 *
 * Return current rendering window height.
 */
int
get_window_height()
{
    return( (int)window_height );
}


/*****************************************************************
 * TAG( get_monitor_width )
 *
 * Query the monitor widget and return its current character width.
 */
int
get_monitor_width( void )
{
    int n;
    short ncols;
    Arg args[1];

    n = 0;
    XtSetArg( args[n], XmNcolumns, &ncols );
    n++;
    XtGetValues( monitor_widg, args, n );

    return (int) ncols;
}


/***************************************************************** * TAG( get_step_stride ) *
 * Return current step stride.
 */
int
get_step_stride()
{
    return( (int) step_stride );
}


/*****************************************************************
 * TAG( put_step_stride )
 *
 * Return current step stride.
 */
void
put_step_stride( int stride )
{
    step_stride = stride;
}


/*****************************************************************
 * TAG( write_start_text )
 *
 * Write start-up text messages to the monitor window.
 * 07/19/2013 Bill Oliver:
 * Removed requirement to have a griz_start_text file by taking configuration
 *        information from griz_config.h file instead.
 */
void
write_start_text( void )
{
    int i;
    char text_line[13][132];
    char asteric[] = "**********************************************************************";
    sprintf(text_line[0],"%s\n\n", asteric);
    sprintf(text_line[1],"                    WELCOME TO GRIZ %s\n\n", PACKAGE_VERSION);
    sprintf(text_line[2],"                      Updated: %s \n\n", PACKAGE_DATE);
    sprintf(text_line[3],"%s\n\n\n", asteric);
    sprintf(text_line[4],"Griz On-Line Manual: \n\n");
    sprintf(text_line[5],"       The Griz manual is now available on-line via the\n");
    sprintf(text_line[6],"       Help option (top-right) on the Griz Gui.\n\n");
    sprintf(text_line[7],"%s\n\n", asteric);
    sprintf(text_line[8],"Contacts:\n\n");
    sprintf(text_line[9],"For questions or problems relating to GRIZ please contact:\n\n");
    sprintf(text_line[10],"%s\n", PACKAGE_BUGREPORT);
    sprintf(text_line[11],"%s\n\n", PACKAGE_BUGREPORT_ALT);
    sprintf(text_line[12], "%s\n", asteric);

#ifdef SERIAL_BATCH
    for(i = 0; i < 13; i++)
    {
        wrt_text("%s\n", text_line[i]);
    }
#else
    for(i = 0; i < 13; i++)
    {
        XmTextInsert(monitor_widg, wpr_position, text_line[i]);
        wpr_position += strlen(text_line[i]);
    }
#endif

    wrt_standard_db_text( env.curr_analy, FALSE );

    return;
}


/*****************************************************************
 * TAG( wrt_standard_db_text )
 *
 * Write standard startup text for a new database.
 */

#define MAX_DBTEXT_LINES 2000
#define MAX_BLOCKS       2000

void
wrt_standard_db_text( Analysis *analy, Bool_type advance )
{
    int i, j, k;
    int look_ahead_index=0;

    int cnt, first, qty_states;
    float start_t, end_t;
    int db_ident;
    MO_class_data *class_ptr=NULL;
    char class_name[M_MAX_NAME_LEN];
    int  particle_count=0;

    int block_label_start, block_label_end;
    int block_start, block_end, block_count=0,
                                num_blocks=0;
    int *blocks;

    int num_class_blocks=0, qty_objects=0,
        temp_label, block_total;

    Bool_type labels_found=FALSE;

    int  qty_classes=0;
    char *class_names[2000];
    int  superclasses[2000];

    static int traverse_order[] =
    {
        G_HEX, G_TET, G_QUAD, G_TRI
    };
    static char *labels[] =
    {
        "Unit", "Node", "Truss", "Beam", "Tri", "Quad", "Tet", "Pyramid",
        "Wedge", "Hex"
    };

    Mesh_data *p_mesh;
    List_head *p_lh;
    int sum, type;
    char *start_text[MAX_DBTEXT_LINES], temp_text[512];
    int rval;

    int block_index, temp_block_index, st_qty;
    int labels_min, labels_max;
    int sclass, superclass;

    State_rec_obj *p_state_rec;
    Subrecord     *p_subr;
    Subrec_obj    *p_subr_obj;
    Famid fid;
    int sclass_cnt=0;

    int status=OK;

    db_ident = analy->db_ident;
    p_mesh   = MESH_P( analy );

    /* For TH databases we could get many lines of block data
     * so we need to allow for this.
     */

    status = mili_get_class_names( analy, &qty_classes, class_names, superclasses );

    p_state_rec = analy->srec_tree;
    cnt = 0;
    sprintf( temp_text, "\nData file: %s\n", env.curr_analy->root_name );
    start_text[cnt++] = (char *) strdup(temp_text) ;

    rval = analy->db_query( analy->db_ident, QRY_QTY_STATES, NULL, NULL,
                            (void *) &qty_states );
    if ( rval == OK )
    {
        sprintf( temp_text, "Number of states: %d\n", qty_states );
        start_text[cnt++] = (char *) strdup(temp_text) ;
        if ( qty_states > 0 )
        {
            first = 1;
            rval = analy->db_query( db_ident, QRY_STATE_TIME, (void *) &first,
                                    NULL, (void *) &start_t );
            if ( rval == OK )
            {
                sprintf( temp_text, "Start time: %.4e\n", start_t );
                start_text[cnt++] = (char *) strdup(temp_text) ;
            }

            rval = analy->db_query( db_ident, QRY_STATE_TIME,
                                    (void *) &qty_states, NULL,
                                    (void *) &end_t );
            if ( rval == OK )
            {
                sprintf( temp_text, "End time: %.4e\n", end_t );
                start_text[cnt++] = (char *) strdup(temp_text) ;
            }

        }
        if ( rval == OK )
        {
            sprintf( temp_text, "\n--------------------------------------------------------------------\n" );
            start_text[cnt++] = (char *) strdup(temp_text) ;

            sprintf( temp_text, "\n\tMili Database-Library Version: \t%s\n", analy->mili_version );
            start_text[cnt++] = (char *) strdup(temp_text) ;
            sprintf( temp_text, "\tMili Database-Host: \t\t%s\n", analy->mili_host );
            start_text[cnt++] = (char *) strdup(temp_text) ;
            sprintf( temp_text, "\tMili Database-Arch: \t\t%s\n", analy->mili_arch );
            start_text[cnt++] = (char *) strdup(temp_text) ;
            sprintf( temp_text, "\tMili Database-Timestamp: \t%s\n\n", analy->mili_timestamp );
            start_text[cnt++] = (char *) strdup(temp_text) ;
            if ( strlen(analy->xmilics_version)>0 )
            {
                sprintf( temp_text, "\tXmili Version: \t%s\n", analy->xmilics_version );
                start_text[cnt++] = (char *) strdup(temp_text) ;
            }
            sprintf( temp_text, "--------------------------------------------------------------------\n\n" );
            start_text[cnt++] = (char *) strdup(temp_text) ;
        }
    }

    /* Sum polygons to render by superclass. */
    for ( i = 0; i < sizeof( traverse_order ) / sizeof( traverse_order[0] );
            i++ )
    {
        type = traverse_order[i];
        p_lh = p_mesh->classes_by_sclass + type;
        sum = 0;

        for ( j = 0; j < p_lh->qty; j++ )
        {
            if ( type == G_HEX || type == G_TET )
                sum += ((MO_class_data **) p_lh->list)[j]->p_vis_data->face_cnt;
            else
                sum += ((MO_class_data **) p_lh->list)[j]->qty;
        }

        if ( sum > 0 )
        {
            if ( type == G_HEX || type == G_TET )
            {
                sprintf( temp_text,
                         "Number of %s faces to render: \t%d\n", labels[type],
                         sum );
                start_text[cnt++] = (char *) strdup(temp_text) ;
            }
            else
            {
                sprintf( temp_text,
                         "Number of %s's to render: \t\t%d\n", labels[type],
                         sum );
                start_text[cnt++] = (char *) strdup(temp_text) ;
            }
        }
    }

    p_lh = p_mesh->classes_by_sclass + G_HEX;
    for ( j = 0; j < p_lh->qty; j++ )
    {
        class_ptr = ((MO_class_data **) p_lh->list)[j];
        if ( is_particle_class( analy, class_ptr->superclass, class_ptr->short_name ))
        {
            particle_count = class_ptr->qty;
            sprintf( temp_text,
                     "Number of Particles[%s] to render: \t%d\n", class_ptr->short_name, particle_count );
            start_text[cnt++] = (char *) strdup(temp_text) ;
        }
    }
    p_lh = p_mesh->classes_by_sclass + G_PARTICLE;
    for ( j = 0; j < p_lh->qty; j++ )
    {
        class_ptr = ((MO_class_data **) p_lh->list)[j];
        if ( is_particle_class( analy, class_ptr->superclass, class_ptr->short_name ))
        {
            particle_count = class_ptr->qty;
            sprintf( temp_text,
                     "Number of Particles[%s] to render: \t%d\n", class_ptr->short_name, particle_count );
            start_text[cnt++] = (char *) strdup(temp_text) ;
        }
    }

    /* IRC: Added March 29, 2005 - Print list of selected objects
     * in state records.
     */
    p_state_rec = analy->srec_tree;
    if (p_state_rec != NULL)
        if(p_state_rec->qty > 0)
        {
            analy->cur_state = 0;

            analy->db_get_state( analy, 0, analy->state_p, &analy->state_p,
                                 &st_qty );

            /* Loop over classes */
            for ( j = 0;
                    j < qty_classes;
                    j++ )
            {

                strcpy( class_name, class_names[j] );
                sclass = superclasses[j];

                qty_objects     = 0;
                num_blocks      = 0;

                class_ptr = get_blocking_info( analy, class_name, sclass,
                                               &qty_objects,
                                               &num_blocks, &blocks );

                if ( qty_objects==0 )
                    continue;

                labels_found = class_ptr->labels_found;

                if ( !analy->stateDB &&  num_blocks==0 )
                    if ( num_blocks==0 )
                        continue;

                if ( qty_objects > 0 )
                {
                    block_index = 0;

                    sprintf( temp_text,"\nClass: %s \t\t\t\t(Total %ss=%d)\n", class_name, class_name, qty_objects );
                    start_text[cnt++] = (char *) strdup(temp_text) ;

                    /************/
                    /* State DB */
                    /************/
                    if ( analy->stateDB )
                    {
                        /******************/
                        /* Without Labels */
                        /******************/
                        /* Case 1 - State DB and no labels */
                        if ( !labels_found )
                        {
                            block_start = 1;
                            block_end   = qty_objects;

                            if ( block_start!=block_end )
                                sprintf( temp_text,
                                         "\tblock[%d]: %d-%d\n", 1,
                                         block_start,
                                         block_end );
                            else
                                sprintf( temp_text,
                                         "\tblock[%d]: %d\n", 1,
                                         block_start);
                            start_text[cnt++] = (char *) strdup(temp_text) ;
                        }
                        else
                        {
                            /***************/
                            /* With Labels */
                            /***************/
                            /* Case 2 - State DB with labels */
                            block_label_start = class_ptr->labels_min;
                            block_label_end   = class_ptr->labels_max;

                            /*
                             * Used for debugging Labels
                             * if ( !strcmp(class_ptr->short_name, "brick") )
                             *    dump_class_labels(class_ptr);
                             */

                            if ( block_label_start!=block_label_end )
                                sprintf( temp_text,
                                         "\tblock[%d]: %d-%d (Labels: %d-%d)\n", 1,
                                         1,
                                         qty_objects,
                                         block_label_start,
                                         block_label_end);
                            else
                                sprintf( temp_text,
                                         "\tblock[%d]: %d (Label: %d)\n", 1,
                                         block_start, block_label_start );
                            start_text[cnt++] = (char *) strdup(temp_text) ;
                        }
                    }

                    /*********/
                    /* TH DB */
                    /*********/
                    if ( !analy->stateDB )
                    {
                        /******************/
                        /* Without Labels */
                        /******************/
                        /* Case 3 - TH DB and no labels */
                        if ( !labels_found )
                        {
                            block_count = 1;
                            block_index = 0;
                            for ( i = 0;
                                    i< num_blocks;
                                    i++ )
                            {
                                /* Check for overflow of text buffer */
                                if ( cnt >= MAX_DBTEXT_LINES )
                                {
                                    sprintf( temp_text, "Error - text length exceeded (length=%d)\n", cnt);
                                    start_text[cnt++] = (char *) strdup(temp_text) ;
                                    i=num_blocks;
                                    break;
                                }

                                block_start       = blocks[block_index] ;
                                block_end         = blocks[block_index+1] ;

                                sprintf( temp_text,
                                         "\tblock[%d]: %d-%d\n", block_count++,
                                         block_start,
                                         block_end );

                                start_text[cnt++] = (char *) strdup(temp_text) ;
                                block_index+=2;
                            } /* end for-i */

                            if ( cnt >= MAX_DBTEXT_LINES-5 )
                            {
                                start_text[cnt] = (char *) strdup("** Error Max lines exceeded!");
                                p_state_rec->qty;
                                break;
                            }

                        }
                        else
                        {
                            /***************/
                            /* With Labels */
                            /***************/
                            /* Case 4 - TH DB with labels */
                            block_count = 1;
                            block_index = 0;
                            for ( i = 0;
                                    i< num_blocks;
                                    i++ )
                            {
                                /* Check for overflow of text buffer */
                                if ( cnt >= MAX_DBTEXT_LINES )
                                {
                                    sprintf( temp_text, "Error - text length exceeded (length=%d)\n", cnt);
                                    start_text[cnt++] = (char *) strdup(temp_text) ;
                                    i=num_blocks;
                                    break;
                                }

                                block_start       = blocks[block_index] ;
                                block_end         = blocks[block_index+1] ;
                                block_label_start = get_class_label( class_ptr, block_start );
                                block_label_end   = get_class_label( class_ptr, block_end );

                                sprintf( temp_text,
                                         "\tblock[%d]: %d-%d (Labels: %d-%d)\n", block_count++,
                                         block_start,
                                         block_end,
                                         block_label_start,
                                         block_label_end);

                                start_text[cnt++] = (char *) strdup(temp_text) ;
                                block_index+=2;
                            } /* end for-i */

                            if ( cnt >= MAX_DBTEXT_LINES-5 )
                            {
                                start_text[cnt] = (char *) strdup("** Error Max lines exceeded!");
                                p_state_rec->qty;
                                break;
                            }

                        }
                    }

                }
            }
        }

    strcat( temp_text, "\n" );
    start_text[cnt] = (char *) strdup(temp_text) ;

#ifdef SERIAL_BATCH
    for ( i = 0;
            i < cnt;
            i++ )
    {
        wrt_text( "%s\n", start_text[i] );
    }
#else
    for ( i = 0;
            i < cnt;
            i++ )
    {
        XmTextInsert( monitor_widg, wpr_position, start_text[i] );
        wpr_position += strlen( start_text[i] );
    }

    if ( advance )
    {
        XtVaSetValues( monitor_widg, XmNcursorPosition, wpr_position, NULL );
        XmTextShowPosition( monitor_widg, wpr_position );
    }
#endif /* SERIAL_BATCH */

    for ( i=0;
            i<cnt;
            i++ )
        free( start_text[i] );

    /* Free up space for class names & superclasses */
    for ( i=0;
            i<qty_classes;
            i++ )
        free( class_names[i] );
}


/*****************************************************************
 * TAG( assemble_blocking )
 *
 * Construct an array of the blocking for an sclass type.
 */
MO_class_data *
assemble_blocking( Analysis *analy, int sclass, char *class_name,
                   int *qty_objects, int *label_block_qty,
                   int *total_blocks, int *blocks, int *blocks_labels )
{
    int st_qty;

    MO_class_data *class_ptr;
    State_rec_obj *p_state_rec;
    Subrecord     *p_subr;
    Subrec_obj    *p_subr_obj;

    int block_index=0, mo_id_index=0, num_blocks=0, new_blocks=0;
    int block_start, block_end, block_count=0;
    Bool_type id_found=FALSE;

    int i,j, k;

    int superclass=0;
    int sclass_cnt=0;

    p_state_rec   = analy->srec_tree;
    *total_blocks = 0;

    if (p_state_rec != NULL)
        if(p_state_rec->qty <=0 )
        {
            *label_block_qty = 0;
            *qty_objects     = 0;
            *total_blocks    = 0;
            return NULL;
        }

    analy->cur_state = 0;

    analy->db_get_state( analy, 0, analy->state_p, &analy->state_p,
                         &st_qty );
    for ( j = 0;
            j < p_state_rec->qty;
            j++ )
    {
        superclass = p_state_rec->subrecs[j].p_object_class->superclass;

        if ( sclass != superclass )
            continue;

        p_subr       = &p_state_rec->subrecs[j].subrec;
        *qty_objects = p_subr->qty_objects;

        strcpy( class_name, p_subr->class_name );
        class_ptr        = mili_get_class_ptr( analy, superclass, p_subr->class_name );
        *label_block_qty = class_ptr->label_blocking.block_qty;

        if ( *label_block_qty==0 || !analy->stateDB )
            num_blocks = p_subr->qty_blocks;
        else
            num_blocks = *label_block_qty;

        if ( num_blocks==0 )
            continue;

        mo_id_index = 0;
        new_blocks  = 0;

        for ( i = 0;
                i< num_blocks;
                i++ )
        {
            new_blocks++;

            if ( *label_block_qty==0 || !analy->stateDB )
            {
                block_start = p_subr->mo_blocks[mo_id_index];
                block_end   = p_subr->mo_blocks[mo_id_index+1];
            }
            else
            {
                block_start = class_ptr->label_blocking.block_objects[i].label_start;
                block_end   = class_ptr->label_blocking.block_objects[i].label_stop;
            }

            /* Make sure that this id is not already in list */
            id_found = FALSE;
            for ( k = 0;
                    k < block_index;
                    k+=2 )
                if ( blocks[k] == block_start )
                {
                    id_found = TRUE;
                    new_blocks--;
                }

            if ( !id_found )
            {
                blocks[block_index]   = block_start;
                blocks[block_index+1] = block_end;

                if ( *label_block_qty >0 )
                {
                    blocks_labels[block_index]   = get_class_label( class_ptr, block_start );
                    blocks_labels[block_index+1] = get_class_label( class_ptr, block_end );
                }
                block_index+=2;
            }
            mo_id_index+=2;
        }
        *total_blocks+= new_blocks;
    }
    return class_ptr;
}

/*****************************************************************
 * TAG( get_blocking_info )
 *
 * Get basic info for object blocking.
 */
MO_class_data *
get_blocking_info( Analysis *analy,   char *class_name, int sclass,
                   int *qty_objects,
                   int *total_blocks, int **blocks )
{
    MO_class_data *class_ptr;
    State_rec_obj *p_state_rec;
    Subrecord     *p_subr;
    int           superclass;
    int           i=0, j=0, k=0;

    int block_index=0, mo_id_index=0, num_blocks=0;
    int block_max_id = -1;
    int block_id=0, block_start, block_end, block_count=0;
    int list_count=0;

    int num_blocks_total=0, *temp_blocks, *block_dup_check, *pib;
    int *blocks_list;

    int status=OK;

    *qty_objects     = 0;

    class_ptr        = mili_get_class_ptr( analy, sclass, class_name );
    *qty_objects     = class_ptr->qty;

    /* Get the block list */
    p_state_rec   = analy->srec_tree;
    p_subr        = &p_state_rec->subrecs[j].subrec;
    *total_blocks = 0;

    if ( analy->stateDB )
    {
        *total_blocks = 1;
        temp_blocks = NEW_N( int, 2, "Array of block numbers - Single Block");
        temp_blocks[0] = 1;
        temp_blocks[1] = *qty_objects;
        *blocks = temp_blocks;
        return class_ptr;
    }

    /* First count the total number of blocks for memory allocation */
    for ( i = 0;
            i < p_state_rec->qty;
            i++ )
    {
        superclass = p_state_rec->subrecs[i].p_object_class->superclass;

        if ( sclass != superclass )
            continue;

        p_subr     = &p_state_rec->subrecs[i].subrec;
        class_ptr  = mili_get_class_ptr( analy, sclass, class_name );

        num_blocks = p_subr->qty_blocks;
        num_blocks_total += num_blocks;

        for ( j = 0;
                j< num_blocks*2;
                j++ )
        {
            block_id = p_subr->mo_blocks[j];
            if ( block_id > block_max_id )
                block_max_id = block_id;
        }
    }

    temp_blocks     = NEW_N( int, num_blocks_total*3, "Array of block numbers - TH Database");
    block_dup_check = NEW_N( int, block_max_id*2, "Array of block indexes used");
    for ( i=0;
            i<block_max_id;
            i++ )
    {
        block_dup_check[i] = -1;
    }

    block_index = 0;

    for ( i = 0;
            i < p_state_rec->qty;
            i++ )
    {
        superclass = p_state_rec->subrecs[i].p_object_class->superclass;

        if ( sclass != superclass )
            continue;

        p_subr    = &p_state_rec->subrecs[i].subrec;
        class_ptr = mili_get_class_ptr( analy, sclass, class_name );

        num_blocks = p_subr->qty_blocks;

        if ( num_blocks==0 )
            continue;

        mo_id_index = 0;

        for ( j = 0;
                j< num_blocks;
                j++ )
        {
            block_start = p_subr->mo_blocks[mo_id_index];
            block_end   = p_subr->mo_blocks[mo_id_index+1];

            if ( block_dup_check[block_start] == 1 && block_dup_check[block_end] == 1 )
            {
                mo_id_index+=2;
                continue;
            }

            block_dup_check[block_start] = 1;
            block_dup_check[block_end]   = 1;

            temp_blocks[block_index]   = block_start;
            temp_blocks[block_index+1] = block_end;

            block_index+=2;
            block_count++;
            mo_id_index+=2;
        }
    }

    for ( i=0;
            i<block_count*2;
            i++ )
    {
        if ( temp_blocks[i] > block_max_id )
            block_max_id = temp_blocks[i];
    }

    blocks_list = NEW_N( int, block_max_id*2, "Array of block numbers");
    for ( i=0;
            i<block_max_id*2;
            i++ )
    {
        blocks_list[i] = -1;
    }

    blocks_to_list( block_count, temp_blocks, blocks_list, FALSE );
    for ( i=0;
            i<block_max_id*2;
            i++ )
    {
        if ( blocks_list[i]<0 ) break;
        else
            list_count++;

    }

    status = list_to_blocks( list_count, blocks_list, &pib, &num_blocks_total );
    *total_blocks = num_blocks_total;

    free( blocks_list );
    free( temp_blocks );
    free( block_dup_check );

    *blocks = pib;
    return class_ptr;
}


/************************************************************
 * TAG( assemble_compare_blocks )
 *
 * Used by the qsort routine to sort block indexes
 */

int
assemble_compare_blocks( int *block1, int *block2 )
{
    if ( *block1 < *block2 )
        return -1;

    if ( *block1 > *block2 )
        return 1;

    return ( 0 );
}


/*****************************************************************
 * TAG( wrt_text )
 *
 * Write a formatted string to a scrolled text widget if GUI is
 * active, else to stderr. (Essentially "wprint" from O'Reilly's
 * "Motif Programming Manual".)
 */
/*STDARG*/
void
/* wrt_text( va_alist ) */
wrt_text( char *fmt, ... )
/* va_dcl */
{
    char msgbuf[BUFSIZ]; /* we're not getting huge strings */
    /*    char *fmt; */
    va_list args;

    va_start( args, fmt );
    /*    fmt = va_arg( args, char * ); */
#ifndef NO_VPRINTF
    (void) vsprintf( msgbuf, fmt, args );
#else /* !NO_VPRINTF */
    {
        FILE foo;
        foo._cnt = BUFSIZ;
        foo._base = foo._ptr = msgbuf; /* (unsigned char *) ?? */
        foo._flag = _IOWRT+_IOSTRG;
        (void) _doprnt( fmt, args, &foo );
        *foo._ptr = '\0'; /* plant terminating null character */
    }
#endif /* NO_VPRINTF */
    va_end( args );

    if ( gui_up )
    {
        XmTextInsert( monitor_widg, wpr_position, msgbuf );
        wpr_position += strlen( msgbuf );
        XtVaSetValues( monitor_widg, XmNcursorPosition, wpr_position, NULL );

        /* XtVaSetValues( monitor_widg,  XmNforeground,  env.dialog_text_color, NULL ); */

        XmTextShowPosition( monitor_widg, wpr_position );
    }
    else
        fprintf( stderr, msgbuf );

    if ( text_history_invoked )
        fprintf( text_history_file, "%s", msgbuf );
}


/* ARGSUSED 2 */
/*****************************************************************
 * TAG( remove_widget_CB )
 *
 * Callback to remove a widget's handle from a list and delete
 * the widget.
 */
static void
remove_widget_CB( Widget w, XtPointer client_data, XtPointer reason )
{
    Widget_list_obj *p_wlo;

    p_wlo = (Widget_list_obj *) client_data;

    XtDestroyWidget( w );
    DELETE( p_wlo, popup_dialog_list );
    popup_dialog_count--;
}


/*****************************************************************
 * TAG( popup_dialog )
 *
 * Popup a warning dialog with the error message.
 */
/*STDARG*/
void
/* popup_dialog( va_alist ) */
popup_dialog( int dtype, ... )
/* va_dcl */
{
    va_list vargs;
    char msgbuf[BUFSIZ]; /* we're not getting huge strings */
    char *fmt;
    /*    Popup_Dialog_Type dtype; */

#ifdef SERIAL_BATCH
#else

    Widget_list_obj *p_wlo;
    XmString dialog_string;
    int n;
    Arg args[5];
    static Position first_popup_x, first_popup_y;
    static int x_incr = 20;
    static int y_incr = 20;

#endif /* SERIAL_BATCH */

    char dialog_msg[BUFSIZ];


    /*
     * Process the arguments. "dtype" will always be the first argument,
     * so when converting to ANSI C and stdargs, it should be listed
     * explicitly in the argument list as the first argument.
     */

    /*    va_start( vargs ); */
    va_start( vargs, dtype );
    /*    dtype = (Popup_Dialog_Type) va_arg( vargs, int ); */
    fmt = va_arg( vargs, char * );
#ifndef NO_VPRINTF
    (void) vsprintf( msgbuf, fmt, vargs );
#else /* !NO_VPRINTF */
    {
        FILE foo;
        foo._cnt = BUFSIZ;
        foo._base = foo._ptr = msgbuf; /* (unsigned char *) ?? */
        foo._flag = _IOWRT+_IOSTRG;
        (void) _doprnt( fmt, vargs, &foo );
        *foo._ptr = '\0'; /* plant terminating null character */
    }
#endif /* NO_VPRINTF */
    va_end( vargs );

    /* Create an appropriate text body for the dialog. */
    switch ( dtype )
    {
    case INFO_POPUP:
        sprintf( dialog_msg, "%s", msgbuf );
        break;
    case USAGE_POPUP:
        sprintf( dialog_msg, "Usage:\n%s", msgbuf );
        break;
    case WARNING_POPUP:
        sprintf( dialog_msg, "WARNING:\n%s", msgbuf );
        break;
    }

    if ( !gui_up )
    {
        /*
         * If in SERIAL_BATCH mode then code should branch here.
         */

        fprintf( stderr, "%s\n", dialog_msg );
        return;
    }

    if ( !env.show_dialog )
    {
        wrt_text( "\n" );
        wrt_text( "\n**************** Notice *******************\n" );
        wrt_text( dialog_msg );
        wrt_text( "\n**************** Notice *******************\n" );
        return;
    }


#ifdef SERIAL_BATCH
#else

    /*
     * Limit the number of dialogs to a sane amount and divert
     * any additional warnings to the monitor window.
     */
    if ( popup_dialog_count >= MAX_WARNING_DIALOGS )
    {
        wrt_text( "Warning dialog limit reached.\n" );
        wrt_text( "%s\n", dialog_msg );
        return;
    }
    else if ( popup_dialog_count == 0 )
    {
        /* Position popup(s) relative to control window. */

        Position ctl_x, ctl_y;
        Dimension ctl_width, ctl_height;

        XtVaGetValues( ctl_shell_widg,
                       XmNx, &ctl_x,
                       XmNy, &ctl_y,
                       XmNwidth, &ctl_width,
                       XmNheight, &ctl_height,
                       NULL );

        first_popup_x = ctl_x + ctl_width / 2 - 100;
        first_popup_y = ctl_y + ctl_height / 2 - 100;
    }

    /* Allocate list node to hold popup dialog handle. */
    p_wlo = NEW( Widget_list_obj, "Warning popup node" );

    dialog_string = XmStringCreateLtoR( dialog_msg,
                                        XmSTRING_DEFAULT_CHARSET );

    /* Create the widget. */
    n = 0;
    XtSetArg( args[n], XmNmessageString, dialog_string );
    n++;
    XtSetArg( args[n], XmNx, first_popup_x + popup_dialog_count * x_incr );
    n++;
    XtSetArg( args[n], XmNy, first_popup_y + popup_dialog_count * y_incr );
    n++;
    p_wlo->handle = ( dtype == WARNING_POPUP )
                    ? XmCreateWarningDialog( ctl_shell_widg, "Warning", args, n )
                    : XmCreateInformationDialog( ctl_shell_widg, "Information",
                            args, n );

    /* ...make it look and act right */
    XtAddCallback( p_wlo->handle, XmNokCallback,
                   (XtCallbackProc) remove_widget_CB, p_wlo );

    XtUnmanageChild( XmMessageBoxGetChild( p_wlo->handle,
                                           XmDIALOG_CANCEL_BUTTON ));
    XtUnmanageChild( XmMessageBoxGetChild( p_wlo->handle,
                                           XmDIALOG_HELP_BUTTON ));

    XmStringFree( dialog_string );

    /* Put it on the list. */
    APPEND( p_wlo, popup_dialog_list );
    popup_dialog_count++;

    /* Show it. */
    XtManageChild( p_wlo->handle );

    XFlush( XtDisplay( ctl_shell_widg ) );
#endif /* SERIAL_BATCH */

}


/*****************************************************************
 * TAG( clear_popup_dialogs )
 *
 * Destroy any extant popup dialogs referenced in the popup list.
 */
void
clear_popup_dialogs( void )
{
    Widget_list_obj *p_wlo, *p_wlo2;

    p_wlo = popup_dialog_list;

    while ( p_wlo != NULL )
    {
        p_wlo2 = p_wlo->next;

        XtDestroyWidget( p_wlo->handle );
        DELETE( p_wlo, popup_dialog_list );
        popup_dialog_count--;

        p_wlo = p_wlo2;
    }
}


/*****************************************************************
 * TAG( popup_fatal )
 *
 * Popup an error dialog with the error message, then exit the
 * program.
 */
void
popup_fatal( char *message )
{
#ifdef SERIAL_BATCH
#else

    Widget error_dialog, dialog_shell;
    XmString error_string, ok_label;
    int n;
    Arg args[5];
    Atom WM_DELETE_WINDOW;

#endif /* SERIAL_BATCH */

    char error_msg[128];


    sprintf( error_msg, "FATAL ERROR:\n%s", message );

    if ( !gui_up )
    {
        fprintf( stderr, "%s\n", error_msg );
        quit( -1 );
    }


#ifdef SERIAL_BATCH
#else

    /* Keep any sneaky workproc's from sliding by...*/
    end_animate_workproc( TRUE );

    error_string = XmStringCreateLtoR( error_msg, XmSTRING_DEFAULT_CHARSET );

    ok_label = XmStringCreateSimple( "Exit" );

    n = 0;
    XtSetArg( args[n], XmNmessageString, error_string );
    n++;
    XtSetArg( args[n], XmNokLabelString, ok_label );
    n++;
    XtSetArg( args[n], XmNdeleteResponse, XmDO_NOTHING );
    n++;
    error_dialog = XmCreateErrorDialog( ctl_shell_widg, "error", args, n );
    XtAddCallback( error_dialog, XmNokCallback, (XtCallbackProc) exit_CB,
                   NULL );

    /*
     * Link the window manager "close" function to the exit callback to
     * prevent the user from using the window manager to close the popup
     * and then trying to continue with GRIZ.
     */
    dialog_shell = XtParent( error_dialog );
    WM_DELETE_WINDOW = XmInternAtom( XtDisplay( ctl_shell_widg ),
                                     "WM_DELETE_WINDOW", FALSE );
    XmAddWMProtocolCallback( dialog_shell, WM_DELETE_WINDOW,
                             (XtCallbackProc) exit_CB,
                             (XtPointer)error_dialog );

    XtUnmanageChild(
        XmMessageBoxGetChild( error_dialog, XmDIALOG_CANCEL_BUTTON ) );
    XtUnmanageChild(
        XmMessageBoxGetChild( error_dialog, XmDIALOG_HELP_BUTTON ) );

    XmStringFree( error_string );
    XmStringFree( ok_label );
    XtAddGrab( XtParent( error_dialog ), TRUE, FALSE );
    XtManageChild( error_dialog );

#endif /* SERIAL_BATCH */
}


/******************************************************************
 * TAG( reset_window_titles )
 *
 * Reset the title to reflect a new database.
 */
void
reset_window_titles( void )
{
    char title[100];
    Analysis *p_analy;
    p_analy = get_analy_ptr();

    init_griz_name( p_analy );

    if ( env.bname )
        sprintf( title, "Control:  %s%s", path_string, env.bname );
    else
        sprintf( title, "Control:  %s%s", path_string, env.plotfile_name );
    XtVaSetValues( ctl_shell_widg, XmNtitle, title, NULL );

    if ( env.bname )
        sprintf( title, "Render:  %s%s", path_string, env.bname );
    else
        sprintf( title, "Render:  %s%s", path_string, env.plotfile_name );
    XtVaSetValues( rendershell_widg, XmNtitle, title, NULL );

    if ( util_panel_widg )
    {
        if ( env.bname )
            sprintf( title, "Utility Panel: %s", env.bname );
        else
            sprintf( title, "Utility Panel: %s", env.plotfile_name );
        XtVaSetValues( util_panel_widg, XmNtitle, title, NULL );
    }

    if ( mtl_mgr_widg )
    {
        if ( env.bname )
            sprintf( title, "Material Manager: %s", env.bname );
        else
            sprintf( title, "Material Manager: %s", env.plotfile_name );
        XtVaSetValues( mtl_mgr_widg, XmNtitle, title, NULL );
    }
}


/*****************************************************************
 * TAG( action_quit )
 *
 * Action routine to execute quit().
 */
static void
action_quit( Widget w, XEvent *event, String params[], int *qty )
{
    write_log_message( );
    quit( 0 );
}


/*****************************************************************
 * TAG( quit )
 *
 * Quit the application.
 */
void
quit( int return_code )
{
    int i=1;

    (void) signal( SIGFPE, x11_signal );

    close_history_file();
    close_analysis( env.curr_analy );
    env.curr_analy->db_close( env.curr_analy );

    if ( return_code !=0 )
        printf("\n\n Griz Completed with return code = %d\n", return_code);
    else
        printf("\n\n ** Griz Completed Normally **\n");

#ifdef TIME_GRIZ
    manage_timer( 8, 1 );
#endif

#ifdef SERIAL_BATCH
    exit(1);
#else
    /* XtDestroyWidget( ctl_shell_widg ); */
    exit( -1 );
#endif
}


/*****************************************************************
 * TAG( switch_opengl_win )
 *
 * Switch among OpenGL rendering windows and their contexts.
 */
void
switch_opengl_win( OpenGL_win opengl_win )
{
    switch ( opengl_win )
    {
    case MESH_VIEW:
        if ( cur_opengl_win != MESH_VIEW )
        {
            glXMakeCurrent( dpy, XtWindow( ogl_widg[MESH_VIEW] ),
                            render_ctx );
            cur_opengl_win = MESH_VIEW;
        }
        break;

    case SWATCH:
        if ( cur_opengl_win != SWATCH )
        {
            glXMakeCurrent( dpy, XtWindow( ogl_widg[SWATCH] ), swatch_ctx );
            cur_opengl_win = SWATCH;
        }
        break;

    default:
        popup_dialog( WARNING_POPUP,
                      "Attempt to make invalid OpenGL window current." );
        glXMakeCurrent( dpy, XtWindow( ogl_widg[MESH_VIEW] ), render_ctx);
        cur_opengl_win = MESH_VIEW;
    }
}


/*****************************************************************
 * TAG( init_alt_cursors )
 *
 * Define alternative cursor resources.
 */
static void
init_alt_cursors( void )
{
    alt_cursors[CURSOR_WATCH] = XCreateFontCursor( dpy, XC_watch );
    alt_cursors[CURSOR_EXCHANGE] = XCreateFontCursor( dpy, XC_exchange );
    alt_cursors[CURSOR_FLEUR] = XCreateFontCursor( dpy, XC_fleur );
}


/*****************************************************************
 * TAG( set_alt_cursor )
 *
 * Switch to an alternative cursor.
 */
void
set_alt_cursor( Cursor_type cursor_type )
{
#ifdef SERIAL_BATCH
    popup_fatal( "set_alt_cursor:  Attempt to execute procedure in batch mode." );
#else

    XDefineCursor( dpy, XtWindow( ogl_widg[MESH_VIEW] ),
                   alt_cursors[cursor_type] );
    XDefineCursor( dpy, XtWindow( ctl_shell_widg ), alt_cursors[cursor_type] );

    /* Also set for Material Manager and (standalone) Utility Panel. */
    if ( mtl_mgr_widg != NULL )
        XDefineCursor( dpy, XtWindow( ctl_shell_widg ),
                       alt_cursors[cursor_type] );
    if ( surf_mgr_widg != NULL )
        XDefineCursor( dpy, XtWindow( ctl_shell_widg ),
                       alt_cursors[cursor_type] );
    if ( !include_util_panel && util_panel_widg != NULL )
        XDefineCursor( dpy, XtWindow( util_panel_widg ),
                       alt_cursors[cursor_type] );

    XFlush( dpy );

#endif /* SERIAL_BATCH */
}


/*****************************************************************
 * TAG( unset_alt_cursor )
 *
 * Set cursor to default.
 */
void
unset_alt_cursor( void )
{
#ifdef SERIAL_BATCH
    popup_fatal( "unset_alt_cursor:  Attempt to execute procedure in batch mode." );
#else

    XUndefineCursor( dpy, XtWindow( ogl_widg[MESH_VIEW] ) );
    XUndefineCursor( dpy, XtWindow( ctl_shell_widg ) );

    /* Also set for Material Manager and (standalone) Utility Panel. */
    if ( mtl_mgr_widg != NULL )
        XUndefineCursor( dpy, XtWindow( ctl_shell_widg ) );
    if ( surf_mgr_widg != NULL )
        XUndefineCursor( dpy, XtWindow( ctl_shell_widg ) );
    if ( !include_util_panel && util_panel_widg != NULL )
        XUndefineCursor( dpy, XtWindow( util_panel_widg ) );

    XFlush( dpy );

#endif /* SERIAL_BATCH */
}


/*****************************************************************
 * TAG( set_motion_threshold )
 *
 * Set the threshold distance that controls when small mouse
 * motions become identified as intentional motion activity.
 */
void
set_motion_threshold( float threshold )
{
    motion_threshold = threshold;
}


/*****************************************************************
 * TAG( pushpop_window )
 *
 * Capture the position and size for all of the windows that are
 * active.
 *
 */
void
pushpop_window( PushPop_type direction )
{
    XWindowChanges xwc;

    if ( direction == PUSHPOP_ABOVE )
        xwc.stack_mode = Above;
    else
        xwc.stack_mode = Below;

    if ( util_panel_widg != NULL )
    {
        XConfigureWindow( dpy, XtWindow( util_panel_widg ), CWStackMode, &xwc );
    }

    if ( mtl_mgr_widg != NULL && mtl_mgr_top_win != 0 )
    {
        XConfigureWindow( dpy, XtWindow( mtl_mgr_widg ),    CWStackMode, &xwc );
    }

    if ( surf_mgr_widg != NULL && surf_mgr_top_win != 0 )
    {
        XConfigureWindow( dpy, XtWindow( surf_mgr_widg ),    CWStackMode, &xwc );
    }

    if ( rendershell_widg != NULL && direction == PUSHPOP_ABOVE )
    {
        XConfigureWindow( dpy, XtWindow( rendershell_widg ), CWStackMode, &xwc );
    }

    if ( ctl_shell_widg != NULL && ctl_top_win != 0 )
    {
        XConfigureWindow( dpy, XtWindow( ctl_shell_widg ), CWStackMode, &xwc );
        XGetWindowAttributes( dpy, XtWindow( ctl_shell_widg ), &ctl_attrib );
    }
}


/*****************************************************************
 * TAG( get_window_attributes )
 *
 * Capture the position and size for all of the windows that are
 * active.
 *
 */
void
get_window_attributes( void )
{
    XWindowAttributes win_attrib;

    if ( rendershell_widg != NULL )
    {
        XGetWindowAttributes( dpy, render_top_win, &win_attrib );
        session->win_render_size[0] = win_attrib.height;
        session->win_render_size[1] = win_attrib.width;
        session->win_render_pos[0]  = win_attrib.x;
        session->win_render_pos[1]  = win_attrib.y;
    }

    if ( ctl_shell_widg != NULL && ctl_top_win != 0 )
    {
        XGetWindowAttributes( dpy, ctl_top_win, &win_attrib );
        session->win_ctl_size[0] = win_attrib.height;
        session->win_ctl_size[1] = win_attrib.width;
        session->win_ctl_pos[0]  = win_attrib.x;
        session->win_ctl_pos[1]  = win_attrib.y;
    }

    if ( util_panel_widg != NULL && !include_util_panel )
    {
        XGetWindowAttributes( dpy, util_panel_top_win, &win_attrib );
        session->win_util_size[0] = win_attrib.height;
        session->win_util_size[1] = win_attrib.width;
        session->win_util_pos[0]  = win_attrib.x;
        session->win_util_pos[1]  = win_attrib.y;

    }

    if ( mtl_mgr_widg != NULL && mtl_mgr_top_win != 0  && !include_mtl_panel )
    {
        XGetWindowAttributes( dpy, mtl_mgr_top_win, &win_attrib );
        session->win_mtl_size[0] = win_attrib.height;
        session->win_mtl_size[1] = win_attrib.width;
        session->win_mtl_pos[0]  = win_attrib.x;
        session->win_mtl_pos[1]  = win_attrib.y;
    }

    if ( surf_mgr_widg != NULL && surf_mgr_top_win != 0 )
    {
        XGetWindowAttributes( dpy, surf_mgr_top_win, &win_attrib );
        session->win_surf_size[0] = win_attrib.height;
        session->win_surf_size[1] = win_attrib.width;
        session->win_surf_pos[0]  = win_attrib.x;
        session->win_surf_pos[1]  = win_attrib.y;
    }
}

/*****************************************************************
 * TAG( put_window_attributes)
 *
 * Update windows with new attributes from the session data
 * structure..
 */
void
put_window_attributes( void )
{
    XWindowChanges xwc;

    if ( rendershell_widg != NULL && session->win_render_size[0] > 1 && !env.window_size_set_on_command_line )
    {
        xwc.height = session->win_render_size[0];
        xwc.width  = session->win_render_size[1];
        xwc.x      = session->win_render_pos[0];
        xwc.y      = session->win_render_pos[1];

        XMoveResizeWindow( dpy, XtWindow( rendershell_widg ), xwc.x, xwc.y, (unsigned int) xwc.width, (unsigned int) xwc.height );
    }

    if ( ctl_shell_widg != NULL && session->win_ctl_size[0] > 1 )
    {
        if ( ctl_top_win == 0 )
            find_ancestral_root_child( ctl_shell_widg, &ctl_top_win );

        xwc.height = session->win_ctl_size[0];
        xwc.width  = session->win_ctl_size[1];
        xwc.x      = session->win_ctl_pos[0];
        xwc.y      = session->win_ctl_pos[1];

        XMoveResizeWindow( dpy, XtWindow( ctl_shell_widg ), xwc.x, xwc.y, (unsigned int) xwc.width, (unsigned int) xwc.height );
    }

    if ( util_panel_widg != NULL && !include_util_panel &&
            session->win_util_size[0] > 1 )
    {
        xwc.height = session->win_util_size[0];
        xwc.width  = session->win_util_size[1];
        xwc.x      = session->win_util_pos[0];
        xwc.y      = session->win_util_pos[1];

        XMoveResizeWindow( dpy, XtWindow( util_panel_widg ), xwc.x, xwc.y, (unsigned int) xwc.width, (unsigned int) xwc.height );
    }

    if ( mtl_mgr_widg != NULL && !include_mtl_panel &&
            session->win_mtl_size[0] > 1 )
    {
        xwc.height = session->win_mtl_size[0];
        xwc.width  = session->win_mtl_size[1];
        xwc.x      = session->win_mtl_pos[0];
        xwc.y      = session->win_mtl_pos[1];

        XMoveResizeWindow( dpy, XtWindow( mtl_mgr_widg ), xwc.x, xwc.y, (unsigned int) xwc.width, (unsigned int) xwc.height );
    }

    if ( surf_mgr_widg != NULL && session->win_surf_size[0] > 1 )
    {
        xwc.height = session->win_surf_size[0];
        xwc.width  = session->win_surf_size[1];
        xwc.x      = session->win_surf_pos[0];
        xwc.y      = session->win_surf_pos[1];

        XMoveResizeWindow( dpy, XtWindow( surf_mgr_widg ), xwc.x, xwc.y, (unsigned int) xwc.width, (unsigned int) xwc.height );
    }
}


/*****************************************************************
 * TAG( write_history_text )
 *
 *
 * This function will write a line of history text to the command
 * window and the text will be hilited in color if hilite_text
 *
 */
void
write_history_text( char * command, Bool_type hilite_text )
{
    XmString text = NULL;
    Widget hist_list;

#ifndef IRIX
    text = XmStringGenerate( command,
                             NULL,
                             XmCHARSET_TEXT,
                             NULL) ;
#endif

    if( text != NULL )
    {
        hist_list=XmCommandGetChild(command_widg, XmDIALOG_HISTORY_LIST);
        XmListAddItem( hist_list, text, 0 );
        XmListSetBottomPos( hist_list, 0 );
        XmStringFree( text );
    }
}

/*****************************************************************
 * TAG( defineBorderColor )
 *
 * This function will set some predefined border colors for all
 * Griz windows in a session.
 *
 */
void defineBorderColor( Display* dpy )
{

    Colormap cmap;

    XColor color, dummy;

    cmap = DefaultColormap( dpy, 0 );

    XAllocNamedColor( dpy, cmap, "bisque", &color, &dummy );
    env.border_colors[0] = color.pixel;
    XAllocNamedColor( dpy, cmap, "blue", &color, &dummy );
    env.border_colors[1] = color.pixel;
    XAllocNamedColor( dpy, cmap, "lime green", &color, &dummy );
    env.border_colors[2] = color.pixel;
    XAllocNamedColor( dpy, cmap, "gold", &color, &dummy );
    env.border_colors[3] = color.pixel;
    XAllocNamedColor( dpy, cmap, "light salmon", &color, &dummy );
    env.border_colors[4] = color.pixel;
    XAllocNamedColor( dpy, cmap, "purple", &color, &dummy );
    env.border_colors[5] = color.pixel;
    XAllocNamedColor( dpy, cmap, "brown", &color, &dummy );
    env.border_colors[6] = color.pixel;
    XAllocNamedColor( dpy, cmap, "coral", &color, &dummy );
    env.border_colors[7] = color.pixel;
    XAllocNamedColor( dpy, cmap, "olive", &color, &dummy );
    env.border_colors[8] = color.pixel;
    XAllocNamedColor( dpy, cmap, "yellow", &color, &dummy );
    env.border_colors[9] = color.pixel;
}

/*****************************************************************
 * TAG( defineDialogColor )
 *
 * This function will set a predefined border colors for all
 * Griz text dislog info/warning messages.
 *
 */
void defineDialogColor( Display* dpy )
{
    Colormap cmap;

    XColor color, dummy;

    cmap = DefaultColormap( dpy, 0 );

    XAllocNamedColor( dpy, cmap, "red", &color, &dummy );
    env.dialog_text_color = color.pixel;
}


/*****************************************************************
 * TAG( popUpAllWindows )
 *
 * This function will set some predefined border colors for all
 * Griz windows in a session.
 *
 */
void popUpAllWindows( Display* dpy )
{

    XRaiseWindow( dpy, render_top_win );

    if ( ctl_shell_widg != NULL && ctl_top_win !=0 )
    {
        XRaiseWindow( dpy, ctl_top_win );
    }

    if ( util_panel_widg != NULL && util_panel_top_win != 0 )
    {
        XRaiseWindow( dpy, util_panel_top_win );
    }

    if ( mtl_mgr_widg != NULL && mtl_mgr_top_win != 0 )
    {
        XRaiseWindow( dpy, mtl_mgr_top_win );
    }
}

/*****************************************************************
 * TAG( init_griz_name )
 *
 * Updates griz name and path info.
 *
 */
void
init_griz_name( Analysis *analy )
{
    size_t namlen;
    char *name = "GRIZ ";
    char beta_release[40];

    strcpy( path_string, "[" );
    strcat( path_string, analy->root_name );
    strcat( path_string, "] " );
    strcat( path_string, analy->path_name );

    /* Initialize the name string. */

    /* Check for Alpha or Beta release of the code and
     * build appropriate info message.
     */
    beta_release[0] = '\0';

    if (env.run_alpha_version != 0.)
        strcpy(beta_release, "** Alpha Release **");
    if (env.run_beta_version != 0.)
        strcpy(beta_release, "** Beta Release **");

    if ( griz_name==NULL )
    {
        namlen = strlen( name ) + strlen( griz_version ) + strlen( beta_release ) + 1;
        griz_name = NEW_N( char, namlen, "Griz name string" );
        sprintf( griz_name, "%s%s%s", name, griz_version, beta_release );
    }
}
