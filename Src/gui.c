/* $Id$ */
/* 
 * gui.c - Graphical user interface routines.
 * 
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Apr 27 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include <string.h>
#include <varargs.h>
#include <sys/types.h>
#include <sys/stat.h>

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
/* Use this for Irix 4.x and 5.1. */
/* #include <X11/GLw/GLwMDrawA.h> */
/* This is the directory (mistake) for Irix 5.2. */
#include <GL/GLwMDrawA.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include "viewer.h"
#include "draw.h"
/* Bitmaps. */
#include "Bitmaps/GrizCheck"
#include "Bitmaps/GrizStart"
#include "Bitmaps/GrizStop"
#include "Bitmaps/GrizLeftStop"
#include "Bitmaps/GrizLeft"
#include "Bitmaps/GrizRight"
#include "Bitmaps/GrizRightStop"

extern char* getenv();

/* 
 * Global boolean to allow utility panel as part of control window.
 * Set indirectly from command line.
 */
Bool_type include_util_panel = FALSE;

/* Griz name & version for window titles. */
static char *griz_name;

/* Local routines. */
static void init_gui();
static void expose_CB();
static void exposeoverlay_CB();
static void resize_CB();
static void res_menu_CB();
static void menu_CB();
static void input_CB();
static void parse_CB();
static void exit_CB();
static void destroy_mtl_mgr_CB();
static void destroy_util_panel_CB();
static void mtl_func_select_CB();
static void mtl_quick_select_CB();
static void mtl_select_CB();
static void mtl_func_operate_CB();
static void col_comp_disarm_CB();
static void col_ed_scale_CB();
static void col_ed_scale_update_CB();
static void expose_swatch_CB();
static void init_swatch_CB();
static void util_render_CB();
static void step_CB();
static void stride_CB();
static void create_app_widg();
static Widget create_menu_bar();
static Widget create_mtl_manager();
static Widget create_free_util_panel();
static Widget create_utility_panel();
static void send_mtl_cmd();
static int load_mtl_mgr_funcs();
static int load_selected_mtls();
static int load_mtl_properties();
static void select_mtl_mgr_mtl();
static void set_rgb_scales();
static void set_shininess_scale();
static void set_scales_to_mtl();
static void update_actions_sens();
static void update_swatch_label();
static Bool_type mtl_func_active();
static void action_create_app_widg();
static void resize_mtl_scrollwin();
static void gress_mtl_mgr();
static void view_mgr_resize();
static void string_convert();
static Boolean animate_workproc_CB();
static void write_start_text();
static void remove_widget_CB();
static void init_alt_cursors();


/*
 * This resource list provides defaults for settable values in the
 * interface widgets.
 */
String fallback_resources[] = {
    "*fontList: -*-screen-medium-*-*-*-13-*-*-*-*-*-*-*",
    "*background: grey", 
    "*monitor*height: 130",
    "*monitor*columns: 60",
    NULL
};


/*
 * Menu button IDs.
 */
typedef enum
{
    BTN_COPYRIGHT,
    BTN_MTL_MGR, 
    BTN_UTIL_PANEL, 
    BTN_QUIT,

    BTN_DRAWFILLED,
    BTN_DRAWHIDDEN,
    BTN_COORDSYS,
    BTN_TITLE,
    BTN_TIME,
    BTN_COLORMAP,
    BTN_MINMAX, 
    BTN_ALLON,
    BTN_ALLOFF,
    BTN_BBOX,
    BTN_EDGES,
    BTN_PERSPECTIVE,
    BTN_ORTHOGRAPHIC,
    BTN_ADJUSTNF,
    BTN_RESETVIEW,

    BTN_HILITE,
    BTN_SELECT,
    BTN_CLEARHILITE,
    BTN_CLEARSELECT,
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

    BTN_TIMEPLOT
} Btn_type;

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
    VAL_HEX_SIG_PD1,       /* 1st prinicipal deviatoric stress */
    VAL_HEX_SIG_PD2,       /* 2nd prinicipal deviatoric stress */
    VAL_HEX_SIG_PD3,       /* 3rd prinicipal deviatoric stress */
    VAL_HEX_SIG_MAX_SHEAR, /* Maximum shear stress */
    VAL_HEX_SIG_P1,        /* 1st principle stress */
    VAL_HEX_SIG_P2,        /* 2nd principle stress */
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
    VAL_ALL_END
};

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
    ALL, 
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

/* OpenGL windows. */
typedef enum
{
    MESH, 
    SWATCH, 
    NO_OGL_WIN /* Always last! */
} OpenGL_win;

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


/* Mouse modes. */
#define MOUSE_STATIC 0
#define MOUSE_MOVE 1

/* Some handles that need to stay around. */
static XtAppContext app_context;
static Display *dpy;
static Widget ctl_shell_widg = NULL;
static Widget rendershell_widg = NULL;
static Widget monitor_widg = NULL;
static Widget mtl_mgr_widg = NULL;
static Widget mtl_mgr_button = NULL;
static Widget util_button = NULL;
static Widget mtl_mgr_func_toggles[MTL_FUNC_QTY];
static Widget util_panel_widg = NULL;
static Widget mtl_row_col = NULL;
static Widget quick_select = NULL;
static Widget color_editor = NULL;
static Widget color_comps[EMISSIVE + 1];
static Widget prop_checks[MTL_PROP_QTY];
static Widget col_ed_scales[2][4];
static Widget swatch_label = NULL;
static Widget op_buttons[MTL_OP_QTY];
static Widget swatch_frame = NULL;
static Widget util_render_ctl = NULL;
static Widget *util_render_btns = NULL;
static Widget stride_label = NULL;

/* Translation specification for "global" translations. */
static char action_spec[256];

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

/* Current mtl manager material. */
static int cur_mtl;

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

/* Pixmaps. */
static Pixmap check, pixmap_start, pixmap_stop, pixmap_leftstop, 
    pixmap_left, pixmap_right, pixmap_rightstop;

/* Lists for select and deselected materials for material manager. */
static Material_list_obj *mtl_select_list = NULL;
static Material_list_obj *mtl_deselect_list = NULL;

/* Stride value for state stepping buttons of Utility panel. */
static int step_stride = 1;

/* Saved rendering mode from "Edges only" transition in Utility panel. */
static Render_mode_type prev_render_mode;

/* Indicator of Utility Panel "Edges Only" callback activity. */
static Bool_type util_panel_CB_active = FALSE;

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
                            GLX_BLUE_SIZE, 1, GLX_STENCIL_SIZE, 1, None };
static int double_buf[] = { GLX_RGBA, GLX_DEPTH_SIZE, 1, GLX_DOUBLEBUFFER,
                            GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1,
                            GLX_BLUE_SIZE, 1, GLX_STENCIL_SIZE, 1, None };
static int single_buf_no_z[] = { GLX_RGBA, GLX_RED_SIZE, 1,
                                 GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
                                 GLX_STENCIL_SIZE, 1, None };
static int double_buf_no_z[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1,
                                 GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
                                 GLX_STENCIL_SIZE, 1, None };

/*
 * ID for the animation workproc.
 */
static XtWorkProcId anim_work_proc_id = 0;
static Boolean stop_animate;


/*****************************************************************
 * TAG( gui_start )
 *
 * Open the gui main windows and initialize everything, then start
 * the gui event loop.
 */
void
gui_start( argc, argv )
int argc;
char **argv;
{
    Widget mainwin_widg, menu_bar, pane, control, command_widg;
    XmString command_label;
    Arg args[20];
    char title[100];
    int n;
    char *name = "GRIZ ";
    XtActionsRec global_actions;
    
    /* Initialize the name string. */
    n = strlen( name ) + strlen( griz_version ) + 1;
    griz_name = NEW_N( char, n, "Griz name string" );
    sprintf( griz_name, "%s%s", name, griz_version );

    /*
     * Create an application shell to hold all of the interface except
     * the rendering window.
     */

    sprintf( title, "%s Control:  %s\n", griz_name, env.plotfile_name );
    n = 0;
    XtSetArg( args[n], XmNiconic, FALSE ); n++;
    XtSetArg( args[n], XmNiconName, "GRIZ" ); n++;
    XtSetArg( args[n], XmNtitle, title ); n++;
    XtSetArg( args[n], XmNallowShellResize, TRUE ); n++;
    ctl_shell_widg = XtAppInitialize( &app_context, "GRIZ",
                                      (XrmOptionDescList)NULL , 0,
                                      (Cardinal*)&argc,
                                      (String*)argv,
                                      fallback_resources,
                                      args, n );

    /* Record the display for use elsewhere. */
    dpy = XtDisplay( ctl_shell_widg );
    
    n = 0;
    mainwin_widg = XtCreateManagedWidget( "mainw",
                                          xmMainWindowWidgetClass,
                                          ctl_shell_widg, args, n );

    menu_bar = create_menu_bar( mainwin_widg );

    /*
     * Everything else for the control window goes onto a pane.
     */

    n = 0;
    XtSetArg( args[n], XmNallowResize, TRUE ); n++;
    pane = XmCreatePanedWindow( mainwin_widg, "pane", args, n );

    n = 0;
    XtSetArg( args[n], XmNeditable, FALSE ); n++;
    XtSetArg( args[n], XmNeditMode, XmMULTI_LINE_EDIT ); n++;
    XtSetArg( args[n], XmNautoShowCursorPosition, FALSE ); n++;
    XtSetArg( args[n], XmNscrollingPolicy, XmAUTOMATIC ); n++;
    monitor_widg = XmCreateScrolledText( pane, "monitor", args, n );
    XtManageChild( monitor_widg );
    
    if ( include_util_panel )
        util_panel_widg = create_utility_panel( pane );

    command_label = XmStringCreateLocalized( "Command:" );
    n = 0;
    /* XtSetArg( args[n], XmNheight, 150 ); n++; */
    XtSetArg( args[n], XmNhistoryVisibleItemCount, 3 ); n++;
    XtSetArg( args[n], XmNselectionLabelString, command_label ); n++;
    command_widg = XmCreateCommand( pane, "command", args, n );
    XtAddCallback( command_widg, XmNcommandEnteredCallback, parse_CB, NULL );
    XtManageChild( command_widg );
    XtManageChild( pane );

    /* Set initial keyboard focus to the command widget. */
    XtVaSetValues( pane, XmNinitialFocus, command_widg, NULL );
    
    XmMainWindowSetAreas( mainwin_widg, menu_bar, NULL, NULL, NULL, pane );

    /* The control widget gets realized down below. */

    /* Start-up notices go into the monitor window. */
    write_start_text();

    /*
     * Have the control window, now build the rendering window.
     */

    /* Create a topLevelShell for the rendering window. */
    sprintf( title, "%s Render:  %s\n", griz_name, env.plotfile_name );
    n = 0;
    XtSetArg( args[n], XtNtitle, title ); n++;
    XtSetArg( args[n], XmNiconic, FALSE ); n++;
    XtSetArg( args[n], XmNwindowGroup, XtWindow( mainwin_widg ) ); n++;
    XtSetArg( args[n], XmNdeleteResponse, XmDO_NOTHING ); n++;
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

    /* Create the OpenGL drawing widget. */
    ogl_widg[MESH] = XtVaCreateManagedWidget( 
        "glwidget", glwMDrawingAreaWidgetClass, rendershell_widg, 
        GLwNvisualInfo, vi,
        XmNwidth, window_width,
        XmNheight, window_height,
        NULL );
    XtAddCallback( ogl_widg[MESH], GLwNexposeCallback, expose_CB, 0 );
    XtAddCallback( ogl_widg[MESH], GLwNresizeCallback, resize_CB, 0 );
    XtAddCallback( ogl_widg[MESH], GLwNinputCallback, input_CB, 0 );
    XtManageChild( ogl_widg[MESH] );
    
    /* 
     * Define translations for creation of Material Manager and 
     * Utility Panel (if not part of Control window).
     */
    global_actions.string = "action_create_app_widg";
    global_actions.proc = action_create_app_widg;
    if ( !include_util_panel )
    {
        sprintf( action_spec, "Ctrl<Key>m: action_create_app_widg( %d ) \n ", 
	         BTN_MTL_MGR );
        sprintf( action_spec + strlen( action_spec ), 
	         "Ctrl<Key>u: action_create_app_widg( %d )", BTN_UTIL_PANEL );
    }
    else
        sprintf( action_spec, "Ctrl<Key>m: action_create_app_widg( %d )", 
	         BTN_MTL_MGR );
    XtAppAddActions( app_context, &global_actions, 1 );
    
    /* Add the creation translations to the rendering window. */
    XtOverrideTranslations( ogl_widg[MESH], 
        XtParseTranslationTable( action_spec ) );

    /* Bring up the control window last. */
    XtPopup( rendershell_widg, XtGrabNone );
    XtRealizeWidget( ctl_shell_widg );

    gui_up = TRUE;
    
    /* 
     * Bind the mesh rendering context and window.
     */
    switch_opengl_win( MESH );
    init_gui();
    
    init_alt_cursors();

    /* Start event processing. */
    XtAppMainLoop( app_context );
}


/*****************************************************************
 * TAG( gui_swap_buffers )
 *
 * Swap buffers in the current OpenGL window.
 */
void
gui_swap_buffers()
{
    GLwDrawingAreaSwapBuffers( ogl_widg[cur_opengl_win] );
}


/*****************************************************************
 * TAG( create_menu_bar )
 *
 * Create the menu bar for the main window.
 */
static Widget
create_menu_bar( parent )
Widget parent;
{
    Widget menu_bar;
    Widget cascade;
    Widget menu_pane;
    Widget submenu_pane;
    Widget button;
    Arg args[10];
    char *title, *com;
    int idx, i, n;
    XmString accel_str;

    n = 0;
    XtSetArg( args[n], XmNx, 0 ); n++;
    XtSetArg( args[n], XmNy, 0 ); n++;
    menu_bar = XmCreateMenuBar( parent, "menu_bar", args, n );
    XtManageChild( menu_bar );

    /* Control menu. */
    n = 0;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    button = XmCreatePushButtonGadget( menu_pane, "Copyright", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_COPYRIGHT );

    accel_str = XmStringCreateSimple( "Ctrl+m" );
    n = 0;
    XtSetArg( args[n], XmNaccelerator, "Ctrl<Key>m" ); n++;
    XtSetArg( args[n], XmNacceleratorText, accel_str ); n++;
    mtl_mgr_button = XmCreatePushButtonGadget( menu_pane, "Material Mgr", 
                                               args, n );
    XmStringFree( accel_str );
    XtManageChild( mtl_mgr_button );
    XtAddCallback( mtl_mgr_button, XmNactivateCallback, menu_CB, 
                   (XtPointer) BTN_MTL_MGR );

    /* If utility panel not part of control window, allow it from pulldown. */
    if ( !include_util_panel )
    {
        accel_str = XmStringCreateSimple( "Ctrl+u" );
        n = 0;
        XtSetArg( args[n], XmNaccelerator, "Ctrl<Key>u" ); n++;
        XtSetArg( args[n], XmNacceleratorText, accel_str ); n++;
        util_button = XmCreatePushButtonGadget( menu_pane, "Utility Panel", 
	                                        args, n );
        XmStringFree( accel_str );
        XtManageChild( util_button );
        XtAddCallback( util_button, XmNactivateCallback, menu_CB, 
	               (XtPointer) BTN_UTIL_PANEL );
    }

    accel_str = XmStringCreateSimple( "Ctrl+q" );
    n = 0;
    XtSetArg( args[n], XmNaccelerator, "Ctrl<Key>q" ); n++;
    XtSetArg( args[n], XmNacceleratorText, accel_str ); n++;
    button = XmCreatePushButtonGadget( menu_pane, "Quit", args, n );
    XmStringFree( accel_str );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_QUIT );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_bar, "Control", args, n );
    XtManageChild( cascade );

    /* Rendering menu. */
    n = 0;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );
 
    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Draw Solid", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_DRAWFILLED );
 
    button = XmCreatePushButtonGadget( menu_pane, "Draw Hidden", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_DRAWHIDDEN );
 
    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );
 
    button = XmCreatePushButtonGadget( menu_pane, "Coord Sys On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_COORDSYS );
 
    button = XmCreatePushButtonGadget( menu_pane, "Title On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_TITLE );

    button = XmCreatePushButtonGadget( menu_pane, "Time On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_TIME );
 
    button = XmCreatePushButtonGadget( menu_pane, "Colormap On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_COLORMAP );
 
    button = XmCreatePushButtonGadget( menu_pane, "Min/max On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_MINMAX );

    button = XmCreatePushButtonGadget( menu_pane, "All On", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_ALLON );

    button = XmCreatePushButtonGadget( menu_pane, "All Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_ALLOFF );

    button = XmCreatePushButtonGadget( menu_pane, "Bound Box On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_BBOX );

    button = XmCreatePushButtonGadget( menu_pane, "Edges On/Off", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_EDGES );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );
 
    button = XmCreatePushButtonGadget( menu_pane, "Perspective", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_PERSPECTIVE );
 
    button = XmCreatePushButtonGadget( menu_pane, "Orthographic", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_ORTHOGRAPHIC );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Adjust Near/Far", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_ADJUSTNF );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Reset View", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_RESETVIEW );
 
    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_bar, "Rendering", args, n );
    XtManageChild( cascade );

    /* Picking menu. */
    n = 0;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Mouse Hilite", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_HILITE );

    button = XmCreatePushButtonGadget( menu_pane, "Mouse Select", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_SELECT );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Clear Hilite", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_CLEARHILITE );

    button = XmCreatePushButtonGadget( menu_pane, "Clear Select", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_CLEARSELECT );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Pick Shells", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_PICSHELL );

    button = XmCreatePushButtonGadget( menu_pane, "Pick Beams", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_PICBEAM );

    button = XmCreateSeparatorGadget( menu_pane, "separator", args, n );
    XtManageChild( button );

    button = XmCreatePushButtonGadget( menu_pane, "Center Hilite On", args, n);
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_CENTERON );

    button = XmCreatePushButtonGadget( menu_pane, "Center Hilite Off", args, n);
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_CENTEROFF );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_bar, "Picking", args, n );
    XtManageChild( cascade );

    /* Result menu. */
    n = 0;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );
 
    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Materials", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, res_menu_CB,
                   trans_result[VAL_NONE][3]  );

    /* Shared result submenu. */
    n = 0;
    submenu_pane = XmCreatePulldownMenu( menu_pane, "submenu_pane", args, n );

    for ( i = 0; share_btns[i] != VAL_ALL_END; i++ )
    {
        idx = resultid_to_index[share_btns[i]];
        title = trans_result[idx][1];
        com = trans_result[idx][3];
        n = 0;
        button = XmCreatePushButtonGadget( submenu_pane, title, args, n );
        XtManageChild( button );
        XtAddCallback( button, XmNactivateCallback, res_menu_CB, com );
    }

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, submenu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_pane, "Shared", args, n );
    XtManageChild( cascade );

    /* Hex result submenu. */
    n = 0;
    submenu_pane = XmCreatePulldownMenu( menu_pane, "submenu_pane", args, n );

    for ( i = 0; hex_btns[i] != VAL_ALL_END; i++ )
    {
        idx = resultid_to_index[hex_btns[i]];
        if ( idx >= 0 )
        {
            title = trans_result[idx][1];
            com = trans_result[idx][3];
            n = 0;
            button = XmCreatePushButtonGadget( submenu_pane, title, args, n );
            XtManageChild( button );
            XtAddCallback( button, XmNactivateCallback, res_menu_CB, com );
        }
    }

    n = 0;
    button = XmCreateSeparatorGadget( submenu_pane, "separator", args, n );
    XtManageChild( button );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Strn Type Infinitesimal", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_INFINITESIMAL );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Strn Type Almansi", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_ALMANSI );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Strn Type Green", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_GREEN );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Strn Type Rate", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_RATE );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, submenu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_pane, "Volume", args, n );
    XtManageChild( cascade );

    /* Shell result submenu. */
    n = 0;
    submenu_pane = XmCreatePulldownMenu( menu_pane, "submenu_pane", args, n );

    for ( i = 0; shell_btns[i] != VAL_ALL_END; i++ )
    {
        idx = resultid_to_index[shell_btns[i]];
        if ( idx >= 0 )
        {
            title = trans_result[idx][1];
            com = trans_result[idx][3];
            n = 0;
            button = XmCreatePushButtonGadget( submenu_pane, title, args, n );
            XtManageChild( button );
            XtAddCallback( button, XmNactivateCallback, res_menu_CB, com );
        }
    }

    n = 0;
    button = XmCreateSeparatorGadget( submenu_pane, "separator", args, n );
    XtManageChild( button );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Global Basis", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_GLOBAL );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Local Basis", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_LOCAL );

    n = 0;
    button = XmCreateSeparatorGadget( submenu_pane, "separator", args, n );
    XtManageChild( button );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Middle Surface", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_MIDDLE );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Inner Surface", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_INNER );

    n = 0;
    button = XmCreatePushButtonGadget( submenu_pane,
                                       "Outer Surface", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_OUTER );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, submenu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_pane, "Shell", args, n );
    XtManageChild( cascade );

    /* Beam result submenu. */
    n = 0;
    submenu_pane = XmCreatePulldownMenu( menu_pane, "submenu_pane", args, n );

    for ( i = 0; beam_btns[i] != VAL_ALL_END; i++ )
    {
        idx = resultid_to_index[beam_btns[i]];
        if ( idx >= 0 )
        {
            title = trans_result[idx][1];
            com = trans_result[idx][3];
            n = 0;
            button = XmCreatePushButtonGadget( submenu_pane, title, args, n );
            XtManageChild( button );
            XtAddCallback( button, XmNactivateCallback, res_menu_CB, com );
        }
    }

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, submenu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_pane, "Beam", args, n );
    XtManageChild( cascade );

    /* Nodal result submenu. */
    n = 0;
    submenu_pane = XmCreatePulldownMenu( menu_pane, "submenu_pane", args, n );

    for ( i = 0; node_btns[i] != VAL_ALL_END; i++ )
    {
        idx = resultid_to_index[node_btns[i]];
        if ( idx >= 0 )
        {
            title = trans_result[idx][1];
            com = trans_result[idx][3];
            n = 0;
            button = XmCreatePushButtonGadget( submenu_pane, title, args, n );
            XtManageChild( button );
            XtAddCallback( button, XmNactivateCallback, res_menu_CB, com );
        }
    }

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, submenu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_pane, "Nodal", args, n );
    XtManageChild( cascade );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_bar, "Result", args, n );
    XtManageChild( cascade );
 
    /* Time menu. */
    n = 0;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Next State", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_NEXTSTATE );

    button = XmCreatePushButtonGadget( menu_pane, "Prev State", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_PREVSTATE );

    button = XmCreatePushButtonGadget( menu_pane, "First State", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_FIRSTSTATE );

    button = XmCreatePushButtonGadget( menu_pane, "Last State", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_LASTSTATE );

    button = XmCreatePushButtonGadget( menu_pane, "Animate States", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_ANIMATE );

    button = XmCreatePushButtonGadget( menu_pane, "Stop Animate", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_STOPANIMATE );

    button = XmCreatePushButtonGadget( menu_pane, "Continue Animate", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_CONTANIMATE );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_bar, "Time", args, n );
    XtManageChild( cascade );

    /* Plot menu. */
    n = 0;
    menu_pane = XmCreatePulldownMenu( menu_bar, "menu_pane", args, n );

    n = 0;
    button = XmCreatePushButtonGadget( menu_pane, "Time Hist Plot", args, n );
    XtManageChild( button );
    XtAddCallback( button, XmNactivateCallback, menu_CB, BTN_TIMEPLOT );

    n = 0;
    XtSetArg( args[n], XmNsubMenuId, menu_pane ); n++;
    cascade = XmCreateCascadeButton( menu_bar, "Plot", args, n );
    XtManageChild( cascade );

    return( menu_bar );
}


/*****************************************************************
 * TAG( create_mtl_manager )
 *
 * Create the material manager widget.
 */
static Widget
create_mtl_manager( main_widg )
Widget main_widg;
{
    Widget mat_mgr, mat_base, widg, func_select, scroll_win, sep1, 
        sep2, sep3, vert_scroll, func_operate, slider_base, mtl_label, 
	frame, red_scale, green_scale, blue_scale, col_comp;
    Arg args[10];
    char win_title[64];
    char mtl_toggle_name[8];
    int n, i, mtl, qty_mtls;
    Dimension width, max_child_width, margin_width, spacing, scrollbar_width, 
        name_len, col_ed_width, max_mgr_width, height, sw_height, fudge;
    short func_width, max_cols, rows;
    XtActionsRec rec;
    static Widget select_buttons[4];
    static Widget *ctl_buttons[2];
    XmString val_text, str;
    int scale_len, val_len, vert_space, scale_offset, vert_height, 
        vert_pos, hori_offset, vert_offset, loc;
    char *func_names[] = 
        { "Visible", "Invisible", "Enable", "Disable", "Color" };
    char *select_names[] = 
    { 
        "All", "None", "Invert", "By func", "Prev" 
    };
    char *op_names[] = { "Preview", "Cancel", "Apply", "Default" };
    char *scale_names[] = { "Red", "Green", "Blue", "Shininess" };
    char *comp_names[] = { "Ambient", "Diffuse", "Specular", "Emissive" };
    Pixel fg, bg;
    
    ctl_buttons[0] = mtl_mgr_func_toggles;
    ctl_buttons[1] = select_buttons;
    qty_mtls = env.curr_analy->num_materials;

    sprintf( win_title, "%s Material Manager", griz_name );
    n = 0;
    XtSetArg( args[n], XtNtitle, win_title ); n++;
    XtSetArg( args[n], XmNiconic, FALSE ); n++;
    XtSetArg( args[n], XmNwindowGroup, XtWindow( main_widg ) ); n++;
    mat_mgr = XtAppCreateShell( "GRIZ", "mat_mgr",
                                topLevelShellWidgetClass,
                                XtDisplay( main_widg ), args, n );

    XtAddCallback( mat_mgr, XmNdestroyCallback, 
	           destroy_mtl_mgr_CB, (XtPointer) NULL );

    XtAddEventHandler( mat_mgr, EnterWindowMask | LeaveWindowMask, False, 
                       gress_mtl_mgr, NULL );

    /* Use a Form widget to manage everything else. */
    mat_base = XtVaCreateManagedWidget( 
        "mat_base", xmFormWidgetClass, mat_mgr, 
	NULL );
	
    widg = XtVaCreateManagedWidget( 
        "Function", xmLabelGadgetClass, mat_base, 
	XmNalignment, XmALIGNMENT_CENTER, 
	XmNtopAttachment, XmATTACH_FORM, 
	XmNrightAttachment, XmATTACH_FORM, 
	XmNleftAttachment, XmATTACH_FORM, 
	NULL );

    /* Use a RowColumn to hold the function select toggles. */
    func_select = XtVaCreateManagedWidget( 
        "func_select", xmRowColumnWidgetClass, mat_base, 
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
        "sep1", xmSeparatorGadgetClass, mat_base, 
	XmNtopAttachment, XmATTACH_WIDGET, 
	XmNtopWidget, func_select, 
	XmNrightAttachment, XmATTACH_FORM, 
	XmNleftAttachment, XmATTACH_FORM, 
	NULL );
	
    mtl_label = XtVaCreateManagedWidget( 
        "Material", xmLabelGadgetClass, mat_base, 
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
        "color_editor", xmBulletinBoardWidgetClass, mat_base, 
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

        XtAddCallback( color_comps[i], XmNdisarmCallback, 
                       col_comp_disarm_CB, (XtPointer) i );
	
	prop_val_changed[i] = FALSE;
        property_vals[i] = NEW_N( GLfloat, 3, "Col comp val" );
    }

    XtVaGetValues( color_editor, XmNforeground, &fg, XmNbackground, &bg, NULL );
    check = XCreatePixmapFromBitmapData( dpy, 
        RootWindow( dpy, DefaultScreen( dpy ) ), GrizCheck_bits, GrizCheck_width, 
	GrizCheck_height, fg, bg, DefaultDepth( dpy, DefaultScreen( dpy ) ) );
	
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

    /* Use a pixmap of a check as the modify indicator. */
    for ( i = 0; i < (EMISSIVE + 1); i++ )
    {
	prop_checks[i] = XtVaCreateManagedWidget( 
            "check", xmLabelWidgetClass, widg, 
	    XmNlabelType, XmPIXMAP, 
	    XmNlabelPixmap, check, 
	    XmNwidth, name_len, 
	    XmNrecomputeSize, False, 
	    XmNmappedWhenManaged, False, 
	    XmNmarginWidth, 0, 
	    XmNmarginHeight, 0, 
	    XmNpositionIndex, ((short) i), 
	    NULL );
    }
    
    /* These need to be #def'd when stable. */
    scale_len = 200;
    val_len = 50;
    vert_height = 25;
    vert_space = 8;
    scale_offset = 3;
    hori_offset = 10 + 3;
    vert_offset = 75;
    
    /* Now add the modify indicator for the shininess scale. */
    prop_checks[SHININESS] = XtVaCreateManagedWidget( 
        "check", xmLabelWidgetClass, color_editor, 
	XmNx, (hori_offset + (name_len - GrizCheck_width) / 2), 
	XmNy, (vert_offset + 3 * vert_height 
	       + 4 * vert_space + vert_height / 2 - GrizCheck_height), 
        XmNlabelType, XmPIXMAP, 
        XmNlabelPixmap, check, 
        XmNmappedWhenManaged, False, 
        XmNmarginWidth, 0, 
        XmNmarginHeight, 0, 
        NULL );

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
        frame = XtVaCreateManagedWidget( 
            "frame", xmFrameWidgetClass, color_editor, 
	    XmNx, hori_offset + name_len + scale_len + spacing, 
	    XmNy, vert_pos, 
/*	    XmNwidth, val_len, */
	    XmNheight, vert_height, 
	    NULL ); 
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
        "swatch", glwMDrawingAreaWidgetClass, swatch_frame, 
	GLwNvisualInfo, vi,
        NULL );

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
   
    /* Init some globals associated with the color editor. */
    prop_val_changed[SHININESS] = FALSE;
    property_vals[SHININESS] = NEW( GLfloat, "Shine comp val" );
    cur_mtl_comp = MTL_PROP_QTY; /* Guaranteed to be a non-functional value. */
    cur_color[0] = 0.0;
    cur_color[1] = 0.0;
    cur_color[2] = 0.0;
    cur_color[3] = 0.0;
    cur_mtl = 0;

    /* Place a separator on top of the color editor. */
    sep3 = XtVaCreateManagedWidget( 
        "sep3", xmSeparatorGadgetClass, mat_base, 
	XmNbottomAttachment, XmATTACH_WIDGET, 
	XmNbottomWidget, color_editor, 
	XmNrightAttachment, XmATTACH_FORM, 
	XmNleftAttachment, XmATTACH_FORM, 
	NULL );

    /* Use a RowColumn to hold the function operate buttons. */
    func_operate = XtVaCreateManagedWidget( 
        "func_operate", xmRowColumnWidgetClass, mat_base, 
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

    max_child_width = 0;
	
    for ( i = 0; i < sizeof( op_names ) / sizeof( op_names[0] ); i++ )
    {
        op_buttons[i] = XtVaCreateManagedWidget( 
            op_names[i], xmPushButtonGadgetClass, func_operate, 
	    XmNsensitive, False, 
	    NULL ); 

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
        "Action", xmLabelGadgetClass, mat_base, 
	XmNalignment, XmALIGNMENT_CENTER, 
	XmNbottomAttachment, XmATTACH_WIDGET, 
	XmNbottomWidget, func_operate, 
	XmNrightAttachment, XmATTACH_FORM, 
	XmNleftAttachment, XmATTACH_FORM, 
	NULL );

    /* Place a separator on top of the operation buttons. */
    sep2 = XtVaCreateManagedWidget( 
        "sep2", xmSeparatorGadgetClass, mat_base, 
	XmNbottomAttachment, XmATTACH_WIDGET, 
	XmNbottomWidget, widg, 
	XmNrightAttachment, XmATTACH_FORM, 
	XmNleftAttachment, XmATTACH_FORM, 
	NULL );

    /* Use a RowColumn to hold the global material select buttons. */
    quick_select = XtVaCreateManagedWidget( 
        "quick_select", xmRowColumnWidgetClass, mat_base, 
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
	
    max_child_width = 0;
    
    for ( i = 0; i < 4; i++ )
    {
        select_buttons[i] = XtVaCreateManagedWidget( 
            select_names[i], xmPushButtonGadgetClass, quick_select, 
	    XmNshadowThickness, 3, 
	    NULL );

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
    XtVaSetValues( quick_select, 
        XmNrightOffset, -((int) (func_width / 2)), NULL );

    if ( func_width > max_mgr_width )
        max_mgr_width = func_width;

    /* 
     * Now insert a scrolled window to hold the material select 
     * toggles and bridge the widgets attached from the top and 
     * the bottom of the form.
     */
     
    scroll_win = XtVaCreateManagedWidget( 
        "mtl_scroll", xmScrolledWindowWidgetClass, mat_base, 
	XmNscrollingPolicy, XmAUTOMATIC, 
	XmNtopAttachment, XmATTACH_WIDGET, 
	XmNtopWidget, mtl_label, 
	XmNrightAttachment, XmATTACH_FORM, 
	XmNleftAttachment, XmATTACH_FORM, 
	XmNbottomAttachment, XmATTACH_WIDGET, 
	XmNbottomWidget, quick_select, 
	NULL );
    
    XtVaGetValues( scroll_win, XmNverticalScrollBar, &vert_scroll, NULL );
    XtVaGetValues( vert_scroll, XmNwidth, &scrollbar_width, NULL );
    
    rec.string = "resize_mtl_scrollwin";
    rec.proc = resize_mtl_scrollwin;
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
    mtl_mgr_cmd = NEW_N( char, 128 + qty_mtls * 4, "Mtl mgr cmd bufr" );

    /* All done, pop it up. */
    XtPopup( mat_mgr, XtGrabNone ); 

    return mat_mgr;
}


/*****************************************************************
 * TAG( create_free_util_panel )
 *
 * Create the utility panel widget.
 */
static Widget
create_free_util_panel( main_widg )
Widget main_widg;
{
    int n;
    char win_title[64];
    Arg args[10];
    Widget util_shell, util_main;

    sprintf( win_title, "%s Utility Panel", griz_name );
    n = 0;
    XtSetArg( args[n], XtNtitle, win_title ); n++;
    XtSetArg( args[n], XmNiconic, FALSE ); n++;
    XtSetArg( args[n], XmNwindowGroup, XtWindow( main_widg ) ); n++;
    util_shell = XtAppCreateShell( "GRIZ", "util_panel",
                                   topLevelShellWidgetClass,
                                   XtDisplay( main_widg ), args, n );
				   
    util_main = create_utility_panel( util_shell );

    /* Pop it up. */
    XtPopup( util_shell, XtGrabNone ); 
    
    return util_shell;
}


/*****************************************************************
 * TAG( create_utility_panel )
 *
 * Create the utility panel widget.
 */
static Widget
create_utility_panel( main_widg )
Widget main_widg;
{
    Widget util_main, widg, state_ctl, step_ctl, anim_ctl, 
        annot_lab, annot_ctl, annots, rend_child, sep1, sep2;
    XtActionsRec rec;
    Pixel fg, bg;
    Pixmap pixmap;
    Dimension width, offset, rc_width, child_width, stride_width, height, 
        margin_width, spacing;
    char mtl_toggle_name[8], stride_text[5];
    int n, i;
    int anim_ctl_left, anim_ctl_right, delta;
    float ratio;
    XmString stride_str;
    Analysis *analy;
    
    analy = env.curr_analy;
    
    /* Use a Form widget to manage everything else. */
    util_main = XtVaCreateManagedWidget( 
        "util_main", xmFormWidgetClass, main_widg, 
	NULL );

    XtAddCallback( util_main, XmNdestroyCallback, 
	           destroy_util_panel_CB, (XtPointer) NULL );

    state_ctl = XtVaCreateManagedWidget(
        "state_ctl", xmRowColumnWidgetClass, util_main, 
	XmNorientation, XmHORIZONTAL, 
	XmNrowColumnType, XmWORK_AREA, 
	XmNtraversalOn, False, 
	XmNtopAttachment, XmATTACH_FORM, 
	XmNrightAttachment, XmATTACH_POSITION, 
	XmNrightPosition, 50, 
	NULL );

    /* Get these for use later. */
    XtVaGetValues( state_ctl, 
        XmNmarginWidth, &margin_width, 
	XmNspacing, &spacing, 
	XmNforeground, &fg, 
	XmNbackground, &bg, 
	NULL );
	
    rc_width = 2 * margin_width + 12 * spacing;
    
    widg = XtVaCreateManagedWidget( 
        "Step", xmLabelGadgetClass, state_ctl, 
	NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    
    stride_width = 3 * child_width / 4;
    
    pixmap_leftstop = XCreatePixmapFromBitmapData( dpy, 
        RootWindow( dpy, DefaultScreen( dpy ) ), GrizLeftStop_bits, 
	GrizLeftStop_width, GrizLeftStop_height, fg, bg, 
	DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget( 
        "first_state", xmPushButtonGadgetClass, state_ctl, 
	XmNlabelType, XmPIXMAP, 
	XmNlabelPixmap, pixmap_leftstop, 
	NULL );
    XtAddCallback( widg, XmNactivateCallback, step_CB, STEP_FIRST );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    pixmap_left = XCreatePixmapFromBitmapData( dpy, 
        RootWindow( dpy, DefaultScreen( dpy ) ), GrizLeft_bits, 
	GrizLeft_width, GrizLeft_height, fg, bg, 
	DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget( 
        "prev_state", xmPushButtonGadgetClass, state_ctl, 
	XmNlabelType, XmPIXMAP, 
	XmNlabelPixmap, pixmap_left, 
	NULL );
    XtAddCallback( widg, XmNactivateCallback, step_CB, STEP_DOWN );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    pixmap_right = XCreatePixmapFromBitmapData( dpy, 
        RootWindow( dpy, DefaultScreen( dpy ) ), GrizRight_bits, 
	GrizRight_width, GrizRight_height, fg, bg, 
	DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget( 
        "next_state", xmPushButtonGadgetClass, state_ctl, 
	XmNlabelType, XmPIXMAP, 
	XmNlabelPixmap, pixmap_right, 
	NULL );
    XtAddCallback( widg, XmNactivateCallback, step_CB, STEP_UP );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;

    pixmap_rightstop = XCreatePixmapFromBitmapData( dpy, 
        RootWindow( dpy, DefaultScreen( dpy ) ), GrizRightStop_bits, 
	GrizRightStop_width, GrizRightStop_height, fg, bg, 
	DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget( 
        "last_state", xmPushButtonGadgetClass, state_ctl, 
	XmNlabelType, XmPIXMAP, 
	XmNlabelPixmap, pixmap_rightstop, 
	NULL );
    XtAddCallback( widg, XmNactivateCallback, step_CB, STEP_LAST );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    
    stride_str = XmStringCreateLtoR( "  Step\n Stride", 
                                     XmSTRING_DEFAULT_CHARSET );
    widg = XtVaCreateManagedWidget( 
        "stride_label", xmLabelGadgetClass, state_ctl, 
	XmNlabelString, stride_str, 
	NULL );
    XmStringFree( stride_str );
    XtVaGetValues( widg, XmNwidth, &child_width, XmNheight, &height, NULL );
    rc_width += child_width;
  
    sprintf( stride_text, "%d", step_stride );
    stride_label = XtVaCreateManagedWidget( 
        "stride_text", xmTextFieldWidgetClass, state_ctl, 
	XmNcolumns, 3, 
	XmNcursorPositionVisible, True, 
	XmNeditable, True, 
	XmNresizeWidth, True, 
	XmNvalue, stride_text, 
	NULL );
    XtAddCallback( stride_label, XmNactivateCallback, stride_CB, STRIDE_EDIT );
    XtVaGetValues( stride_label, XmNwidth, &child_width, NULL );
    rc_width += child_width;
	
    widg = XtVaCreateManagedWidget( 
        "stride_incr", xmArrowButtonWidgetClass, state_ctl, 
	NULL );
    XtAddCallback( widg, XmNactivateCallback, stride_CB, STRIDE_INCREMENT );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    
    widg = XtVaCreateManagedWidget( 
        "stride_decr", xmArrowButtonWidgetClass, state_ctl, 
	XmNarrowDirection, XmARROW_DOWN, 
	NULL );
    XtAddCallback( widg, XmNactivateCallback, stride_CB, STRIDE_DECREMENT );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    
    widg = XtVaCreateManagedWidget( 
        "  Animate", xmLabelGadgetClass, state_ctl, 
	NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    
    pixmap_start = XCreatePixmapFromBitmapData( dpy, 
        RootWindow( dpy, DefaultScreen( dpy ) ), GrizStart_bits, 
	GrizStart_width, GrizStart_height, fg, bg, 
	DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget( 
        "start_anim", xmPushButtonGadgetClass, state_ctl, 
	XmNlabelType, XmPIXMAP, 
	XmNlabelPixmap, pixmap_start, 
	NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    XtAddCallback( widg, XmNactivateCallback, menu_CB, BTN_ANIMATE );
    
    pixmap_stop = XCreatePixmapFromBitmapData( dpy, 
        RootWindow( dpy, DefaultScreen( dpy ) ), GrizStop_bits, 
	GrizStop_width, GrizStop_height, fg, bg, 
	DefaultDepth( dpy, DefaultScreen( dpy ) ) );
    widg = XtVaCreateManagedWidget( 
        "stop_anim", xmPushButtonGadgetClass, state_ctl, 
	XmNlabelType, XmPIXMAP, 
	XmNlabelPixmap, pixmap_stop, 
	NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    XtAddCallback( widg, XmNactivateCallback, menu_CB, BTN_STOPANIMATE );
    
    widg = XtVaCreateManagedWidget( 
        "cont_anim", xmPushButtonGadgetClass, state_ctl, 
	XmNlabelType, XmPIXMAP, 
	XmNlabelPixmap, pixmap_right, 
	NULL );
    XtVaGetValues( widg, XmNwidth, &child_width, NULL );
    rc_width += child_width;
    XtAddCallback( widg, XmNactivateCallback, menu_CB, BTN_CONTANIMATE );
    
    XtVaSetValues( state_ctl, 
        XmNrightOffset, -((int) (rc_width / 2)), NULL );

    sep1 = XtVaCreateManagedWidget(
        "sep1", xmSeparatorGadgetClass, util_main, 
	XmNtopAttachment, XmATTACH_WIDGET, 
	XmNtopWidget, state_ctl, 
	XmNrightAttachment, XmATTACH_FORM, 
	XmNleftAttachment, XmATTACH_FORM, 
	NULL );

    util_render_ctl = XtVaCreateManagedWidget( 
        "util_render_ctl", xmRowColumnWidgetClass, util_main, 
	XmNtopAttachment, XmATTACH_WIDGET, 
	XmNtopWidget, sep1, 
	XmNrightAttachment, XmATTACH_POSITION, 
	XmNrightPosition, 50, 
	XmNorientation, XmHORIZONTAL, 
	XmNpacking, XmPACK_COLUMN, 
	XmNtraversalOn, False, 
	NULL );

    util_render_btns = NEW_N( Widget, UTIL_PANEL_BTN_QTY, "Util render btns" );

    rend_child = XtVaCreateManagedWidget( 
        "render_select", xmRowColumnWidgetClass, util_render_ctl, 
	XmNisAligned, True, 
	XmNentryAlignment, XmALIGNMENT_CENTER, 
	XmNorientation, XmVERTICAL, 
	XmNpacking, XmPACK_COLUMN, 
	NULL );

    child_width = 0;
    
    widg = XtVaCreateManagedWidget( 
        "Mesh View", xmLabelGadgetClass, rend_child, 
	NULL );
    XtVaGetValues( widg, XmNwidth, &width, NULL );
    if ( width > child_width ) 
        child_width = width;
	
    util_render_btns[VIEW_SOLID] = XtVaCreateManagedWidget( 
        "Solid", xmToggleButtonWidgetClass, rend_child, 
	XmNindicatorOn,  False, 
	XmNset,  ( analy->render_mode == RENDER_FILLED ), 
	XmNshadowThickness, 3, 
	XmNfillOnSelect, True, 
        NULL );
    XtVaGetValues( util_render_btns[VIEW_SOLID], XmNwidth, &width, NULL );
    if ( width > child_width ) 
        child_width = width;
    XtAddCallback( util_render_btns[VIEW_SOLID], XmNvalueChangedCallback, 
                   util_render_CB, (XtPointer) VIEW_SOLID );
	
    util_render_btns[VIEW_SOLID_MESH] = XtVaCreateManagedWidget( 
        "Solid Mesh", xmToggleButtonWidgetClass, rend_child, 
	XmNindicatorOn, False, 
	XmNset, ( analy->render_mode == RENDER_HIDDEN ), 
	XmNshadowThickness, 3, 
	XmNfillOnSelect, True, 
	NULL );
    XtVaGetValues( util_render_btns[VIEW_SOLID_MESH], XmNwidth, &width, NULL );
    if ( width > child_width ) 
        child_width = width;
    XtAddCallback( util_render_btns[VIEW_SOLID_MESH], XmNvalueChangedCallback, 
                   util_render_CB, (XtPointer) VIEW_SOLID_MESH );
	
    util_render_btns[VIEW_EDGES] = XtVaCreateManagedWidget( 
        "Edges Only", xmToggleButtonWidgetClass, rend_child, 
	XmNindicatorOn, False, 
	XmNset, ( analy->show_edges 
	          && env.curr_analy->render_mode == RENDER_NONE ), 
	XmNshadowThickness, 3, 
	XmNfillOnSelect, True, 
	NULL );
    XtVaGetValues( util_render_btns[VIEW_EDGES], XmNwidth, &width, NULL );
    if ( width > child_width ) 
        child_width = width;
    XtAddCallback( util_render_btns[VIEW_EDGES], XmNvalueChangedCallback, 
                   util_render_CB, VIEW_EDGES );

    rend_child = XtVaCreateManagedWidget( 
        "render_pick", xmRowColumnWidgetClass, util_render_ctl, 
	XmNisAligned, True, 
	XmNentryAlignment, XmALIGNMENT_CENTER, 
	XmNradioBehavior, True, 
	XmNradioAlwaysOne, True, 
	XmNorientation, XmVERTICAL, 
	XmNpacking, XmPACK_COLUMN, 
	NULL );

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

    rend_child = XtVaCreateManagedWidget( 
        "bt2_pick", xmRowColumnWidgetClass, util_render_ctl, 
	XmNisAligned, True, 
	XmNentryAlignment, XmALIGNMENT_CENTER, 
	XmNradioBehavior, True, 
	XmNradioAlwaysOne, True, 
	XmNorientation, XmVERTICAL, 
	XmNpacking, XmPACK_COLUMN, 
	NULL );

    widg = XtVaCreateManagedWidget( 
        "Btn-2 Pick", xmLabelGadgetClass, rend_child, 
	NULL );
    XtVaGetValues( widg, XmNwidth, &width, NULL );
    if ( width > child_width ) 
        child_width = width;
    util_render_btns[BTN2_SHELL] = XtVaCreateManagedWidget( 
        "Shells", xmToggleButtonWidgetClass, rend_child, 
	XmNindicatorOn, False, 
	XmNset, ( analy->pick_beams != TRUE ), 
	XmNshadowThickness, 3, 
	XmNfillOnSelect, True, 
        NULL );
    XtVaGetValues( util_render_btns[BTN2_SHELL], XmNwidth, &width, NULL );
    if ( width > child_width ) 
        child_width = width;
    XtAddCallback( util_render_btns[BTN2_SHELL], XmNvalueChangedCallback, 
                   util_render_CB, (XtPointer) BTN2_SHELL );

    util_render_btns[BTN2_BEAM] = XtVaCreateManagedWidget( 
        "Beams", xmToggleButtonWidgetClass, rend_child, 
	XmNindicatorOn, False, 
	XmNset, ( analy->pick_beams == TRUE ), 
	XmNshadowThickness, 3, 
	XmNfillOnSelect, True, 
	NULL );
    XtVaGetValues( util_render_btns[BTN2_BEAM], XmNwidth, &width, NULL );
    if ( width > child_width ) 
        child_width = width;
    XtAddCallback( util_render_btns[BTN2_BEAM], XmNvalueChangedCallback, 
                   util_render_CB, (XtPointer) BTN2_BEAM );
	
    rend_child = XtVaCreateManagedWidget( 
        "render_clean", xmRowColumnWidgetClass, util_render_ctl, 
	XmNisAligned, True, 
	XmNentryAlignment, XmALIGNMENT_CENTER, 
	XmNorientation, XmVERTICAL, 
	XmNpacking, XmPACK_COLUMN, 
	NULL );

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

    rc_width = 10 * margin_width + 3 * spacing + 4 * child_width;
    
    XtVaSetValues( util_render_ctl, 
        XmNrightOffset, -((int) (rc_width / 2)), NULL );
	
    return util_main;
}


/*****************************************************************
 * TAG( init_gui )
 *
 * This routine is called upon start-up of the gui.  It initializes
 * the GL window.
 */
static void
init_gui()
{
    FILE *test_file;
    char *init_file;
    char init_cmd[100];

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
    if ( ( test_file = fopen( "grizinit", "r" ) ) != NULL )
    {
        fclose( test_file );
        parse_command( "h grizinit", env.curr_analy );
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
}


/*****************************************************************
 * TAG( expose_CB )
 *
 * Called when expose events occur.  Redraws the mesh window.
 */
static void
expose_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    OpenGL_win save_win;
    
    save_win = cur_opengl_win;
    switch_opengl_win( MESH );
    
    update_display( env.curr_analy );
    
    if ( save_win != MESH )
        switch_opengl_win( save_win );
}


/*****************************************************************
 * TAG( resize_CB )
 *
 * Called when resize events occur.  Resets the GL window size.
 */
static void
resize_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    Position winx, winy;
    Dimension width, height;

/*
    GLXwinset( XtDisplay(w), XtWindow(w) );
*/

    /* Don't use OpenGL until window is realized! */
    if ( XtIsRealized(w) )
    {
        XtVaGetValues( w, XmNwidth, &width, XmNheight, &height, NULL );

        /* Set viewport to fill the OpenGL window. */
        glViewport( 0, 0, (GLint)width, (GLint)height );
        set_mesh_view();
        update_display( env.curr_analy );
    }
}

/*****************************************************************
 * TAG( res_menu_CB )
 *
 * Handles menu button events for the result menu buttons.
 */
static void
res_menu_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    char com_str[80];
    char *com;

    /* The result title is passed in the client_data. */
    com = (char *)client_data;
    sprintf( com_str, "show \"%s\"", com );
    parse_command( com_str, env.curr_analy );
}


/*****************************************************************
 * TAG( menu_CB )
 *
 * Handles menu button events.
 */
static void
menu_CB( w, client_data, call_data )
Widget w;
caddr_t client_data;
caddr_t call_data;
{
    Btn_type btn;

    btn = (Btn_type)client_data;

    switch( btn )
    {
        case BTN_COPYRIGHT:
            parse_command( "copyrt", env.curr_analy );
            break;
        case BTN_MTL_MGR:
	    create_app_widg( BTN_MTL_MGR );
            break;
        case BTN_UTIL_PANEL:
	    create_app_widg( BTN_UTIL_PANEL );
            break;
        case BTN_QUIT:
            quit( 0 );
            break;

        case BTN_DRAWFILLED:
            parse_command( "switch solid", env.curr_analy );
            break;
        case BTN_DRAWHIDDEN:
            parse_command( "switch hidden", env.curr_analy );
            break;
        case BTN_COORDSYS:
            if ( env.curr_analy->show_coord )
                parse_command( "off coord", env.curr_analy );
            else 
                parse_command( "on coord", env.curr_analy );
            break;
        case BTN_TITLE:
            if ( env.curr_analy->show_title )
                parse_command( "off title", env.curr_analy );
            else 
                parse_command( "on title", env.curr_analy );
            break;
        case BTN_TIME:
            if ( env.curr_analy->show_time )
                parse_command( "off time", env.curr_analy );
            else 
                parse_command( "on time", env.curr_analy );
            break;
        case BTN_COLORMAP:
            if ( env.curr_analy->show_colormap )
                parse_command( "off cmap", env.curr_analy );
            else 
                parse_command( "on cmap", env.curr_analy );
            break;
        case BTN_MINMAX:
            if ( env.curr_analy->show_minmax )
                parse_command( "off minmax", env.curr_analy );
            else 
                parse_command( "on minmax", env.curr_analy );
            break;
        case BTN_ALLON:
            parse_command( "on all", env.curr_analy );
            break;
        case BTN_ALLOFF:
            parse_command( "off all", env.curr_analy );
            break;
        case BTN_BBOX:
            if ( env.curr_analy->show_bbox )
                parse_command( "off box", env.curr_analy );
            else
                parse_command( "on box", env.curr_analy );
            break;
        case BTN_EDGES:
            if ( env.curr_analy->show_edges )
                parse_command( "off edges", env.curr_analy );
            else
                parse_command( "on edges", env.curr_analy );
            break;
        case BTN_PERSPECTIVE:
            parse_command( "switch persp", env.curr_analy );
            break;
        case BTN_ORTHOGRAPHIC:
            parse_command( "switch ortho", env.curr_analy );
            break;
        case BTN_ADJUSTNF:
            parse_command( "rnf", env.curr_analy );
            break;
        case BTN_RESETVIEW:
            parse_command( "rview", env.curr_analy );
            break;

        case BTN_HILITE:
            parse_command( "switch pichil", env.curr_analy );
            break;
        case BTN_SELECT:
            parse_command( "switch picsel", env.curr_analy );
            break;
        case BTN_CLEARHILITE:
            parse_command( "clrhil", env.curr_analy );
            break;
        case BTN_CLEARSELECT:
            parse_command( "clrsel", env.curr_analy );
            break;
        case BTN_PICSHELL:
            parse_command( "switch picsh", env.curr_analy );
            break;
        case BTN_PICBEAM:
            parse_command( "switch picbm", env.curr_analy );
            break;
        case BTN_CENTERON:
            parse_command( "vcent hi", env.curr_analy );
            break;
        case BTN_CENTEROFF:
            parse_command( "vcent off", env.curr_analy );
            break;

        case BTN_INFINITESIMAL:
            parse_command( "switch infin", env.curr_analy );
            break;
        case BTN_ALMANSI:
            parse_command( "switch alman", env.curr_analy );
            break;
        case BTN_GREEN:
            parse_command( "switch grn", env.curr_analy );
            break;
        case BTN_RATE:
            parse_command( "switch rate", env.curr_analy );
            break;
        case BTN_GLOBAL:
            parse_command( "switch rglob", env.curr_analy );
            break;
        case BTN_LOCAL:
            parse_command( "switch rloc", env.curr_analy );
            break;
        case BTN_MIDDLE:
            parse_command( "switch middle", env.curr_analy );
            break;
        case BTN_INNER:
            parse_command( "switch inner", env.curr_analy );
            break;
        case BTN_OUTER:
            parse_command( "switch outer", env.curr_analy );
            break;

        case BTN_NEXTSTATE:
            parse_command( "n", env.curr_analy );
            break;
        case BTN_PREVSTATE:
            parse_command( "p", env.curr_analy );
            break;
        case BTN_FIRSTSTATE:
            parse_command( "f", env.curr_analy );
            break;
        case BTN_LASTSTATE:
            parse_command( "l", env.curr_analy );
            break;
        case BTN_ANIMATE:
            parse_command( "anim", env.curr_analy );
            break;
        case BTN_STOPANIMATE:
            parse_command( "stopan", env.curr_analy );
            break;
        case BTN_CONTANIMATE:
            parse_command( "animc", env.curr_analy );
            break;

        case BTN_TIMEPLOT:
            parse_command( "timhis", env.curr_analy );
            break;

        default:
            wrt_text( "Menu button %d unrecognized!", btn );
    }
}


/*****************************************************************
 * TAG( input_CB )
 *
 * Callback for input events.
 */
static void
input_CB( w, client_data, call_data )
Widget w;
caddr_t client_data;
XmScaleCallbackStruct *call_data;
{
    static int posx, posy;
    int orig_posx, orig_posy;
    char buffer[1];
/*  KeySym keysym; */
    float angle, dx, dy, scale_fac, scale[3];
    char str[50];
    static int mode;
    int ident, obj_type;
    Bool_type identify_only;
    static char *obj_type_names[] = { "n", "b", "s", "h" };
    XButtonEvent *evt;

    switch( call_data->event->type )
    {
/*
        case KeyRelease:
*/
            /* It is necessary to convert the keycode to a keysym before
             * checking if it is an escape.
             */
/*
            if ( XLookupString(call_data->event,buffer,1,&keysym,NULL) == 1 &&
                 keysym == (KeySym)XK_Escape )
                quit(0);
            break;
*/
        case ButtonPress:
            posx = call_data->event->xbutton.x;
            posy = call_data->event->xbutton.y;
            mode = MOUSE_STATIC;
            break;

        case ButtonRelease:
            if ( mode == MOUSE_STATIC )
            {
	        identify_only = ( call_data->event->xbutton.state & ShiftMask
		                  || call_data->event->xbutton.state 
				     & ControlMask );
		
                switch( call_data->event->xbutton.button )
                {
                    case Button1:
                        ident = select_item( NODE_T, posx, posy, identify_only, 
			                     env.curr_analy );
			obj_type = NODE_T;
                        break;
                    case Button2:
                        if ( env.curr_analy->pick_beams )
			{
                            ident = select_item( BEAM_T, posx, posy, 
			                         identify_only, 
			                         env.curr_analy );
			    obj_type = BEAM_T;
			}
                        else
			{
                            ident = select_item( SHELL_T, posx, posy, 
			                         identify_only, 
			                         env.curr_analy );
			    obj_type = SHELL_T;
			}
                        break;
                    case Button3:
                        ident = select_item( BRICK_T, posx, posy, identify_only, 
			                     env.curr_analy );
			obj_type = BRICK_T;
                        break;
                }
		
		if ( identify_only && ident > 0 )
		    if ( call_data->event->xbutton.state & ShiftMask )
		    {
			sprintf( str, "tellpos %s %d", obj_type_names[obj_type], 
			         ident );
		        parse_command( str, env.curr_analy );
		    }
		    else
		    {
		        ident--;
		        switch ( obj_type )
			{
			    case BEAM_T:
			        select_mtl_mgr_mtl( 
				    env.curr_analy->geom_p->beams->mat[ident] );
				break;
			    case SHELL_T:
			        select_mtl_mgr_mtl( 
				    env.curr_analy->geom_p->shells->mat[ident] );
				break;
			    case BRICK_T:
			        select_mtl_mgr_mtl( 
				    env.curr_analy->geom_p->bricks->mat[ident] );
				break;
			    default:
			        popup_dialog( INFO_POPUP, 
				              "To select a material, %s\n%s", 
					      "please pick", 
					      "an element instead of a node." );
				break;
			}
		    }
            }
            else
            {
	        unset_alt_cursor();
                update_display( env.curr_analy );
            }
            break;

        case MotionNotify:
	    set_alt_cursor( CURSOR_FLEUR );
            mode = MOUSE_MOVE;
            orig_posx = posx;
            orig_posy = posy;
            posx = call_data->event->xbutton.x;
            posy = call_data->event->xbutton.y;
            if ( call_data->event->xmotion.state & Button1Mask )
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
            }
            else if ( call_data->event->xmotion.state & Button2Mask )
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
            }
            else if ( call_data->event->xmotion.state & Button3Mask )
            {
                dx = (posx - orig_posx) / 700.0;
                dy = - (posy - orig_posy) / 700.0;
                inc_mesh_trans( dx, dy, 0.0 );
                sprintf( str, "tx %f", dx );
                history_command( str );
                sprintf( str, "ty %f", dy );
                history_command( str );
            }

            /* Fast wireframe display. */
            quick_update_display( env.curr_analy );
            break;
    }
}


/*****************************************************************
 * TAG( parse_CB )
 *
 * The callback for handling commands entered in the command dialog.
 */
static void
parse_CB( w, client_data, call_data )
Widget w;
caddr_t client_data;
XmCommandCallbackStruct *call_data;
{
    char text[200];

    string_convert( call_data->value, text );

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
mtl_func_select_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmToggleButtonCallbackStruct *call_data;
{
    Widget *function_toggles;
    Boolean set, comp_set, color_set, sensitive;
    int i;
    Mtl_mgr_glo_sel_type compliment;
    
    function_toggles = (Widget *) client_data;
    
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
        if ( call_data->set )
	{
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
	    switch_opengl_win( MESH );
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

	    switch_opengl_win( MESH );
	    XtUnmanageChild( swatch_frame );
	    XtUnmapWidget( swatch_label );
	    XtSetSensitive( color_editor, False );
	    
	    if ( preview_set )
	    {
		/* Cancel the preview. */
		send_mtl_cmd( "mtl cancel", 2 );
	        XtSetSensitive( mtl_row_col, True );
	        XtSetSensitive( quick_select, True );
		preview_set = FALSE;
	    }
	}
	else if ( set )
	    XtVaSetValues( function_toggles[compliment], XmNset, False, NULL );
    }
    
    update_actions_sens();
}


/*****************************************************************
 * TAG( mtl_quick_select_CB )
 *
 * The callback for the material manager quick select buttons 
 * which allow certain selections among the entire set of 
 * materials in one step.
 */
static void
mtl_quick_select_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmPushButtonCallbackStruct call_data;
{
    Widget **ctl_buttons;
    Boolean set, vis_set, invis_set, enable_set, disable_set;
    Bool_type *mtl_invis, *mtl_disable;
    int qty_mtls, i;
    WidgetList children;
    Material_list_obj *p_mtl, *old_selects;
    
    qty_mtls = env.curr_analy->num_materials;
    mtl_invis = env.curr_analy->hide_material;
    mtl_disable = env.curr_analy->disable_material;
    ctl_buttons = (Widget **) client_data;
    XtVaGetValues( mtl_row_col, XmNchildren, &children, NULL );
    old_selects = mtl_select_list;
    
    if ( w == ctl_buttons[1][ALL] )
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
    
    if ( mtl_select_list == NULL )
        cur_mtl = 0;
	
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
mtl_select_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmToggleButtonCallbackStruct *call_data;
{
    int mtl;
    Material_list_obj *p_mtl, *old_selects;
    
    mtl = (int) client_data;
    old_selects = mtl_select_list;
    
    /* 
     * Material list nodes are conserved between the select and deselect
     * lists and the two lists should reflect exactly which toggles are 
     * set and unset.  However, very quick clicking on a toggle seems to
     * be able to cause, for example, two deselect requests on the same
     * toggle sequentially, which can cause an apparent inconsistency in
     * the lists.  The tests below for empty lists and unfound materials
     * are an attempt to avoid problems for "quick-clickers".
     */
    if ( call_data->set )
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
	    if ( mtl_select_list == NULL )
	        cur_mtl = 0;
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
col_comp_disarm_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmToggleButtonCallbackStruct *call_data;
{
    Material_property_type comp;
    int i;
    Material_property_type j;
    
    comp = (Material_property_type) client_data;
   
    /* Find which component has been set/unset. */
    for ( j = AMBIENT; j < MTL_PROP_QTY; j++ )
        if ( w == color_comps[j] )
            break;
    
    if ( call_data->set )
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



/*****************************************************************
 * TAG( expose_swatch_CB )
 *
 * The callback for the material manager color editor which
 * redraws the swatch on expose.
 */
static void
expose_swatch_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
GLwDrawingAreaCallbackStruct *call_data;
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
init_swatch_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
GLwDrawingAreaCallbackStruct *call_data;
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
    
    switch_opengl_win( MESH );
}


/*****************************************************************
 * TAG( col_ed_scale_CB )
 *
 * The callback for the material manager color editor color and 
 * shininess scales which updates the numeric display.
 */
static void
col_ed_scale_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmScaleCallbackStruct *call_data;
{
    XmString sval;
    char valbuf[5];
    Color_editor_scale_type scale;
    GLfloat fval;
    
    scale = (Color_editor_scale_type) client_data;
    
    if ( scale != SHININESS_SCALE )
    {
        fval = (GLfloat) call_data->value / 100.0;
        sprintf( valbuf, "%4.2f", fval );
    }
    else
	sprintf( valbuf, "%d", (int) call_data->value );

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
col_ed_scale_update_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmScaleCallbackStruct *call_data;
{
    Color_editor_scale_type scale;
    GLfloat fval;
    int i, ival;
    
    fval = 
    scale = (Color_editor_scale_type) client_data;
    
    if ( scale == SHININESS_SCALE )
    {
	prop_val_changed[SHININESS] = TRUE;
	property_vals[SHININESS][0] = (GLfloat) call_data->value;
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
	        property_vals[cur_mtl_comp][i] = (GLfloat) call_data->value 
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
mtl_func_operate_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmPushButtonCallbackStruct call_data;
{
    Mtl_mgr_op_type op;
    char *p_src, *p_dest;
    int len, t_cnt, token_cnt, i;
    
    op = (Mtl_mgr_op_type) client_data;
    token_cnt = 0;
    
    switch_opengl_win( MESH );
    
    memset( mtl_mgr_cmd, 0, 128 + env.curr_analy->num_materials * 4 );
    
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
	    XtSetSensitive( quick_select, False );
	    XtSetSensitive( mtl_row_col, False );
	    
	    preview_set = TRUE;
	    break;
	    
	case OP_CANCEL:
	    for ( p_src = "cancel "; *p_dest = *p_src; p_src++, p_dest++ );
	    token_cnt++;
		
	    send_mtl_cmd( mtl_mgr_cmd, token_cnt );
		
	    preview_set = FALSE;
	    
	    XtSetSensitive( quick_select, True );
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
	        XtSetSensitive( quick_select, True );
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
	        XtSetSensitive( quick_select, True );
	        XtSetSensitive( mtl_row_col, True );
	    }
	    break;
	    
	default:
	    wrt_text( "Unknown Material Manager operation; ignored.\n" );
    }

    update_actions_sens();
    
    if ( XtIsSensitive( color_editor ) )
        switch_opengl_win( SWATCH );
}


/*****************************************************************
 * TAG( destroy_mtl_mgr_CB )
 *
 * Callback to destroy allocated resources for the material 
 * manager.
 */
static void
destroy_mtl_mgr_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmAnyCallbackStruct call_data;
{
    Material_property_type i;
    
    switch_opengl_win( MESH );
    
    if ( preview_set )
        send_mtl_cmd( "mtl cancel", 2 );
    
    for ( i = 0; i < MTL_PROP_QTY; i++ )
    {
        free( property_vals[i] );
	property_vals[i] = NULL;
    }
    
    DELETE_LIST( mtl_select_list );
    DELETE_LIST( mtl_deselect_list );
    mtl_select_list = NULL;
    mtl_deselect_list = NULL;
    
    free( mtl_mgr_cmd );
    mtl_mgr_cmd = NULL;
    
    XFreePixmap( dpy, check );
    check = NULL;

    mtl_mgr_widg = NULL;
}


/*****************************************************************
 * TAG( destroy_util_panel_CB )
 *
 * Callback to destroy allocated resources for the utility 
 * panel.
 */
static void
destroy_util_panel_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmAnyCallbackStruct call_data;
{
    step_stride = 1;
    
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

    util_panel_widg = NULL;
}


/*****************************************************************
 * TAG( util_render_CB )
 *
 * Callback for the utility rendering function toggles.
 */
static void
util_render_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmToggleButtonCallbackStruct *call_data;
{
    Util_panel_button_type btn;
    
    btn = (Util_panel_button_type) client_data;
    
    switch( btn )
    {
	case VIEW_SOLID:
	    if ( call_data->set )
	    {
	        parse_command( "sw solid", env.curr_analy );
	        if ( all_true( env.curr_analy->hide_material, 
	                       env.curr_analy->num_materials ) )
	            parse_command( "mtl vis all", env.curr_analy );
	    }
	    else
	        /* Explicitly unsetting filled view. Do something... */
		parse_command( "sw none", env.curr_analy );
	    break;
	case VIEW_SOLID_MESH:
	    if ( call_data->set )
	    {
	        parse_command( "sw hidden", env.curr_analy );
	        if ( all_true( env.curr_analy->hide_material, 
	                       env.curr_analy->num_materials ) )
	            parse_command( "mtl vis all", env.curr_analy );
	    }
	    else
	        /* Explicitly unsetting mesh view. Do something... */
		parse_command( "sw none", env.curr_analy );
	    break;
	case VIEW_EDGES:
	    if ( call_data->set )
	    {
            util_panel_CB_active = TRUE;
	        parse_command( "sw none", env.curr_analy );
            util_panel_CB_active = FALSE;
                parse_command( "on edges", env.curr_analy );
	    }
	    else
	        parse_command( "off edges", env.curr_analy );
	    break;
	case PICK_MODE_SELECT:
	    if ( call_data->set )
	        parse_command( "sw picsel", env.curr_analy );
	    break;
	case PICK_MODE_HILITE:
	    if ( call_data->set )
	        parse_command( "sw pichil", env.curr_analy );
	    break;
	case BTN2_BEAM:
	    if ( call_data->set )
	        parse_command( "sw picbm", env.curr_analy );
	    break;
	case BTN2_SHELL:
	    if ( call_data->set )
	        parse_command( "sw picsh", env.curr_analy );
	    break;
    }
}


/*****************************************************************
 * TAG( step_CB )
 *
 * Callback for the utility panel state step buttons.
 */
static void
step_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmAnyCallbackStruct *call_data;
{
    Util_panel_button_type btn;
    Analysis *analy;
    int st_num;
    char cmd[16];
    
    btn = (Util_panel_button_type) client_data;
    analy = env.curr_analy;
    
    switch( btn )
    {
	case STEP_FIRST:
	    parse_command( "f", analy );
	    break;
	case STEP_DOWN:
	    st_num = analy->cur_state + 1 - step_stride;
	    sprintf( cmd, "state %d", st_num );
	    parse_command( cmd, analy );
	    break;
	case STEP_UP:
	    st_num = analy->cur_state + 1 + step_stride;
	    sprintf( cmd, "state %d", st_num );
	    parse_command( cmd, analy );
	    break;
	case STEP_LAST:
	    parse_command( "l", analy );
	    break;
    }
}


/*****************************************************************
 * TAG( stride_CB )
 *
 * Callback for the utility panel step stride display and controls.
 */
static void
stride_CB( w, client_data, call_data )
Widget w;
XtPointer client_data;
XmAnyCallbackStruct *call_data;
{
    char stride_str[8];
    Util_panel_button_type stride_mod;
    XmString lab;
    int sval;
    char *text;
    
    stride_mod = (Util_panel_button_type) client_data;
    
    switch ( stride_mod )
    {
        case STRIDE_INCREMENT:
	    if ( step_stride < env.curr_analy->num_states )
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
	    if ( sval > env.curr_analy->num_states || sval < 1 )
	    {
		sprintf( stride_str, "%d", step_stride );
		XmTextSetString( stride_label, stride_str );
	    }
	    else 
	        step_stride = sval;
    }
}


/*****************************************************************
 * TAG( create_app_widg )
 *
 * Routine to create additional standalone interface widgets.
 */
static void
create_app_widg( btn )
Btn_type btn;
{
    Widget *app_widg;
    Widget (*create_func)();
    WidgetList children;
    
    /* Assign parameters for creation. */
    if ( btn == BTN_MTL_MGR )
    {
	app_widg = &mtl_mgr_widg;
	create_func = create_mtl_manager;
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
update_util_panel( button )
Util_panel_button_type button;
{
    Boolean set;
    Bool_type all_invis;
    
    /* 
     * If utility panel not up or this called as a result of 
     * button activity on utility panel, nothing to do.
     */
    if ( util_panel_widg == NULL || util_panel_CB_active )
        return;
    
    /*
     * Render_modes "point cloud" and "none" are not currently
     * available as buttons, so don't try to set them.
     */
    if ( button != VIEW_POINT_CLOUD && button != VIEW_NONE )
    {
        XtVaGetValues( util_render_btns[button], XmNset, &set, NULL );
        if ( !set && button != VIEW_EDGES )
            XtVaSetValues( util_render_btns[button], XmNset, True, NULL );
    }
    
    switch( button )
    {
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
	    
	    if ( env.curr_analy->render_mode == RENDER_NONE
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
	    update_util_panel( VIEW_EDGES );
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
	case BTN2_SHELL:
	    XtVaGetValues( util_render_btns[BTN2_BEAM], XmNset, &set, NULL );
	    if ( set )
	        XtVaSetValues( util_render_btns[BTN2_BEAM], XmNset, False, 
			       NULL );
	    break;
	case BTN2_BEAM:
	    XtVaGetValues( util_render_btns[BTN2_SHELL], XmNset, &set, NULL );
	    if ( set )
	        XtVaSetValues( util_render_btns[BTN2_SHELL], XmNset, False, 
			       NULL );
	    break;
    }
}


/*****************************************************************
 * TAG( select_mtl_mgr_mtl )
 *
 * "Software" select of a Material Manager material.
 */
static void
select_mtl_mgr_mtl( mtl )
int mtl;
{
    Boolean set;
    WidgetList children;
    Material_list_obj *p_mtl, **src_list, **dest_list;
    
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
	src_list = &mtl_select_list;
	dest_list = &mtl_deselect_list;
    }
    else
    {
        /* Toggle from deselected to selected. */
    	XtVaSetValues( children[mtl], XmNset, True, NULL );
	src_list = &mtl_deselect_list;
	dest_list = &mtl_select_list;
    }
    
    /* Find material's entry in the appropriate list. */
    for ( p_mtl = *src_list; p_mtl != NULL; p_mtl = p_mtl->next )
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
    UNLINK( p_mtl, *src_list );
    INSERT( p_mtl, *dest_list );

    if ( mtl_select_list == NULL )
        cur_mtl = 0;
		
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
destroy_mtl_mgr()
{
    
    if ( mtl_mgr_widg == NULL )
        return;

    XtDestroyWidget( mtl_mgr_widg );
}


/*****************************************************************
 * TAG( update_swatch_label )
 *
 * Update the material swatch label.
 */
static void
update_swatch_label()
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
mtl_func_active()
{
    int i;
    
    for ( i = 0; i < MTL_FUNC_QTY; i++ )
	if ( XmToggleButtonGetState( mtl_mgr_func_toggles[i] ) )
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
set_scales_to_mtl()
{
    int idx;
    static GLfloat black[] = { 0.0, 0.0, 0.0, 0.0 };
    Boolean rgb_sens;
    GLfloat *prop_val;
    
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
        idx = (mtl_select_list->mtl - 1) % MAX_MATERIALS;
        
	/*
	 * Set shininess according to value-changed flag.  If this routine
	 * is called to initialize the rgb scales, we don't want an
	 * adjusted shininess value to be overwritten with the current
	 * material value.
	 */
	if ( prop_val_changed[SHININESS] )
	    set_shininess_scale( property_vals[SHININESS][0] );
	else
	    set_shininess_scale( v_win->matl_shininess[idx] );
	
	/* Never update if scales are insensitive. */
	if ( rgb_sens )
	{
	    if ( prop_val_changed[cur_mtl_comp] )
	        set_rgb_scales( property_vals[cur_mtl_comp] );
	    else
	        switch( cur_mtl_comp )
		{
		    case AMBIENT:
		        set_rgb_scales( v_win->matl_ambient[idx] );
			break;
		    case DIFFUSE:
		        set_rgb_scales( v_win->matl_diffuse[idx] );
			break;
		    case SPECULAR:
		        set_rgb_scales( v_win->matl_specular[idx] );
			break;
		    case EMISSIVE:
		        set_rgb_scales( v_win->matl_emission[idx] );
			break;
		    default:
		        set_rgb_scales( black );
		}
	    
            cur_mtl = ( cur_mtl_comp == MTL_PROP_QTY ) 
	              ? 0 : mtl_select_list->mtl;
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
set_rgb_scales( col )
GLfloat col[4];
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
set_shininess_scale( shine )
GLfloat shine;
{
    XmString sval;
    char valbuf[5];
    int i, min, max;
    
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
send_mtl_cmd( cmd, tok_qty )
char *cmd;
int tok_qty;
{
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
	    
	    parse_command( cbuf, env.curr_analy );
	    
	    free( cbuf );
	    
	    p_c += j;
	    sent += limit;
	}
    }
    else
        parse_command( cmd, env.curr_analy );
}


/*****************************************************************
 * TAG( load_mtl_mgr_funcs )
 *
 * Copy material manager function command strings into buffer
 * for active functions.
 */
static int
load_mtl_mgr_funcs( p_buf, p_token_cnt )
char *p_buf;
int *p_token_cnt;
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
 * TAG( load_selected_mtls )
 *
 * Copy material manager selected materials into buffer as text.
 */
static int
load_selected_mtls( p_buf, p_tok_cnt )
char *p_buf;
int *p_tok_cnt;
{
    char *p_src, *p_dest, *p_init;
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
    
    return (int) (p_dest - p_init);
}


/*****************************************************************
 * TAG( load_mtl_properties )
 *
 * Copy material manager color editor properties as text for
 * those material properties which user has edited.
 */
static int
load_mtl_properties( p_buf, p_tok_cnt )
char *p_buf;
int *p_tok_cnt;
{
    int i, c_cnt, c_add, t_cnt;
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
update_actions_sens()
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
 * TAG( gress_mtl_mgr )
 *
 * Event handler for when pointer enters or leaves the material 
 * manager to set the proper rendering context and window.
 */
static void
gress_mtl_mgr( w, client_data, event, continue_dispatch )
Widget w;
XtPointer client_data;
XEvent *event;
Boolean *continue_dispatch;
{
    if ( event->xcrossing.type == EnterNotify && XtIsSensitive( color_editor ) )
        switch_opengl_win( SWATCH );
    else
        switch_opengl_win( MESH );
}


/*****************************************************************
 * TAG( action_create_app_widg )
 *
 * Action routine for creating the Material Manager or Utility
 * Panel.
 */
static void
action_create_app_widg( w, event, params, qty )
Widget w;
XEvent *event;
String params[];
int *qty;
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


/*****************************************************************
 * TAG( resize_mtl_scrollwin )
 *
 * Resize action routine for the material manager ScrolledWindow.
 */
static void
resize_mtl_scrollwin( w, event, params, qty )
Widget w;
XEvent *event;
String params[];
int qty;
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
    rows = (short) ceil( (float) env.curr_analy->num_materials / max_cols );
    XtVaSetValues( row_col, XmNnumColumns, rows, NULL );
}


/*****************************************************************
 * TAG( string_convert )
 *
 * Convert a compound Motif string to a normal "C" string.
 */
static void
string_convert( str, buf )
XmString str;
char *buf;
{
    XmStringContext context;
    char *text, *p;
    XmStringCharSet charset;
    XmStringDirection direction;
    Boolean separator;

    if ( !XmStringInitContext( &context, str ) )
    {
        XtWarning( "Can't convert compound string." );
        return;
    }

    /* The running pointer is p. */
    p = buf;

    while ( XmStringGetNextSegment( context, &text, &charset, &direction,
                                    &separator ) )
    {
        p += (strlen(strcpy(p, text)));

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


/*****************************************************************
 * TAG( exit_CB )
 *
 * The callback for the error dialog that kills FIZ after the user
 * pushes the "Exit" (ok) button.
 */
static void
exit_CB( w, client_data, reason )
Widget w;
XtPointer client_data;
XmAnyCallbackStruct reason;
{
    quit( -1 );
}


/*****************************************************************
 * TAG( init_animate_workproc )
 *
 * Start up the animation work proc.  This is used so that
 * view controls will stay active while the animation happens.
 */
void
init_animate_workproc()
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
end_animate_workproc( remove_workproc )
Bool_type remove_workproc;
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
set_window_size( w_width, w_height )
int w_width;
int w_height;
{
    window_width = (Dimension) w_width;
    window_height = (Dimension) w_height;

    return;
}


/*****************************************************************
 * TAG( get_monitor_width )
 *
 * Query the monitor widget and return its current character width.
 */
int
get_monitor_width()
{
    int n;
    short ncols;
    Arg args[1];

    n = 0;
    XtSetArg( args[n], XmNcolumns, &ncols ); n++;
    XtGetValues( monitor_widg, args, n );

    return (int) ncols;
}


/*****************************************************************
 * TAG( write_start_text )
 *
 * Write start-up text messages to the monitor window.
 */
static void
write_start_text()
{
    struct stat statbuf;
    FILE *text_file;
    char *griz_home;
    char start_text[5][80], text_line[132], file_spec[132];
    int i, cnt;

    /*
     * Look for a start-up text file and if it exists, pump it out for
     * the user to see.
     */
    griz_home = getenv( "GRIZHOME" );
    if ( griz_home != NULL )
    {
        sprintf( file_spec, "%s/griz_start_text", griz_home );
        if ( stat( file_spec, &statbuf ) == 0 )
        {
	    text_file = fopen( file_spec, "r" );

	    if ( text_file )
	    {
	        /*
	         * The start-up text file exists and is open, so copy its
	         * contents into the monitor widget line-by-line.
	         */
	        while ( fgets( text_line, sizeof( text_line ), text_file ) )
	        {
		    XmTextInsert( monitor_widg, wpr_position, text_line );
                    wpr_position += strlen( text_line );
	        }
            }
        }
    }

    /* This is standard start text. */

    cnt = 0;
    sprintf( start_text[cnt++], "\nData file: %s\n",
	     env.curr_analy->root_name );
    if ( env.curr_analy->num_states > 0 )
    {
        sprintf( start_text[cnt++], "Start time: %.4e\n",
	         env.curr_analy->state_times[0] );
        sprintf( start_text[cnt++], "End time: %.4e\n",
	         env.curr_analy->state_times[env.curr_analy->num_states-1] );
    }
    else
    {
        sprintf( start_text[cnt++], "Start time: NULL\n" );
        sprintf( start_text[cnt++], "End time: NULL\n" );
    }
    sprintf( start_text[cnt++], "Number of states: %d\n",
	     env.curr_analy->num_states );
    sprintf( start_text[cnt++], "Number of polygons to render: %d\n\n",
	     ( env.curr_analy->geom_p->shells == NULL )
	     ? env.curr_analy->face_cnt
	       : env.curr_analy->face_cnt
	         + env.curr_analy->geom_p->shells->cnt );

    for ( i = 0; i < cnt; i++ )
    {
        XmTextInsert( monitor_widg, wpr_position, start_text[i] );
        wpr_position += strlen( start_text[i] );
    }

    return;
}


/*****************************************************************
 * TAG( wrt_text )
 *
 * Write a formatted string to a scrolled text widget if GUI is
 * active, else to stderr. (Essentially "wprint" from O'Reilly's
 * "Motif Programming Manual".)
 */
/*VARARGS*/
void
wrt_text( va_alist )
va_dcl
{
    char msgbuf[BUFSIZ]; /* we're not getting huge strings */
    char *fmt;
    va_list args;

    va_start( args );
    fmt = va_arg( args, char * );
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
        XmTextShowPosition( monitor_widg, wpr_position );

        if ( text_history_invoked )
           fprintf( text_history_file, "%s", msgbuf );
    }
    else
        fprintf( stderr, msgbuf );
}


/*****************************************************************
 * TAG( remove_widget_CB )
 *
 * Callback to remove a widget's handle from a list and delete
 * the widget.
 */
static void
remove_widget_CB( w, client_data, reason )
Widget w;
Widget_list_obj *client_data;
XmAnyCallbackStruct reason;
{
    XtDestroyWidget( w );
    DELETE( client_data, popup_dialog_list );
    popup_dialog_count--;
}


/*****************************************************************
 * TAG( popup_dialog )
 *
 * Popup a warning dialog with the error message.
 */
/*VARARGS*/
void
popup_dialog( va_alist )
va_dcl
{
    va_list vargs;
    char msgbuf[BUFSIZ]; /* we're not getting huge strings */
    char *fmt;
    Popup_Dialog_Type dtype;
    Widget_list_obj *p_wlo;
    char dialog_msg[256];
    XmString dialog_string;
    int n;
    Arg args[5];
    Position pop_x, pop_y;
    static Position first_popup_x, first_popup_y;
    static int x_incr = 20;
    static int y_incr = 20;

    /* 
     * Process the arguments. "dtype" will always be the first argument, 
     * so when converting to ANSI C and stdargs, it should be listed
     * explicitly in the argument list as the first argument.
     */
    va_start( vargs );
    dtype = (Popup_Dialog_Type) va_arg( vargs, int );
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
	fprintf( stderr, "%s\n", dialog_msg );
	return;
    }

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
    XtSetArg( args[n], XmNmessageString, dialog_string ); n++;
    XtSetArg( args[n], XmNx, first_popup_x + popup_dialog_count * x_incr ); n++;
    XtSetArg( args[n], XmNy, first_popup_y + popup_dialog_count * y_incr ); n++;
    p_wlo->handle = ( dtype == WARNING_POPUP )
                    ? XmCreateWarningDialog( ctl_shell_widg, "Warning", args, n )
		    : XmCreateInformationDialog( ctl_shell_widg, "Information", 
		                                 args, n );

    /* ...make it look and act right */
    XtAddCallback( p_wlo->handle, XmNokCallback, remove_widget_CB, p_wlo );
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
}


/*****************************************************************
 * TAG( clear_popup_dialogs )
 *
 * Destroy any extant popup dialogs referenced in the popup list.
 */
void
clear_popup_dialogs()
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
popup_fatal( message )
{
    Widget error_dialog, dialog_shell;
    char error_msg[128];
    XmString error_string, ok_label;
    int n;
    Arg args[5];
    Atom WM_DELETE_WINDOW;

    sprintf( error_msg, "FATAL ERROR:\n%s", message );

    if ( !gui_up )
    {
	fprintf( stderr, error_msg );
	quit( -1 );
    }

    /* Keep any sneaky workproc's from sliding by...*/
    end_animate_workproc( TRUE );

    error_string = XmStringCreateLtoR( error_msg, XmSTRING_DEFAULT_CHARSET );

    ok_label = XmStringCreateSimple( "Exit" );

    n = 0;
    XtSetArg( args[n], XmNmessageString, error_string ); n++;
    XtSetArg( args[n], XmNokLabelString, ok_label ); n++;
    XtSetArg( args[n], XmNdeleteResponse, XmDO_NOTHING ); n++;
    error_dialog = XmCreateErrorDialog( ctl_shell_widg, "error", args, n );
    XtAddCallback( error_dialog, XmNokCallback, exit_CB, NULL );

    /*
     * Link the window manager "close" function to the exit callback to
     * prevent the user from using the window manager to close the popup
     * and then trying to continue with GRIZ.
     */
    dialog_shell = XtParent( error_dialog );
    WM_DELETE_WINDOW = XmInternAtom( XtDisplay( ctl_shell_widg ),
				     "WM_DELETE_WINDOW", FALSE );
    XmAddWMProtocolCallback( dialog_shell, WM_DELETE_WINDOW, exit_CB,
			     error_dialog );

    XtUnmanageChild(
        XmMessageBoxGetChild( error_dialog, XmDIALOG_CANCEL_BUTTON ) );
    XtUnmanageChild(
        XmMessageBoxGetChild( error_dialog, XmDIALOG_HELP_BUTTON ) );

    XmStringFree( error_string );
    XmStringFree( ok_label );
    XtAddGrab( XtParent( error_dialog ), TRUE, FALSE );
    XtManageChild( error_dialog );
}


/******************************************************************
 * TAG( reset_window_titles )
 *
 * Reset the title to reflect a new database.
 */
void
reset_window_titles()
{
    char title[100];

    sprintf( title, "%s Control:  %s\n", griz_name, env.plotfile_name );
    XtVaSetValues( ctl_shell_widg, XmNtitle, title, NULL );

    sprintf( title, "%s Render:  %s\n", griz_name, env.plotfile_name );
    XtVaSetValues( rendershell_widg, XmNtitle, title, NULL );
}


/*****************************************************************
 * TAG( quit )
 *
 * Quit the application.
 */
void
quit( return_code )
int return_code;
{
    close_history_file();
    exit( return_code );
}


/*****************************************************************
 * TAG( switch_opengl_win )
 *
 * Switch among OpenGL rendering windows and their contexts.
 */
void
switch_opengl_win( opengl_win )
OpenGL_win opengl_win;
{
    switch ( opengl_win )
    {
	case MESH:
	    if ( cur_opengl_win != MESH )
	    {
                glXMakeCurrent( dpy, XtWindow( ogl_widg[MESH] ), render_ctx );
		cur_opengl_win = MESH;
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
            glXMakeCurrent( dpy, XtWindow( ogl_widg[MESH] ), render_ctx );
	    cur_opengl_win = MESH;
    }
}


/*****************************************************************
 * TAG( init_alt_cursors )
 *
 * Define alternative cursor resources.
 */
static void
init_alt_cursors()
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
set_alt_cursor( cursor_type )
Cursor_type cursor_type;
{
    /* Always set for rendering window and control window. */
    XDefineCursor( dpy, XtWindow( ogl_widg[MESH] ), alt_cursors[cursor_type] );
    XDefineCursor( dpy, XtWindow( ctl_shell_widg ), alt_cursors[cursor_type] );
    
    /* Also set for Material Manager and (standalone) Utility Panel. */
    if ( mtl_mgr_widg != NULL )
        XDefineCursor( dpy, XtWindow( ctl_shell_widg ), 
	               alt_cursors[cursor_type] );
    if ( !include_util_panel && util_panel_widg != NULL )
        XDefineCursor( dpy, XtWindow( util_panel_widg ), 
	               alt_cursors[cursor_type] );

    XFlush( dpy );
}


/*****************************************************************
 * TAG( unset_alt_cursor )
 *
 * Set cursor to default.
 */
void
unset_alt_cursor()
{
    /* Always set for rendering window and control window. */
    XUndefineCursor( dpy, XtWindow( ogl_widg[MESH] ) );
    XUndefineCursor( dpy, XtWindow( ctl_shell_widg ) );
    
    /* Also set for Material Manager and (standalone) Utility Panel. */
    if ( mtl_mgr_widg != NULL )
        XUndefineCursor( dpy, XtWindow( ctl_shell_widg ) );
    if ( !include_util_panel && util_panel_widg != NULL )
        XUndefineCursor( dpy, XtWindow( util_panel_widg ) );

    XFlush( dpy );
}

