/* $Id$ */
/*
 * results.h - Data structures for the selection of analysis results.
 *
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      Jan 6 1992
 *
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
 *************************************************************************
 *
 * Modification History
 *
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Aug 26, 2005: Add option to hide result from pull-down 
 *                list using new variable 'hide_in_menu'.
 *                See SRC# 339.
 *************************************************************************
 *
 */

#ifndef RESULTS_H
#define RESULTS_H

#include "misc.h"

typedef struct _result_origin_flags
{
    unsigned int is_primal      : 1;
    unsigned int is_alias       : 1;
    unsigned int is_derived     : 1;
    unsigned int is_interpreted : 1;
    unsigned int is_unit_result : 1;
    unsigned int is_elem_result : 1;
    unsigned int is_node_result : 1;
    unsigned int is_mat_result  : 1;
    unsigned int is_mesh_result : 1;
    unsigned int is_surface_result : 1;
} Result_origin_flags;


typedef struct _result_modifier_flags
{
    unsigned int use_ref_surface    : 1;
    unsigned int use_ref_frame      : 1;
    unsigned int use_strain_variety : 1;
    unsigned int time_derivative    : 1;
    unsigned int coord_transform    : 1;
    unsigned int use_ref_state      : 1;
} Result_modifier_flags;

typedef struct _dimensions
{
    unsigned int d2 : 1;
    unsigned int d3 : 1;
} Dimensions;


typedef struct _result_candidate
{
    int superclass;
    Dimensions dim;
    Result_origin_flags origin;
    Bool_type single_precision_input;
    Bool_type hide_in_menu ;
/**/
/*    void (*compute_func)( Analysis *, float *, Bool_type ); */
    void (*compute_func)();
    Bool_type (*check_compute_func)();
    char **short_names;
    char **long_names;
    char **primals;
    int *primal_superclasses;
} Result_candidate;


typedef struct _list_head
{
    int qty;
    void *list;
} List_head;

/*****************************************************************
 * TAG( RESULT_DEFINES )
 *
 */
#ifndef EXTREME_MIN 
#define EXTREME_MIN 0
#define EXTREME_MAX 1
#endif

/*****************************************************************
 * TAG( RESULT_DEFINES )
 *
 */
#ifndef EXTREME_MIN
#define EXTREME_MIN 0
#define EXTREME_MAX 1
#endif
                                                                                    
/************************************************************
 * TAG( Primal_result )
 *
 * Structure which stores utilization data for state variables.
 * Notes:
 *   "srec_map" is an array of List_head structs, one per srec 
 *   format.  If the "qty" field of a List_head struct is non-
 *   zero, then the "list" field points to an array of integers, 
 *   each entry of which contains an index of a subrecord that 
 *   binds the primal result.
 *
 *   Field short_name always points to a string which is allocated
 *   elsewhere (in a State_variable struct or Htable_entry key)
 *   and so is never freed.  Field long_name is the same when
 *   origin.is_primal is true, but should be allocated locally
 *   if origin.is_alias is true (and then can be freed when the
 *   Primal_result struct is freed).
 */
typedef struct _primal_result
{
    State_variable *var;
    Result_origin_flags origin;
    List_head *srec_map;
    char *short_name;
    char *long_name;
    Bool_type in_menu;
} Primal_result;


typedef struct _subrecord_result
{
    int subrec_id;
    int index;
    Result_candidate *candidate;
    Bool_type indirect;
} Subrecord_result;


/*****************************************************************
 * TAG( Derived_result )
 *
 * Structure which holds availability data for a derived result.
 * Notes:
 *   The "srec_map" entry is conceptually like that for the Primal
 *   result struct, but the contents reference more data:
 *   "srec_map" is an array of List_head structs, one per srec 
 *   format.  If the "qty" field of a List_head struct is non-zero,
 *   then the "list" field points to an array of Subrecord_result 
 *   structs, each of which indicates the subrecord that supports 
 *   the derived result calculation and the index into a result 
 *   candidate struct's entries to the particular derived result.
 */
typedef struct _derived_result
{
    Result_origin_flags origin;
    List_head *srec_map;
    Bool_type in_menu;
    Bool_type has_indirect;
} Derived_result;


/*****************************************************************
 * TAG( Result_modifier_type )
 *
 * Options which affect how some results are calculated.
 */
typedef enum
{
    STRAIN_TYPE = 0,
    REFERENCE_SURFACE,
    REFERENCE_FRAME, 
    TIME_DERIVATIVE, 
    REFERENCE_STATE, 
    COORD_TRANSFORM,
    QTY_RESULT_MODIFIER_TYPES
} Result_modifier_type;


/*****************************************************************
 * TAG( Strain_type )
 *
 * Options for strain calculation.
 */
typedef enum
{
    INFINITESIMAL,
    GREEN_LAGRANGE,
    ALMANSI,
    RATE
} Strain_type;


/*****************************************************************
 * TAG( Ref_surf_type )
 * TAG( Ref_frame_type )
 *
 * Options for shell stress calculation.
 */
typedef enum
{
    MIDDLE,
    INNER,
    OUTER
} Ref_surf_type;

typedef enum
{
    GLOBAL,
    LOCAL
} Ref_frame_type;



#ifdef OLD_RESULTS
/*****************************************************************
 * TAG( Result_type )
 *
 * Enumerated type which specifies the analysis values which can be
 * obtained from the data.
 */
typedef enum
{
    VAL_NONE,              /* No result value selected. */

    VAL_NODE_BEGIN,        /* DUMMY */

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
    VAL_NODE_DISPR,        /* Nodal Radial displacement */
    VAL_NODE_DISPMAG,      /* Nodal displacement magnitude */
    VAL_NODE_MODEDISPMAG,  /* Nodal modal displacement magnitude */
    VAL_NODE_PINTENSE,     /* Nodal pressure intensity */
    VAL_NODE_HELICITY,     /* Nodal helicity */
    VAL_NODE_ENSTROPHY,    /* Nodal enstrophy */
    VAL_NODE_K,            /* Nodal K value */
    VAL_NODE_EPSILON,      /* Nodal epsilon */
    VAL_NODE_A2,           /* Nodal A2 */
    VAL_PROJECTED_VEC,     /* Projected vector magnitude */

    /* For file reader only. */
    VAL_NODE_POS,

    VAL_NODE_END,          /* DUMMY */
    VAL_HEX_BEGIN,         /* DUMMY */

    VAL_HEX_RELVOL,        /* Relative volume */
    VAL_HEX_VOL_STRAIN,    /* Volumetric strain */
    VAL_HEX_EPS_PD1,       /* 1st principal deviatoric strain */
    VAL_HEX_EPS_PD2,       /* 2nd principal deviatoric strain */
    VAL_HEX_EPS_PD3,       /* 3rd principal deviatoric strain */
    VAL_HEX_EPS_MAX_SHEAR, /* Maximum shear strain */
    VAL_HEX_EPS_P1,        /* 1st principal strain */
    VAL_HEX_EPS_P2,        /* 2nd principal strain */
    VAL_HEX_EPS_P3,        /* 3rd principal strain */

    /* For file reader only. */
    VAL_HEX_SIGX,
    VAL_HEX_SIGY,
    VAL_HEX_SIGZ,
    VAL_HEX_SIGXY,
    VAL_HEX_SIGYZ,
    VAL_HEX_SIGZX,
    VAL_HEX_EPS_EFF,

    VAL_HEX_END,           /* DUMMY */
    VAL_SHELL_BEGIN,       /* DUMMY */

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

/* There is no computation for these
    VAL_SHELL_EFF4,        * Lower surface effective plastic strain *
    VAL_SHELL_EFF5,        * Upper surface effective plastic strain *
*/

    /* For file reader only. */
    VAL_SHELL_SIGX_MID,
    VAL_SHELL_SIGY_MID,
    VAL_SHELL_SIGZ_MID,
    VAL_SHELL_SIGXY_MID,
    VAL_SHELL_SIGYZ_MID,
    VAL_SHELL_SIGZX_MID,
    VAL_SHELL_EPS_EFF_MID,
    VAL_SHELL_SIGX_IN,
    VAL_SHELL_SIGY_IN,
    VAL_SHELL_SIGZ_IN,
    VAL_SHELL_SIGXY_IN,
    VAL_SHELL_SIGYZ_IN,
    VAL_SHELL_SIGZX_IN,
    VAL_SHELL_EPS_EFF_IN,
    VAL_SHELL_SIGX_OUT,
    VAL_SHELL_SIGY_OUT,
    VAL_SHELL_SIGZ_OUT,
    VAL_SHELL_SIGXY_OUT,
    VAL_SHELL_SIGYZ_OUT,
    VAL_SHELL_SIGZX_OUT,
    VAL_SHELL_EPS_EFF_OUT,
    VAL_SHELL_ELDEP1,
    VAL_SHELL_ELDEP2,
    VAL_SHELL_EPSX_IN,
    VAL_SHELL_EPSY_IN,
    VAL_SHELL_EPSZ_IN,
    VAL_SHELL_EPSXY_IN,
    VAL_SHELL_EPSYZ_IN,
    VAL_SHELL_EPSZX_IN,
    VAL_SHELL_EPSX_OUT,
    VAL_SHELL_EPSY_OUT,
    VAL_SHELL_EPSZ_OUT,
    VAL_SHELL_EPSXY_OUT,
    VAL_SHELL_EPSYZ_OUT,
    VAL_SHELL_EPSZX_OUT,

    VAL_SHELL_END,         /* DUMMY */
    VAL_BEAM_BEGIN,        /* DUMMY */

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

    VAL_BEAM_END,          /* DUMMY */
    VAL_SHARE_BEGIN,       /* DUMMY */

    VAL_SHARE_SIGX,        /* X stress */
    VAL_SHARE_SIGY,        /* Y stress */
    VAL_SHARE_SIGZ,        /* Z stress */
    VAL_SHARE_SIGXY,       /* XY stress */
    VAL_SHARE_SIGYZ,       /* YZ stress */
    VAL_SHARE_SIGZX,       /* ZX stress */
    VAL_SHARE_SIG_EFF,     /* Effective stress */
    VAL_SHARE_SIG_PD1,     /* 1st prinicipal deviatoric stress */
    VAL_SHARE_SIG_PD2,     /* 2nd prinicipal deviatoric stress */
    VAL_SHARE_SIG_PD3,     /* 3rd prinicipal deviatoric stress */
    VAL_SHARE_SIG_MAX_SHEAR,/* Maximum shear stress */
    VAL_SHARE_SIG_P1,      /* 1st principal stress */
    VAL_SHARE_SIG_P2,      /* 2nd principal stress */
    VAL_SHARE_SIG_P3,      /* 3rd principal stress */
    VAL_SHARE_EPSX,        /* X strain */
    VAL_SHARE_EPSY,        /* Y strain */
    VAL_SHARE_EPSZ,        /* Z strain */
    VAL_SHARE_EPSXY,       /* XY strain */
    VAL_SHARE_EPSYZ,       /* YZ strain */
    VAL_SHARE_EPSZX,       /* ZX strain */
    VAL_SHARE_EPS_EFF ,    /* Effective plastic strain */
    VAL_SHARE_PRESS,       /* Pressure */

    VAL_SHARE_END,         /* DUMMY */

    /*
     * For file reader only.  Due to the file format, these
     * are treated as seperate from hex, shell and beam results.
     */
    VAL_HEX_ACTIVITY,
    VAL_SHELL_ACTIVITY,
    VAL_BEAM_ACTIVITY,

    VAL_ALL_END            /* DUMMY */

} Result_type;


/*****************************************************************
 * TAG( trans_result )
 *
 * Translation table which associates a result type with the command
 * string used to identify that result type.
 *
 * Each row in the table contains:
 *
 * 0 - Result ID (int)
 * 1 - Result title (string)
 * 2 - Function which computes the result (function)
 *         its arguments are: (analy, result_array)
 * 3 - Command string for displaying result (string)
 */
extern char *trans_result[][4];


/*****************************************************************
 * TAG( resultid_to_index )
 *
 * Table which gives the index in the trans_result table for each
 * result id.
 */
extern int *resultid_to_index;
#endif


#endif
