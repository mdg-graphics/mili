/* $Id$ */
/* 
 * draw.c - Routines for drawing meshes.
 * 
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Oct 23 1991
 *
 * Copyright (c) 1991 Regents of the University of California
 */

#ifdef sgi
#define TAN tanf
#define ATAN2 atan2f
#endif
#ifdef hpux
#define TAN tanf
#define ATAN2 atan2f
#endif
#ifdef __alpha
#define TAN tanf
#define ATAN2 atan2f
#endif
#ifndef TAN
#define TAN tan
#define ATAN2 atan2
#endif

#include "viewer.h"
#include "draw.h"
#include "image.h"

#define SCL_MAX ((float) CMAP_SIZE - 2.01)

typedef struct _Render_poly_obj
{
    struct _Render_poly_obj *next;
    struct _Render_poly_obj *prev;
    int cnt;
    float pts[4][3];
    float norm[4][3];
    float cols[4][4];
    float vals[4];
} Render_poly_obj;


/* Object which holds all the display information. */
View_win_obj *v_win = NULL;

/* Cache some constants that are needed for "good" interpolation. */
static Transf_mat cur_view_mat;
static float proj_param_x;
static float proj_param_y;

/* Vector lengths in pixels for drawing vector plots. */
#define VEC_2D_LENGTH 20.0
#define VEC_3D_LENGTH 100.0

/* Vertical spacing coeefficient and offset for text. */
#define LINE_SPACING_COEFF (1.25)
#define TOP_OFFSET (0.5)

/* Some annotation strings. */
char *strain_label[] = 
{ 
    "infinitesimal", "Green-Lagrange", "Almansi", "Rate" 
};
char *ref_surf_label[] = { "middle", "inner", "outer" };
char *ref_frame_label[] = { "global", "local" };


GLfloat black[] = {0.0, 0.0, 0.0};
GLfloat white[] = {1.0, 1.0, 1.0};
GLfloat red[] = {1.0, 0.0, 0.0};
GLfloat green[] = {0.0, 1.0, 0.0};
GLfloat blue[] = {0.0, 0.0, 1.0};
GLfloat magenta[] = {1.0, 0.0, 1.0};
GLfloat cyan[] = {0.0, 1.0, 1.0};
GLfloat yellow[] = {1.0, 1.0, 0.0};

#define MATERIAL_COLOR_CNT 20
static GLfloat material_colors[MATERIAL_COLOR_CNT][3] =
{ {0.933, 0.867, 0.51},    /* Light goldenrod */
  {0.529, 0.808, 0.922},   /* Sky blue */
  {0.725, 0.333, 0.827},   /* Medium orchid */
  {0.804, 0.361, 0.361},   /* Indian red */
  {0.4, 0.804, 0.667},     /* Medium aquamarine */
  {0.416, 0.353, 0.804},   /* Slate blue */
  {1.0, 0.647, 0.0},       /* Orange */
  {0.545, 0.271, 0.075},   /* Brown */
  {0.118, 0.565, 1.0},     /* Dodger blue */
  {0.573, 0.545, 0.341},   /* SeaGreen */
  {0.961, 0.871, 0.702},   /* Wheat */
  {0.824, 0.706, 0.549},   /* Tan */
  {1.0, 0.412, 0.706},     /* Hot pink */
  {0.627, 0.125, 0.941},   /* Purple */
  {0.282, 0.82, 0.8},      /* Medium turqoise */
  {0.902, 0.157, 0.157},   /* Red */
  {0.157, 0.902, 0.157},   /* Green */
  {0.902, 0.902, 0.157},   /* Yellow */
  {0.157, 0.902, 0.902},   /* Cyan */
  {0.902, 0.157, 0.902},   /* Magenta */
};


/* Local routines. */
static void look_rot_mat();
static void define_one_material();
static void color_lookup();
static void draw_grid();
static void draw_grid_2d();
static int check_for_tri_face();
static void get_min_max();
static void draw_hex();
static void draw_shells();
static void draw_beams();
static void draw_shells_2d();
static void draw_beams_2d();
static void draw_nodes();
static void draw_edges();
static void draw_hilite();
static void draw_elem_numbers();
static void draw_bbox();
static void draw_extern_polys();
static void draw_ref_polys();
static void draw_vec_result();
static void draw_vec_result_2d();
static void draw_node_vec();
static void load_vec_result();
static void find_front_faces();
static void draw_reg_carpet();
static void draw_vol_carpet();
static void draw_shell_carpet();
static void draw_ref_carpet();
static void draw_traces();
static void draw_trace_list();
static void draw_sphere();
static void draw_poly();
static void draw_edged_poly();
static void draw_plain_poly();
static float length_2d();
static void scan_poly();
static void draw_poly_2d();
static void draw_line();
static void draw_3d_text();
static void draw_foreground();
static void memory_to_screen();
static void get_verts_of_bbox();
static void hvec_copy();
static void antialias_lines();
static void begin_draw_poly();
static void end_draw_poly();
static void linear_variable_scale();

static double round();
static double tfloor();
static double machine_tolerance();



/*
 * SECTION_TAG( Grid window )
 */


/*****************************************************************
 * TAG( init_mesh_window )
 *
 * Initialize GL window for viewing the mesh.
 */
void
init_mesh_window( analy )
Analysis *analy;
{
    int i;

    /* Initialize the view window data structures. */
    v_win = NEW( View_win_obj, "View window" );

    /* Lighting is on for 3D and off for 2D. */
    if ( analy->dimension == 3 )
        v_win->lighting = TRUE;
    else
        v_win->lighting = FALSE;

    /* Orthographic or perspective depends on dimension. */
    if ( analy->dimension == 2 )
        set_orthographic( TRUE );
    else
        set_orthographic( FALSE );

    /* Initialize view. */
    VEC_SET( v_win->look_from, 0.0, 0.0, 20.0 );
    VEC_SET( v_win->look_at, 0.0, 0.0, 0.0 );
    VEC_SET( v_win->look_up, 0.0, 1.0, 0.0 );
    v_win->cam_angle = RAD_TO_DEG( 2.0*ATAN2(1.0, v_win->look_from[2]) );
    v_win->near = v_win->look_from[2] - 2.0;
    v_win->far = v_win->look_from[2] + 2.0;
    v_win->aspect_correct = 1.0;

    mat_copy( &v_win->rot_mat, &ident_matrix );
    VEC_SET( v_win->trans, 0.0, 0.0, 0.0 );
    VEC_SET( v_win->scale, 1.0, 1.0, 1.0 );

    bbox_nodes( analy, analy->bbox_source, TRUE, analy->bbox[0], 
                analy->bbox[1] );
    set_view_to_bbox( analy->bbox[0], analy->bbox[1], analy->dimension );

    /* A little initial rotation to make things look nice. */
    if ( analy->dimension == 3 )
    {
        inc_mesh_rot( 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, DEG_TO_RAD( 30 ) );
        inc_mesh_rot( 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, DEG_TO_RAD( 15 ) );
    }

    /* Set the default colors and load the result colormap. */
    set_color( "bg", 1.0, 1.0, 1.0 );            /* White. */
    set_color( "fg", 0.0, 0.0, 0.0 );            /* Black. */
    set_color( "con", 1.0, 0.0, 1.0 );           /* Magenta. */
    set_color( "hilite", 1.0, 0.0, 0.0 );        /* Red. */
    set_color( "select", 0.0, 1.0, 0.0 );        /* Green. */
    set_color( "rmin", 1.0, 0.0, 1.0 );          /* Magenta. */
    set_color( "rmax", 1.0, 0.0, 0.0 );          /* Red. */
    hot_cold_colormap();

    /* Select and load a Hershey font. */
    hfont( "futura.l" );

    /* Initialize video title array. */
    for ( i = 0; i < 4; i++ )
        v_win->vid_title[i][0] = '\0';

    /* Turn on Z-buffering for 3D but not for 2D. */
    if ( analy->dimension == 3 )
    {
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LEQUAL );
    }
    else
        glDisable( GL_DEPTH_TEST );

/*
glPolygonOffsetEXT( 0, .001 );*/
    /* Normalize normal vectors after transformation.  This is crucial. */
    if ( analy->dimension == 3 )
        glEnable( GL_NORMALIZE );

    /* Set the background color for clears. */
    glClearDepth( 1.0 );
    glClearColor( v_win->backgrnd_color[0],
                  v_win->backgrnd_color[1],
                  v_win->backgrnd_color[2], 0.0 );
    glClearStencil( 0 );
    
    /* Set alignment for glReadPixels during raster output operations. */
    glPixelStorei( GL_PACK_ALIGNMENT, (GLint) 1 );
    
    /* Ask for best line anti-aliasing. */
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

    /* Set up for perspective or orthographic viewing. */
    set_mesh_view();

    /* Initialize the GL model view matrix stack. */
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    /* Define material properties for each material. */
    define_materials( NULL, 0 );

    if ( v_win->lighting )
    {
        /* Some of these light values are the defaults, but they
         * are listed here explicitly so they can be changed later.
         */
        static GLfloat lmodel_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
        static GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
        static GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
        static GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
        static GLfloat light0_position[] = { 0.0, 0.0, 10.0, 0.0 };
        static GLfloat light1_position[] = { 10.0, 10.0, -25.0, 1.0 };
/**
        This creates a positional light as opposed to a directional light
        static GLfloat light1_position[] = { 10.0, 10.0, -25.0, 1.0 };

        This creates a directional light
        static GLfloat light1_position[] = { 10.0, 10.0, -25.0, 0.0 };
**/

        /* Default to one-sided lighting (this is the OpenGL default). */
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 0 );

        /* Enable lighting. */
        glEnable( GL_LIGHTING );

        /* Set the global ambient light level. */
        glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lmodel_ambient );

        /* Enable first light. */
        glLightfv( GL_LIGHT0, GL_AMBIENT, light_ambient );
        glLightfv( GL_LIGHT0, GL_DIFFUSE, light_diffuse );
        glLightfv( GL_LIGHT0, GL_SPECULAR, light_specular );
        glLightfv( GL_LIGHT0, GL_POSITION, light0_position );
        glEnable( GL_LIGHT0 );
        v_win->light_active[0] = TRUE;
        hvec_copy( v_win->light_pos[0], light0_position );

        /* Enable second light. */
        glLightfv( GL_LIGHT1, GL_AMBIENT, light_ambient );
        glLightfv( GL_LIGHT1, GL_DIFFUSE, light_diffuse );
        glLightfv( GL_LIGHT1, GL_SPECULAR, light_specular );
        glLightfv( GL_LIGHT1, GL_POSITION, light1_position );
        glEnable( GL_LIGHT1 );
        v_win->light_active[1] = TRUE;
        hvec_copy( v_win->light_pos[1], light1_position );

        /* Initialize the current material (the number is arbitrary). */
        v_win->current_material = 1;
        glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, v_win->matl_ambient[1] );
        glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, v_win->matl_diffuse[1] );
        glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, v_win->matl_specular[1]);
        glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, v_win->matl_emission[1]);
        glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS,
                     v_win->matl_shininess[1] );

        /* By default, leave lighting disabled. */
        glDisable( GL_LIGHTING );
    }

    /* Clear the mesh window. */
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
             GL_STENCIL_BUFFER_BIT );
}


/*****************************************************************
 * TAG( reset_mesh_window )
 *
 * Reset the GL window for viewing the mesh.  This routine is
 * called after a new analysis has been loaded.  The routine
 * resets the bbox scaling and translation.
 */
void
reset_mesh_window( analy )
Analysis *analy;
{
    bbox_nodes( analy, analy->bbox_source, TRUE, analy->bbox[0], 
                analy->bbox[1] );
    set_view_to_bbox( analy->bbox[0], analy->bbox[1], analy->dimension );

    /* In case data has changed from 3D to 2D or vice-versa. */
    set_mesh_view();

    /* Return to default material properties. */
    if ( analy->dimension == 3 )
        define_materials( NULL, 0 );
}


/*****************************************************************
 * TAG( get_pick_ray )
 *
 * From pick coordinates on the screen, get a ray that goes through
 * the scene.  Returns the eyepoint as the point on the ray and
 * the ray direction is in the view direction.
 */
void
get_pick_ray( sx, sy, pt, dir, analy )
int sx;
int sy;
float pt[3];
float dir[3];
Analysis *analy;
{
    Transf_mat view_mat;
    float ppt[3], eyept[3];
    float midx, midy, px, py, cx, cy;

    /*
     * Get the pick point and eye point in view coordinates.
     * Inverse transform the pick point and eye point into world
     * coordinates.  The ray is then computed from these two points.
     */
    midx = 0.5*v_win->vp_width + v_win->win_x;
    midy = 0.5*v_win->vp_height + v_win->win_y;
    px = (sx - midx)/(0.5 * v_win->vp_width);
    py = (sy - midy)/(0.5 * v_win->vp_height);
    get_foreground_window( &ppt[2], &cx, &cy );
    ppt[0] = px * cx;
    ppt[1] = py * cy;

    /* In Motif, Y direction of window is reverse that of GL. */
    ppt[1] = -ppt[1];

    /* Get the inverse viewing transformation matrix. */
    inv_view_transf_mat( &view_mat );

    if ( v_win->orthographic )
    {
        VEC_COPY( eyept, ppt );
        eyept[2] = 0.0;
    }
    else
    {
        VEC_SET( eyept, 0.0, 0.0, 0.0 );
    }

    point_transform( pt, ppt, &view_mat );
    VEC_COPY( ppt, pt );
    point_transform( pt, eyept, &view_mat );
    VEC_SUB( dir, ppt, pt );
}


/*
 * SECTION_TAG( View control )
 */


/*****************************************************************
 * TAG( set_orthographic )
 *
 * Select a perspective or orthographic view.
 */
void
set_orthographic( val )
Bool_type val;
{
    v_win->orthographic = val;
}


/*****************************************************************
 * TAG( set_mesh_view )
 *
 * Set up the perspective or orthographic view for the mesh window,
 * taking into account the dimensions of the current viewport and
 * the camera angle.
 */
void
set_mesh_view()
{
    GLint param[4];
    GLdouble xp, yp, cp;
    float aspect;

    /* Get the current viewport location and size. */
    glGetIntegerv( GL_VIEWPORT, param );
    v_win->win_x = param[0];
    v_win->win_y = param[1];
    v_win->vp_width = param[2];
    v_win->vp_height = param[3];

    /* Correct the aspect ratio for NTSC video, if requested. */
    aspect = v_win->aspect_correct * v_win->vp_width / (float)v_win->vp_height;

    /* Get the window dimensions at the near plane. */
    if ( v_win->orthographic )
        cp = 1.0;
    else
        cp = v_win->near * TAN( DEG_TO_RAD( 0.5*v_win->cam_angle ) );

    if ( aspect >= 1.0 )
    {
        xp = cp * aspect;
        yp = cp;
    }
    else
    {
        xp = cp;
        yp = cp / aspect;
    }

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    /*
     * We could special case 2D data, but choose not to for now.
     */
    if ( v_win->orthographic )
        glOrtho( -xp, xp, -yp, yp, v_win->near, v_win->far );
    else
        glFrustum( -xp, xp, -yp, yp, v_win->near, v_win->far );

    glMatrixMode( GL_MODELVIEW );
}


/************************************************************
 * TAG( aspect_correct )
 *
 * If video is TRUE, correct the screen aspect ratio for NTSC.
 * We found NTSC to have an aspect ratio of about x/y = 0.92.
 * Note that this means the image on the workstation screen won't
 * look right.  If video is false, the aspect is set to 1.0.
 */
void
aspect_correct( video, analy )
Bool_type video;
Analysis *analy;
{

    if ( video )
        v_win->aspect_correct = 0.92;
    else
        v_win->aspect_correct = 1.0;

    set_mesh_view();
}


/*****************************************************************
 * TAG( set_camera_angle )
 *
 * Set the view angle.
 */
void
set_camera_angle( angle, analy )
float angle;
Analysis *analy;
{
    v_win->cam_angle = angle;
    adjust_near_far( analy );
}


/*****************************************************************
 * TAG( set_view_to_bbox )
 *
 * This routine sets a scaling and translation of the mesh
 * so that it fits the viewport well.  Routine is passed the
 * bounding box of the mesh and the dimension of the mesh.
 * Routine can be called repeatedly.
 */
void
set_view_to_bbox( bb_lo, bb_hi, view_dimen )
float bb_lo[3];
float bb_hi[3];
int view_dimen;
{
    int i;
    float dimx, dimy, dimz, dim;

    for ( i = 0; i < 3; i++ )
        v_win->bbox_trans[i] = - ( bb_lo[i] + 0.5*(bb_hi[i]-bb_lo[i]) );

    dimx = 0.5*(bb_hi[0] - bb_lo[0]);
    dimy = 0.5*(bb_hi[1] - bb_lo[1]);
    dimz = 0.5*(bb_hi[2] - bb_lo[2]);
    dim = MAX( MAX( dimx, dimy ), dimz );

    if ( view_dimen == 3 )
        v_win->bbox_scale = 0.75 / dim;
    else  /* view_dimen == 2 */
        v_win->bbox_scale = 0.9 / dim;
}


/*****************************************************************
 * TAG( inc_mesh_rot )
 *
 * Increment the current mesh rotation.  Rotates about an axis
 * specified by a point and vector.  Angle is in radians.
 */
void
inc_mesh_rot( px, py, pz, vx, vy, vz, angle )
float px, py, pz, vx, vy, vz;
float angle;
{
    float pt[3];
    float vec[3];

    pt[0] = px;
    pt[1] = py;
    pt[2] = pz;
    vec[0] = vx;
    vec[1] = vy;
    vec[2] = vz;

    mat_rotate( &v_win->rot_mat, pt, vec, angle );
}


/*****************************************************************
 * TAG( inc_mesh_trans )
 *
 * Increment the current mesh translation.
 */
void
inc_mesh_trans( tx, ty, tz )
float tx, ty, tz;
{
    v_win->trans[0] += tx;
    v_win->trans[1] += ty;
    v_win->trans[2] += tz;
}


/*****************************************************************
 * TAG( set_mesh_scale )
 *
 * Set the current mesh scaling.
 */
void
set_mesh_scale( scale_x, scale_y, scale_z )
float scale_x;
float scale_y;
float scale_z;
{
    v_win->scale[0] = scale_x;
    v_win->scale[1] = scale_y;
    v_win->scale[2] = scale_z;
}


/*****************************************************************
 * TAG( get_mesh_scale )
 *
 * Returns the current mesh scaling factor.
 */
void
get_mesh_scale( scale )
float scale[3];
{
    VEC_COPY( scale, v_win->scale );
}


/*****************************************************************
 * TAG( reset_mesh_transform )
 *
 * Resets the mesh view to the default.
 */
void
reset_mesh_transform()
{
    int i;

    mat_copy( &v_win->rot_mat, &ident_matrix );
    for ( i = 0; i < 3; i++ )
        v_win->trans[i] = 0.0;
    VEC_SET( v_win->scale, 1.0, 1.0, 1.0 );
}


/*****************************************************************
 * TAG( look_from look_at look_up )
 *
 * Modify the mesh view so that the user is looking from one
 * point toward a second point, with the specified up vector in
 * the Y direction.
 */
void
look_from( pt )
float pt[3];
{
    VEC_COPY( v_win->look_from, pt );
}

void
look_at( pt )
float pt[3];
{
    VEC_COPY( v_win->look_at, pt );
}

void
look_up( vec )
float vec[3];
{
    VEC_COPY( v_win->look_up, vec );
}


/************************************************************
 * TAG( move_look_from )
 *
 * Move the look_from point along the specified axis (X == 0, etc.).
 */
void
move_look_from( axis, incr )
int axis;
float incr;
{
    v_win->look_from[axis] += incr;
}


/************************************************************
 * TAG( move_look_at )
 *
 * Move the look_at point along the specified axis (X == 0, etc.).
 */
void
move_look_at( axis, incr )
int axis;
float incr;
{
    v_win->look_at[axis] += incr;
}


/*****************************************************************
 * TAG( look_rot_mat )
 *
 * Creates a rotation matrix for the look from/at transform.
 */
static void
look_rot_mat( from_pt, at_pt, up_vec, rot_mat )
float from_pt[3];
float at_pt[3];
float up_vec[3];
Transf_mat *rot_mat;
{
    float v1[3], v2[3], v3[3];
    int i;

    /* Do the look at. */
    mat_copy( rot_mat, &ident_matrix );
    VEC_SUB( v3, from_pt, at_pt );
    vec_norm( v3 );
    VEC_CROSS( v1, up_vec, v3 );
    vec_norm( v1 );
    VEC_CROSS( v2, v3, v1 );
    vec_norm( v2 );

    for ( i = 0; i < 3; i++ )
    {
        rot_mat->mat[i][0] = v1[i];
        rot_mat->mat[i][1] = v2[i];
        rot_mat->mat[i][2] = v3[i];
    }
}


/*****************************************************************
 * TAG( adjust_near_far )
 *
 * Resets the near and far planes of the viewing fulcrum in order
 * to avoid cutting off any of the model.
 */
void
adjust_near_far( analy )
Analysis *analy;
{
    Transf_mat view_trans;
    float verts[8][3];
    float t_pt[3];
    float low_z, high_z;
    int i;
    float new_near, new_far;

    /*
     * Set up the viewing transformation matrix.
     */
    view_transf_mat( &view_trans );

    /*
     * Get the corners of the bounding box.
     */
    get_verts_of_bbox( analy->bbox, verts );

    /*
     * Transform corners and get near and far Z values.
     */
    point_transform( t_pt, verts[0], &view_trans );
    low_z = t_pt[2];
    high_z = t_pt[2];

    for ( i = 1; i < 8; i++ )
    {
        point_transform( t_pt, verts[i], &view_trans );
        if ( t_pt[2] < low_z )
            low_z = t_pt[2];
        else if ( t_pt[2] > high_z )
            high_z = t_pt[2];
    }

    new_near = -high_z - 1.0;
    new_far = -low_z + 1.0;

    /*
     * Near/far planes shouldn't go behind the eyepoint.
     */
    if ( new_near < 0.0 || new_far < new_near )
    {
        popup_dialog( WARNING_POPUP, "Unable to reset near/far planes.%s%s%s%s", 
	              "\nIf using material invisibility, try \"bbox vis\"", 
	              "\nto minimize the mesh bounding box then \"rnf\" again OR"
	              "\nuse \"info\" to see current near/far planes and", 
		      "\nadjust manually using \"near\" and \"far\"." );
	
    }
    else
    {
        v_win->near = new_near;
        v_win->far = new_far;
        set_mesh_view();
    }
}


/*****************************************************************
 * TAG( set_near )
 *
 * Set the position of the near cutting plane.
 */
void
set_near( near_dist )
float near_dist;
{
    v_win->near = near_dist;
}


/*****************************************************************
 * TAG( set_far )
 *
 * Set the position of the far cutting plane.
 */
void
set_far( far_dist )
float far_dist;
{
    v_win->far = far_dist;
}


/************************************************************
 * TAG( center_view )
 *
 * Center the view about a point specified in one of three
 * ways.
 */
void
center_view( analy )
Analysis *analy;
{
    Transf_mat view_trans;
    Nodal *nodes, *onodes;
    Beam_geom *beams;
    Shell_geom *shells;
    Hex_geom *bricks;
    float ctr_pt[3], opt[3];
    float *pt, *factors;
    int hi_type;
    int hi_num;
    int node;
    int i, j;
    Bool_type dscale;

    dscale = analy->displace_exag;
    if ( analy->displace_exag )
    {
	dscale = TRUE;
	onodes = analy->geom_p->nodes;
	factors = analy->displace_scale;
    }
    else
        dscale = FALSE;
    
    pt = analy->view_center;

    /* Get the centering point. */
    switch ( analy->center_view )
    {
	case NO_CENTER:
	    VEC_SET( pt, 0.0, 0.0, 0.0 );
	    break;

	case HILITE:
	    VEC_SET( pt, 0.0, 0.0, 0.0 );
	    if ( dscale )
	        VEC_SET( opt, 0.0, 0.0, 0.0 );
            nodes = analy->state_p->nodes;
            hi_type = analy->hilite_type;
            hi_num = analy->hilite_num;

            /* Nothing highlighted. */
            if ( hi_type == 0 )
                return;

            /* Get the centering point. */
            switch ( hi_type )
            {
                case 0:
                    /* Nothing hilighted. */
                    return;
                case 1:
                    for ( i = 0; i < 3; i++ )
		    {
                        pt[i] = nodes->xyz[i][hi_num];
			if ( dscale )
			    opt[i] = onodes->xyz[i][hi_num];
		    }
                    break;
                case 2:
                    beams = analy->geom_p->beams;
                    for ( i = 0; i < 2; i++ )
                        for ( j = 0; j < 3; j++ )
                            pt[j] += nodes->xyz[j][beams->nodes[i][hi_num]]
				     / 2.0;
		    if ( dscale )
			for ( i = 0; i < 2; i++ )
			    for ( j = 0; j < 3; j++ )
				opt[j] += onodes->xyz[j][beams->nodes[i][hi_num]]
					  / 2.0;
                    break;
                case 3:
                    /* Highlight a shell element. */
                    shells = analy->geom_p->shells;
                    for ( i = 0; i < 4; i++ )
                        for ( j = 0; j < 3; j++ )
                            pt[j] += nodes->xyz[j][shells->nodes[i][hi_num]]
				     / 4.0;
		    if ( dscale )
			for ( i = 0; i < 4; i++ )
			    for ( j = 0; j < 3; j++ )
				opt[j] += onodes->xyz[j][shells->nodes[i][hi_num]]
					  / 4.0;
                    break;
                case 4:
                    /* Highlight a brick element. */
                    bricks = analy->geom_p->bricks;
                    for ( i = 0; i < 8; i++ )
                        for ( j = 0; j < 3; j++ )
                            pt[j] += nodes->xyz[j][bricks->nodes[i][hi_num]]
				     / 8.0;
		    if ( dscale )
			for ( i = 0; i < 8; i++ )
			    for ( j = 0; j < 3; j++ )
				opt[j] += onodes->xyz[j][bricks->nodes[i][hi_num]]
					  / 8.0;
                    break;
            }
	    break;

	case NODE:
            nodes = analy->state_p->nodes;
            for ( i = 0; i < 3; i++ )
                pt[i] = nodes->xyz[i][analy->center_node];
	    if ( dscale )
		for ( i = 0; i < 3; i++ )
		    opt[i] = onodes->xyz[i][analy->center_node];
            break;
	
	case POINT:
            /* Do nothing, coords already in pt (i.e., analy->view_center). */
	    if ( dscale )
	        VEC_SET( opt, 0.0, 0.0, 0.0 );
            break;
	
	default:
	    return;
    }

    if ( dscale )
    {
	/* Scale the point's displacements. */
	for ( i = 0; i < 3; i++ )
	    pt[i] = opt[i] + factors[i] * (pt[i] - opt[i]);
    }

    view_transf_mat( &view_trans );
    point_transform( ctr_pt, pt, &view_trans );
    inc_mesh_trans( -ctr_pt[0], -ctr_pt[1], 
                    -((v_win->near + v_win->far) * 0.5 + ctr_pt[2]) );
}


/*****************************************************************
 * TAG( view_transf_mat )
 *
 * Create a view transformation matrix for the current view.
 */
void
view_transf_mat( view_trans )
Transf_mat *view_trans;
{
    Transf_mat look_rot;
    float scal;

    /*
     * Set up the viewing transformation matrix.
     */
    mat_copy( view_trans, &ident_matrix );
    mat_translate( view_trans, v_win->bbox_trans[0], v_win->bbox_trans[1],
                   v_win->bbox_trans[2] );
    scal = v_win->bbox_scale;
    mat_scale( view_trans, scal*v_win->scale[0], scal*v_win->scale[1],
               scal*v_win->scale[2] );
    mat_mul( view_trans, view_trans, &v_win->rot_mat );
    mat_translate( view_trans, v_win->trans[0], v_win->trans[1],
                   v_win->trans[2] );
    mat_translate( view_trans, -v_win->look_from[0], -v_win->look_from[1],
                   -v_win->look_from[2] );
    look_rot_mat( v_win->look_from, v_win->look_at, v_win->look_up,
                  &look_rot );
    mat_mul( view_trans, view_trans, &look_rot );
}


/*****************************************************************
 * TAG( inv_view_transf_mat )
 *
 * Create the inverse of the current view transformation matrix.
 */
void
inv_view_transf_mat( inv_trans )
Transf_mat *inv_trans;
{
    Transf_mat rot_mat, look_rot;
    float scal;

    /*
     * Set up the inverse viewing transformation matrix.
     */
    mat_copy( inv_trans, &ident_matrix );
    look_rot_mat( v_win->look_from, v_win->look_at, v_win->look_up,
                  &look_rot );
    invert_rot_mat( &rot_mat, &look_rot );
    mat_mul( inv_trans, inv_trans, &rot_mat );
    mat_translate( inv_trans, v_win->look_from[0], v_win->look_from[1],
                   v_win->look_from[2] );
    mat_translate( inv_trans, -v_win->trans[0], -v_win->trans[1],
                   -v_win->trans[2] );
    invert_rot_mat( &rot_mat, &v_win->rot_mat );
    mat_mul( inv_trans, inv_trans, &rot_mat );
    scal = 1.0 / v_win->bbox_scale;
    mat_scale( inv_trans, scal/v_win->scale[0], scal/v_win->scale[1],
               scal/v_win->scale[2] );
    mat_translate( inv_trans, -v_win->bbox_trans[0], -v_win->bbox_trans[1],
                   -v_win->bbox_trans[2] );
}


/************************************************************
 * TAG( print_view )
 *
 * Print information about the current view matrix.
 */
void
print_view()
{
    float vec[3];
    int i;

    mat_to_angles( &v_win->rot_mat, vec );
    wrt_text( "Current view parameters\n" );
    wrt_text( "    Rotation, X: %f  Y: %f  Z: %f\n",
              RAD_TO_DEG(vec[0]), RAD_TO_DEG(vec[1]), RAD_TO_DEG(vec[2]) );
    wrt_text( "    Translation, X: %f  Y: %f  Z: %f\n",
              v_win->trans[0], v_win->trans[1], v_win->trans[2] );
    wrt_text( "    Scale: %f %f %f\n", v_win->scale[0], v_win->scale[1],
              v_win->scale[2] );
    wrt_text( "    Near plane: %f\n", v_win->near );
    wrt_text( "    Far plane: %f\n", v_win->far );
    wrt_text( "    Look from position: %f %f %f\n", v_win->look_from[0],
              v_win->look_from[1], v_win->look_from[2] );
    wrt_text( "    Look at position: %f %f %f\n", v_win->look_at[0],
              v_win->look_at[1], v_win->look_at[2] );
    wrt_text( "    Look up position: %f %f %f\n", v_win->look_up[0],
              v_win->look_up[1], v_win->look_up[2] );
    wrt_text( "    View angle: %f\n", v_win->cam_angle );

    for ( i = 0; i < 6; i++ )
        if ( v_win->light_active[i] )
            wrt_text( "    Light %d is at point: %f, %f, %f, %f\n", i + 1,
                      v_win->light_pos[i][0], v_win->light_pos[i][1],
                      v_win->light_pos[i][2], v_win->light_pos[i][3] );
}


/*
 * SECTION_TAG( Materials and colors )
 */


/*****************************************************************
 * TAG( define_materials )
 *
 * Defines gl material properties for materials in the mesh.
 */
extern void
define_materials( mats, qty )
int *mats;
int qty;
{
    int i;

    if ( mats == NULL )
        /* Define material properties for all materials. */
        for ( i = 0; i < MAX_MATERIALS; i++ )
            define_one_material( i );
    else
        /* Define material properties only for the listed materials. */
        for ( i = 0; i < qty; i++ )
	    define_one_material( mats[i] );
}


/*****************************************************************
 * TAG( define_one_material )
 *
 * Defines gl material properties for one material in the mesh.
 */
static void
define_one_material( mtl_idx )
int mtl_idx;
{
    int idx, j;

    /*
     * Here are the OpenGL material default values.
     *
     *     GL_AMBIENT  0.2, 0.2, 0.2, 1.0
     *     GL_DIFFUSE  0.8, 0.8, 0.8, 1.0
     *     GL_AMBIENT_AND_DIFFUSE
     *     GL_SPECULAR  0.0, 0.0, 0.0, 1.0
     *     GL_SHININESS  0.0
     *     GL_EMISSION  0.0, 0.0, 0.0, 1.0
     */

    for ( j = 0; j < 3; j++ )
    {
        idx = mtl_idx % MATERIAL_COLOR_CNT;
        v_win->matl_ambient[mtl_idx][j] = material_colors[idx][j];
        v_win->matl_diffuse[mtl_idx][j] = material_colors[idx][j];
        v_win->matl_specular[mtl_idx][j] = 0.0;
        v_win->matl_emission[mtl_idx][j] = 0.0;
    }
    v_win->matl_ambient[mtl_idx][3] = 1.0;
    v_win->matl_diffuse[mtl_idx][3] = 1.0;
    v_win->matl_specular[mtl_idx][3] = 1.0;
    v_win->matl_emission[mtl_idx][3] = 1.0;
    v_win->matl_shininess[mtl_idx] = 0.0;
    
    /* If update affects current OpenGL material, update it. */
    if ( mtl_idx == v_win->current_material )
    {
	update_current_material( AMBIENT );
	update_current_material( DIFFUSE );
	update_current_material( SPECULAR );
	update_current_material( EMISSIVE );
	update_current_material( SHININESS );
    }
}


/*****************************************************************
 * TAG( change_current_material )
 *
 * Change the currently selected material properties.
 */
void
change_current_material( num )
int num;
{
    int old;

    old = v_win->current_material;
    old = old % MAX_MATERIALS;
    num = num % MAX_MATERIALS;

    /* Update only the properties that are different. */

/* Ambient and diffuse get overridden by the current color anyway. */
/*
    if ( v_win->matl_ambient[num][0] != v_win->matl_ambient[old][0] ||
         v_win->matl_ambient[num][1] != v_win->matl_ambient[old][1] ||
         v_win->matl_ambient[num][2] != v_win->matl_ambient[old][2] )
        glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,
                      v_win->matl_ambient[num] );

    if ( v_win->matl_diffuse[num][0] != v_win->matl_diffuse[old][0] ||
         v_win->matl_diffuse[num][1] != v_win->matl_diffuse[old][1] ||
         v_win->matl_diffuse[num][2] != v_win->matl_diffuse[old][2] ||
         v_win->matl_diffuse[num][3] != v_win->matl_diffuse[old][3] )
        glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,
                      v_win->matl_diffuse[num] );
*/

    if ( v_win->matl_specular[num][0] != v_win->matl_specular[old][0] ||
         v_win->matl_specular[num][1] != v_win->matl_specular[old][1] ||
         v_win->matl_specular[num][2] != v_win->matl_specular[old][2] )
        glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR,
                      v_win->matl_specular[num] );

    if ( v_win->matl_emission[num][0] != v_win->matl_emission[old][0] ||
         v_win->matl_emission[num][1] != v_win->matl_emission[old][1] ||
         v_win->matl_emission[num][2] != v_win->matl_emission[old][2] )
        glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION,
                      v_win->matl_emission[num] );

    if ( v_win->matl_shininess[num] != v_win->matl_shininess[old] )
        glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS,
                     v_win->matl_shininess[num] );

    v_win->current_material = num;
}


/*****************************************************************
 * TAG( update_current_material )
 *
 * Update a property for the currently selected material.
 */
void
update_current_material( prop )
Material_property_type prop;
{
    int mtl;
    
    mtl = v_win->current_material;
    
    switch( prop )
    {
	case AMBIENT:
            glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, 
	                  v_win->matl_ambient[mtl] );
	    break;
	case DIFFUSE:
            glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,
                          v_win->matl_diffuse[mtl] );
	    break;
	case SPECULAR:
            glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR,
                          v_win->matl_specular[mtl] );
	    break;
	case EMISSIVE:
            glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION,
                          v_win->matl_emission[mtl] );
	    break;
	case SHININESS:
            glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS,
                         v_win->matl_shininess[mtl] );
	    break;
    }
}


/************************************************************
 * TAG( set_material )
 *
 * Define a material's rendering properties.
 */
void
set_material( token_cnt, tokens )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
{
    float arr[30];
    int matl_num, i;

    /* Get the material number. */
    sscanf( tokens[1], "%d", &matl_num );
    matl_num--;

    matl_num = matl_num % MAX_MATERIALS;

    for ( i = 2; i < token_cnt; i++ )
    {
        if ( strcmp( tokens[i], "amb" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &v_win->matl_ambient[matl_num][0] );
            sscanf( tokens[i+2], "%f", &v_win->matl_ambient[matl_num][1] );
            sscanf( tokens[i+3], "%f", &v_win->matl_ambient[matl_num][2] );
            i += 3;

            /* Need to update GL if this material is currently in use. */
            if ( matl_num == v_win->current_material )
                glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,
                              v_win->matl_ambient[matl_num] );
        }
        else if ( strcmp( tokens[i], "diff" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &v_win->matl_diffuse[matl_num][0] );
            sscanf( tokens[i+2], "%f", &v_win->matl_diffuse[matl_num][1] );
            sscanf( tokens[i+3], "%f", &v_win->matl_diffuse[matl_num][2] );
            i += 3;

            if ( matl_num == v_win->current_material )
                glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,
                              v_win->matl_diffuse[matl_num] );

        }
        else if ( strcmp( tokens[i], "spec" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &v_win->matl_specular[matl_num][0] );
            sscanf( tokens[i+2], "%f", &v_win->matl_specular[matl_num][1] );
            sscanf( tokens[i+3], "%f", &v_win->matl_specular[matl_num][2] );
            i += 3;

            if ( matl_num == v_win->current_material )
                glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR,
                              v_win->matl_specular[matl_num] );
        }
        else if ( strcmp( tokens[i], "emis" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &v_win->matl_emission[matl_num][0] );
            sscanf( tokens[i+2], "%f", &v_win->matl_emission[matl_num][1] );
            sscanf( tokens[i+3], "%f", &v_win->matl_emission[matl_num][2] );
            i += 3;

            if ( matl_num == v_win->current_material )
                glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION,
                              v_win->matl_emission[matl_num] );
        }
        else if ( strcmp( tokens[i], "shine" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &v_win->matl_shininess[matl_num] );
            i++;

            if ( matl_num == v_win->current_material )
                glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS,
                             v_win->matl_shininess[matl_num] );
        }
        else if ( strcmp( tokens[i], "alpha" ) == 0 )
        {
            /* The alpha value at a vertex is just the material's diffuse
             * alpha value -- the other alphas are ignored by OpenGL.
             */
            sscanf( tokens[i+1], "%f", &v_win->matl_diffuse[matl_num][3] );
            i++;

            if ( matl_num == v_win->current_material )
                glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,
                              v_win->matl_diffuse[matl_num] );
        }
    }
}


/*****************************************************************
 * TAG( set_color )
 *
 * Sets a color for the mesh window.  The options are:
 *
 *     "fg" -- Color of lines and text.
 *     "bg" -- Background color.
 *     "con" -- Color of contour lines.
 *     "hilite" -- Color of node & element pick highlights.
 *     "select" -- Color of node & element selection highlights.
 *     "vec" -- Color of 3D vectors for vector command.
 *     "vechd" -- Color for vector heads in vector carpets.
 *     "rmin" -- Color of result minimum threshold.
 *     "rmax" -- Color of result maximum threshold.
 */
void
set_color( select, r_col, g_col, b_col )
char *select;
float r_col;
float g_col;
float b_col;
{
    /* Sanity check. */
    if ( r_col > 1.0 || g_col > 1.0 || b_col > 1.0 )
    {
        /* Assume the user accidentally entered an integer in [0, 255]. */
        r_col = r_col / 255.0;
        g_col = g_col / 255.0;
        b_col = b_col / 255.0;
        wrt_text( "Colors should be in the range (0.0, 1.0).\n" );
    }

    if ( strcmp( select, "fg" ) == 0 )
    {
        v_win->foregrnd_color[0] = r_col;
        v_win->foregrnd_color[1] = g_col;
        v_win->foregrnd_color[2] = b_col;
    }
    else if ( strcmp( select, "bg" ) == 0 )
    {
        v_win->backgrnd_color[0] = r_col;
        v_win->backgrnd_color[1] = g_col;
        v_win->backgrnd_color[2] = b_col;

        /* Set the background color for clears. */
        glClearColor( v_win->backgrnd_color[0],
                      v_win->backgrnd_color[1],
                      v_win->backgrnd_color[2], 0.0 );
    }
    else if ( strcmp( select, "con" ) == 0 )
    {
        v_win->contour_color[0] = r_col;
        v_win->contour_color[1] = g_col;
        v_win->contour_color[2] = b_col;
    }
    else if ( strcmp( select, "hilite" ) == 0 )
    {
        v_win->hilite_color[0] = r_col;
        v_win->hilite_color[1] = g_col;
        v_win->hilite_color[2] = b_col;
    }
    else if ( strcmp( select, "select" ) == 0 )
    {
        v_win->select_color[0] = r_col;
        v_win->select_color[1] = g_col;
        v_win->select_color[2] = b_col;
    }
    else if ( strcmp( select, "vec" ) == 0 )
    {
        v_win->vector_color[0] = r_col;
        v_win->vector_color[1] = g_col;
        v_win->vector_color[2] = b_col;
    }
    else if ( strcmp( select, "vechd" ) == 0 )
    {
        /* Color for vector heads in vector carpets. */
        v_win->vector_hd_color[0] = r_col;
        v_win->vector_hd_color[1] = g_col;
        v_win->vector_hd_color[2] = b_col;
    }
    else if ( strcmp( select, "rmin" ) == 0 )
    {
        /* Color for result minimum cutoff. */
        v_win->rmin_color[0] = r_col;
        v_win->rmin_color[1] = g_col;
        v_win->rmin_color[2] = b_col;
    }
    else if ( strcmp( select, "rmax" ) == 0 )
    {
        /* Color for result maximum cutoff. */
        v_win->rmax_color[0] = r_col;
        v_win->rmax_color[1] = g_col;
        v_win->rmax_color[2] = b_col;
    }
    else
        wrt_text( "Color selection \"%s\" not recognized.\n", select );
}


/************************************************************
 * TAG( color_lookup )
 *
 * Lookup a color in the colormap, and store it in the first
 * argument.  If colorflag is FALSE, the routine just returns
 * the material color.
 */
static void
color_lookup( col, val, result_min, result_max, threshold, matl, colorflag )
float col[4];
float val;
float result_min;
float result_max;
float threshold;
int matl;
Bool_type colorflag;
{
    int idx;
    float scl_max;

    /* Get alpha from the material. */
    col[3] = v_win->matl_diffuse[ matl%MAX_MATERIALS ][3];

    /* If not doing colormapping or if result is near zero, use the
     * default material color.  Otherwise, do the color table lookup.
     */
    if ( !colorflag )
    {
        VEC_COPY( col, v_win->matl_diffuse[ matl%MAX_MATERIALS ] );
    }
    else if ( val < threshold && val > -threshold )
    {
        VEC_COPY( col, v_win->matl_diffuse[ matl%MAX_MATERIALS ] );
    }
    else if ( val > result_max )
    {
        VEC_COPY( col, v_win->colormap[255] );
    }
    else if ( val < result_min )
    {
        VEC_COPY( col, v_win->colormap[0] );
    }
    else if ( result_min == result_max )
    {
        VEC_COPY( col, v_win->colormap[1] );
    }
    else
    {
        scl_max = (float) CMAP_SIZE - 2.01;
        idx = (int)( scl_max * (val-result_min)/(result_max-result_min) ) + 1;
        VEC_COPY( col, v_win->colormap[idx] );
    }
}


/*
 * SECTION_TAG( Draw grid )
 */


/************************************************************
 * TAG( update_display )
 *
 * Redraw the grid window, using the current viewing
 * transformation to draw the grid.  This routine assumes
 * that the calling routine has set the current GL window
 * to be the grid view window.
 */
void
update_display( analy )
Analysis *analy;
{
    Transf_mat look_rot;
    float arr[16], scal;
    RGB_raster_obj *p_rro;

    if ( env.timing )
    {
        wrt_text( "\nTiming for rendering...\n" );
        check_timing( 0 );
    }
    
    /* Set cursor to let user know something is happening. */
    set_alt_cursor( CURSOR_WATCH );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
             GL_STENCIL_BUFFER_BIT );
    
    if ( analy->show_background_image && analy->background_image != NULL )
    {
        p_rro = analy->background_image;

        glDisable( GL_DEPTH_TEST );
        memory_to_screen( FALSE, p_rro->img_width, p_rro->img_height, 
                          p_rro->alpha, p_rro->raster );
        glEnable( GL_DEPTH_TEST );
    }

    glPushMatrix();

    /*
     * Set up all the viewing transforms.  Transformations are
     * specified in opposite order to the order in which they
     * are applied.
     */
    look_rot_mat( v_win->look_from, v_win->look_at,
                  v_win->look_up, &look_rot );
    mat_to_array( &look_rot, arr );
    glMultMatrixf( arr );
    glTranslatef( -v_win->look_from[0], -v_win->look_from[1],
                  -v_win->look_from[2] );
    glTranslatef( v_win->trans[0], v_win->trans[1], v_win->trans[2] );
    mat_to_array( &v_win->rot_mat, arr );
    glMultMatrixf( arr );
    scal = v_win->bbox_scale;
    glScalef( scal*v_win->scale[0], scal*v_win->scale[1],
              scal*v_win->scale[2] );
    glTranslatef( v_win->bbox_trans[0], v_win->bbox_trans[1],
                  v_win->bbox_trans[2] );

    /* Draw the grid. */
    if ( analy->dimension == 3 )
        draw_grid( analy );
    else
        draw_grid_2d( analy );

    glPopMatrix();

    /* Draw all the foreground stuff. */
    draw_foreground( analy );

    gui_swap_buffers();

    unset_alt_cursor();
    
    if ( env.timing )
        check_timing( 1 );
}


/************************************************************
 * TAG( draw_grid )
 *
 * Draw the element grid.  This routine handles all procedures
 * associated with drawing the grid, except for the viewing
 * transformation.
 */
static void
draw_grid( analy )
Analysis *analy;
{
    Triangle_poly *tri;
    Bool_type colorflag;
    float verts[4][3];
    float norms[4][3];
    float cols[4][4];
    float vals[4];
    float rdiff;
    float scl_max;
    int i, j, k;
    
    /* Cap for colorscale interpolation. */
    scl_max = SCL_MAX;

    /* Enable color to change AMBIENT & DIFFUSE property of the material. */
    glEnable( GL_COLOR_MATERIAL );

    /* Turn lighting on. */
    if ( v_win->lighting )
        glEnable( GL_LIGHTING );

    /* Bias the depth-buffer if edges are on so they'll render in front. */
    if ( analy->show_edges )
        glDepthRange( analy->edge_zbias, 1 );

    /*
     * Draw iso-surfaces.
     */
    if ( analy->show_isosurfs && !analy->show_carpet )
    {
        /* Difference between result min and max. */
        rdiff = analy->result_mm[1] - analy->result_mm[0]; 
        if ( rdiff == 0.0 )
            rdiff = 1.0;

        /* Use two-sided lighting. */
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 1 );

        /* Set up for polygon drawing. */
        begin_draw_poly( analy );

        for ( tri = analy->isosurf_poly; tri != NULL; tri = tri->next )
        {
            /*
             * Only compute once per polygon, since polygons are constant
             * color.
             */
            k = (int)( (tri->result[0] - analy->result_mm[0]) *
                       scl_max / rdiff ) + 1;

            for ( i = 0; i < 3; i++ )
            {
                VEC_COPY( cols[i], v_win->colormap[k] );
                VEC_COPY( norms[i], tri->norm );
                VEC_COPY( verts[i], tri->vtx[i] );
                vals[i] = tri->result[0];
            }
            draw_poly( 3, verts, norms, cols, vals, -1, analy );
        }

        /* Back to the defaults. */
        end_draw_poly( analy );
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 0 );
    }

    /*
     * Draw cut planes.
     */
    if ( analy->show_cut && !analy->show_carpet )
    {
        /* Use two-sided lighting. */
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 1 );

        /* Set up for polygon drawing. */
        begin_draw_poly( analy );

        for ( tri = analy->cut_poly; tri != NULL; tri = tri->next )
        {
            colorflag = analy->show_hex_result &&
                        !analy->disable_material[tri->mat];

            for ( i = 0; i < 3; i++ )
            {
                color_lookup( cols[i], tri->result[i],
                              analy->result_mm[0], analy->result_mm[1],
                              analy->zero_result, tri->mat, colorflag );
                VEC_COPY( norms[i], tri->norm );
                VEC_COPY( verts[i], tri->vtx[i] );
                vals[i] = tri->result[i];
            }
            draw_poly( 3, verts, norms, cols, vals, -1, analy );
        }

        /* Back to the defaults. */
        end_draw_poly( analy );
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 0 );
    }

    /* Back to the default. */
    glDisable( GL_COLOR_MATERIAL );

    /*
     * Draw external surface polygons.
     */
    if ( analy->show_extern_polys )
        draw_extern_polys( analy );

    /*
     * Draw reference surface polygons.
     */
    if ( analy->show_ref_polys )
        draw_ref_polys( analy );

    /*
     * Draw the elements.
     */
    draw_hex( analy );
    draw_shells( analy );

    /* Turn lighting off (back to the default). */
    if ( v_win->lighting )
        glDisable( GL_LIGHTING );

    /* Remove depth-buffer bias applied to polygons. */
    if ( analy->show_edges )
        glDepthRange( 0, 1 );

    draw_beams( analy );

    /*
     * Draw point cloud.
     */
    if ( analy->render_mode == RENDER_POINT_CLOUD )
        draw_nodes( analy );

    /*
     * Draw mesh edges.
     */
    if ( analy->show_edges )
        draw_edges( analy );

    /*
     * Draw contours.
     */
    if ( analy->show_contours )
    {
        Contour_obj *cont;

/*        lsetdepth( v_win->z_front_near, v_win->z_front_far ); */

        antialias_lines( TRUE, analy->z_buffer_lines );
        glLineWidth( (GLfloat) analy->contour_width );

        glColor3fv( v_win->contour_color );
        for ( cont = analy->contours; cont != NULL; cont = cont->next )
            draw_line( cont->cnt, cont->pts, -1, analy, FALSE );

        glLineWidth( (GLfloat) 1.0 );
        antialias_lines( FALSE, 0 );

        /* Reset near/far planes to the defaults. */
/*        lsetdepth( v_win->z_default_near, v_win->z_default_far ); */
    }

    /*
     * Draw vector grid.
     */
    if ( analy->show_vectors )
    {
        if ( analy->vectors_at_nodes == TRUE )
            draw_node_vec( analy );
        else if ( analy->vec_pts != NULL )
            draw_vec_result( analy );
        else
             popup_dialog( INFO_POPUP, "No grid defined for vectors." );
    }

    /*
     * Draw regular vector carpet.
     */
    if ( analy->show_carpet )
        draw_reg_carpet( analy );

    /*
     * Draw grid vector carpet or iso or cut surface carpet.
     */
    if ( analy->show_carpet )
    {
        draw_vol_carpet( analy );
        draw_shell_carpet( analy );
    }

    /*
     * Draw reference surface vector carpet.
     */
    if ( analy->show_carpet )
        draw_ref_carpet( analy );

    /*
     * Draw particle traces.
     */
    if ( analy->show_traces )
        draw_traces( analy );

    /*
     * Draw node or element numbers.
     */
    if ( analy->show_node_nums || analy->show_elem_nums )
        draw_elem_numbers( analy );

    /*
     * Highlight a node or element.
     */
    if ( analy->hilite_type > 0 )
        draw_hilite( analy->hilite_type - 1, analy->hilite_num,
                     v_win->hilite_color, analy );

    /*
     * Highlight selected nodes and elements.
     */
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < analy->num_select[i]; j++ )
            draw_hilite( i, analy->select_elems[i][j],
                         v_win->select_color, analy );

    /*
     * Draw wireframe bounding box.
     */
    if ( analy->show_bbox )
        draw_bbox( analy->bbox );
}


/************************************************************
 * TAG( draw_grid_2d )
 *
 * Draw the element grid.  This routine handles all procedures
 * associated with drawing the grid, except for the viewing
 * transformation.
 */
static void
draw_grid_2d( analy )
Analysis *analy;
{
    int i, j;

    /*
     * Draw external surface polygons.
     */
/*
    if ( analy->show_extern_polys )
        draw_extern_polys( analy );
*/

    /*
     * Draw the elements.
     */
    draw_shells_2d( analy );
    draw_beams_2d( analy );

    /*
     * Draw point cloud.
     */
    if ( analy->render_mode == RENDER_POINT_CLOUD )
        draw_nodes( analy );

    /*
     * Draw mesh edges.
     */
    if ( analy->show_edges )
        draw_edges( analy );

    /*
     * Draw contours.
     */
    if ( analy->show_contours )
    {
        Contour_obj *cont;

        antialias_lines( TRUE, analy->z_buffer_lines );
        glLineWidth( (GLfloat) analy->contour_width );

        glColor3fv( v_win->contour_color );
        for ( cont = analy->contours; cont != NULL; cont = cont->next )
            draw_line( cont->cnt, cont->pts, -1, analy, FALSE );

        glLineWidth( (GLfloat) 1.0 );
        antialias_lines( FALSE, 0 );
    }

    /*
     * Draw vector result.
     */
    if ( analy->show_vectors )
    {
        if ( analy->vectors_at_nodes == TRUE )
            draw_node_vec( analy );
        else if ( analy->vec_pts != NULL )
            draw_vec_result_2d( analy );
        else
             popup_dialog( INFO_POPUP, "No grid defined for vectors." );
    }

    /*
     * Draw grid vector carpet.
     */
    if ( analy->show_carpet )
        draw_shell_carpet( analy );

    /*
     * Draw particle traces.
     */
/*
    if ( analy->show_traces )
        draw_traces( analy );
*/

    /*
     * Draw node or element numbers.
     */
    if ( analy->show_node_nums || analy->show_elem_nums )
        draw_elem_numbers( analy );

    /*
     * Highlight a node or element.
     */
    if ( analy->hilite_type > 0 )
        draw_hilite( analy->hilite_type - 1, analy->hilite_num,
                     v_win->hilite_color, analy );

    /*
     * Highlight selected nodes and elements.
     */
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < analy->num_select[i]; j++ )
            draw_hilite( i, analy->select_elems[i][j],
                         v_win->select_color, analy );

    /*
     * Draw wireframe bounding box.
     */
    if ( analy->show_bbox )
        draw_bbox( analy->bbox );
}


/************************************************************
 * TAG( check_for_tri_face )
 *
 * Check for a triangular brick face (i.e., one of the four
 * face nodes is repeated.)  if the face is triangular, reorder
 * the nodes so the repeated node is last.  The new ordering
 * is returned in "ord".  The routine returns the number of
 * nodes for the face (3 or 4).
 */
static int
check_for_tri_face( fc, el, bricks, verts, ord )
int fc;
int el;
Hex_geom *bricks;
float verts[4][3];
int ord[4];
{
    float tverts[4][3];
    int shift, nodes[4];
    int i, j;

    if ( bricks->degen_elems )
    {
        for ( i = 0; i < 4; i++ )
            nodes[i] = bricks->nodes[ fc_nd_nums[fc][i] ][el];

        for ( i = 0; i < 4; i++ )
            if ( nodes[i] == nodes[(i+1)%4] )
            {
                /* Triangle. */
                if ( i == 3 )
                    shift = 3;
                else
                    shift = 2 - i;

                for ( j = 0; j < 4; j++ )
                    ord[j] = ( j + 4 - shift ) % 4;

                /* Reorder the vertices. */
                for ( j = 0; j < 4; j++ )
                {
                    VEC_COPY( tverts[j], verts[ ord[j] ] );
                }
                for ( j = 0; j < 4; j++ )
                {
                    VEC_COPY( verts[j], tverts[j] );
                }

                return( 3 );
            }
    }

    /* Quad. */
    for ( j = 0; j < 4; j++ )
        ord[j] = j;

    return( 4 );
}


/************************************************************
 * TAG( get_min_max )
 *
 * Get the appropriate result min and max.
 */
static void
get_min_max( analy, no_interp, p_min, p_max )
Analysis *analy;
Bool_type no_interp;
float *p_min;
float *p_max;
{
    /* Nodal results never get an element min/max. */
    if ( is_nodal_result( analy->result_id ) )
    {
        *p_min = analy->result_mm[0];
        *p_max = analy->result_mm[1];
    }
    else if ( analy->interp_mode == NO_INTERP || no_interp )
    {
        if ( analy->mm_result_set[0] )
	    *p_min = analy->result_mm[0];
	else if ( analy->use_global_mm )
	    *p_min = analy->global_elem_mm[0];
	else
	    *p_min = analy->elem_state_mm.el_minmax[0];

        if ( analy->mm_result_set[1] )
	    *p_max = analy->result_mm[1];
	else if ( analy->use_global_mm )
	    *p_max = analy->global_elem_mm[1];
	else
	    *p_max = analy->elem_state_mm.el_minmax[1];
    }
    else
    {
        *p_min = analy->result_mm[0];
        *p_max = analy->result_mm[1];
    }
}


/************************************************************
 * TAG( draw_hex )
 *
 * Draw the external faces of hex volume elements in the model.
 */
static void
draw_hex( analy )
Analysis *analy;
{
    Bool_type colorflag;
    Hex_geom *bricks;
    float verts[4][3];
    float norms[4][3];
    float cols[4][4];
    float val, res[4];
    float rmin, rmax;
    float tverts[4][3], v1[3], v2[3], nor[3], ray[3];
    int matl, el, fc, nd, ord[4], cnt;
    int i, j, k;

    if ( analy->geom_p->bricks == NULL )
        return;

    if ( analy->render_mode != RENDER_FILLED &&
         analy->render_mode != RENDER_HIDDEN )
        return;

    bricks = analy->geom_p->bricks;
    get_min_max( analy, FALSE, &rmin, &rmax );

    /* Enable color to change AMBIENT & DIFFUSE property of the material. */
    glEnable( GL_COLOR_MATERIAL );

    /* Throw away backward-facing faces. */
    glEnable( GL_CULL_FACE );

    /* Set up for polygon drawing. */
    begin_draw_poly( analy );

    for ( i = 0; i < analy->face_cnt; i++ )
    {
        el = analy->face_el[i];
        fc = analy->face_fc[i];

        /*
         * Remove faces that are shared with shell elements, so
         * the polygons are not drawn twice.
         */
        if ( analy->shared_faces && analy->geom_p->shells != NULL &&
             face_matches_shell( el, fc, analy ) )
            continue;

        get_face_verts( el, fc, analy, verts );

        /* Cull backfacing polygons by hand to speed things up. */
        if ( !analy->reflect && analy->manual_backface_cull )
	{
            for ( j = 0; j < 4; j++ )
                point_transform( tverts[j], verts[j], &cur_view_mat );
            VEC_SUB( v1, tverts[2], tverts[0] );
            VEC_SUB( v2, tverts[3], tverts[1] );
            VEC_CROSS( nor, v1, v2 );
            if ( v_win->orthographic )
            {
                VEC_SET( ray, 0.0, 0.0, -1.0 );
            }
            else
            {
                for ( k = 0; k < 3; k++ )
                    ray[k] = 0.25 * ( tverts[0][k] + tverts[1][k] +
                                      tverts[2][k] + tverts[3][k] );
            }

            if ( VEC_DOT( ray, nor ) >= 0.0 )
                continue;
        }

        /*
         * Check for triangular (degenerate) face and reorder nodes
         * if needed.
         */
        cnt = check_for_tri_face( fc, el, bricks, verts, ord );

        matl = bricks->mat[el]; 
        if ( v_win->current_material != matl )
            change_current_material( matl );

        /* Colorflag is TRUE if displaying a result, otherwise
         * polygons are drawn in the material color.
         */
        colorflag = analy->show_hex_result && !analy->disable_material[matl];

        for ( j = 0; j < 4; j++ )
        {
            nd = bricks->nodes[ fc_nd_nums[fc][ord[j]] ][el];

            for ( k = 0; k < 3; k++ )
                norms[ ord[j] ][k] = analy->face_norm[ ord[j] ][k][i];

            if ( analy->interp_mode == GOOD_INTERP )
            {
                res[ ord[j] ] = analy->result[nd];
            }
            else if ( analy->interp_mode == NO_INTERP 
	              && !is_nodal_result( analy->result_id ) )
            {
                /* Bound is needed because min/max is for nodes. */
/**/
/* rmin/rmax now set appropriately dependent on interp_mode
                val = BOUND( rmin, analy->hex_result[el], rmax );
*/
                val = analy->hex_result[el];
                color_lookup( cols[ ord[j] ], val, rmin, rmax,
                              analy->zero_result, matl, colorflag );
            }
            else
            {
                color_lookup( cols[ ord[j] ], analy->result[nd],
                              rmin, rmax, analy->zero_result, matl,
                              colorflag );
            }
        }

        draw_poly( cnt, verts, norms, cols, res, matl, analy );
    }

    /* Back to the defaults. */
    end_draw_poly( analy );
    glDisable( GL_CULL_FACE );
    glDisable( GL_COLOR_MATERIAL );
}


/************************************************************
 * TAG( draw_shells )
 *
 * Draw the shell elements in the model.
 */
static void
draw_shells( analy )
Analysis *analy;
{
    Shell_geom *shells;
    Bool_type colorflag;
    float *activity;
    float verts[4][3];
    float norms[4][3];
    float cols[4][4];
    float val, res[4];
    float rmin, rmax;
    int matl, cnt, nd, i, j, k;

    if ( analy->geom_p->shells == NULL )
        return;

    if ( analy->render_mode != RENDER_FILLED &&
         analy->render_mode != RENDER_HIDDEN )
        return;

    shells = analy->geom_p->shells;
    activity = analy->state_p->activity_present ?
               analy->state_p->shells->activity : NULL;
    get_min_max( analy, FALSE, &rmin, &rmax );

    /* Enable color to change AMBIENT & DIFFUSE property of the material. */
    glEnable( GL_COLOR_MATERIAL );

    /* Use two-sided lighting for shells. */
    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 1 );

    /* Set up for polygon drawing. */
    begin_draw_poly( analy );

    for ( i = 0; i < shells->cnt; i++ )
    {
        /* Check for inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Skip if this material is invisible. */
        if ( analy->hide_material[shells->mat[i]] )
             continue;

        matl = shells->mat[i];
        if ( v_win->current_material != matl )
            change_current_material( matl );

        /* Colorflag is TRUE if displaying a result, otherwise
         * polygons are drawn in the material color.
         */
        colorflag = analy->show_shell_result &&
                    !analy->disable_material[matl];

        get_shell_verts( i, analy, verts );

        for ( j = 0; j < 4; j++ )
        {
            nd = shells->nodes[j][i];

            for ( k = 0; k < 3; k++ )
                norms[j][k] = analy->shell_norm[j][k][i];

            if ( analy->interp_mode == GOOD_INTERP )
            {
                res[j] = analy->result[nd];
            }
            else if ( analy->interp_mode == NO_INTERP 
	              && !is_nodal_result( analy->result_id ) )
            {
                /* Bound is needed because min/max is for nodes. */
/**/
/* As for hex's...
                val = BOUND( rmin, analy->shell_result[i], rmax );
*/
                val = analy->shell_result[i];
                color_lookup( cols[j], val, rmin, rmax,
                              analy->zero_result, matl, colorflag );
            }
            else
            {
                color_lookup( cols[j], analy->result[nd], rmin, rmax,
                              analy->zero_result, matl, colorflag );
            }
        }

        /* Check for triangular (degenerate) shell element. */
        cnt = 4;
        if ( shells->degen_elems &&
             shells->nodes[2][i] == shells->nodes[3][i] )
            cnt = 3;

        draw_poly( cnt, verts, norms, cols, res, matl, analy );
    }

    /* Back to the defaults. */
    end_draw_poly( analy );
    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 0 );
    glDisable( GL_COLOR_MATERIAL );
}


/************************************************************
 * TAG( draw_beams )
 *
 * Draw the beam elements in the model.
 */
static void
draw_beams( analy )
Analysis *analy;
{
    Beam_geom *beams;
    Beam_state *st_beams;
    Bool_type colorflag, verts_ok;
    float *activity;
    float col[4];
    float verts[2][3];
    float pts[6];
    float threshold, val, rmin, rmax;
    GLfloat nearfar[2];
    int matl, i, j, k;

    if ( analy->geom_p->beams == NULL )
        return;

    if ( analy->render_mode != RENDER_FILLED &&
         analy->render_mode != RENDER_HIDDEN )
        return;

    beams = analy->geom_p->beams;
    activity = analy->state_p->activity_present ?
               analy->state_p->beams->activity : NULL;
    get_min_max( analy, TRUE, &rmin, &rmax );
    threshold = analy->zero_result;

    antialias_lines( TRUE, analy->z_buffer_lines );
    glLineWidth( 3 );

    /* Bias the depth-buffer so beams are in front. */
    if ( analy->zbias_beams )
    {
	glGetFloatv( GL_DEPTH_RANGE, nearfar );
        glDepthRange( 0, 1 - analy->beam_zbias );
    }

    for ( i = 0; i < beams->cnt; i++ )
    {
        /* Check for inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Skip if this material is invisible. */
        matl = beams->mat[i];
        if ( analy->hide_material[matl] )
             continue;

        verts_ok = get_beam_verts( i, analy, verts );
	if ( !verts_ok )
	{
	    wrt_text( "Warning - beam %d ignored; bad node(s).\n", 
		      i + 1 );
	    continue;
	}

        colorflag = is_beam_result( analy->result_id ) &&
                    !analy->disable_material[matl];
        /*
         * Min and max come from averaged nodal values.  Since
         * we're drawing element values for beams, need to clamp
         * them to the nodal min/max.
         */
/**/
/* As for shells and hex's...
        val = BOUND( rmin, analy->beam_result[i], rmax );
*/
        val = analy->beam_result[i];
        color_lookup( col, val, rmin, rmax, threshold, matl, colorflag );
        glColor3fv( col );

        for ( j = 0; j < 2; j++ )
            for ( k = 0; k < 3; k++ )
                pts[j*3+k] = verts[j][k];

        draw_line( 2, pts, matl, analy, FALSE );
    }
    
    /* Remove depth bias. */
    if ( analy->zbias_beams )
        glDepthRange( nearfar[0], nearfar[1] );

    antialias_lines( FALSE, 0 );
    glLineWidth( 1.0 );
}


/************************************************************
 * TAG( draw_shells_2d )
 *
 * Draw the shell elements in the model.
 */
static void
draw_shells_2d( analy )
Analysis *analy;
{
    int i, j, k, nd, matl;
    Shell_geom *shells;
    Nodal *nodes, *onodes;
    Bool_type colorflag;
    float *activity;
    float orig, val;
    float rmin, rmax;
    float verts[4][3];
    float cols[4][4];
    float pts[12];
    float res[4];

    if ( analy->geom_p->shells == NULL )
        return;

    if ( analy->render_mode != RENDER_FILLED &&
         analy->render_mode != RENDER_HIDDEN )
        return;

    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;
    activity = analy->state_p->activity_present ?
               analy->state_p->shells->activity : NULL;
    rmin = analy->result_mm[0];
    rmax = analy->result_mm[1];

    /* Set up for polygon drawing. */
    begin_draw_poly( analy );

    /*
     * Draw filled polygons for the shell elements.
     */
    for ( i = 0; i < shells->cnt; i++ )
    {
        /* Check for inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Skip if this material is invisible. */
        if ( analy->hide_material[shells->mat[i]] )
             continue;

        matl = shells->mat[i];

        /* If displaying a result, enable color to change
         * the DIFFUSE property of the material.
         */
        colorflag = analy->show_shell_result &&
                    !analy->disable_material[matl];

        for ( j = 0; j < 4; j++ )
        {
            nd = shells->nodes[j][i];

            if ( analy->displace_exag )
            {
                /* Scale the node displacements. */
                for ( k = 0; k < 3; k++ )
                {
                    orig = onodes->xyz[k][nd];
                    verts[j][k] = orig + analy->displace_scale[k]*
                                  (nodes->xyz[k][nd] - orig);
                }
            }
            else
            {
                for ( k = 0; k < 3; k++ )
                    verts[j][k] = nodes->xyz[k][nd];
            }

            if ( analy->interp_mode == GOOD_INTERP )
            {
                res[j] = analy->result[nd];
            }
            else if ( analy->interp_mode == NO_INTERP 
	              && !is_nodal_result( analy->result_id ) )
            {
                /* Bound is needed because min/max is for nodes. */
                val = BOUND( rmin, analy->shell_result[i], rmax );
                color_lookup( cols[j], val, rmin, rmax,
                              analy->zero_result, matl, colorflag );
            }
            else
            {
                color_lookup( cols[j], analy->result[nd], rmin, rmax,
                              analy->zero_result, matl, colorflag );
            }
        }

        draw_poly_2d( 4, verts, cols, res, matl, analy );
    }

    end_draw_poly( analy );

    /*
     * Draw the outlines of the shell elements.
     */
    if ( analy->render_mode == RENDER_HIDDEN )
    {
        glColor3fv( v_win->foregrnd_color );

        for ( i = 0; i < shells->cnt; i++ )
        {
            /* Check for inactive elements. */
            if ( activity && activity[i] == 0.0 )
                continue;

            /* Skip if this material is invisible. */
            if ( analy->hide_material[shells->mat[i]] )
                 continue;

            for ( j = 0; j < 4; j++ )
            {
                nd = shells->nodes[j][i];

                if ( analy->displace_exag )
                {
                    /* Scale the node displacements. */
                    for ( k = 0; k < 3; k++ )
                    {
                        orig = onodes->xyz[k][nd];
                        pts[j*3+k] = orig + analy->displace_scale[k]*
                                     (nodes->xyz[k][nd] - orig);
                    }
                }
                else
                {
                    for ( k = 0; k < 3; k++ )
                        pts[j*3+k] = nodes->xyz[k][nd];
                }
            }
            draw_line( 4, pts, shells->mat[i], analy, TRUE );
        }
    }   
}


/************************************************************
 * TAG( draw_beams_2d )
 *
 * Draw the beam elements in the model.
 */
static void
draw_beams_2d( analy )
Analysis *analy;
{
    int i, j, k, nd, matl;
    Beam_geom *beams;
    Nodal *nodes, *onodes;
    Bool_type colorflag;
    float *activity;
    float col[4];
    float pts[6];
    float orig, threshold, val, rmin, rmax;

    if ( analy->geom_p->beams == NULL )
        return;

    if ( analy->render_mode != RENDER_FILLED &&
         analy->render_mode != RENDER_HIDDEN )
        return;

    beams = analy->geom_p->beams;
    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;
    activity = analy->state_p->activity_present ?
               analy->state_p->beams->activity : NULL;
    rmin = analy->result_mm[0];
    rmax = analy->result_mm[1];
    threshold = analy->zero_result;

    antialias_lines( TRUE, analy->z_buffer_lines );
    glLineWidth( 3.0 );

    for ( i = 0; i < beams->cnt; i++ )
    {
        /* Check for inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        matl = beams->mat[i];

        /* Skip if this material is invisible. */
        if ( analy->hide_material[matl] )
             continue;

        colorflag = is_beam_result( analy->result_id ) &&
                    !analy->disable_material[matl];
        /*
         * Min and max come from averaged nodal values.  Since
         * we're drawing element values for beams, need to clamp
         * them to the nodal min/max.
         */
        val = BOUND( rmin, analy->beam_result[i], rmax );
        color_lookup( col, val, rmin, rmax, threshold, matl, colorflag );
        glColor3fv( col );

        for ( j = 0; j < 2; j++ )
        {
            nd = beams->nodes[j][i];

            if ( analy->displace_exag )
            {
                /* Scale the node displacements. */
                for ( k = 0; k < 3; k++ )
                {
                    orig = onodes->xyz[k][nd];
                    pts[j*3+k] = orig + analy->displace_scale[k]*
                                 (nodes->xyz[k][nd] - orig);
                }
            }
            else
            {
                for ( k = 0; k < 3; k++ )
                    pts[j*3+k] = nodes->xyz[k][nd];
            }
        }
        draw_line( 2, pts, matl, analy, FALSE );
    }

    antialias_lines( FALSE, 0 );
    glLineWidth( 1.0 );
}


/************************************************************
 * TAG( draw_nodes )
 *
 * Draw the mesh nodes for "point cloud" rendering.
 */
static void
draw_nodes( analy )
Analysis *analy;
{
    Nodal *nodes;
    float pt[3];
    float col[4];
    int i, j;

    nodes = analy->state_p->nodes;

    glBegin( GL_POINTS );

    for ( i = 0; i < nodes->cnt; i++ )
    {
        for ( j = 0; j < 3; j++ )
            pt[j] = nodes->xyz[j][i];

        if ( analy->result_id == VAL_NONE )
        {
            VEC_COPY( col, v_win->foregrnd_color );
        }
        else
            color_lookup( col, analy->result[i], analy->result_mm[0],
                          analy->result_mm[1], analy->zero_result, -1, TRUE );

        glColor3fv( col );
        glVertex3fv( pt );
    }

    glEnd();
}


/************************************************************
 * TAG( draw_edges )
 *
 * Draw the mesh edges.
 */
static void
draw_edges( analy )
Analysis *analy;
{
    Nodal *nodes, *onodes;
    float pts[6];
    float orig;
    int nd, i, j, k;

    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;

    antialias_lines( TRUE, analy->z_buffer_lines );

    /* Bias the depth buffer so edges are in front of polygons. */
    glDepthRange( 0, 1 - analy->edge_zbias );

    glColor3fv( v_win->foregrnd_color  );
    for ( i = 0; i < analy->m_edges_cnt; i++ )
    {
        for ( j = 0; j < 2; j++ )
        {
            nd = analy->m_edges[j][i];

            if ( analy->displace_exag )
            {
                /* Scale the node displacements. */
                for ( k = 0; k < 3; k++ )
                {
                    orig = onodes->xyz[k][nd];
                    pts[j*3+k] = orig + analy->displace_scale[k]*
                                 (nodes->xyz[k][nd] - orig);
                }
            }
            else
            {
                for ( k = 0; k < 3; k++ )
                    pts[j*3+k] = nodes->xyz[k][nd];
            }
        }

        draw_line( 2, pts, analy->m_edge_mtl[i], analy, FALSE );
    }

    /* Remove depth bias. */
    glDepthRange( 0, 1 );

    antialias_lines( FALSE, 0 );
}


/************************************************************
 * TAG( draw_hilite )
 *
 * If a node or element is highlighted, this routine draws
 * the highlight.  The hilite_type argument should be
 *
 *     0 - Node
 *     1 - Beam
 *     2 - Shell
 *     3 - Brick
 */
static void
draw_hilite( hilite_type, hilite_num, hilite_col, analy )
int hilite_type;
int hilite_num;
float hilite_col[3];
Analysis *analy;
{
    Nodal *nodes;
    Beam_geom *beams;
    Shell_geom *shells;
    Hex_geom *bricks;
    float verts[8][3], leng[3], vec[3];
    float rfac, radius;
    char label[30];
    int vert_cnt;
    int i, j;
    Bool_type verts_ok;
    float val;
    int fracsz;

    rfac = 0.1;                             /* Radius scale factor. */
    fracsz = analy->float_frac_size;

    /* Outline the hilighted element. */
    switch ( hilite_type )
    {
        case 0:
            /* Highlight a node. */
            nodes = analy->state_p->nodes;

            /* Validity check. */
            if ( hilite_num < 0 || hilite_num >= nodes->cnt )
            {
                wrt_text( "Node number out of range!\n" );
                return;
            }
            if ( analy->result_id != VAL_NONE )
	    {
	        val = analy->perform_unit_conversion
		      ? analy->result[hilite_num] * analy->conversion_scale
		        + analy->conversion_offset
		      : analy->result[hilite_num];
                sprintf( label, " Node %d (%.*e)", hilite_num+1, fracsz, val );
	    }
            else
                sprintf( label, " Node %d", hilite_num+1 );

            get_node_vert( hilite_num, analy, verts[0] );
            vert_cnt = 1;

            for ( i = 0; i < 3; i++ )
                leng[i] = analy->bbox[1][i] - analy->bbox[0][i];
            radius = 0.01 * (leng[0] + leng[1] + leng[2]) / 3.0;
            radius *= 1.0 / v_win->scale[0];
            break;
        case 1:
            /* Highlight a beam element. */
            beams = analy->geom_p->beams;

            /* Validity check. */
            if ( beams == NULL || hilite_num < 0 || hilite_num >= beams->cnt )
            {
                wrt_text( "Beam number out of range!\n" );
                return;
            }

            verts_ok = get_beam_verts( hilite_num, analy, verts );
	    if ( !verts_ok )
	    {
		wrt_text( "Warning - beam %d ignored; bad node(s).\n", 
		          hilite_num + 1 );
		return;
	    }
            vert_cnt = 2;
	    
            if ( is_beam_result(analy->result_id) )
	    {
	        val = analy->perform_unit_conversion
		      ? analy->beam_result[hilite_num] * analy->conversion_scale
		        + analy->conversion_offset
		      : analy->beam_result[hilite_num];
                sprintf( label, " Beam %d (%.*e)", hilite_num+1, fracsz, val );
	    }
            else
                sprintf( label, " Beam %d", hilite_num+1 );

            VEC_SUB( vec, verts[1], verts[0] );
            leng[0] = VEC_LENGTH( vec );
            radius = rfac * leng[0];

            /* Outline the element. */
            glDepthFunc( GL_ALWAYS );
            glColor3fv( hilite_col );
            glBegin( GL_LINES );
            glVertex3fv( verts[0] );
            glVertex3fv( verts[1] );
            glEnd();
            glDepthFunc( GL_LEQUAL );

            break;
        case 2:
            /* Highlight a shell element. */
            shells = analy->geom_p->shells;

            /* Validity check. */
            if ( shells == NULL || hilite_num < 0 || hilite_num >=shells->cnt )
            {
                hilite_type = 0;
                wrt_text( "Shell number out of range!\n" );
                return;
            }
            if ( is_shell_result(analy->result_id) ||
                 is_shared_result(analy->result_id) )
	    {
	        val = analy->perform_unit_conversion
		      ? analy->shell_result[hilite_num] * analy->conversion_scale
		        + analy->conversion_offset
		      : analy->shell_result[hilite_num];
                sprintf( label, " Shell %d (%.*e)", hilite_num+1, fracsz, val );
	    }
            else
                sprintf( label, " Shell %d", hilite_num+1 );

            get_shell_verts( hilite_num, analy, verts );
            vert_cnt = 4;

            VEC_SUB( vec, verts[1], verts[0] );
            leng[0] = VEC_LENGTH( vec );
            VEC_SUB( vec, verts[3], verts[0] );
            leng[1] = VEC_LENGTH( vec );
            radius = rfac * (leng[0] + leng[1]) / 2.0;

            /* Outline the element. */
            glDepthFunc( GL_ALWAYS );
            glColor3fv( hilite_col );
            glBegin( GL_LINE_LOOP );
            for ( i = 0; i < 4; i++ )
                glVertex3fv( verts[i] );
            glEnd();
            glDepthFunc( GL_LEQUAL );

            break;
        case 3:
            /* Highlight a brick element. */
            bricks = analy->geom_p->bricks;

            /* Validity check. */
            if ( bricks == NULL || hilite_num < 0 || hilite_num >=bricks->cnt )
            {
                hilite_type = 0;
                wrt_text( "Brick number out of range!\n" );
                return;
            }
            if ( is_hex_result(analy->result_id) ||
                 is_shared_result(analy->result_id) )
	    {
	        val = analy->perform_unit_conversion
		      ? analy->hex_result[hilite_num] * analy->conversion_scale
		        + analy->conversion_offset
		      : analy->hex_result[hilite_num];
                sprintf( label, " Brick %d (%.*e)", hilite_num+1, fracsz, val );
	    }
            else
                sprintf( label, " Brick %d", hilite_num+1 );

            get_hex_verts( hilite_num, analy, verts );
            vert_cnt = 8;

            VEC_SUB( vec, verts[1], verts[0] );
            leng[0] = VEC_LENGTH( vec );
            VEC_SUB( vec, verts[3], verts[0] );
            leng[1] = VEC_LENGTH( vec );
            VEC_SUB( vec, verts[4], verts[0] );
            leng[2] = VEC_LENGTH( vec );
            radius = rfac * (leng[0] + leng[1] + leng[2]) / 3.0;

            /* Outline the element. */
            glDepthFunc( GL_ALWAYS );
            glColor3fv( hilite_col );
            glBegin( GL_LINES );
            for ( i = 0; i < 12; i++ )
            {
                glVertex3fv( verts[edge_node_nums[i][0]] );
                glVertex3fv( verts[edge_node_nums[i][1]] );
            }
            glEnd();
            glDepthFunc( GL_LEQUAL );

            break;
    }

    /* Draw spheres at the nodes of the element. */
    if ( analy->dimension == 3 )
    {
        if ( v_win->lighting )
            glEnable( GL_LIGHTING );
        glEnable( GL_COLOR_MATERIAL );
        glColor3fv( hilite_col );
        for ( i = 0; i < vert_cnt; i++ )
            draw_sphere( verts[i], radius );
        glDisable( GL_COLOR_MATERIAL );
        if ( v_win->lighting )
            glDisable( GL_LIGHTING );
    }

    /* Get position for the label. */
    for ( j = 0; j < 3; j++ )
        vec[j] = 0.0;
    for ( i = 0; i < vert_cnt; i++ )
        for ( j = 0; j < 3; j++ )
            vec[j] += verts[i][j] / vert_cnt;            

    /* Draw the element label, it goes on top of everything. */
    glDepthFunc( GL_ALWAYS );
    glColor3fv( v_win->foregrnd_color );
    draw_3d_text( vec, label );
    glDepthFunc( GL_LEQUAL );
}


/************************************************************
 * TAG( draw_elem_numbers )
 *
 * Do the node or element numbering.
 */
static void
draw_elem_numbers( analy )
Analysis *analy;
{
    Nodal *nodes;
    Beam_geom *beams;
    Shell_geom *shells;
    Hex_geom *bricks;
    float pt[3];
    char label[30];
    int el, face, nd, *ndflag;
    int i, j, k;

    nodes = analy->state_p->nodes;
    beams = analy->geom_p->beams;
    shells = analy->geom_p->shells;
    bricks = analy->geom_p->bricks;

    glColor3fv( v_win->foregrnd_color );
    glDepthFunc( GL_ALWAYS );

    /*
     * Label elements.
     */
    if ( analy->show_elem_nums )
    {
        if ( beams != NULL )
        {
            for ( i = 0; i < beams->cnt; i++ )
            {
                /* Get label position and label. */
                if ( analy->hide_material[beams->mat[i]] )
                    continue;
                memset( pt, 0, 3*sizeof(float) );
                for ( j = 0; j < 2; j++ )
                    for ( k = 0; k < 3; k++ )
                        pt[k] += nodes->xyz[k][beams->nodes[j][i]] / 2.0;
                sprintf( label, "%d", i+1 );
                draw_3d_text( pt, label );
            }
        }
        if ( shells != NULL )
        {
            for ( i = 0; i < shells->cnt; i++ )
            {
                /* Get label position and label. */
                if ( analy->hide_material[shells->mat[i]] )
                    continue;
                memset( pt, 0, 3*sizeof(float) );
                for ( j = 0; j < 4; j++ )
                    for ( k = 0; k < 3; k++ )
                        pt[k] += nodes->xyz[k][shells->nodes[j][i]] / 4.0;
                sprintf( label, "%d", i+1 );
                draw_3d_text( pt, label );
            }
        }
        if ( bricks != NULL )
        {
            for ( i = 0; i < analy->face_cnt; i++ )
            {
                el = analy->face_el[i];
                face = analy->face_fc[i];

                if ( analy->hide_material[bricks->mat[el]] )
                    continue;

                memset( pt, 0, 3*sizeof(float) );
                for ( j = 0; j < 4; j++ )
                {
                    nd = bricks->nodes[ fc_nd_nums[face][j] ][el];
                    for ( k = 0; k < 3; k++ )
                        pt[k] += nodes->xyz[k][nd] / 4.0;
                }
                sprintf( label, "%d", el+1 );
                draw_3d_text( pt, label );
            }
        }
    }    

    /*
     * Label nodes.
     */

    if ( analy->show_node_nums )
    {
        ndflag = NEW_N( int, nodes->cnt, "Temp node label flags" );

        /* Mark the nodes to be labeled. */
        if ( beams != NULL )
            for ( i = 0; i < beams->cnt; i++ )
                if ( !analy->hide_material[beams->mat[i]] )
                    for ( j = 0; j < 2; j++ )
                        ndflag[ beams->nodes[j][i] ] = 1;
        if ( shells != NULL )
            for ( i = 0; i < shells->cnt; i++ )
                if ( !analy->hide_material[shells->mat[i]] )
                    for ( j = 0; j < 4; j++ )
                        ndflag[ shells->nodes[j][i] ] = 1;
        if ( bricks != NULL )
        {
            for ( i = 0; i < analy->face_cnt; i++ )
            {
                el = analy->face_el[i];
                face = analy->face_fc[i];

                if ( !analy->hide_material[bricks->mat[el]] )
                    for ( j = 0; j < 4; j++ )
                        ndflag[bricks->nodes[fc_nd_nums[face][j]][el]] = 1;
            }
        }

        for ( i = 0; i < nodes->cnt; i++ )
        {
            if ( ndflag[i] )
            {
                pt[0] = nodes->xyz[0][i];
                pt[1] = nodes->xyz[1][i];
                pt[2] = nodes->xyz[2][i];

                sprintf( label, "%d", i+1 );

                draw_3d_text( pt, label );
            }
        }

        free( ndflag );
    }
    
    glDepthFunc( GL_LEQUAL );
}


/************************************************************
 * TAG( draw_bbox )
 *
 * Draw the bounding box of the mesh.
 */
static void
draw_bbox( bbox )
float bbox[2][3];
{
    float verts[8][3];
    int i;

    get_verts_of_bbox( bbox, verts );

    antialias_lines( TRUE, FALSE );

    glColor3fv( v_win->foregrnd_color );

    glBegin( GL_LINES );
    for ( i = 0; i < 12; i++ )
    {
        glVertex3fv( verts[edge_node_nums[i][0]] );
        glVertex3fv( verts[edge_node_nums[i][1]] );
    }
    glEnd();

    antialias_lines( FALSE, 0 );
}


/************************************************************
 * TAG( draw_extern_polys )
 *
 * Draw polygons that were read from an external file.
 */
static void
draw_extern_polys( analy )
Analysis *analy;
{
    Surf_poly *poly;
    float cols[4][4];
    float res[4];
    int matl, i;

    /* Dummy values. */
    for ( i = 0; i < 4; i++ )
        res[i] = 0.0;

    /* Set up for polygon drawing. */
    begin_draw_poly( analy );

    for ( poly = analy->extern_polys; poly != NULL; poly = poly->next )
    {
        matl = poly->mat;
        if ( v_win->current_material != matl )
            change_current_material( matl );

        for ( i = 0; i < poly->cnt; i++ )
            hvec_copy( cols[i], v_win->matl_diffuse[ matl%MAX_MATERIALS ] );

        draw_poly( poly->cnt, poly->vtx, poly->norm, cols, res, matl, analy );
    }

    /* Back to the default. */
    end_draw_poly( analy );
}


/************************************************************
 * TAG( draw_ref_polys )
 *
 * Draw reference surface polygons.
 */
static void
draw_ref_polys( analy )
Analysis *analy;
{
    extern float crease_threshold;
    Ref_poly *poly;
    Nodal *nodes;
    Bool_type colorflag;
    float v1[3], v2[3], f_norm[3], n_norm[3], dot;
    float verts[4][3];
    float norms[4][3];
    float cols[4][4];
    float res[4];
    int matl;
    int i, j;

    nodes = analy->state_p->nodes;

    /* Enable color to change AMBIENT and DIFFUSE properties. */
    glEnable( GL_COLOR_MATERIAL );

    /* Set up for polygon drawing. */
    begin_draw_poly( analy );

    for ( poly = analy->ref_polys; poly != NULL; poly = poly->next )
    {
        /* Flip polygon orientation by reversing vertex order. */
        for ( i = 0; i < 4; i++ )
            for ( j = 0; j < 3; j++ )
                verts[3-i][j] = nodes->xyz[j][poly->nodes[i]];

        /* Get polygon average normal. */
        VEC_SUB( v1, verts[2], verts[0] );
        VEC_SUB( v2, verts[3], verts[1] );
        VEC_CROSS( f_norm, v1, v2 );
        vec_norm( f_norm );

        /* Get polygon vertex normals using edge detection. */
        for ( i = 0; i < 4; i++ )
        {
            for ( j = 0; j < 3; j++ )
                n_norm[j] = analy->node_norm[j][poly->nodes[i]];
            vec_norm( n_norm );

            dot = VEC_DOT( f_norm, n_norm );

            if ( dot < crease_threshold )
            {
                /* Edge detected, use face normal. */
                for ( j = 0; j < 3; j++ )
                    norms[i][j] = f_norm[j];
            }
            else
            {
                /* No edge, use node normal. */
                for ( j = 0; j < 3; j++ )
                    norms[i][j] = n_norm[j];
            }
        }

        matl = poly->mat;
        if ( v_win->current_material != matl )
            change_current_material( matl );

        /* Colorflag is TRUE if displaying a result, otherwise
         * polygons are drawn in the material color.
         */
        colorflag = analy->result_on_refs &&
                    analy->show_hex_result &&
                    !analy->disable_material[matl];

        for ( i = 0; i < 4; i++ )
        {
            if ( analy->interp_mode == GOOD_INTERP )
            {
                res[3-i] = analy->result[poly->nodes[i]];
            }
            else
            {
                /* We don't know the element number, so there's no way
                 * to implement NO_INTERP on reference faces -- the
                 * vertex values are always interpolated.
                 */
                color_lookup( cols[3-i], analy->result[poly->nodes[i]],
                              analy->result_mm[0], analy->result_mm[1],
                              analy->zero_result, matl, colorflag );
            }
        }

        draw_poly( 4, verts, norms, cols, res, matl, analy );
    }

    /* Back to the defaults. */
    end_draw_poly( analy );
    glDisable( GL_COLOR_MATERIAL );
}


/************************************************************
 * TAG( draw_vec_result )
 *
 * Draw a vector result with a bunch of little line segment
 * glyphs.
 */
static void
draw_vec_result( analy )
Analysis *analy;
{
    Vector_pt_obj *pt;
    float vec_leng, vmag, vmin, vmax, diff;
    float tmp[3], pts[6];
    float leng[3], radius;
    int cnt, i;
    float scl_max;
    
    /* Cap for colorscale interpolation. */
    scl_max = SCL_MAX;

    /* See if there are some to draw. */
    if ( analy->vec_pts == NULL )
        return;

    /* Make the max vector length about VEC_3D_LENGTH pixels long. */

    /*
     * This (unpublished) option added to scale and color vectors by
     * the current result.  This must be exercised very carefully and
     * in general should only be used when the result is the magnitude
     * function of the vector quantity.
     */
    if ( analy->scale_vec_by_result && analy->result_id != VAL_NONE )
    {
        vmax = analy->result_mm[1];
        vmin = analy->result_mm[0];
    }
    else
    {
        /* Original assignments. */
        vmax = analy->vec_max_mag;
        vmin = analy->vec_min_mag;
    }
    
    diff = vmax - vmin;
    vec_leng = analy->vec_scale * VEC_3D_LENGTH /
               (v_win->vp_height * v_win->bbox_scale * vmax);

    antialias_lines( TRUE, analy->z_buffer_lines );

    /* Draw the vectors. */
    for ( pt = analy->vec_pts, cnt = 0; pt != NULL; pt = pt->next, cnt++ )
    {
        if ( analy->vec_col_set )
            glColor3fv( v_win->vector_color );
        else
        {
            /* Set the color based on the magnitude.  Avoid divide-by-zero. */
            if ( APX_EQ( diff, 0.0 ) )
                vmag = 0.0;
            else
                vmag = (sqrt((double)(VEC_DOT(pt->vec, pt->vec))) - vmin)/diff;
            i = (int)(vmag * scl_max) + 1;
            glColor3fv( v_win->colormap[i] );
        }

        /* Scale the vector, using zero as the min and vmax as the max. */
        VEC_COPY( pts, pt->pt );
        VEC_ADDS( tmp, vec_leng, pt->vec, pt->pt );
        pts[3] = tmp[0];
        pts[4] = tmp[1];
        pts[5] = tmp[2];

        draw_line( 2, pts, -1, analy, FALSE );
    }

    antialias_lines( FALSE, 0 );

    /* Get sphere radius for base of vector. */
    for ( i = 0; i < 3; i++ )
        leng[i] = analy->bbox[1][i] - analy->bbox[0][i];
    radius = 0.01 * (leng[0] + leng[1] + leng[2]) / 3.0;
    radius *= 1.0 / v_win->scale[0];

    /* Draw spheres. */
    if ( analy->show_vector_spheres )
    {
        glEnable( GL_COLOR_MATERIAL );
        if ( v_win->lighting )
            glEnable( GL_LIGHTING );

        for ( pt = analy->vec_pts; pt != NULL; pt = pt->next )
        {
            if ( analy->vec_col_set )
                glColor3fv( v_win->vector_color );
            else
            {
                /* Set the color based on the magnitude. */
                if ( APX_EQ( diff, 0.0 ) )
                    vmag = 0.0;
                else
                    vmag = (sqrt((double)(VEC_DOT(pt->vec, pt->vec)))-vmin) /
                           diff;
                i = (int)(vmag * scl_max) + 1;
                glColor3fv( v_win->colormap[i] );
            }
            draw_sphere( pt->pt, radius );
        }

        glDisable( GL_COLOR_MATERIAL );
        if ( v_win->lighting )
            glDisable( GL_LIGHTING );
    }
}


/************************************************************
 * TAG( draw_vec_result_2d )
 *
 * Draw a vector result with a bunch of little line segment
 * glyphs.  For 2D vectors, we can try to draw the vector
 * heads.
 */
static void
draw_vec_result_2d( analy )
Analysis *analy;
{
    Vector_pt_obj *pt;
    Transf_mat tmat;
    float vpx, vpy, pixsize, vec_leng, vmag, vmin, vmax;
    float diff, angle;
    float tmp[3], pts[6], res[3];
    float headpts[3][3], verts[4][3];
    float cols[4][4];
    int cnt, i;
    float scl_max;
    
    /* Cap for colorscale interpolation. */
    scl_max = SCL_MAX;

    /* See if there are some to draw. */
    if ( analy->vec_pts == NULL )
        return;

    /* Dummy. */
    VEC_SET( res, 0.0, 0.0, 0.0 );

    vpx = (float) v_win->vp_width;
    vpy = (float) v_win->vp_height;
    if ( vpx >= vpy )
        pixsize = 2.0 / ( vpy * v_win->bbox_scale * v_win->scale[1] );
    else
        pixsize = 2.0 / ( vpx * v_win->bbox_scale * v_win->scale[0] );

    /* Make the max vector length about VEC_2D_LENGTH pixels long. */
    vmax = analy->vec_max_mag;
    vmin = analy->vec_min_mag;
    diff = vmax - vmin;
    vec_leng = analy->vec_scale * VEC_2D_LENGTH * pixsize / vmax;

    /* Create the vector head points. */
    headpts[0][0] = 0.0;
    headpts[0][1] = -2.0*pixsize*analy->vec_head_scale;
    headpts[0][2] = 0.0;
    headpts[1][0] = 5.0*pixsize*analy->vec_head_scale;
    headpts[1][1] = 0.0;
    headpts[1][2] = 0.0;
    headpts[2][0] = 0.0;
    headpts[2][1] = 2.0*pixsize*analy->vec_head_scale;
    headpts[2][2] = 0.0;

    antialias_lines( TRUE, analy->z_buffer_lines );

    /* Draw the vectors. */
    for ( pt = analy->vec_pts, cnt = 0; pt != NULL; pt = pt->next, cnt++ )
    {
        if ( analy->vec_col_set )
        {
            VEC_COPY( cols[0], v_win->vector_color );
            glColor3fv( cols[0] );
        }
        else
        {
            /* Set the color based on the magnitude.  Avoid divide-by-zero. */
            if ( APX_EQ( diff, 0.0 ) )
                vmag = 0.0;
            else
                vmag = (sqrt((double)(VEC_DOT(pt->vec, pt->vec))) - vmin)/diff;
            i = (int)(vmag * scl_max) + 1;
            VEC_COPY( cols[0], v_win->colormap[i] );
            glColor3fv( cols[0] );
        }

        /* Scale the vector, using zero as the min and vmax as the max. */
        VEC_COPY( pts, pt->pt );
        VEC_ADDS( tmp, vec_leng, pt->vec, pt->pt );
        pts[3] = tmp[0];
        pts[4] = tmp[1];
        pts[5] = tmp[2];

        draw_line( 2, pts, -1, analy, FALSE );

        /* Draw the vector head. */
        if ( DEF_GT( VEC_LENGTH( pt->vec ), 0.0 ) )
        {
            VEC_COPY( cols[1], cols[0] );
            VEC_COPY( cols[2], cols[0] );

            VEC_SCALE( tmp, vec_leng, pt->vec );
            angle = atan2( (double)tmp[1], (double)tmp[0] );

            mat_copy( &tmat, &ident_matrix );
            mat_translate( &tmat, VEC_LENGTH( tmp ), 0.0, 0.0 );
            mat_rotate_z( &tmat, angle );
            mat_translate( &tmat, pt->pt[0], pt->pt[1], pt->pt[2] );

            for ( i = 0; i < 3; i++ )
                point_transform( verts[i], headpts[i], &tmat );
            draw_poly_2d( 3, verts, cols, res, -1, analy );
        }
    }

    antialias_lines( FALSE, 0 );
}


/************************************************************
 * TAG( draw_node_vec )
 *
 * Draw a vector result at the grid nodes.
 */
static void
draw_node_vec( analy )
Analysis *analy;
{
    Nodal *nodes;
    Bool_type draw_heads;
    Transf_mat tmat;
    float *vec_result[3];
    float vec_leng, vmag, vmin, vmax, diff;
    float vpx, vpy, pixsize, angle;
    float vec[3], pts[6], tmp[3];
    float res[3], headpts[3][3], verts[4][3], cols[4][4];
    float leng[3];
    float radius;
    int i, j;
    float scl_max;
    
    /* Cap for colorscale interpolation. */
    scl_max = SCL_MAX;

    nodes = analy->state_p->nodes;

    /* Load the three results into vector result array. */
    load_vec_result( analy, vec_result, &vmin, &vmax );

    /* Make the max vector length about VEC_3D_LENGTH pixels long. */
    diff = vmax - vmin;
    vec_leng = analy->vec_scale * VEC_3D_LENGTH /
               (v_win->vp_height * v_win->bbox_scale * vmax);

    if ( analy->dimension == 2 )
        draw_heads = TRUE;
    else
        draw_heads = FALSE;

    /* Get vector head points. */
    if ( draw_heads )
    {
        /* Dummy. */
        VEC_SET( res, 0.0, 0.0, 0.0 );

        vpx = (float) v_win->vp_width;
        vpy = (float) v_win->vp_height;
        if ( vpx >= vpy )
            pixsize = 2.0 / ( vpy * v_win->bbox_scale * v_win->scale[1] );
        else
            pixsize = 2.0 / ( vpx * v_win->bbox_scale * v_win->scale[0] );

        /* Make the max vector length about VEC_2D_LENGTH pixels long. */
        vec_leng = analy->vec_scale * VEC_2D_LENGTH * pixsize / vmax;

        /* Create the vector head points. */
        headpts[0][0] = 0.0;
        headpts[0][1] = -2.0*pixsize*analy->vec_head_scale;
        headpts[0][2] = 0.0;
        headpts[1][0] = 5.0*pixsize*analy->vec_head_scale;
        headpts[1][1] = 0.0;
        headpts[1][2] = 0.0;
        headpts[2][0] = 0.0;
        headpts[2][1] = 2.0*pixsize*analy->vec_head_scale;
        headpts[2][2] = 0.0;
    }
    else if ( analy->show_vector_spheres )
    {
        /* Get sphere radius for base of vector. */
        for ( i = 0; i < 3; i++ )
            leng[i] = analy->bbox[1][i] - analy->bbox[0][i];
        radius = 0.01 * (leng[0] + leng[1] + leng[2]) / 3.0;
        radius *= 1.0 / v_win->scale[0];

        /* Initialize lighting for sphere rendering. */
        glEnable( GL_COLOR_MATERIAL );
        if ( v_win->lighting )
            glEnable( GL_LIGHTING );
    }

    antialias_lines( TRUE, analy->z_buffer_lines );

    /* Draw the vectors. */
    for ( i = 0; i < nodes->cnt; i++ )
    {
        for ( j = 0; j < 3; j++ )
        {
            pts[j] = nodes->xyz[j][i];
            vec[j] = vec_result[j][i];
        }

        if ( analy->vec_col_set )
        {
            VEC_COPY( cols[0], v_win->vector_color );
        }
        else
        {
            /* Set the color based on the magnitude.  Avoid divide-by-zero. */
            if ( APX_EQ( diff, 0.0 ) )
                vmag = 0.0;
            else
                vmag = (sqrt((double)(VEC_DOT(vec, vec))) - vmin)/diff;
            j = (int)(vmag * scl_max) + 1;
            VEC_COPY( cols[0], v_win->colormap[j] );
        }
        glColor3fv( cols[0] );

        /* Scale the vector, using zero as the min and vmax as the max. */
        VEC_ADDS( tmp, vec_leng, vec, pts );
        pts[3] = tmp[0];
        pts[4] = tmp[1];
        pts[5] = tmp[2];

        draw_line( 2, pts, -1, analy, FALSE );

        /* Draw the vector head. */
        if ( draw_heads )
        {
            if ( DEF_GT( VEC_LENGTH( vec ), 0.0 ) )
            {
                VEC_COPY( cols[1], cols[0] );
                VEC_COPY( cols[2], cols[0] );

                VEC_SCALE( tmp, vec_leng, vec );
                angle = atan2( (double)tmp[1], (double)tmp[0] );

                mat_copy( &tmat, &ident_matrix );
                mat_translate( &tmat, VEC_LENGTH( tmp ), 0.0, 0.0 );
                mat_rotate_z( &tmat, angle );
                mat_translate( &tmat, pts[0], pts[1], pts[2] );

                for ( j = 0; j < 3; j++ )
                    point_transform( verts[j], headpts[j], &tmat );
                draw_poly_2d( 3, verts, cols, res, -1, analy );
            }
        }
	else if ( analy->show_vector_spheres )
            draw_sphere( pts, radius );
    }

    antialias_lines( FALSE, 0 );

    if ( !draw_heads && analy->show_vector_spheres )
    {
        glDisable( GL_COLOR_MATERIAL );
        if ( v_win->lighting )
            glDisable( GL_LIGHTING );
    }
    
    /* Free temporary arrays. */
    for ( i = 0; i < 3; i++ )
        free( vec_result[i] );
}


/*
 * Carpetting routines
 *
 *     Carpeting reference surfaces
 *     Carpeting cut planes and isosurfaces
 *     Volume carpeting - regular sampling method
 *     Volume carpeting - density/sorting method (not implemented yet)
 */


/************************************************************
 * TAG( load_vec_result )
 *
 * Load in the current vector result.  Also, return the min and
 * max magnitude of the vector result.  The vec result array is
 * allocated by this routine and needs to be deallocated by the
 * caller.
 */
static void
load_vec_result( analy, vec_result, vmin, vmax )
Analysis *analy;
float *vec_result[3];
float *vmin;
float *vmax;
{
    Result_type tmp_id;
    Bool_type init, convert;
    float *tmp_res;
    float vec[3];
    float mag, min, max;
    int node_cnt, i;
    float scale, offset;

    node_cnt = analy->state_p->nodes->cnt;

    for ( i = 0; i < 3; i++ )
        vec_result[i] = NEW_N( float, node_cnt, "Vec result" );

    tmp_id = analy->result_id;
    tmp_res = analy->result;
    for ( i = 0; i < 3; i++ )
    {
        analy->result_id = analy->vec_id[i];
        analy->result = vec_result[i];
        load_result( analy, FALSE );
    }
    analy->result_id = tmp_id;
    analy->result = tmp_res;
    
    convert = analy->perform_unit_conversion;
    if ( convert )
    {
        scale = analy->conversion_scale;
        offset = analy->conversion_offset;
    }

    /* Get the min and max magnitude for the vector result. */
    init = FALSE;
    for ( i = 0; i < node_cnt; i++ )
    {
        if ( convert )
        {
            vec_result[0][i] = vec_result[0][i] * scale + offset;
            vec_result[1][i] = vec_result[1][i] * scale + offset;
            vec_result[2][i] = vec_result[2][i] * scale + offset;
        }

        vec[0] = vec_result[0][i];
        vec[1] = vec_result[1][i];
        vec[2] = vec_result[2][i];
        mag = VEC_DOT( vec, vec );

        if ( init )
        {
            if ( mag > max )
                max = mag;
            else if ( mag < min )
                min = mag;
        }
        else
        {
            min = mag;
            max = mag;
            init = TRUE;
        }
    }
    *vmin = sqrt( (double)min );
    *vmax = sqrt( (double)max );
}


/************************************************************
 * TAG( find_front_faces )
 *
 * Determines which of the six faces of a 3D grid are front-
 * facing in the current view, and returns this information
 * in the front array.
 */
static void
find_front_faces( front, analy )
Bool_type front[6];
Analysis *analy;
{
    static float face_norms[6][3] = { {-1.0,  0.0,  0.0},
                                      { 1.0,  0.0,  0.0},
                                      { 0.0, -1.0,  0.0},
                                      { 0.0,  1.0,  0.0},
                                      { 0.0,  0.0, -1.0},
                                      { 0.0,  0.0,  1.0} };
    Transf_mat inv_rot_mat;
    float eye[3], rot_eye[3];
    int i, j;

    /*
     * This doesn't take into account the look_at point or the
     * perspective, but we'll forget about that for now.
     */
    VEC_COPY( eye, v_win->look_from );

    /* Get the inverse of the view rotation matrix. */
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 4; j++ )
            inv_rot_mat.mat[i][j] = v_win->rot_mat.mat[j][i];

    point_transform( rot_eye, eye, &inv_rot_mat );

    for ( i = 0; i < 6; i++ )
    {
        if ( VEC_DOT( face_norms[i], rot_eye ) > 0.0 )
            front[i] = TRUE;
        else
            front[i] = FALSE;
    }
}


/************************************************************
 * TAG( draw_carpet_vector )
 *
 * Draw a single carpet vector "hair".
 */
static void
draw_carpet_vector( pt, vec, vec_leng, vmin, vmax, analy )
float pt[3];
float vec[3];
float vec_leng;
float vmin;
float vmax;
Analysis *analy;
{
    float pts[2][3], cols[2][4];
    float vec2[3], vec3[3];
    float vmag, diff, scale;
    int i;
    float scl_max;
    
    /* Cap for colorscale interpolation. */
    scl_max = SCL_MAX;

    diff = vmax - vmin;

    if ( analy->vec_length_factor > 0.0 )
        vec_leng = vec_leng * vmax;

    /* Get vector color. */
    cols[0][3] = 1.0;
    cols[1][3] = 1.0;
    if ( analy->vec_col_set )
    {
        VEC_COPY( cols[0], v_win->vector_color );
    }
    else
    {
        /* Set the color based on the magnitude. */
        if ( APX_EQ( diff, 0.0 ) )
            vmag = 0.0;
        else
            vmag = (sqrt((double)VEC_DOT(vec, vec)) - vmin)/diff;
        i = (int)(vmag * scl_max) + 1;
        VEC_COPY( cols[0], v_win->colormap[i] );
    }
    if ( analy->vec_hd_col_set )
    {
        VEC_COPY( cols[1], v_win->vector_hd_color );
    }
    else
    {
        VEC_COPY( cols[1], cols[0] );
    }

    /* Jitter intensity of vector randomly. */
    if ( analy->vec_jitter_factor > 0.0 )
    {
        scale = analy->vec_jitter_factor * drand48() +
                1.0 - analy->vec_jitter_factor;
        VEC_SCALE( cols[0], scale, cols[0] );
        VEC_SCALE( cols[1], scale, cols[1] );
    }

    /* Modulate opacity based on importance. */
    if ( analy->vec_import_factor > 0.0 )
    {
        if ( APX_EQ( diff, 0.0 ) )
            vmag = 0.0;
        else
            vmag = (sqrt((double)VEC_DOT(vec, vec)) - vmin)/diff;

        /* The factor tells us the maximum opacity. */
        vmag = vmag * analy->vec_import_factor;

        /* Do a squared drop off. */
        cols[0][3] = vmag * vmag;
        cols[1][3] = cols[0][3];
    }

    /* Offset the vector scaling. */
    if ( analy->vec_length_factor > 0.0 )
    {
        vmag = sqrt((double)VEC_DOT(vec, vec));
        if ( vmag == 0.0 )
            return;
        VEC_SCALE( vec2, analy->vec_length_factor / vmag, vec );
        VEC_SCALE( vec3, (1.0 - analy->vec_length_factor) / vmax, vec );
        VEC_ADD( vec, vec2, vec3 );
    }

    /* Scale the vector. */
    VEC_ADDS( pts[0], -0.5 * vec_leng, vec, pt );
    VEC_ADDS( pts[1], vec_leng, vec, pts[0] );

    /* Draw a line segment -- no reflection is done. */
    glBegin( GL_LINES );
    glColor4fv( cols[0] );
    glVertex3fv( pts[0] );
    glColor4fv( cols[1] );
    glVertex3fv( pts[1] );
    glEnd();
}


/************************************************************
 * TAG( draw_reg_carpet )
 *
 * Draw a vector carpet on a jittered regular grid through the
 * volume of the model.
 */
static void
draw_reg_carpet( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    Bool_type front[6];
    float *vec_result[3];
    float vec_leng, vmin, vmax;
    float r, s, t, h[8];
    float vec[3], pt[3];
    int *reg_dim;
    int si, ei, incri, sj, ej, incrj, sk, ek, incrk;
    int ii, jj, kk, el, idx;
    int i, j, k;

    /* See if there are some to draw. */
    if ( analy->reg_dim[0] == 0 )
        return;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    reg_dim = analy->reg_dim;

    /* Load the three results into vector result array. */
    load_vec_result( analy, vec_result, &vmin, &vmax );

    /* Make the max vector length about VEC_3D_LENGTH pixels long. */
    vec_leng = analy->vec_scale * VEC_3D_LENGTH /
               (v_win->vp_height * v_win->bbox_scale * vmax);

    /* Determine traversal directions. */
    find_front_faces( front, analy );

    if ( front[0] )
    {
        si = reg_dim[0] - 1;
        ei = -1;
        incri = -1;
    }
    else
    {
        si = 0;
        ei = reg_dim[0];
        incri = 1;
    }
    if ( front[2] )
    {
        sj = reg_dim[1] - 1;
        ej = -1;
        incrj = -1;
    }
    else
    {
        sj = 0;
        ej = reg_dim[1];
        incrj = 1;
    }
    if ( front[4] )
    {
        sk = reg_dim[2] - 1;
        ek = -1;
        incrk = -1;
    }
    else
    {
        sk = 0;
        ek = reg_dim[2];
        incrk = 1;
    }

    antialias_lines( TRUE, analy->z_buffer_lines );

    /* Loop through grid points from back to front. */
    for ( ii = si; ii != ei; ii += incri )
        for ( jj = sj; jj != ej; jj += incrj )
            for ( kk = sk; kk != ek; kk += incrk )
            {
                idx = ii*reg_dim[1]*reg_dim[2] + jj*reg_dim[2] + kk;

                /* Calculate the shape functions. */
                el = analy->reg_carpet_elem[idx];

                r = analy->reg_carpet_coords[0][idx];
                s = analy->reg_carpet_coords[1][idx];
                t = analy->reg_carpet_coords[2][idx];
                shape_fns_3d( r, s, t, h );

                /* Get the physical coordinates of the vector point. */
                VEC_SET( pt, 0.0, 0.0, 0.0 );
                for ( j = 0; j < 8; j++ )
                    for ( k = 0; k < 3; k++ )
                        pt[k] += h[j]*nodes->xyz[k][ bricks->nodes[j][el] ];

                /* Interpolate the vector quantity to the vector point. */
                VEC_SET( vec, 0.0, 0.0, 0.0 );
                for ( j = 0; j < 8; j++ )
                    for ( k = 0; k < 3; k++ )
                        vec[k] += h[j]*vec_result[k][ bricks->nodes[j][el] ];

                /* Draw the vector glyph. */
                draw_carpet_vector( pt, vec, vec_leng, vmin, vmax, analy );
            }

    antialias_lines( FALSE, 0 );

    /* Free temporary arrays. */
    for ( i = 0; i < 3; i++ )
        free( vec_result[i] );
}


/************************************************************
 * TAG( draw_vol_carpet )
 *
 * Draw carpet points contained in volume elements.
 */
static void
draw_vol_carpet( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    float *vec_result[3];
    float vec_leng, vmin, vmax;
    float r, s, t, h[8];
    float vec[3], pt[3];
    int *index;
    int el, i, j, k;

    /* See if there are some to draw. */
    if ( analy->vol_carpet_cnt == 0 )
        return;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;

    /* Load the three results into vector result array. */
    load_vec_result( analy, vec_result, &vmin, &vmax );

    /* Avoid divide-by-zero, nothing to show in this case. */
    if ( vmax == 0.0 )
        return;

    /* Make the max vector length about VEC_3D_LENGTH pixels long. */
    vec_leng = analy->vec_scale * VEC_3D_LENGTH /
               (v_win->vp_height * v_win->bbox_scale * vmax);

    antialias_lines( TRUE, analy->z_buffer_lines );

    /* Sort the vector points. */
    index = NEW_N( int, analy->vol_carpet_cnt, "Carpet tmp" );
    if ( TRUE )
    {
        sort_carpet_points( analy, analy->vol_carpet_cnt,
                            analy->vol_carpet_coords,
                            analy->vol_carpet_elem, index, TRUE );
    }
    else
    {
        for ( i = 0; i < analy->vol_carpet_cnt; i++ )
            index[i] = i;
    }

    for ( i = analy->vol_carpet_cnt - 1; i >= 0; i-- )
    {
        /* Calculate the shape functions. */
        el = analy->vol_carpet_elem[index[i]];

        r = analy->vol_carpet_coords[0][index[i]];
        s = analy->vol_carpet_coords[1][index[i]];
        t = analy->vol_carpet_coords[2][index[i]];
        shape_fns_3d( r, s, t, h );

        /* Get the physical coordinates of the vector point. */
        VEC_SET( pt, 0.0, 0.0, 0.0 );
        for ( j = 0; j < 8; j++ )
            for ( k = 0; k < 3; k++ )
                pt[k] += h[j]*nodes->xyz[k][ bricks->nodes[j][el] ];

        /* Interpolate the vector quantity to the vector point. */
        VEC_SET( vec, 0.0, 0.0, 0.0 );
        for ( j = 0; j < 8; j++ )
            for ( k = 0; k < 3; k++ )
                vec[k] += h[j]*vec_result[k][ bricks->nodes[j][el] ];

        /* Draw the vector glyph. */
        draw_carpet_vector( pt, vec, vec_leng, vmin, vmax, analy );
    }

    antialias_lines( FALSE, 0 );

    /* Free temporary arrays. */
    for ( i = 0; i < 3; i++ )
        free( vec_result[i] );
    free( index );
}


/************************************************************
 * TAG( draw_shell_carpet )
 *
 * Draw carpet points contained in shell elements.
 */
static void
draw_shell_carpet( analy )
Analysis *analy;
{
    Shell_geom *shells;
    Nodal *nodes;
    float *vec_result[3];
    float vec_leng, vmin, vmax;
    float r, s, h[4];
    float vec[3], pt[3];
    int *index;
    int el, i, j, k;

    /* See if there are some to draw. */
    if ( analy->shell_carpet_cnt == 0 )
        return;

    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;

    /* Load the three results into vector result array. */
    load_vec_result( analy, vec_result, &vmin, &vmax );

    /* Avoid divide-by-zero, nothing to show in this case. */
    if ( vmax == 0.0 )
        return;

    /* Make the max vector length about VEC_3D_LENGTH pixels long. */
    vec_leng = analy->vec_scale * VEC_3D_LENGTH /
               (v_win->vp_height * v_win->bbox_scale * vmax);

    antialias_lines( TRUE, analy->z_buffer_lines );

    /* Sort the vector points. */
    index = NEW_N( int, analy->shell_carpet_cnt, "Carpet tmp" );
    if ( analy->dimension == 3 )
    {
        sort_carpet_points( analy, analy->shell_carpet_cnt,
                            analy->shell_carpet_coords,
                            analy->shell_carpet_elem, index, FALSE );
    }
    else
    {
        for ( i = 0; i < analy->shell_carpet_cnt; i++ )
            index[i] = i;
    }

    for ( i = analy->shell_carpet_cnt - 1; i >= 0; i-- )
    {
        /* Calculate the shape functions. */
        el = analy->shell_carpet_elem[index[i]];
        r = analy->shell_carpet_coords[0][index[i]];
        s = analy->shell_carpet_coords[1][index[i]];
        shape_fns_2d( r, s, h );

        /* Get the physical coordinates of the vector point. */
        VEC_SET( pt, 0.0, 0.0, 0.0 );
        for ( j = 0; j < 4; j++ )
            for ( k = 0; k < 3; k++ )
                pt[k] += h[j]*nodes->xyz[k][ shells->nodes[j][el] ];

        /* Interpolate the vector quantity to the vector point. */
        VEC_SET( vec, 0.0, 0.0, 0.0 );
        for ( j = 0; j < 4; j++ )
            for ( k = 0; k < 3; k++ )
                vec[k] += h[j]*vec_result[k][ shells->nodes[j][el] ];

        /* Draw the vector glyph. */
        draw_carpet_vector( pt, vec, vec_leng, vmin, vmax, analy );
    }

    antialias_lines( FALSE, 0 );

    /* Free temporary arrays. */
    for ( i = 0; i < 3; i++ )
        free( vec_result[i] );
    free( index );
}


/************************************************************
 * TAG( draw_ref_carpet )
 *
 * Draw a vector carpet on the reference surface.
 */
static void
draw_ref_carpet( analy )
Analysis *analy;
{
    Ref_poly *poly;
    Nodal *nodes;
    float *vec_result[3];
    float vec_leng, vmin, vmax;
    float pr, ps, h[4];
    float verts[4][3], vec[3], pt[3];
    float vecs_per_area, vec_num, remain;
    float scale;
    int vec_cnt, seed;
    int i, j, k;

    /* See if there are some to draw. */
    if ( analy->ref_polys == NULL )
        return;

    nodes = analy->state_p->nodes;

    /* Load the three results into vector result array. */
    load_vec_result( analy, vec_result, &vmin, &vmax );

    /* Make the max vector length about VEC_3D_LENGTH pixels long. */
    vec_leng = analy->vec_scale * VEC_3D_LENGTH /
               (v_win->vp_height * v_win->bbox_scale * vmax);
    vecs_per_area = 1.0 / ( analy->vec_cell_size * analy->vec_cell_size );

    antialias_lines( TRUE, analy->z_buffer_lines );

    /* Seed the random function so jitter is consistent across passes. */
    seed = 50731;
    srand48( seed );

    for ( poly = analy->ref_polys; poly != NULL; poly = poly->next )
    {
        for ( i = 0; i < 4; i++ )
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][poly->nodes[i]];

        vec_num = area_of_quad( verts ) * vecs_per_area;
        vec_cnt = (int) vec_num;
        remain = vec_num - vec_cnt;

        if ( drand48() < remain )
            ++vec_cnt;

        /* Draw the vectors for this face. */
        for ( i = 0; i < vec_cnt; i++ )
        {
            pr = 2.0*drand48() - 1.0;
            ps = 2.0*drand48() - 1.0;

            /* Map these to physical coordinates, using shape functions. */
            shape_fns_2d( pr, ps, h );

            for ( k = 0; k < 3; k++ )
                pt[k] = h[0]*verts[0][k] + h[1]*verts[1][k] +
                        h[2]*verts[2][k] + h[3]*verts[3][k];

            /* Interpolate the vector quantities to the display point. */
            VEC_SET( vec, 0.0, 0.0, 0.0 );
            for ( j = 0; j < 4; j++ )
                for ( k = 0; k < 3; k++ )
                    vec[k] += h[j]*vec_result[k][poly->nodes[j]];

            /* Draw the vector glyph. */
            draw_carpet_vector( pt, vec, vec_leng, vmin, vmax, analy );
        }
    }

    antialias_lines( FALSE, 0 );

    /* Free temporary array. */
    for ( i = 0; i < 3; i++ )
        free( vec_result[i] );
}


/************************************************************
 * TAG( draw_traces )
 *
 * Draw all particle trace paths.
 */
static void
draw_traces( analy )
Analysis *analy;
{

    /* Draw existing traces. */
    if ( analy->trace_pts != NULL )
        draw_trace_list( analy->trace_pts, analy );
    
    /* Draw traces currently under construction. */
    if ( analy->new_trace_pts != NULL )
        draw_trace_list( analy->new_trace_pts, analy );
}


/************************************************************
 * TAG( draw_trace_list )
 *
 * Draw particle trace paths in a list.
 */
static void
draw_trace_list( ptlist, analy )
Trace_pt_obj *ptlist;
Analysis *analy;
{
    Trace_pt_obj *pt;
    float tmp[3], pts[6];
    float time;
    float rgb[3];
    int i;
    int skip, limit, new_skip, new_limit;
    Bool_type init_subtrace;

    limit = analy->ptrace_limit;

    antialias_lines( TRUE, analy->z_buffer_lines );
    glLineWidth( (GLfloat) analy->trace_width );

    time = analy->state_p->time;

    for ( pt = ptlist; pt != NULL; pt = pt->next )
    {
        if ( pt->color[0] < 0 )
            glColor3fv( v_win->foregrnd_color );
        else
        {
            VEC_COPY( rgb, pt->color );
            glColor3fv( rgb );
        }

        /* For static field traces, only draw if current time is trace time. */
	if ( pt->time[0] == pt->time[1] )
	{
	    if ( pt->time[0] != time )
	        continue;
	    else
	        i = pt->cnt;
	}
	else
            /* Only draw the trace up to the current time. */
            for ( i = 0; i < pt->cnt; i++ )
                if ( pt->time[i] > time )
                    break;
        
	skip = 0;
	if ( limit > 0 )
	    if ( i > limit )
	    {
		skip = i - limit;
		i = limit;
	    }
        
	if ( analy->trace_disable != NULL )
	{
	    init_subtrace = TRUE;
	    while ( find_next_subtrace( init_subtrace, skip, i, pt, analy, 
	                                &new_skip, &new_limit ) )
	    {
		init_subtrace = FALSE;
		draw_line( new_limit, pt->pts + 3 * new_skip, -1, analy, FALSE );
	    }
	}
	else
            draw_line( i, pt->pts + 3 * skip, -1, analy, FALSE );
    }

    glLineWidth( (GLfloat) 1.0 );
    antialias_lines( FALSE, 0 );
}


/************************************************************
 * TAG( draw_sphere )
 *
 * Draw a polygonized sphere.  The center of the sphere and
 * the radius are specified by parameters.  There are better
 * ways to do this -- e.g. call the GL utility code.
 */
static void
draw_sphere( ctr, radius )
float ctr[3];
float radius;
{
    float latincr, longincr, latangle, longangle, length;
    float vert[3], norm[3];
    int latres, longres, i, j;

    latres = 8;
    longres = 5;

    latincr = 2.0*PI / latres;
    longincr = PI / longres;

    /* Draw a unit sphere. */
    glBegin( GL_QUADS );
    for ( i = 0, latangle = 0.0; i < latres; i++, latangle += latincr )
    {
        for ( j = 0, longangle = -PI/2.0;
              j < longres;
              j++, longangle += longincr )
        {
            length = cos( (double)longangle );
            norm[0] = cos( (double)latangle ) * length;
            norm[1] = sin( (double)latangle ) * length;
            norm[2] = sin( (double)longangle );
            VEC_ADDS( vert, radius, norm, ctr );
            glNormal3fv( norm );
            glVertex3fv( vert );
            norm[0] = cos( (double)(latangle+latincr) ) * length;
            norm[1] = sin( (double)(latangle+latincr) ) * length;
            VEC_ADDS( vert, radius, norm, ctr );
            glNormal3fv( norm );
            glVertex3fv( vert );
            length = cos( (double)(longangle+longincr) );
            norm[0] = cos( (double)(latangle+latincr) ) * length;
            norm[1] = sin( (double)(latangle+latincr) ) * length;
            norm[2] = sin( (double)(longangle+longincr) );
            VEC_ADDS( vert, radius, norm, ctr );
            glNormal3fv( norm );
            glVertex3fv( vert );
            norm[0] = cos( (double)latangle ) * length;
            norm[1] = sin( (double)latangle ) * length;
            VEC_ADDS( vert, radius, norm, ctr );
            glNormal3fv( norm );
            glVertex3fv( vert );
        }
    }
    glEnd();
}


/************************************************************
 * TAG( draw_poly )
 *
 * Draw a polygon, reflecting it as needed.  The routine expects
 * either a quadrilateral or a triangle.  The matl flag is the
 * material of the polygon.  A value of -1 indicates
 * that no material is associated with the polygon.
 *
 * NOTE: the calling routine needs to set up and shut down the 
 * stencil buffer for polygon edge drawing (in hidden line mode.)
 */
static void
draw_poly( cnt, pts, norm, cols, vals, matl, analy )
int cnt;
float pts[4][3];
float norm[4][3];
float cols[4][4];
float vals[4];
int matl;
Analysis *analy;
{
    Refl_plane_obj *plane;
    Render_poly_obj *poly, *rpoly, *origpoly, *reflpoly;
    float rpts[4][3];
    float rnorm[4][3];
    float rcols[4][4];
    float rvals[4];
    int i, ii, j;

    if ( analy->translate_material )
    {
        if ( matl >= 0 )
            for ( i = 0; i < cnt; i++ )
                for ( j = 0; j < 3; j++ )
                    pts[i][j] += analy->mtl_trans[j][matl];
    }

    /* Draw the initial polygon. */
    if ( analy->render_mode == RENDER_HIDDEN )
        draw_edged_poly( cnt, pts, norm, cols, vals, matl, analy );
    else
        draw_plain_poly( cnt, pts, norm, cols, vals, matl, analy );

    if ( !analy->reflect || analy->refl_planes == NULL )
        return;

    if ( analy->refl_planes->next == NULL )
    {
        /* Just one reflection plane; blast it out.
         * The polygon's node order is reversed by reflecting
         * the individual points, so we have to correct the order.
         */
        plane = analy->refl_planes;
        for ( i = 0; i < cnt; i++ )
        {
            ii = cnt - 1 - i;
            point_transform( rpts[i], pts[ii], &plane->pt_transf );
            point_transform( rnorm[i], norm[ii], &plane->norm_transf );
            hvec_copy( rcols[i], cols[ii] );
            rvals[i] = vals[ii];
        }

        if ( analy->render_mode == RENDER_HIDDEN )
            draw_edged_poly( cnt, rpts, rnorm, rcols, rvals, matl, analy );
        else
            draw_plain_poly( cnt, rpts, rnorm, rcols, rvals, matl, analy );
    }
    else
    {
        /* More than one reflection plane, do it the hard way. */

        origpoly = NEW( Render_poly_obj, "Reflection polygon" );
        origpoly->cnt = cnt;
        for ( i = 0; i < cnt; i++ )
        {
            VEC_COPY( origpoly->pts[i], pts[i] );
            VEC_COPY( origpoly->norm[i], norm[i] );
            hvec_copy( origpoly->cols[i], cols[i] );
            origpoly->vals[i] = vals[i];
        }

        reflpoly = NULL;

        for ( plane = analy->refl_planes;
              plane != NULL;
              plane = plane->next )
        {
            for ( poly = origpoly; poly != NULL; poly = poly->next )
            {
                rpoly = NEW( Render_poly_obj, "Reflection polygon" );

                for ( i = 0; i < cnt; i++ )
                {
                    ii = cnt - 1 - i;
                    point_transform( rpoly->pts[i], poly->pts[ii],
                                     &plane->pt_transf );
                    point_transform( rpoly->norm[i], poly->norm[ii],
                                     &plane->norm_transf );
                    hvec_copy( rpoly->cols[i], poly->cols[ii] );
                    rpoly->vals[i] = poly->vals[ii];
                }

                if ( analy->render_mode == RENDER_HIDDEN )
                    draw_edged_poly( cnt, rpoly->pts, rpoly->norm,
                                     rpoly->cols, rpoly->vals, matl, analy );
                else
                    draw_plain_poly( cnt, rpoly->pts, rpoly->norm,
                                     rpoly->cols, rpoly->vals, matl, analy );

                INSERT( rpoly, reflpoly );
            }

            if ( analy->refl_orig_only )
                free( reflpoly );
            else
                APPEND( reflpoly, origpoly );
            reflpoly = NULL;
        }

        DELETE_LIST( origpoly );
    }
}


/************************************************************
 * TAG( draw_edged_poly )
 *
 * Draw a polygon with borders, for hidden line drawings.
 * The routine expects either a quadrilateral or a triangle.
 *
 * NOTE: stenciling must be set up by the calling routine.
 */
static void
draw_edged_poly( cnt, pts, norm, cols, vals, matl, analy )
int cnt;
float pts[4][3];
float norm[4][3];
float cols[4][4];
float vals[4];
int matl;
Analysis *analy;
{
    int i;

    /* Outline the polygon and write into the stencil buffer. */
    glBegin( GL_LINE_LOOP );
    for ( i = 0; i < cnt; i++ )
        glVertex3fv( pts[i] );
    glEnd();

    /* Fill the polygon. */
    /*
    glEnable( GL_LIGHTING );
    */
    glStencilFunc( GL_EQUAL, 0, 1 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
    
    if ( analy->interp_mode == GOOD_INTERP )
    {
        /* Scan convert the polygon by hand. */
        scan_poly( cnt, pts, norm, vals, matl, analy );
    }
    else
    {
        glBegin( GL_POLYGON );
        for ( i = 0; i < cnt; i++ )
        {
            glColor3fv( cols[i] );
            glNormal3fv( norm[i] );
            glVertex3fv( pts[i] );
        }
        glEnd();
    }

    /* Clear the stencil buffer. */
    /*
    glDisable( GL_LIGHTING );
    */
    glStencilFunc( GL_ALWAYS, 0, 1 );
    glStencilOp( GL_INVERT, GL_INVERT, GL_INVERT );
    glColor3fv( v_win->foregrnd_color  );
    glBegin( GL_LINE_LOOP );
    for ( i = 0; i < cnt; i++ )
        glVertex3fv( pts[i] );
    glEnd();
}


/************************************************************
 * TAG( draw_plain_poly )
 *
 * Draw a polygon with no borders.  The routine expects
 * either a quadrilateral or a triangle.
 */
static void
draw_plain_poly( cnt, pts, norm, cols, vals, matl, analy )
int cnt;
float pts[4][3];
float norm[4][3];
float cols[4][4];
float vals[4];
int matl;
Analysis *analy;
{
    int i;

    if ( analy->interp_mode == GOOD_INTERP )
    {
        /* Scan convert the polygon by hand. */
        scan_poly( cnt, pts, norm, vals, matl, analy );
    }
    else
    {
        glBegin( GL_POLYGON );
        for ( i = 0; i < cnt; i++ )
        {
            glColor3fv( cols[i] );
            glNormal3fv( norm[i] );
            glVertex3fv( pts[i] );
        }
        glEnd();
    }
}


/************************************************************
 * TAG( length_2d )
 *
 * Return the length between two 2D points.  Helper function
 * for scan_poly.
 */
static float
length_2d( pt1, pt2 )
float pt1[2];
float pt2[2];
{
    float dx, dy;

    dx = pt2[0] - pt1[0];
    dy = pt2[1] - pt1[1];
    return sqrt( (double)( dx*dx + dy*dy ) );
}


/************************************************************
 * TAG( scan_poly )
 *
 * Draw a polygon by subdividing it into pixel-sized fragments.
 * This routine allows us to implement correct (but slow) linear
 * interpolation in the polygon interior.  Handles 3D and 2D
 * polygons.
 */
static void
scan_poly( cnt, pts, norm, vals, matl, analy )
int cnt;
float pts[4][3];
float norm[4][3];
float vals[4];
int matl;
Analysis *analy;
{
    Bool_type colorflag;
    float pt[3], proj_pts[4][2];
    float d1, d2;
    float r, s, h[4], val;
    float *dpts, *dcol, *dnorm;
    float rmin, rmax, threshold;
    int dist, ni, nj;
    int i, ii, j, k;

    /*
     * For triangular faces, duplicate the last vertex value
     * and then treat as a quad.  Is interpolation correct in
     * this case?
     */
    if ( cnt == 3 )
    {
        VEC_COPY( pts[3], pts[2] );
        VEC_COPY( norm[3], norm[2] );
        vals[3] = vals[2];
    }

    if ( vals[0] == vals[1] && vals[0] == vals[2] && vals[0] == vals[3] )
    {
        /*
         * If vertex values are all the same, no need to subdivide polygon.
         */
        ni = 2;
        nj = 2;
    }
    else
    {
        /* Transform the vertices and project to the screen. */
        for ( i = 0; i < 4; i++ )
        {
            /* Run the vertex through the view matrix. */
            point_transform( pt, pts[i], &cur_view_mat );

            /*
             * See glFrustrum() and glOrtho() in the OpenGL Reference Manual
             * to see where these factors come from.
             */
            if ( v_win->orthographic )
            {
                proj_pts[i][0] = proj_param_x * pt[0];
                proj_pts[i][1] = proj_param_y * pt[1];
            }
            else
            {
                proj_pts[i][0] = proj_param_x * pt[0] / -pt[2];
                proj_pts[i][1] = proj_param_y * pt[1] / -pt[2];
            }
        }

        /* Estimate the number of subdivisions needed in each direction. */
        d1 = length_2d( proj_pts[0], proj_pts[1] );
        d2 = length_2d( proj_pts[2], proj_pts[3] );
	/*
	 * Force subdivision by incrementing distances.  This is really only
	 * important for polygons with dimensions of approx. 2 pixels, but
	 * will have little computational impact for larger polygons where
	 * it's unnecessary.
	 */
        /* dist = MAX( d1, d2 ); */
        dist = MAX( d1 + 1.5, d2 + 1.5 );
        ni = MAX( dist, 2 );
        d1 = length_2d( proj_pts[1], proj_pts[2] );
        d2 = length_2d( proj_pts[0], proj_pts[3] );
        dist = MAX( d1 + 1.5, d2 + 1.5 );
        nj = MAX( dist, 2 );
    }

    rmin = analy->result_mm[0];
    rmax = analy->result_mm[1];
    threshold = analy->zero_result;
    colorflag = analy->result_id != VAL_NONE &&
                !analy->disable_material[matl];

    /* Dice the polygon and render the fragments. */
    dpts = NEW_N( float, ni*nj*3, "Tmp scan poly" );
    dcol = NEW_N( float, ni*nj*4, "Tmp scan poly" );
    dnorm = NEW_N( float, ni*nj*3, "Tmp scan poly" );

    for ( i = 0; i < ni; i++ )
        for ( j = 0; j < nj; j++ )
        {
            r = 2.0 * i / (ni - 1.0) - 1.0;
            s = 2.0 * j / (nj - 1.0) - 1.0;
            shape_fns_2d( r, s, h );

            for ( k = 0; k < 3; k++ )
                dpts[i*nj*3 + j*3 + k] = h[0]*pts[0][k] + h[1]*pts[1][k] +
                                         h[2]*pts[2][k] + h[3]*pts[3][k];
            for ( k = 0; k < 3; k++ )
                dnorm[i*nj*3 + j*3 + k] = h[0]*norm[0][k] + h[1]*norm[1][k] +
                                          h[2]*norm[2][k] + h[3]*norm[3][k];

            val = h[0]*vals[0] + h[1]*vals[1] + h[2]*vals[2] + h[3]*vals[3];

            color_lookup( &dcol[i*nj*4 + j*4], val, rmin, rmax,
                          threshold, matl, colorflag );
        }

    if ( analy->dimension == 3 )
    {
        for ( i = 0, ii = 1; i < ni - 1; i++, ii++ )
        {
            glBegin( GL_QUAD_STRIP );
            for ( j = 0; j < nj; j++ )
            {
                glColor3fv( &dcol[i*nj*4 + j*4] );
                glNormal3fv( &dnorm[i*nj*3 + j*3] );
                glVertex3fv( &dpts[i*nj*3 + j*3] );
                glColor3fv( &dcol[ii*nj*4 + j*4] );
                glNormal3fv( &dnorm[ii*nj*3 + j*3] );
                glVertex3fv( &dpts[ii*nj*3 + j*3] );
            }
            glEnd();
        }
    }
    else
    {
        /* We make this a separate loop only for efficiency. */
        for ( i = 0, ii = 1; i < ni - 1; i++, ii++ )
        {
            glBegin( GL_QUAD_STRIP );
            for ( j = 0; j < nj; j++ )
            {
                glColor3fv( &dcol[i*nj*4 + j*4] );
                glVertex2fv( &dpts[i*nj*3 + j*3] );
                glColor3fv( &dcol[ii*nj*4 + j*4] );
                glVertex2fv( &dpts[ii*nj*3 + j*3] );
            }
            glEnd();
        }
    }

    free( dpts );
    free( dcol );
    free( dnorm );
}


/************************************************************
 * TAG( draw_poly_2d )
 *
 * Draw a 2D polygon, reflecting it as needed.  The routine expects
 * either a quadrilateral or a triangle.  The matl flag is the
 * material of the polygon.  A value of -1 indicates that no
 * material is associated with the polygon.
 */
static void
draw_poly_2d( cnt, pts, cols, vals, matl, analy )
int cnt;
float pts[4][3];
float cols[4][4];
float vals[4];
int matl;
Analysis *analy;
{
    Bool_type good_interp;
    Refl_plane_obj *plane;
    Render_poly_obj *poly, *rpoly, *origpoly, *reflpoly;
    float rpts[4][3];
    float rcols[4][4];
    float rvals[4];
    float norm[4][3];
    int i, ii, j;

    good_interp = ( analy->interp_mode == GOOD_INTERP && matl >= 0 );

    if ( analy->translate_material )
    {
        if ( matl >= 0 )
            for ( i = 0; i < cnt; i++ )
                for ( j = 0; j < 2; j++ )
                    pts[i][j] += analy->mtl_trans[j][matl];
    }

    /* Draw the initial polygon. */
    if ( good_interp )
    {
        /* This is just a dummy argument in this case. */
        for ( i = 0; i < cnt; i++ )
        {
            VEC_SET( norm[i], 0.0, 0.0, 1.0 );
        }
        scan_poly( cnt, pts, norm, vals, matl, analy );
    }
    else
    {
        glBegin( GL_POLYGON );
        for ( i = 0; i < cnt; i++ )
        {
            glColor3fv( cols[i] );
            glVertex2fv( pts[i] );
        }
        glEnd();
    }

    if ( !analy->reflect || analy->refl_planes == NULL )
        return;

    if ( analy->refl_planes->next == NULL )
    {
        /* Just one reflection plane; blast it out.
         * The polygon's node order is reversed by reflecting
         * the individual points, so we have to correct the order.
         */
        plane = analy->refl_planes;
        for ( i = 0; i < cnt; i++ )
        {
            ii = cnt - 1 - i;
            point_transform( rpts[i], pts[ii], &plane->pt_transf );
            hvec_copy( rcols[i], cols[ii] );
            rvals[i] = vals[ii];
        }

        if ( good_interp )
            scan_poly( cnt, rpts, norm, rvals, matl, analy );
        else
        {
            glBegin( GL_POLYGON );
            for ( i = 0; i < cnt; i++ )
            {
                glColor3fv( rcols[i] );
                glVertex2fv( rpts[i] );
            }
            glEnd();
        }
    }
    else
    {
        /* More than one reflection plane, do it the hard way. */

        origpoly = NEW( Render_poly_obj, "Reflection polygon" );
        origpoly->cnt = cnt;
        for ( i = 0; i < cnt; i++ )
        {
            VEC_COPY( origpoly->pts[i], pts[i] );
            hvec_copy( origpoly->cols[i], cols[i] );
            origpoly->vals[i] = vals[i];
        }

        reflpoly = NULL;

        for ( plane = analy->refl_planes;
              plane != NULL;
              plane = plane->next )
        {
            for ( poly = origpoly; poly != NULL; poly = poly->next )
            {
                rpoly = NEW( Render_poly_obj, "Reflection polygon" );

                for ( i = 0; i < cnt; i++ )
                {
                    ii = cnt - 1 - i;
                    point_transform( rpoly->pts[i], poly->pts[ii],
                                     &plane->pt_transf );
                    hvec_copy( rpoly->cols[i], poly->cols[ii] );
                    rpoly->vals[i] = poly->vals[ii];
                }

                if ( good_interp )
                    scan_poly( cnt, rpoly->pts, norm, rpoly->vals,
                               matl, analy );
                else
                {
                    glBegin( GL_POLYGON );
                    for ( i = 0; i < cnt; i++ )
                    {
                        glColor3fv( rpoly->cols[i] );
                        glVertex2fv( rpoly->pts[i] );
                    }
                    glEnd();
                }

                INSERT( rpoly, reflpoly );
            }

            if ( analy->refl_orig_only )
                free( reflpoly );
            else
                APPEND( reflpoly, origpoly );
            reflpoly = NULL;
        }

        DELETE_LIST( origpoly );
    }
}


/************************************************************
 * TAG( draw_line )
 *
 * Draw a polyline, reflecting it as needed.  The "close" flag
 * should be TRUE for a closed polyline and FALSE for an open
 * polyline.  The matl flag is the material of the line.
 * A value of -1 indicates that no material is associated with
 * the line.
 */
static void
draw_line( cnt, pts, matl, analy, close )
int cnt;
float *pts;
int matl;
Analysis *analy;
Bool_type close;
{
    GLenum line_mode;
    Refl_plane_obj *plane;
    float rpt[3];
    float *origpts[128], *reflpts[128];
    int origcnt, reflcnt;
    int i, j;

    /* Handles up to 6 cumulative reflection planes. */

    if ( analy->translate_material )
    {
        if ( matl >= 0 )
            for ( i = 0; i < cnt; i++ )
                for ( j = 0; j < 3; j++ )
                    pts[i*3+j] += analy->mtl_trans[j][matl];
    }

    if ( close )
        line_mode = GL_LINE_LOOP;
    else
        line_mode = GL_LINE_STRIP;

    /* Draw the original line. */
    glBegin( line_mode );
    for ( i = 0; i < cnt; i++ )
        glVertex3fv( &pts[i*3] );
    glEnd();

    if ( !analy->reflect || analy->refl_planes == NULL )
        return;

    if ( analy->refl_planes->next == NULL )
    {
        /* Just one reflection plane; blast it out. */
        plane = analy->refl_planes;
        glBegin( line_mode );
        for ( i = 0; i < cnt; i++ )
        {
            point_transform( rpt, &pts[i*3], &plane->pt_transf );
            glVertex3fv( rpt );
        }
        glEnd();
    }
    else if ( analy->refl_orig_only )
    {
        for ( plane = analy->refl_planes;
              plane != NULL;
              plane = plane->next )
        {
            glBegin( line_mode );
            for ( i = 0; i < cnt; i++ )
            {
                point_transform( rpt, &pts[i*3], &plane->pt_transf );
                glVertex3fv( rpt );
            }
            glEnd();
        }
    }
    else
    {
        origpts[0] = pts;
        origcnt = 1;
        reflcnt = 0;

        for ( plane = analy->refl_planes;
              plane != NULL;
              plane = plane->next )
        {
            for ( j = 0; j < origcnt; j++ )
            {
                reflpts[reflcnt] = NEW_N( float, cnt*3, "Reflect line" );
                glBegin( line_mode );
                for ( i = 0; i < cnt; i++ )
                {
                    point_transform( &reflpts[reflcnt][i*3], &origpts[j][i*3],
                                     &plane->pt_transf );
                    glVertex3fv( &reflpts[reflcnt][i*3] );
                }
                glEnd();
                reflcnt++;
            }

            for ( j = 0; j < reflcnt; j++ )
            {
                if ( origcnt >= 128 )
                    popup_fatal( "Too many reflection planes!\n" );

                origpts[origcnt] = reflpts[j];
                origcnt++;
            }
            reflcnt = 0;
        }

        for ( j = 1; j < origcnt; j++ )
            free( origpts[j] );
    }
}


/************************************************************
 * TAG( draw_3d_text )
 *
 * Draw 3D text at the position specified by pt.  This routine
 * orients the text toward the viewer and draws it with the
 * proper scaling and rotation.
 */
static void
draw_3d_text( pt, text )
float pt[3];
char *text;
{
    Transf_mat mat;
    float arr[16], tpt[3], spt[3];
    float zpos, cx, cy, text_height;
    int i, j;

    /* Run the point through the model transform matrix. */
    glGetFloatv( GL_MODELVIEW_MATRIX, arr );
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 4; j++ )
            mat.mat[i][j] = arr[i*4 + j];
    point_transform( tpt, pt, &mat );

    /* Get drawing window and position. */
    get_foreground_window( &zpos, &cx, &cy );

    /* Project the point to the drawing window. */
    if ( v_win->orthographic )
    {
        spt[0] = tpt[0];
        spt[1] = tpt[1];
        spt[2] = zpos;
    }
    else
    {
        spt[0] = tpt[0] * zpos / tpt[2];
        spt[1] = tpt[1] * zpos / tpt[2];
        spt[2] = zpos;
    }

    /* Get the text size. */
    text_height = 14.0 * 2.0*cy / v_win->vp_height;

    /* Clear the model view matrix and draw the text. */
    glPushMatrix();
    glLoadIdentity();

    hmove( spt[0], spt[1], spt[2] );
    htextsize( text_height, text_height );
    antialias_lines( TRUE, TRUE );
    glLineWidth( 1.25 );
    hcharstr( text );
    glLineWidth( 1.0 );
    antialias_lines( FALSE, 0 );

    /* Restore the model view matrix. */
    glPopMatrix();
}


/************************************************************
 * TAG( get_foreground_window )
 *
 * Calculate a Z position for the foreground that won't get
 * clipped by the near/far planes.  Also returns the view
 * coordinates of the upper right corner of the window at
 * the Z position.
 */
void
get_foreground_window( zpos, cx, cy )
float *zpos;
float *cx;
float *cy;
{
    float aspect, cp;

    /* Get a Z position that won't get clipped by near/far planes.
     * Translate all the text to that position.  Have to scale in
     * X and Y if perspective viewing is used.
     */
    *zpos = - (v_win->far + v_win->near)/2.0;

    aspect = v_win->aspect_correct * v_win->vp_width / (float)v_win->vp_height;

    /* Get the viewport dimensions at the drawing (zpos) plane. */
    if ( v_win->orthographic )
        cp = 1.0;
    else
        cp = - *zpos * TAN( DEG_TO_RAD( 0.5*v_win->cam_angle ) );

    if ( aspect >= 1.0 )
    {
        *cx = cp * aspect;
        *cy = cp;
    }
    else
    {
        *cx = cp;
        *cy = cp / aspect;
    }
}


/************************************************************
 * TAG( draw_foreground )
 *
 * Draw the foreground stuff in the mesh window (colormap,
 * information strings, world coordinate system, etc.)
 * stuff in the mesh window (colormap, information strings, etc.)
 */
static void
draw_foreground( analy )
Analysis *analy;
{
    Transf_mat mat, tmat;
    float (*ttmat)[3];
    float pt[3], pto[3], pti[3], ptj[3], ptk[3], ptv[3];
    float world_to_vp[2], vp_to_world[2];
    float zpos, cx, cy;
    float xpos, ypos, xp, yp, xsize, ysize;
    float text_height, b_width, b_height;
    float low, high;
    float leng, sub_leng;
    float arr[16];
    Bool_type extend_colormap;
    Bool_type show_dirvec;
    char str[90];
    char *vec_x, *vec_y, *vec_z;
    int nstripes, i, frst, lst;
    static char *el_label[] = { "node", "beam", "shell", "brick" };
    float *el_mm;
    int *el_type, *el_id;
    int mm_node_types[2];
    int mod_cnt;
    Result_modifier_type mods[QTY_RESULT_MODIFIER_TYPES];
    int fracsz;
    float scale_y, value;
    float scale_minimum, scale_maximum, distance;
    int qty_of_intervals, scale_error_status;
    float rmin_offset, rmax_offset;
    int ntrips;
    float low_text_bound, high_text_bound;
    float comparison_tolerance;
    float scl_max;
    float map_text_offset;
    
    /* Cap for colorscale interpolation. */
    scl_max = SCL_MAX;

    /* Foreground overwrites everything else in the scene. */
    glDepthFunc( GL_ALWAYS );

    /* Get drawing window and position. */
    get_foreground_window( &zpos, &cx, &cy );

    world_to_vp[0] = v_win->vp_width / (2.0*cx);
    world_to_vp[1] = v_win->vp_height / (2.0*cy);
    vp_to_world[0] = 2.0*cx / v_win->vp_width;
    vp_to_world[1] = 2.0*cy / v_win->vp_height;

    /* Translate everything to the drawing plane. */
    glPushMatrix();
    glTranslatef( 0.0, 0.0, zpos );

    /* For the textual information. */
    text_height = 14.0 * vp_to_world[1];
    htextsize( text_height, text_height );
    fracsz = analy->float_frac_size;
    
    if ( analy->result_id == VAL_PROJECTED_VEC )
    {
        show_dirvec = TRUE;
        map_text_offset = text_height * LINE_SPACING_COEFF;
    }
    else
    {
        show_dirvec = FALSE;
        map_text_offset = 0.0;
    }

    /* Colormap. */
    if ( analy->show_colormap && analy->result_id != VAL_NONE )
    {
        /* Extend the cutoff colors if cutoff is being used. */
        if ( analy->mm_result_set[0] || analy->mm_result_set[1] )
            extend_colormap = TRUE;
        else
            extend_colormap = FALSE;
        /* Width of black border. */
        b_width = 3.5 * vp_to_world[0];
        b_height = 3.5 * vp_to_world[1];

        if ( analy->use_colormap_pos )
        {
            xpos = cx * analy->colormap_pos[0];
            ypos = cy * analy->colormap_pos[1];
            xsize = cx * analy->colormap_pos[2];
            ysize = cy * analy->colormap_pos[3];
        }
        else
        {
            xpos = cx - vp_to_world[0]*40 - b_width;
/* "255" is somewhat arbitrary and should become a parameter of the
   colorscale size.
            ypos = cy - vp_to_world[1]*255 - b_height; 
*/
            ypos = cy - vp_to_world[1]*240 - b_height - map_text_offset;
            xsize = vp_to_world[0]*25;
            ysize = vp_to_world[1]*200;
        }

        /* Put a black border on the colormap. */
        glColor3fv( black );
        glRectf( xpos - b_width, ypos - b_height,
                 xpos + xsize + b_width, ypos + ysize + b_height );

        /* Draw the colormap. */
        nstripes = (int)(ysize*world_to_vp[1] + 0.5);
        yp = ypos;
        if ( extend_colormap )
        {
            /*
             * When using cutoff, may want to extend the cutoff
             * colors a little.
             */
            if ( analy->mm_result_set[0] )
                frst = nstripes/20;
            else
                frst = 0;
            if ( analy->mm_result_set[1] )
                lst = nstripes-nstripes/20;
            else
                lst = nstripes;
            for ( i = 0; i < frst; i++ )
            {
                glColor3fv( v_win->colormap[0] );
                glRectf( xpos, yp, xpos + xsize, yp + vp_to_world[1] );
                yp += vp_to_world[1];
            }
            for ( i = frst; i < lst; i++ )
            {
                glColor3fv( v_win->colormap[ (int)((i-frst)*scl_max /
                                             (float)(lst-frst-1)) + 1 ] );
                glRectf( xpos, yp, xpos + xsize, yp + vp_to_world[1] );
                yp += vp_to_world[1];
            }
            for ( i = lst; i < nstripes; i++ )
            {
                glColor3fv( v_win->colormap[CMAP_SIZE - 1] );
                glRectf( xpos, yp, xpos + xsize, yp + vp_to_world[1] );
                yp += vp_to_world[1];
            }
        }
        else
        {
	    scl_max = (float) CMAP_SIZE - 0.01;
	    
            for ( i = 0; i < nstripes; i++ )
            {
                glColor3fv( v_win->colormap[ (int)(i * scl_max /
                                                   (float)(nstripes - 1)) ] );
                glRectf( xpos, yp, xpos + xsize, yp + vp_to_world[1] );
                yp += vp_to_world[1];
            }
        }

        glColor3fv( v_win->foregrnd_color );

        /* Draw the writing (scale) next to the colormap. */
        if ( analy->show_colorscale )
        {
            antialias_lines( TRUE, TRUE );
            glLineWidth( 1.25 );

            get_min_max( analy, is_beam_result( analy->result_id ), 
	                 &low, &high );

            if ( analy->perform_unit_conversion )
	    {
                low = (analy->conversion_scale * low) 
		      + analy->conversion_offset;
                high = (analy->conversion_scale * high) 
		       + analy->conversion_offset;
	    }
	    
            hrightjustify( TRUE );


            /* Result title. */
/* Temporary adjustment.  Should become an explicit function of colorscale
   position.
            hmove2( xpos + xsize, ypos+ysize+b_height+2.0*text_height ); 
*/
            hmove2( xpos + xsize, 
                    ypos + ysize + b_height + 1.0 * text_height 
                    + map_text_offset );
            hcharstr( analy->result_title );

            if ( show_dirvec )
            {
                vec_x = trans_result[resultid_to_index[analy->vec_id[0]]][3];
                vec_y = trans_result[resultid_to_index[analy->vec_id[1]]][3];
                vec_z = trans_result[resultid_to_index[analy->vec_id[2]]][3];

                sprintf( str, "(%s, %s, %s)", vec_x, vec_y, vec_z );

                hmove2( xpos + xsize, 
                        ypos + ysize + b_height + 1.0 * text_height );
                hcharstr( str );
            }

            /* Account for min/max result thresholds. */
            if ( TRUE == analy->mm_result_set[0] )
                rmin_offset = 0.05 * ysize;
            else
                rmin_offset = 0.0;

            if ( TRUE == analy->mm_result_set[1] )
                rmax_offset = 0.05 * ysize;
            else
                rmax_offset = 0.0;

            /* Always label the low value. */
            xp = xpos - (2.0 * b_width) - (0.6 * text_height);
            yp = ypos + rmin_offset;

            hmove2( xp, yp - (0.6 * text_height) );
            sprintf( str, "%.*e", fracsz, low );
            hcharstr( str );

            xp = xpos - (2.0 * b_width);

            glBegin( GL_TRIANGLES );
            glVertex2f( xp, yp );
            glVertex2f( xp - (0.4 * text_height), yp + (0.2 * text_height) );
            glVertex2f( xp - (0.4 * text_height), yp - (0.2 * text_height) );
            glEnd();

            /* Conditionally render the high value and scale. */
            if ( low != high )
	    {
                scale_y = ((ypos + ysize - rmax_offset) - (ypos + rmin_offset)) 
		          / (high - low);

                /* Compute legend scale intervals */
                qty_of_intervals = 5;
                linear_variable_scale( low, high, qty_of_intervals, 
	                           &scale_minimum, &scale_maximum, &distance,  
                                   &scale_error_status );

                low_text_bound = yp - (0.6 * text_height) + text_height;

                /* Label the high value. */
                xp = xpos - (2.0 * b_width) - (0.6 * text_height);
                yp = ypos + rmin_offset + (scale_y * (high - low));

                high_text_bound = yp - (0.6 * text_height);

                hmove2( xp, yp - (0.6 * text_height) );
                sprintf( str, "%.*e", fracsz, high );
                hcharstr( str );

                xp = xpos - (2.0 * b_width);

                glBegin( GL_TRIANGLES );
                glVertex2f( xp, yp );
                glVertex2f( xp - (0.4 * text_height), yp + (0.2 * text_height) );
                glVertex2f( xp - (0.4 * text_height), yp - (0.2 * text_height) );
                glEnd();

                /* If scaling succeeded, render the scale. */
                if ( FALSE == scale_error_status )
                {
                    /* Label scaled values at computed intervals */

                    comparison_tolerance = machine_tolerance();
                    ntrips = MAX( round( (double)((scale_maximum - scale_minimum + distance) / distance),
                                     comparison_tolerance ), 0.0 );

                    for ( i = 0; i < ntrips; i++ )
                    {
                        value = scale_minimum + (i * distance);

                        /* 
			 * NOTE:  scaled values MAY exceed bounds of tightly 
			 * restricted legend limits. 
			 */

                        yp = ypos + rmin_offset + (scale_y * (value - low));

                        if ( (low < value)
                             && (value < high)
                             && (low_text_bound <= yp - (0.6 * text_height))
                             && (yp - (0.6 * text_height)
			         + text_height <= high_text_bound) )
                        {
                            xp = xpos - (2.0 * b_width) - (0.6 * text_height);

                            hmove2( xp, yp - (0.6 * text_height) );
                            sprintf( str, "%.*e", fracsz, value );
                            hcharstr( str );

                            xp = xpos - (2.0 * b_width);

                            glBegin( GL_TRIANGLES );
                            glVertex2f( xp, yp );
                            glVertex2f( xp - (0.4 * text_height), 
			                yp + (0.2 * text_height) );
                            glVertex2f( xp - (0.4 * text_height), 
			                yp - (0.2 * text_height) );
                            glEnd();

                            low_text_bound = yp - (0.6 * text_height) + text_height;
                        }
                    }
		}
	    }

            /* Set up for left side text. */
            hleftjustify( TRUE );
            xp = -cx + text_height;
            yp = cy - (LINE_SPACING_COEFF + TOP_OFFSET) * text_height;
	
            /* Allow for presence of minmax. */
            if ( analy->show_minmax )
                yp -= 2.0 * LINE_SPACING_COEFF * text_height;
	
            /* 
	     * Strain type, reference surface, and reference frame, 
	     * if applicable. 
	     */
            mod_cnt = get_result_modifiers( analy, mods );
            if ( mod_cnt > 0 )
            {
                for ( i = 0; i < mod_cnt; i++ )
                {
                    switch( mods[i] )
                    {
                        case STRAIN_TYPE:
                            hmove2( xp, yp );
                            sprintf( str, "Strain type: %s", 
		                     strain_label[analy->strain_variety] );
                            hcharstr( str );
                            yp -= LINE_SPACING_COEFF * text_height;
                            break;
		    
                        case REFERENCE_SURFACE:
                            hmove2( xp, yp );
                            sprintf( str, "Surface: %s", 
		                     ref_surf_label[analy->ref_surf] );
                            hcharstr( str );
                            yp -= LINE_SPACING_COEFF * text_height;
                            break;
		    
                        case REFERENCE_FRAME:
                            hmove2( xp, yp );
                            sprintf( str, "Ref frame: %s", 
		                     ref_frame_label[analy->ref_frame] );
                            hcharstr( str );
                            yp -= LINE_SPACING_COEFF * text_height;
                            break;
		    
                        case TIME_DERIVATIVE:
                            hmove2( xp, yp );
                            sprintf( str, "Result type: Time derivative" );
                            hcharstr( str );
                            yp -= LINE_SPACING_COEFF * text_height;
                            break;
                    }
                }
            }
	    
	    /* Result scale and offset, if applicable. */
	    if ( analy->perform_unit_conversion )
	    {
		hmove2( xp, yp );
		sprintf( str, "Scale/offset: %.2f/%.2f", 
		         analy->conversion_scale, analy->conversion_offset );
		hcharstr( str );
		yp -= LINE_SPACING_COEFF * text_height;
	    }
	    
            antialias_lines( FALSE, 0 );
            glLineWidth( 1.0 );
        }
    }
    else if ( analy->show_colormap )
    {
        antialias_lines( TRUE, TRUE );
        glLineWidth( 1.25 );
	
        /* Draw the material colors and label them. */
        if ( analy->use_colormap_pos )
        {
            xpos = analy->colormap_pos[0];
            ypos = analy->colormap_pos[1];
            xsize = analy->colormap_pos[2];
            ysize = analy->colormap_pos[3];
        }
        else
        {
            xpos = cx - vp_to_world[0]*40;
            ypos = cy - vp_to_world[1]*250;
            xsize = vp_to_world[0]*25;
            ysize = vp_to_world[1]*200;
        }
        xp = xpos + xsize - text_height;
        yp = ypos + ysize;

        hrightjustify( TRUE );

        for ( i = 0; i < analy->num_materials; i++ )
        {
            glColor3fv( v_win->matl_diffuse[ i%MAX_MATERIALS ] );
            glRectf( xp, yp, xp + text_height, yp + text_height );

            glColor3fv( v_win->foregrnd_color );
            hmove2( xp - 0.5*text_height, yp );
            sprintf( str, "%d", i+1 );
            hcharstr( str );

            yp -= 1.5*text_height;
        }
        hmove2( xp + text_height, ypos + ysize + 1.5*text_height );
        hcharstr( "Materials" );

        antialias_lines( FALSE, 0 );
        glLineWidth( 1.0 );
    }
    
    antialias_lines( TRUE, TRUE );
    glLineWidth( 1.25 );

    /* File title. */
    if ( analy->show_title )
    {
        glColor3fv( v_win->foregrnd_color );
        hcentertext( TRUE );
        hmove2( 0.0, -cy + 2.5 * text_height );
        hcharstr( analy->title );
        hcentertext( FALSE );
    }

    /* Time. */
    if ( analy->show_time )
    {
        glColor3fv( v_win->foregrnd_color );
        hcentertext( TRUE );
        hmove2( 0.0, -cy + 1.0 * text_height );
        sprintf( str, "t = %.5e", analy->state_p->time );
        hcharstr( str );
        hcentertext( FALSE );
    }

    /* Global coordinate system. */
    if ( analy->show_coord )
    {
        leng = 35*vp_to_world[0];
        sub_leng = leng / 10.0;

        /* Rotate the coord system properly, then translate it down
         * to the lower right corner of the view window.
         */
        look_rot_mat( v_win->look_from, v_win->look_at, v_win->look_up, &mat );
        mat_mul( &tmat, &v_win->rot_mat, &mat );
        mat_translate( &mat, cx - 60*vp_to_world[0],
                       -cy + 60*vp_to_world[0], 0.0 );
        mat_mul( &tmat, &tmat, &mat );

        /* Draw the axes. */
        VEC_SET( pt, 0.0, 0.0, 0.0 );
        point_transform( pto, pt, &tmat );
        VEC_SET( pt, leng, 0.0, 0.0 );
        point_transform( pti, pt, &tmat );
        VEC_SET( pt, 0.0, leng, 0.0 );
        point_transform( ptj, pt, &tmat );
        VEC_SET( pt, 0.0, 0.0, leng );
        point_transform( ptk, pt, &tmat );

        glColor3fv( v_win->foregrnd_color );

        glBegin( GL_LINES );
        glVertex3fv( pto );
        glVertex3fv( pti );
        glVertex3fv( pto );
        glVertex3fv( ptj );
        glVertex3fv( pto );
        glVertex3fv( ptk );
        glEnd();

        if ( show_dirvec )
        {
            VEC_SET( pt, analy->dir_vec[0] * leng * 0.75, 
                     analy->dir_vec[1] * leng * 0.75, 
                     analy->dir_vec[2] * leng * 0.75 );
            point_transform( ptv, pt, &tmat );
            
            glColor3fv( material_colors[15] ); /* Red */
            glLineWidth( 2.25 );

            glBegin( GL_LINES );
            glVertex3fv( pto );
            glVertex3fv( ptv );
            glEnd();
        
            glLineWidth( 1.25 );
            glColor3fv( v_win->foregrnd_color );
        }

        /* Label the axes. */
        VEC_SET( pt, leng + sub_leng, sub_leng, sub_leng );
        point_transform( pti, pt, &tmat );
        VEC_SET( pt, sub_leng, leng + sub_leng, sub_leng );
        point_transform( ptj, pt, &tmat );
        VEC_SET( pt, sub_leng, sub_leng, leng + sub_leng );
        point_transform( ptk, pt, &tmat );

        antialias_lines( FALSE, 0 );
	glLineWidth( 1.0 );
	
        draw_3d_text( pti, "X" );
        draw_3d_text( ptj, "Y" );
        draw_3d_text( ptk, "Z" );

        antialias_lines( TRUE, TRUE );
	glLineWidth( 1.25 );
        
        /* Show tensor transformation coordinate system if on. */
        if ( analy->do_tensor_transform 
             && analy->tensor_transform_matrix != NULL )
        {
            ttmat = analy->tensor_transform_matrix;

            /* Draw the axis lines. */
            VEC_SET( pt, 0.0, 0.0, 0.0 );
            point_transform( pto, pt, &tmat );
            VEC_SET( pt, ttmat[0][0] * leng, ttmat[0][1] * leng, 
                     ttmat[0][2] * leng );
            point_transform( pti, pt, &tmat );
            VEC_SET( pt, ttmat[1][0] * leng, ttmat[1][1] * leng, 
                     ttmat[1][2] * leng );
            point_transform( ptj, pt, &tmat );
            VEC_SET( pt, ttmat[2][0] * leng, ttmat[2][1] * leng, 
                     ttmat[2][2] * leng );
            point_transform( ptk, pt, &tmat );
/**/
            /* Hard-code brown/red for now. */
            glColor3f( 0.6, 0.2, 0.0 );

            glBegin( GL_LINES );
            glVertex3fv( pto );
            glVertex3fv( pti );
            glVertex3fv( pto );
            glVertex3fv( ptj );
            glVertex3fv( pto );
            glVertex3fv( ptk );
            glEnd();
            
            /* Draw the axis labels. */
            VEC_SET( pt, ttmat[0][0] * leng + sub_leng, 
                     ttmat[0][1] * leng - sub_leng, 
                     ttmat[0][2] * leng - sub_leng );
            point_transform( pti, pt, &tmat );
            VEC_SET( pt, ttmat[1][0] * leng - sub_leng, 
                     ttmat[1][1] * leng + sub_leng, 
                     ttmat[1][2] * leng - sub_leng );
            point_transform( ptj, pt, &tmat );
            VEC_SET( pt, ttmat[2][0] * leng - sub_leng, 
                     ttmat[2][1] * leng - sub_leng, 
                     ttmat[2][2] * leng + sub_leng );
            point_transform( ptk, pt, &tmat );

            antialias_lines( FALSE, 0 );
            glLineWidth( 1.0 );
            
            draw_3d_text( pti, "x" );
            draw_3d_text( ptj, "y" );
            draw_3d_text( ptk, "z" );

            antialias_lines( TRUE, TRUE );
            glLineWidth( 1.25 );
            glColor3fv( v_win->foregrnd_color );
        }
    }
    
    /* Result value min/max. */
    if ( analy->show_minmax )
    {
        if ( is_nodal_result( analy->result_id ) )
	{
	    mm_node_types[0] = mm_node_types[1] = NODE_T;
            el_mm = analy->state_mm;
	    el_type = mm_node_types;
	    el_id = analy->state_mm_nodes;
	}
	else
	{
            el_mm = analy->elem_state_mm.el_minmax;
	    el_type = analy->elem_state_mm.el_type;
	    el_id = analy->elem_state_mm.mesh_object;
	}
	
	if ( analy->perform_unit_conversion )
	{
	    low = (analy->conversion_scale * el_mm[0]) 
		  + analy->conversion_offset;
	    high = (analy->conversion_scale * el_mm[1]) 
		   + analy->conversion_offset;
	}
	else
	{
	    low = el_mm[0];
	    high = el_mm[1];
	}
	
	xp = -cx + text_height;
	yp = cy - (LINE_SPACING_COEFF + TOP_OFFSET) * text_height;
	
	glColor3fv( v_win->foregrnd_color );
	hleftjustify( TRUE );
	hmove2( xp, yp );
	if ( analy->result_id != VAL_NONE )
	    sprintf( str, "max: %.*e, %s %d", fracsz, high, el_label[el_type[1]], 
	             el_id[1] );
	else
	    sprintf( str, "max: (no result)" );
	hcharstr( str );
	hmove2( xp, yp - LINE_SPACING_COEFF * text_height );
	if ( analy->result_id != VAL_NONE )
	    sprintf( str, "min: %.*e, %s %d", fracsz, low, 
	             el_label[el_type[0]], el_id[0] );
	else
	    sprintf( str, "min: (no result)" );
	hcharstr( str );
    }

    antialias_lines( FALSE, 0 );
    glLineWidth( 1.0 );

    /* Draw a "safe action area" box for the conversion to NTSC video.
     * The region outside the box may get lost.  This box was defined
     * using trial and error for a 486 x 720 screen size.
     */
    if ( analy->show_safe )
    {
        glColor3fv( red );
        glBegin( GL_LINE_LOOP );
        glVertex2f( -0.91*cx, -0.86*cy );
        glVertex2f( 0.91*cx, -0.86*cy );
        glVertex2f( 0.91*cx, 0.93*cy );
        glVertex2f( -0.91*cx, 0.93*cy );
        glEnd();
    }

    glPopMatrix();

    /* End of foreground overwriting. */
    glDepthFunc( GL_LEQUAL );
}


/************************************************************
 * TAG( quick_update_display )
 *
 * Fast display refresh for interactive view control.  Only the
 * edges of the mesh and the foreground are drawn.
 */
void
quick_update_display( analy )
Analysis *analy;
{
    Transf_mat look_rot;
    float arr[16], scal;
    
    /* Center the view on a point before rendering. */
    if ( analy->center_view )
        center_view( analy );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
             GL_STENCIL_BUFFER_BIT );

    glPushMatrix();

    /*
     * Set up all the viewing transforms.  Transformations are
     * specified in opposite order to the order in which they
     * are applied.
     */
    look_rot_mat( v_win->look_from, v_win->look_at, v_win->look_up, &look_rot );    mat_to_array( &look_rot, arr );
    glMultMatrixf( arr );
    glTranslatef( -v_win->look_from[0], -v_win->look_from[1],
                  -v_win->look_from[2] );
    glTranslatef( v_win->trans[0], v_win->trans[1], v_win->trans[2] );
    mat_to_array( &v_win->rot_mat, arr );
    glMultMatrixf( arr );
    scal = v_win->bbox_scale;
    glScalef( scal*v_win->scale[0], scal*v_win->scale[1],
              scal*v_win->scale[2] );
    glTranslatef( v_win->bbox_trans[0], v_win->bbox_trans[1],
                  v_win->bbox_trans[2] );

    /* Draw the grid edges. */
    if ( analy->m_edges_cnt > 0 )
        draw_edges( analy );
    else
        draw_bbox( analy->bbox );

    glPopMatrix();

    /* Draw the foreground. */
    if ( analy->show_bbox || analy->show_coord || analy->show_time ||
         analy->show_title || analy->show_safe || analy->show_colormap )
    {
        draw_foreground( analy );
    }

    gui_swap_buffers();
}


/*
 * SECTION_TAG( Image saving )
 */


/************************************************************
 * TAG( rgb_to_screen )
 *
 * Load an rgb file image into memory.
 */
void
rgb_to_screen( fname, background, analy )
char *fname;
Bool_type background;
Analysis *analy;
{
    IMAGE *img;
    int img_width, img_height, channels;
    int i, j;
    int upa;
    unsigned char *raster, *prgb_byte;
    unsigned short *rbuf, *gbuf, *bbuf, *abuf;
    unsigned short *pr_short, *pg_short, *pb_short, *pa_short;
    Bool_type alpha;
    RGB_raster_obj *p_rro;
    
    img = iopen( fname, "r" );
    if ( img == NULL )
    {
        popup_dialog( INFO_POPUP, 
                      "Unable to open rgb image file \"%s\".", fname );
        return;
    }
    
    channels = img->zsize;
    if ( channels != 3 && channels != 4 )
    {
        popup_dialog( INFO_POPUP, "File \"%s\" %s%s%s", fname, 
                      "does not contain a 3 channel (RGB) or\n",
                      "4 channel (RGBA) image.  Other image types are not",
                      " supported." );
        iclose( img );
        return;
    }
    
    alpha = ( channels == 4 );
    img_width = img->xsize;
    img_height = img->ysize;
    
    raster = NEW_N( unsigned char, img_width * img_height * channels,
                    "Image memory" );
    
    rbuf = NEW_N( unsigned short, img_width, "RGB red row buf" );
    gbuf = NEW_N( unsigned short, img_width, "RGB green row buf" );
    bbuf = NEW_N( unsigned short, img_width, "RGB blue row buf" );
    if ( alpha )
        abuf = NEW_N( unsigned short, img_width, "RGB alpha row buf" );
    
    /*
     * Multiplex r, g, b, and alpha values into the raster buffer
     * on a row-by-row basis.
     */
    prgb_byte = raster;
    for( j = 0; j < img_height; j++ )
    {
        getrow( img, rbuf, j, 0 );
        getrow( img, gbuf, j, 1 );
        getrow( img, bbuf, j, 2 );
        if ( alpha )
            getrow( img, abuf, j, 3 );
        
        pr_short = rbuf;
        pg_short = gbuf;
        pb_short = bbuf;
        pa_short = abuf;
        
        for( i = 0; i < img_width; i++ )
        {
            *prgb_byte++ = (unsigned char) *pr_short++;
            *prgb_byte++ = (unsigned char) *pg_short++;
            *prgb_byte++ = (unsigned char) *pb_short++;
            if ( alpha )
                *prgb_byte++ = (unsigned char) *pa_short++;
        }
    }
    
    iclose( img );

    free( rbuf );
    free( gbuf );
    free( bbuf );
    if ( alpha )
        free( abuf );
    
    if ( background )
    {
        /*
         * Prepare to use during background initialization, but don't
         * load the image now.
         */
        if ( analy->background_image != NULL )
        {
            p_rro = analy->background_image;
            free( p_rro->raster );
        }
        else
            p_rro = NEW( RGB_raster_obj, "Background raster" );

        p_rro->raster = raster;
        p_rro->img_width = img_width;
        p_rro->img_height = img_height;
        p_rro->alpha = alpha;
        
        analy->background_image = p_rro;
    }
    else
    {
        /* Not for a background, so load it now and release it. */
        memory_to_screen( TRUE, img_width, img_height, alpha, raster );
        free( raster );
    }
}


/************************************************************
 * TAG( memory_to_screen )
 *
 * Load the framebuffer with pixels from an in-memory image
 * raster.
 */
static void
memory_to_screen( clear, width, height, alpha, raster )
Bool_type clear;
int width;
int height;
Bool_type alpha;
unsigned char *raster;
{
    int upa;

    /* Save the current value of the unpack alignment. */
    glGetIntegerv( GL_UNPACK_ALIGNMENT, &upa );

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    
    /* glDrawBuffer( GL_BACK ); */
    
    /*
     * Clear everything to the default background color in case the
     * image has a smaller dimension than the current window.
     */
    if ( clear )
        glClear( GL_COLOR_BUFFER_BIT );
    
    /* Now copy the pixels... */
    glDrawPixels( width, height, (alpha ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, 
                  raster );
    
    if ( clear )
        gui_swap_buffers();
    
    /* Reset the unpack alignment if necessary. */
    if ( upa != 1 )
        glPixelStorei( GL_UNPACK_ALIGNMENT, upa );
}


/************************************************************
 * TAG( screen_to_memory )
 *
 * Copy the screen image to an array in memory.  The save_alpha
 * parameter specifies whether to save the alpha channel.  The
 * return array is given by the parameter ipdat, and should be of
 * size xsize*ysize*4 if the alpha is requested and xsize*ysize*3
 * otherwise.
 */
void
screen_to_memory( save_alpha, xsize, ysize, ipdat )
Bool_type save_alpha;
int xsize;
int ysize;
unsigned char *ipdat;
{
    if ( v_win->vp_width != xsize || v_win->vp_height != ysize )
    {
        popup_dialog( WARNING_POPUP,
                "Viewport width and height don't match memory dimensions." );
        return;
    }

    glReadBuffer( GL_FRONT );

    if ( save_alpha )
        glReadPixels( 0, 0, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, ipdat );
    else
        glReadPixels( 0, 0, xsize, ysize, GL_RGB, GL_UNSIGNED_BYTE, ipdat );
}


/************************************************************
 * TAG( screen_to_rgb )
 *
 * Simple snap to rle-encoded rgb/rgba file.
 */
void
screen_to_rgb( fname, alpha )
char fname[];
Bool_type alpha;
{
    unsigned char *ipdat, *prgb_byte;
    unsigned short *rbuf, *gbuf, *bbuf, *abuf;
    unsigned short *pr_short, *pg_short, *pb_short, *pa_short;
    int i, j;
    int vp_width, vp_height;
    int bufsiz;
    int components;
    IMAGE *img;
    
    components = alpha ? 4 : 3;

    vp_width = v_win->vp_width;
    vp_height = v_win->vp_height;
    
    img = iopen( fname, "w", RLE( 1 ), 3, vp_width, vp_height, components );
    if ( img == 0 )
    {
	popup_dialog( WARNING_POPUP, "Unable to open rgb image file \"%s\".\n",
	              fname );
	return;
    }
    
    bufsiz = vp_width * vp_height * components;
    ipdat = NEW_N( char, bufsiz, "Screen to rgb" );

    /* Read the pixel data into ipdat. */
    screen_to_memory( alpha, vp_width, vp_height, ipdat );

    wrt_text( "Writing data to image file %s\n", fname );
    
    rbuf = NEW_N( unsigned short, vp_width, "RGB red row buf" );
    gbuf = NEW_N( unsigned short, vp_width, "RGB green row buf" );
    bbuf = NEW_N( unsigned short, vp_width, "RGB blue row buf" );
    if ( alpha )
        abuf = NEW_N( unsigned short, vp_width, "RGB alpha row buf" );
    
    /*
     * Demultiplex r, g, b, and alpha values into separate buffers
     * for output to rgb file on a raster row-by-row basis.
     */
    prgb_byte = ipdat;
    for( j = 0; j < vp_height; j++ )
    {
	pr_short = rbuf;
	pg_short = gbuf;
	pb_short = bbuf;
	pa_short = abuf;
	
        for( i = 0; i < vp_width; i++ )
        {
	    *pr_short++ = (unsigned short) *prgb_byte++;
	    *pg_short++ = (unsigned short) *prgb_byte++;
	    *pb_short++ = (unsigned short) *prgb_byte++;
	    if ( alpha )
	        *pa_short++ = (unsigned short) *prgb_byte++;
        }
	
	putrow( img, rbuf, j, 0 );
	putrow( img, gbuf, j, 1 );
	putrow( img, bbuf, j, 2 );
	if ( alpha )
	    putrow( img, abuf, j, 3 );
    }
    
    iclose( img );

    free( ipdat );
    free( rbuf );
    free( gbuf );
    free( bbuf );
    if ( alpha )
        free( abuf );
}


/************************************************************
 * TAG( screen_to_ps )
 *
 * Simple snap to postscript raster file.  By Doug Speck.
 *
 * Utility funcs prologue, epilogue, and puthexpix swiped from Utah Raster
 * Toolkit program "rletops", necessitating copyright info below.
 */

/*
 * rletops.c - Convert RLE to postscript file.
 *
 * Author:      Rod Bogart & John W. Peterson
 *              Computer Science Dept.
 *              University of Utah
 * Date:        Tue Nov  4 1986
 * Copyright (c) 1986 Rod Bogart
 *
 * Modified by: Gregg Townsend
 *              Department of Computer Science
 *              University of Arizona
 * Date:        June 22, 1990
 * Changes:     50% speedup using putc(), add -C option, translate to page ctr
 *              Fix comments to conform to EPS Version 2.0  (Al Clark)
 *
 * Based on "tobw.c" by Spencer Thomas, and
 * "rps" by Marc Majka (UBC Vision lab)
 *
 * EPSF code from Mike Zyda (Naval Postgraduate School)
 */

#define DFLT_PAGE_HEIGHT        (11.0)
#define DFLT_PAGE_WIDTH         (8.5)
#define DFLT_MARGIN             (1.0)

static int gencps = 1;          /* Only generate color PostScript. */
static void prologue(), epilogue(), puthexpix();

void
screen_to_ps( fname )
char fname[];
{
    unsigned char *ipdat;
    int vp_width, vp_height;
    int nrow, ncol;
    int i, j, rotate, add_line;
    FILE *outfile;
    float page_height = DFLT_PAGE_HEIGHT,
          page_width = DFLT_PAGE_WIDTH,
          margin = DFLT_MARGIN,
          max_prn_height, max_prn_width,
          prn_aspect, img_aspect,
          prn_img_height, prn_img_width,
          page_ymin, page_ymax, page_xmin, page_xmax;

    if ( (outfile = fopen( fname, "w")) == 0 )
    {
        wrt_text( "Unable to open output file %s.\n", fname );
        return;
    }

    /* calc aspect ratios for print page and screen image */
    max_prn_height = page_height - 2.0 * margin;
    max_prn_width = page_width - 2.0 * margin;
    prn_aspect = max_prn_height / max_prn_width;

    vp_width = v_win->vp_width;
    vp_height = v_win->vp_height;

    img_aspect = vp_height / (float) vp_width;

    /* Compare aspect ratios to see if rotation necessary. */
    if ( prn_aspect >= 1.0 && img_aspect < 1.0 ||
         prn_aspect < 1.0 && img_aspect >= 1.0 )
    {
        rotate = TRUE;
        img_aspect = 1.0 / img_aspect;
        nrow = vp_width;
        ncol = vp_height;
        add_line = vp_width & 0x1;
    }
    else
    {
        rotate = FALSE;
        nrow = vp_height;
        ncol = vp_width;
        add_line = vp_height & 0x1;
    }

    /* scale image as large as possible */
    if (img_aspect <= prn_aspect)
    {
        prn_img_width = max_prn_width;
        prn_img_height = img_aspect * max_prn_width;
    }
    else
    {
        prn_img_height = max_prn_height;
        prn_img_width = max_prn_height / img_aspect;
    }

    ipdat = NEW_N( char, vp_width*vp_height*3, "Screen to ps" );

    /* Read the pixel data into ipdat. */
    screen_to_memory( FALSE, vp_width, vp_height, ipdat );

    /* Calc bounding box dimensions (inches). */
    page_xmin = (page_width - prn_img_width) / 2.0;
    page_xmax = page_xmin + prn_img_width;
    page_ymin = (page_height - prn_img_height) / 2.0;
    page_ymax = page_ymin + prn_img_height;

    /* Write to postscript file. */
    prologue( outfile, 0, nrow + add_line, ncol, page_xmin, page_ymin,
              page_xmax, page_ymax);

    /* Write image raster. */
    if (rotate)
    {
        for( j = 0; j < nrow; j++)
            for( i = ncol - 1; i >= 0; i--)
            {
                puthexpix( outfile, *(ipdat + i*nrow*3 + j*3) );
                puthexpix( outfile, *(ipdat + i*nrow*3 + j*3 + 1) );
                puthexpix( outfile, *(ipdat + i*nrow*3 + j*3 + 2) );
            }
    }
    else
    {
        for( i = 0; i < nrow; i++)
            for( j = 0; j < ncol; j++)
            {
                puthexpix( outfile, *(ipdat + i*ncol*3 + j*3) );
                puthexpix( outfile, *(ipdat + i*ncol*3 + j*3 + 1) );
                puthexpix( outfile, *(ipdat + i*ncol*3 + j*3 + 2) );
            }
    }

    /* Laserwriters need an even number of scan lines (from rletops). */
    if (add_line)
        for( j = 0; j < ncol; j++)
        {
            puthexpix( outfile, 0xFF);
            puthexpix( outfile, 0xFF);
            puthexpix( outfile, 0xFF);
        }

    epilogue( outfile, 0 );

    fclose( outfile );
    free( ipdat );
}

static void
prologue(outfile,scribe_mode,nr,nc,x1,y1,x2,y2)
FILE *outfile;
int scribe_mode;
int nr,nc;
float x1,y1,x2,y2;
{
    fprintf(outfile,"%%!\n");
    fprintf(outfile, "%%%%BoundingBox: %d %d %d %d\n",
            (int)(x1 * 72), (int)(y1 * 72),
            (int)(x2 * 72), (int)(y2 * 72));
    fprintf(outfile, "%%%%EndComments\n");
    fprintf(outfile,"gsave\n");
    if ( !scribe_mode )
        fprintf(outfile,"initgraphics\n");
    fprintf(outfile,"72 72 scale\n");
    fprintf(outfile,"/imline %d string def\n",nc*2*(gencps?3:1));
    fprintf(outfile,"/drawimage {\n");
    fprintf(outfile,"    %d %d 8\n",nc,nr);
    fprintf(outfile,"    [%d 0 0 %d 0 %d]\n",nc,-1*nr,nr);
    fprintf(outfile,"    { currentfile imline readhexstring pop } ");
    if ( gencps )
        fprintf(outfile,"false 3 colorimage\n");
    else
        fprintf(outfile,"image\n");
    fprintf(outfile,"} def\n");
    fprintf(outfile,"%f %f translate\n",x1,y2);
    fprintf(outfile,"%f %f scale\n",x2-x1,y1-y2);
    fprintf(outfile,"drawimage\n");
}

static void
epilogue(outfile, scribemode)
FILE *outfile;
int scribemode;
{
    fprintf(outfile,"\n");
    if (!scribemode)
        fprintf(outfile,"showpage\n");
    fprintf(outfile,"grestore\n");
}

static void
puthexpix(outfile,p)
FILE *outfile;
unsigned char p;
{
    static int npixo = 0;
    static char tohex[] = "0123456789ABCDEF";

    putc(tohex[(p>>4)&0xF],outfile);
    putc(tohex[p&0xF],outfile);
    npixo += 1;
    if ( npixo >= 32 )
    {
        putc('\n',outfile);
        npixo = 0;
    }
}


/*
 * SECTION_TAG( Colormap )
 */


/************************************************************
 * TAG( read_text_colormap )
 *
 * Read a colormap from a text file.  The file should contain
 * CMAP_SIZE float triples for r,g,b, in the range [0, 1].
 */
void
read_text_colormap( fname )
char *fname;
{
    FILE *infile;
    float r, g, b;
    int i;

    if ( ( infile = fopen(fname, "r") ) == NULL )
    {
        wrt_text( "Couldn't open file %s.\n", fname );
        return;
    }

    for ( i = 0; i < CMAP_SIZE; i++ )
    {
        fscanf( infile, "%f%f%f", &r, &g, &b );
        v_win->colormap[i][0] = r;
        v_win->colormap[i][1] = g;
        v_win->colormap[i][2] = b;
        if ( feof( infile ) )
        {
	    fclose( infile );
            popup_dialog( WARNING_POPUP, 
	                  "Palette file end reached too soon." );
	    hot_cold_colormap();
            return;
        }
    }
    
    /* Save the colormap's native min and max colors. */
    VEC_COPY( v_win->cmap_min_color, v_win->colormap[0] );
    VEC_COPY( v_win->cmap_max_color, v_win->colormap[CMAP_SIZE - 1] );

    fclose( infile );
}


/************************************************************
 * TAG( hot_cold_colormap )
 *
 * Fills the colormap array with a hot-to-cold colormap.
 * By Jeffery W. Long.
 */
void
hot_cold_colormap()
{
    float delta, rv, gv, bv;
    int group_sz, i, j;

    group_sz = CMAP_SIZE / 4;   /* Number of indices per group. */
    i = 0;                      /* Next color index. */
    delta = 1.0 / (group_sz-1);

    /* Blue to cyan group. */
    rv = 0.0;
    gv = 0.0;
    bv = 1.0;
    for ( j = 0; j < group_sz; j++ )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        gv = MIN( 1.0, gv + delta );
        i++;
    }

    /* Cyan to Green group. */
    gv = 1.0;
    bv = 1.0;
    for ( j = 0; j < group_sz; j++ )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        bv = MAX( 0.0, bv - delta );
        i++;
    }

    /* Green to yellow group. */
    bv = 0.0;
    for ( j = 0; j < group_sz; j++ )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        rv = MIN( 1.0, rv + delta );
        i++;
    }

    /* Yellow to red group. */
    rv = 1.0;
    for ( j = 0; j < group_sz; j++ )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        gv = MAX( 0.0, gv - delta );
        i++;
    }

    /* Take care of any remainder. */
    while ( i < CMAP_SIZE )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        i++;
    }
    
    /* Save the colormap's native min and max colors. */
    VEC_COPY( v_win->cmap_min_color, v_win->colormap[0] );
    VEC_COPY( v_win->cmap_max_color, v_win->colormap[CMAP_SIZE - 1] );
}


/************************************************************
 * TAG( invert_colormap )
 *
 * Invert the colormap.  Just flip it upside down.
 */
void
invert_colormap()
{
    float tmp;
    int i, j;
    int loop_max;
    int idx_max;
    GLfloat ctmp[3];
    
    loop_max = CMAP_SIZE / 2 - 1;
    idx_max = CMAP_SIZE - 1;

    for ( i = 0; i <= loop_max; i++ )
        for ( j = 0; j < 3; j++ )
        {
            tmp = v_win->colormap[i][j];
            v_win->colormap[i][j] = v_win->colormap[idx_max-i][j];
            v_win->colormap[idx_max-i][j] = tmp;
        }

    /* Update the cached native min and max colors. */
    VEC_COPY( v_win->cmap_min_color, v_win->colormap[0] );
    VEC_COPY( v_win->cmap_max_color, v_win->colormap[idx_max] );
}


/************************************************************
 * TAG( set_cutoff_colors )
 *
 * Load cutoff threshold colors into the colormap or reset
 * those colormap entries to their native values.
 */
void
set_cutoff_colors( set, cut_low, cut_high )
Bool_type set;
Bool_type cut_low;
Bool_type cut_high;
{
    if ( set )
    {
        /* Set the low cutoff color. */
        if ( cut_low )
            VEC_COPY( v_win->colormap[0], v_win->rmin_color );

        /* Set the high cutoff color. */
        if ( cut_high )
            VEC_COPY( v_win->colormap[CMAP_SIZE - 1], v_win->rmax_color );
    }
    else
    {
        /* Reset the low cutoff color. */
        if ( cut_low )
            VEC_COPY( v_win->colormap[0], v_win->cmap_min_color );

        /* Reset the high cutoff color. */
        if ( cut_high )
            VEC_COPY( v_win->colormap[CMAP_SIZE - 1], v_win->cmap_max_color );
    }
}


/************************************************************
 * TAG( cutoff_colormap )
 *
 * Fills the given array with a cutoff colormap.  The parameters
 * tell whether the low or high or both cutoffs are active.
 * The cutoff colormap is a hot-to-cold map with out-of-bounds
 * values at the ends.
 */
void
cutoff_colormap( cut_low, cut_high )
Bool_type cut_low;
Bool_type cut_high;
{
    float delta, rv, gv, bv;
    int group_sz, i, j;

    group_sz = CMAP_SIZE / 4;   /* Number of indices per group. */
    i = 0;                      /* Next color index. */
    delta = 1.0 / (group_sz-1);

    /* Blue to cyan group. */
    rv = 0.0;
    gv = 0.0;
    bv = 1.0;
    for ( j = 0; j < group_sz; j++ )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        gv = MIN( 1.0, gv + delta );
        i++;
    }

    /* Cyan to Green group. */
    gv = 1.0;
    bv = 1.0;
    for ( j = 0; j < group_sz; j++ )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        bv = MAX( 0.0, bv - delta );
        i++;
    }

    /* Green to yellow group. */
    bv = 0.0;
    for ( j = 0; j < group_sz; j++ )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        rv = MIN( 1.0, rv + delta );
        i++;
    }

    /* Yellow to red group. */
    rv = 1.0;
    for ( j = 0; j < group_sz; j++ )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        gv = MAX( 0.0, gv - delta );
        i++;
    }

    /* Take care of any remainder. */
    while ( i < CMAP_SIZE )
    {
        v_win->colormap[i][0] = rv;
        v_win->colormap[i][1] = gv;
        v_win->colormap[i][2] = bv;
        i++;
    }

    /* Set the low cutoff color. */
    if ( cut_low )
        VEC_COPY( v_win->colormap[0], v_win->rmin_color );

    /* Set the high cutoff color. */
    if ( cut_high )
        VEC_COPY( v_win->colormap[CMAP_SIZE - 1], v_win->rmax_color );
}


/************************************************************
 * TAG( gray_colormap )
 *
 * Fills the colormap array with a grayscale or inverted
 * grayscale colormap.
 * By Jeffery W. Long.
 */
void
gray_colormap( inverse )
Bool_type inverse;
{
    float start, step;
    int sz, i;

    sz = CMAP_SIZE;

    if ( inverse )
    {
        step = -1.0 / (float)(sz-1);
        start = 1.0;
    }
    else
    {
        step = 1.0 / (float)(sz-1);
        start = 0.0;
    }

    for ( i = 0; i < sz; i++ )
    {
        v_win->colormap[i][0] = start + i*step;
        v_win->colormap[i][1] = start + i*step;
        v_win->colormap[i][2] = start + i*step;
    }
    
    /* Save the colormap's native min and max colors. */
    VEC_COPY( v_win->cmap_min_color, v_win->colormap[0] );
    VEC_COPY( v_win->cmap_max_color, v_win->colormap[CMAP_SIZE - 1] );
}


/************************************************************
 * TAG( contour_colormap )
 *
 * Create N contours in the colormap by sampling the current
 * colormap and creating color bands.
 */
void
contour_colormap( ncont )
int ncont;
{
    float band_sz, col[3], thresh;
    int idx, lo, hi, max;
    int i, j;

    max = CMAP_SIZE - 2;
    thresh = (float) max - 0.001;
    band_sz = thresh / ncont;

    for ( i = 0; i < ncont; i++ )
    {
        idx = (int)(thresh * i / (ncont - 1.0)) + 1;
        VEC_COPY( col, v_win->colormap[idx] );

        lo = i * band_sz + 1;
        hi = (i+1) * band_sz + 1;
        if ( hi > max )
            hi = max;

        for ( j = lo; j <= hi; j++ )
        {
            VEC_COPY( v_win->colormap[j], col );
        }
    }
}


/*
 * SECTION_TAG( Lights )
 */


/************************************************************
 * TAG( delete_lights )
 *
 * Delete any currently bound lights.
 */
void
delete_lights( analy )
Analysis *analy;
{
    int i;
    static GLenum lights[] = 
    {
	GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3, GL_LIGHT4, GL_LIGHT5
    };

    for ( i = 0; i < 6; i++ )
    {
        glDisable( lights[i] );
        v_win->light_active[i] = FALSE;
    }
}


/************************************************************
 * TAG( set_light )
 *
 * Define and enable a light.
 */
void
set_light( token_cnt, tokens )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
{
    float vec[4], val;
    int light_num, light_id, i;

    /* Get the light number. */
    sscanf( tokens[1], "%d", &light_num );
    --light_num;
    v_win->light_active[light_num] = TRUE;

    switch ( light_num )
    {
        case 0:
            light_id = GL_LIGHT0;
            break;
        case 1:
            light_id = GL_LIGHT1;
            break;
        case 2:
            light_id = GL_LIGHT2;
            break;
        case 3:
            light_id = GL_LIGHT3;
            break;
        case 4:
            light_id = GL_LIGHT4;
            break;
        case 5:
            light_id = GL_LIGHT5;
            break;
        default:
            wrt_text( "Light number %d out of range.\n", light_num + 1 );
	    return;
    }
    
    /* Process the light position. */
    sscanf( tokens[2], "%f", &vec[0] );
    sscanf( tokens[3], "%f", &vec[1] );
    sscanf( tokens[4], "%f", &vec[2] );
    sscanf( tokens[5], "%f", &vec[3] );
    glLightfv( light_id, GL_POSITION, vec );
    v_win->light_pos[light_num][0] = vec[0];
    v_win->light_pos[light_num][1] = vec[1];
    v_win->light_pos[light_num][2] = vec[2];
    v_win->light_pos[light_num][3] = vec[3];

    for ( i = 6; i < token_cnt; i++ )
    {
        if ( strcmp( tokens[i], "amb" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &vec[0] );
            sscanf( tokens[i+2], "%f", &vec[1] );
            sscanf( tokens[i+3], "%f", &vec[2] );
            vec[3] = 1.0;
            glLightfv( light_id, GL_AMBIENT, vec );
            i += 3;
        }
        else if ( strcmp( tokens[i], "diff" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &vec[0] );
            sscanf( tokens[i+2], "%f", &vec[1] );
            sscanf( tokens[i+3], "%f", &vec[2] );
            vec[3] = 1.0;
            glLightfv( light_id, GL_DIFFUSE, vec );
            i += 3;
        }
        else if ( strcmp( tokens[i], "spec" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &vec[0] );
            sscanf( tokens[i+2], "%f", &vec[1] );
            sscanf( tokens[i+3], "%f", &vec[2] );
            vec[3] = 1.0;
            glLightfv( light_id, GL_SPECULAR, vec );
            i += 3;
        }

        else if ( strcmp( tokens[i], "spotdir" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &vec[0] );
            sscanf( tokens[i+2], "%f", &vec[1] );
            sscanf( tokens[i+3], "%f", &vec[2] );
            glLightfv( light_id, GL_SPOT_DIRECTION, vec );
            i += 3;
        }
        else if ( strcmp( tokens[i], "spot" ) == 0 )
        {
            sscanf( tokens[i+1], "%f", &val );
            glLightf( light_id, GL_SPOT_EXPONENT, val );
            i++;
            sscanf( tokens[i+1], "%f", &val );
            glLightf( light_id, GL_SPOT_CUTOFF, val );
            i++;
        }
	else
	    wrt_text( "Invalid light parameter \"%s\" ignored.\n", tokens[i] );
    }
    glEnable( light_id );
}


/************************************************************
 * TAG( move_light )
 *
 * Move a light along the specified axis (X == 0, etc.).
 */
void
move_light( light_num, axis, incr )
int light_num;
int axis;
float incr;
{
    float arr[5];
    int light_id;

    if ( light_num < 0 || light_num > 5 )
    {
        wrt_text( "Light number %d out of range.\n", light_num + 1 );
        return;
    }

    v_win->light_pos[light_num][axis] += incr;

    switch ( light_num )
    {
        case 0:
            light_id = GL_LIGHT0;
            break;
        case 1:
            light_id = GL_LIGHT1;
            break;
        case 2:
            light_id = GL_LIGHT2;
            break;
        case 3:
            light_id = GL_LIGHT3;
            break;
        case 4:
            light_id = GL_LIGHT4;
            break;
        case 5:
            light_id = GL_LIGHT5;
            break;
        default:
            wrt_text( "Light number %d out of range.\n", light_num );
    }

    glLightfv( light_id, GL_POSITION, v_win->light_pos[light_num] );
}


/*
 * SECTION_TAG( Title screens )
 */


/************************************************************
 * TAG( set_vid_title )
 *
 * Set a video title line.  The argument nline is the number
 * of the line to be set (0-3).
 */
void
set_vid_title( nline, title )
int nline;
char *title;
{
    if ( nline >= 0 && nline < 4 )
        strcpy( v_win->vid_title[nline], title );
    else
        wrt_text( "Illegal line number.\n" );
}


/************************************************************
 * TAG( draw_vid_title )
 *
 * Put up a video title screen.
 */
void
draw_vid_title()
{
    char str[100];
    float zpos, cx, cy, text_height;
    int i;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glColor3fv( v_win->foregrnd_color );

    antialias_lines( TRUE, TRUE );
    glLineWidth( 1.5 );

    /* Get drawing window and position. */
    get_foreground_window( &zpos, &cx, &cy );

    text_height = 0.1*cy;
    hcentertext( TRUE );
    htextsize( text_height, text_height );

    for ( i = 0; i < 4; i++ )
    {
        hmove( 0.0, 0.45*cy - i*0.2*cy, zpos );
        hcharstr( v_win->vid_title[i] );
    }

    hmove( 0.0, -0.45*cy, zpos );
    hcharstr( env.date );

    hcentertext( FALSE );

    antialias_lines( FALSE, 0 );
    glLineWidth( 1.0 );

    gui_swap_buffers();
}


/************************************************************
 * TAG( copyright )
 *
 * Put up the program name and copyright screen.
 */
void
copyright( analy )
Analysis *analy;
{
    float pos[3], cx, cy, vp_to_world_y, text_height;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glColor3fv( v_win->foregrnd_color  );

    antialias_lines( TRUE, TRUE );
    glLineWidth( 1.5 );

    /* Get drawing window and position. */
    get_foreground_window( &pos[2], &cx, &cy );

    pos[0] = 0.0;

    vp_to_world_y = 2.0*cy / v_win->vp_height;
    text_height = 18.0 * vp_to_world_y;
    hcentertext( TRUE );

    htextsize( 3.0*text_height, 3.0*text_height );
    pos[1] = cy - 5.0*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "GRIZ" );

    pos[1] -= 3.0*text_height;
    hmove( pos[0], pos[1], pos[2] );
    htextsize( text_height, text_height );
    hcharstr( "Visualization of Finite Element Analysis Results" );
    pos[1] -= 1.5*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "" );

    pos[1] -= 3.0*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "Methods Development Group" );
    pos[1] -= 1.5*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "Lawrence Livermore National Laboratory" );

    pos[1] -= 2.0*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "Authors:" );
    pos[1] -= 1.5*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "Don Dovey" );
    pos[1] -= 1.5*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "Tom Spelce" );
    pos[1] -= 1.5*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "Doug Speck" );

    pos[1] -= 3.0*text_height;
    hmove( pos[0], pos[1], pos[2] );
    text_height = 14.0 * vp_to_world_y;
    htextsize( text_height, text_height );
    hcharstr("Copyright (c) 1992 The Regents of the University of California");
    hcentertext( FALSE );

    antialias_lines( FALSE, 0 );
    glLineWidth( 1.0 );

    gui_swap_buffers();
}


/*
 * SECTION_TAG( Swatch )
 */


/*****************************************************************
 * TAG( init_swatch )
 *
 * Init the swatch rendering context.
 */
void
init_swatch()
{
    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glClear( GL_COLOR_BUFFER_BIT );
    glFlush();
    gui_swap_buffers();
}


/*****************************************************************
 * TAG( draw_mtl_swatch )
 *
 * Draw the color swatch with a black border.
 */
void
draw_mtl_swatch( mtl_color )
GLfloat mtl_color[3];
{
    glClear( GL_COLOR_BUFFER_BIT );
    glColor3f( mtl_color[0], mtl_color[1], mtl_color[2] );
    gluOrtho2D( -1.0, 1.0, -1.0, 1.0 );
    glBegin( GL_POLYGON );
        glVertex2f( -0.75, -0.75 );
        glVertex2f( -0.75, 0.75 );
        glVertex2f( 0.75, 0.75 );
        glVertex2f( 0.75, -0.75 );
    glEnd();
    glFlush();
	
    gui_swap_buffers();
}


/*
 * SECTION_TAG( Miscellaneous )
 */


/************************************************************
 * TAG( get_verts_of_bbox )
 *
 * Return the eight vertices of a bounding box from the
 * min and max vertices.
 */
static void
get_verts_of_bbox( bbox, verts )
float bbox[2][3];
float verts[8][3];
{
    int i;

    for ( i = 0; i < 8; i++ )
    {
        VEC_COPY( verts[i], bbox[0] );
    }
    verts[1][0] = bbox[1][0];
    verts[2][0] = bbox[1][0];
    verts[2][1] = bbox[1][1];
    verts[3][1] = bbox[1][1];
    verts[4][2] = bbox[1][2];
    verts[5][0] = bbox[1][0];
    verts[5][2] = bbox[1][2];
    VEC_COPY( verts[6], bbox[1] );
    verts[7][1] = bbox[1][1];
    verts[7][2] = bbox[1][2];
}


/************************************************************
 * TAG( hvec_copy )
 *
 * Copy a homogenous vector.
 */
static void
hvec_copy( b, a )
float b[4];
float a[4];
{
    b[0] = a[0];
    b[1] = a[1];
    b[2] = a[2];
    b[3] = a[3];
}


/************************************************************
 * TAG( antialias_lines )
 *
 * Switch on or off line antialiasing.
 */
static void
antialias_lines( select, depth_buffer_lines )
Bool_type select;
Bool_type depth_buffer_lines;
{
    if ( select == TRUE )
    {
        /* Antialias the lines. */
        glEnable( GL_LINE_SMOOTH );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	/*
	 * For correct antialiasing, this should be uncommented
	 * AND line segments should be depth-sorted and rendered
	 * back-to-front (but there's no code for this yet).
	 * Disabling depth masking (i.e., making the Z-buffer
	 * read only instead of read/write as GRIZ did previously)
	 * without depth-sorting will allow incorrect overlaps
	 * between lines.
	 */
	if ( !depth_buffer_lines )
	    glDepthMask( GL_FALSE );
    }
    else
    {
        /* Antialiasing off. */
        glDisable( GL_LINE_SMOOTH );
        glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );
    }
}


/************************************************************
 * TAG( begin_draw_poly )
 *
 * Set up the stencil buffer for hidden line drawing, and
 * initialize some variables needed for polygon projection
 * when doing "good" interpolation.
 */
static void
begin_draw_poly( analy )
Analysis *analy;
{
    float xp, yp, cp;
    float aspect;

    /* Cache the view matrix for later use. */
    view_transf_mat( &cur_view_mat );

    /* Stencil buffer setup -- not used for 2D grids. */
    if ( analy->render_mode == RENDER_HIDDEN && analy->dimension == 3 )
    {
        glEnable( GL_STENCIL_TEST );
        glStencilFunc( GL_ALWAYS, 0, 1 );
        glStencilOp( GL_INVERT, GL_INVERT, GL_INVERT );
        glColor3fv( v_win->foregrnd_color  );
        /*
        glDisable( GL_LIGHTING );
        */
    }

    /* Calculate constants needed for projecting polygons to screen. */
    if ( analy->interp_mode == GOOD_INTERP )
    {
        /* Correct the aspect ratio for NTSC video, if requested. */
        aspect = v_win->aspect_correct * v_win->vp_width /
                 (float)v_win->vp_height;

        /* Get the window dimensions at the near plane. */
        if ( v_win->orthographic )
            cp = 1.0;
        else
            cp = v_win->near * TAN( DEG_TO_RAD( 0.5*v_win->cam_angle ) );

        if ( aspect >= 1.0 )
        {
            xp = cp * aspect;
            yp = cp;
        }
        else
        {
            xp = cp;
            yp = cp / aspect;
        }

        /* Projection parameters. */
        if ( v_win->orthographic )
        {
            /*
             * See glOrtho() and glFrustrum() in the OpenGL Reference
             * Manual to see where these factors come from.
             */
            proj_param_x = v_win->vp_width / ( xp * 2.0 );
            proj_param_y = v_win->vp_height / ( yp * 2.0 );
        }
        else
        {
            proj_param_x = v_win->vp_width * v_win->near / ( xp * 2.0 );
            proj_param_y = v_win->vp_height * v_win->near / ( yp * 2.0 );
        }
    }
}


/************************************************************
 * TAG( end_draw_poly )
 *
 * Turn off the stencil buffer to end hidden line drawing.
 */
static void
end_draw_poly( analy )
Analysis *analy;
{
    if ( analy->render_mode == RENDER_HIDDEN )
    {
        /*
        glEnable( GL_LIGHTING );
        */
        glDisable( GL_STENCIL_TEST );
    }
}


/**/
/************************************************************
 * TAG( add_clip_plane )
 *
 * .
 */
void
add_clip_plane( abc, d )
float abc[];
float d;
{
    GLdouble pln[4];
    
    pln[0] = abc[0];
    pln[1] = abc[1];
    pln[2] = abc[2];
    pln[3] = d;
    glClipPlane( GL_CLIP_PLANE0, pln );
    glEnable( GL_CLIP_PLANE0 );
}


/************************************************************
 * TAG( linear_variable_scale )
 *
 * Establish a linear scale, i.e., a scale having an interval size
 * which is a product of an integer power of 10 and 1, 2, or 5,
 * and scale values which are integer multiples of the interval
 * size.
 *
 * Given "data_minimum", "data_maximum", and "qty_of_intervals"
 * the routine computes a new scale range from "scale_minimum" to
 * "scale_maximum" divisible into approximately "qty_of_intervals"
 * linear intervals of size "distance".
 *
 */
static void
linear_variable_scale( data_minimum
                      ,data_maximum
                      ,qty_of_intervals
                      ,scale_minimum
                      ,scale_maximum
                      ,distance
                      ,error_status )
float  data_minimum;
float  data_maximum;
int    qty_of_intervals;
float *scale_minimum;
float *scale_maximum;
float *distance;
int   *error_status;
{
    float
          approximate_interval
         ,float_magnitude
         ,geometric_mean [3]
         ,scaled_interval;

    int
        evaluate
       ,i
       ,log_approximate_interval
       ,magnitude;


   /* Establish compensation for computer round-off */

    static
           float epsilon = 0.00002;


    /* Establish acceptable scale interval values (times an integer power of 10 */

    static
           float interval_size[4] = { 1.0, 2.0, 5.0, 10.0 };


    /* */


    if ( (data_maximum > data_minimum) && (qty_of_intervals > 0) )
    {
        *error_status = FALSE;


        /* Establish geometric means between adjacent scale interval values */

        geometric_mean[0] = sqrt( (double) interval_size[0] *
                                  (double) interval_size[1] );
        geometric_mean[1] = sqrt( (double) interval_size[1] *
                                  (double) interval_size[2] );
        geometric_mean[2] = sqrt( (double) interval_size[2] *
                                  (double) interval_size[3] );


        /* Compute approximate interval size of data */

        approximate_interval = ( data_maximum - data_minimum ) / qty_of_intervals;

        log_approximate_interval = (int) log10( approximate_interval );

        if ( approximate_interval < 1.0 )
            log_approximate_interval--;


        /* Scale approximate interval to fall within range:  1 -- 10 */

        scaled_interval = approximate_interval /
                          pow( 10.0, (double) log_approximate_interval );


        /* Establish closest permissible value for scaled interval */

        evaluate = TRUE;
        i = 0;

        while ( (TRUE == evaluate) && (i < 3) )
        {
            if ( scaled_interval < geometric_mean[i] )
                evaluate = FALSE;
            else
                i++;
        }

        *distance = interval_size[i] *
                    pow( 10.0, (double) log_approximate_interval );


        /* Compute new scale minimum */

        float_magnitude = data_minimum / *distance;
        magnitude        = (int) float_magnitude;

        if ( float_magnitude < 0.0 )
            magnitude--;

        if ( fabs( (float) magnitude + 1.0 - float_magnitude ) < epsilon ) 
            magnitude++;

        *scale_minimum = *distance * (float) magnitude;


        /* Compute new scale maximum */

        float_magnitude = data_maximum / *distance;
        magnitude       = (int) ( float_magnitude + 1.0 );

        if ( float_magnitude < -1.0 )
            magnitude--;

        if ( fabs( float_magnitude + 1.0 - (float) magnitude) < epsilon ) 
            magnitude--;

        *scale_maximum = *distance * magnitude;


        /* Adjust, as required, scale limits to account for computer round-off */

        *scale_minimum = MIN( *scale_minimum, data_minimum );
        *scale_maximum = MAX( *scale_maximum, data_maximum );
    }
    else
        *error_status = TRUE;
}


/************************************************************
 * TAG( round )
 *
 * Perform "fuzzy" rounding
 *
 */
static double
round (x, comparison_tolerance)
double x;
double comparison_tolerance;
{
    extern double tfloor ();

    return( tfloor( (x + 0.5), comparison_tolerance ) );
}


/************************************************************
 * TAG( tfloor )
 *
 * Perform "fuzzy" floor operation
 *
 */
static double
tfloor (x, comparison_tolerance)
double x;
double comparison_tolerance;
{
#define DFLOOR(a)  (DINT( (a) ) - (double)fmod( (2.0 + DSIGN( 1.0, (a) )), 3.0 ))
#define DINT(a)    ((a) - (double)fmod( (a), 1.0 ))
#define DSIGN(a,b) ((b) >= 0.0 ? (double)fabs( (a) ) : -(double)fabs( (a) ))

double q, rmax, eps5;
double temp_tfloor;


    if ( x >= 0.0 )
       q = 1.0;
    else
       q = 1.0 - comparison_tolerance;

    rmax = q / (2.0 - comparison_tolerance);

    eps5 = comparison_tolerance / q;

    temp_tfloor = DFLOOR( x + MAX( comparison_tolerance,
                          MIN( rmax, eps5 * (double)fabs( 1.0 + DFLOOR( x ) ))));

    if ( (x <= 0.0) || ((temp_tfloor - x) < rmax) )
        return( temp_tfloor );
    else
        return( temp_tfloor - 1.0 );
}


/************************************************************
 * TAG( machine_tolerance )
 *
 * Compute machine tolerance
 *
 */
static double
machine_tolerance ()
{
double a;
double b;
double c;
double tolerance;


    a = 4.0 / 3.0;

label:

    b = a - 1.0;

    c = b + b + b;

    tolerance = (double)fabs( c - 1.0);

    if ( 0.0 == tolerance )
        goto label;

    return( tolerance );
}


/************************************************************
 * TAG( check_interp_mode )
 *
 * To be called when isocontours or isosurfaces are turned
 * on to notify users of inconsistency with non-interpolated
 * results.
 *
 */
void
check_interp_mode( analy )
Analysis *analy;
{
    if ( analy->interp_mode == NO_INTERP
         && !is_nodal_result( analy->result_id ) )
        popup_dialog( INFO_POPUP, "%s\n%s\n%s\n%s", 
	              "You are currently visualizing non-interpolated results.", 
		      "Isocontours/Isosurfaces are of necessity generated from", 
		      "results which have been interpolated to the nodes to", 
		      "provide variation over the elements." );
}
