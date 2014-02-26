/* $Id$ */
/*
 * io_wrappers.c - Wrappers for Mili and Taurus plotfile I/O functions
 *                 which implement a standard Griz I/O interface.
 *
 *      Douglas E. Speck
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      14 Feb 1997
 *
 *************************************************************************
 *
 * Modification History
 *
 * Date        By Whom      Description
 * ==========  ===========  ==============================================
 * 03/29/2005 I. R. Corey   Added capability to compute hex strain for
 *                          timehistory databases.
 *                          See SRC#: 291
 *
 * 05/27/2005 I. R. Corey   Fixed problem with checking for object_ids in
 *                          load_nodpos_timehist.
 *
 * 02/27/2008 I. R. Corey
 *                          load_nodpos_timehist.
 *                          See SRC#: 291
 *
 * 12/05/2008 I. R. Corey   Add DP calculations for all derived nodal
 *                          results. See mili_db_set_results().
 *                          See SRC#: 556
 *
 * 04/02/2009 I. R. Corey   Fixed a problem with searching for shell
 *                          nodal positions (coords) needed to compute
 *                          quad rotations.
 *                          See SRC#: 596
 *
 * 05/22/2009 I. R. Corey   Fixed a problem with loading object ids
 *                          for functions load_quad_nodpos_timehist()
 *                          and load_hex_nodpos_timehist().
 *                          See SRC#: 605
 *
 * 06/25/2009 I. R. Corey   Changed the allocate/init of object id array
 *                          from number of nodes to number of object ids.
 *                          See SRC#: 610
 *
 * 03/17/2010 I. R. Corey   Added check for missing nodes that can occur
 *                          if a 2 node beam is defined.
 *                          See SRC#: 610
 *
 * 12/28/2012 I. R. Corey   Added capability to view global & material
 *                          results for a file with no nodes or elements.
 *                          See SourceForge#: 19225
 *
 * 03/27/2013 I. R. Corey   Added check for material numbers > number of
 *                          materials.
 *                          See SourceForge#:19777
 *************************************************************************
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "viewer.h"

#define OK 0


static int create_st_variable( Famid fid, Hash_table *p_sv_ht, char *p_name,
                               State_variable **pp_svar );
static int create_derived_results( Analysis *analy, int srec_id, int subrec_id,
                                   Result_candidate *p_rc, Hash_table *p_ht );
static void gen_material_data( MO_class_data *, Mesh_data * );
static void check_degen_hexs( MO_class_data *p_hex_class );
static void check_degen_quads( MO_class_data *p_quad_class );
static void check_degen_tris( MO_class_data *p_tri_class );
extern void fix_title( char *title );
char        *get_subrecord( Analysis *analy, int num );

int mili_compare_labels( MO_class_labels *label1, MO_class_labels *label2 );


/************************************************************
 * TAG( mili_db_open )
 *
 * Open a Mili database family.
 *
 * This would be a lot cleaner if mc_open() just took a
 * consolidated path/name string instead of separate arguments.
 */
extern int
mili_db_open( char *path_root, int *p_dbid )
{
    char *p_c;
    char *p_root_start, *p_root_end;
    char *p_src, *p_dest;
    char *path;
    char root[512];
    char path_text[256];
    int rval;
    Famid dbid;

    /* Scan forward to end of name string. */
    for ( p_c = path_root; *p_c != '\0'; p_c++ );

    /* Scan backward to last non-slash character. */
    for ( p_c--; *p_c == '/' && p_c != path_root; p_c-- );
    p_root_end = p_c;

    /* Scan backward to last slash character preceding last non-slash char. */
    for ( ; *p_c != '/' && p_c != path_root; p_c-- );

    p_root_start = ( *p_c == '/' ) ? p_c + 1 : p_c;

    /* Generate the path argument to mc_open(). */
    if ( p_root_start == path_root )
        /* No path preceding root name. */
        path = NULL;
    else
    {
        /* Copy path (less last slash). */

        path = path_text;

        for ( p_src = path_root, p_dest = path_text;
                p_src < p_root_start - 1;
                *p_dest++ = *p_src++ );

        if ( p_src == path_root )
            /* Path must be just "/".  If that's what the app wants... */
            *p_dest++ = *path_root;

        *p_dest = '\0';
    }

    /* Generate root name argument to mc_open(). */
    for ( p_src = p_root_start, p_dest = root;
            p_src <= p_root_end;
            *p_dest++ = *p_src++ );
    *p_dest = '\0';

    rval = mc_open( root, path, "r", &dbid );

    if ( rval != OK )
    {
        mc_print_error( "mili_db_open() call mc_open()", rval );
        return GRIZ_FAIL;
    }
    else
        *p_dbid = dbid;

    return OK;
}


/************************************************************
 * TAG( mili_db_get_geom )
 *
 * Load the geometry (mesh definition) data from a Mili
 * family.
 */


extern int
mili_db_get_geom( int dbid, Mesh_data **p_mtable, int *p_mesh_qty )
{
    Hash_table *p_ht;
    Mesh_data *mesh_array;
    Htable_entry *p_hte;
    MO_class_data *p_mocd, *p_node_class=NULL;
    MO_class_data **mo_classes;
    char short_name[M_MAX_NAME_LEN], long_name[M_MAX_NAME_LEN];
    Elem_data *p_ed;
    Material_data *p_matd;
    Mesh_data *p_md;
    int mesh_qty, qty_classes;
    int obj_qty;
    int i, j, k, l;
    int idx, index;
    int int_args[3];
    int dims;
    static int elem_sclasses[] =
    {
        M_TRUSS,
        M_BEAM,
        M_TRI,
        M_QUAD,
        M_TET,
        M_PYRAMID,
        M_WEDGE,
        M_HEX,
        M_PARTICLE
    };
    int qty_esclasses, elem_class_count;
    static int qty_connects[] =
    {
        2, 3, 3, 4, 4, 5, 6, 8, 1
    };
    Bool_type have_Nodal;
    List_head *p_lh;
    int start_ident, stop_ident;
    int rval, status;
    int surface_qty;
    int qty_facets;
    int total_facets;

    int label_index;
    int *temp_labels, *temp_elems;
    int obj_id;
    int ii;

    int block_qty=0, block_index=0, block_range_index=0,
        *block_range=NULL, elem_sclass=0, mat_index=0;

    Analysis *p_analysis;

    if ( *p_mtable != NULL )
    {
        popup_dialog( WARNING_POPUP,
                      "Mesh table pointer not NULL at initialization." );
        return 1;
    }

    /* Get dimensionality of data. */
    rval = mc_query_family( dbid, QTY_DIMENSIONS, NULL, NULL, (void *) &dims );
    if ( rval != OK )
    {
        mc_print_error( "mili_db_get_geom() call mc_query_family()", rval );
        return GRIZ_FAIL;
    }

    /* Get number of meshes in family. */
    rval = mc_query_family( dbid, QTY_MESHES, NULL, NULL, (void *) &mesh_qty );
    if ( rval != OK )
    {
        mc_print_error( "mili_db_get_geom() call mc_query_family()", rval );
        return GRIZ_FAIL;
    }

    /* Allocate array of pointers to mesh geom hash tables. */
    mesh_array = NEW_N( Mesh_data, mesh_qty, "Mesh data array" );

    qty_esclasses = sizeof( elem_sclasses ) / sizeof( elem_sclasses[0] );

    for ( i = 0; i < mesh_qty; i++ )
    {
        /* Allocate a hash table for the mesh geometry. */
        p_md = mesh_array + i;
        p_ht = htable_create( 151 );
        p_md->class_table = p_ht;

        /*
         * Load nodal coordinates.
         *
         * This logic loads all M_NODE classes, although Griz only wants
         * or expects to deal with a single class "node".  However, if
         * the app that wrote the db bound variables to an M_NODE class
         * other than "node", this will allow the class to exist and
         * keep the state descriptor initialization logic flowing.
         */

        int_args[0] = i;
        int_args[1] = M_NODE;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != 0 )
        {
            mc_print_error( "mili_db_get_geom() call mc_query_family()", rval );
            return GRIZ_FAIL;
        }

        have_Nodal = FALSE;

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_class_info( dbid, i, M_NODE, j, short_name, long_name,
                                      &obj_qty );
            if ( rval != 0 )
            {
                mc_print_error( "mili_db_get_geom() call mc_get_class_info()",
                                rval );
                return GRIZ_FAIL;
            }

            /* Create mesh geometry table entry. */
            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Nodes geom table entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_NODE;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = obj_qty;
            if ( dims == 3 )
                p_mocd->objects.nodes3d = NEW_N( GVec3D, obj_qty,
                                                 "3D node coord array" );
            else
                p_mocd->objects.nodes2d = NEW_N( GVec2D, obj_qty,
                                                 "2D node coord array" );

            p_hte->data = (void *) p_mocd;

#ifdef TIME_OPEN_ANALYSIS
            printf( "Timing nodal coords read...\n" );
            manage_timer( 0, 0 );
#endif

            /* Now load the coordinates array. */
            rval = mc_load_nodes( dbid, i, short_name,
                                  (void *) p_mocd->objects.nodes );
            if ( rval != 0 )
            {
                mc_print_error( "mili_db_get_geom() call mc_load_nodes()",
                                rval );
                return GRIZ_FAIL;
            }

            p_node_class = p_mocd;

            /* Allocate the data buffer for scalar I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, obj_qty, "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );


#ifdef TIME_OPEN_ANALYSIS
            manage_timer( 0, 1 );
            putc( (int) '\n', stdout );
#endif

            if ( strcmp( short_name, "node" ) == 0 )
            {
                have_Nodal = TRUE;

                /* Keep a reference to node geometry handy. */
                p_md->node_geom = p_mocd;

                temp_labels = NEW_N( int, obj_qty, "Temp labels " );

                /* Load the Nodal Labels */

                status = mc_load_node_labels( dbid,  p_mocd->mesh_id, short_name,
                                              &block_qty, &block_range,
                                              temp_labels );

                p_mocd->labels_max = -1;    /* MININT not on all systems */
                p_mocd->labels_min = MAXINT;

                if ( status!= OK )
                    p_mocd->labels_found = FALSE;

                if ( status == OK && block_qty> 0 )
                {
                    /* Allocate space for object labels  */
                    p_mocd->labels = NEW_N( MO_class_labels, obj_qty, "Class labels " );
                    if ( p_mocd->labels == NULL )
                        popup_fatal( "Unable to allocate labels on class load" );

                    p_mocd->labels_index = NEW_N( int, obj_qty, "Class labels index" );
                    if ( p_mocd->labels_index == NULL )
                        popup_fatal( "Unable to allocate labels index on class load" );

                    p_mocd->labels_found = TRUE;
                    for (obj_id=0;
                            obj_id<p_mocd->qty;
                            obj_id++)
                    {
                        p_mocd->labels[obj_id].local_id  = obj_id;
                        p_mocd->labels[obj_id].label_num = temp_labels[obj_id];
                        if ( temp_labels[obj_id] > p_mocd->labels_max )
                            p_mocd->labels_max = temp_labels[obj_id];
                        if ( temp_labels[obj_id] <= p_mocd->labels_min )
                            p_mocd->labels_min = temp_labels[obj_id];
                    }

                    /* Sort the labels */

                    /*qsort(p_mocd->labels, obj_qty, sizeof(MO_class_labels),
                          (void *) mili_compare_labels); */
                    qsort(p_mocd->labels, obj_qty, sizeof(MO_class_labels),
                           mili_compare_labels);

                    /* Create a mapping for the 1-n label index */

                    for (obj_id=0;
                            obj_id<obj_qty;
                            obj_id++)
                        p_mocd->labels_index[p_mocd->labels[obj_id].local_id] = obj_id;
                }
                else
                {
                    /* no labels were found so create them here */
                    block_qty = 1;
                    block_range = (int *) malloc(2);
                    if(block_range == NULL)
                    {
                        popup_fatal( "Unable to allocate block_range" );
                    }

                    for(index = 0; index < obj_qty; index++)
                    {
                        temp_labels[index] = index + 1;
                    }

                    block_range[0] = 1;
                    block_range[1] = obj_qty;

                    p_mocd->labels = NEW_N( MO_class_labels, obj_qty, "Class labels " );
                    if ( p_mocd->labels == NULL )
                        popup_fatal( "Unable to allocate labels on class load" );

                    p_mocd->labels_index = NEW_N( int, obj_qty, "Class labels index" );
                    if ( p_mocd->labels_index == NULL )
                        popup_fatal( "Unable to allocate labels index on class load" );

                    p_mocd->labels_found = TRUE;

                    for (obj_id=0;
                            obj_id<p_mocd->qty;
                            obj_id++)
                    {
                        p_mocd->labels[obj_id].local_id  = obj_id;
                        p_mocd->labels[obj_id].label_num = temp_labels[obj_id];
                        if ( temp_labels[obj_id] > p_mocd->labels_max )
                            p_mocd->labels_max = temp_labels[obj_id];
                        if ( temp_labels[obj_id] <= p_mocd->labels_min )
                            p_mocd->labels_min = temp_labels[obj_id];
                    }

                    /* Sort the labels */

                    /*qsort(p_mocd->labels, obj_qty, sizeof(MO_class_labels),
                          (void *) mili_compare_labels); */
                    qsort(p_mocd->labels, obj_qty, sizeof(MO_class_labels),
                           mili_compare_labels);

                    /* Create a mapping for the 1-n label index */

                    for (obj_id=0;
                            obj_id<obj_qty;
                            obj_id++)
                        p_mocd->labels_index[p_mocd->labels[obj_id].local_id] = obj_id;


                }

                p_mocd->label_blocking.block_qty = 0;

                if ( p_mocd->labels_found &&
                        block_qty>0 && block_range )
                {
                    /* Construct the Label Blocking table of contents */
                    p_mocd->label_blocking.block_qty           = block_qty;
                    p_mocd->label_blocking.block_total_objects = 0;
                    p_mocd->label_blocking.block_min           = MAXINT;
                    p_mocd->label_blocking.block_max           = MININT;
                    p_mocd->label_blocking.block_objects = NEW_N( Label_block_data, block_qty, "Node Class Label Block Objects" );

                    block_index = 0;
                    for ( k=0;
                            k<block_qty;
                            k++ )
                    {
                        /* Update min and max labels for this block */
                        if (block_range[block_index]<p_mocd->label_blocking.block_min)
                            p_mocd->label_blocking.block_min = block_range[block_index];
                        if (block_range[block_index]>p_mocd->label_blocking.block_max)
                            p_mocd->label_blocking.block_max = block_range[block_index];

                        p_mocd->label_blocking.block_objects[k].label_start = block_range[block_index++];
                        p_mocd->label_blocking.block_objects[k].label_stop  = block_range[block_index++];
                        p_mocd->label_blocking.block_total_objects += (p_mocd->label_blocking.block_objects[k].label_stop -
                                p_mocd->label_blocking.block_objects[k].label_start)+1;
                    }
                }
                free( temp_labels );
            }


            /* Update the mesh references by superclass. */
            p_lh = p_md->classes_by_sclass + M_NODE;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *,
                                  mo_classes, p_lh->qty, 1,
                                  "Extend node sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;
        }

        if ( !have_Nodal )
        {
            popup_dialog( WARNING_POPUP,
                          "Node object classs not found \"node\"." );

            /* htable_delete( p_ht, delete_mo_class_data, TRUE );
               return OK; */

            /* No nodal or element data in this problem so create a
             * fake nodal class.
             */
            p_mocd = NEW( MO_class_data, "Nodes geom table entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, " " );
            griz_str_dup( &p_mocd->long_name, " " );
            p_mocd->superclass = M_UNIT;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = 0;

            /* Allocate the data buffer for I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, 10000,
                                         "Class data buffer" );

            /* Keep a reference to node geometry handy. */
            p_md->node_geom = p_mocd;
        }

        mesh_array->hex_vol_sums = NULL;

        /*
         * Load element connectivities.
         */

#ifdef TIME_OPEN_ANALYSIS
        printf( "Timing connectivity load and sort into geom...\n" );
        manage_timer( 0, 0 );
#endif

        elem_class_count = 0;
        int_args[0] = i;
        for ( j = 0; j < qty_esclasses; j++ )
        {
            int_args[1] = elem_sclasses[j];
            rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS,
                                    (void *) int_args, NULL,
                                    (void *) &qty_classes );
            if ( rval != 0 )
            {
                mc_print_error( "mili_db_get_geom() call mc_query_family()",
                                rval );
                return GRIZ_FAIL;
            }

            for ( k = 0; k < qty_classes; k++ )
            {
                rval = mc_get_class_info( dbid, i, elem_sclasses[j], k,
                                          short_name, long_name, &obj_qty );
                if ( rval != 0 )
                {
                    mc_print_error( "mili_db_get_geom() call "
                                    "mc_get_class_info()", rval );
                    return GRIZ_FAIL;
                }

                if ( obj_qty==0 )
                    continue;

                /* Create mesh geometry table entry. */
                htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

                p_mocd = NEW( MO_class_data, "Elem geom table entry" );

                p_mocd->mesh_id = i;
                griz_str_dup( &p_mocd->short_name, short_name );
                griz_str_dup( &p_mocd->long_name, long_name );

                p_mocd->superclass = elem_sclasses[j];
                p_mocd->elem_class_index = elem_class_count++;
                p_mocd->qty = obj_qty;

                p_ed = NEW( Elem_data, "Element conn struct" );
                p_mocd->objects.elems = p_ed;
                p_ed->nodes = NEW_N( int, obj_qty * qty_connects[j],
                                     "Element connectivities" );
                p_ed->mat    = NEW_N( int, obj_qty, "Element materials" );
                p_ed->part   = NEW_N( int, obj_qty, "Element parts" );
                p_ed->volume = NULL;

                /* Allocate the data buffer for I/O and result derivation. */
                p_mocd->data_buffer = NEW_N( float, obj_qty,
                                             "Class data buffer" );
                if ( p_mocd->data_buffer == NULL )
                    popup_fatal( "Unable to alloc data buffer on class load" );

                p_hte->data = (void *) p_mocd;

                rval = mc_load_conns( dbid, i, short_name, p_ed->nodes,
                                      p_ed->mat, p_ed->part );
                if ( rval != 0 )
                {
                    mc_print_error( "mili_db_get_geom() call "
                                    "mc_load_conns()", rval );
                    return GRIZ_FAIL;
                }

                temp_elems  = NEW_N( int, obj_qty, "Temp element ids " );
                temp_labels = NEW_N( int, obj_qty, "Temp labels " );

                /* Load the Element Labels */
                status = mc_load_conn_labels( dbid, p_mocd->mesh_id, short_name,
                                              obj_qty,
                                              &block_qty, &block_range,
                                              temp_elems, temp_labels );

                p_mocd->labels_max = -1;    /* MININT not on all systems */
                p_mocd->labels_min = MAXINT;

                if ( status!= OK )
                    p_mocd->labels_found = FALSE;

                if ( status == OK && block_qty> 0 )
                {
                    /* Allocate space for object labels  */
                    p_mocd->labels = NEW_N( MO_class_labels, obj_qty, "Class labels " );
                    if ( p_mocd->labels == NULL )
                        popup_fatal( "Unable to allocate labels on class load" );

                    p_mocd->labels_index = NEW_N( int, obj_qty, "Class labels index" );
                    if ( p_mocd->labels_index == NULL )
                        popup_fatal( "Unable to allocate labels index on class load" );

                    p_mocd->labels_found = TRUE;
                    for (obj_id=0;
                            obj_id<p_mocd->qty;
                            obj_id++)
                    {
                        p_mocd->labels[obj_id].local_id  = temp_elems[obj_id]-1;
                        p_mocd->labels[obj_id].label_num = temp_labels[obj_id];

                        if ( temp_labels[obj_id] > p_mocd->labels_max )
                            p_mocd->labels_max = temp_labels[obj_id];
                        if ( temp_labels[obj_id] <= p_mocd->labels_min )
                            p_mocd->labels_min = temp_labels[obj_id];
                    }

                    /* Sort the labels */

                    /*qsort(p_mocd->labels, obj_qty, sizeof(MO_class_labels),
                          (void *) mili_compare_labels); */

                    qsort(p_mocd->labels, obj_qty, sizeof(MO_class_labels),
                          mili_compare_labels);
                    /* Create a mapping for the 1-n label index */

                    for (obj_id=0;
                            obj_id<obj_qty;
                            obj_id++)
                    {
                        p_mocd->labels_index[p_mocd->labels[obj_id].local_id] = obj_id;
                        mat_index = p_mocd->labels[obj_id].local_id;
                        if ( p_mocd->labels[obj_id].local_id<0 || p_mocd->labels[obj_id].local_id>=obj_qty )
                            mat_index = p_mocd->labels[obj_id].local_id;
                    }
                }
                else
                {
                    /* no labels were found so create them here */
                    block_qty = 1;
                    block_range = (int *) malloc(2);
                    if(block_range == NULL)
                    {
                        popup_fatal( "Unable to allocate block_range" );
                    }

                    for(index = 0; index < obj_qty; index++)
                    {
                        temp_labels[index] = index + 1;
                        temp_elems[index] = index + 1;
                    }

                    block_range[0] = 1;
                    block_range[1] = obj_qty;
                    p_mocd->labels_found = TRUE;

                    /* Allocate space for object labels  */
                    p_mocd->labels = NEW_N( MO_class_labels, obj_qty, "Class labels " );
                    if ( p_mocd->labels == NULL )
                        popup_fatal( "Unable to allocate labels on class load" );

                    p_mocd->labels_index = NEW_N( int, obj_qty, "Class labels index" );
                    if ( p_mocd->labels_index == NULL )
                        popup_fatal( "Unable to allocate labels index on class load" );


                    for (obj_id=0;
                            obj_id<p_mocd->qty;
                            obj_id++)
                    {
                        p_mocd->labels[obj_id].local_id  = temp_elems[obj_id]-1;
                        p_mocd->labels[obj_id].label_num = temp_labels[obj_id];

                        if ( temp_labels[obj_id] > p_mocd->labels_max )
                            p_mocd->labels_max = temp_labels[obj_id];
                        if ( temp_labels[obj_id] <= p_mocd->labels_min )
                            p_mocd->labels_min = temp_labels[obj_id];
                    }

                    qsort(p_mocd->labels, obj_qty, sizeof(MO_class_labels),
                          mili_compare_labels);

                    for (obj_id=0;
                            obj_id<obj_qty;
                            obj_id++)
                    {
                        p_mocd->labels_index[p_mocd->labels[obj_id].local_id] = obj_id;
                        mat_index = p_mocd->labels[obj_id].local_id;
                        if ( p_mocd->labels[obj_id].local_id<0 || p_mocd->labels[obj_id].local_id>=obj_qty )
                            mat_index = p_mocd->labels[obj_id].local_id;
                    }
                }

                free( temp_labels );
                free( temp_elems );

                /* If we have multiple blocks of labels then identify them
                 * by loading into a structure for later retrieval.
                 */

                if ( p_mocd->labels_found &&
                        block_qty>0 && block_range )
                {
                    p_mocd->label_blocking.block_qty           = block_qty;
                    p_mocd->label_blocking.block_total_objects = 0;
                    p_mocd->label_blocking.block_min           = MAXINT;
                    p_mocd->label_blocking.block_max           = MININT;

                    p_mocd->label_blocking.block_objects = NEW_N( Label_block_data, block_qty, "Element Class Label Block Objects" );

                    block_range_index = 0;
                    mat_index = 0;

                    for ( block_index=0;
                            block_index<block_qty;
                            block_index++ )
                    {
                        /* Update min and max labels for this block */

                        if (block_range[block_range_index]<p_mocd->label_blocking.block_min)
                            p_mocd->label_blocking.block_min = block_range[block_range_index];
                        if (block_range[block_range_index]>p_mocd->label_blocking.block_max)
                            p_mocd->label_blocking.block_max = block_range[block_range_index];

                        p_mocd->label_blocking.block_objects[block_index].label_start = block_range[block_range_index++];
                        p_mocd->label_blocking.block_objects[block_index].label_stop  = block_range[block_range_index++];

                    }

                    if ( block_range )
                        free( block_range );
                }

                /* Element superclass-specific actions. */
                switch ( elem_sclasses[j] )
                {
                case M_HEX:
                    check_degen_hexs( p_mocd );
                    p_lh = p_md->classes_by_sclass + M_HEX;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend hex sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;

                    break;

                    /* #ifdef HAVE_WEDGE_PYRAMID */
                case M_WEDGE:
                    popup_dialog( INFO_POPUP, "%s\n%s (class \"%s\")",
                                  "Checking for degenerate wedge elements",
                                  "is not implemented.", short_name );
                    p_lh = p_md->classes_by_sclass + M_WEDGE;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend wedge sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_PYRAMID:
                    popup_dialog( INFO_POPUP, "%s\n%s (class \"%s\")",
                                  "Checking for degenerate pyramid",
                                  "elements is not implemented.",
                                  short_name );
                    p_lh = p_md->classes_by_sclass + M_PYRAMID;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend pyramid sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                    /* #endif */
                case M_TET:
                    popup_dialog( INFO_POPUP, "%s\n%s (class \"%s\")",
                                  "Checking for degenerate tet elements",
                                  "is not implemented.", short_name );
                    p_lh = p_md->classes_by_sclass + M_TET;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend tet sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_QUAD:
                    check_degen_quads( p_mocd );
                    p_lh = p_md->classes_by_sclass + M_QUAD;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend quad sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_TRI:
                    check_degen_tris( p_mocd );
                    if ( p_mocd->objects.elems->has_degen )
                        popup_dialog( INFO_POPUP, "%s\n(class \"%s\").",
                                      "Degenerate tri element(s) detected",
                                      short_name );
                    p_lh = p_md->classes_by_sclass + M_TRI;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend tri sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_BEAM:
                    p_lh = p_md->classes_by_sclass + M_BEAM;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend beam sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_TRUSS:
                    p_lh = p_md->classes_by_sclass + M_TRUSS;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend truss sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_PARTICLE:
                    p_lh = p_md->classes_by_sclass + M_PARTICLE;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend particle sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;

                    break;
                default:
                    /* do nothing */
                    ;
                }
            }
        }

#ifdef TIME_OPEN_ANALYSIS
        manage_timer( 0, 1 );
        putc( (int) '\n', stdout );
#endif

        /* Update the Mesh_data struct with element class info. */
        p_md->elem_class_qty = elem_class_count;


        /*
         * Load surface mesh object classes.
         */

        int_args[0] = i;
        int_args[1] = M_SURFACE;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != 0 )
        {
            mc_print_error( "mili_db_get_geom() call mc_query_family()", rval );
            return GRIZ_FAIL;
        }

        surface_qty = 0;

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_class_info( dbid, i, M_SURFACE, j, short_name, long_name,
                                      &obj_qty );
            if ( 0 != rval )
            {
                mc_print_error( "mili_db_get_geom() call mc_get_class_info()",
                                rval );
                return GRIZ_FAIL;
            }

            /* Create mesh geometry table entry. */
            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Surface geom table entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_SURFACE;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = obj_qty;
            p_mocd->objects.surfaces = NEW_N( Surface_data, obj_qty,
                                              "Surface data structure array" );

#ifdef TIME_OPEN_ANALYSIS
            printf( "Timing surface connectivities read...\n" );
            manage_timer( 0, 0 );
#endif
            total_facets = 0;
            for ( k = 0; k < obj_qty; k++ )
            {
                /*
                 * NOTE:  Surface id's are "1-based", therefore need to
                 *        add "+1" to surface id (k)
                 */

                /* Load each surface data struct */

                int_args[1] = k + 1;
                rval = mc_query_family( dbid, QTY_FACETS_IN_SURFACE,
                                        (void *) int_args, short_name,
                                        (void *) &qty_facets );

                total_facets += qty_facets;

                p_mocd->objects.surfaces[k].facet_qty = qty_facets;

                p_mocd->objects.surfaces[k].nodes = NEW_N( int, 4*qty_facets,
                                                    "Surface connectivities" );
                p_mocd->objects.surfaces[k].surface_id = surface_qty;
                surface_qty++;
            }

            for ( k = 0; k < obj_qty; k++ )
            {

                /* Now load the coordinates array.*/
                rval = mc_load_surface( dbid, i, k, short_name,
                                        (void *) p_mocd->objects.surfaces[k].nodes );
                if ( rval != 0 )
                {
                    mc_print_error( "mili_db_get_geom() call mc_load_surface()",
                                    rval );
                    return GRIZ_FAIL;
                }
            }

            /* Allocate the data buffer for I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, total_facets,
                                         "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to alloc data buffer on class load" );

#ifdef TIME_OPEN_ANALYSIS
            manage_timer( 0, 1 );
            putc( (int) '\n', stdout );
#endif

            p_hte->data = (void *) p_mocd;


            /* Update the mesh references by superclass. */
            p_lh = p_md->classes_by_sclass + M_SURFACE;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *,
                                  mo_classes, p_lh->qty, 1,
                                  "Extend surface sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;
        }



        /*
         * Load simple mesh object classes.
         */

        /* M_UNIT classes. */

        int_args[0] = i;
        int_args[1] = M_UNIT;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != 0 )
        {
            mc_print_error( "mili_db_get_geom() call mc_query_family()", rval );
            return GRIZ_FAIL;
        }

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_simple_class_info( dbid, i, M_UNIT, j, short_name,
                                             long_name, &start_ident,
                                             &stop_ident );
            if ( rval != 0 )
            {
                mc_print_error( "mili_db_get_geom() call "
                                "mc_get_simple_class_info()", rval );
                return GRIZ_FAIL;
            }

            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Simple class geom entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_UNIT;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = stop_ident - start_ident + 1;
            p_mocd->simple_start = start_ident;
            p_mocd->simple_stop = stop_ident;

            /* Allocate the data buffer for I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, p_mocd->qty,
                                         "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );

            p_lh = p_md->classes_by_sclass + M_UNIT;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *, mo_classes, p_lh->qty, 1,
                                  "Extend unit sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;

            p_hte->data = (void *) p_mocd;
        }

        /* M_MAT classes. */

        int_args[1] = M_MAT;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != 0 )
        {
            mc_print_error( "mili_db_get_geom() call mc_query_family()", rval );
            return GRIZ_FAIL;
        }

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_simple_class_info( dbid, i, M_MAT, j, short_name,
                                             long_name, &start_ident,
                                             &stop_ident );
            if ( rval != 0 )
            {
                mc_print_error( "mili_db_get_geom() call "
                                "mc_get_simple_class_info()", rval );
                return GRIZ_FAIL;
            }

            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Simple class geom entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_MAT;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = stop_ident - start_ident + 1;
            p_mocd->simple_start = start_ident;
            p_mocd->simple_stop = stop_ident;

            p_matd = NEW_N( Material_data, p_mocd->qty, "Material data array" );
            p_mocd->objects.materials = p_matd;
            gen_material_data( p_mocd, p_md );

            /* Allocate the data buffer for I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, p_mocd->qty,
                                         "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );

            p_lh = p_md->classes_by_sclass + M_MAT;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *, mo_classes, p_lh->qty, 1,
                                  "Extend material sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;

            p_hte->data = (void *) p_mocd;
        }

        /* M_MESH classes. */

        int_args[1] = M_MESH;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != 0 )
        {
            mc_print_error( "mili_db_get_geom() call mc_query_family()",
                            rval );
            return GRIZ_FAIL;
        }

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_simple_class_info( dbid, i, M_MESH, j, short_name,
                                             long_name, &start_ident,
                                             &stop_ident );
            if ( rval != 0 )
            {
                mc_print_error( "mili_db_get_geom() call "
                                "mc_get_simple_class_info()", rval );
                return GRIZ_FAIL;
            }

            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Simple class geom entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_MESH;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = stop_ident - start_ident + 1;
            p_mocd->simple_start = start_ident;
            p_mocd->simple_stop = stop_ident;

            /* Allocate the data buffer for I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, p_mocd->qty,
                                         "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );

            p_lh = p_md->classes_by_sclass + M_MESH;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *, mo_classes, p_lh->qty, 1,
                                  "Extend mesh sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;

            p_hte->data = (void *) p_mocd;
        }
    }

    /* Pass back address of geometry hash table array. */
    *p_mtable   = mesh_array;
    *p_mesh_qty = mesh_qty;

    p_analysis = get_analy_ptr();
    status = get_hex_volumes( dbid, p_analysis );
    return (OK);
}


/************************************************************
 * TAG( gen_material_data )
 *
 * Analyze element classes to generate by-material data.
 */
static void
gen_material_data( MO_class_data *p_mat_class, Mesh_data *p_mesh )
{
    Material_data *p_mtld;
    int i, j, k;
    int mtl;
    int first, datum;
    int elem_qty, qty_classes, qty, qty_mats, elem_sum;
    int *mats, *offsets, *mat_elem_lists, *elem_list, *mat_elem_qtys;
    MO_class_data **mo_classes, **mat_mo_classes;
    Int_2tuple *p_i2t;

#ifdef TIME_OPEN_ANALYSIS
    printf( "Timing gen_material_data...\n" );
    manage_timer( 0, 0 );
#endif

    p_mtld = p_mat_class->objects.materials;
    qty_mats = p_mat_class->qty;

    mat_elem_qtys = NEW_N( int, qty_mats, "Material elem quantities array" );
    mat_mo_classes = NEW_N( MO_class_data *, qty_mats, "Mat elem class ptrs" );

    /* Search through all element classes to sum elements. */
    elem_sum = 0;
    for ( i = G_TRUSS; i < G_MAT; i++ )
    {
        qty_classes = p_mesh->classes_by_sclass[i].qty;
        if ( qty_classes == 0 )
            continue;

        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[i].list;
        for ( j = 0; j < qty_classes; j++ )
        {
            elem_qty = mo_classes[j]->qty;
            mats = mo_classes[j]->objects.elems->mat;

            /*
             * Pass through the elements to count elements for each material
             * and assign element class pointers by material.
             */
            for ( k = 0; k < elem_qty; k++ )
            {
                mtl = mats[k];

                if ( mtl>=qty_mats )
                {
                    printf( "\n\n** Error: A Material was found (#%d) that is greater than the total number of materials (%d).", mtl, qty_mats );
                    printf( "\n** Error: Griz exiting!\n" );
                    exit(0);
                }

                mat_elem_qtys[mtl]++;
                mat_mo_classes[mtl] = mo_classes[j];
            }

            /* Sum all elements. */
            elem_sum += elem_qty;
        }
    }

    /* Allocate the BIG array (an array of all elements, by material). */
    mat_elem_lists = NEW_N( int, elem_sum, "Mtl elem lists array" );
    if ( mat_elem_lists == NULL )
        popup_fatal( "Allocation failure in material data processing." );

    /* Allocate/init the material offsets array. */
    offsets = NEW_N( int, qty_mats, "Material offsets array" );
    for ( i = 1; i < qty_mats; i++ )
        offsets[i] = offsets[i - 1] + mat_elem_qtys[i - 1];

    /* Pass through all elements again to sort into material lists. */
    for ( i = G_TRUSS; i < G_MAT; i++ )
    {
        qty_classes = p_mesh->classes_by_sclass[i].qty;
        if ( qty_classes == 0 )
            continue;

        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[i].list;
        for ( j = 0; j < qty_classes; j++ )
        {
            elem_qty = mo_classes[j]->qty;
            mats = mo_classes[j]->objects.elems->mat;

            /* Record the element id at the current offset for its material. */
            for ( k = 0; k < elem_qty; k++ )
                mat_elem_lists[ offsets[ mats[k] ]++ ] = k;
        }
    }

    /* Re-init the offsets. */
    offsets[0] = 0;
    for ( i = 1; i < qty_mats; i++ )
        offsets[i] = offsets[i - 1] + mat_elem_qtys[i - 1];

    /* Compress material element lists into material element block lists. */
    for ( i = 0; i < qty_mats; i++ )
    {
        elem_list = mat_elem_lists + offsets[i];
        elem_qty = mat_elem_qtys[i];

        /* Init to process current material's list of elements. */
        datum = first = elem_list[0];
        p_i2t = NULL;
        qty = 0;
        for ( j = 1; j < elem_qty; j++ )
        {
            if ( elem_list[j] == datum + j )
                continue;

            /* New block; record previous block. */
            p_i2t = RENEW_N( Int_2tuple, p_i2t, qty, 1, "Extend mtl elem blk" );
            p_i2t[qty][0] = first;
            p_i2t[qty][1] = datum + j - 1;
            qty++;

            /* Initialize new block. */
            first = elem_list[j];
            datum = first - j;
        }

        p_mtld[i].elem_qty = elem_qty;

        /* If there were elements for the material... */
        if ( elem_qty > 0 )
        {
            /* Record the last block. */
            p_i2t = RENEW_N( Int_2tuple, p_i2t, qty, 1, "Extend mtl elem blk" );
            p_i2t[qty][0] = first;
            p_i2t[qty][1] = datum + j - 1;
            qty++;

            p_mtld[i].elem_block_qty = qty;
            p_mtld[i].elem_blocks = p_i2t;
            p_mtld[i].elem_class = mat_mo_classes[i];
        }
    }

#ifdef TIME_OPEN_ANALYSIS
    manage_timer( 0, 1 );
    putc( (int) '\n', stdout );
#endif

#ifdef DUMP_MAT_LISTS
    for ( i = 0; i < qty_mats; i++ )
    {
        printf( "Material %d: %d %s elements\n", i + 1, mat_elem_qtys[i],
                mat_mo_classes[i]->long_name );

        p_i2t = p_mtld[i].elem_blocks;
        for ( j = 0; j < p_mtld[i].elem_block_qty; j++ )
            printf( "%d - %d\n", p_i2t[j][0] + 1, p_i2t[j][1] + 1 );
    }
#endif

    free( mat_elem_lists );
    free( mat_elem_qtys );
    free( mat_mo_classes );
    free( offsets );
}


/************************************************************
 * TAG( check_degen_hexs )
 *
 * Check for degenerate hex elements (6-node wedges, 5-node pyramids
 * or 4-node tetrahedra) in the list of volume elements.  DYNA
 * specifies a special node ordering for these degenerate elements
 * which screws up the face drawing.  So this routine reorders the
 * nodes to a better ordering for face drawing.
 */
static void
check_degen_hexs( MO_class_data *p_hex_class )
{
    int i;
    int (*connects)[8];
    int hex_qty;

    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    hex_qty = p_hex_class->qty;

    for ( i = 0; i < hex_qty; i++ )
    {
        if ( connects[i][4] == connects[i][5] &&
                connects[i][5] == connects[i][6] &&
                connects[i][6] == connects[i][7] )
        {
            if ( connects[i][3] == connects[i][4] )
            {
                /* Tetrahedral element. */
                connects[i][3] = connects[i][2];
                p_hex_class->objects.elems->has_degen = TRUE;
            }
            else
            {
                /* Pyramid element. */
                p_hex_class->objects.elems->has_degen = TRUE;
            }
        }
        else if ( connects[i][4] == connects[i][5] &&
                  connects[i][6] == connects[i][7] )
        {
            /* Prism-shaped (triangular wedge) element. */
            /* Has Dyna changed?  These now break (but didn't historically?)
            connects[i][5] = connects[i][6];
            connects[i][7] = connects[i][4];
            */
            p_hex_class->objects.elems->has_degen = TRUE;
        }
    }
}


/************************************************************
 * TAG( check_degen_quads )
 *
 * Check for degenerate quad elements (3-node triangles instead
 * of 4-node quadrilaterals).  Reorder the nodes of triangular
 * quad elements so that the repeated node is last.
 */
static void
check_degen_quads( MO_class_data *p_quad_class )
{
    int tmp, shift, i, j, k;
    int (*connects)[4];
    int quad_qty;

    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
    quad_qty = p_quad_class->qty;

    for ( i = 0; i < quad_qty; i++ )
    {
        if ( connects[i][0] == connects[i][1] )
        {
            shift = 2;
            p_quad_class->objects.elems->has_degen = TRUE;
        }
        else if ( connects[i][1] == connects[i][2] )
        {
            shift = 1;
            p_quad_class->objects.elems->has_degen = TRUE;
        }
        else if ( connects[i][2] == connects[i][3] )
        {
            shift = 0;
            p_quad_class->objects.elems->has_degen = TRUE;
        }
        else if ( connects[i][0] == connects[i][3] )
        {
            shift = 3;
            p_quad_class->objects.elems->has_degen = TRUE;
        }
        else
            continue;

        for ( j = 0; j < shift; j++ )
        {
            tmp = connects[i][3];
            for ( k = 3; k > 0; k-- )
                connects[i][k] = connects[i][k - 1];
            connects[i][0] = tmp;
        }
    }
}


/************************************************************
 * TAG( check_degen_tris )
 *
 * Check for degenerate tri elements.
 *
 * Just flagging existence (for now?)...
 */
static void
check_degen_tris( MO_class_data *p_tri_class )
{
    int i;
    int (*connects)[3];
    int tri_qty;

    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
    tri_qty = p_tri_class->qty;
    p_tri_class->objects.elems->has_degen = FALSE;

    for ( i = 0; i < tri_qty; i++ )
    {
        if ( connects[i][0] == connects[i][1] )
            p_tri_class->objects.elems->has_degen = TRUE;
        else if ( connects[i][1] == connects[i][2] )
            p_tri_class->objects.elems->has_degen = TRUE;
        else if ( connects[i][0] == connects[i][2] )
            p_tri_class->objects.elems->has_degen = TRUE;
        else
            continue;
    }
}

/************************************************************
 * TAG( mili_db_get_st_descriptors )
 *
 * Query a database and store information about the available
 * state record formats and referenced state variables.
 */
extern int
mili_db_get_st_descriptors( Analysis *analy, int dbid )
{
    int srec_qty, svar_qty, subrec_qty, mesh_node_qty;
    Famid fid;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrecs;
    Subrecord *p_subr;
    int rval;
    Hash_table *p_sv_ht, *p_primal_ht;
    int i, j, k;
    char **svar_names;
    int mesh_id;
    Htable_entry *p_hte;
    int class_size;
    MO_class_data *p_mocd;
    Bool_type nodal, particle;
    int *node_work_array=NULL;
    State_variable *p_svar;
    Mesh_data *p_mesh;

    int subrec_index=0;

    fid = (Famid) dbid;

    analy->num_bad_subrecs=0;
    analy->bad_subrecs=NULL;

    /* Get state record format count for this database. */
    rval = mc_query_family( fid, QTY_SREC_FMTS, NULL, NULL,
                            (void *) &srec_qty );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_get_st_descriptors() call mc_query_family()",
                        rval );
        return GRIZ_FAIL;
    }

    /* Allocate array of state record structs. */
    p_sro = NEW_N( State_rec_obj, srec_qty, "Srec tree" );

    /* Get state variable quantity for this database. */
    rval = mc_query_family( fid, QTY_SVARS, NULL, NULL, (void *) &svar_qty );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_get_st_descriptors() call mc_query_family()",
                        rval );
        return GRIZ_FAIL;
    }

    if ( svar_qty > 0 )
    {
        if ( svar_qty < 100 )
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

    /* Loop over srecs */
    for ( i = 0; i < srec_qty; i++ )
    {
        /* Get subrecord count for this state record. */
        rval = mc_query_family( fid, QTY_SUBRECS, (void *) &i, NULL,
                                (void *) &subrec_qty );
        if ( rval != 0 )
        {
            mc_print_error( "mili_db_get_st_descriptors() call "
                            "mc_query_family()", rval );
            return GRIZ_FAIL;
        }

        /* Allocate array of Subrec_obj's. */
        p_subrecs = NEW_N( Subrec_obj, subrec_qty, "Srec subrec branch" );
        p_sro[i].subrecs = p_subrecs;
        p_sro[i].qty = subrec_qty;

        /* Init nodal position and velocity subrec indices to invalid values. */
        p_sro[i].node_pos_subrec = -1;
        p_sro[i].node_vel_subrec = -1;
        p_sro[i].particle_pos_subrec = -1;

        /* Get mesh_id & mesh for this srec. */
        rval = mc_query_family( fid, SREC_MESH, (void *) &i, NULL,
                                (void *) &mesh_id );
        if ( rval != 0 )
        {
            mc_print_error( "mili_db_get_st_descriptors() call "
                            "mc_query_family()", rval );
            return GRIZ_FAIL;
        }
        p_mesh = analy->mesh_table + mesh_id;

        /* Allocate a temporary working array for subrec node list creation. */
        mesh_node_qty = p_mesh->node_geom->qty;
        node_work_array = NEW_N( int, mesh_node_qty, "Temp node array" );

        /* Loop over subrecs */
        for ( j = 0;
                j < subrec_qty;
                j++ )
        {
            p_subr = &p_subrecs[subrec_index].subrec;

            /* Get binding */
            rval = mc_get_subrec_def( fid, i, j, p_subr );
            if ( rval != 0 )
            {
                mc_print_error( "mili_db_get_st_descriptors() call "
                                "mc_get_subrec_def()", rval );
                return GRIZ_FAIL;
            }

            if ( p_subr->qty_objects==0 )
            {
                if ( analy->bad_subrecs==NULL )
                {
                    analy->bad_subrecs = NEW_N( int, subrec_qty, "List of bad subrecords" );
                }
                analy->bad_subrecs[analy->num_bad_subrecs++] = j;
                subrec_index++;
                continue;
            }

            htable_search( p_mesh->class_table, p_subr->class_name, FIND_ENTRY,
                           &p_hte );
            p_mocd = ((MO_class_data *) p_hte->data);
            p_subrecs[subrec_index].p_object_class = p_mocd;

            /* Create list of nodes referenced by objects bound to subrecord. */
            create_subrec_node_list( node_work_array, mesh_node_qty,
                                     p_subrecs + subrec_index );

            /*
             * Create ident array if indexing is required.
             */

            class_size = p_mocd->qty;

            if ( p_subr->qty_objects != class_size
                    || ( class_size > 1
                         && ( p_subr->qty_blocks != 1
                              || p_subr->mo_blocks[0] != 1 ) ) )
            {
                /*
                 * Subrecord does not completely and continuously represent
                 * object class, so an index map is needed to get from
                 * sequence number in the subrecord to an object ident.
                 */
                p_subrecs[subrec_index].object_ids = NEW_N( int, p_subr->qty_objects,
                                                     "Subrec ident map array" );
                blocks_to_list( p_subr->qty_blocks, p_subr->mo_blocks,
                                p_subrecs[subrec_index].object_ids, TRUE );
            }
            else
                p_subrecs[subrec_index].object_ids = NULL;

            /*
             * M_NODE class "node" is special - need it for node positions
             * and velocities.
             */
            if ( strcmp( p_subr->class_name, "node" ) == 0 )
                nodal = TRUE;
            else
                nodal = FALSE;

            /* For particle databases, particle position needed. */
            if ( strcmp( p_subr->class_name, particle_cname ) == 0 )
                particle = TRUE;
            else
                particle = FALSE;

            /*
             * Loop over svars and create state variable and primal result
             * table entries.
             */
            svar_names = p_subr->svar_names;
            k=0;

            for ( k = 0; k < p_subr->qty_svars; k++ )
            {
                rval = create_st_variable( fid, p_sv_ht, svar_names[k],
                                           &p_svar );
                if ( rval != 0 )
                {
                    popup_dialog( WARNING_POPUP,
                                  "mili_db_get_st_descriptors() - unable to "
                                  "create state variable." );
                    return rval;
                }
                create_primal_result( p_mesh, i, subrec_index, p_subrecs + subrec_index, p_primal_ht,
                                      srec_qty, svar_names[k], p_sv_ht );

                if ( nodal )
                {
                    if ( strcmp( svar_names[k], "nodpos" ) == 0 )
                    {
                        if ( p_sro[i].node_pos_subrec != -1 )
                            popup_dialog( WARNING_POPUP,
                                          "Multiple \"node\" position subrecs." );

                        p_sro[i].node_pos_subrec = subrec_index;
                        analy->stateDB = FALSE;
                        if( mesh_node_qty == p_subr->qty_objects )
                        {
                            /* This is a state database and not a time
                             * history database.
                             */
                            analy->stateDB = TRUE;
                        }

                        /* Note if data is double precision. */
                        if ( p_svar->num_type == M_FLOAT8 )
                            p_mesh->double_precision_nodpos = TRUE;
                    }
                    else if ( strcmp( svar_names[k], "nodvel" ) == 0 )
                    {
                        if ( p_sro[i].node_vel_subrec != -1 )
                            popup_dialog( WARNING_POPUP,
                                          "Multiple \"node\" velocity subrecs." );
                        p_sro[i].node_vel_subrec = j;
                    }
                }

                if ( particle )
                {
                    if ( strcmp( svar_names[k], "partpos" ) == 0 )
                    {
                        p_sro[i].particle_pos_subrec = subrec_index;

                        /* Note if data is double precision. */
                        if ( p_svar->num_type == M_FLOAT8 )
                            p_mesh->double_precision_partpos = TRUE;
                    }
                }
            }
            subrec_index++;
        }

        p_sro[i].qty = subrec_index;
        free( node_work_array );
    }

    /*
     * Return the subrecord tree,state variable hash table, and primal result
     * hash table.
     */
    analy->srec_tree = p_sro;
    analy->qty_srec_fmts = srec_qty;
    analy->st_var_table = p_sv_ht;
    analy->primal_results = p_primal_ht;

    return OK;
}

/************************************************************
 * TAG( create_subrec_node_list )
 *
 * Create list of nodes referenced by objects bound to subrecord.
 */
void
create_subrec_node_list( int *node_tmp, int node_tmp_size, Subrec_obj *p_so )
{
    int conn_qty, node_cnt;
    int i, j, k;
    int superclass;
    int start, stop;
    int facet_qty;
    Subrecord *p_subrec;
    MO_class_data *p_elem_class;
    int *connects, *node_list, *elem_conns;
    Int_2tuple *mo_blocks;

    /*
     * No node list for non-element superclass subrecords.
     * Non-proper G_NODE subrecords can just use object_ids array.
     */
    superclass = p_so->p_object_class->superclass;
    if ( !IS_ELEMENT_SCLASS( superclass ) )
        return;

    conn_qty = qty_connects[superclass];
    p_elem_class = p_so->p_object_class;

    /* Mark all nodes referenced by elements; count marked nodes. */
    connects = p_elem_class->objects.elems->nodes;
    p_subrec = &p_so->subrec;
    mo_blocks = (Int_2tuple *) p_subrec->mo_blocks;

    node_cnt = 0;
    if( superclass != G_SURFACE )
    {
        for ( i = 0; i < p_subrec->qty_blocks; i++ )
        {
            start = mo_blocks[i][0];
            stop = mo_blocks[i][1];

            for ( j = start; j <= stop; j++ )
            {
                elem_conns = connects + (j-1) * conn_qty;

                for ( k = 0; k < conn_qty; k++ )
                {
                    if ( elem_conns[k]<0 ) /* Check for missing node */
                        continue;

                    if ( node_tmp[elem_conns[k]] )
                        continue;
                    else
                    {
                        node_tmp[elem_conns[k]] = TRUE;
                        node_cnt++;
                    }
                }
            }
        }
    }
    else
    {
        for ( i = 0; i < p_subrec->qty_blocks; i++ )
        {
            facet_qty = p_elem_class->objects.surfaces->facet_qty;
            for ( j = 1; j <= facet_qty; j++ )
            {
                elem_conns = connects + (j-1) * conn_qty;

                for ( k = 0; k < 4; k++ )
                {
                    if ( node_tmp[elem_conns[k]] )
                        continue;
                    else
                    {
                        node_tmp[elem_conns[k]] = TRUE;
                        node_cnt++;
                    }
                }
            }
        }
    }

    /* Build list of marked nodes. */
    p_so->ref_node_qty = node_cnt;
    node_list = NEW_N( int, node_cnt, "Subrec node list" );

    node_cnt = 0;
    for ( i = 0; i < node_tmp_size; i++ )
    {
        if ( node_tmp[i] )
        {
            node_list[node_cnt] = i;
            node_cnt++;
        }
    }
    p_so->referenced_nodes = node_list;

    /* Clear the working array before returning. */
    memset( (void *) node_tmp, (int) '\0', node_tmp_size * sizeof( int ) );
}


/************************************************************
 * TAG( mili_db_cleanse_state_var )
 *
 * Free dynamically allocated memory from a State_variable struct.
 */
extern int
mili_db_cleanse_state_var( State_variable *p_st_var )
{
    int rval;

    rval = mc_cleanse_st_variable( p_st_var );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_cleanse_state_var() call "
                        "mc_cleanse_st_variable()", rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( create_st_variable )
 *
 * Create State_variable table entries for a db.
 */
static int
create_st_variable( Famid fid, Hash_table *p_sv_ht, char *p_name,
                    State_variable **pp_svar )
{
    int rval;
    Hash_action op;
    Htable_entry *p_hte;
    State_variable *p_sv;
    int i;

    /* Only enter svar if not already present in table. */
    if( strncmp( p_name, "sand", 4 ) == 0 )
        op = ENTER_ALWAYS;
    else
        op = ENTER_UNIQUE;

    rval = htable_search( p_sv_ht, p_name, op, &p_hte );

    /* If this is a new entry in the state variable table... */
    if ( rval == OK )
    {
        /* Create the State_variable and store it in the hash table entry. */
        p_sv = NEW( State_variable, "New state var" );
        rval = mc_get_svar_def( fid, p_name, p_sv );
        if ( rval != 0 )
        {
            mc_print_error( "create_st_variable() call mc_get_svar_def()",
                            rval );
            return GRIZ_FAIL;
        }

        p_hte->data = (void *) p_sv;

        if ( pp_svar != NULL )
            *pp_svar = p_sv;

        /* For vectors and vector arrays, create the components. */
        if ( p_sv->agg_type == VECTOR || p_sv->agg_type == VEC_ARRAY )
        {
            for ( i = 0; i < p_sv->vec_size; i++ )
                rval = create_st_variable( fid, p_sv_ht, p_sv->components[i],
                                           NULL );
            if ( rval != 0 )
                return rval;
        }
    }

    return OK;
}


/************************************************************
 * TAG( create_primal_result )
 *
 * Create Primal_result entries for a db.
 */
int
create_primal_result( Mesh_data *p_mesh, int srec_id, int subrec_id,
                      Subrec_obj *p_subr_obj, Hash_table *p_primal_ht,
                      int qty_srec_fmts, char *p_name, Hash_table *p_sv_ht )
{
    int rval;
    Htable_entry *p_hte, *p_hte2;
    State_variable *p_sv;
    Primal_result *p_pr;
    int i;
    int *p_i;
    int superclass;
    char *p_sand_var;

    /*
     * See if primal result has already been entered in table.
     * Use ENTER_MERGE to return entry even when it already exists so
     * we can update the presence tree and check for SCALAR type with
     * "sand" flag test.
     */
    rval = htable_search( p_primal_ht, p_name, ENTER_MERGE, &p_hte );

    /* If new... */
    if ( rval == OK )
    {
        /* Create the Primal_result. */
        p_pr = NEW( Primal_result, "New primal result" );

        /* Get the State_variable for it. */
        rval = htable_search( p_sv_ht, p_name, FIND_ENTRY, &p_hte2 );
        p_sv = (State_variable *) p_hte2->data;
        p_pr->var = p_sv;

        /* Reference names from the State_variable. */
        p_pr->short_name = p_sv->short_name;
        p_pr->long_name = p_sv->long_name;

        p_pr->origin.is_primal = 1;

        superclass = p_subr_obj->p_object_class->superclass;
        if ( superclass == M_NODE )
            p_pr->origin.is_node_result = 1;
        else if ( superclass == M_MAT )
            p_pr->origin.is_mat_result = 1;
        else if ( superclass == M_MESH )
            p_pr->origin.is_mesh_result = 1;
        else if ( superclass == M_UNIT )
            p_pr->origin.is_unit_result = 1;
        else if ( superclass == M_SURFACE )
        {
            p_pr->origin.is_surface_result = 1;
            p_pr->origin.is_elem_result = 1;
        }
        else
            p_pr->origin.is_elem_result = 1;

        /* Initialize the state rec/subrec presence map. */
        p_pr->srec_map = NEW_N( List_head, qty_srec_fmts, "PR srec map" );
        p_i = NEW( int, "Subrec primal list" );
        *p_i = subrec_id;
        p_pr->srec_map[srec_id].list = (void *) p_i;
        p_pr->srec_map[srec_id].qty = 1;

        /* Store primal result. */
        p_hte->data = (void *) p_pr;
    }
    else
    {
        /* Primal result already exists, so just update the presence tree. */
        p_pr = (Primal_result *) p_hte->data;

        if ( p_pr->srec_map[srec_id].list == NULL )
        {
            /* First occurrence in this srec format. */
            p_i = NEW( int, "Subrec primal list" );
            *p_i = subrec_id;
            p_pr->srec_map[srec_id].list = (void *) p_i;
            p_pr->srec_map[srec_id].qty = 1;
        }
        else
        {
            /*
             * Potentially first occurrence for this subrecord.  Traverse
             * the identified subrecs to find current subrec; if not
             * present, add it.
             */
            p_i = (int *) p_pr->srec_map[srec_id].list;
            for ( i = 0; i < p_pr->srec_map[srec_id].qty; i++ )
                if ( p_i[i] == subrec_id )
                    break;

            if ( i == p_pr->srec_map[srec_id].qty )
            {
                p_i = RENEW_N( int, p_i, i, 1, "Extend subrec map list" );
                p_i[i] = subrec_id;

                /* Assign back in case realloc moved the array. */
                p_pr->srec_map[srec_id].list = (void *) p_i;
                p_pr->srec_map[srec_id].qty++;
            }

        }
    }

    /* Check for sand flag. */
    p_sand_var = ( db_type == EXODUS ) ? "STATUS" : "sand";
    if( strncmp( p_name, p_sand_var, 4 ) == 0
            && ((Primal_result *) (p_hte->data))->var->agg_type == SCALAR )
    {
        p_subr_obj->sand = TRUE;

        if ( ((Primal_result *) (p_hte->data))->var->num_type == M_FLOAT8 )
            p_mesh->double_precision_sand = TRUE;
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_set_results )
 *
 * Evaluate which derived result calculations may occur for
 * a Mili database and fill load-result table for derived
 * results.
 */
extern int
mili_db_set_results( Analysis *analy )
{
    Result_candidate *p_rc;
    int i, j, k, m;
    int qty_candidates;
    int pr_qty, subr_qty;
    Hash_table *p_pr_ht;
    Hash_table *p_dr_ht;
    Htable_entry *p_hte;
    int **counts;
    int rval;
    Primal_result *p_pr;
    int *p_i;
    Subrec_obj *p_subrecs;
    Mesh_data *meshes;
    int cand_primal_sclass, subrec_idx, found_th=FALSE;

    /* Sanity check. */
    if ( analy->primal_results == NULL )
        return OK;

    /* Count derived result candidates. */
    for ( qty_candidates = 0;
            possible_results[qty_candidates].superclass != QTY_SCLASS;
            qty_candidates++ );
    p_pr_ht = analy->primal_results;

    /* Base derived result table size on primal result table size. */
    if ( p_pr_ht->qty_entries < 151 )
        p_dr_ht = htable_create( 151 );
    else
        p_dr_ht = htable_create( 1009 );

    /* Create counts tree. */
    counts = NEW_N( int *, analy->qty_srec_fmts, "Counts tree trunk" );
    for ( j = 0; j < analy->qty_srec_fmts; j++ )
        counts[j] = NEW_N( int, analy->srec_tree[j].qty,
                           "Counts tree branch" );

    for ( i = 0; i < qty_candidates; i++ )
    {
        p_rc = &possible_results[i];

        /* Ensure mesh dimensionality matches result requirements. */
        if ( analy->dimension == 2 )
        {
            if ( !p_rc->dim.d2 )
                continue;
        }
        else
        {
            if ( !p_rc->dim.d3 )
                continue;
        }

        /*
         * Ensure there exists a mesh object class with a superclass
         * which matches the candidate.  Some results may be calculated
         * from results from a different class (i.e., hex strain from
         * nodal positions), so we need to make sure there's an object
         * class to provide the derived result on.
         */
        meshes = analy->mesh_table;

        for ( j = 0; j < analy->mesh_qty; j++ )
            if ( meshes[j].classes_by_sclass[p_rc->superclass].qty != 0 )
                break;
        if ( j == analy->mesh_qty )
            continue;

        /*
         * Loop over primal results required for current possible derived
         * result calculation; increment per-subrec counter for each
         * subrecord which contains each required result when the superclass
         * of the subrecord objects matches the superclass of the candidate
         * primal.
         */
        for ( j = 0; p_rc->primals[j] != NULL; j++ )
        {
            rval = htable_search( p_pr_ht, p_rc->primals[j], FIND_ENTRY,
                                  &p_hte );
            if ( rval == OK )
                p_pr = (Primal_result *) p_hte->data;
            else
                continue;


            found_th=FALSE;

            /* Look for nodal coordinates in other place for TH databases if
             * "noddpos" is missing.
             */

            /*           HEX
            */
            if ( !strcmp("hx",p_rc->primals[j]) && p_rc->superclass==M_HEX )
            {
                rval = htable_search( p_pr_ht, "nodpos", FIND_ENTRY,
                                      &p_hte );
                if ( rval == OK )
                    found_th=FALSE;
                else
                    found_th=TRUE;
            }

            /*           SHELL
            */
            if ( !strcmp("shlx",p_rc->primals[j]) && p_rc->superclass==M_QUAD )
            {
                rval = htable_search( p_pr_ht, "nodpos", FIND_ENTRY,
                                      &p_hte );
                if ( rval == OK )
                    found_th=FALSE;
                else
                    found_th=TRUE;
            }


            /* Primal precision must match candidate requirement. */
            if ( strcmp(p_pr->short_name, "nodpos")
                    && p_rc->single_precision_input
                    && p_pr->var->num_type != G_FLOAT
                    && p_pr->var->num_type != G_FLOAT4 )
                continue;

            cand_primal_sclass = p_rc->primal_superclasses[j];

            /*
             * Increment counts for subrecords which contain primal result.
             * and match the candidate primal superclass.
             */
            for ( k = 0; k < analy->qty_srec_fmts; k++ )
            {
                p_subrecs = analy->srec_tree[k].subrecs;
                p_i = (int *) p_pr->srec_map[k].list;
                subr_qty = p_pr->srec_map[k].qty;
                for ( m = 0; m < subr_qty; m++ )
                {
                    subrec_idx = p_i[m];

                    if ( cand_primal_sclass
                            == p_subrecs[subrec_idx].p_object_class->superclass )
                        counts[k][subrec_idx]++;
                }
            }
        }

        pr_qty = j;

        if (found_th)
            pr_qty=0;

        if ( pr_qty > 0 )
        {
            /*
             * Each subrecord count equal to the quantity of primals required
             * for current possible result indicates a subrecord on which
             * the result derivation can take place.
             */
            for ( j = 0; j < analy->qty_srec_fmts; j++ )
            {
                for ( k = 0; k < analy->srec_tree[j].qty; k++ )
                {
                    if ( counts[j][k] == pr_qty )
                        create_derived_results( analy, j, k, p_rc, p_dr_ht );
                }
            }
        }
        else
        {
            /*
             * No primal result dependencies for this derived result.  We
             * need to associate with a subrecord in order to generate a
             * correct menu entry, so search the subrecords for a superclass
             * match.
             *
             * This is a hack, because it depends on having data in the
             * database to give the derived result something to piggy-back
             * on, when there really is no dependency.  The current design
             * needs to be extended to allow for such "dynamic" results as
             * "pvmag" which is a nodal derived result but doesn't require
             * nodal data in the db to be calculable.  This will probably
             * be a necessary addition to support interpreted results
             * anyway.
             */

            for ( j = 0; j < analy->qty_srec_fmts; j++ )
            {
                p_subrecs = analy->srec_tree[j].subrecs;
                for ( k = 0; k < analy->srec_tree[j].qty; k++ )
                {
                    if ( p_subrecs[k].p_object_class->superclass
                            == p_rc->superclass )
                    {
                        create_derived_results( analy, j, k, p_rc, p_dr_ht );

                        /* Just do one per state record format. */
                        break;
                    }
                }
            }
        }

        /* Clear counts tree. */
        for ( j = 0; j < analy->qty_srec_fmts; j++ )
            for ( k = 0; k < analy->srec_tree[j].qty; k++ )
                counts[j][k] = 0;

    }

    for ( j = 0; j < analy->qty_srec_fmts; j++ )
        free( counts[j] );
    free( counts );

    analy->derived_results = p_dr_ht;

    return OK;
}


/************************************************************
 * TAG( create_derived_results )
 *
 * Create Derived_result entries for a db. Each result in
 * a Result_candidate struct can be calculated on the identified
 * subrecord of the identified state record format.
 */
static int
create_derived_results( Analysis *analy, int srec_id, int subrec_id,
                        Result_candidate *p_rc, Hash_table *p_ht )
{
    int rval;
    Htable_entry *p_hte;
    Derived_result *p_dr;
    int i, j;
    int qty_srecs, qty_subrecs;
    Subrecord_result *p_sr;
    Subrec_obj *p_subrec;
    Bool_type added;

    p_subrec = analy->srec_tree[srec_id].subrecs + subrec_id;
    if ( p_subrec->subrec.qty_objects==0 )
        return OK;

    qty_srecs = analy->qty_srec_fmts;

    for ( i = 0; p_rc->short_names[i] != NULL; i++ )
    {
        rval = htable_search( p_ht, p_rc->short_names[i], ENTER_MERGE, &p_hte );

        /* If new... */
        if ( rval == OK )
        {
            /* Create the Derived_result. */
            p_dr = NEW( Derived_result, "New DR" );

            p_dr->origin = p_rc->origin;

            if ( p_rc->superclass == M_NODE )
                p_dr->origin.is_node_result = 1;
            else if ( p_rc->superclass == M_MAT )
                p_dr->origin.is_mat_result = 1;
            else if ( p_rc->superclass == M_MESH )
                p_dr->origin.is_mesh_result = 1;
            else if ( p_rc->superclass == M_UNIT )
                p_dr->origin.is_unit_result = 1;
            else
                p_dr->origin.is_elem_result = 1;

            /* Initialize the state rec/subrec presence map. */
            p_dr->srec_map = NEW_N( List_head, qty_srecs, "DR srec map" );
            p_hte->data = (void *) p_dr;
        }
        else
            p_dr = (Derived_result *) p_hte->data;

        /* Add a new candidate reference to the derived result. */
        if ( p_dr->srec_map[srec_id].list == NULL )
        {
            /* First occurrence in this srec format. */
            p_sr = NEW( Subrecord_result, "Subrec result list" );
            p_sr->subrec_id = subrec_id;
            p_sr->index = i;
            p_sr->candidate = p_rc;
            p_dr->srec_map[srec_id].list = (void *) p_sr;
            p_dr->srec_map[srec_id].qty = 1;

            j = 0;
            added = TRUE;
        }
        else
        {
            /*
             * Traverse subrecord references to ensure this will be
             * the first entry of this subrec_id for this result.
             * Multiple entries in possible_results[] can exist if
             * a candidate can be derived from alternative sets of
             * primal results.  This logic keeps the first occurrence.
             *
             * Potential problem - If two different subrecords bind
             * the same objects with different state vars, and those
             * different state vars will enable the same derived
             * result, then a load_result() call for the result will
             * cause the calculation to occur twice, once with each
             * supporting set of primals (for example, nodal velocity
             * supported by either primal node velocity or primal
             * node position, with the primal components bound in
             * different subrecords).
             */
            p_sr = (Subrecord_result *) p_dr->srec_map[srec_id].list;
            qty_subrecs = p_dr->srec_map[srec_id].qty;
            for ( j = 0; j < qty_subrecs; j++ )
                if ( p_sr[j].subrec_id == subrec_id )
                    break;

            if ( j == qty_subrecs )
            {
                p_sr = RENEW_N( Subrecord_result, p_sr, j, 1,
                                "Extend subrec result list" );
                p_sr[j].subrec_id = subrec_id;
                p_sr[j].index = i;
                p_sr[j].candidate = p_rc;
                p_sr[j].indirect = FALSE;

                /* Assign back in case realloc moved the array. */
                p_dr->srec_map[srec_id].list = (void *) p_sr;
                p_dr->srec_map[srec_id].qty++;
                added = TRUE;
            }
            else
                added = FALSE;
        }

        if ( added )
        {
            p_subrec = analy->srec_tree[srec_id].subrecs + subrec_id;

            if ( p_rc->superclass != p_subrec->p_object_class->superclass )
            {
                p_sr[j].indirect = TRUE;
                p_dr->has_indirect = TRUE;
            }

        }
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_get_state )
 *
 * Seek to a particular state in a Mili database and
 * update nodal positions for the mesh.
 */
extern int
mili_db_get_state( Analysis *analy, int state_no, State2 *p_st,
                   State2 **pp_new_st, int *state_qty )
{
    int st_qty;
    float st_time;
    State_variable sv;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrecs, *p_subrec;
    int i, j;
    int dbid;
    int srec_id, mesh_id;
    int dims;
    int ec_index;
    int st;
    int rval;
    int subrec_size;
    double *p_double;
    float *p_single;
    void *input_buf, *ibuf;
    int *object_ids;
    Mesh_data *p_md;
    char *primal, *svar;
    int infoStringId=0, lenInfoString=0;
    char infoString[256];
    float *p_float;

    /*
     * Variables:  "nodes3d", "src_nodes3d",
     *             "nodes2d", "src_nodes2d",
     *             "sand_in", "sand_out"
     *
     * are declared but never referenced  LAS
    GVec3D *nodes3d, *src_nodes3d;
    GVec2D *nodes2d, *src_nodes2d;
    float *sand_in, *sand_out;
    */

    dbid = analy->db_ident;

    rval = mc_query_family( dbid, QTY_STATES, NULL, NULL, (void *) &st_qty );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_get_state() call "
                        "mc_query_family( QTY_STATES )", rval );
        return GRIZ_FAIL;
    }

    rval = mc_query_family( dbid, QTY_DIMENSIONS, NULL, NULL, (void *) &dims );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_get_state() call "
                        "mc_query_family( QTY_DIMENSIONS )", rval );
        return GRIZ_FAIL;
    }

    /* Pass back the current quantity of states in the db. */
    if ( state_qty != NULL )
        *state_qty = st_qty;

    if ( st_qty == 0 || analy->qty_srec_fmts == 0 )
    {
        if ( p_st == NULL )
            p_st = mk_state2( analy, NULL, dims, 0, st_qty, p_st );

        /* No states, so use node positions from geometry definition. */
        p_st->nodes = analy->mesh_table->node_geom->objects;

        p_st->position_constant = TRUE;

        *pp_new_st = p_st;
        return OK;
    }

    if ( state_no < 0 || state_no >= st_qty )
    {
        popup_dialog( WARNING_POPUP,
                      "Get-state request for nonexistent state." );
        *pp_new_st = p_st;
        return GRIZ_FAIL;
    }
    else
        st = state_no + 1;

    rval = mc_query_family( dbid, SREC_FMT_ID, (void *) &st, NULL,
                            (void *) &srec_id );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_get_state() call "
                        "mc_query_family( SREC_FMT_ID )", rval );
        return GRIZ_FAIL;
    }

    p_sro = analy->srec_tree + srec_id;
    p_subrecs = p_sro->subrecs;

    mesh_id = p_subrecs[0].p_object_class->mesh_id;

    p_md = analy->mesh_table + mesh_id;

    /* Update or create State2 struct. */
    p_st = mk_state2( analy, p_sro, dims, srec_id, st_qty, p_st );

    p_st->state_no = state_no;

    rval = mc_query_family( dbid, STATE_TIME, (void *) &st, NULL,
                            (void *) &st_time );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_get_state() call "
                        "mc_query_family( STATE_TIME )", rval );
        return GRIZ_FAIL;
    }

    p_st->time = st_time;

    /* Read node position array if it exists, re-ordering if necessary. */
    if ( analy->stateDB )
        load_nodpos( analy, p_sro, p_md, dims, st, TRUE, p_st->nodes.nodes );

    /* Read sand flags if they exist, re-ordering if necessary. */
    /* Also check for string results with will be displayed as messages in GUI */
    analy->infoMsgCnt=0;

    for ( i = 0; i < p_sro->qty; i++ )
    {
        p_subrec = p_subrecs + i;
        if ( p_subrec->subrec.qty_svars>0 )
        {
            svar = p_subrec->subrec.svar_names[0];

            rval = mc_get_svar_def( dbid, svar, &sv );
            if ( rval != 0 )
            {
                mc_print_error( "mc_get_svar_def()", rval );
                return GRIZ_FAIL;
            }

            if (sv.num_type == M_STRING )
            {
                rval = mc_read_results( dbid, st, i, 1, &svar,
                                        (void *)infoString );
                if ( rval==0 )
                {
                    int intVal=0;
                    if ( analy->infoMsg[analy->infoMsgCnt]!=NULL )
                        free(analy->infoMsg[analy->infoMsgCnt] );
                    lenInfoString = p_subrec->subrec.qty_objects;
                    infoString[100]='\0';
                    for ( j=0;
                            j<lenInfoString-2;
                            j++ )
                    {
                        intVal = (int) infoString[j];
                        if ( intVal<32 || intVal>126 )
                        {
                            infoString[j] = '\0';
                            break;
                        }
                    }

                    analy->infoMsg[analy->infoMsgCnt] = NEW_N( char, lenInfoString, "New Info Message" );
                    strcpy( analy->infoMsg[analy->infoMsgCnt], infoString );
                    analy->infoMsgCnt++;
                }
            }
        }

        if ( p_subrecs[i].sand )
        {
            p_subrec = p_subrecs + i;

            /*  primal = p_subrec->subrec.name;  OLD Method of finding Sand vars looking for the sub-record named sand */
            primal = "sand";

            object_ids = p_subrec->object_ids;
            ec_index = p_subrec->p_object_class->elem_class_index;
            if ( object_ids == NULL )
            {
                if ( !p_md->double_precision_sand )
                {
                    rval = mc_read_results( dbid, st, i, 1, &primal,
                                            (void *) p_st->elem_class_sand[ec_index] );
                    if ( rval != 0 )
                    {
                        mc_print_error( "mili_db_get_state() call "
                                        "mc_read_results() for \"sand\"",
                                        rval );
                        return GRIZ_FAIL;
                    }
                }
                else
                {
                    subrec_size = p_subrec->subrec.qty_objects;
                    input_buf = get_st_input_buffer( analy, subrec_size,
                                                     TRUE, &ibuf );

                    rval = mc_read_results( dbid, st, i, 1, &primal,
                                            input_buf );
                    if ( rval != 0 )
                    {
                        mc_print_error( "mili_db_get_state() call "
                                        "mc_read_results() for "
                                        "double precision \"sand\"",
                                        rval );
                        if ( ibuf != NULL )
                            free( ibuf );

                        return GRIZ_FAIL;
                    }

                    /* Convert to single precision. */
                    p_double = (double *) input_buf;
                    p_single = p_st->elem_class_sand[ec_index];
                    for ( i = 0; i < subrec_size; i++ )
                        p_single[i] = (float) p_double[i];

                    if ( ibuf != NULL )
                        free( ibuf );
                }
            }
            else
            {
                subrec_size = p_subrec->subrec.qty_objects;
                input_buf = get_st_input_buffer( analy, subrec_size,
                                                 p_md->double_precision_sand,
                                                 &ibuf );

                rval = mc_read_results( dbid, st, i, 1, &primal, input_buf );
                if ( rval != 0 )
                {
                    mc_print_error( "mili_db_get_state() call "
                                    "mc_read_results() for \"sand\"", rval );
                    if ( ibuf != NULL )
                        free( ibuf );

                    return GRIZ_FAIL;
                }

                if ( p_md->double_precision_sand )
                {
                    /* Convert in place to single precision. */
                    p_double = (double *) input_buf;
                    p_single = (float *) input_buf;
                    for ( i = 0; i < subrec_size; i++ )
                        p_single[i] = (float) p_double[i];
                }

                reorder_float_array( p_subrec->subrec.qty_objects, object_ids,
                                     1, input_buf,
                                     p_st->elem_class_sand[ec_index] );

                if ( ibuf != NULL )
                    free( ibuf );
            }
        }
    }

    if ( p_st->sph_present )
    {
        primal     = "sph_itype";
        p_subrec   = p_subrecs + p_st->sph_srec_id;
        object_ids = p_subrec->object_ids;
        subrec_size = p_subrec->subrec.qty_objects;
        input_buf = get_st_input_buffer( analy, subrec_size,
                                         TRUE, &ibuf );
        rval = mc_read_results( dbid, st, p_st->sph_srec_id, 1, &primal,
                                input_buf );
        p_float = (float *) input_buf;
        for ( i=0;
                i<subrec_size;
                i++ )
            p_st->sph_class_itype[i] = (int) p_float[i];

        if ( ibuf != NULL )
            free( ibuf );
    }

    /* If nodal positions weren't part of state data, get from geometry. */
    if ( !analy->stateDB )
    {
        p_st->nodes = p_md->node_geom->objects;

        p_st->position_constant = TRUE;
    }

    /* Read particle position array if it exists, re-ordering if necessary. */
    if ( p_sro->particle_pos_subrec != -1 )
    {
        /*
         * Read particle position array.
         * Note that references to "particles2d" union field could just as
         * easily be to the "particles3d" field (just want the contents).
         */
        p_subrec = p_subrecs + p_sro->particle_pos_subrec;
        object_ids = p_subrec->object_ids;
        primal = "partpos";
        if ( object_ids == NULL )
        {
            if ( !p_md->double_precision_partpos )
            {
                rval = mc_read_results( dbid, st, p_sro->particle_pos_subrec, 1,
                                        &primal,
                                        (void *) p_st->particles.particles2d );
                if ( rval != 0 )
                {
                    mc_print_error( "mili_db_get_state() call "
                                    "mc_read_results() for \"partpos\"", rval );
                    return GRIZ_FAIL;
                }
            }
            else
            {
                subrec_size = p_subrec->subrec.qty_objects * dims;
                input_buf = get_st_input_buffer( analy, subrec_size, TRUE,
                                                 &ibuf );

                rval = mc_read_results( dbid, st, p_sro->particle_pos_subrec, 1,
                                        &primal, input_buf );
                if ( rval != 0 )
                {
                    mc_print_error( "mili_db_get_state() call "
                                    "mc_read_results() for double precision "
                                    "\"partpos\"", rval );
                    if ( ibuf != NULL )
                        free( ibuf );

                    return GRIZ_FAIL;
                }

                /* Convert to single precision. */
                p_double = (double *) input_buf;
                p_single = (float *) p_st->particles.particles2d;
                for ( i = 0; i < subrec_size; i++ )
                    p_single[i] = (float) p_double[i];

                if ( ibuf != NULL )
                    free( ibuf );
            }
        }
        else
        {
            subrec_size = p_subrec->subrec.qty_objects * dims;
            input_buf = get_st_input_buffer( analy, subrec_size,
                                             p_md->double_precision_partpos,
                                             &ibuf );

            rval = mc_read_results( dbid, st, p_sro->particle_pos_subrec, 1,
                                    &primal, input_buf );
            if ( rval != 0 )
            {
                mc_print_error( "mili_db_get_state() call "
                                "mc_read_results() for \"partpos\"", rval );
                if ( ibuf != NULL )
                    free( ibuf );

                return GRIZ_FAIL;
            }

            if ( p_md->double_precision_partpos )
            {
                /* Convert in place to single precision. */
                p_double = (double *) input_buf;
                p_single = (float *) input_buf;
                for ( i = 0; i < subrec_size; i++ )
                    p_single[i] = (float) p_double[i];
            }

            reorder_float_array( p_subrec->subrec.qty_objects, object_ids,
                                 dims, input_buf,
                                 (float *) p_st->particles.particles2d );

            if ( ibuf != NULL )
                free( ibuf );
        }
    }

    *pp_new_st = p_st;
    return OK;
}


/************************************************************
 * TAG( load_nodpos )
 *
 * Load nodal position data from a specified state, converting
 * to single precision if necessary.
 */
extern int
load_nodpos( Analysis *analy, State_rec_obj *p_sro, Mesh_data *p_md,
             int dimensions, int db_state, Bool_type limit_single_prec,
             void *node_buf )
{
    Subrec_obj *p_subrec, *p_subrecs;
    int *object_ids;
    char *primal;
    int rval, subrec_size, i;
    void *input_buf, *ibuf;
    double *p_double;
    float  *p_single;

    p_subrecs = p_sro->subrecs;
    p_subrec = p_subrecs + p_sro->node_pos_subrec;
    object_ids = p_subrec->object_ids;
    primal = "nodpos";
    if ( object_ids == NULL )
    {
        if ( !p_md->double_precision_nodpos || !limit_single_prec )
        {
            /*
             * If nodpos is single precision, or if it's double precision and
             * double precision is OK, just use the caller's data buffer and
             * get the data and we're done.
             */

            rval = mc_read_results( analy->db_ident, db_state,
                                    p_sro->node_pos_subrec, 1, &primal,
                                    node_buf );
            if ( rval != 0 )
            {
                mc_print_error( "load_nodpos() call mc_read_results()"
                                " for \"nodpos\"", rval );
                return GRIZ_FAIL;
            }
        }
        else
        {
            /* Double precision data, but caller wants single precision data. */

            subrec_size = p_subrec->subrec.qty_objects * dimensions;

            /* Get an input buffer. */
            input_buf = get_st_input_buffer( analy, subrec_size, TRUE,
                                             &ibuf );

            /* Read in the data. */
            rval = mc_read_results( analy->db_ident, db_state,
                                    p_sro->node_pos_subrec, 1, &primal,
                                    (void *) input_buf );
            if ( rval != 0 )
            {
                mc_print_error( "load_nodpos() call mc_read_results()"
                                " for double precision \"nodpos\"", rval );
                if ( ibuf != NULL )
                    free( ibuf );

                return GRIZ_FAIL;
            }

            /* Convert to single precision. */
            p_double = input_buf;
            p_single = node_buf;
            for ( i = 0; i < subrec_size; i++ )
                p_single[i] = (float) p_double[i];

            if ( ibuf != NULL )
                free( ibuf );
        }
    }
    else
    {
        /* Get an input buffer. */
        subrec_size = p_subrec->subrec.qty_objects * dimensions;
        input_buf = get_st_input_buffer( analy, subrec_size,
                                         p_md->double_precision_nodpos,
                                         &ibuf );

        rval = mc_read_results( analy->db_ident, db_state,
                                p_sro->node_pos_subrec, 1, &primal, input_buf );
        if ( rval != 0 )
        {
            mc_print_error( "load_nodpos() call mc_read_results()"
                            " for \"nodpos\"", rval );
            if ( ibuf != NULL )
                free( ibuf );

            return GRIZ_FAIL;
        }

        if ( limit_single_prec )
        {
            if ( p_md->double_precision_nodpos )
            {
                /* Convert in place to single precision. */
                p_double = (double *) input_buf;
                p_single = (float *) input_buf;
                for ( i = 0; i < subrec_size; i++ )
                    p_single[i] = (float) p_double[i];
            }

            reorder_float_array( p_subrec->subrec.qty_objects, object_ids,
                                 dimensions, input_buf, (float *) node_buf );
        }
        else
        {
            /* Assume data is double precision, but warn if not. */
            reorder_double_array( p_subrec->subrec.qty_objects, object_ids,
                                  dimensions, input_buf, (double *) node_buf );

            if ( !p_md->double_precision_nodpos )
                popup_dialog( WARNING_POPUP, "Double precision node position"
                              " request with single precision primal data" );
        }

        if ( ibuf != NULL )
            free( ibuf );
    }

    return OK;
}



/************************************************************
 * TAG( load_hex_nodpos_timehist )
 *
 *  Load Hex nodal position data from a time history database,
 *  converting to single precision if necessary. This function
 *  will return the list of element numbers in an array called
 *  obj_map and the number of elements returned in obj_cnt.
 */
extern int
load_hex_nodpos_timehist( Analysis *analy, int state, int single_precision,
                          int *obj_cnt, int **obj_map, GVec3D2P **new_nodes)
{
    int   axis_index, obj_index, i, j;

    int   dbid = analy->db_ident;
    int   rval;
    int   subrec_coord_index, subrec_size;

    void    *ibuf;
    float   *input_buf_flt;
    double  *input_buf_dbl;

    int     idnum, *object_ids;
    int     class_qty, node_qty;
    int     nd, (*connects)[8];

    MO_class_data *p_hex_class;

    State_rec_obj *p_state_rec;
    Subrec_obj    *p_subr_obj;
    Subrecord     *p_subr;
    State_variable *p_stvar;
    Famid fid;
    int matnum=1;
    int ordering;

    char *axis_string[3] = {"hx", "hy", "hz"};
    char *primal[2];
    char  primal_coord[64];

    Bool_type hexcoord_found=FALSE;

    int      *obj_map_tmp;
    GVec3D2P *new_nodes_tmp;

    fid = analy->db_ident;
    p_state_rec = analy->srec_tree;

    for ( j = 0;
            j < p_state_rec->qty;
            j++ )
    {
        p_subr = &p_state_rec->subrecs[j].subrec;

        if (!strcmp(p_subr->class_name, "brick") &&
                (!strcmp(p_subr->name, "hexcoord") || strstr(p_subr->name, "hex_th_coord")) )
        {
            subrec_coord_index = j;
            hexcoord_found     = TRUE;
        }
    }

    if (!hexcoord_found)
    {
        *obj_map = NULL;
        return (!OK);
    }


    p_subr      = &p_state_rec->subrecs[subrec_coord_index].subrec;
    ordering    = p_subr->organization;
    object_ids  = p_state_rec->subrecs[subrec_coord_index].object_ids;
    subrec_size = p_subr->qty_objects;
    p_stvar = NEW( State_variable, "New state var" );
    rval = mc_get_svar_def( fid, "hx", p_stvar );
    if ( rval != 0 )
    {
        mc_print_error( "mc_get_svar_def()", rval );
        return GRIZ_FAIL;
    }

    if (p_stvar->num_type != M_FLOAT8 )
        input_buf_flt = get_st_input_buffer( analy, subrec_size,
                                             FALSE, &ibuf );
    else
        input_buf_dbl = get_st_input_buffer( analy, subrec_size,
                                             TRUE, &ibuf );

    class_qty = MESH( analy ).classes_by_sclass[G_HEX].qty;
    node_qty  = MESH_P( analy )->node_geom->qty;

    /* Allocate space for new coordinates */
    new_nodes_tmp  = NEW_N_MALLOC( GVec3D2P,
                                   node_qty,
                                   "new nodes 3d" );

    obj_map_tmp    = NEW_N( int,
                            subrec_size+2,
                            "new nodes - object map" );

    *obj_cnt = subrec_size;

    for ( i = 0;
            i < subrec_size+2;
            i++ )
        obj_map_tmp[i] = -1;

    for ( j = 0;
            j < class_qty;
            j++ )
    {
        p_hex_class = ((MO_class_data **)
                       MESH( analy ).classes_by_sclass[G_HEX].list)[j];

        connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    }

    primal[0] = primal_coord;

    /* Object Ordered Results */
    /* X Coordinates */

    if ( ordering == OBJECT_ORDERED )
        for (axis_index = 0;
                axis_index < 3;
                axis_index++)
            for (j = 0;
                    j < 8;
                    j++)
            {
                sprintf(primal_coord,"hexcoord[%s,%1d]", axis_string[axis_index], j+1);

                if (p_stvar->num_type != M_FLOAT8 )
                    rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                            (void *) input_buf_flt );
                else
                    rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                            (void *) input_buf_dbl );

                if ( rval != 0 )
                {
                    mc_print_error( "load_nodpos_timehist() call mc_read_results()"
                                    " for \"hexcoord[hx,1]\"", rval );
                    return GRIZ_FAIL;
                }

                for ( i = 0;
                        i < subrec_size;
                        i++ )

                {
                    if( object_ids )
                        idnum = object_ids[i];
                    else
                        idnum = i;

                    obj_map_tmp[i] = idnum;

                    nd = connects[idnum][j];

                    if ( nd >= node_qty )
                    {
                        mc_print_error( "load_nodpos_timehist() node qty exceeded for nd ", nd);
                        return GRIZ_FAIL;
                    }


                    if (p_stvar->num_type != M_FLOAT8 )
                        new_nodes_tmp[nd][axis_index] = input_buf_flt[i];
                    else
                        new_nodes_tmp[nd][axis_index] = input_buf_dbl[i];
                }
            }

    /* RESULT_ORDERED */
    else
        for (axis_index = 0;
                axis_index < 3;
                axis_index++)
        {
            sprintf(primal_coord,"hexcoord[%s]", axis_string[axis_index]);

            if (p_stvar->num_type != M_FLOAT8 )
                rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                        (void *) input_buf_flt );
            else
                rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                        (void *) input_buf_dbl );


            if ( rval != 0 )
            {
                sprintf(primal_coord, "%s", axis_string[axis_index]);

                if (p_stvar->num_type != M_FLOAT8 )
                    rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                            (void *) input_buf_flt );
                else
                    rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                            (void *) input_buf_dbl );
            }

            if ( rval != 0 )
            {
                mc_print_error( "load_nodpos_timehist() call mc_read_results()"
                                " for \"hexcoord[hx]\"", rval );
                return GRIZ_FAIL;
            }

            obj_index = 0;
            for ( i = 0;
                    i < subrec_size;
                    i++ )

            {
                if( object_ids )
                {
                    idnum = object_ids[i];
                }
                else
                    idnum = i;

                obj_map_tmp[i] = idnum;

                for (j = 0;
                        j < 8;
                        j++)
                {
                    nd = connects[idnum][j];

                    if ( nd >= node_qty )
                    {
                        mc_print_error( "load_hex_nodpos_timehist() node qty exceeded for nd ", nd);
                        return GRIZ_FAIL;
                    }

                    if (p_stvar->num_type != M_FLOAT8 )
                        new_nodes_tmp[nd][axis_index] = input_buf_flt[obj_index+j];
                    else
                        new_nodes_tmp[nd][axis_index] = input_buf_dbl[obj_index+j];
                }
                obj_index+=8;
            }
        }

    *obj_map   = obj_map_tmp;
    *new_nodes = new_nodes_tmp;

    return (OK);
}


/************************************************************
 * TAG( load_quad_nodpos_timehist )
 *
 *  Load Quad nodal position data from a time history database,
 *  converting to single precision if necessary. This function
 *  will return the list of element numbers in an array called
 *  obj_map and the number of elements returned in obj_cnt.
 */
extern int
load_quad_nodpos_timehist( Analysis *analy, int state,  int single_precision,
                           int *obj_cnt, int **obj_map, GVec3D2P **new_nodes)
{
    int   axis_index, obj_index, i, j;

    int   dbid = analy->db_ident;
    int   rval;
    int   subrec_coord_index, subrec_conn_index, subrec_size;

    void    *ibuf;
    float   *input_buf_flt;
    double  *input_buf_dbl;

    int     idnum, *object_ids;
    int     class_qty, node_qty;
    int     nd, (*connects)[4];

    MO_class_data *p_quad_class;

    State_rec_obj *p_state_rec;
    Subrec_obj    *p_subr_obj;
    Subrecord     *p_subr;
    State_variable *p_stvar;
    Famid fid;
    int ordering;

    char *axis_string[3] = {"shlx", "shly", "shlz"};

    char *primal[2];
    char  primal_coord[64];

    Bool_type quadcoord_found=FALSE;

    int      *obj_map_tmp;
    GVec3D2P *new_nodes_tmp;

    fid = analy->db_ident;
    p_state_rec = analy->srec_tree;

    for ( j = 0;
            j < p_state_rec->qty;
            j++ )
    {
        p_subr = &p_state_rec->subrecs[j].subrec;

        if (!strcmp(p_subr->class_name, "shell") &&
                (!strcmp(p_subr->name, "shlcoord") || !strcmp(p_subr->name, "shell_th_coord") ||
                 strstr(p_subr->name, "shell_th_coord")) )
        {
            quadcoord_found = TRUE;
            subrec_coord_index = j;
        }
    }

    if (!quadcoord_found)
    {
        *obj_map = NULL;
        return (!OK);
    }

    p_subr      = &p_state_rec->subrecs[subrec_coord_index].subrec;
    ordering    = p_subr->organization;
    object_ids  = p_state_rec->subrecs[subrec_coord_index].object_ids;
    subrec_size = p_subr->qty_objects;
    p_stvar = NEW( State_variable, "New state var" );

    rval = mc_get_svar_def( fid, "shlx", p_stvar );
    if ( rval != 0 )
    {
        mc_print_error( "mc_get_svar_def()", rval );
        return GRIZ_FAIL;
    }

    if (p_stvar->num_type != M_FLOAT8 )
        input_buf_flt = get_st_input_buffer( analy, subrec_size,
                                             FALSE, &ibuf );
    else
        input_buf_dbl = get_st_input_buffer( analy, subrec_size,
                                             TRUE, &ibuf );

    class_qty = MESH( analy ).classes_by_sclass[G_QUAD].qty;
    node_qty  = MESH_P( analy )->node_geom->qty;

    /* Allocate space for new coordinates */
    new_nodes_tmp = NEW_N_MALLOC( GVec3D2P,
                                  node_qty,
                                  "new nodes 3d" );

    obj_map_tmp = NEW_N( int,
                         subrec_size+2,
                         "new nodes - object map" );

    *obj_cnt =  subrec_size;

    for ( i = 0;
            i < subrec_size+2 ;
            i++ )
        obj_map_tmp[i] = -1;

    for ( j = 0;
            j < class_qty;
            j++ )
    {
        p_quad_class = ((MO_class_data **)
                        MESH( analy ).classes_by_sclass[G_QUAD].list)[j];

        connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
    }

    primal[0] = primal_coord;

    /* Object Ordered Results */
    /* X Coordinates */

    if ( ordering == OBJECT_ORDERED )
        for (axis_index = 0;
                axis_index < 3;
                axis_index++)
            for (j = 0;
                    j < 8;
                    j++)
            {
                sprintf(primal_coord,"shlcoord[%s,%1d]", axis_string[axis_index], j+1);

                if (p_stvar->num_type != M_FLOAT8 )
                    rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                            (void *) input_buf_flt );
                else
                    rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                            (void *) input_buf_dbl );

                if ( rval != 0 )
                {
                    mc_print_error( "load_quad_nodpos_timehist() call mc_read_results()"
                                    " for \"quadcoord[hx,1]\"", rval );
                    return GRIZ_FAIL;
                }

                for ( i = 0;
                        i < subrec_size;
                        i++ )

                {
                    if( object_ids )
                        idnum = object_ids[i];
                    else
                        idnum = i;

                    obj_map_tmp[i] = idnum;

                    nd = connects[idnum][j];

                    if ( nd >= node_qty )
                    {
                        mc_print_error( "load_quad_nodpos_timehist() node qty exceeded for nd ", nd);
                        return GRIZ_FAIL;
                    }


                    if (p_stvar->num_type != M_FLOAT8 )
                        new_nodes_tmp[nd][axis_index] = input_buf_flt[i];
                    else
                        new_nodes_tmp[nd][axis_index] = input_buf_dbl[i];
                }
            }
    /* RESULT_ORDERED */
    else
        for (axis_index = 0;
                axis_index < 3;
                axis_index++)
        {
            sprintf(primal_coord,"shlcoord[%s]", axis_string[axis_index]);

            if (p_stvar->num_type != M_FLOAT8 )
                rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                        (void *) input_buf_flt );
            else
                rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                        (void *) input_buf_dbl );


            if ( rval != 0 )
            {
                sprintf(primal_coord, "%s", axis_string[axis_index]);

                if (p_stvar->num_type != M_FLOAT8 )
                    rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                            (void *) input_buf_flt );
                else
                    rval = mc_read_results( dbid, state, subrec_coord_index, 1, primal,
                                            (void *) input_buf_dbl );
            }

            if ( rval != 0 )
            {
                mc_print_error( "load_quad_nodpos_timehist() call mc_read_results()"
                                " for \"shlcoord[shlx]\"", rval );
                return GRIZ_FAIL;
            }

            obj_index = 0;
            for ( i = 0;
                    i < subrec_size;
                    i++ )

            {
                if( object_ids )
                    idnum = object_ids[i];
                else
                    idnum = i;

                obj_map_tmp[i] = idnum;

                for (j = 0;
                        j < 4;
                        j++)
                {
                    nd = connects[idnum][j];

                    if ( nd >= node_qty )
                    {
                        mc_print_error( "load_quad_nodpos_timehist() node qty exceeded for nd ", nd);
                        return GRIZ_FAIL;
                    }

                    if (p_stvar->num_type != M_FLOAT8 )
                        new_nodes_tmp[nd][axis_index] = input_buf_flt[obj_index+j];
                    else
                        new_nodes_tmp[nd][axis_index] = input_buf_dbl[obj_index+j];
                }
                obj_index+=4;
            }
        }

    *obj_map   = obj_map_tmp;
    *new_nodes = new_nodes_tmp;

    return (OK);
}

/************************************************************
 * TAG( load_quad_timehist_obj_ids )
 *
 *  Load Quad time-history object id list and object count.
 *
 */
extern int
load_quad_objids_timehist( Analysis *analy, int *obj_cnt, int **obj_ids )
{
    int  i;
    int *temp_ids;
    int   subrec_coord_index, subrec_conn_index, subrec_size;

    int     class_qty, node_qty;
    MO_class_data *p_quad_class;

    State_rec_obj *p_state_rec;
    Subrec_obj    *p_subr_obj;
    Subrecord     *p_subr;
    State_variable *p_stvar;

    Bool_type quadcoord_found=FALSE;

    p_state_rec = analy->srec_tree;

    for ( i = 0;
            i < p_state_rec->qty;
            i++ )
    {
        p_subr = &p_state_rec->subrecs[i].subrec;

        if (!strcmp(p_subr->name, "shlcoord") || strstr(p_subr->name, "hex_th_coord"))
        {
            quadcoord_found = TRUE;
            subrec_coord_index = i;
        }
    }

    if (!quadcoord_found)
    {
        *obj_ids = NULL;
        return (OK);
    }

    *obj_cnt = p_subr->qty_objects;
    *obj_ids = p_state_rec->subrecs[subrec_coord_index].object_ids;

    return (OK);
}


/************************************************************
 * TAG( get_st_input_buffer )
 *
 * Get a buffer for mili_db_get_state() to read state-dependent data into.
 * This function abstracts the buffer source, which is either
 * analy->tmp_result (preferred) or locally allocated memory, and returns
 * that.  If locally allocated, that is returned in *new_buf so that the
 * caller can unambiguously know when to free() the buffer (i.e., the
 * function call always returns a non-NULL address, but *new_buf will be
 * NULL or non-NULL depending on whether the caller is to free() or not).
 */
void *
get_st_input_buffer( Analysis *analy, int size, Bool_type double_precision,
                     void **new_buf )
{
    double *dbuf;
    float *fbuf;

    if ( analy->tmp_result[0] != NULL )
    {
        *new_buf = NULL;
        return analy->tmp_result[0];
    }

    if ( double_precision )
    {
        dbuf = NEW_N( double, size, "Temp State2 double input buffer" );
        *new_buf = dbuf;
        return dbuf;
    }
    else
    {
        fbuf = NEW_N( float, size, "Temp State2 float input buffer" );
        *new_buf = fbuf;
        return fbuf;
    }
}


/************************************************************
 * TAG( mk_state2 )
 *
 * Update or allocate, then initialize a State2 struct for the
 * specified state record format.
 */
State2 *
mk_state2( Analysis *analy, State_rec_obj *p_sro, int dims, int srec_id,
           int qty_states, State2 *p_st )
{
    State2 *p_state;
    Bool_type sand;
    int nsubr, sub;
    int i, j;
    int qty;
    float **pp_f;
    int mesh_id;
    int elem_class_index, elem_class_qty;
    Subrec_obj *p_subrecs;
    Mesh_data *p_md;
    MO_class_data *p_class;

    int sph_qty = 0;

    /* Get the mesh_id and qty of element classes via one of the subrecs. */
    if ( p_sro != NULL )
    {
        p_subrecs = p_sro->subrecs;
        mesh_id = p_subrecs[0].p_object_class->mesh_id;
        p_md = analy->mesh_table + mesh_id;
        elem_class_qty = p_md->elem_class_qty;
    }

    if ( p_st == NULL )
    {
        if ( (p_state = NEW( State2, "State2 struct" )) == NULL )
            popup_fatal( "Can't allocate space for State2." );

        if ( qty_states == 0 )
            return p_state;
    }
    else
    {
        if ( srec_id == p_st->srec_id || qty_states == 0 )
        {
            /* No format change so just reset time and state_no. */
            p_st->time = 0.0;
            p_st->state_no = 0;

            return p_st;
        }
        else
        {
            /* Cleanup prior to allocating new node and sand arrays. */
            free( p_st->nodes.nodes );

            if ( p_st->sand_present && p_st->elem_class_sand != NULL )
            {
                for ( i = 0; i < elem_class_qty; i++ )
                    if ( p_st->elem_class_sand[i] != NULL )
                        free( p_st->elem_class_sand[i] );

                free( p_st->elem_class_sand );

                p_st->sand_present = FALSE;
            }

            if ( p_st->particles.particles2d != NULL )
                free( (void *) p_st->particles.particles2d );

            p_state = p_st;
        }
    }

    /* Count subrecords and find nodal data subrecord. */
    nsubr = -1;
    sand = FALSE;
    p_state->sph_present     = FALSE;
    p_state->sph_class_itype = NULL;

    for ( qty = 0; qty < p_sro->qty; qty++ )
    {
        if ( nsubr < 0
                && strcmp( p_subrecs[qty].subrec.class_name, "node" ) == 0 )
            nsubr = qty;

        if ( p_subrecs[qty].sand )
            sand = TRUE;

        if ( strcmp( p_subrecs[qty].subrec.class_name, "sph" ) == 0 )
        {
            if (p_subrecs[qty].subrec.qty_svars>0 )
            {
                for ( j=0;
                        j<p_subrecs[qty].subrec.qty_svars;
                        j++ )
                    if ( strcmp("sph_itype", p_subrecs[qty].subrec.svar_names[j])==0 )
                    {
                        p_state->sph_present = TRUE;
                        p_state->sph_srec_id = qty;
                        sph_qty = p_subrecs[qty].subrec.qty_objects;
                    }
            }
        }
    }

    if ( p_state->sph_present && sph_qty>0 )
    {
        p_state->sph_class_itype = NEW_N( int, sph_qty, "State2 subrec sph id" );
    }


    if ( sand )
    {
        /*
         * Update to below: mili_db_get_state() does not require proper
         * subrecords, or even complete ones.  If a subrecord is
         * incomplete, the class sand array can still be filled as long
         * as the elements bound to multiple subrecords form a complete
         * class.
         */

        /*
         * Sand arrays are required to be in "proper" subrecords, i.e.,
         * the subrecords bind all mesh objects in the class in 1-to-n
         * order.  Since the sand flag is really a class-wide element
         * property, they potentially exist for all element classes in
         * a mesh.  But actual existence is determined by the subrecords
         * defined in a particular state record format.  So, we allocate
         * an array of arrays equal to the number of element classes in
         * the mesh, but for any given format, only the arrays for those
         * classes which have sand flags are instantiated.  This makes
         * use of the element class indices and quantities which were
         * derived when classes were first read into Griz, which elsewhere
         * in Griz obviates searching and matching class names to locate
         * sand data.
         */

        p_state->sand_present = TRUE;

        /* Allocate pointers for sand arrays. */
        pp_f = NEW_N( float *, elem_class_qty, "State2 subrec sand" );
        for( i = 0; i < elem_class_qty; i++ )
            pp_f[i] = NULL;

        /* Pass through again and initialize for sand data. */
        for ( i = 0; i < qty; i++ )
            if ( p_subrecs[i].sand )
            {
                p_class = p_subrecs[i].p_object_class;

                elem_class_index = p_class->elem_class_index;

                /*
                 * Allocate sand array by class size to support both
                 * complete and proper subrecords as well as the case of class
                 * support distributed across several subrecords.
                 */
                if ( pp_f[elem_class_index] == NULL )
                    pp_f[elem_class_index] = NEW_N( float, p_class->qty,
                                                    "State2 sand array" );

                if ( pp_f[elem_class_index] == NULL )
                    popup_fatal( "Can't allocate space for sand flags." );
            }

        p_state->elem_class_sand = pp_f;
    }

    if ( nsubr < 0 )
        p_state->nodes.nodes = NULL;
    else
    {
        if ( analy->dimension == 2 )
            p_state->nodes.nodes2d = NEW_N( GVec2D,
                                            p_subrecs[nsubr].subrec.qty_objects,
                                            "State2 nodes 2d" );
        else
            p_state->nodes.nodes3d = NEW_N( GVec3D,
                                            p_subrecs[nsubr].subrec.qty_objects,
                                            "State2 nodes 3d" );

        if ( p_state->nodes.nodes3d == NULL )
            popup_fatal( "Can't allocate space for State2 nodes." );
    }

    /* Particle positions. */
    if ( p_sro->particle_pos_subrec != -1 )
    {
        sub = p_sro->particle_pos_subrec;

        if ( analy->dimension == 2 )
            p_state->particles.particles2d = NEW_N(
                                                 GVec2D,
                                                 p_subrecs[sub].subrec.qty_objects,
                                                 "State2 particles 2d" );
        else
            p_state->particles.particles3d = NEW_N(
                                                 GVec3D,
                                                 p_subrecs[sub].subrec.qty_objects,
                                                 "State2 particles 3d" );
    }

    p_state->srec_id = srec_id;

    return p_state;
}


/************************************************************
 * TAG( mili_db_get_subrec_def )
 *
 * Fill a Subrecord structure for a Mili database.
 */
extern int
mili_db_get_subrec_def( int dbid, int srec_id, int subrec_id,
                        Subrecord *p_subrec )
{
    int rval;

    rval = mc_get_subrec_def( dbid, srec_id, subrec_id, p_subrec );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_get_subrec_def() call mc_get_subrec_def()",
                        rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_cleanse_subrec )
 *
 * Free dynamically allocated memory for a Subrecord structure.
 */
extern int
mili_db_cleanse_subrec( Subrecord *p_subrec )
{
    int rval;

    rval = mc_cleanse_subrec( p_subrec );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_cleanse_subrec() call mc_cleanse_subrec()",
                        rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_get_results )
 *
 * Load primal results from a Mili database.
 */
extern int
mili_db_get_results( int dbid, int state, int subrec_id, int qty,
                     char **results, void *data )
{
    int i=0;
    int errorCnt=0;
    int result_qty=0;
    int rval;
    float *vals;

    rval = mc_read_results( dbid, state, subrec_id, qty, results, data );
    if ( rval != 0 )
    {
        mc_print_error( "mili_db_get_results() call mc_read_results()", rval );
        return GRIZ_FAIL;
    }

    /* Check all result data for Nan */
    if ( env.checkresults )
    {
        result_qty = get_result_qty( env.curr_analy, subrec_id );
        vals = (float *) data;
        for ( i=0;
                i<result_qty;
                i++ )
        {
            if ( isnan(vals[i]) )
            {
                if ( errorCnt==0 )
                    popup_dialog( WARNING_POPUP, "Bad result found for %s at index %d. Resetting to 1.0e30", results[0], i );
                if ( errorCnt<10 )
                    printf("\nError: Bad result found for %s at index %d. Resetting to 1.0e30", results[0], i);
                vals[i] = 1.0e30;
                errorCnt++;
            }
        }
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_query )
 *
 * Query a Mili database.
 */
extern int
mili_db_query( int dbid, int query_type, void *num_args, char *char_arg,
               void *p_info )
{
    int rval;

    rval = mc_query_family( dbid, query_type, num_args, char_arg, p_info );
    if ( rval != 0 )
    {
        if ( rval!=INVALID_STATE )
            mc_print_error( "mili_db_query() call mc_query_family()", rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_get_title )
 *
 * Load the title from a Mili database.
 */
extern int
mili_db_get_title( int dbid, char *title_bufr )
{
    int rval;

    rval = mc_read_string( dbid, "title", title_bufr );
    if ( rval != OK )
    {
        mc_print_error( "mili_db_get_title() call mc_read_string()", rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_get_dimension )
 *
 * Get the mesh dimensionality from a Mili database.
 */
extern int
mili_db_get_dimension( int dbid, int *p_dim )
{
    int rval;

    rval = mc_query_family( dbid, QTY_DIMENSIONS, NULL, NULL, (void *) p_dim );
    if ( rval != OK )
    {
        mc_print_error( "mili_db_get_dimension() call mc_query_family()",
                        rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_set_buffer_qty )
 *
 * Set the quantity of states' data buffered in memory.
 */
extern int
mili_db_set_buffer_qty( int dbid, int mesh_id, char *class_name, int buf_qty )
{
    int rval;

    rval = mc_set_buffer_qty( dbid, mesh_id, class_name, buf_qty );
    if ( rval != OK )
    {
        mc_print_error( "mili_db_set_buffer_qty() call mc_set_buffer_qty()",
                        rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( mili_db_close )
 *
 * Free resources associated with a Mili database and close
 * the database.
 *
 * A lot of this should probably be moved into close_analysis().
 */
extern int
mili_db_close( Analysis *analy )
{
    Famid fid;
    Subrec_obj *p_subrec;
    State_rec_obj *p_srec;
    Mesh_data *p_mesh;
    int i, j;
    int rval;

    fid = analy->db_ident;

    rval = OK;

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
            rval = mc_cleanse_subrec( &p_subrec[j].subrec );
            if ( rval != OK )
                popup_dialog( WARNING_POPUP,
                              "mili_db_close() - unable to cleanse srec %d "
                              "subrec %d.", i, j );

            if ( p_subrec[j].object_ids != NULL )
                free( p_subrec[j].object_ids );

            if ( p_subrec[j].referenced_nodes != NULL )
                free( p_subrec[j].referenced_nodes );
        }

        free( p_subrec );
    }

    if ( p_srec != NULL )
    {
        free( p_srec );
        analy->srec_tree = NULL;
    }

    /* Free the state variable hash table. */
    if ( analy->st_var_table != NULL )
    {
        htable_delete( analy->st_var_table, st_var_delete, TRUE );
        analy->st_var_table = NULL;
    }

    /* Free the primal result hash table. */
    if ( analy->primal_results != NULL )
    {
        htable_delete( analy->primal_results, delete_primal_result, TRUE );
        analy->primal_results = NULL;
    }

    /* Free the derived result hash table. */
    if ( analy->derived_results != NULL )
    {
        htable_delete( analy->derived_results, delete_derived_result, TRUE );
        analy->derived_results = NULL;
    }

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
     * Close the Mili database.
     */

    rval = mc_close( fid );
    if ( rval != OK )
    {
        mc_print_error( "mili_db_close() call mc_close()", rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( taurus_db_open )
 *
 * Open a Taurus plotfile database family.
 */
extern int
taurus_db_open( char *path_root, int *p_dbid )
{
    char *p_c;
    char *p_root_start, *p_root_end;
    char *p_src, *p_dest;
    char *path;
    char root[512];
    char path_text[256];
    int rval;
    Famid dbid;

    /* Scan forward to end of name string. */
    for ( p_c = path_root; *p_c != '\0'; p_c++ );

    /* Scan backward to last non-slash character. */
    for ( p_c--; *p_c == '/' && p_c != path_root; p_c-- );
    p_root_end = p_c;

    /* Scan backward to last slash character preceding last non-slash char. */
    for ( ; *p_c != '/' && p_c != path_root; p_c-- );

    p_root_start = ( *p_c == '/' ) ? p_c + 1 : p_c;

    /* Generate the path argument to taurus_open(). */
    if ( p_root_start == path_root )
        /* No path preceding root name. */
        path = NULL;
    else
    {
        /* Copy path (less last slash). */

        path = path_text;

        for ( p_src = path_root, p_dest = path_text;
                p_src < p_root_start - 1;
                *p_dest++ = *p_src++ );

        if ( p_src == path_root )
            /* Path must be just "/".  If that's what the app wants... */
            *p_dest++ = *path_root;

        *p_dest = '\0';
    }

    /* Generate root name argument to taurus_open(). */
    for ( p_src = p_root_start, p_dest = root;
            p_src <= p_root_end;
            *p_dest++ = *p_src++ );
    *p_dest = '\0';

    rval = taurus_open( root, path, "r", &dbid );

    if ( rval == 0 )
        *p_dbid = dbid;

    return rval;
}

/************************************************************
 * TAG( taurus_db_get_geom )
 *
 * Load the geometry (mesh definition) data from a Taurus
 * plotfile database family.
 */
extern int
taurus_db_get_geom( int dbid, Mesh_data **p_mtable, int *p_mesh_qty )
{
    Hash_table *p_ht;
    Mesh_data *mesh_array;
    Htable_entry *p_hte;
    MO_class_data *p_mocd;
    MO_class_data **mo_classes;
    char short_name[M_MAX_NAME_LEN], long_name[M_MAX_NAME_LEN];
    Elem_data *p_ed;
    Material_data *p_matd;
    Mesh_data *p_md;
    int mesh_qty, qty_classes;
    int obj_qty;
    int i, j, k;
    int int_args[3];
    int dims;
    static int elem_sclasses[] =
    {
        M_TRUSS, M_BEAM, M_TRI, M_QUAD, M_TET, M_PYRAMID, M_WEDGE, M_HEX, M_PARTICLE
    };
    int qty_esclasses, elem_class_count;
    static int qty_connects[] =
    {
        2, 3, 3, 4, 4, 5, 6, 8, 1
    };
    Bool_type have_Nodal;
    List_head *p_lh;
    int start_ident, stop_ident;
    int rval;

    if ( *p_mtable != NULL )
    {
        popup_dialog( WARNING_POPUP,
                      "Mesh table pointer not NULL at initialization." );
        return 1;
    }

    /* Get dimensionality of data. */
    rval = mc_query_family( dbid, QTY_DIMENSIONS, NULL, NULL, (void *) &dims );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                        rval );
        return GRIZ_FAIL;
    }

    /* Get number of meshes in family. */
    rval = mc_query_family( dbid, QTY_MESHES, NULL, NULL, (void *) &mesh_qty );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                        rval );
        return GRIZ_FAIL;
    }

    /* Allocate array of pointers to mesh geom hash tables. */
    mesh_array = NEW_N( Mesh_data, mesh_qty, "Mesh data array" );

    qty_esclasses = sizeof( elem_sclasses ) / sizeof( elem_sclasses[0] );

    for ( i = 0; i < mesh_qty; i++ )
    {
        /* Allocate a hash table for the mesh geometry. */
        p_md = mesh_array + i;
        p_ht = htable_create( 151 );
        p_md->class_table = p_ht;

        /*
         * Load nodal coordinates.
         *
         * This logic loads all M_NODE classes, although Griz only wants
         * or expects to deal with a single class "node".  However, if
         * the app that wrote the db bound variables to an M_NODE class
         * other than "node", this will allow the class to exist and
         * keep the state descriptor initialization logic flowing.
         */

        int_args[0] = i;
        int_args[1] = M_NODE;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != OK )
        {
            mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                            rval );
            return GRIZ_FAIL;
        }

        have_Nodal = FALSE;

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_class_info( dbid, i, M_NODE, j, short_name, long_name,
                                      &obj_qty );
            if ( rval != OK )
            {
                mc_print_error( "taurus_db_get_geom() call mc_get_class_info()",
                                rval );
                return GRIZ_FAIL;
            }

            /* Create mesh geometry table entry. */
            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Nodes geom table entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_NODE;
            p_mocd->elem_class_index = -1;
            p_mocd->qty        = obj_qty;
            p_mocd->labels     = NULL;
            p_mocd->labels_max = obj_qty;

            if ( dims == 3 )
                p_mocd->objects.nodes3d = NEW_N( GVec3D, obj_qty,
                                                 "3D node coord array" );
            else
                p_mocd->objects.nodes2d = NEW_N( GVec2D, obj_qty,
                                                 "2D node coord array" );

            p_hte->data = (void *) p_mocd;

#ifdef TIME_OPEN_ANALYSIS
            printf( "Timing nodal coords read...\n" );
            manage_timer( 0, 0 );
#endif

            /* Now load the coordinates array. */
            taurus_load_nodes( dbid, i, short_name,
                               (void *) p_mocd->objects.nodes3d );

            /* Allocate the data buffer for scalar I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, obj_qty, "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );

#ifdef TIME_OPEN_ANALYSIS
            manage_timer( 0, 1 );
            putc( (int) '\n', stdout );
#endif

            if ( strcmp( short_name, "node" ) == 0 )
            {
                have_Nodal = TRUE;

                /* Keep a reference to node geometry handy. */
                p_md->node_geom = p_mocd;
            }

            /* Update the mesh references by superclass. */
            p_lh = p_md->classes_by_sclass + M_NODE;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *,
                                  mo_classes, p_lh->qty, 1,
                                  "Extend node sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;
        }

        if ( !have_Nodal )
        {
            popup_dialog( WARNING_POPUP,
                          "Griz requires a node object class \"node\"." );
            htable_delete( p_ht, delete_mo_class_data, TRUE );

            return OK;
        }

        /*
         * Load element connectivities.
         */

#ifdef TIME_OPEN_ANALYSIS
        printf( "Timing connectivity load and sort into geom...\n" );
        manage_timer( 0, 0 );
#endif

        elem_class_count = 0;
        int_args[0] = i;
        for ( j = 0; j < qty_esclasses; j++ )
        {
            int_args[1] = elem_sclasses[j];
            rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS,
                                    (void *) int_args, NULL,
                                    (void *) &qty_classes );
            if ( rval != OK )
            {
                mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                                rval );
                return GRIZ_FAIL;
            }

            for ( k = 0; k < qty_classes; k++ )
            {
                rval = mc_get_class_info( dbid, i, elem_sclasses[j], k,
                                          short_name, long_name, &obj_qty );
                if ( rval != OK )
                {
                    mc_print_error( "taurus_db_get_geom() call "
                                    "mc_get_class_info()", rval );
                    return GRIZ_FAIL;
                }

                /* Create mesh geometry table entry. */
                htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

                p_mocd = NEW( MO_class_data, "Elem geom table entry" );

                p_mocd->mesh_id = i;
                griz_str_dup( &p_mocd->short_name, short_name );
                griz_str_dup( &p_mocd->long_name, long_name );
                p_mocd->superclass = elem_sclasses[j];
                p_mocd->elem_class_index = elem_class_count++;
                p_mocd->qty = obj_qty;
                p_mocd->labels_max = obj_qty;

                p_ed = NEW( Elem_data, "Element conn struct" );
                p_mocd->objects.elems = p_ed;
                p_ed->nodes = NEW_N( int, obj_qty * qty_connects[j],
                                     "Element connectivities" );
                p_ed->mat = NEW_N( int, obj_qty, "Element materials" );
                p_ed->part = NEW_N( int, obj_qty, "Element parts" );

                /* Allocate the data buffer for I/O and result derivation. */
                p_mocd->data_buffer = NEW_N( float, obj_qty,
                                             "Class data buffer" );
                if ( p_mocd->data_buffer == NULL )
                    popup_fatal( "Unable to alloc data buffer on class load" );

                p_hte->data = (void *) p_mocd;

                taurus_load_conns( dbid, i, short_name, p_ed->nodes, p_ed->mat,
                                   p_ed->part );

                /* Element superclass-specific actions. */
                switch ( elem_sclasses[j] )
                {
                case M_HEX:
                    check_degen_hexs( p_mocd );
                    p_lh = p_md->classes_by_sclass + M_HEX;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend hex sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                    /* #ifdef HAVE_WEDGE_PYRAMID */
                case M_WEDGE:
                    popup_dialog( INFO_POPUP, "%s\n%s (class \"%s\")",
                                  "Checking for degenerate wedge elements",
                                  "is not implemented.", short_name );
                    p_lh = p_md->classes_by_sclass + M_WEDGE;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend wedge sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_PYRAMID:
                    popup_dialog( INFO_POPUP, "%s\n%s (class \"%s\")",
                                  "Checking for degenerate pyramid",
                                  "elements is not implemented.",
                                  short_name );
                    p_lh = p_md->classes_by_sclass + M_PYRAMID;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend pyramid sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                    /* #endif */
                case M_TET:
                    popup_dialog( INFO_POPUP, "%s\n%s (class \"%s\")",
                                  "Checking for degenerate tet elements",
                                  "is not implemented.", short_name );
                    p_lh = p_md->classes_by_sclass + M_TET;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend tet sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_QUAD:
                    check_degen_quads( p_mocd );
                    p_lh = p_md->classes_by_sclass + M_QUAD;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend quad sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_TRI:
                    check_degen_tris( p_mocd );
                    if ( p_mocd->objects.elems->has_degen )
                        popup_dialog( INFO_POPUP, "%s\n(class \"%s\").",
                                      "Degenerate tri element(s) detected",
                                      short_name );
                    p_lh = p_md->classes_by_sclass + M_TRI;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend tri sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_BEAM:
                    popup_dialog( INFO_POPUP, "%s\n%s (class \"%s\")",
                                  "Checking for degenerate beam elements",
                                  "is not implemented.", short_name );
                    p_lh = p_md->classes_by_sclass + M_BEAM;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend beam sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                case M_TRUSS:
                    popup_dialog( INFO_POPUP, "%s\n%s (class \"%s\")",
                                  "Checking for degenerate truss elements",
                                  "is not implemented.", short_name );
                    p_lh = p_md->classes_by_sclass + M_TRUSS;
                    mo_classes = (MO_class_data **) p_lh->list;
                    mo_classes = (void *)
                                 RENEW_N( MO_class_data *,
                                          mo_classes, p_lh->qty, 1,
                                          "Extend truss sclass array" );
                    mo_classes[p_lh->qty] = p_mocd;
                    p_lh->qty++;
                    p_lh->list = (void *) mo_classes;
                    break;
                default:
                    /* do nothing */
                    ;
                }
            }
        }

#ifdef TIME_OPEN_ANALYSIS
        manage_timer( 0, 1 );
        putc( (int) '\n', stdout );
#endif

        /* Update the Mesh_data struct with element class info. */
        p_md->elem_class_qty = elem_class_count;

        /*
         * Load simple mesh object classes.
         */

        /* M_UNIT classes. */

        int_args[0] = i;
        int_args[1] = M_UNIT;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != OK )
        {
            mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                            rval );
            return GRIZ_FAIL;
        }

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_simple_class_info( dbid, i, M_UNIT, j, short_name,
                                             long_name, &start_ident,
                                             &stop_ident );
            if ( rval != OK )
            {
                mc_print_error( "taurus_db_get_geom() call "
                                "mc_get_simple_class_info()", rval );
                return GRIZ_FAIL;
            }

            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Simple class geom entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_UNIT;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = stop_ident - start_ident + 1;
            p_mocd->labels_max = p_mocd->qty;
            p_mocd->simple_start = start_ident;
            p_mocd->simple_stop = stop_ident;

            /* Allocate the data buffer for I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, p_mocd->qty,
                                         "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );

            p_lh = p_md->classes_by_sclass + M_UNIT;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *, mo_classes, p_lh->qty, 1,
                                  "Extend unit sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;

            p_hte->data = (void *) p_mocd;
        }

        /* M_MAT classes. */

        int_args[1] = M_MAT;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != OK )
        {
            mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                            rval );
            return GRIZ_FAIL;
        }

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_simple_class_info( dbid, i, M_MAT, j, short_name,
                                             long_name, &start_ident,
                                             &stop_ident );
            if ( rval != OK )
            {
                mc_print_error( "taurus_db_get_geom() call "
                                "mc_get_simple_class_info()", rval );
                return GRIZ_FAIL;
            }

            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Simple class geom entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_MAT;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = stop_ident - start_ident + 1;
            p_mocd->labels_max = p_mocd->qty;
            p_mocd->simple_start = start_ident;
            p_mocd->simple_stop = stop_ident;

            p_matd = NEW_N( Material_data, p_mocd->qty, "Material data array" );
            p_mocd->objects.materials = p_matd;
            gen_material_data( p_mocd, p_md );

            /* Allocate the data buffer for I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, p_mocd->qty,
                                         "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );

            p_lh = p_md->classes_by_sclass + M_MAT;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *, mo_classes, p_lh->qty, 1,
                                  "Extend material sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;

            p_hte->data = (void *) p_mocd;
        }

        /* M_MESH classes. */

        int_args[1] = M_MESH;
        rval = mc_query_family( dbid, QTY_CLASS_IN_SCLASS, (void *) int_args,
                                NULL, (void *) &qty_classes );
        if ( rval != OK )
        {
            mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                            rval );
            return GRIZ_FAIL;
        }

        for ( j = 0; j < qty_classes; j++ )
        {
            rval = mc_get_simple_class_info( dbid, i, M_MESH, j, short_name,
                                             long_name, &start_ident,
                                             &stop_ident );
            if ( rval != OK )
            {
                mc_print_error( "taurus_db_get_geom() call "
                                "mc_get_simple_class_info()", rval );
                return GRIZ_FAIL;
            }

            htable_search( p_ht, short_name, ENTER_ALWAYS, &p_hte );

            p_mocd = NEW( MO_class_data, "Simple class geom entry" );

            p_mocd->mesh_id = i;
            griz_str_dup( &p_mocd->short_name, short_name );
            griz_str_dup( &p_mocd->long_name, long_name );
            p_mocd->superclass = M_MESH;
            p_mocd->elem_class_index = -1;
            p_mocd->qty = stop_ident - start_ident + 1;
            p_mocd->labels_max = p_mocd->qty;

            p_mocd->simple_start = start_ident;
            p_mocd->simple_stop = stop_ident;

            /* Allocate the data buffer for I/O and result derivation. */
            p_mocd->data_buffer = NEW_N( float, p_mocd->qty,
                                         "Class data buffer" );
            if ( p_mocd->data_buffer == NULL )
                popup_fatal( "Unable to allocate data buffer on class load" );

            p_lh = p_md->classes_by_sclass + M_MESH;
            mo_classes = (MO_class_data **) p_lh->list;
            mo_classes = (void *)
                         RENEW_N( MO_class_data *, mo_classes, p_lh->qty, 1,
                                  "Extend mesh sclass array" );
            mo_classes[p_lh->qty] = p_mocd;
            p_lh->qty++;
            p_lh->list = (void *) mo_classes;

            p_hte->data = (void *) p_mocd;
        }
    }

    /* Pass back address of geometry hash table array. */
    *p_mtable = mesh_array;
    *p_mesh_qty = mesh_qty;

    return (OK);
}


/************************************************************
 * TAG( taurus_db_get_st_descriptors )
 *
 * Query a database and store information about the available
 * state record formats and referenced state variables.
 */
extern int
taurus_db_get_st_descriptors( Analysis *analy, int dbid )
{
    int srec_qty, svar_qty, subrec_qty, mesh_node_qty;
    Famid fid;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrecs;
    Subrecord *p_subr;
    int rval;
    Hash_table *p_sv_ht, *p_primal_ht;
    int i, j, k;
    char **svar_names;
    int mesh_id;
    Htable_entry *p_hte;
    int class_size;
    MO_class_data *p_mocd;
    Bool_type nodal;
    int *node_work_array;

    fid = (Famid) dbid;

    /* Get state record format count for this database. */
    rval = mc_query_family( fid, QTY_SREC_FMTS, NULL, NULL,
                            (void *) &srec_qty );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_get_st_descriptors() call mc_query_family()",
                        rval );
        return GRIZ_FAIL;
    }

    /* Allocate array of state record structs. */
    p_sro = NEW_N( State_rec_obj, srec_qty, "Srec tree" );

    /* Get state variable quantity for this database. */
    rval = mc_query_family( fid, QTY_SVARS, NULL, NULL, (void *) &svar_qty );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_get_st_descriptors() call mc_query_family()",
                        rval );
        return GRIZ_FAIL;
    }

    if ( svar_qty > 0 )
    {
        if ( svar_qty < 100 )
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

    /* Loop over srecs */
    for ( i = 0; i < srec_qty; i++ )
    {
        /* Get subrecord count for this state record. */
        rval = mc_query_family( fid, QTY_SUBRECS, (void *) &i, NULL,
                                (void *) &subrec_qty );
        if ( rval != OK )
        {
            mc_print_error( "taurus_db_get_st_descriptors() call "
                            "mc_query_family()", rval );
            return GRIZ_FAIL;
        }

        /* Allocate array of Subrec_obj's. */
        p_subrecs = NEW_N( Subrec_obj, subrec_qty, "Srec subrec branch" );
        p_sro[i].subrecs = p_subrecs;
        p_sro[i].qty = subrec_qty;

        /* Init nodal position and velocity subrec indices to invalid values. */
        p_sro[i].node_pos_subrec = -1;
        p_sro[i].node_vel_subrec = -1;

        /* Get mesh_id for this srec. */
        rval = mc_query_family( fid, SREC_MESH, (void *) &i, NULL,
                                (void *) &mesh_id );
        if ( rval != OK )
        {
            mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                            rval );
            return GRIZ_FAIL;
        }

        /* Allocate a temporary working array for subrec node list creation. */
        mesh_node_qty = analy->mesh_table[mesh_id].node_geom->qty;
        node_work_array = NEW_N( int, mesh_node_qty, "Temp node array" );

        /* Loop over subrecs */
        for ( j = 0; j < subrec_qty; j++ )
        {
            p_subr = &p_subrecs[j].subrec;

            /* Get binding */
            taurus_get_subrec_def( fid, i, j, p_subr );

            htable_search( analy->mesh_table[mesh_id].class_table,
                           p_subr->class_name, FIND_ENTRY, &p_hte );
            p_mocd = ((MO_class_data *) p_hte->data);
            p_subrecs[j].p_object_class = p_mocd;

            /* Create list of nodes referenced by objects bound to subrecord. */
            create_subrec_node_list( node_work_array, mesh_node_qty,
                                     p_subrecs + j );

            /*
             * Create ident array if indexing is required.
             */

            class_size = p_mocd->qty;

            if ( p_subr->qty_objects != class_size
                    || ( class_size > 1
                         && ( p_subr->qty_blocks != 1
                              || p_subr->mo_blocks[0] != 1 ) ) )
            {
                /*
                 * Subrecord does not completely and continuously represent
                 * object class, so an index map is needed to get from
                 * sequence number in the subrecord to an object ident.
                 */
                p_subrecs[j].object_ids = NEW_N( int, p_subr->qty_objects,
                                                 "Subrec ident map array" );
                blocks_to_list( p_subr->qty_blocks, p_subr->mo_blocks,
                                p_subrecs[j].object_ids, TRUE );
            }
            else
                p_subrecs[j].object_ids = NULL;

            /*
             * M_NODE class "node" is special - need it for node positions
             * and velocities.
             */
            if ( strcmp( p_subr->class_name, "node" ) == 0 )
                nodal = TRUE;
            else
                nodal = FALSE;

            /*
             * Loop over svars and create state variable and primal result
             * table entries.
             */
            svar_names = p_subr->svar_names;
            for ( k = 0; k < p_subr->qty_svars; k++ )
            {
                create_st_variable( fid, p_sv_ht, svar_names[k], NULL );
                create_primal_result( analy->mesh_table + mesh_id, i, j,
                                      p_subrecs + j, p_primal_ht, srec_qty,
                                      svar_names[k], p_sv_ht );

                if ( nodal )
                {
                    if ( strcmp( svar_names[k], "nodpos" ) == 0 )
                    {
                        if ( p_sro[i].node_pos_subrec != -1 )
                            popup_dialog( WARNING_POPUP,
                                          "Multiple \"node\" position subrecs." );
                        p_sro[i].node_pos_subrec = j;
                    }
                    else if ( strcmp( svar_names[k], "nodvel" ) == 0 )
                    {
                        if ( p_sro[i].node_vel_subrec != -1 )
                            popup_dialog( WARNING_POPUP,
                                          "Multiple \"node\" velocity subrecs." );
                        p_sro[i].node_vel_subrec = j;
                    }
                }
            }
        }

        free( node_work_array );
    }

    /*
     * Return the subrecord tree,state variable hash table, and primal result
     * hash table.
     */
    analy->srec_tree = p_sro;
    analy->qty_srec_fmts = srec_qty;
    analy->st_var_table = p_sv_ht;
    analy->primal_results = p_primal_ht;

    return OK;
}

/************************************************************
 * TAG( taurus_db_set_results )
 *
 * Evaluate which derived result calculations may occur for
 * a database and fill load-result table for derived
 * results.
 */
extern int
taurus_db_set_results( Analysis *analy )
{
    Result_candidate *p_rc;
    int i, j, k, m;
    int qty_candidates;
    int pr_qty, subr_qty;
    Hash_table *p_pr_ht;
    Hash_table *p_dr_ht;
    Htable_entry *p_hte;
    int **counts;
    int rval;
    Primal_result *p_pr;
    Subrec_obj *p_subrecs;
    int *p_i;
    Mesh_data *meshes;

    /* Count derived result candidates. */
    for ( qty_candidates = 0;
            possible_results[qty_candidates].superclass != QTY_SCLASS;
            qty_candidates++ );
    p_pr_ht = analy->primal_results;

    /* Base derived result table size on primal result table size. */
    if ( p_pr_ht->qty_entries < 151 )
        p_dr_ht = htable_create( 151 );
    else
        p_dr_ht = htable_create( 1009 );

    /* Create counts tree. */
    counts = NEW_N( int *, analy->qty_srec_fmts, "Counts tree trunk" );
    for ( j = 0; j < analy->qty_srec_fmts; j++ )
        counts[j] = NEW_N( int, analy->srec_tree[j].qty,
                           "Counts tree branch" );

    for ( i = 0; i < qty_candidates; i++ )
    {
        p_rc = &possible_results[i];

        /* Ensure mesh dimensionality matches result requirements. */
        if ( analy->dimension == 2 )
        {
            if ( !p_rc->dim.d2 )
                continue;
        }
        else
        {
            if ( !p_rc->dim.d3 )
                continue;
        }

        /*
         * Ensure there exists a mesh object class with a superclass
         * which matches the candidate.  Some results may be calculated
         * from results from a different class (i.e., hex strain from
         * nodal positions), so we need to make sure there's an object
         * class to provide the derived result on.
         */
        meshes = analy->mesh_table;

        for ( j = 0; j < analy->mesh_qty; j++ )
            if ( meshes[j].classes_by_sclass[p_rc->superclass].qty != 0 )
                break;

        if ( j == analy->mesh_qty )
            continue;

        /*
         * Loop over primal results required for current possible derived
         * result calculation; increment per-subrec counter for each
         * subrecord which contains each required result.
         */
        for ( j = 0; p_rc->primals[j] != NULL; j++ )
        {
            rval = htable_search( p_pr_ht, p_rc->primals[j], FIND_ENTRY,
                                  &p_hte );
            if ( rval == OK )
                p_pr = (Primal_result *) p_hte->data;
            else
                continue;

            /* Increment counts for subrecords which contain primal result. */
            for ( k = 0; k < analy->qty_srec_fmts; k++ )
            {
                p_i = (int *) p_pr->srec_map[k].list;
                subr_qty = p_pr->srec_map[k].qty;
                for ( m = 0; m < subr_qty; m++ )
                    counts[k][p_i[m]]++;
            }
        }

        pr_qty = j;
        if ( pr_qty > 0 )
        {
            /*
             * Each subrecord count equal to the quantity of primals required
             * for current possible result indicates a subrecord on which
             * the result derivation can take place.
            */
            for ( j = 0; j < analy->qty_srec_fmts; j++ )
            {
                for ( k = 0; k < analy->srec_tree[j].qty; k++ )
                {
                    if ( counts[j][k] == pr_qty )
                        create_derived_results( analy, j, k, p_rc, p_dr_ht );
                }
            }
        }
        else
        {
            /*
             * No primal result dependencies for this derived result.  We
             * need to associate with a subrecord in order to generate a
             * correct menu entry, so search the subrecords for a superclass
             * match.
             *
             * This is a hack, because it depends on having data in the
             * database to give the derived result something to piggy-back
             * on, when there really is no dependency.  The current design
             * needs to be extended to allow for such "dynamic" results as
             * "pvmag" which is a nodal derived result but doesn't require
             * nodal data in the db to be calculable.  This will probably
             * be a necessary addition to support interpreted results
             * anyway.
             */

            for ( j = 0; j < analy->qty_srec_fmts; j++ )
            {
                p_subrecs = analy->srec_tree[j].subrecs;
                for ( k = 0; k < analy->srec_tree[j].qty; k++ )
                {
                    if ( p_subrecs[k].p_object_class->superclass
                            == p_rc->superclass )
                    {
                        create_derived_results( analy, j, k, p_rc, p_dr_ht );

                        /* Just do one per state record format. */
                        break;
                    }
                }
            }
        }
        /* Clear counts tree. */
        for ( j = 0; j < analy->qty_srec_fmts; j++ )
            for ( k = 0; k < analy->srec_tree[j].qty; k++ )
                counts[j][k] = 0;

    }

    for ( j = 0; j < analy->qty_srec_fmts; j++ )
        free( counts[j] );
    free( counts );

    analy->derived_results = p_dr_ht;

    return OK;
}


/************************************************************
 * TAG( taurus_db_get_state )
 *
 * Seek to a particular state in a Taurus plotfile database
 * and update nodal positions for the mesh.
 */
extern int
taurus_db_get_state( Analysis *analy, int state_no, State2 *p_st,
                     State2 **pp_new_st, int *state_qty )
{
    /*
     * Just return mili_db_get_state() call value since it will evaluate
     * return value from any mc...() calls.
     */
    return ( mili_db_get_state( analy, state_no, p_st, pp_new_st, state_qty ) );
}


/************************************************************
 * TAG( taurus_db_get_subrec_def )
 *
 * Fill a Subrecord structure for a Taurus database.
 */
extern int
taurus_db_get_subrec_def( int dbid, int srec_id, int subrec_id,
                          Subrecord *p_subrec )
{
    /*
     * Do taurus_...() calls follow same error conventions as mc_...() calls?
     * If so, we should capture the return value here to call
     * mc_print_error(), then return OK/GRIZ_FAIL.
     */
    return taurus_get_subrec_def( dbid, srec_id, subrec_id, p_subrec );
}


/************************************************************
 * TAG( taurus_db_cleanse_subrec )
 *
 * Free dynamically allocated memory for a Subrecord structure.
 */
extern int
taurus_db_cleanse_subrec( Subrecord *p_subrec )
{
    int rval;

    rval = mc_cleanse_subrec( p_subrec );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_cleanse_subrec() call mc_cleanse_subrec()",
                        rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( taurus_db_get_results )
 *
 * Load primal results from a Taurus plotfile database.
 */
extern int
taurus_db_get_results( int dbid, int state, int subrec_id, int qty,
                       char **results, void *data )
{
    int rval;

    rval = mc_read_results( dbid, state, subrec_id, qty, results, data );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_get_results() call mc_read_results()",
                        rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( taurus_db_query )
 *
 * Query a taurus database.
 */
extern int
taurus_db_query( int dbid, int query_type, void *num_args, char *char_arg,
                 void *p_info )
{
    int rval;

    rval = mc_query_family( dbid, query_type, num_args, char_arg, p_info );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_query() call mc_query_family()", rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( taurus_db_get_title )
 *
 * Load the title from a Taurus plotfile database.
 */
extern int
taurus_db_get_title( int dbid, char *title_bufr )
{
    int rval;

    rval = mc_read_string( dbid, "title", title_bufr );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_get_title() call mc_read_string()", rval );
        return GRIZ_FAIL;
    }

    fix_title( title_bufr );

    return OK;
}


/************************************************************
 * TAG( taurus_db_get_dimension )
 *
 * Get the mesh dimensionality from a Taurus plotfile database.
 */
extern int
taurus_db_get_dimension( int db_ident, int *p_dim )
{
    int rval;

    rval = mc_query_family( db_ident, QTY_DIMENSIONS, NULL, NULL,
                            (void *) p_dim );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_get_dimension() call mc_query_family()",
                        rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( taurus_db_set_buffer_qty )
 *
 * Set the quantity of states' data buffered in memory.
 */
extern int
taurus_db_set_buffer_qty( int dbid, int mesh_id, char *class_name, int buf_qty )
{
    int rval;

    rval = mc_set_buffer_qty( dbid, mesh_id, class_name, buf_qty );
    if ( rval != OK )
    {
        mc_print_error( "taurus_db_set_buffer_qty() call mc_set_buffer_qty()",
                        rval );
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( taurus_db_close )
 *
 * Close Taurus plotfile database.
 */
extern int
taurus_db_close( Analysis *analy )
{
    Famid fid;
    Subrec_obj *p_subrec;
    State_rec_obj *p_srec;
    Mesh_data *p_mesh;
    int i, j;
    int rval;

    fid = analy->db_ident;

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
            rval = mc_cleanse_subrec( &p_subrec[j].subrec );
            if ( rval != OK )
                mc_print_error( "taurus_db_close() call mc_cleanse_subrec()",
                                rval );

            if ( p_subrec[j].object_ids != NULL )
                free( p_subrec[j].object_ids );

            if ( p_subrec[j].referenced_nodes != NULL )
                free( p_subrec[j].referenced_nodes );
        }

        free( p_subrec );
    }

    if ( p_srec != NULL )
    {
        free( p_srec );
        analy->srec_tree = NULL;
    }

    /* Free the state variable hash table. */
    if ( analy->st_var_table != NULL )
    {
        htable_delete( analy->st_var_table, st_var_delete, TRUE );
        analy->st_var_table = NULL;
    }

    /* Free the primal result hash table. */
    if ( analy->primal_results != NULL )
    {
        htable_delete( analy->primal_results, delete_primal_result, TRUE );
        analy->primal_results = NULL;
    }

    /* Free the derived result hash table. */
    if ( analy->derived_results != NULL )
    {
        htable_delete( analy->derived_results, delete_derived_result, TRUE );
        analy->derived_results = NULL;
    }

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
     * Close the current Taurus file
     */

    taurus_close( fid );

    return ( 0 );
}

/************************************************************
 * TAG( mili_db_get_param_array )
 *
 * Get a named application parameter - an array of data
 * is returned.
 */
extern int
mili_db_get_param_array( int dbid, char *name, void **data)
{
    int rval;

    rval = OK;

    /* mili call goes here */

    rval = mc_read_param_array(dbid, name, data);
    if ( rval != OK )
    {
        return GRIZ_FAIL;
    }

    return OK;
}


/************************************************************
 * TAG( mili_get_class_ptr )
 *
 * Get a pointer to the specified class .
 */
extern MO_class_data *
mili_get_class_ptr( Analysis *analy, int superclass,
                    char *class_name )
{

    int class_qty;
    MO_class_data **mo_classes;
    int i;

    class_qty = MESH_P( analy )->classes_by_sclass[superclass].qty;

    if ( class_qty==0 )
        return ( NULL );

    mo_classes = (MO_class_data **)
                 MESH_P( analy )->classes_by_sclass[superclass].list;

    for ( i=0;
            i<class_qty;
            i++)
        if ( !strcmp( class_name, mo_classes[i]->long_name  ) ||
                !strcmp( class_name, mo_classes[i]->short_name ) )
            return ( mo_classes[i] );

    return ( NULL );
}


/************************************************************
 * TAG( mili_get_class_names )
 *
 * Get full list of classes names in the Mili database.
 */
extern int
mili_get_class_names( Analysis *analy, int *qty_classes,
                      char **class_names, int *superclasses )
{

    int class_qty=0, sclass=0;
    MO_class_data **mo_classes;
    int i, j;
    int class_names_index=0, class_found_index=0;
    Bool_type class_found=FALSE;


    for ( sclass=0;
            sclass<QTY_SCLASS;
            sclass++ )
    {

        class_qty = MESH_P( analy )->classes_by_sclass[sclass].qty;

        if ( class_qty>0 )
        {
            mo_classes = (MO_class_data **) MESH_P( analy )->classes_by_sclass[sclass].list;
            class_found=FALSE;
            for ( i=0;
                    i<class_qty;
                    i++ )
            {
                for ( j=0;
                        j<class_names_index;
                        j++ )
                    if ( !strcmp( mo_classes[i]->short_name, class_names[j]) )
                    {
                        class_found=TRUE;
                        break;
                    }
                if ( !class_found )
                {
                    class_names[class_names_index]  = strdup( mo_classes[i]->short_name );
                    superclasses[class_names_index] = mo_classes[i]->superclass;
                    class_names_index++;
                    (*qty_classes)++;
                }
            }
        }
    }

    return ( 0 );
}

/************************************************************
 * TAG( mili_compare_labels )
 *
 * Used by the qsort routine to sort Mili Labels.
 */

int
mili_compare_labels( MO_class_labels *label1, MO_class_labels *label2 )
{
    if ( label1->label_num < label2->label_num )
        return -1;

    if ( label1->label_num > label2->label_num )
        return 1;

    return ( 0 );
}


/************************************************************
 * TAG( get_subrecord )
 *
 * Returns the name of a sub-record 'num'
 *
 */

char *get_subrecord( Analysis *analy, int num )
{
    State_rec_obj *p_state_rec;
    Subrecord     *p_subr;

    p_state_rec = analy->srec_tree;

    if ( num>=p_state_rec->qty )
        return NULL;

    p_subr = &p_state_rec->subrecs[num].subrec;
    return (p_subr->name);
}

/************************************************************
 * TAG( get_hex_volumes )
 *
 * Computes the volume for all hexes in the model.
 *
 */

extern int
get_hex_volumes( int dbid, Analysis *analy )
{
    int i=0, j=0, k=0, l=0;
    int int_args[3];

    int mesh_qty=0;
    int sclass=0;
    int qty_nodes=0, nd=0;
    int qty_classes=0, obj_qty=0;

    int hex_id=0;
    float xx[8], yy[8], zz[8];
    GVec3D *coords;
    int (*connects)[8];

    float volume=0.0;

    Mesh_data *p_mesh;
    MO_class_data **mo_classes, *p_node_class, *p_mocd;
    char short_name[M_MAX_NAME_LEN], long_name[M_MAX_NAME_LEN];

    int status=OK;

    /* Get number of meshes in family. */
    status = mc_query_family( dbid, QTY_MESHES, NULL, NULL, (void *) &mesh_qty );
    p_mesh = MESH_P( analy );

    p_node_class = p_mesh->node_geom;

    qty_nodes = p_node_class->qty;
    if ( qty_nodes==0 )
        return OK;

    for ( i = 0;
            i < mesh_qty;
            i++ )
    {
        int_args[0] = i;
        sclass = M_HEX;
        int_args[1] = sclass;
        status = mc_query_family( dbid, QTY_CLASS_IN_SCLASS,
                                  (void *) int_args, NULL,
                                  (void *) &qty_classes );
        if ( status != OK )
        {
            mc_print_error( "taurus_db_get_geom() call mc_query_family()",
                            status );
            return GRIZ_FAIL;
        }

        if ( p_mesh->hex_vol_sums==NULL )
            p_mesh->hex_vol_sums = NEW_N( float, qty_nodes, "Sum of vol for Hexes at nodes" );
        else
            for ( i = 0;
                    i < qty_nodes;
                    i++ )
                p_mesh->hex_vol_sums[i] = 0.;

        qty_classes = p_mesh->classes_by_sclass[sclass].qty;

        if ( qty_classes<=1 || p_mesh->classes_by_sclass[sclass].list == NULL )
            continue;

        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[sclass].list;

        if ( analy->state_p && analy->cur_state>0 )
            coords = analy->state_p->nodes.nodes3d;
        else
            coords = (GVec3D *) p_node_class->objects.nodes;

        for ( k = 0;
                k < qty_classes;
                k++ )
        {
            p_mocd = mo_classes[k];

            status = mc_get_class_info( dbid, i, sclass, k,
                                        short_name, long_name, &obj_qty );
            switch ( sclass )
            {
            case M_HEX:
                if ( p_mocd->objects.elems->volume==NULL )
                    p_mocd->objects.elems->volume = NEW_N( float, p_mocd->qty, "Volume for Hexes at nodes" );

                connects = (int (*)[8]) p_mocd->objects.elems->nodes;
                for ( hex_id=0;
                        hex_id<p_mocd->qty;
                        hex_id++ )
                {

                    /* Compute volume sums */
                    for ( l = 0;
                            l < 8;
                            l++ )
                    {
                        nd = connects[hex_id][l];

                        xx[l] = coords[nd][0];
                        yy[l] = coords[nd][1];
                        zz[l] = coords[nd][2];
                    }
                    volume = hex_vol( xx, yy, zz );

                    p_mocd->objects.elems->volume[hex_id] = volume;

                    /* Compute volume sums */
                    for ( l = 0;
                            l < 8;
                            l++ )
                    {
                        nd = connects[hex_id][l];
                        p_mesh->hex_vol_sums[nd] += volume;
                    }
                }
                break;

            default:
                /* do nothing */
                ;
            }
        }
    }
    return OK;
}
