/* $Id$ */
/*
 * viewer.h - Definitions for MDG mesh viewer.
 *
 *  	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 *     	Jan 2 1992
 *
 * Copyright (c) 1991 Regents of the University of California
 */

#ifndef VIEWER_H
#define VIEWER_H

#include "misc.h"
#include "list.h"
#include "geometric.h"
#include "mdg.h"
#include "results.h"

/* Maximum number of elements that can be selected at any time. */
#define MAXSELECT 20

/* Dimensions for default and video rendering window sizes */
#define DEFAULT_WIDTH (600)
#define DEFAULT_HEIGHT (600)
#define VIDEO_WIDTH (720)
#define VIDEO_HEIGHT (486)

/* Default number of significant figures in floating point numbers. */
#define DEFAULT_FLOAT_FRACTION_SIZE (2)

/* Number of materials with settable properties. */
#define MAX_MATERIALS 5000

/* Default depth-buffer bias for rendering lines in front of polygons. */
#define DFLT_ZBIAS .005


/*****************************************************************
 * TAG( RGB_raster_obj )
 *
 * An in-memory RGB or RGBA image raster with associated information.
 */
typedef struct _rgb_raster_obj
{
    int img_width;
    int img_height;
    unsigned char *raster;
    Bool_type alpha;
} RGB_raster_obj;


/*****************************************************************
 * TAG( Trace_segment_obj )
 *
 * Information about a particle subtrace which exists within one
 * material.
 */
typedef struct _Trace_segment
{
    struct _Trace_segment *next;
    struct _Trace_segment *prev;
    int material;
    int last_pt_idx;
} Trace_segment_obj;


/*****************************************************************
 * TAG( Trace_pt_def_obj )
 *
 * Start point definition for a particle trace.
 */
typedef struct _Trace_pt_def_obj
{
    struct _Trace_pt_def_obj *next;
    struct _Trace_pt_def_obj *prev;
    float pt[3];
    int elem;
    int material;
    float color[3];
    float xi[3];
} Trace_pt_def_obj;


/*****************************************************************
 * TAG( Trace_pt_obj )
 *
 * Start point for a particle trace.
 */
typedef struct _Trace_pt_obj
{
    struct _Trace_pt_obj *next;
    struct _Trace_pt_obj *prev;
    float pt[3];
    int elnum;
    float xi[3];
    float vec[3];
    Bool_type in_grid;
    int cnt;
    float *pts;
    float *time;
    float color[3];
    int mtl_num;
    int last_elem;
    Trace_segment_obj *mtl_segments;
} Trace_pt_obj;


/*****************************************************************
 * TAG( Vector_pt_obj )
 *
 * Point location for a displayed vector.
 */
typedef struct _Vector_pt_obj
{
    struct _Vector_pt_obj *next;
    struct _Vector_pt_obj *prev;
    float pt[3];
    int elnum;
    float xi[3];
    float vec[3];
} Vector_pt_obj;


/*****************************************************************
 * TAG( Plane_obj )
 *
 * A single plane.
 */
typedef struct _Plane_obj
{ 
    struct _Plane_obj *next;
    struct _Plane_obj *prev;
    float pt[3];
    float norm[3];
} Plane_obj; 


/*****************************************************************
 * TAG( Refl_plane_obj )
 *
 * A reflection plane.
 */
typedef struct _Refl_plane_obj
{
    struct _Refl_plane_obj *next;
    struct _Refl_plane_obj *prev;
    float pt[3];
    float norm[3];
    Transf_mat pt_transf;
    Transf_mat norm_transf;
} Refl_plane_obj;


/*****************************************************************
 * TAG( Result_spec )
 *
 * Stores all information to unambiguously identify a result.
 */
typedef struct _Result_spec
{ 
    Result_type ident;
    Strain_type strain_variety;
    Ref_surf_type ref_surf;
    Ref_frame_type ref_frame;
} Result_spec; 


/*****************************************************************
 * TAG( Minmax_obj )
 *
 * Stores the global min/max for a specific result variable.
 */
typedef struct _Minmax_obj
{ 
    struct _Minmax_obj *next;
    struct _Minmax_obj *prev;
    Result_spec result;
    float minmax[2];     /* Nodal result (original or interpolated). */
    float el_minmax[2];  /* Element result. */
    int mesh_object[2];
    int el_type[2];
} Minmax_obj; 


/*****************************************************************
 * TAG( Triangle_poly )
 *
 * Triangular polygon.
 */
typedef struct _Triangle_poly
{ 
    struct _Triangle_poly *next;
    struct _Triangle_poly *prev;
    float vtx[3][3]; 
    float result[3];
    float norm[3];
    int mat;
    int elem;
} Triangle_poly; 


/*****************************************************************
 * TAG( Surf_poly )
 *
 * Surface polygon.
 */
typedef struct _Surf_poly
{
    struct _Surf_poly *next;
    struct _Surf_poly *prev;
    int cnt;
    float vtx[4][3];
    float norm[4][3];
    int mat;
} Surf_poly;


/*****************************************************************
 * TAG( Ref_poly )
 *
 * Reference polygon.
 */
typedef struct _Ref_poly
{
    struct _Ref_poly *next;
    struct _Ref_poly *prev;
    int nodes[4];
    int mat;
} Ref_poly;


/*****************************************************************
 * TAG( Contour_obj )
 *
 * Holds a contour segment.
 */
typedef struct _Contour_obj
{ 
    struct _Contour_obj *next;
    struct _Contour_obj *prev;
    int cnt;
    float *pts;
    float contour_value;
    float start[2];
    float end[2];
} Contour_obj;


/*****************************************************************
 * TAG( Render_mode_type )
 *
 * Values for the render flag.
 */
typedef enum
{
    RENDER_FILLED,
    RENDER_HIDDEN,
    RENDER_POINT_CLOUD,
    RENDER_NONE
} Render_mode_type;


/*****************************************************************
 * TAG( Interp_mode_type )
 *
 * Values for the interpolation flag.
 */
typedef enum
{
    NO_INTERP,
    REG_INTERP,
    GOOD_INTERP
} Interp_mode_type;


/*****************************************************************
 * TAG( Mouse_mode_type )
 *
 * Various modes for mouse input.
 */
typedef enum
{
    MOUSE_HILITE,
    MOUSE_SELECT
} Mouse_mode_type;


/*****************************************************************
 * TAG( Trans_type )
 *
 * Various modes for the translation widgets.
 */
typedef enum
{
    TRANSLATE_MESH,
    TRANSLATE_LIGHT,
    TRANSLATE_FROM,
    TRANSLATE_AT
} Trans_type;


/*****************************************************************
 * TAG( Filter_type )
 *
 * Filter types for time-history plot smoothing.
 */
typedef enum
{
    BOX_FILTER,
    TRIANGLE_FILTER,
    SYNC_FILTER
} Filter_type;


/*****************************************************************
 * TAG( Exploded_view_type )
 *
 * Types of material exploded views.
 */
typedef enum
{
    SPHERICAL,
    CYLINDRICAL,
    AXIAL
} Exploded_view_type;


/*****************************************************************
 * TAG( Center_view_sel_type )
 *
 * Ways of selecting a center for centering the view.
 */
typedef enum
{
    NO_CENTER = 0,
    HILITE,
    NODE,
    POINT
} Center_view_sel_type;


/*****************************************************************
 * TAG( Material_property_type )
 *
 * Material manager material properties. 
 */
typedef enum
{
    AMBIENT, 
    DIFFUSE, 
    SPECULAR, 
    EMISSIVE, 
    SHININESS, 
    MTL_PROP_QTY /* Always last! */
} Material_property_type;


/*****************************************************************
 * TAG( Util_panel_button_type )
 *
 * Function buttons in the Utility Panel.  
 * Defined here for access by interpret.c as well as gui.c.
 */
typedef enum
{
    STEP_FIRST, 
    STEP_UP, 
    STEP_DOWN, 
    STEP_LAST, 
    STRIDE_EDIT, 
    STRIDE_INCREMENT, 
    STRIDE_DECREMENT, 
    VIEW_SOLID, 
    VIEW_SOLID_MESH, 
    VIEW_EDGES, 
    VIEW_POINT_CLOUD, 
    VIEW_NONE, 
    PICK_MODE_SELECT, 
    PICK_MODE_HILITE, 
    BTN2_SHELL, 
    BTN2_BEAM, 
    CLEAN_SELECT, 
    CLEAN_HILITE, 
    CLEAN_NEARFAR, 
    UTIL_PANEL_BTN_QTY /* Always last! */
} Util_panel_button_type;


/*****************************************************************
 * TAG( Cursor_type )
 *
 * Enumeration of non-default cursors that Griz uses.  
 */
typedef enum
{
    CURSOR_WATCH, 
    CURSOR_EXCHANGE, 
    CURSOR_FLEUR, 
    CURSOR_QTY
} Cursor_type;


/*****************************************************************
 * TAG( Analysis )
 *
 * Struct which contains all the info related to an analysis.
 */
typedef struct _Analysis
{
    struct _Analysis *next;
    struct _Analysis *prev;

    char root_name[50];
    char title[80];
    int num_states;
    int cur_state;
    float *state_times;
    Geom *geom_p;
    State *state_p;
    int dimension;

    Bool_type normals_constant;
    int *hex_adj[6];
    Bool_type *hex_visib;
    int face_cnt;
    int *face_el;
    int *face_fc;
    float *face_norm[4][3];
    float *shell_norm[4][3];
    float *node_norm[3];
    Bool_type shared_faces;
    float bbox[2][3];
    char *bbox_source;
    int num_blocks;
    int *block_lo;
    int *block_hi;
    float *block_bbox[2][3];
    Bool_type keep_max_bbox_extent;
    
    Bool_type show_background_image;
    RGB_raster_obj *background_image;

    Bool_type refresh;
    Bool_type normals_smooth;
    Render_mode_type render_mode;
    Interp_mode_type interp_mode;
    Bool_type show_bbox;
    Bool_type show_coord;
    Bool_type show_time;
    Bool_type show_title;
    Bool_type show_safe;
    Bool_type show_colormap;
    Bool_type show_colorscale;
    Bool_type show_minmax;
    Bool_type use_colormap_pos;
    int float_frac_size;
    Bool_type auto_frac_size;
    float colormap_pos[4];
    Mouse_mode_type mouse_mode;
    Bool_type pick_beams;
    int hilite_type;
    int hilite_num;
    Center_view_sel_type center_view;
    int center_node;
    float view_center[3];
    Bool_type show_node_nums;
    Bool_type show_elem_nums;
    int num_select[4];
    int select_elems[4][MAXSELECT];
    Refl_plane_obj *refl_planes;
    Bool_type reflect;
    Bool_type refl_orig_only;
    Bool_type manual_backface_cull;
    Bool_type displace_exag;
    float displace_scale[3];

    int num_materials;
    Bool_type *hide_material;
    Bool_type *disable_material;
    Bool_type translate_material;
    float *mtl_trans[3];

    Result_type result_id;
    char result_title[80];
    float *result;
    float *beam_result;
    float *shell_result;
    float *hex_result;
    float *tmp_result[6];                 /* Cache for result data. */
    Bool_type show_hex_result;
    Bool_type show_shell_result;
    float zero_result;
    Strain_type strain_variety;
    Ref_surf_type ref_surf;
    Ref_frame_type ref_frame;

    Bool_type use_global_mm;
    Bool_type result_mod;
    float result_mm[2];            /* For result at nodes. */
    float state_mm[2];             /* For result at nodes. */
    int state_mm_nodes[2];         /* For result at nodes. */
    float global_mm[2];            /* For result at nodes. */
    int global_mm_nodes[2];        /* For result at nodes. */
    Minmax_obj *result_mm_list;    /* Cache for global min/max's (at nodes). */
    Bool_type mm_result_set[2];
    float global_elem_mm[2];       /* For result on element. */
    Minmax_obj elem_state_mm;      /* For result on element. */
    Minmax_obj tmp_elem_mm;        /* For result on element. */

    int th_result_type;
    int th_result_cnt;
    Result_spec th_result_spec[30];
    float *th_result;
    Bool_type th_smooth;
    int th_filter_width;
    Filter_type th_filter_type;
    
    Bool_type zbias_beams;
    float beam_zbias;
    
    Bool_type z_buffer_lines;

    Bool_type show_edges;
    int m_edges_cnt;
    int *m_edges[2];
    int *m_edge_mtl;
    float edge_zbias;
   
    int contour_cnt;
    float *contour_vals;
    Bool_type show_contours;
    Contour_obj *contours;
    float contour_width;
    Bool_type show_isosurfs;
    Triangle_poly *isosurf_poly;

    Plane_obj *cut_planes;
    Bool_type show_roughcut;
    Bool_type show_cut;
    Triangle_poly *cut_poly;

    Bool_type show_vectors;
    Bool_type show_vector_spheres;
    Result_type vec_id[3];
    Bool_type vec_col_set;
    Bool_type vec_hd_col_set;
    float vec_min_mag;
    float vec_max_mag;
    float vec_scale;
    float vec_head_scale;
    Vector_pt_obj *vec_pts;
    Bool_type vectors_at_nodes;
    Bool_type scale_vec_by_result;
    
    float dir_vec[3];
    
    Bool_type do_tensor_transform;
    float (*tensor_transform_matrix)[3];

    Bool_type show_carpet;
    float vec_cell_size;
    float vec_jitter_factor;
    float vec_length_factor;
    float vec_import_factor;
    int vol_carpet_cnt;
    float *vol_carpet_coords[3];
    int *vol_carpet_elem;
    int shell_carpet_cnt;
    float *shell_carpet_coords[2];
    int *shell_carpet_elem;
    float reg_cell_size;
    int reg_dim[3];
    float *reg_carpet_coords[3];
    int *reg_carpet_elem;

    Bool_type show_traces;
    Trace_pt_def_obj *trace_pt_defs;
    Trace_pt_obj *trace_pts;
    Trace_pt_obj *new_trace_pts;
    float trace_delta_t;
    int ptrace_limit;
    int trace_disable_qty;
    int *trace_disable;
    float trace_width;

    Bool_type show_extern_polys;
    Surf_poly *extern_polys;
    Bool_type show_ref_polys;
    Bool_type result_on_refs;
    Ref_poly *ref_polys;

    Bool_type perform_unit_conversion;
    float conversion_scale;
    float conversion_offset;
} Analysis;


/*****************************************************************
 * TAG( Environ )
 * 
 * The environment struct contains global program variables which
 * aren't associated with a particular analysis.
 */
typedef struct   
{
    Analysis *curr_analy;
    Bool_type save_history; 
    FILE *history_file;    
    char plotfile_name[100];
    char user_name[30];
    char date[20];
    Bool_type timing;
    Bool_type result_timing;
    Bool_type single_buffer;
    Bool_type foreground;
} Environ;    
  
extern Environ env;

#define MAXTOKENS 25
#define TOKENLENGTH 80


/* viewer.c */
extern void load_analysis();
extern void open_history_file();
extern void close_history_file();
extern void history_command();
extern void read_history_file();
extern char *griz_version;

/* faces.c */
extern void create_hex_adj( /* analy */ );
extern void set_hex_visibility( /* analy */ );
extern void get_hex_faces( /* analy */ );
extern void reset_face_visibility( /* analy */ );
extern void get_face_verts( /* elem, face, analy, verts */ );
extern void get_hex_verts( /* elem, analy, verts */ );
extern void get_shell_verts( /* elem, analy, verts */ );
extern Bool_type get_beam_verts( /* elem, analy, verts */ );
extern void get_node_vert( /* node_num, analy, vert */ );
extern void compute_normals( /* analy */ );
extern void bbox_nodes( /* nodes, bb_lo, bb_hi */ );
extern void count_materials( /* analy */ );
extern Bool_type face_matches_shell( /* el, face, analy */ );
extern int select_item( /* item_type, posx, posy, find_only, analy */ );
extern void get_mesh_edges( /* analy */ );
extern void create_elem_blocks( /* analy */ );
extern void write_ref_file( /* tokens, token_cnt, analy */ );

/* time.c */
extern void change_state();
extern void change_time();
extern void parse_animate_command();
extern void continue_animate();
extern Bool_type step_animate();
extern void end_animate();
extern void get_global_minmax();
extern void tellmm();
extern void outmm();

/* VISUALIZATION */

/* iso_surface.c */
extern void gen_cuts( /* analy */ );
extern void gen_isosurfs( /* analy */ );
extern float get_tri_average_area( /* analy */ );

/* contour.c */
extern void set_contour_vals();
extern void add_contour_val();
extern void gen_contours();
extern void free_contours();
extern void report_contour_vals();

/* shape.c */
extern void shape_fns_2d();
extern void shape_derivs_2d();
extern void shape_fns_3d();
extern void shape_derivs_3d();
extern Bool_type pt_in_hex();
extern Bool_type pt_in_quad();

/* flow.c */
extern void gen_trace_points();
extern void free_trace_points();
extern void particle_trace();
extern Bool_type find_next_subtrace();
extern void save_particle_traces();
extern void read_particle_traces();
extern void gen_vec_points();
extern void update_vec_points();
extern void gen_carpet_points();
extern void sort_carpet_points();
extern void gen_reg_carpet_points();

/* DISPLAY ROUTINES. */

/* draw.c */
extern char *strain_label[];
extern char *ref_surf_label[];
extern char *ref_frame_label[];
extern void init_mesh_window();
extern void reset_mesh_window();
extern void get_pick_ray();
extern void set_orthographic();
extern void set_mesh_view();
extern void aspect_correct();
extern void set_camera_angle();
extern void inc_mesh_rot();
extern void inc_mesh_trans();
extern void set_mesh_scale();
extern void get_mesh_scale();
extern void reset_mesh_transform();
extern void set_view_to_bbox();
extern void look_from();
extern void look_at();
extern void look_up();
extern void move_look_from();
extern void move_look_at();
extern void adjust_near_far();
extern void set_near();
extern void set_far();
extern void center_view();
extern void view_transf_mat();
extern void inv_view_transf_mat();
extern void print_view();
extern void define_materials();
extern void change_current_material();
extern void update_current_material();
extern void set_material();
extern void set_color();
extern void update_display();
extern void get_foreground_window();
extern void quick_update_display();
extern void rgb_to_screen();
extern void screen_to_memory();
extern void screen_to_rgb();
extern void screen_to_ps();
extern void read_hdf_colormap();
extern void read_text_colormap();
extern void hot_cold_colormap();
extern void invert_colormap();
extern void set_cutoff_colors();
extern void cutoff_colormap();
extern void gray_colormap();
extern void contour_colormap();
extern void delete_lights();
extern void set_light();
extern void move_light();
extern void set_vid_title();
extern void draw_vid_title();
extern void copyright();
extern void check_interp_mode();
/**/
extern void add_clip_plane();
extern void init_swatch();
extern void draw_mtl_swatch();

/* time_hist.c */
extern void time_hist_plot();
extern void parse_mth_command();
extern void mat_time_hist();
extern void parse_gather_command();
extern void gather_results();
extern void clear_gather();

/* poly.c */
extern void read_slp_file();
extern void read_ref_file();
extern void gen_ref_from_coord();
float get_ref_average_area();
extern void write_obj_file();
extern void write_vv_connectivity_file();
extern void write_vv_state_file();

/* USER INTERFACE. */

/* interpret.c */
extern void read_token();
extern void parse_command();
extern void update_vis();
extern void create_reflect_mats();
extern void write_preamble();
extern Bool_type is_numeric_token();

/* gui.c */
extern void gui_start();
extern void gui_swap_buffers();
extern void init_animate_workproc();
extern void end_animate_workproc();
extern void set_window_size();
extern int get_monitor_width();
extern void wrt_text();
extern void popup_dialog();
extern void clear_popup_dialogs();
extern void popup_fatal();
extern void quit();
extern void switch_opengl_win();
extern void destroy_mtl_mgr();
extern void update_util_panel();
extern void reset_window_titles();
extern void set_alt_cursor();
extern void unset_alt_cursor();
extern void set_motion_threshold();

/* DERIVED VARIABLES. */

/* results.c */
extern void init_resultid_table();
extern int lookup_result_name( /* result_com */ );
extern int is_derived( /* Result_type */ );
extern int is_hex_result( /* Result_type */ );
extern int is_shell_result( /* Result_type */ );
extern int is_beam_result( /* Result_type */ );
extern int is_nodal_result( /* Result_type */ );
extern Bool_type mod_required( /* analy, modifier, old, new */ );
extern int get_result_modifier();
extern float hex_vol( /* xx, yy, zz */ );
extern float hex_vol_exact( /* xx, yy, zz */ );
extern void hex_to_nodal( /* val_hex, val_nodal, analy */ );
extern void shell_to_nodal( /* val_shell, val_nodal, analy */ );
extern void init_mm_obj( /* Minmax_obj *p_minmax_obj */ );
extern void load_result( /* analy */ );

/* show.c */
extern int parse_show_command();
extern void cache_global_minmax();
extern Bool_type reset_global_minmax();
extern void compute_shell_in_data();
extern void compute_beam_in_data();
extern void compute_node_in_data();
extern void init_resultid_table();
extern int is_derived();
extern Bool_type match_result();

/* strain.c */
extern void compute_share_eff_strain();
extern void compute_share_strain();
extern void compute_hex_strain();
extern void compute_beam_axial_strain();
extern void set_diameter();
extern void set_youngs_mod();
  /* Should go in hex.c or something.... */
extern void compute_hex_relative_volume();

/* stress.c */
extern void compute_share_stress();
extern void compute_share_press();
extern void compute_share_effstress();
extern void compute_share_prin_stress();
extern void compute_shell_surface_stress();

/* node.c */
extern void compute_node_displacement();
extern void compute_node_velocity();
extern void compute_node_acceleration();
extern void compute_node_temperature();
extern void compute_node_pr_intense();
extern void set_pr_ref();
extern void compute_node_helicity();
extern void compute_node_enstrophy();
extern void compute_vector_component();

/* frame.c */
extern void global_to_local_mtx();
extern void transform_tensors();
extern Bool_type transform_primal_stress_strain();

/* explode.c */
extern int associate_matl_exp();
extern void explode_material();
extern void free_matl_exp();
extern void remove_exp_assoc();
extern void report_exp_assoc();

#endif VIEWER_H
