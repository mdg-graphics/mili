/* $Id$ */
/*
 * traction_force.h
 *
 * This work was produced at the University of California, Lawrence
 * Livermore National Laboratory (UC LLNL) under contract no.
 * W-7405-ENG-48 (Contract 48) between the U.S. Department of Energy
 * (DOE) and The Regents of the University of California (University)
 * for the operation of UC LLNL. Copyright is reserved to the University
 * for purposes of controlled dissemination, commercialization through
 * formal licensing, or other disposition under terms of Contract 48;
 * DOE policies, regulations and orders; and U.S. statutes. The rights
 * of the Federal Government are reserved under Contract 48 subject to
 * the restrictions agreed upon by the DOE and University as allowed
 * under DOE Acquisition Letter 97-1.
 *
 *
 * DISCLAIMER
 *
 * This work was prepared as an account of work sponsored by an agency
 * of the United States Government. Neither the United States Government
 * nor the University of California nor any of their employees, makes
 * any warranty, express or implied, or assumes any liability or
 * responsibility for the accuracy, completeness, or usefulness of any
 * information, apparatus, product, or process disclosed, or represents
 * that its use would not infringe privately-owned rights.  Reference
 * herein to any specific commercial products, process, or service by
 * trade name, trademark, manufacturer or otherwise does not necessarily
 * constitute or imply its endorsement, recommendation, or favoring by
 * the United States Government or the University of California. The
 * views and opinions of authors expressed herein do not necessarily
 * state or reflect those of the United States Government or the
 * University of California, and shall not be used for advertising or
 * product endorsement purposes.
 *
 */

/*
 * Global parameters -- surface; traction
 */

#ifndef GLOBAL_TRACTION_H
#define GLOBAL_TRACTION_H

#ifdef TRACTION_ALLOCATE
#define GLOBAL_TRACTION
#else
#define GLOBAL_TRACTION extern
#endif



#define ROUND(a)       ( (a) + 0.5 )

#define TWO_PI         PI * 2.0
#define TWO_THIRDS     2.0 / 3.0

enum
{
    MAXIMUM_SURFACE_POINTS = 333
    ,cubed                  = 1
    ,invalid_state          = INT_MIN
    ,null_element           = INT_MIN
    ,squared                = 0
    ,auto_computing_state   = 0
    ,valid_state            = 0
};



typedef void (*p_surface_function)();


typedef struct _vector
{
    float x;
    float y;
    float z;
} Vector;

typedef struct _vector_rst
{
    float r;
    float s;
    float t;
} Vector_rst;

typedef struct _surface_parameter_data
{
    Vector  p;
    Vector  v;
    float   a;
    float   b;
    float   delta_a;
    float   delta_b;
    float   h;
    int     n;
    char   *type;
} Surface_parameters;

typedef struct _traction_data
{
    Vector     point;                        /* x y z */
    Vector_rst element_centered_coordinates; /* r s t */
    Vector     normal;                       /* n.x n.y n.z */
    float      area;
    int        element;
} Traction_data;

extern Display
*dpy_copy;

GLOBAL_TRACTION Surface_parameters
surface_parameter_table;

GLOBAL_TRACTION Traction_data
*traction_data_table;

GLOBAL_TRACTION Vector
u0
,u1
,u2;

GLOBAL_TRACTION char
*traction_material_list;

GLOBAL_TRACTION float
surface_area;

GLOBAL_TRACTION int
qty_tdt;

extern int
surface_command_processed
,traction_data_table_initialized;

GLOBAL_TRACTION p_surface_function
surface_target;




/*
 * procedure prototypes
 */

extern void init_vec_points_hex( Vector_pt_obj **, int, Analysis *, Bool_type );


GLOBAL_TRACTION p_surface_function  surface_function_lookup( char *name );

GLOBAL_TRACTION float is_valid_number( const char *string, int *validity_status );
GLOBAL_TRACTION float traction_area( Analysis *analy, char *traction_material_list );

GLOBAL_TRACTION int   auto_compute_n( Analysis *analy );
GLOBAL_TRACTION int   sft_compare( const void *key, const void *sft );

GLOBAL_TRACTION void  compute_u0_u1_and_u2_vectors( Vector v, Vector *u0, Vector *u1, Vector *u2 );
GLOBAL_TRACTION void  initialize_surface_parameter_table();
GLOBAL_TRACTION void  normalize_vector( Vector *a );
GLOBAL_TRACTION void  traction( Analysis *analy, char *traction_material_list );

GLOBAL_TRACTION int parse_surface_command( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt );
GLOBAL_TRACTION int parse_traction_command( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt );
GLOBAL_TRACTION int parse_poly( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt );
GLOBAL_TRACTION int parse_rect( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt );
GLOBAL_TRACTION int parse_ring( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt );
GLOBAL_TRACTION int parse_spot( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt );
GLOBAL_TRACTION int parse_tube( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt );

GLOBAL_TRACTION void poly();
GLOBAL_TRACTION void rect();
GLOBAL_TRACTION void ring();
GLOBAL_TRACTION void spot();
GLOBAL_TRACTION void tube();


#endif
