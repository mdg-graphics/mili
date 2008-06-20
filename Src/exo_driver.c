/* $Id$ */
/*
 * exo_driver.c - Wrappers for Exodus II plotfile I/O functions
 *                which implement a standard Griz I/O interface.
 *
 *      Douglas E. Speck
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      2 Sep 1999
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
 */

/*
 * cc -c -n32 exo_driver.c
 * ld -shared -all exo_driver.o -lc -L/usr/people/speck/Exodus/cbind/lib/sgi32 \
 *  libexoIIc.a -L/usr/people/speck/Exodus/netcdf-3.4/src/libsrc libnetcdf.a \
 *  -o libgex.so
 */

#include <stdlib.h>
#include "viewer.h"
#include "exodusII.h"
#include "exo_driver.h"

#define OK 0

/* Cache quantities from ex_get_init for later Griz use. */
static Bool_type called_ex_get_init = FALSE;
static int dims, node_qty, elem_qty, elem_blk_qty, node_set_qty, side_set_qty;
static int qty_global_vars, qty_nodal_vars, qty_elem_vars;

static int qty_states;
static float *state_times;

static char title[MAX_LINE_LENGTH + 1];

static char *node_class_short_name = "node";
static char *node_class_long_name = "Node";
static char *mesh_class_short_name = "glob";
static char *mesh_class_long_name = "Global";

static float *node_x, *node_y, *node_z;
static int node_x_index, node_y_index, node_z_index;
static int *status_var_index;

static float *global_data_buffer = NULL;

static Mesh_data *p_mesh_array;
static int mesh_qty;
static Subrecord *format_subrecs;
static int format_subrec_qty;
static Elem_block_data *elem_block_data;
static int non_elem_subrec_qty;
static List_head *subrec_block_lists;
static Hash_table **subrec_var_htables;
static MO_class_data **nonproper_classes;
static int nonproper_class_qty;

static int init_geom_classes( int );
static int gen_state_description( int );
static void identify_exodus_superclass( char *, int, int * );
static void interleave_node_vals( float * );
static int calc_subrec_qty( int, int * );
static int exodus_create_st_variable( int, Hash_table *, char * );
static void gen_material_data( MO_class_data *, Mesh_data * );
static void exodus_st_var_delete( void *p_state_variable );


/************************************************************
 * TAG( exodus_db_open )
 *
 * Open an Exodus plotfile.
 */
extern int
exodus_db_open( char *path_root, int *p_dbid )
{
    int exo_id;
    int status;
    int CPU_word_size, IO_word_size;
    float exo_version;
    float fdum;
    char cdum[1];
    
    CPU_word_size = sizeof( float );
    IO_word_size = 0;
    exo_id = ex_open( path_root, EX_READ, &CPU_word_size, &IO_word_size,
                      &exo_version );
    
    if ( exo_id < 0 )
        return exo_id;
    
    /* Read times. */
    status = ex_inquire( exo_id, EX_INQ_TIME, &qty_states, &fdum, cdum );
    if ( status < 0 )
    {
        ex_close( exo_id );
        return status;
    }
    
    if ( qty_states > 0 )
    {
        state_times = NEW_N( float, qty_states, "State times array" );
        
        status = ex_get_all_times( exo_id, state_times );
        
        if ( status < 0 )
        {
            ex_close( exo_id );
            free( state_times );
            
            return status;
        }
    }
    
    /*
     * Perform basic initialization of mesh object classes and the 
     * state record format now to create a repository of information
     * to enable the ExodusII driver to act like a Mili db for several
     * upcoming GIO calls and (potentially) db_query calls (which
     * technically should not depend on having already called the
     * GIO get_geom and get_st_descriptor calls).  The geometry data will
     * be "given" over to Griz when get geom is called, but a copy of
     * the state format data will be kept locally since it is relatively
     * small and will support db query calls.
     */
    status = init_geom_classes( exo_id );
    status = gen_state_description( exo_id );
    
    *p_dbid = exo_id;
    return 0;
}


/************************************************************
 * TAG( init_geom_classes )
 *
 * Perform initial examination of ExodusII data to allocate
 * mesh object classes.  Performs enough initialization to
 * permit building a state record description; does not read 
 * in coordinates or element connectivities.
 */
static int
init_geom_classes( int dbid )
{
    int status;
    Hash_table *p_ht;
    Htable_entry *p_hte;
    Mesh_data *p_md;
    MO_class_data *p_mocd;
    int elem_class_count;
    int i, j;
    char elem_type[MAX_STR_LENGTH + 1];
    int qty_elems_in_blk, qty_nodes_per_elem, qty_attributes;
    int rval;
    int sclass;
    int *elem_block_ids;
    char *mat_class_name = "Material";
    Material_data *p_matd;

    if ( !called_ex_get_init )
    {
        status = ex_get_init( dbid, title, &dims, &node_qty, &elem_qty, 
                              &elem_blk_qty, &node_set_qty, &side_set_qty );
        
        if ( status < 0 )
            return status;
        else
            called_ex_get_init = TRUE;
    }
    
    /*
     * ExodusII db's will always (?) have one mesh, but a coherent naming scheme
     * could potentially be used to "knit" several together, demanding
     * multi-mesh capability.  This code follows the pattern of 
     * mili_db_get_geom() (since Griz is set up for an array of meshes) but
     * hard-codes the qty to one.
     */
    
    mesh_qty = 1;

    /* Allocate array of pointers to mesh geom hash tables. */
    p_mesh_array = NEW_N( Mesh_data, mesh_qty, "Mesh data array" );
    
    for ( i = 0; i < mesh_qty; i++ )
    {
        /* Allocate a hash table for the mesh geometry. */
        p_md = p_mesh_array + i;
        p_ht = htable_create( 151 );
        p_md->class_table = p_ht;
            
        /* 
         * Create mesh geometry table entry. 
         * Hard-code class names consistent with Taurus nodal class names.
         */

        htable_search( p_ht, node_class_short_name, ENTER_ALWAYS, &p_hte );
        
        p_mocd = NEW( MO_class_data, "Nodes geom table entry" );
        
        p_mocd->mesh_id = i;
        griz_str_dup( &p_mocd->short_name, node_class_short_name );
        griz_str_dup( &p_mocd->long_name, node_class_long_name );
        p_mocd->superclass = M_NODE;
        p_mocd->elem_class_index = -1;
        p_mocd->qty = node_qty;
        
        p_hte->data = (void *) p_mocd;
        p_md->node_geom = p_mocd;
 
        /*
         * Prepare to init element classes.
         */
        elem_class_count = 0;
        
        elem_block_data = NEW_N( Elem_block_data, elem_blk_qty,
                                 "Element block data structs" );
        elem_block_ids = NEW_N( int, elem_blk_qty, "Element block ids" );
        
        status = ex_get_elem_blk_ids( dbid, elem_block_ids );
        if ( status < 0 )
            return status;
        
        /* 
         * Process element block data to identify the element classes 
         * and their sizes. 
         */
        for ( j = 0; j < elem_blk_qty; j++ )
        {
            status = ex_get_elem_block( dbid, elem_block_ids[j], elem_type,
                                        &qty_elems_in_blk, &qty_nodes_per_elem,
                                        &qty_attributes );
            if ( status < 0 )
                return status;

            rval = htable_search( p_ht, elem_type, ENTER_MERGE, &p_hte );

            if ( rval == ENTRY_EXISTS )
                p_mocd = (MO_class_data *) p_hte->data;
            else
            {
                p_mocd = NEW( MO_class_data, "New elem geom table entry" );
                
                p_mocd->mesh_id = i;
                griz_str_dup( &p_mocd->short_name, elem_type );
                griz_str_dup( &p_mocd->long_name, elem_type );
                
                identify_exodus_superclass( elem_type, qty_nodes_per_elem, 
                                            &sclass );
                p_mocd->superclass = sclass;
                if ( sclass != G_UNIT )
                    p_mocd->elem_class_index = elem_class_count++;
                else
                {
                    p_mocd->elem_class_index = -1;
                    p_mocd->simple_start = 1;
                }
                
                p_hte->data = (void *) p_mocd;
            }
            
            elem_block_data[j].p_class = p_mocd;
            elem_block_data[j].block_id = elem_block_ids[j];
            elem_block_data[j].block_size = qty_elems_in_blk;
            elem_block_data[j].class_id_base = p_mocd->qty;
            p_mocd->qty += qty_elems_in_blk;
            if ( sclass == G_UNIT )
                p_mocd->simple_stop = p_mocd->qty;
        }

        free( elem_block_ids );
        
        /* Update the Mesh_data struct with element class info. */
        p_md->elem_class_qty = elem_class_count;
        
        /* 
         * Create a mesh class - Griz needs it, and ExodusII does allow
         * global state variables. 
         */
        rval = htable_search( p_ht, mesh_class_short_name, ENTER_MERGE, 
                              &p_hte );
        p_mocd = NEW( MO_class_data, "New elem geom table entry" );
        
        p_mocd->mesh_id = i;
        griz_str_dup( &p_mocd->short_name, mesh_class_short_name );
        griz_str_dup( &p_mocd->long_name, mesh_class_long_name );
        p_mocd->superclass = G_MESH;
        p_mocd->elem_class_index = -1;
        p_mocd->simple_start = 1;
        p_mocd->simple_stop = 1;
        p_mocd->qty = 1;
        
        p_hte->data = (void *) p_mocd;
        
        /*
         * ExodusII doesn't support G_MAT as a class for data output,
         * but create it anyway since Griz may rely on it as a necessary 
         * part of a mesh definition.
         */

        /* 
         * Material class - Exodus element blocks appear to correspond
         * most accurately to Dyna/Nike materials.
         */
        rval = htable_search( p_ht, mat_class_name, ENTER_MERGE, &p_hte );
        p_mocd = NEW( MO_class_data, "New elem geom table entry" );
        
        p_mocd->mesh_id = i;
        griz_str_dup( &p_mocd->short_name, mat_class_name );
        griz_str_dup( &p_mocd->long_name, mat_class_name );
        p_mocd->superclass = G_MAT;
        p_mocd->elem_class_index = -1;
        p_mocd->simple_start = 1;
        p_mocd->simple_stop = elem_blk_qty;
        p_mocd->qty = elem_blk_qty;
        p_matd = NEW_N( Material_data, elem_blk_qty, "Material data array" );
        p_mocd->objects.materials = p_matd;
        gen_material_data( p_mocd, p_md );
        
        p_hte->data = (void *) p_mocd;
    }

    return OK;
}


/************************************************************
 * TAG( exodus_db_get_geom )
 *
 * Load the geometry (mesh definition) data from an Exodus
 * family.
 */
extern int
exodus_db_get_geom( int dbid, Mesh_data **p_mtable, int *p_mesh_qty )
{
    int status;
    int qty_classes;
    Hash_table *p_ht;
    Htable_entry *p_hte;
    Mesh_data *p_md;
    MO_class_data *p_mocd;
    MO_class_data **p_mo_classes;
    MO_class_data **mo_classes;
    GVec3D *nodes3d;
    GVec2D *nodes2d;
    int i, j, k;
    int sclass;
    Elem_data *p_ed;
    int *mat, *part, *nodes;
    int first, last;
    List_head *p_lh;
    int node_count;
    
    if ( *p_mtable != NULL )
    {
        popup_dialog( WARNING_POPUP,
                      "Mesh table pointer not NULL at initialization." );
        return 1;
    }
    
    for ( i = 0; i < mesh_qty; i++ )
    {
        /* Allocate a hash table for the mesh geometry. */
        p_md = p_mesh_array + i;
        p_ht = p_md->class_table;
        
        /* 
         * Load nodal coordinates. 
         */

        htable_search( p_ht, node_class_short_name, FIND_ENTRY, &p_hte );
        p_mocd = (MO_class_data *) p_hte->data;

        /* Allocate/init coordinate buffers. */
        node_x = NEW_N( float, node_qty * dims, "Temp mesh node coord arrays" );
        node_y = node_x + node_qty;

        /* Allocate permanent mesh initial nodal coordinate array. */
        if ( dims == 3 )
        {
            nodes3d = NEW_N( GVec3D, node_qty, "3D node coord array" );
            p_mocd->objects.nodes3d = nodes3d;
            node_z = node_y + node_qty;
        }
        else
        {
            nodes2d = NEW_N( GVec2D, node_qty, "2D node coord array" );
            p_mocd->objects.nodes2d = nodes2d;
        }
        
        status = ex_get_coord( dbid, (void *) node_x, (void *) node_y, 
                               (void *) node_z );
        if ( status < 0 )
        {
            free( node_x );
            free( p_mocd->objects.nodes3d );
            
            return status;
        }
        
        /* Interleave the coordinates into node class memory. */
        interleave_node_vals( p_mocd->objects.nodes );
            
        /* Load connectivities, material numbers, and part numbers. */
        for ( j = 0; j < elem_blk_qty; j++ )
        {
            p_mocd = elem_block_data[j].p_class;
            sclass = p_mocd->superclass;

            if ( sclass == G_UNIT )
                continue;
            
            /* Allocate structures if first block for this class. */
            if ( p_mocd->objects.elems == NULL )
            {
                p_ed = NEW( Elem_data, "New element data struct" );
                p_ed->nodes = NEW_N( int, p_mocd->qty * qty_connects[sclass],
                                     "Elem class connectivities" );
                p_ed->mat = NEW_N( int, p_mocd->qty, "Elem class materials" );
                p_ed->part = NEW_N( int, p_mocd->qty, "Elem class parts" );
                p_mocd->objects.elems = p_ed;
            }
            else
                p_ed = p_mocd->objects.elems;
            
            /*
             * Load the connectivities into the correct location. 
             * Note that these are 1-based and still need to be
             * decremented for consistency with Griz's interpretation.
             */
            status = ex_get_elem_conn( dbid, elem_block_data[j].block_id,
                                       p_ed->nodes + qty_connects[sclass]
                                       * elem_block_data[j].class_id_base );

            if ( status < 0 )
                return status;
            
            /* Init material and part for elements in the block. */
            mat = p_ed->mat;
            part = p_ed->part;
            first = elem_block_data[j].class_id_base;
            last = first + elem_block_data[j].block_size;
            for ( k = first; k < last; k++ )
            {
                mat[k] = j;
                part[k] = j;
            }
        }

        /* Prepare for class-specific initializations. */
        qty_classes = htable_get_data( p_ht, (void ***) &p_mo_classes );
        
        for ( j = 0; j < qty_classes; j++ )
        {
            p_mocd = p_mo_classes[j];
            
            /* Add reference in the classes-by-superclass list. */
            p_lh = p_md->classes_by_sclass + p_mocd->superclass;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *,
                                  mo_classes, p_lh->qty, 1, 
                                  "Extend sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;

            /* Superclass-specific actions. */
            switch ( p_mocd->superclass )
            {
                case G_HEX:
                case G_TET:
                case G_QUAD:
                case G_TRI:
                case G_BEAM:
                case G_TRUSS:
#ifdef HAVE_WEDGE_PYRAMID
                case G_WEDGE:
                case G_PYRAMID:
#endif
                    /* Decrement node id's to put on zero base. */
                    nodes = p_mocd->objects.elems->nodes;
                    node_count = p_mocd->qty * qty_connects[p_mocd->superclass];
                    for ( k = 0; k < node_count; k++ )
                        nodes[k]--;
                    
                    break;
                default:
                    /* do nothing */;
            }

            /* Allocate the data buffer for scalar I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, p_mocd->qty, 
                                         "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );
        }
        
        free( p_mo_classes );
    }

    /* Pass back address of geometry hash table array. */
    *p_mtable = p_mesh_array;
    *p_mesh_qty = mesh_qty;

    return (OK);
}


/************************************************************
 * TAG( identify_exodus_superclass )
 *
 * Assign a Griz superclass for an ExodusII element type.  A
 * superclass is always assigned (G_UNIT by default) so that
 * unidentified and unsupported types can be handled if only
 * in a rudimentary fashion.
 * 
 * Attempt to identify superclass first by evaluating the
 * first three characters of the Exodus II element type, which
 * should be unique.  Fall back on determination based on the
 * number of nodes (could do this first, but I'm not clear 
 * about some of the Exodus element types, so this may be 
 * less reliable than the text analysis).
 */
static void 
identify_exodus_superclass( char *elem_type, int qty_nodes, int *superclass )
{
    int sclass;
    
    /* Default. */
    sclass = G_UNIT;

    /* Just update the ones Griz supports. */
    switch ( elem_type[0] )
    {
        case 'H':
            if ( elem_type[1] == 'E' && elem_type[2] == 'X' )
                if ( qty_nodes == 8 )
                    sclass = G_HEX;
            break;
            
        case 'S':
            if ( elem_type[1] == 'H' && elem_type[2] == 'E' )
                if ( qty_nodes == 4 )
                    sclass = G_QUAD;
            break;
            
        case 'B':
            if ( elem_type[1] == 'E' && elem_type[2] == 'A' )
                if ( qty_nodes == 3 )
                    sclass = G_BEAM;
            break;
            
        case 'T':
            if ( elem_type[1] == 'E' )
            {
                if ( elem_type[2] == 'T' )
                    if ( qty_nodes == 4 )
                        sclass = G_TET;
            }
            else if ( elem_type[1] == 'R' )
            {
                if ( elem_type[2] == 'I' )
                {
                    if ( qty_nodes == 3 )
                        sclass = G_TRI;
                }
                else if ( elem_type[2] == 'U' )
                {
                    if ( qty_nodes == 3 )
                        sclass = G_BEAM;
                    else if ( qty_nodes == 2 )
                        sclass = G_TRUSS;
                }
            }
            break;
            
        case 'Q':
            if ( elem_type[1] == 'U' && elem_type[2] == 'A' )
                if ( qty_nodes == 4 )
                    sclass = G_QUAD;
            break;
            
        default:
            /* 
             * Couldn't identfy explicitly by the ExodusII type
             * name, but a few types can be unambiguously 
             * identified by the node quantity (and maybe mesh
             * dimensionality).
             */
            switch ( qty_nodes )
            {
                case 2:
                    sclass = G_TRUSS;
                    break;
                    
                case 4:
                    if ( dims == 2 )
                        sclass = G_QUAD;
                    break;
                    
                case 8:
                    sclass = G_HEX;
                    break;
            }

            break;
    }
    
    *superclass = sclass;
}


/************************************************************
 * TAG( gen_material_data )
 *
 * Generate by-material data.  This is trivial for ExodusII
 * db's as long as we continue to equate element blocks
 * with materials (which Larry Schoof of Sandia says is _not_
 * the intent of element blocks but is how the analysis code
 * developers have been using them anyway).
 */
static void
gen_material_data( MO_class_data *p_mat_class, Mesh_data *p_mesh )
{
    Material_data *p_mtld;
    int i;
    
    p_mtld = p_mat_class->objects.materials;

    for ( i = 0; i < elem_blk_qty; i++ )
    {
        p_mtld[i].elem_block_qty = 1;
        p_mtld[i].elem_blocks = NEW( Int_2tuple, "Material elem block" );
        p_mtld[i].elem_blocks[0][0] = elem_block_data[i].class_id_base;
        p_mtld[i].elem_blocks[0][1] = p_mtld[i].elem_blocks[0][0]
                                      + elem_block_data[i].block_size - 1;
        p_mtld[i].elem_class = elem_block_data[i].p_class;
    }
}


/************************************************************
 * TAG( gen_state_description )
 *
 * Generate a description of the ExodusII db contents as a
 * state record format.
 */
static int
gen_state_description( int dbid )
{
    char **p_global_names, **p_nodal_names, **p_elem_names;
    char *global_name_data, *nodal_name_data, *elem_name_data;
    int max_name_len;
    int qty_subrecs, subrec_svar_qty, qty_elem_subrecs;
    int i, j;
    int status;
    Hash_table *var_ht;
    Htable_entry *p_hte;
    int *truth_table;
    int table_size;
    Bool_type *bound_flags;
    Subrecord *p_subrecs;
    Subrecord *p_subr;
    MO_class_data *p_class;
    List_head *p_block_list;
    int *mo_block;
    Subrecord empty_subrec = 
    {
        NULL, -1, 0, NULL, NULL, 0, 0, NULL
    };
    List_head empty_list_head = 
    {
        0, NULL
    };
    char cbuf[64];
    char *subrec_name_fmt = "%s%02d";
    
    /*
     * ExodusII does not permit variations in output variables
     * over time, so there'll only be one output format.
     */
    
    max_name_len = MAX_STR_LENGTH + 1;
    
    /* Get quantities of state variables in the db. */
    status = ex_get_var_param( dbid, "g", &qty_global_vars );
    if ( status < 0 )
        return status;
    else if ( qty_global_vars > 0 )
    {
        p_global_names = NEW_N( char *, qty_global_vars, "Global name ptrs" );
        global_name_data = NEW_N( char, qty_global_vars * max_name_len,
                                  "Global name array" );
        for ( i = 0; i < qty_global_vars; i++ )
            p_global_names[i] = global_name_data + i * max_name_len;
        status = ex_get_var_names( dbid, "g", qty_global_vars, p_global_names );
        
        /* Allocate an input buffer for global data. */
        global_data_buffer = NEW_N( float, qty_global_vars, 
                                    "Exodus global data buffer" );
    }
    if ( status < 0 )
        return status;

    status = ex_get_var_param( dbid, "n", &qty_nodal_vars );
    if ( status < 0 )
        return status;
    else if ( qty_nodal_vars > 0 )
    {
        p_nodal_names = NEW_N( char *, qty_nodal_vars, "Nodal name ptrs" );
        nodal_name_data = NEW_N( char, qty_nodal_vars * max_name_len,
                                  "Nodal name array" );
        for ( i = 0; i < qty_nodal_vars; i++ )
            p_nodal_names[i] = nodal_name_data + i * max_name_len;
        status = ex_get_var_names( dbid, "n", qty_nodal_vars, p_nodal_names );
    }
    if ( status < 0 )
        return status;

    status = ex_get_var_param( dbid, "e", &qty_elem_vars );
    if ( status < 0 )
        return status;
    else if ( qty_elem_vars > 0 )
    {
        p_elem_names = NEW_N( char *, qty_elem_vars, "Elem name ptrs" );
        elem_name_data = NEW_N( char, qty_elem_vars * max_name_len,
                                "Elem name array" );
        for ( i = 0; i < qty_elem_vars; i++ )
            p_elem_names[i] = elem_name_data + i * max_name_len;
        status = ex_get_var_names( dbid, "e", qty_elem_vars, p_elem_names );
    }
    if ( status < 0 )
        return status;

    /*
     * The following conditions imply the existence of a subrecord:
     *   global variables exist;
     *   nodal variables exist;
     *   each unique set of element variables shared by one or more
     *     element blocks associated with the same element class;
     */
    
    qty_subrecs = 0;
    p_subrecs = NULL;
    subrec_var_htables = NULL;
    
    if ( qty_global_vars > 0 )
    {
        p_subrecs = RENEWC_N( Subrecord, p_subrecs, qty_subrecs, 1,
                              "Extend subrecord object array" );
        
        /* Create hash table to map var names to indices. */
        subrec_var_htables = RENEW_N( Hash_table *, subrec_var_htables, 
                                      qty_subrecs, 1, "Subrec var htable ptr" );
        var_ht = htable_create( 151 );
        subrec_var_htables[qty_subrecs] = var_ht;

        /* Initialize. */
        p_subrecs[qty_subrecs] = empty_subrec;

        p_subr = p_subrecs + qty_subrecs;
        qty_subrecs++;
        
        /* Load the Subrecord struct. */
        sprintf( cbuf, subrec_name_fmt, mesh_class_long_name, qty_subrecs );
        griz_str_dup( &p_subr->name, cbuf );
        p_subr->organization = OBJECT_ORDERED;
        p_subr->qty_svars = qty_global_vars;
        p_subr->svar_names = p_global_names;
        p_subr->svar_names = NEW_N( char *, qty_global_vars, "Elem name ptrs" );
        for ( j = 0; j < qty_global_vars; j++ )
        {
            griz_str_dup( &p_subr->svar_names[j], p_global_names[j] );
            htable_search( var_ht, p_global_names[j], ENTER_UNIQUE, &p_hte );
            p_hte->data = (void *) (j + 1);
        }
        griz_str_dup( &p_subr->class_name, mesh_class_short_name );
        p_subr->qty_objects = 1;
        p_subr->qty_blocks = 1;
        p_subr->mo_blocks = NEW_N( int, 2, "Global subrecord object block" );
        p_subr->mo_blocks[0] = 1;
        p_subr->mo_blocks[1] = 1;
        
        free( p_global_names );
        free( global_name_data );
    }
        
    if ( qty_nodal_vars > 0 )
    {
        p_subrecs = RENEWC_N( Subrecord, p_subrecs, qty_subrecs, 1,
                              "Extend subrecord object array" );
        
        /* Create hash table to map var names to indices. */
        subrec_var_htables = RENEW_N( Hash_table *, subrec_var_htables, 
                                      qty_subrecs, 1, "Subrec var htable ptr" );
        var_ht = htable_create( 151 );
        subrec_var_htables[qty_subrecs] = var_ht;
        
        /* Initialize. */
        p_subrecs[qty_subrecs] = empty_subrec;
        
        p_subr = p_subrecs + qty_subrecs;
        qty_subrecs++;
        
        /* Load the Subrecord struct. */
        sprintf( cbuf, subrec_name_fmt, node_class_long_name, qty_subrecs );
        griz_str_dup( &p_subr->name, cbuf );
        p_subr->organization = RESULT_ORDERED;
        p_subr->qty_svars = qty_nodal_vars;
        p_subr->svar_names = NEW_N( char *, qty_nodal_vars, "Elem name ptrs" );
        for ( j = 0; j < qty_nodal_vars; j++ )
        {
            griz_str_dup( &p_subr->svar_names[j], p_nodal_names[j] );
            htable_search( var_ht, p_nodal_names[j], ENTER_UNIQUE, &p_hte );
            p_hte->data = (void *) (j + 1);
        }
        griz_str_dup( &p_subr->class_name, node_class_short_name );
        p_subr->qty_objects = node_qty;
        p_subr->qty_blocks = 1;
        p_subr->mo_blocks = NEW_N( int, 2, "Nodal subrecord object block" );
        p_subr->mo_blocks[0] = 1;
        p_subr->mo_blocks[1] = node_qty;
        
        free( p_nodal_names );
        free( nodal_name_data );
    }

    if ( qty_elem_vars > 0 )
    {
        /* Read the element variable truth table. */
        table_size = elem_blk_qty * qty_elem_vars;
        truth_table = NEW_N( int, table_size, "Elem var truth table" );
        status = ex_get_elem_var_tab( dbid, elem_blk_qty, qty_elem_vars,
                                      truth_table );
        if ( status < 0 )
            return status;

        /* 
         * For each class, create a subrecord for each set of element
         * blocks that has a unique set of state variables.  Maintain a 
         * record of all blocks that make up each subrecord for later use 
         * in result input.
         *
         * In code and comments below, we must differentiate between
         * ExodusII's "element blocks" and the "mesh object blocks"
         * used in the Subrecord struct to identify objects bound to
         * the subrecord.  They both identify ranges of objects, but
         * they have different applications.
         */
        bound_flags = NEW_N( Bool_type, elem_blk_qty, "Elem block bools" );
        qty_elem_subrecs = 0;
        non_elem_subrec_qty = qty_subrecs;
        nonproper_class_qty = 0;
        nonproper_classes = NEW_N( MO_class_data *, 
                                   p_mesh_array->elem_class_qty,
                                   "Nonproper class list" );
        
        for ( i = 0; i < elem_blk_qty; i++ )
        {
            if ( bound_flags[i] )
                continue;
            else
                bound_flags[i] = TRUE;
            
            /* An unbound block begins a new subrecord definition. */
            p_subrecs = RENEWC_N( Subrecord, p_subrecs, qty_subrecs, 1,
                                  "Extend subrecord object array" );
        
            /* Create hash table to map var names to indices. */
            subrec_var_htables = RENEW_N( Hash_table *, subrec_var_htables, 
                                          qty_subrecs, 1, 
                                          "Subrec var htable ptr" );
            var_ht = htable_create( 151 );
            subrec_var_htables[qty_subrecs] = var_ht;

            /* Clear. */
            p_subrecs[qty_subrecs] = empty_subrec;
            
            p_class = elem_block_data[i].p_class;
            
            /*
             * Initialize the Subrecord struct. 
             */

            p_subr = p_subrecs + qty_subrecs;
            qty_subrecs++;
            p_subr->organization = RESULT_ORDERED;
            
            sprintf( cbuf, subrec_name_fmt, p_class->long_name, qty_subrecs );
            griz_str_dup( &p_subr->name, cbuf );
            
            /* Increment subrecord count. */
            
            /* Build name list by checking truth table. */
            subrec_svar_qty = 0;
            for ( j = 0; j < qty_elem_vars; j++ )
            {
                if ( truth_table[i * qty_elem_vars + j] )
                {
                    p_subr->svar_names = RENEW_N( char *, 
                                                  p_subr->svar_names,
                                                  subrec_svar_qty, 1,
                                                  "New subrec svar name" );
                    griz_str_dup( p_subr->svar_names + subrec_svar_qty,
                                  p_elem_names[j] );
                    subrec_svar_qty++;
                    
                    /* Save variable index. */
                    htable_search( var_ht, p_elem_names[j], ENTER_UNIQUE, 
                                   &p_hte );
                    p_hte->data = (void *) (j + 1);
                }
            }
            p_subr->qty_svars = subrec_svar_qty;

            griz_str_dup( &p_subr->class_name, p_class->short_name );
            p_subr->qty_objects = elem_block_data[i].block_size;
            p_subr->qty_blocks = 1;
            p_subr->mo_blocks = NEW_N( int, 2, 
                                       "Elem subrecord object block" );
            p_subr->mo_blocks[0] = 1;
            p_subr->mo_blocks[1] = elem_block_data[i].block_size;
            
            /*
             * Initiate a list of ExodusII element blocks which will
             * effectively be bound to the new subrecord.
             */
            
            /* Create new list for this subrecord's bound elem blocks. */
            subrec_block_lists = RENEWC_N( List_head, subrec_block_lists,
                                           qty_elem_subrecs, 1,
                                           "New subrec block list" );
            subrec_block_lists[qty_elem_subrecs] = empty_list_head;

            /* Save reference to current subrecord's elem block list. */
            p_block_list = subrec_block_lists + qty_elem_subrecs;
            qty_elem_subrecs++;
            
            /* Add elem block to elem block list. */
            p_block_list->list = RENEW_N( int, p_block_list->list,
                                          p_block_list->qty, 1,
                                          "New block list entry" );
            ((int *) p_block_list->list)[p_block_list->qty] = i;
            p_block_list->qty++;
            
            /* 
             * Search rest of elem blocks for one with a matching set of 
             * variables and that belongs to the same class.
             */

            for ( j = i + 1; j < elem_blk_qty; j++ )
            {
                if ( elem_block_data[j].p_class 
                     != elem_block_data[i].p_class )
                    continue;
                
                if ( bool_compare_array( qty_elem_vars, 
                                         truth_table + i * qty_elem_vars,
                                         truth_table + j * qty_elem_vars ) )
                {
                    /* 
                     * Same set of variables, same class -> add to subrec. 
                     */
                    
                    p_subr->qty_objects += elem_block_data[j].block_size;

                    /* Get last mesh object block from subrecord. */
                    mo_block = p_subr->mo_blocks + (p_subr->qty_blocks - 1) * 2;
                    
                    /* Try to merge new idents into last subrec mo block. */
                    if ( mo_block[1] + 1 == elem_block_data[j].class_id_base )
                        mo_block[1] += elem_block_data[j].block_size;
                    else
                    {
                        /* Can't merge, so append a new mo block. */
                        p_subr->mo_blocks = RENEW_N( int, p_subr->mo_blocks,
                                                     p_subr->qty_blocks * 2, 2,
                                                     "Addl elem subr obj blk" );
                        mo_block = p_subr->mo_blocks + p_subr->qty_blocks * 2;
                        mo_block[0] = elem_block_data[j].class_id_base;
                        mo_block[1] = mo_block[0] 
                                      + elem_block_data[j].block_size - 1;
                        p_subr->qty_blocks++;
                    }

                    /* Element block is now bound to a subrecord def. */
                    bound_flags[j] = TRUE;
                    
                    /* Add elem block to bound elem block list. */
                    p_block_list->list = RENEW_N( int, p_block_list->list,
                                                  p_block_list->qty, 1,
                                                  "New block list entry" );
                    ((int *) p_block_list->list)[p_block_list->qty] = j;
                    p_block_list->qty++;
                }
                else
                {
                    /* 
                     * Different set of variables, so all subrecords of this
                     * class will be nonproper since elements can only
                     * appear in one block and a block will only bind to
                     * one subrecord.  Indicate by saving class pointer 
                     * in a list for that purpose.
                     */
                    nonproper_classes[nonproper_class_qty++] = p_class;
                }
            }
        }
        
        free( bound_flags );
        free( truth_table );
        
        free( p_elem_names );
        free( elem_name_data );
    }
    
    format_subrecs = p_subrecs;
    format_subrec_qty = qty_subrecs;
    
    return OK;
}


/************************************************************
 * TAG( exodus_db_get_st_descriptors )
 *
 * Generate a description of the ExodusII db contents as a
 * state record format.
 */
extern int
exodus_db_get_st_descriptors( Analysis *analy, int dbid )
{
    State_rec_obj *p_sro;
    int qty_svars;
    int qty_subrecs;
    int i, j;
    int rval;
    Hash_table *p_sv_ht, *p_primal_ht;
    Htable_entry *p_hte;
    Subrec_obj *p_so;
    Subrecord *p_s;
    int mesh_node_qty;
    int *node_work_array;
    int disp_cnt, vel_cnt;
    char **svar_names;
    Bool_type nodal;

    /* Create the state variable and primal result hash tables. */

    qty_svars = qty_global_vars + qty_nodal_vars + qty_elem_vars;

    if ( qty_svars > 0 )
    {
        if ( qty_svars < 100 )
        {
            p_sv_ht = htable_create( 151 );
            p_primal_ht = htable_create( 151 );
        }
        else
        {
            p_sv_ht = htable_create( 1009 );
            p_primal_ht = htable_create( 1009 );
        }
    }
    else
        return OK;

    /* Allocate array of Subrec_obj structs and copy Subrecord structs. */
    qty_subrecs = format_subrec_qty;
    p_sro = NEW( State_rec_obj, "Exodus state rec format" );
    p_sro->qty = qty_subrecs;
    p_sro->subrecs = NEW_N( Subrec_obj, qty_subrecs, "Exodus subrecs" );
    p_so = p_sro->subrecs;
    
    for ( i = 0; i < qty_subrecs; i++ )
    {
        p_s = &p_so[i].subrec;
        griz_str_dup( &p_s->name, format_subrecs[i].name );
        p_s->organization = format_subrecs[i].organization;
        p_s->qty_svars = format_subrecs[i].qty_svars;
        p_s->svar_names = NEW_N( char *, p_s->qty_svars, "Subr svar nam ptrs" );
        for ( j = 0; j < p_s->qty_svars; j++ )
            griz_str_dup( p_s->svar_names + j, format_subrecs[i].svar_names[j] );
        griz_str_dup( &p_s->class_name, format_subrecs[i].class_name );
        p_s->qty_objects = format_subrecs[i].qty_objects;
        p_s->qty_blocks = format_subrecs[i].qty_blocks;
        p_s->mo_blocks = NEW_N( int, p_s->qty_blocks * 2, "Subr mo blocks" );
        for ( j = 0; j < 2 * p_s->qty_blocks; j++ )
            p_s->mo_blocks[j] = format_subrecs[i].mo_blocks[j];
    }

    status_var_index = NEW_N( int, qty_subrecs, "Subr status var index array" );
        
    /* Finish initializing Subrec_obj structs.  */
    
    mesh_node_qty = analy->mesh_table[0].node_geom->qty;
    node_work_array = NEW_N( int, mesh_node_qty, "Temp node array" );

    for ( i = 0; i < qty_subrecs; i++ )
    {
        p_s = &p_so[i].subrec;

        /* Get the mesh object class pointer. */
        rval = htable_search( analy->mesh_table[0].class_table, 
                              p_s->class_name, FIND_ENTRY, &p_hte );
        p_so[i].p_object_class = (MO_class_data *) p_hte->data;
        
        /* If class is on the non-proper list, generate object ids array. */
        for ( j = 0; j < nonproper_class_qty; j++ )
            if ( p_so[i].p_object_class == nonproper_classes[j] )
                break;
        
        if ( j < nonproper_class_qty )
        {
            p_so[i].object_ids = NEW_N( int, p_s->qty_objects,
                                        "Subrec ident map array" );
            blocks_to_list( p_s->qty_blocks, p_s->mo_blocks,
                            p_so[i].object_ids, TRUE );
        }
        
        /* Generate referenced nodes list. */
        create_subrec_node_list( node_work_array, mesh_node_qty, 
                                 p_so + i );
        
        /* 
         * M_NODE class "node" is special - need it for node positions
         * and velocities.
         */
        if ( strcmp( p_s->class_name, "node" ) == 0 )
            nodal = TRUE;
        else
            nodal = FALSE;

        /* 
         * Loop over svars and create state variable and primal result
         * table entries.
         */
        disp_cnt = 0;
        vel_cnt = 0;
        svar_names = p_s->svar_names;
        for ( j = 0; j < p_s->qty_svars; j++ )
        {
            exodus_create_st_variable( dbid, p_sv_ht, svar_names[j] );
            create_primal_result( analy->mesh_table, 0, i, p_so + i, 
                                  p_primal_ht, 1, svar_names[j], p_sv_ht );

            if ( nodal )
            {
                if ( strcmp( svar_names[j], "DISPLX" ) == 0 )
                {
                    disp_cnt++;
                    node_x_index = j + 1;
                }
                else if ( strcmp( svar_names[j], "DISPLY" ) == 0 )
                {
                    disp_cnt++;
                    node_y_index = j + 1;
                }
                else if ( strcmp( svar_names[j], "DISPLZ" ) == 0 )
                {
                    disp_cnt++;
                    node_z_index = j + 1;
                }
                else if ( strcmp( svar_names[j], "VELX" ) == 0
                          || strcmp( svar_names[j], "VELY" ) == 0
                          || strcmp( svar_names[j], "VELZ" ) == 0 )
                    vel_cnt++;
            }
            else if ( strcmp( svar_names[j], "STATUS" ) == 0 )
            {
                htable_search( subrec_var_htables[i], "STATUS", FIND_ENTRY,
                               &p_hte );
                status_var_index[i] = (int) p_hte->data;
            }
        }

        if ( disp_cnt == dims )
            p_sro->node_pos_subrec = i;
        if ( vel_cnt == dims )
            p_sro->node_vel_subrec = i;
    }
    
    free( node_work_array );
    
    /* 
     * Return the subrecord tree,state variable hash table, and primal result
     * hash table. 
     */
    analy->srec_tree = p_sro;
    analy->qty_srec_fmts = 1;
    analy->st_var_table = p_sv_ht;
    analy->primal_results = p_primal_ht;
    
    return OK;
}


/************************************************************
 * TAG( exodus_create_st_variable )
 *
 * Create State_variable table entries for a db.
 */
static int 
exodus_create_st_variable( int dbid, Hash_table *p_sv_ht, char *p_name )
{
    int rval;
    Hash_action op;
    Htable_entry *p_hte;
    State_variable *p_sv;
    
    /* Only enter svar if not already present in table. */
    if( strcmp( p_name, "sand" ) == 0 )
        op = ENTER_ALWAYS;
    else
       op = ENTER_UNIQUE;

    rval = htable_search( p_sv_ht, p_name, op, &p_hte );
    
    /* If this is a new entry in the state variable table... */
    if ( rval == OK )
    {
        /* Create the State_variable and store it in the hash table entry. */
        p_sv = NEW( State_variable, "New state var" );
        griz_str_dup( &p_sv->short_name, p_name );
        griz_str_dup( &p_sv->long_name, p_name );
        p_sv->num_type = M_FLOAT;
        p_sv->agg_type = SCALAR;
        p_hte->data = (void *) p_sv;
    }

    return OK; /* Why not rval? */
}


/************************************************************
 * TAG( exodus_db_get_state )
 *
 * Seek to a particular state in an Exodus database and 
 * update nodal positions for the mesh.
 */
extern int
exodus_db_get_state( Analysis *analy, int state_no, State2 *p_st, 
                     State2 **pp_new_st, int *state_qty )
{
    int st_qty;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrecs, *p_subrec;
    int i, j, k;
    int dbid;
    int srec_id;
    int ec_index;
    int st;
    Mesh_data *p_md;
    float *orig_nodes, *cur_nodes;
    int blk_cnt;
    int *p_eblks;
    Elem_block_data *p_ebd;
    float *p_sand;
    int elem_qty, limit;
    float fdum;
    char cdum[1];
    int status;

    dbid = analy->db_ident;

    /* See if database has grown since initial open. */
    status = ex_inquire( dbid, EX_INQ_TIME, &st_qty, &fdum, cdum );
    if ( st_qty > qty_states )
    {
        /* Update times array. */
        state_times = RENEW_N( float, state_times, qty_states, 
                               st_qty - qty_states,
                               "Extend state times array" );
        
        for ( i = qty_states; i < st_qty; i++ )
            status = ex_get_time( dbid, i + 1, state_times + i );
        
        qty_states = st_qty;
    }
    
    /* Pass back the current quantity of states in the db. */
    if ( state_qty != NULL )
        *state_qty = qty_states;
    
    p_md = analy->mesh_table;
    
    if ( qty_states == 0 )
    {
        if ( p_st == NULL )
            p_st = mk_state2( analy, NULL, dims, 0, qty_states, p_st );
        
        /* No states, so use node positions from geometry definition. */
        p_st->nodes = p_md->node_geom->objects;
        
        p_st->position_constant = TRUE;
 
        *pp_new_st = p_st;

        return OK;
    }

    if ( state_no < 0 || state_no >= qty_states )
    {
        popup_dialog( WARNING_POPUP, 
                      "Get-state request for nonexistent state." );
        
        *pp_new_st = p_st;

        return GRIZ_FAIL;
    }
    else
        st = state_no + 1;

    srec_id = 0;
    p_sro = analy->srec_tree + srec_id;
    p_subrecs = p_sro->subrecs;
    
    /* Update or create State2 struct. */
    p_st = mk_state2( analy, p_sro, dims, srec_id, qty_states, p_st );
    
    p_st->state_no = state_no;
    
    p_st->time = state_times[state_no];

    /* Read node position arrays if they exist. */
    if ( p_sro->node_pos_subrec != -1 )
    {
        /* Read node displacement arrays. */
        status = ex_get_nodal_var( dbid, st, node_x_index, node_qty, node_x );
        status = ex_get_nodal_var( dbid, st, node_y_index, node_qty, node_y );
        
        if ( dims == 3 )
            status = ex_get_nodal_var( dbid, st, node_z_index, node_qty, 
                                       node_z );
        
        interleave_node_vals( p_st->nodes.nodes );
        
        /* Convert displacements to positions. */
        orig_nodes = analy->mesh_table[0].node_geom->objects.nodes;
        cur_nodes = p_st->nodes.nodes;
        limit = dims * node_qty;
        for ( i = 0; i < limit; i++ )
            cur_nodes[i] += orig_nodes[i];
    }

    /* Read sand flags if they exist. */
    for ( i = non_elem_subrec_qty; i < p_sro->qty; i++ )
    {
        if ( p_subrecs[i].sand )
        {
            /*
             * Don't need to pay attention to object_ids because
             * Exodus elements will always be in order within the
             * block and we can just use the class_id_base to locate 
             * where in the sand data array to write flag values.
             */

            p_subrec = p_subrecs + i;
            ec_index = p_subrec->p_object_class->elem_class_index;
            blk_cnt = subrec_block_lists[i - non_elem_subrec_qty].qty;
            p_eblks = (int *) subrec_block_lists[i - non_elem_subrec_qty].list;

            for ( j = 0; j < blk_cnt; j++ )
            {
                p_ebd = elem_block_data + p_eblks[j];
                p_sand = p_st->elem_class_sand[ec_index] + p_ebd->class_id_base;
                elem_qty = p_ebd->block_size;
                ex_get_elem_var( dbid, st, status_var_index[i], p_ebd->block_id,
                                 elem_qty, (void *) p_sand );
                
                /* Need to toggle the values for interpretation by Griz. */
                for ( k = 0; k < elem_qty; k++ )
                    if ( p_sand[k] > 0.0 )
                        p_sand[k] = 0.0;
                    else
                        p_sand[k] = 1.0;
            }
        }
    }

    /* If nodal positions weren't part of state data, get from geometry. */
    if ( p_sro->node_pos_subrec == -1 )
    {
        p_st->nodes = p_md->node_geom->objects;
        
        p_st->position_constant = TRUE;
    }
    
    *pp_new_st = p_st;
    
    return OK;
}


/************************************************************
 * TAG( exodus_db_get_subrec_def )
 *
 * Fill a Subrecord structure for a Exodus database.
 */
extern int
exodus_db_get_subrec_def( int dbid, int srec_id, int subrec_id, 
                          Subrecord *p_subrec )
{
    Subrecord *p_s;
    int j;

    if ( srec_id != 0 )
        return INVALID_SREC_INDEX;
    
    if ( subrec_id < 0 || subrec_id  > format_subrec_qty - 1 )
        return INVALID_SUBREC_INDEX;
    
    /* Get pointer to source Subrecord. */
    p_s = format_subrecs + subrec_id;

    griz_str_dup( &p_subrec->name, p_s->name );
    p_subrec->organization = p_s->organization;
    p_subrec->qty_svars = p_s->qty_svars;
    p_subrec->svar_names = NEW_N( char *, p_subrec->qty_svars, 
                                  "Subr svar nam ptrs" );
    for ( j = 0; j < p_subrec->qty_svars; j++ )
        griz_str_dup( p_subrec->svar_names + j, p_s->svar_names[j] );
    griz_str_dup( &p_subrec->class_name, p_s->class_name );
    p_subrec->qty_objects = p_s->qty_objects;
    p_subrec->qty_blocks = p_s->qty_blocks;
    p_subrec->mo_blocks = NEW_N( int, p_subrec->qty_blocks * 2, 
                                 "Subr mo blocks" );
    for ( j = 0; j < 2 * p_s->qty_blocks; j++ )
        p_subrec->mo_blocks[j] = p_s->mo_blocks[j];

    return OK;
}


/************************************************************
 * TAG( exodus_db_cleanse_subrec )
 *
 * Free dynamically allocated memory for a Subrecord structure.
 * 
 * This logic copied from mc_cleanse_subrec(), and must be kept
 * current with it.  Under AIX's dynamic linking, this code 
 * couldn't resolve Mili symbols without linking in the Mili
 * object files that define them.  It might be possible to 
 * create a file of Mili function wrappers in Griz, and link
 * that object file in building libgex.so, then call the
 * wrapper functions from exo_driver.
 */
extern int
exodus_db_cleanse_subrec( Subrecord *p_subrec )
{
    int i;
    
    free( p_subrec->name );
    free( p_subrec->class_name );
    free( p_subrec->mo_blocks );
    
    for ( i = 0; i < p_subrec->qty_svars; i++ )
        free( p_subrec->svar_names[i] );
    
    free( p_subrec->svar_names );
    
    return OK;
}


/************************************************************
 * TAG( exodus_db_cleanse_state_var )
 *
 * Free dynamically allocated memory from a State_variable struct.
 * 
 * Comment above about mc_cleanse_subrec() applies equally here
 * regarding mc_cleanse_svar().
 */
extern int
exodus_db_cleanse_state_var( State_variable *p_svar )
{
    int i;
    
    free( p_svar->short_name );
    free( p_svar->long_name );
    
    if ( p_svar->agg_type != 0 )
    {
        free( p_svar->dims );
        
        if ( p_svar->agg_type == VEC_ARRAY || p_svar->agg_type == VECTOR )
        {
            for ( i = 0; i < p_svar->vec_size; i++ )
                free( p_svar->components[i] );
        
            free( p_svar->components );
        }
    }
    
    return OK;
}


/************************************************************
 * TAG( exodus_db_get_results )
 *
 * Load primal results from an Exodus database.
 */
extern int
exodus_db_get_results( int dbid, int state, int subrec_id, int qty, 
                       char **results, void *data )
{
    int status;
    int i, j;
    int superclass;
    Subrec_obj *p_subrec;
    float *p_dest, *p_stride_start;
    int res_stride;
    int var_index, list_index;
    int blk_cnt, obj_qty;
    int *p_eblks;
    Elem_block_data *p_ebd;
    Htable_entry *p_hte;
    Hash_table *p_ht;
    
    p_subrec = env.curr_analy->srec_tree[0].subrecs + subrec_id;
    superclass = p_subrec->p_object_class->superclass;
    p_ht = subrec_var_htables[subrec_id];
    
    switch ( superclass )
    {
        case G_UNIT:
        case G_TRUSS:
        case G_BEAM:
        case G_TRI:
        case G_QUAD:
        case G_TET:
        case G_PYRAMID:
        case G_WEDGE:
        case G_HEX:
            res_stride = p_subrec->subrec.qty_objects;
            list_index = subrec_id - non_elem_subrec_qty;
            blk_cnt = subrec_block_lists[list_index].qty;
            p_eblks = (int *) subrec_block_lists[list_index].list;
            p_stride_start = (float *) data;

            /*
             * Loop over results fastest and blocks slowest under the 
             * assumption that it's fastest to read all within a block 
             * before changing blocks.
             */

            for ( i = 0; i < blk_cnt; i++ )
            {
                p_ebd = elem_block_data + p_eblks[i];
                obj_qty = p_ebd->block_size;
                
                for ( j = 0; j < qty; j++ )
                {
                    p_dest = p_stride_start + j * res_stride;
                    
                    htable_search( p_ht, results[j], FIND_ENTRY, &p_hte );
                    var_index = (int) p_hte->data;

                    status = ex_get_elem_var( dbid, state, var_index, 
                                              p_ebd->block_id, obj_qty, 
                                              (void *) p_dest );
                    if ( status < 0 )
                        break;
                }
                
                if ( j < qty )
                    break;
                
                p_stride_start += obj_qty;
            }
            
            break;
            
        case G_NODE:
            p_dest = (float *) data;
            
            for ( i = 0; i < qty; i++ )
            {
                /* Need index of requested var among all nodal vars. */
                htable_search( p_ht, results[i], FIND_ENTRY, &p_hte );
                var_index = (int) p_hte->data;

                status = ex_get_nodal_var( dbid, state, var_index, node_qty, 
                                           p_dest );

                if ( status < 0 )
                    break;

                p_dest += node_qty;
            }
            break;
            
        case G_MESH:
            status = ex_get_glob_vars( dbid, state, qty_global_vars,
                                       global_data_buffer );
            if ( status < 0 )
                break;

            p_dest = (float *) data;
            
            /* Copy requested variables into databuffer. */
            for ( i = 0; i < qty; i++ )
            {
                /* Need index of requested var among all global vars. */
                htable_search( p_ht, results[i], FIND_ENTRY, &p_hte );
                var_index = (int) p_hte->data - 1;
                p_dest[i] = global_data_buffer[var_index];
            }
            break;
    }

    return status;
}


/************************************************************
 * TAG( exodus_db_query )
 *
 * Query an Exodus database.
 */
extern int
exodus_db_query( int dbid, int query_type, void *num_args, char *char_arg,
               void *p_info )
{
    int rval, status;
    int srec_idx, subrec_idx;
    int add;
    int i, j;
    Hash_table *p_ht;
    Htable_entry *p_hte;
    int *int_args;
    float time;
    int qty;
    int *p_i;
    float *p_f;
    int mesh_id;
    int state_no, max_st;
    int idx;
    float fdum;
    char cdum[1];
    
    rval = OK;
   
    /* Most queries expect integer numeric arguments. */ 
    int_args = (int *) num_args;
    
    switch( query_type )
    {
        case QRY_QTY_STATES:
            status = ex_inquire( dbid, EX_INQ_TIME, &qty, &fdum, cdum );
            if ( qty > qty_states )
            {
                /* Update times array. */
                
                state_times = RENEW_N( float, state_times, qty_states, 
                                       qty - qty_states,
                                       "Extend state times array" );
                
                for ( i = qty_states; i < qty; i++ )
                    status = ex_get_time( dbid, i + 1, state_times + i );
                
                qty_states = qty;
            }
            *((int *) p_info) = qty_states;
            break;
        case QRY_QTY_DIMENSIONS:
            *((int *) p_info) = dims;
            break;
        case QRY_QTY_MESHES:
            *((int *) p_info) = 1;
            break;
        case QRY_QTY_SREC_FMTS:
            *((int *) p_info) = 1;
            break;
        case QRY_QTY_SUBRECS:
            rval = calc_subrec_qty( dbid, &qty );
            *((int *) p_info) = qty;
            break;
        case QRY_QTY_SUBREC_SVARS:
            srec_idx = int_args[0];
            if ( srec_idx != 0 )
                rval = (int) INVALID_SREC_INDEX;
            else
            {
                subrec_idx = int_args[1];
                if ( subrec_idx < 0 
                     || subrec_idx > format_subrec_qty - 1 )
                    rval = (int) INVALID_SUBREC_INDEX;
                else
                    *((int *) p_info) = format_subrecs[subrec_idx].qty_svars;
            }
            break;
        case QRY_QTY_SVARS:
            *((int *) p_info) = qty_global_vars + qty_nodal_vars 
                                + qty_elem_vars;
            break;
        case QRY_QTY_NODE_BLKS:
            *((int *) p_info) = 1;
            break;
        case QRY_QTY_NODES_IN_BLK:
            *((int *) p_info) = node_qty;
            break;
        case QRY_QTY_CLASS_IN_SCLASS:
            /* Access the mesh_table even though the app owns it. */
            *((int *) p_info) = 
                p_mesh_array->classes_by_sclass[int_args[1]].qty;
            break;
        case QRY_QTY_ELEM_CONN_DEFS:
            /* Treat each block as a conn def entry. */
            *((int *) p_info) = elem_blk_qty;
            break;
        case QRY_QTY_ELEMS_IN_DEF:
            /* Interpret the index as an index among element blocks. */
            if ( int_args[0] != 0 )
                rval = NO_MESH;
            else if ( int_args[1] < 0 || int_args[1] > elem_blk_qty - 1 )
                rval = INVALID_INDEX;
            else
                *((int *) p_info) = elem_block_data[int_args[1]].block_size;
            break;
        case QRY_SREC_FMT_ID:
            *((int *) p_info) = 0;
            break;
        case QRY_SUBREC_CLASS:
            srec_idx = int_args[0];
            if ( srec_idx != 0 )
                rval = (int) INVALID_SREC_INDEX;
            else
            {
                subrec_idx = int_args[1];
                if ( subrec_idx < 0 || subrec_idx > format_subrec_qty - 1 )
                    rval = (int) INVALID_SUBREC_INDEX;
                else
                    strcpy( (char *) p_info, 
                            format_subrecs[subrec_idx].class_name );
            }
            break;
        case QRY_SREC_MESH:
            *((int *) p_info) = 0;
            break;
        case QRY_CLASS_SUPERCLASS:
            mesh_id = int_args[0];
            if ( mesh_id != 0 )
                rval = NO_MESH;
            else
            {
                p_ht = p_mesh_array[mesh_id].class_table;
                status = htable_search( p_ht, char_arg, FIND_ENTRY, &p_hte );
                if ( status == OK )
                    *((int *) p_info) = 
                        ((MO_class_data *) p_hte->data)->superclass;
                else
                    rval = status;
            }
            break;
        case QRY_STATE_TIME:
            state_no = int_args[0];
            if ( state_no < 1 || state_no > qty_states )
                rval = INVALID_STATE;
            else
                *((float *) p_info) = state_times[state_no - 1];
            break;
        case QRY_SERIES_TIMES:
            state_no = int_args[0];
            max_st = int_args[1];
            if ( state_no < 1 || state_no > qty_states
                 || max_st < 1 || max_st > qty_states )
            {
                rval = INVALID_STATE;
                break;
            }
            state_no--;
            max_st--;
            p_f = (float *) p_info;
            add = ( (max_st - state_no & 0x80000000) == 0 ) ? 1 : -1;
            for ( i = state_no, j = 0; i != max_st; i += add, j++ )
                p_f[j] = state_times[i];
            p_f[j] = state_times[i];
            break;
        case QRY_MULTIPLE_TIMES:
            qty = int_args[0];
            for ( i = 1; i <= qty; i++ )
                if ( int_args[i] < 1 || int_args[i] > qty_states )
                {
                    rval = INVALID_STATE;
                    break;
                }
            if ( rval == OK )
            {
                p_f = (float *) p_info;
                for ( i = 1; i <= qty; i++ )
                    p_f[i - 1] = state_times[int_args[i] - 1];
            }
            break;
        case QRY_SERIES_SREC_FMTS:
            for ( i = 0; i < 2; i++ )
                if ( int_args[i] < 1 || int_args[i] > qty_states )
                {
                    rval = INVALID_STATE;
                    break;
                }
            if ( rval == OK )
            {
                p_i = (int *) p_info;
                max_st = int_args[1];
                for ( i = int_args[0] - 1, idx = 0; i < max_st; i++ )
                    p_i[idx++] = 0;
            }
            break;
        case QRY_STATE_OF_TIME:
            time = ((float *) num_args)[0];
            p_i = (int *) p_info;
            i = 0;
            if ( qty_states > 1 )
            {
                i = 0;
                while ( i < qty_states && time >= state_times[i] )
                    i++;
                if ( i == 0 )
                {
                    /* Time precedes first state time. */
                    rval = INVALID_TIME;
                }
                else if ( i < qty_states )
                {
                    /* The "usual" case, time somewhere in the middle. */
                    p_i[0] = i;
                    p_i[1] = i + 1;
                }
                else if ( time == state_times[i - 1] )
                {
                    /* Time matches last state time. */
                    p_i[0] = i - 1;
                    p_i[1] = i;
                }
                else
                {
                    /* Time exceeds last state time. */
                    rval = INVALID_TIME;
                }
            }
            else if ( qty_states == 1 && time == state_times[0] )
                p_i[0] = p_i[1] = 1;
            else /* qty_states == 0 -> no state data */
                rval = INVALID_TIME;
            break;
        default:
            rval = UNKNOWN_QUERY_TYPE;
    }

    return (int) rval;
}


/************************************************************
 * TAG( exodus_db_get_title )
 *
 * Load the title from an Exodus database.
 */
extern int
exodus_db_get_title( int dbid, char *title_bufr )
{
    int status;
    size_t cpy_len;
    
    status = ex_get_init( dbid, title, &dims, &node_qty, &elem_qty, 
                          &elem_blk_qty, &node_set_qty, &side_set_qty );
    
    if ( status < 0 )
        return status;
    
    cpy_len = strlen( title );

    if ( cpy_len >= G_MAX_STRING_LEN )
        cpy_len = G_MAX_STRING_LEN - 1;

    strncpy( title_bufr, title, cpy_len );
    
    called_ex_get_init = TRUE;
    
    return 0;
}


/************************************************************
 * TAG( exodus_db_get_dimension )
 *
 * Get the mesh dimensionality from an Exodus database.
 */
extern int
exodus_db_get_dimension( int dbid, int *p_dim )
{
    int status;

    if ( !called_ex_get_init )
    {
        status = ex_get_init( dbid, title, &dims, &node_qty, &elem_qty, 
                              &elem_blk_qty, &node_set_qty, &side_set_qty );
        
        if ( status < 0 )
            return status;
        else
            called_ex_get_init = TRUE;
    }
    
    *p_dim = dims;
    
    return 0;
}


/************************************************************
 * TAG( exodus_db_set_buffer_qty )
 *
 * This functionality not supported in Exodus.
 */
extern int
exodus_db_set_buffer_qty( int dbid, int mesh_id, char *class_name, int buf_qty )
{
    return OK;
}


/*****************************************************************
 * TAG( exodus_st_var_delete )
 *
 * Function to clean and delete a State_variable struct.  Designed
 * as a hash table data deletion function for htable_delete().  
 *
 * This function created to replace use of st_var_delete for
 * Exodus db's, since, under AIX, that function's reference to
 * env.curr_analy->db_cleanse_state_var() resulted in an illegal
 * instruction (why???).
 */
static void
exodus_st_var_delete( void *p_state_variable )
{
    exodus_db_cleanse_state_var( (State_variable *) p_state_variable );
    free( p_state_variable );
}


/************************************************************
 * TAG( exodus_db_close )
 *
 * Free resources associated with an Exodus database and close 
 * the database.
 *
 * A lot of this should probably be moved into close_analysis(),
 */
extern int
exodus_db_close( Analysis *analy )
{
    int dbid;
    Subrec_obj *p_subrec;
    State_rec_obj *p_srec;
    Mesh_data *p_mesh;
    int i, j;
    
    dbid = analy->db_ident;
    
    /*
     * Free Griz's structures for managing data from the data base.
     */
     
    /* Free the state record format tree. */
    p_srec = analy->srec_tree;
    for ( i = 0; i < analy->qty_srec_fmts; i++ )
    {
        p_subrec = p_srec[i].subrecs;
	
        for ( j = 0; j < p_srec[i].qty; j++ )
        {
            exodus_db_cleanse_subrec( &p_subrec[j].subrec );
    
            if ( p_subrec[j].object_ids != NULL )
                free( p_subrec[j].object_ids );

            if ( p_subrec[j].referenced_nodes != NULL )
                free( p_subrec[j].referenced_nodes );
        }

        free( p_subrec );
    }
    
    free( p_srec );
    analy->srec_tree = NULL;
    
    /* Free the state variable hash table. */
    htable_delete( analy->st_var_table, exodus_st_var_delete, TRUE );
    analy->st_var_table = NULL;
    
    /* Free the primal result hash table. */
    htable_delete( analy->primal_results, delete_primal_result, TRUE );
    analy->primal_results = NULL;
    
    /* Free the mesh geometry tree. */
    for ( i = 0; i < analy->mesh_qty; i++ )
    {
        p_mesh = analy->mesh_table + i;

        htable_delete( p_mesh->class_table, delete_mo_class_data, TRUE );
        if ( p_mesh->hide_material != NULL )
            free( p_mesh->hide_material );
        if ( p_mesh->disable_material != NULL )
            free( p_mesh->disable_material );
        for ( j = 0; j < 3; j++ )
            if ( p_mesh->mtl_trans[j] != NULL )
                free( p_mesh->mtl_trans[j] );
        if ( p_mesh->edge_list != NULL )
        {
            free( p_mesh->edge_list->list );
            if ( p_mesh->edge_list->overflow != NULL )
                free( p_mesh->edge_list->overflow );
            free( p_mesh->edge_list );
        }
        
        for ( j = 0; j < QTY_SCLASS; j++ )
            if ( p_mesh->classes_by_sclass[j].list != NULL )
                free( p_mesh->classes_by_sclass[j].list );
    }
    free( analy->mesh_table );
    analy->mesh_table = NULL;
    
    /*
     * Free all the file scope driver stuff.
     */

    if ( state_times != NULL )
    {
        free( state_times );
        state_times = NULL;
    }
    
    if ( elem_block_data != NULL )
    {
        free( elem_block_data );
        elem_block_data = NULL;
    }
    
    if ( node_x != NULL )
    {
        free( node_x );
        node_x = node_y = node_z = NULL;
    }
    
    if ( format_subrecs != NULL )
    {
        for ( i = 0; i < format_subrec_qty; i++ )
        {
            exodus_db_cleanse_subrec( format_subrecs + i );
            htable_delete( subrec_var_htables[i], NULL, FALSE );
        }

        free( format_subrecs );
        format_subrecs = NULL;
        format_subrec_qty = 0;
        
        free( subrec_var_htables );
        subrec_var_htables = NULL;
    }
    
    if ( nonproper_classes != NULL )
    {
        free( nonproper_classes );
        nonproper_classes = NULL;
        nonproper_class_qty = 0;
    }
    
    if ( subrec_block_lists != NULL )
    {
        for ( i = 0; i < format_subrec_qty - non_elem_subrec_qty; i++ )
            if ( subrec_block_lists[i].qty > 0 )
                free( subrec_block_lists[i].list );
        free( subrec_block_lists );
        subrec_block_lists = NULL;
        non_elem_subrec_qty = 0;
    }
    
    if ( status_var_index != NULL )
    {
        free( status_var_index );
        status_var_index = NULL;
    }

    /*
     * Reset remaining static variables.
     */

    called_ex_get_init = FALSE;
    
    dims = node_qty = elem_qty = elem_blk_qty = node_set_qty = side_set_qty = 0;
    qty_global_vars = qty_nodal_vars = qty_elem_vars = 0;
    qty_states = 0;
    title[0] = '\0';
    p_mesh_array = NULL;
    mesh_qty = 0;
   
    /*
     * Close the Exodus database.
     */
    
    ex_close( dbid );
    
    return ( 0 );
}

        
/************************************************************
 * TAG( interleave_node_vals )
 *
 * Interleave the x, y, and z (if appropriate) coordinates
 * from the file scope input buffers into a vector array.
 */
static void
interleave_node_vals( float *coords )
{
    GVec3D *nodes3d;
    GVec2D *nodes2d;
    int i;

    if ( dims == 3 )
    {
        nodes3d = (GVec3D *) coords;

        for ( i = 0; i < node_qty; i++ )
        {
            nodes3d[i][0] = node_x[i];
            nodes3d[i][1] = node_y[i];
            nodes3d[i][2] = node_z[i];
        }
    }
    else
    {
        nodes2d = (GVec2D *) coords;

        for ( i = 0; i < node_qty; i++ )
        {
            nodes2d[i][0] = node_x[i];
            nodes2d[i][1] = node_y[i];
        }
    }
}

        
/************************************************************
 * TAG( calc_subrec_qty )
 *
 * Determine the effective quantity of Griz subrecords in an
 * ExodusII file.
 */
static int
calc_subrec_qty( int dbid, int *subrec_qty )
{
    int qty;
    int qty_global_vars, qty_nodal_vars, qty_elem_vars;
    int table_size;
    int *truth_table;
    int status;
    Bool_type *bound_flags;
    int i, j;

    status = ex_get_var_param( dbid, "g", &qty_global_vars );
    if ( status < 0 )
        return status;

    status = ex_get_var_param( dbid, "n", &qty_nodal_vars );
    if ( status < 0 )
        return status;

    status = ex_get_var_param( dbid, "e", &qty_elem_vars );
    if ( status < 0 )
        return status;

    qty = 0;

    if ( qty_nodal_vars > 0 )
        qty++;

    if ( qty_elem_vars > 0 )
        qty++;

    if ( qty_elem_vars > 0 )
    {
        /* Read the element variable truth table. */
        table_size = elem_blk_qty * qty_elem_vars;
        truth_table = NEW_N( int, table_size, "Elem var truth table" );
        status = ex_get_elem_var_tab( dbid, elem_blk_qty, qty_elem_vars,
                                      truth_table );
        if ( status < 0 )
            return status;

        /* 
         * For each class, count a subrecord for each set of element
         * blocks that has a unique set of state variables. 
         */

        bound_flags = NEW_N( Bool_type, elem_blk_qty, "Elem block bools" );
        
        for ( i = 0; i < elem_blk_qty; i++ )
        {
            if ( bound_flags[i] )
                continue;
            else
                bound_flags[i] = TRUE;
            
            /* Increment subrec count. */
            qty++;
            
            /* 
             * Search rest of elem blocks for one with a matching set of 
             * variables and that belongs to the same class.
             */

            for ( j = i + 1; j < elem_blk_qty; j++ )
            {
                if ( elem_block_data[j].p_class 
                     != elem_block_data[i].p_class )
                    continue;
                
                if ( bool_compare_array( qty_elem_vars, 
                                         truth_table + i * qty_elem_vars,
                                         truth_table + j * qty_elem_vars ) )
                    bound_flags[j] = TRUE;
            }
        }
        
        free( bound_flags );
        free( truth_table );
    }

    *subrec_qty = qty;
    
    return OK;
}


