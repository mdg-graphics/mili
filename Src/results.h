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
typedef struct _subrec_obj Subrec_obj; // Forward declaration so it can be used in Primal_result.

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
    int valid_superclasses[QTY_SCLASS]; // Superclasses for which this result can be calculated
    Dimensions dim; // Dimensions for with the result can be calculated
    Result_origin_flags origin;
    Bool_type single_precision_input;
    Bool_type hide_in_menu;
    void (*compute_func)();
    Bool_type (*check_compute_func)();
    char * group_name; // A name to group this result under in the gui
    char **short_names; // Short names for derived results
    char **long_names;
    char **primals; // primal results required to calculate the derived result.
    int primal_superclass;
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
    char *short_name;
    char *long_name;
    State_variable *var;
    Result_origin_flags origin;
    List_head *srec_map;        // List of subrec numbers.
    Subrec_obj **subrecs;         // List of Subrec_objs
    int qty_subrecs;
    Bool_type is_shared;        // Is the result shared by multiple element classes
    Bool_type in_vector_array;  // Is the result in a vector array. Used when creating menus.
    int owning_vec_count;
    struct _primal_result **owning_vector_result;
    char **original_names_per_subrec;
    Bool_type in_menu;
} Primal_result;


/************************************************************
 * TAG( ES_in_menu )
 *
 * Structure which stores whether an element sets components
 * have been added to the menu.
 *
 */
typedef struct _ES_in_menu
{
    char component_name[32];
    char parent_menu[32];
    Bool_type in_menu;
} ES_in_menu;



typedef struct _subrecord_result
{
    int subrec_id;
    int index;
    int superclass;
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
    int *srec_ids;
    int srec_id_cnt;
    Subrec_obj **subrecs;         // List of Subrec_objs
    int qty_subrecs;
    char * group_name; // A name to group this result under in the gui
    Bool_type is_shared;        // Is the result shared by multiple element classes
    Bool_type in_menu;
    Bool_type has_indirect;
    Bool_type hide_in_menu;
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

#endif
