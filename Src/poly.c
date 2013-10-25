/* $Id$ */
/*
 * poly.c - Read in and write out polygon data.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Jan 2 1992
 *
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Nov 29, 2012: Fixed problem with writing double
 *                geometry nodal data.
 *
 ************************************************************************
 */

#include <stdlib.h>
#include "viewer.h"
#include "draw.h"
#include "mdg.h"

/* Local routines. */
static float obj_color_lookup( float, float, float, float, Bool_type );
static void output_obj_hex_class( int, Bool_type *, int *, int *,
                                  MO_class_data *, Mesh_data *, Analysis *,
                                  FILE * );
static void output_obj_tet_class( int, Bool_type *, int *, int *,
                                  MO_class_data *, Mesh_data *, Analysis *,
                                  FILE * );
static void output_obj_quad_class( int, Bool_type *, int *, int *,
                                   MO_class_data *, Mesh_data *, Analysis *,
                                   FILE * );
static void output_obj_tri_class( int, Bool_type *, int *, int *,
                                  MO_class_data *, Mesh_data *, Analysis *,
                                  FILE * );
static void output_obj_vertex( int, float [3], Bool_type, int, FILE *,
                               Analysis * );
static void output_obj_face( Bool_type, int [], int, Bool_type *, FILE * );
static void output_obj_face_with_texture( Bool_type, int [], int, Bool_type *,
        FILE * );
static void output_hid_vertex( float [3], int, FILE *, Analysis * );
static void output_hid_face( int [4], int, Bool_type *, Bool_type, FILE * );
static void output_hid_beam( int [4], int, FILE * );
static void mark_class_nodes( Analysis *, Mesh_data *, Bool_type [QTY_SCLASS],
                              int * );
void write_geom_file( char *, Analysis *, Bool_type );

/************************************************************
 * TAG( read_slp_file )
 *
 * Read in polygons from an slp file.
 */
void
read_slp_file( char *fname, Analysis *analy )
{
    FILE *infile;
    Surf_poly *poly;
    char token[80];
    int obj_cnt;
    int poly_cnt;
    int i, j;

    /*
     * SLP file format:
     *
     *     solid CUBE
     *       color 1.0 1.0 1.0
     *       facet
     *           normal 0.00e+00 0.00e+00 1.00e+00
     *           normal 0.00e+00 0.00e+00 1.00e+00
     *           normal 0.00e+00 0.00e+00 1.00e+00
     *         outer loop
     *           vertex 0.00e+00 0.00e+00 0.00e+00
     *           vertex 1.00e+00 0.00e+00 0.00e+00
     *           vertex 1.00e+00 1.00e+00 0.00e+00
     *         endloop
     *       endfacet
     *       facet
     *         .
     *         .
     *         .
     *       endfacet
     *        .
     *        .
     *        .
     *     endsolid CUBE
     */

    if ( ( infile = fopen(fname, "r") ) == NULL )
    {
        popup_dialog( INFO_POPUP, "Unable to open file %s", fname );
        return;
    }

    obj_cnt = 0;
    poly_cnt = 0;

    /* Get "solid". */
    read_token( infile, token, 80 );

    while ( !feof( infile ) )
    {
        if ( strcmp( token, "solid" ) != 0 )
            break;

        /* Get solid's name. */
        read_token( infile, token, 80 );

        read_token( infile, token, 80 );
        while ( strcmp( token, "endsolid" ) != 0 )
        {
            if ( strcmp( token, "color" ) == 0 )
            {
                /* Read in color. */
                for ( i = 0; i < 3; i++ )
                    read_token( infile, token, 80 );
            }
            else if ( strcmp( token, "facet" ) == 0 )
            {
                /* Allocate a polygon object. */
                poly = NEW( Surf_poly, "Surface polygon" );
                poly->cnt = 3;
                poly->mat = obj_cnt;

                /* Read normals. */
                for ( i = 0; i < 3; i++ )
                {
                    /* Read "normal". */
                    read_token( infile, token, 80 );

                    /* Read normal components. */
                    for ( j = 0; j < 3; j++ )
                    {
                        read_token( infile, token, 80 );
                        sscanf( token, "%f", &poly->norm[i][j] );
                    }
                }

                /* Read "outer". */
                read_token( infile, token, 80 );

                /* Read "loop". */
                read_token( infile, token, 80 );

                /* Read vertices. */
                for ( i = 0; i < 3; i++ )
                {
                    /* Read "vertex". */
                    read_token( infile, token, 80 );

                    /* Read vertex components. */
                    for ( j = 0; j < 3; j++ )
                    {
                        read_token( infile, token, 80 );
                        sscanf( token, "%f", &poly->vtx[i][j] );
                    }
                }

                /* Read "endloop". */
                read_token( infile, token, 80 );

                /* Read "endfacet". */
                read_token( infile, token, 80 );

                INSERT( poly, analy->extern_polys );

                poly_cnt++;
            }

            read_token( infile, token, 80 );
        }

        /* Get endsolid's name. */
        read_token( infile, token, 80 );

        /* Get "solid". */
        read_token( infile, token, 80 );
    }

    wrt_text( "Read %d polygons from SLP file\n\n", poly_cnt );

    fclose( infile );
}


/************************************************************
 * TAG( read_ref_file )
 *
 * Read in a reference surface file.
 */
void
read_ref_file( char *fname, Analysis *analy )
{
    FILE *infile;
    Ref_poly *poly;
    char token[80];
    char *p_tok;
    int poly_cnt;
    int nd, i, j;
    int rsmat;

    /*
     * The ref surface file gives us a way to refer to particular
     * surfaces on hex volume elements.  The file is organized as
     * follows:
     *
     *     N_srf [material number]
     *     n1 n2 n3 n4
     *     n1 n2 n3 n4
     *     .
     *     .
     *     .
     *
     * The material number is optional, and is allowed as a way of
     * independently setting the surface color by tying it to an
     * arbitrary material.  It also allows for unlinking a reference
     * surface from translations of the material which make up the
     * element providing the face(s) of the reference surface.
     *
     * This routine reads in the ref surface file and puts the
     * ref surface polygons on the external surface list.  This
     * way, display of these surface polygons can be turned on
     * and off separately from the rest of the mesh.
     */

    if ( ( infile = fopen(fname, "r") ) == NULL )
    {
        popup_dialog( WARNING_POPUP, "Couldn't open file %s.\n", fname );
        return;
    }

    /* Get the polygon count. */
    fgets( token, 80, infile );
    p_tok = strtok( token, " \n" );
    sscanf( p_tok, "%d", &poly_cnt );

    /* Get the material number if it exists. */
    p_tok = strtok( NULL, " \n" );
    rsmat = ( p_tok != NULL ) ? atoi( p_tok ) - 1 : 0;


    for ( i = 0; i < poly_cnt; i++ )
    {
        poly = NEW( Ref_poly, "Reference polygon" );
        poly->mat = rsmat;

        for ( j = 0; j < 4; j++ )
        {
            read_token( infile, token, 80 );
            sscanf( token, "%d", &nd );
            poly->nodes[j] = nd-1;
        }

        /* Put the face on the reference polygon list. */
        INSERT( poly, analy->ref_polys );
    }
}


#ifdef HAVE_REF_STUFF
/************************************************************
 * TAG( gen_ref_from_coord )
 *
 * Generate a reference surface by finding all element faces
 * that lie in a particular axis-aligned plane.
 */
void
gen_ref_from_coord( Analysis *analy, int axis, float coord )
{
    Hex_geom *bricks;
    Nodal *nodes;
    Ref_poly *poly;
    Bool_type in_plane;
    float fcoord;
    int fnodes[4];
    int el, fc, i;

    /* Search through all elements and pull their faces.  If the face
     * is in the desired plane, use it.  How to avoid pulling faces
     * twice?  Use roughcut and check element visibility in this loop.
     * Remember, this is just a hack.
     */

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;

    for ( el = 0; el < bricks->cnt; el++ )
    {
        /* Skip invisible elements. */
        if ( !analy->hex_visib[el] )
            continue;

        for ( fc = 0; fc < 6; fc++ )
        {
            in_plane = TRUE;
            for ( i = 0; i < 4; i++ )
            {
                fnodes[i] = bricks->nodes[ fc_nd_nums[fc][i] ][el];
                fcoord = nodes->xyz[axis][ fnodes[i] ];
                if ( !( fabs((double)fcoord - coord) < 1.0e-3 ) )
                    in_plane = FALSE;
            }

            if ( in_plane )
            {
                /* Add this face to the reference surface. */
                poly = NEW( Ref_poly, "Reference polygon" );
                for ( i = 0; i < 4; i++ )
                    poly->nodes[i] = fnodes[i];
                poly->mat = 0;

                INSERT( poly, analy->ref_polys );
            }
        }
    }
}


/************************************************************
 * TAG( get_ref_average_area )
 *
 * Returns the average area of the reference surface faces.
 */
float
get_ref_average_area( Analysis *analy )
{
    Nodal *nodes;
    Ref_poly *poly;
    float verts[4][3];
    float area;
    int cnt, i, j;

    if ( analy->ref_polys == NULL )
        return 0.0;

    nodes = analy->state_p->nodes;

    area = 0.0;
    cnt = 0;
    for ( poly = analy->ref_polys; poly != NULL; poly = poly->next )
    {
        cnt++;

        for ( i = 0; i < 4; i++ )
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][poly->nodes[i]];

        area += area_of_quad( verts );
    }

    return ( area / cnt );
}
#endif


/* Wavefront .obj format output routines. */

/************************************************************
 * TAG( obj_color_lookup )
 *
 * Return the colormap value for a node in the mesh using the
 * same lookup procedure as in "color_lookup()".  The colormap
 * value is a real number in the range [0.0, 1.0].
 */
static float
obj_color_lookup( float val, float result_min, float result_max,
                  float threshold, Bool_type logscale)
{
    int idx, idx_new;

    double new_val,     new_result_min,     new_result_max,     new_result_shift;
    double new_result_mult;
    double new_val_log, new_result_min_log, new_result_max_log;

    /* If result is near zero, use the default material color.
     * Otherwise, do the color table lookup.
     */
    if ( val < threshold && val > -threshold )
    {
        idx = (int)( 253.99*(0.0-result_min)/(result_max-result_min) ) + 1;
    }
    else if ( val > result_max )
    {
        idx = 255;
    }
    else if ( val < result_min )
    {
        idx = 0;
    }
    else if ( result_min == result_max )
    {
        idx = 1;
    }
    else
    {
        if (logscale)
        {
            idx = (int)( 253.99*(val-result_min)/(result_max-result_min) ) + 1;

            /* Shift data to positive range */
            log_scale_data_shift( val,      result_min, result_max,
                                  &new_val, &new_result_min,
                                  &new_result_max, &new_result_shift,
                                  &new_result_mult);

            new_val_log        = log10(new_val);
            new_result_min_log = log10(new_result_min);
            new_result_max_log = log10(new_result_max);

            idx_new = (int)( 253.99 * ((new_val_log-new_result_min_log)/
                                       (new_result_max_log-new_result_min_log)) ) + 1;
            idx = idx_new;
        }
        else
        {
            idx = (int)( 253.99*(val-result_min)/(result_max-result_min) ) + 1;
        }
    }

    return( idx / 255.01 );
}


/************************************************************
 * TAG( write_obj_file )
 *
 * Write the current surface geometry as polygons to a
 * Wavefront .obj file.
 */
void
write_obj_file( char *fname, Analysis *analy )
{
    FILE *outfile;
    Refl_plane_obj *plane;
    Bool_type *reverse_order;
    float orig;
    float vert[3];
    int num_instances;
    int num_matls, *matl_cnts;
    int *node_nums;
    int i, j, cnt;
    Mesh_data *p_mesh;
    MO_class_data *p_node_geom;
    MO_class_data **mo_classes;
    int class_qty, node_qty;
    GVec3D *nodes3d, *onodes3d;
    GVec2D *nodes2d, *onodes2d;
    Bool_type superclasses[QTY_SCLASS] =
    {
        FALSE, FALSE, FALSE, FALSE,     /* G_UNIT, G_NODE, G_TRUSS, G_BEAM */
        TRUE, TRUE, TRUE,               /* G_TRI, G_QUAD, G_TET */
        FALSE, FALSE,                   /* G_PYRAMID, G_WEDGE */
        TRUE,                           /* G_HEX */
        FALSE, FALSE                    /* G_MAT, G_MESH */
    };
    Bool_type output_result;

    if ( ( outfile = fopen(fname, "w") ) == NULL )
    {
        popup_dialog( INFO_POPUP, "Unable to open file %s", fname );
        return;
    }

    p_mesh = MESH_P( analy );
    p_node_geom = p_mesh->node_geom;
    node_qty = p_node_geom->qty;
    num_matls = p_mesh->material_qty;
    matl_cnts = NEW_N( int, num_matls, "Wavefront output tmp array" );

    /* See if we need to do reflections. */
    if ( analy->reflect && analy->refl_planes != NULL )
    {
        /* Count total number of instances, including all reflections. */
        num_instances = 1;
        for ( plane = analy->refl_planes; plane != NULL; plane = plane->next )
        {
            if ( analy->refl_orig_only )
                num_instances++;
            else
                num_instances *= 2;
        }

        /* Mark which faces need to have their node order reversed. */
        reverse_order = NEW_N( int, num_instances, "Wavefront out tmp array" );
        reverse_order[0] = FALSE;
        if ( analy->refl_orig_only )
        {
            for ( i = 1; i < num_instances; i++ )
                reverse_order[i] = TRUE;
        }
        else
        {
            for ( j = 1, plane = analy->refl_planes;
                    plane != NULL;
                    plane = plane->next )
            {
                for ( i = 0; i < j; i++ )
                    reverse_order[j+i] = !reverse_order[i];
                j *= 2;
            }
        }
    }
    else
        num_instances = 1;


    /*
     * Mark all the nodes that need to be output.
     */
    node_nums = NEW_N( int, node_qty, "Wavefront output tmp array" );

    mark_class_nodes( analy, p_mesh, superclasses, node_nums );

    output_result = ( analy->cur_result != NULL );

    /*
     * Output the nodes.
     */
    if ( analy->dimension == 3 )
    {
        nodes3d = analy->state_p->nodes.nodes3d;
        onodes3d = (GVec3D *) analy->cur_ref_state_data;

        for ( i = 0, cnt = 1; i < node_qty; i++ )
        {
            if ( node_nums[i] == 1 )
            {
                if ( analy->displace_exag )
                {
                    /* Scale the node displacements. */
                    for ( j = 0; j < 3; j++ )
                    {
                        orig = onodes3d[i][j];
                        vert[j] = orig + analy->displace_scale[j]
                                  * (nodes3d[i][j] - orig);
                    }
                }
                else
                {
                    for ( j = 0; j < 3; j++ )
                        vert[j] = nodes3d[i][j];
                }

                output_obj_vertex( i, vert, output_result, num_instances,
                                   outfile, analy );

                node_nums[i] = cnt;
                cnt += num_instances;
            }
        }
    }
    else
    {
        nodes2d = analy->state_p->nodes.nodes2d;
        onodes2d = (GVec2D *) analy->cur_ref_state_data;
        vert[2] = 0.0;

        for ( i = 0, cnt = 1; i < node_qty; i++ )
        {
            if ( node_nums[i] == 1 )
            {
                if ( analy->displace_exag )
                {
                    /* Scale the node displacements. */
                    for ( j = 0; j < 2; j++ )
                    {
                        orig = onodes2d[i][j];
                        vert[j] = orig + analy->displace_scale[j]
                                  * (nodes2d[i][j] - orig);
                    }
                }
                else
                {
                    for ( j = 0; j < 2; j++ )
                        vert[j] = nodes2d[i][j];
                }

                output_obj_vertex( i, vert, output_result, num_instances,
                                   outfile, analy );

                node_nums[i] = cnt;
                cnt += num_instances;
            }
        }
    }


    if ( p_mesh->classes_by_sclass[G_HEX].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_HEX].list;
        class_qty = p_mesh->classes_by_sclass[G_HEX].qty;

        for ( i = 0; i < class_qty; i++ )
            output_obj_hex_class( num_instances, reverse_order, matl_cnts,
                                  node_nums, mo_classes[i], p_mesh, analy,
                                  outfile );
    }

    if ( p_mesh->classes_by_sclass[G_TET].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TET].list;
        class_qty = p_mesh->classes_by_sclass[G_TET].qty;

        for ( i = 0; i < class_qty; i++ )
            output_obj_tet_class( num_instances, reverse_order, matl_cnts,
                                  node_nums, mo_classes[i], p_mesh, analy,
                                  outfile );
    }

    if ( p_mesh->classes_by_sclass[G_QUAD].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_QUAD].list;
        class_qty = p_mesh->classes_by_sclass[G_QUAD].qty;

        for ( i = 0; i < class_qty; i++ )
            output_obj_quad_class( num_instances, reverse_order, matl_cnts,
                                   node_nums, mo_classes[i], p_mesh, analy,
                                   outfile );
    }

    if ( p_mesh->classes_by_sclass[G_TRI].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TRI].list;
        class_qty = p_mesh->classes_by_sclass[G_TRI].qty;

        for ( i = 0; i < class_qty; i++ )
            output_obj_tri_class( num_instances, reverse_order, matl_cnts,
                                  node_nums, mo_classes[i], p_mesh, analy,
                                  outfile );
    }

    /* If zero-thresholding is active, print out the zero slot. */
    if ( analy->zero_result > 0.0 )
    {
        i = (int)( 253.99*(0.0 - analy->result_mm[0])/
                   (analy->result_mm[1] - analy->result_mm[0]) ) + 1;
        wrt_text( "Material color in colormap entry #%d\n\n", i+1 );
    }

    free( matl_cnts );
    free( node_nums );
    if ( num_instances > 1 )
        free( reverse_order );

    fclose( outfile );
}


#ifdef OLD_WRT_OBJ_FILE
/************************************************************
 * TAG( write_obj_file )
 *
 * Write the current surface geometry as polygons to a
 * Wavefront .obj file.
 */
void
write_obj_file( char *fname, Analysis *analy )
{
    FILE *outfile;
    Hex_geom *bricks;
    Shell_geom *shells;
    Nodal *nodes, *onodes;
    Refl_plane_obj *plane;
    char str[80];
    Bool_type *reverse_order;
    float *activity;
    float orig;
    float vert[3];
    int vert_idx[4];
    int num_instances;
    int num_matls, *matl_cnts, *mats;
    int *node_nums;
    int i, j, k, el, fc, nd, cnt;

    if ( ( outfile = fopen(fname, "w") ) == NULL )
    {
        wrt_text( "Couldn't open file %s.\n", fname );
        return;
    }

    nodes = analy->state_p->nodes;
    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    onodes = analy->geom_p->nodes;

    num_matls = analy->num_materials;
    matl_cnts = NEW_N( int, num_matls, "Wavefront output tmp array" );

    /* See if we need to do reflections. */
    if ( analy->reflect && analy->refl_planes != NULL )
    {
        /* Count total number of instances, including all reflections. */
        num_instances = 1;
        for ( plane = analy->refl_planes; plane != NULL; plane = plane->next )
        {
            if ( analy->refl_orig_only )
                num_instances++;
            else
                num_instances *= 2;
        }

        /* Mark which faces need to have their node order reversed. */
        reverse_order = NEW_N( int, num_instances, "Wavefront out tmp array" );
        reverse_order[0] = FALSE;
        if ( analy->refl_orig_only )
        {
            for ( i = 1; i < num_instances; i++ )
                reverse_order[i] = TRUE;
        }
        else
        {
            for ( j = 1, plane = analy->refl_planes;
                    plane != NULL;
                    plane = plane->next )
            {
                for ( i = 0; i < j; i++ )
                    reverse_order[j+i] = !reverse_order[i];
                j *= 2;
            }
        }
    }
    else
        num_instances = 1;


    /*
     * Mark all the nodes that need to be output.
     */
    node_nums = NEW_N( int, nodes->cnt, "Wavefront output tmp array" );

    if ( bricks != NULL )
    {
        for ( j = 0; j < analy->face_cnt; j++ )
        {
            el = analy->face_el[j];
            fc = analy->face_fc[j];

            for ( k = 0; k < 4; k++ )
            {
                nd = bricks->nodes[ fc_nd_nums[fc][k] ][el];
                node_nums[nd] = 1;
            }
        }
    }

    if ( shells != NULL )
    {
        for ( j = 0; j < shells->cnt; j++ )
        {
            /* Skip if this material is invisible. */
            if ( analy->hide_material[shells->mat[j]] )
                continue;

            for ( k = 0; k < 4; k++ )
            {
                nd = shells->nodes[k][j];
                node_nums[nd] = 1;
            }
        }
    }

    /*
     * Output the nodes.
     */
    for ( i = 0, cnt = 1; i < nodes->cnt; i++ )
    {
        if ( node_nums[i] == 1 )
        {
            if ( analy->displace_exag )
            {
                /* Scale the node displacements. */
                for ( j = 0; j < 3; j++ )
                {
                    orig = onodes->xyz[j][i];
                    vert[j] = orig + analy->displace_scale[j]*
                              (nodes->xyz[j][i] - orig);
                }
            }
            else
            {
                for ( j = 0; j < 3; j++ )
                    vert[j] = nodes->xyz[j][i];
            }

            output_obj_vertex( i, vert, num_instances, outfile, analy );

            node_nums[i] = cnt;
            cnt += num_instances;
        }
    }


    if ( bricks != NULL )
    {
        /*
         * Check which materials need to be output.
         */
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

            matl_cnts[bricks->mat[el]]++;
        }

        /*
         * Output brick faces.
         */
        for ( i = 0; i < num_matls; i++ )
        {
            if ( matl_cnts[i] > 0 )
            {
                fprintf( outfile, "#\n# Brick Group (Material %d)\n#\n", i+1 );
                fprintf( outfile, "g mat%d whole\n", i+1 );


                for ( j = 0; j < analy->face_cnt; j++ )
                {
                    el = analy->face_el[j];
                    fc = analy->face_fc[j];

                    if ( bricks->mat[el] == i )
                    {
                        /*
                         * Remove faces that are shared with shell elements.
                         */
                        if ( analy->shared_faces &&
                                analy->geom_p->shells != NULL &&
                                face_matches_shell( el, fc, analy ) )
                            continue;

                        for ( k = 0; k < 4; k++ )
                        {
                            nd = bricks->nodes[ fc_nd_nums[fc][k] ][el];
                            vert_idx[k] = node_nums[nd];
                        }

                        output_obj_face( vert_idx, num_instances,
                                         reverse_order, outfile );
                    }
                }
            }
        }
    }

    if ( shells != NULL )
    {
        activity = analy->state_p->activity_present ?
                   analy->state_p->shells->activity : NULL;
        memset( matl_cnts, 0, num_matls*sizeof( int ) );

        /*
         * Check which materials need to be output.
         */
        for ( i = 0; i < shells->cnt; i++ )
        {
            /* Check for inactive elements. */
            if ( activity && activity[i] == 0.0 )
                continue;

            /* Skip if this material is invisible. */
            if ( analy->hide_material[shells->mat[i]] )
                continue;

            matl_cnts[shells->mat[i]]++;
        }

        /*
         * Output shell faces.
         */
        for ( i = 0; i < num_matls; i++ )
        {
            if ( matl_cnts[i] > 0 )
            {
                fprintf( outfile, "#\n# Shell Group (Material %d)\n#\n", i+1 );
                fprintf( outfile, "g mat%d whole\n", i+1 );

                for ( j = 0; j < shells->cnt; j++ )
                {
                    if ( shells->mat[j] != i )
                        continue;

                    /* Check for inactive elements. */
                    if ( activity && activity[j] == 0.0 )
                        continue;

                    /* Skip if this material is invisible. */
                    if ( analy->hide_material[shells->mat[j]] )
                        continue;

                    for ( k = 0; k < 4; k++ )
                    {
                        nd = shells->nodes[k][j];
                        vert_idx[k] = node_nums[nd];
                    }

                    output_obj_face( vert_idx, num_instances,
                                     reverse_order, outfile );
                }
            }
        }
    }

    /* If zero-thresholding is active, print out the zero slot. */
    if ( analy->zero_result > 0.0 )
    {
        i = (int)( 253.99*(0.0 - analy->result_mm[0])/
                   (analy->result_mm[1] - analy->result_mm[0]) ) + 1;
        wrt_text( "\nMaterial color in colormap entry #%d\n\n", i+1 );
    }

    free( matl_cnts );
    free( node_nums );
    if ( num_instances > 1 )
        free( reverse_order );

    fclose( outfile );
}
#endif


/************************************************************
 * TAG( output_obj_hex_class )
 *
 * Output faces from a hex class to an .obj file.
 */
static void
output_obj_hex_class( int num_instances, Bool_type *reverse_order,
                      int *matl_cnts, int *node_nums,
                      MO_class_data *p_hex_class, Mesh_data *p_mesh,
                      Analysis *analy, FILE *outfile )
{
    int (*connects8)[8];
    int *mat;
    int vert_idx[4];
    Visibility_data *p_vd;
    int face_cnt;
    int *face_el, *face_fc;
    int i, j, k, el, fc, nd;
    int num_matls;
    void (*output_face)( Bool_type, int [], int, Bool_type *, FILE* );

    num_matls = p_mesh->material_qty;
    connects8 = (int (*)[8]) p_hex_class->objects.elems->nodes;
    mat = p_hex_class->objects.elems->mat;
    p_vd = p_hex_class->p_vis_data;
    face_cnt = p_vd->face_cnt;
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;

    memset( matl_cnts, 0, num_matls*sizeof( int ) );

    for ( j = 0; j < face_cnt; j++ )
    {
        el = face_el[j];
        fc = face_fc[j];

        /*
         * Remove faces that are shared with shell elements, so
         * the polygons are not drawn twice.
         */
        if ( analy->shared_faces
                && p_mesh->classes_by_sclass[G_QUAD].qty != 0
                && face_matches_quad( el, fc, p_hex_class, p_mesh, analy ) )
            continue;

        matl_cnts[mat[el]]++;
    }

    output_face = ( analy->cur_result != NULL )
                  ? output_obj_face_with_texture
                  : output_obj_face;

    /*
     * Output hex faces.
     */
    for ( i = 0; i < num_matls; i++ )
    {
        if ( matl_cnts[i] > 0 )
        {
            fprintf( outfile, "#\n# Hex Group (Material %d)\n#\n",
                     i + 1 );
            fprintf( outfile, "g mat%d whole\n", i + 1 );


            for ( j = 0; j < face_cnt; j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                if ( mat[el] == i )
                {
                    /*
                     * Remove faces that are shared with shell elements.
                     */
                    if ( analy->shared_faces
                            && p_mesh->classes_by_sclass[G_QUAD].qty != 0
                            && face_matches_quad( el, fc, p_hex_class, p_mesh,
                                                  analy ) )
                        continue;

                    for ( k = 0; k < 4; k++ )
                    {
                        nd = connects8[el][ fc_nd_nums[fc][k] ];
                        vert_idx[k] = node_nums[nd];
                    }

                    output_face( TRUE, vert_idx, num_instances, reverse_order,
                                 outfile );
                }
            }
        }
    }
}


/************************************************************
 * TAG( output_obj_tet_class )
 *
 * Output faces from a tet class to an .obj file.
 */
static void
output_obj_tet_class( int num_instances, Bool_type *reverse_order,
                      int *matl_cnts, int *node_nums,
                      MO_class_data *p_tet_class, Mesh_data *p_mesh,
                      Analysis *analy, FILE *outfile )
{
    int (*connects4)[4];
    int *mat;
    int vert_idx[3];
    Visibility_data *p_vd;
    int face_cnt;
    int *face_el, *face_fc;
    int i, j, k, el, fc, nd, num_matls;
    void (*output_face)( Bool_type, int [], int, Bool_type *, FILE* );

    num_matls = p_mesh->material_qty;
    connects4 = (int (*)[4]) p_tet_class->objects.elems->nodes;
    mat = p_tet_class->objects.elems->mat;
    p_vd = p_tet_class->p_vis_data;
    face_cnt = p_vd->face_cnt;
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;

    memset( matl_cnts, 0, num_matls*sizeof( int ) );

    for ( j = 0; j < face_cnt; j++ )
    {
        el = face_el[j];
        fc = face_fc[j];

        /*
         * Remove faces that are shared with shell elements, so
         * the polygons are not drawn twice.
         */
        if ( analy->shared_faces
                && p_mesh->classes_by_sclass[G_TRI].qty != 0
                && face_matches_tri( el, fc, p_tet_class, p_mesh, analy ) )
            continue;

        matl_cnts[mat[el]]++;
    }

    output_face = ( analy->cur_result != NULL )
                  ? output_obj_face_with_texture
                  : output_obj_face;

    /*
     * Output tet faces.
     */
    for ( i = 0; i < num_matls; i++ )
    {
        if ( matl_cnts[i] > 0 )
        {
            fprintf( outfile, "#\n# Tet Group (Material %d)\n#\n",
                     i + 1 );
            fprintf( outfile, "g mat%d whole\n", i + 1 );


            for ( j = 0; j < face_cnt; j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                if ( mat[el] == i )
                {
                    /*
                     * Remove faces that are shared with tri elements.
                     */
                    if ( analy->shared_faces
                            && p_mesh->classes_by_sclass[G_TRI].qty != 0
                            && face_matches_tri( el, fc, p_tet_class, p_mesh,
                                                 analy ) )
                        continue;

                    for ( k = 0; k < 3; k++ )
                    {
                        nd = connects4[el][ tet_fc_nd_nums[fc][k] ];
                        vert_idx[k] = node_nums[nd];
                    }

                    output_face( FALSE, vert_idx, num_instances, reverse_order,
                                 outfile );
                }
            }
        }
    }
}


/************************************************************
 * TAG( output_obj_quad_class )
 *
 * Output faces from a quad class to an .obj file.
 */
static void
output_obj_quad_class( int num_instances, Bool_type *reverse_order,
                       int *matl_cnts, int *node_nums,
                       MO_class_data *p_quad_class, Mesh_data *p_mesh,
                       Analysis *analy, FILE *outfile )
{
    int (*connects4)[4];
    int *mat;
    int vert_idx[4];
    int quad_qty;
    int i, j, k, nd, num_matls;
    float *activity;
    unsigned char *hide_mtl;
    void (*output_face)( Bool_type, int [], int, Bool_type *, FILE* );

    num_matls = p_mesh->material_qty;
    hide_mtl = p_mesh->hide_material;
    connects4 = (int (*)[4]) p_quad_class->objects.elems->nodes;
    mat = p_quad_class->objects.elems->mat;
    quad_qty = p_quad_class->qty;

    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_quad_class->elem_class_index]
               : NULL;

    memset( matl_cnts, 0, num_matls*sizeof( int ) );

    /*
     * Check which materials need to be output.
     */
    for ( i = 0; i < quad_qty; i++ )
    {
        /* Check for inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Skip if this material is invisible. */
        if ( hide_mtl[mat[i]]
                || hide_by_object_type( p_quad_class, mat[i], i, analy, NULL ) )
            continue;

        matl_cnts[mat[i]]++;
    }

    output_face = ( analy->cur_result != NULL )
                  ? output_obj_face_with_texture
                  : output_obj_face;

    /*
     * Output quad faces.
     */
    for ( i = 0; i < num_matls; i++ )
    {
        if ( matl_cnts[i] > 0 )
        {
            fprintf( outfile, "#\n# Quad Group (Material %d)\n#\n", i+1 );
            fprintf( outfile, "g mat%d whole\n", i+1 );

            for ( j = 0; j < quad_qty; j++ )
            {
                if ( mat[j] != i )
                    continue;

                /* Check for inactive elements. */
                if ( activity && activity[j] == 0.0 )
                    continue;

                /* Skip if this material is invisible. */
                if ( hide_mtl[mat[j]]
                        || hide_by_object_type( p_quad_class, mat[j], j, analy, NULL ) )
                    continue;

                for ( k = 0; k < 4; k++ )
                {
                    nd = connects4[j][k];
                    vert_idx[k] = node_nums[nd];
                }

                output_face( TRUE, vert_idx, num_instances, reverse_order,
                             outfile );
            }
        }
    }
}


/************************************************************
 * TAG( output_obj_tri_class )
 *
 * Output faces from a tri class to an .obj file.
 */
static void
output_obj_tri_class( int num_instances, Bool_type *reverse_order,
                      int *matl_cnts, int *node_nums,
                      MO_class_data *p_tri_class, Mesh_data *p_mesh,
                      Analysis *analy, FILE *outfile )
{
    int (*connects3)[3];
    int *mat;
    int vert_idx[3];
    int tri_qty;
    int i, j, k, nd, num_matls;
    float *activity;
    unsigned char *hide_mtl;
    void (*output_face)( Bool_type, int [], int, Bool_type *, FILE* );

    num_matls = p_mesh->material_qty;
    hide_mtl = p_mesh->hide_material;
    connects3 = (int (*)[3]) p_tri_class->objects.elems->nodes;
    mat = p_tri_class->objects.elems->mat;
    tri_qty = p_tri_class->qty;

    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_tri_class->elem_class_index]
               : NULL;

    memset( matl_cnts, 0, num_matls*sizeof( int ) );

    /*
     * Check which materials need to be output.
     */
    for ( i = 0; i < tri_qty; i++ )
    {
        /* Check for inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Skip if this material is invisible. */
        if ( hide_mtl[mat[i]] )
            continue;

        matl_cnts[mat[i]]++;
    }

    output_face = ( analy->cur_result != NULL )
                  ? output_obj_face_with_texture
                  : output_obj_face;

    /*
     * Output tri faces.
     */
    for ( i = 0; i < num_matls; i++ )
    {
        if ( matl_cnts[i] > 0 )
        {
            fprintf( outfile, "#\n# Tri Group (Material %d)\n#\n", i+1 );
            fprintf( outfile, "g mat%d whole\n", i+1 );

            for ( j = 0; j < tri_qty; j++ )
            {
                if ( mat[j] != i )
                    continue;

                /* Check for inactive elements. */
                if ( activity && activity[j] == 0.0 )
                    continue;

                /* Skip if this material is invisible. */
                if ( hide_mtl[mat[j]] )
                    continue;

                for ( k = 0; k < 3; k++ )
                {
                    nd = connects3[j][k];
                    vert_idx[k] = node_nums[nd];
                }

                output_face( FALSE, vert_idx, num_instances, reverse_order,
                             outfile );
            }
        }
    }
}


/************************************************************
 * TAG( output_obj_vertex )
 *
 * Output a vertex to an .obj file.  This routine performs
 * any reflections required by reflection planes.
 */
static void
output_obj_vertex( int nd, float nd_pos[3], Bool_type output_res,
                   int num_instances, FILE *outfile, Analysis *analy )
{
    Refl_plane_obj *plane;
    float inst_pos[64][3];
    float cmap_idx;
    int cnt, i;

    /* Output the original vertex. */
    fprintf( outfile, "v %f %f %f\n", nd_pos[0], nd_pos[1], nd_pos[2] );

    if ( output_res )
    {
        cmap_idx = obj_color_lookup( NODAL_RESULT_BUFFER( analy )[nd],
                                     analy->result_mm[0], analy->result_mm[1],
                                     analy->zero_result, analy->logscale);
        fprintf( outfile, "vt %f %f %f\n", cmap_idx, cmap_idx, cmap_idx );
    }

    if ( num_instances == 1 )
        return;

    /* Reflect the vertex. */
    VEC_COPY( inst_pos[0], nd_pos );

    cnt = 1;
    for ( plane = analy->refl_planes; plane != NULL; plane = plane->next )
    {
        if ( analy->refl_orig_only )
        {
            point_transform( inst_pos[cnt], inst_pos[0],
                             &plane->pt_transf );
            cnt++;
        }
        else
        {
            for ( i = 0; i < cnt; i++ )
                point_transform( inst_pos[cnt+i], inst_pos[i],
                                 &plane->pt_transf );
            cnt *= 2;
        }
    }

    /* Output the reflected vertices. */
    for ( i = 1; i < cnt; i++ )
    {
        fprintf( outfile, "v %f %f %f\n", inst_pos[i][0],
                 inst_pos[i][1], inst_pos[i][2] );

        if ( output_res )
            fprintf( outfile, "vt %f %f %f\n", cmap_idx, cmap_idx, cmap_idx );
    }
}


/************************************************************
 * TAG( output_obj_face )
 *
 * Output a quad or tri face to an .obj file.  This routine
 * performs any reflections required by reflection planes.
 */
static void
output_obj_face( Bool_type quad, int vert_nums[], int num_instances,
                 Bool_type *reverse_order, FILE *outfile )
{
    int i;

    /* Output the original face. */
    fprintf( outfile, "f %d %d %d",
             vert_nums[0], vert_nums[1], vert_nums[2] );
    if ( quad )
        fprintf( outfile, " %d\n", vert_nums[3] );
    else
        fprintf( outfile, "\n" );

    /* We need to reverse node ordering for each reflection. */
    if ( quad )
    {
        for ( i = 1; i < num_instances; i++ )
        {
            if ( reverse_order[i] )
                fprintf( outfile, "f %d %d %d %d\n",
                         vert_nums[3]+i,
                         vert_nums[2]+i,
                         vert_nums[1]+i,
                         vert_nums[0]+i );
            else
                fprintf( outfile, "f %d %d %d %d\n",
                         vert_nums[0]+i,
                         vert_nums[1]+i,
                         vert_nums[2]+i,
                         vert_nums[3]+i );
        }
    }
    else /* triangle */
    {
        for ( i = 1; i < num_instances; i++ )
        {
            if ( reverse_order[i] )
                fprintf( outfile, "f %d %d %d\n",
                         vert_nums[2]+i,
                         vert_nums[1]+i,
                         vert_nums[0]+i );
            else
                fprintf( outfile, "f %d %d %d\n",
                         vert_nums[0]+i,
                         vert_nums[1]+i,
                         vert_nums[2]+i );
        }
    }
}


/************************************************************
 * TAG( output_obj_face_with_texture )
 *
 * Output a quad or tri face to an .obj file, including the
 * texture vertex reference.  This routine performs any
 * reflections required by reflection planes.
 */
static void
output_obj_face_with_texture( Bool_type quad, int vert_nums[],
                              int num_instances, Bool_type *reverse_order,
                              FILE *outfile )
{
    int i;

    /* Output the original face. */
    fprintf( outfile, "f %d/%d %d/%d %d/%d",
             vert_nums[0], vert_nums[0],
             vert_nums[1], vert_nums[1],
             vert_nums[2], vert_nums[2] );
    if ( quad )
        fprintf( outfile, " %d/%d\n",
                 vert_nums[3], vert_nums[3] );
    else
        fprintf( outfile, "\n" );

    /* We need to reverse node ordering for each reflection. */
    if ( quad )
    {
        for ( i = 1; i < num_instances; i++ )
        {
            if ( reverse_order[i] )
                fprintf( outfile, "f %d/%d %d/%d %d/%d %d/%d\n",
                         vert_nums[3]+i, vert_nums[3]+i,
                         vert_nums[2]+i, vert_nums[2]+i,
                         vert_nums[1]+i, vert_nums[1]+i,
                         vert_nums[0]+i, vert_nums[0]+i );
            else
                fprintf( outfile, "f %d/%d %d/%d %d/%d %d/%d\n",
                         vert_nums[0]+i, vert_nums[0]+i,
                         vert_nums[1]+i, vert_nums[1]+i,
                         vert_nums[2]+i, vert_nums[2]+i,
                         vert_nums[3]+i, vert_nums[3]+i );
        }
    }
    else /* triangle */
    {
        for ( i = 1; i < num_instances; i++ )
        {
            if ( reverse_order[i] )
                fprintf( outfile, "f %d/%d %d/%d %d/%d\n",
                         vert_nums[2]+i, vert_nums[2]+i,
                         vert_nums[1]+i, vert_nums[1]+i,
                         vert_nums[0]+i, vert_nums[0]+i );
            else
                fprintf( outfile, "f %d/%d %d/%d %d/%d\n",
                         vert_nums[0]+i, vert_nums[0]+i,
                         vert_nums[1]+i, vert_nums[1]+i,
                         vert_nums[2]+i, vert_nums[2]+i );
        }
    }
}


/* HIDDEN .hid output routines. */

/************************************************************
 * TAG( output_hid_view )
 *
 * Write the current view parameters to a .hid file.
 */
static void
output_hid_view( FILE *outfile, Analysis *analy )
{
    Transf_mat view_trans;
    float vpaspect;
    int i;

    /*
     * This stuff is out of date.  This routine and the corresponding
     * code in "hidden" should be rewritten.
     *
     * dist = VEC_LENGTH( v_win->eye_pos );
     *
     * view_angle = (Angle)(10.0*2.0*
     *               RAD_TO_DEG( atan2((double)1.0,(double)(dist-1.0)) )+0.5);
     *
     * perspective( view_angle, aspect, dist+v_win->near, dist+v_win->far );
     * lookat( v_win->eye_pos[0], v_win->eye_pos[1], v_win->eye_pos[2],
     *         0.0, 0.0, 0.0, 0 );
     */

    /*
     * Eye pos is always ( 0.0, 0.0, dist ).
     * Look direction is along negative Z.
     */

    vpaspect = v_win->vp_width / (float) v_win->vp_height;

    view_transf_mat( &view_trans );

    /*
     * Output the current view parameters.
     */
    if ( v_win->orthographic )
        fprintf( outfile, "ORTHOGRAPHIC\n" );
    else
        fprintf( outfile, "PERSPECTIVE\n" );
    fprintf( outfile, "VIEWPORT_ASPECT_X/Y %f\n", vpaspect );
    fprintf( outfile, "EYEPOINT %f %f %f\n", v_win->look_from[0],
             v_win->look_from[1], v_win->look_from[2] );

    fprintf( outfile, "VIEW_MATRIX\n" );
    for ( i = 0; i < 4; i++ )
        fprintf( outfile, "%f %f %f %f\n",
                 view_trans.mat[i][0], view_trans.mat[i][1],
                 view_trans.mat[i][2], view_trans.mat[i][3] );
}


/************************************************************
 * TAG( write_hid_file )
 *
 * Write the current geometry to a .hid file for use with the
 * hidden line program.  Note that node numbering starts at
 * zero in a hidden file!
 */
extern void
write_hid_file( char *fname, Analysis *analy )
{
    FILE *outfile;
    Refl_plane_obj *plane;
    Bool_type *reverse_order;
    float *activity;
    float orig;
    float vert[3];
    int vert_idx[4];
    int num_instances;
    int *node_nums, *face_el, *face_fc, *mat;
    int i, j, k, el, fc, nd, cnt, node_cnt;
    Mesh_data *p_mesh;
    unsigned char *hide_material;
    GVec3D *nodes, *onodes;
    MO_class_data *p_class, *p_node_geom;
    MO_class_data **mo_classes;
    Visibility_data *p_vd;
    int (*connects8)[8];
    int (*connects4)[4];
    int (*connects3)[3];
    int (*connects2)[2];
    int face_cnt, class_qty, node_qty, elem_qty;
    Bool_type quad_shared, tri_shared;
    int superclasses[QTY_SCLASS] =
    {
        0, 0,           /* G_UNIT, G_NODE */
        1, 1, 1, 1, 1,  /* G_TRUSS, G_BEAM, G_TRI, G_QUAD, G_TET */
        0, 0,           /* G_PYRAMID, G_WEDGE */
        1,              /* G_HEX */
        0, 0            /* G_MAT, G_MESH */
    };

    /*
     * Open the output file.
     */
    if ( ( outfile = fopen(fname, "w") ) == NULL )
    {
        popup_dialog( WARNING_POPUP, "Couldn't open file %s; aborted.\n",
                      fname );
        return;
    }

    /*
     * Output the current view parameters.
     */
    output_hid_view( outfile, analy );


    /*
     * Output nodes, faces, shells, beams.
     */

    p_mesh = MESH_P( analy );
    hide_material = p_mesh->hide_material;
    p_node_geom = p_mesh->node_geom;
    node_qty = p_node_geom->qty;
    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;

    /* See if we need to do reflections. */
    if ( analy->reflect && analy->refl_planes != NULL )
    {
        /* Count total number of instances, including all reflections. */
        num_instances = 1;
        for ( plane = analy->refl_planes; plane != NULL; plane = plane->next )
        {
            if ( analy->refl_orig_only )
                num_instances++;
            else
                num_instances *= 2;
        }

        /* Mark which faces need to have their node order reversed. */
        reverse_order = NEW_N( int, num_instances, "Hidden out tmp array" );
        reverse_order[0] = FALSE;
        if ( analy->refl_orig_only )
        {
            for ( i = 1; i < num_instances; i++ )
                reverse_order[i] = TRUE;
        }
        else
        {
            for ( j = 1, plane = analy->refl_planes;
                    plane != NULL;
                    plane = plane->next )
            {
                for ( i = 0; i < j; i++ )
                    reverse_order[j+i] = !reverse_order[i];
                j *= 2;
            }
        }
    }
    else
        num_instances = 1;


    /*
     * Mark all the nodes that need to be output.
     */
    node_nums = NEW_N( int, node_qty, "Hidden output tmp array" );

    mark_class_nodes( analy, p_mesh, superclasses, node_nums );

    /*
     * Count the nodes.
     */
    for ( node_cnt = 0, i = 0; i < node_qty; i++ )
    {
        if ( node_nums[i] == 1 )
            node_cnt++;
    }

    /*
     * Output the nodes.
     */
    fprintf( outfile, "NODES %d\n", node_cnt * num_instances );

    for ( i = 0, cnt = 0; i < node_qty; i++ )
    {
        if ( node_nums[i] == 1 )
        {
            if ( analy->displace_exag )
            {
                /* Scale the node displacements. */
                for ( j = 0; j < 3; j++ )
                {
                    orig = onodes[i][j];
                    vert[j] = orig + analy->displace_scale[j]*
                              (nodes[i][j] - orig);
                }
            }
            else
            {
                for ( j = 0; j < 3; j++ )
                    vert[j] = nodes[i][j];
            }

            output_hid_vertex( vert, num_instances, outfile, analy );

            node_nums[i] = cnt;
            cnt += num_instances;
        }
    }

    /*
     * Output hex faces.
     */

    quad_shared = analy->shared_faces
                  && p_mesh->classes_by_sclass[G_QUAD].qty != 0;

    fprintf( outfile, "ONE_SIDED_POLYS %d\n",
             superclasses[G_HEX] * num_instances );

    if ( p_mesh->classes_by_sclass[G_HEX].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_HEX].list;
        class_qty = p_mesh->classes_by_sclass[G_HEX].qty;

        for ( i = 0; i < class_qty; i++ )
        {
            p_class = mo_classes[i];
            connects8 = (int (*)[8]) p_class->objects.elems->nodes;
            p_vd = p_class->p_vis_data;
            face_cnt = p_vd->face_cnt;
            face_el = p_vd->face_el;
            face_fc = p_vd->face_fc;

            for ( j = 0; j < face_cnt; j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                /*
                 * Remove faces that are shared with quad elements, so
                 * the polygons are not drawn twice.
                 */
                if ( quad_shared && face_matches_quad( el, fc, p_class, p_mesh,
                                                       analy ) )
                    continue;

                for ( k = 0; k < 4; k++ )
                {
                    nd = connects8[el][ fc_nd_nums[fc][k] ];
                    vert_idx[k] = node_nums[nd];
                }

                output_hid_face( vert_idx, num_instances, reverse_order,
                                 TRUE, outfile );
            }
        }
    }

    /*
     * Output tet faces.
     */

    /**/
    /* Need to add this capability to hidden program.  Should change above
     * key to ONE_SIDED_QUADS.
     */

    tri_shared = analy->shared_faces
                 && p_mesh->classes_by_sclass[G_TRI].qty != 0;

    fprintf( outfile, "ONE_SIDED_TRIANGLES %d\n",
             superclasses[G_TET] * num_instances );

    if ( p_mesh->classes_by_sclass[G_TET].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TET].list;
        class_qty = p_mesh->classes_by_sclass[G_TET].qty;

        for ( i = 0; i < class_qty; i++ )
        {
            p_class = mo_classes[i];
            connects4 = (int (*)[4]) p_class->objects.elems->nodes;
            p_vd = p_class->p_vis_data;
            face_cnt = p_vd->face_cnt;
            face_el = p_vd->face_el;
            face_fc = p_vd->face_fc;

            for ( j = 0; j < face_cnt; j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                /*
                 * Remove faces that are shared with tri elements, so
                 * the polygons are not drawn twice.
                 */
                if ( tri_shared && face_matches_tri( el, fc, p_class, p_mesh,
                                                     analy ) )
                    continue;

                for ( k = 0; k < 3; k++ )
                {
                    nd = connects4[el][ tet_fc_nd_nums[fc][k] ];
                    vert_idx[k] = node_nums[nd];
                }

                output_hid_face( vert_idx, num_instances, reverse_order,
                                 FALSE, outfile );
            }
        }
    }

    /*
     * Output quad elements.
     */
    fprintf( outfile, "TWO_SIDED_POLYS %d\n",
             superclasses[G_QUAD] * num_instances );

    if ( p_mesh->classes_by_sclass[G_QUAD].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_QUAD].list;
        class_qty = p_mesh->classes_by_sclass[G_QUAD].qty;

        for ( i = 0; i < class_qty; i++ )
        {
            p_class = mo_classes[i];
            connects4 = (int (*)[4]) p_class->objects.elems->nodes;
            elem_qty = p_class->qty;
            mat = p_class->objects.elems->mat;
            activity = analy->state_p->sand_present
                       ? analy->state_p->elem_class_sand[p_class->elem_class_index]
                       : NULL;

            for ( j = 0; j < elem_qty; j++ )
            {
                /* Skip if invisible material or element inactive. */
                if ( hide_material[mat[j]]
                        || ( activity && activity[j] )
                        ||   hide_by_object_type( p_class, mat[j], j, analy, NULL ) )
                    continue;

                for ( k = 0; k < 4; k++ )
                    vert_idx[k] = node_nums[ connects4[j][k] ];

                output_hid_face( vert_idx, num_instances, reverse_order, TRUE,
                                 outfile );
            }
        }
    }

    /**/
    /* Need to add this capability to hidden program. Should change above
     * key to TWO_SIDED_QUADS.
     */

    /*
     * Output tri elements.
     */
    fprintf( outfile, "TWO_SIDED_TRIANGLES %d\n",
             superclasses[G_TRI] * num_instances );

    if ( p_mesh->classes_by_sclass[G_TRI].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TRI].list;
        class_qty = p_mesh->classes_by_sclass[G_TRI].qty;

        for ( i = 0; i < class_qty; i++ )
        {
            p_class = mo_classes[i];
            connects3 = (int (*)[3]) p_class->objects.elems->nodes;
            elem_qty = p_class->qty;
            mat = p_class->objects.elems->mat;
            activity = analy->state_p->sand_present
                       ? analy->state_p->elem_class_sand[p_class->elem_class_index]
                       : NULL;

            for ( j = 0; j < elem_qty; j++ )
            {
                /* Skip if invisible material or element inactive. */
                if ( hide_material[mat[j]]
                        || ( activity && activity[j] )
                        || hide_by_object_type( p_class, mat[j], j, analy, NULL ) )
                    continue;

                for ( k = 0; k < 3; k++ )
                    vert_idx[k] = node_nums[ connects3[j][k] ];

                output_hid_face( vert_idx, num_instances, reverse_order, FALSE,
                                 outfile );
            }
        }
    }

    /*
     * Output beams and trusses.
     */
    fprintf( outfile, "LINES %d\n",
             (superclasses[G_BEAM] + superclasses[G_TRUSS]) * num_instances );

    if ( p_mesh->classes_by_sclass[G_BEAM].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_BEAM].list;
        class_qty = p_mesh->classes_by_sclass[G_BEAM].qty;

        for ( i = 0; i < class_qty; i++ )
        {
            p_class = mo_classes[i];
            connects3 = (int (*)[3]) p_class->objects.elems->nodes;
            elem_qty = p_class->qty;
            mat = p_class->objects.elems->mat;
            activity = analy->state_p->sand_present
                       ? analy->state_p->elem_class_sand[p_class->elem_class_index]
                       : NULL;

            for ( j = 0; j < elem_qty; j++ )
            {
                /* Skip if invisible material or element inactive. */
                if ( hide_material[mat[j]]
                        || ( activity && activity[j] )
                        ||   hide_by_object_type( p_class, mat[j], j, analy, NULL ) )
                    continue;

                vert_idx[0] = node_nums[ connects3[j][0] ];
                vert_idx[1] = node_nums[ connects3[j][1] ];

                output_hid_beam( vert_idx, num_instances, outfile );
            }
        }
    }

    if ( p_mesh->classes_by_sclass[G_TRUSS].qty != 0 )
    {
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TRUSS].list;
        class_qty = p_mesh->classes_by_sclass[G_TRUSS].qty;

        for ( i = 0; i < class_qty; i++ )
        {
            p_class = mo_classes[i];
            connects2 = (int (*)[2]) p_class->objects.elems->nodes;
            elem_qty = p_class->qty;
            mat = p_class->objects.elems->mat;
            activity = analy->state_p->sand_present
                       ? analy->state_p->elem_class_sand[p_class->elem_class_index]
                       : NULL;

            for ( j = 0; j < elem_qty; j++ )
            {
                /* Skip if invisible material or element inactive. */
                if ( hide_material[mat[j]]
                        || ( activity && activity[j] )
                        ||   hide_by_object_type( p_class, mat[j], j, analy, NULL ) )
                    continue;

                vert_idx[0] = node_nums[ connects2[j][0] ];
                vert_idx[1] = node_nums[ connects2[j][1] ];

                output_hid_beam( vert_idx, num_instances, outfile );
            }
        }
    }

    free( node_nums );
    if ( num_instances > 1 )
        free( reverse_order );

    fclose( outfile );
}


/************************************************************
 * TAG( output_hid_vertex )
 *
 * Output a vertex to an .hid file.  This routine performs
 * any reflections required by reflection planes.
 */
static void
output_hid_vertex( float nd_pos[3], int num_instances, FILE *outfile,
                   Analysis *analy )
{
    Refl_plane_obj *plane;
    float inst_pos[64][3];
    int cnt, i;

    /* Output the original vertex. */
    fprintf( outfile, "v %f %f %f\n", nd_pos[0], nd_pos[1], nd_pos[2] );

    if ( num_instances == 1 )
        return;

    /* Reflect the vertex. */
    VEC_COPY( inst_pos[0], nd_pos );

    cnt = 1;
    for ( plane = analy->refl_planes; plane != NULL; plane = plane->next )
    {
        if ( analy->refl_orig_only )
        {
            point_transform( inst_pos[cnt], inst_pos[0],
                             &plane->pt_transf );
            cnt++;
        }
        else
        {
            for ( i = 0; i < cnt; i++ )
                point_transform( inst_pos[cnt+i], inst_pos[i],
                                 &plane->pt_transf );
            cnt *= 2;
        }
    }

    /* Output the reflected vertices. */
    for ( i = 1; i < cnt; i++ )
    {
        fprintf( outfile, "v %f %f %f\n", inst_pos[i][0],
                 inst_pos[i][1], inst_pos[i][2] );
    }
}


/************************************************************
 * TAG( output_hid_face )
 *
 * Output a face to an .hid file.  This routine performs
 * any reflections required by reflection planes.
 */
static void
output_hid_face( int vert_nums[4], int num_instances, Bool_type *reverse_order,
                 Bool_type quad_flag, FILE *outfile )
{
    int i;

    /* Output the original face. */
    if ( quad_flag )
        fprintf( outfile, "f %d %d %d %d\n",
                 vert_nums[0], vert_nums[1], vert_nums[2], vert_nums[3] );
    else
        fprintf( outfile, "f %d %d %d\n",
                 vert_nums[0], vert_nums[1], vert_nums[2] );

    /* We need to reverse node ordering for each reflection. */
    for ( i = 1; i < num_instances; i++ )
    {
        if ( quad_flag )
        {
            if ( reverse_order[i] )
                fprintf( outfile, "f %d %d %d %d\n",
                         vert_nums[3]+i, vert_nums[2]+i,
                         vert_nums[1]+i, vert_nums[0]+i );
            else
                fprintf( outfile, "f %d %d %d %d\n",
                         vert_nums[0]+i, vert_nums[1]+i,
                         vert_nums[2]+i, vert_nums[3]+i );
        }
        else
        {
            if ( reverse_order[i] )
                fprintf( outfile, "f %d %d %d\n",
                         vert_nums[2]+i, vert_nums[1]+i, vert_nums[0]+i );
            else
                fprintf( outfile, "f %d %d %d\n",
                         vert_nums[0]+i, vert_nums[1]+i, vert_nums[2]+i );
        }
    }
}


/************************************************************
 * TAG( output_hid_beam )
 *
 * Output a beam to an .hid file.  This routine performs
 * any reflections required by reflection planes.
 */
static void
output_hid_beam( int vert_nums[4], int num_instances, FILE *outfile )
{
    int i;

    /* Output the original beam. */
    fprintf( outfile, "b %d %d\n", vert_nums[0], vert_nums[1] );

    /* We need to reverse node ordering for each reflection. */
    for ( i = 1; i < num_instances; i++ )
        fprintf( outfile, "b %d %d\n", vert_nums[0]+i, vert_nums[1]+i );
}


/************************************************************
 * TAG( mark_class_nodes )
 *
 * Traverse element classes selected by their superclass and mark
 * all nodes touched by visible elements in the classes.  The count
 * of elements/faces generating touched nodes per superclass is
 * passed back in array superclasses.
 */
static void
mark_class_nodes( Analysis *analy, Mesh_data *p_mesh,
                  int superclasses[QTY_SCLASS], int *node_nums )
{
    int i, j, k, l, el, fc, nd;
    int obj_qty, class_qty, obj_cnt;
    MO_class_data **p_mo_classes;
    MO_class_data *p_class;
    int (*connects8)[8];
    int (*connects4)[4];
    int (*connects3)[3];
    int (*connects2)[2];
    int *face_el, *face_fc;
    Visibility_data *p_vd;
    int *mat;
    unsigned char *hide_mtl;
    State2 *p_state;
    float **class_activ;
    float *activ;

    hide_mtl = p_mesh->hide_material;
    p_state = analy->state_p;
    class_activ = p_state->elem_class_sand;

    for ( i = 0; i < QTY_SCLASS; i++ )
    {
        if ( superclasses[i] != 1 )
            continue;

        class_qty = p_mesh->classes_by_sclass[i].qty;
        p_mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[i].list;

        obj_cnt = 0;

        for ( j = 0; j < class_qty; j++ )
        {
            switch ( i )
            {
            case G_TRUSS:
                p_class = p_mo_classes[j];
                activ = p_state->sand_present
                        ? class_activ[p_class->elem_class_index] : NULL;
                connects2 = (int (*)[2]) p_class->objects.elems->nodes;
                mat = p_class->objects.elems->mat;
                obj_qty = p_class->qty;

                for ( k = 0; k < obj_qty; k++ )
                {
                    /* Skip if invisible material or element deleted. */
                    if ( hide_mtl[mat[k]]
                            || ( activ && activ[k] == 0.0 )
                            ||   hide_by_object_type( p_class, mat[k], k, analy, NULL ) )
                        continue;

                    obj_cnt++;

                    node_nums[ connects2[k][0] ] = 1;
                    node_nums[ connects2[k][1] ] = 1;
                }
                break;

            case G_BEAM:
                p_class = p_mo_classes[j];
                activ = p_state->sand_present
                        ? class_activ[p_class->elem_class_index] : NULL;
                connects3 = (int (*)[3]) p_class->objects.elems->nodes;
                mat = p_class->objects.elems->mat;
                obj_qty = p_class->qty;

                for ( k = 0; k < obj_qty; k++ )
                {
                    /* Skip if invisible material or element deleted. */
                    if ( hide_mtl[mat[k]]
                            || ( activ && activ[k] == 0.0 )
                            ||   hide_by_object_type( p_class, mat[k], k, analy, NULL ))
                        continue;

                    obj_cnt++;

                    node_nums[ connects3[k][0] ] = 1;
                    node_nums[ connects3[k][1] ] = 1;
                }
                break;

            case G_TRI:
                p_class = p_mo_classes[j];
                activ = p_state->sand_present
                        ? class_activ[p_class->elem_class_index] : NULL;
                connects3 = (int (*)[3]) p_class->objects.elems->nodes;
                mat = p_class->objects.elems->mat;
                obj_qty = p_class->qty;

                for ( k = 0; k < obj_qty; k++ )
                {
                    /* Skip if invisible material or element deleted. */
                    if ( hide_mtl[mat[k]]
                            || ( activ && activ[k] == 0.0 )
                            ||   hide_by_object_type( p_class, mat[k], k, analy, NULL ) )
                        continue;

                    obj_cnt++;

                    for ( l = 0; l < 3; l++ )
                        node_nums[ connects3[k][l] ] = 1;
                }
                break;

            case G_QUAD:
                p_class = p_mo_classes[j];
                activ = p_state->sand_present
                        ? class_activ[p_class->elem_class_index] : NULL;
                connects4 = (int (*)[4]) p_class->objects.elems->nodes;
                mat = p_class->objects.elems->mat;
                obj_qty = p_class->qty;

                for ( k = 0; k < obj_qty; k++ )
                {
                    /* Skip if invisible material or element deleted. */
                    if ( hide_mtl[mat[k]]
                            || ( activ && activ[k] == 0.0 )
                            ||   hide_by_object_type( p_class, mat[k], k, analy, NULL ) )
                        continue;

                    obj_cnt++;

                    for ( l = 0; l < 4; l++ )
                        node_nums[ connects4[k][l] ] = 1;
                }
                break;

            case G_TET:
                p_class = p_mo_classes[j];
                connects4 = (int (*)[4]) p_class->objects.elems->nodes;
                p_vd = p_class->p_vis_data;
                obj_qty = p_vd->face_cnt;
                obj_cnt += obj_qty;
                face_el = p_vd->face_el;
                face_fc = p_vd->face_fc;

                for ( k = 0; k < obj_qty; k++ )
                {
                    el = face_el[k];
                    fc = face_fc[k];

                    for ( l = 0; l < 3; l++ )
                    {
                        nd = connects4[el][ tet_fc_nd_nums[fc][l] ];
                        node_nums[nd] = 1;
                    }
                }
                break;

            case G_HEX:
                p_class = p_mo_classes[j];
                connects8 = (int (*)[8]) p_class->objects.elems->nodes;
                p_vd = p_class->p_vis_data;
                obj_qty = p_vd->face_cnt;
                obj_cnt += obj_qty;
                face_el = p_vd->face_el;
                face_fc = p_vd->face_fc;

                for ( k = 0; k < obj_qty; k++ )
                {
                    el = face_el[k];
                    fc = face_fc[k];

                    for ( l = 0; l < 4; l++ )
                    {
                        nd = connects8[el][ fc_nd_nums[fc][l] ];
                        node_nums[nd] = 1;
                    }
                }
                break;
            }
        }

        superclasses[i] = obj_cnt;
    }
}


#ifdef OLD_BECKER_VV
/* Barry Becker's volume visualization output routines. */


/************************************************************
 * TAG( write_vv_connectivity_file )
 *
 * Write the mesh connectivity for use in Barry Becker's
 * volume visualization code.
 */
void
write_vv_connectivity_file( char *fname, Analysis *analy )
{
    FILE *outfile;
    Hex_geom *bricks;
    char str[80];
    int ival[8];
    int i;

    bricks = analy->geom_p->bricks;
    if ( bricks == NULL )
        return;

    if ( ( outfile = fopen(fname, "wb") ) == NULL )
    {
        popup_dialog( INFO_POPUP, "Unable to open file %s", fname );
        return;
    }

    /* Number of bricks. */
    ival[0] = bricks->cnt;
    fwrite( ival, sizeof(int), 1, outfile );

    /* Max number of nodes/elem. */
    ival[0] = 8;
    fwrite( ival, sizeof(int), 1, outfile );

    /* Output the connectivity. */
    for ( i = 0; i < bricks->cnt; i++ )
    {
        ival[0] = bricks->nodes[0][i];
        ival[1] = bricks->nodes[1][i];
        ival[2] = bricks->nodes[2][i];
        ival[3] = bricks->nodes[3][i];
        ival[4] = bricks->nodes[4][i];
        ival[5] = bricks->nodes[5][i];
        ival[6] = bricks->nodes[6][i];
        ival[7] = bricks->nodes[7][i];
        fwrite( ival, sizeof(int), 8, outfile );
    }

    /* Number of bricks (again). */
    ival[0] = 0;
    /*
        ival[0] = bricks->cnt;
    */
    fwrite( ival, sizeof(int), 1, outfile );

    /* Number of nodes/brick for each brick. */
    /*
        for ( i = 0; i < bricks->cnt; i++ )
        {
            ival[0] = 8;
            fwrite( ival, sizeof(int), 1, outfile );
        }
    */

    fclose( outfile );
}


/************************************************************
 * TAG( write_vv_state_file )
 *
 * Write the current mesh node positions and nodal result to
 * a file for use in Barry Becker's volume visualization code.
 */
void
write_vv_state_file( char *fname, Analysis *analy )
{
    FILE *outfile;
    Nodal *nodes;
    char str[80];
    float val[7];
    int ival[2];
    int i;

    nodes = analy->state_p->nodes;

    if ( ( outfile = fopen(fname, "wb") ) == NULL )
    {
        popup_dialog( INFO_POPUP, "Unable to open file %s", fname );
        return;
    }

    /* State time. */
    val[0] = analy->state_times[analy->cur_state];
    fwrite( val, sizeof(float), 1, outfile );

    /* Number of nodes. */
    ival[0] = nodes->cnt;
    fwrite( ival, sizeof(int), 1, outfile );

    /* Number of results. */
    /*
        ival[0] = 1;
    */
    ival[0] = 4;
    fwrite( ival, sizeof(int), 1, outfile );


    /* Get node velocities. */
    get_result( VAL_NODE_VELX, analy->cur_state, analy->tmp_result[0] );
    get_result( VAL_NODE_VELY, analy->cur_state, analy->tmp_result[1] );
    get_result( VAL_NODE_VELZ, analy->cur_state, analy->tmp_result[2] );

    /* Output the nodes. */
    for ( i = 0; i < nodes->cnt; i++ )
    {
        val[0] = nodes->xyz[0][i];
        val[1] = nodes->xyz[1][i];
        val[2] = nodes->xyz[2][i];
        /*
                val[3] = analy->result[i];
        */

        val[3] = analy->tmp_result[0][i];
        val[4] = analy->tmp_result[1][i];
        val[5] = analy->tmp_result[2][i];

        val[6] = analy->result[i];

        fwrite( val, sizeof(float), 7, outfile );
        /*
                fwrite( val, sizeof(float), 4, outfile );
        */
    }
    fclose( outfile );
}
#endif


/************************************************************
 * TAG( write_geom_file )
 *
 * Write the current mesh (node & element, or surface
 * geometry) to a Nike/Dyna input file.
 */
void
write_geom_file( char *fname, Analysis *analy, Bool_type surface_only )
{
    FILE *fp;
    MO_class_data *p_class, *p_node_class, **mo_classes;
    Mesh_data *p_mesh;
    int *nodeNums, nodeNum, *mats, elemId=0;
    int el, fc;
    int labelNum;
    int i, j, k;

    unsigned char *hide_material;
    float *nodes3dsp=NULL;
    double *nodes3d2p=NULL;
    State_rec_obj *p_sro;

    int dummyInt=0;

    int qty_mat_classes=0;
    int qty_hex_classes=0, qty_shell_classes=0, qty_beam_classes=0, qty_truss_classes=0;
    int qty_hex_elems=0, qty_shell_elems=0, qty_beam_elems=0, qty_truss_elems=0;

    Bool_type *nodesUsed; /* A list of active nodes */
    int qty_nodes=0, nodeIndex=0, nodes_used_cnt=0;
    int qty_mats=0;

    float verts[4][3];
    double nodePos=0.0;

    /* Face variables */
    Visibility_data *p_vd;
    int face_qty=0, face_list_qty=0, face_list_index=0, face_list_cnt=0, *face_el, *face_fc;
    int faceNodes[4], *faceNodeList, *faceMatList;
    char lineOut[100];

    int tempCnt=0;

    if (MESH_P( analy )->double_precision_nodpos)
    {
        p_sro = analy->srec_tree + analy->state_p->srec_id;
        load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                     analy->cur_state + 1, FALSE,
                     (void *)  analy->tmp_result[0] );
        nodes3d2p = (double *) analy->tmp_result[0];
    }
    else
        nodes3dsp = analy->state_p->nodes.nodes;

    p_mesh        = MESH_P( analy );
    hide_material = p_mesh->hide_material;
    p_node_class  = p_mesh->node_geom;
    qty_nodes     = p_node_class->qty;

    if ( ( fp = fopen(fname, "w") ) == NULL )
    {
        popup_dialog( INFO_POPUP, "Unable to open file for outgeom: %s", fname );
        return;
    }

    if ( analy->dimension != 3 )
    {
        popup_dialog( INFO_POPUP, "This function only processes 3D models - this model is 2D" );
        return;
    }

    /* Get mat, node, and element counts */

    qty_mat_classes   = p_mesh->classes_by_sclass[G_MAT].qty;
    qty_hex_classes   = p_mesh->classes_by_sclass[G_HEX].qty;
    qty_shell_classes = p_mesh->classes_by_sclass[G_QUAD].qty;
    qty_beam_classes  = p_mesh->classes_by_sclass[G_BEAM].qty;
    qty_truss_classes = p_mesh->classes_by_sclass[G_TRUSS].qty;

    nodesUsed = NEW_N( Bool_type, qty_nodes, "Node output tmp array" );
    for ( i= 0;
            i < qty_nodes;
            i++ )
        nodesUsed[i] = FALSE;

    /* Get number of materials */

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_MAT].list;
    for ( i=0;
            i<qty_mat_classes;
            i++ )
    {
        qty_mats += mo_classes[i]->qty;
    }

    /* Go through all the elements and mark the nodes used */

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_HEX].list;
    for ( i=0;
            i<qty_hex_classes;
            i++ )
    {
        p_class = mo_classes[i];
        if ( is_particle_class( analy, p_class->superclass, p_class->short_name ) )
            continue;

        qty_hex_elems += p_class->qty;
        nodeNums = p_class->objects.elems->nodes;
        mats     = p_class->objects.elems->mat;

        if ( surface_only )
        {
            p_vd     = p_class->p_vis_data;
            face_qty = p_vd->face_cnt;
            face_el  = p_vd->face_el;
            face_fc  = p_vd->face_fc;

            nodeIndex=0;
            for ( j = 0;
                    j < face_qty;
                    j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                /*
                 * Remove faces that are shared with quad elements, so
                 * the polygons are not drawn twice.
                 */
                if ( hide_material[mats[el]] )
                    continue;
                if ( analy->shared_faces
                        && p_mesh->classes_by_sclass[G_QUAD].qty > 0
                        && face_matches_quad( el, fc, p_class, p_mesh, analy ) )
                    continue;

                face_list_qty++;
                get_hex_face_nodes( el, fc, p_class, analy, faceNodes );
                for ( k=0;
                        k<4;
                        k++ )
                {
                    if ( !nodesUsed[faceNodes[k]] )
                    {
                        nodes_used_cnt++;
                        nodesUsed[faceNodes[k]]=TRUE;
                    }
                    nodeIndex++;
                }

            }
        }
        else
        {
            nodeIndex=0;
            for ( j=0;
                    j<p_class->qty;
                    j++ )
            {
                if ( !hide_material[mats[j]] )
                    for ( k=0;
                            k<8;
                            k++ )
                    {
                        if ( !nodesUsed[nodeNums[nodeIndex]] )
                        {
                            nodes_used_cnt++;
                            nodesUsed[nodeNums[nodeIndex]]=TRUE;
                        }
                        nodeIndex++;
                    }
            }
        }
    }

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_QUAD].list;
    for ( i=0;
            i<qty_shell_classes;
            i++ )
    {

        p_class = mo_classes[i];
        qty_shell_elems += p_class->qty;
        nodeNums = p_class->objects.elems->nodes;
        mats      = p_class->objects.elems->mat;

        /* All Quads by definition are on the surface */

        nodeIndex=0;
        for ( j=0;
                j<p_class->qty;
                j++ )
        {
            if ( !hide_material[mats[j]] )
                for ( k=0;
                        k<4;
                        k++ )
                {
                    if ( !nodesUsed[nodeNums[nodeIndex]] )
                    {
                        nodes_used_cnt++;
                        nodesUsed[nodeNums[nodeIndex]]=TRUE;
                    }
                    nodeIndex++;
                }
        }
    }

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_BEAM].list;
    for ( i=0;
            i<qty_beam_classes;
            i++ )
    {

        p_class = mo_classes[i];
        qty_beam_elems += p_class->qty;
        nodeNums  = p_class->objects.elems->nodes;
        mats      = p_class->objects.elems->mat;

        nodeIndex=0;
        for ( j=0;
                j<p_class->qty;
                j++ )
        {
            if ( !hide_material[mats[j]] )
                for ( k=0;
                        k<3;
                        k++ )
                {
                    if ( !nodesUsed[nodeNums[nodeIndex]] )
                    {
                        nodes_used_cnt++;
                        nodesUsed[nodeNums[nodeIndex]]=TRUE;
                    }
                    nodeIndex++;
                }
        }
    }

    /* Write header data */

    fprintf( fp, "\n*--------------------- ANALYSIS OUTPUT DATA FROM GRIZ ------------------*\n" );
    fprintf( fp, "* Griz Version: \t%s\n", GRIZ_VERSION );
    fprintf( fp, "* Generated on: \t%s\n", env.date );
    fprintf( fp, "*\n" );
    fprintf( fp, "* Plotfile name: \t%s\n", env.plotfile_name );
    fprintf( fp, "* Plotfile title: \t%s\n", analy->title );
    fprintf( fp, "*\n" );

    /* Write summary data */

    fprintf( fp, "**-------------------------- CONTROL CARD #2 ---------------------------*\n" );
    fprintf( fp, "**\n" );
    fprintf( fp, "* number of materials[1] nodal points[2] solid hexahedron elements[3] beam\n" );
    fprintf( fp, "* elements[4] 4-node shell elements[5] 8-node solid shell elements[6]\n" );
    fprintf( fp, "* interface segments[7] interface interval[8] min. shell time step[9]\n" );

    if ( !surface_only )
        fprintf( fp, "%5d%10d%10d%10d%10d%10d 0.000E+00  0.0\n",
                 qty_mats, nodes_used_cnt, qty_hex_elems, qty_beam_elems, qty_shell_elems,
                 dummyInt );
    else
        fprintf( fp, "%5d%10d%10d%10d%10d%10d 0.000E+00  0.0\n",
                 qty_mats, nodes_used_cnt, 0, 0, face_list_qty,
                 dummyInt );

    if ( nodes_used_cnt==0 )
    {
        popup_dialog( INFO_POPUP, "There were no nodes visible to output." );
        return;
    }

    /* Write place holder for material cards */
    fprintf( fp, "*\n" );
    fprintf( fp, "*--------------------------- MATERIAL CARDS ---------------------------*\n" );
    fprintf( fp, "*\n" );

    /* Write the nodes */
    fprintf( fp, "*\n" );
    fprintf( fp, "*-------------------------- NODE DEFINITIONS --------------------------*\n" );
    fprintf( fp, "*\n" );

    nodeIndex=0;
    for ( i= 0;
            i < qty_nodes;
            i++ )
    {
        if ( !nodesUsed[i] )
            continue;

        tempCnt++;

        if ( p_node_class->labels_found )
            labelNum = get_class_label( p_node_class, i+1 );
        else
            labelNum = i+1;
        fprintf( fp, "%8d    0 ", labelNum );

        for ( j = 0;
                j < 3;
                j++ )
        {
            if ( !MESH_P( analy )->double_precision_nodpos ||  nodes3d2p==NULL )
                nodePos = (double) nodes3dsp[nodeIndex];
            else
                nodePos = (double) nodes3d2p[nodeIndex];
            nodeIndex++;
            sprintf( lineOut, "%+15.13e ", nodePos );
            if ( lineOut[0]=='+' )
                lineOut[0]=' ';
            fprintf( fp, "%s", lineOut );
        }
        fprintf( fp, "     0\n" );
    }

    free( nodesUsed );

    /* Write Hex Connectivity  */
    fprintf( fp, "*\n" );
    fprintf( fp, "*------------------ ELEMENT CARDS FOR SOLID ELEMENTS ------------------*\n" );
    fprintf( fp, "*\n" );

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_HEX].list;
    for ( i=0;
            i<qty_hex_classes;
            i++ )
    {

        p_class = mo_classes[i];
        if ( is_particle_class( analy, p_class->superclass, p_class->short_name ) )
            continue;
        nodeNums = p_class->objects.elems->nodes;
        mats     = p_class->objects.elems->mat;

        if ( surface_only )
        {
            p_vd     = p_class->p_vis_data;
            face_qty = p_vd->face_cnt;
            face_el  = p_vd->face_el;
            face_fc  = p_vd->face_fc;

            faceNodeList = NEW_N( int, face_list_qty*4, "Node list for all surface faces" );
            faceMatList = NEW_N( int, face_list_qty, "Material list for all surface faces" );
            for ( j = 0;
                    j < face_qty;
                    j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                /*
                 * Remove faces that are shared with quad elements, so
                 * the polygons are not drawn twice.
                 */
                if ( hide_material[mats[el]] )
                    continue;

                faceMatList[face_list_cnt++] = mats[el];

                if ( analy->shared_faces
                        && p_mesh->classes_by_sclass[G_QUAD].qty > 0
                        && face_matches_quad( el, fc, p_class, p_mesh, analy ) )
                    continue;

                get_hex_face_nodes( el, fc, p_class, analy, faceNodes );
                for ( k=0;
                        k<4;
                        k++ )
                    faceNodeList[face_list_index++] = faceNodes[k];
            }
        }
        else
        {
            nodeIndex=0;
            elemId=1;
            for ( j=0;
                    j<p_class->qty;
                    j++ )
            {

                /* Exclude hidden materials */
                if ( hide_material[mats[j]] )
                {
                    elemId++;
                    continue;
                }

                if ( p_class->labels_found )
                    labelNum = get_class_label( p_class, elemId );
                else
                    labelNum = elemId;

                fprintf( fp, "%8d%5d ", labelNum, mats[j]+1 );

                for ( k=0;
                        k<8;
                        k++ )
                {
                    nodeNum = nodeNums[nodeIndex++];
                    fprintf( fp, "%8d", nodeNum+1 );
                }
                fprintf( fp, "\n" );
                elemId++;
            }
        }
    }

    /* Write Shell Connectivity  */
    fprintf( fp, "*\n" );
    fprintf( fp, "*------------------ ELEMENT CARDS FOR SHELL ELEMENTS ------------------*\n" );
    fprintf( fp, "*\n" );

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_QUAD].list;
    for ( i=0;
            i<qty_shell_classes;
            i++ )
    {

        p_class  = mo_classes[i];
        nodeNums = p_class->objects.elems->nodes;
        mats     = p_class->objects.elems->mat;

        nodeIndex=0;
        elemId=1;
        for ( j=0;
                j<p_class->qty;
                j++ )
        {

            /* Exclude hidden materials */
            if ( hide_material[mats[j]] )
            {
                elemId++;
                continue;
            }

            if ( p_class->labels_found )
                labelNum = get_class_label( p_class, elemId );
            else
                labelNum = elemId;

            fprintf( fp, "%8d%5d ", labelNum, mats[j]+1 );

            for ( k=0;
                    k<4;
                    k++ )
            {
                nodeNum = nodeNums[nodeIndex++];
                fprintf( fp, "%8d", nodeNum+1 );
            }
            fprintf( fp, "\n" );
            elemId++;
        }
    }

    /* Write out surface faces if we are generating surface geometry */
    face_list_index=0;
    if ( elemId==0 )
        elemId=1;
    for ( i=0;
            i<face_list_qty;
            i++)
    {
        fprintf( fp, "%8d%5d ", elemId++, faceMatList[i]+1 );

        for ( k=0;
                k<4;
                k++ )
        {
            nodeNum = faceNodeList[face_list_index++];
            fprintf( fp, "%8d", nodeNum+1 );
        }
        fprintf( fp, "\n" );
    }

    if ( face_list_qty>0 )
    {
        free( faceNodeList );
        free( faceMatList );
    }

    /* Write Beam Connectivity  */
    fprintf( fp, "*\n" );
    fprintf( fp, "*------------------ ELEMENT CARDS FOR BEAM ELEMENTS ------------------*\n" );
    fprintf( fp, "*\n" );

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_BEAM].list;
    for ( i=0;
            i<qty_beam_classes;
            i++ )
    {

        p_class  = mo_classes[i];
        nodeNums = p_class->objects.elems->nodes;
        mats     = p_class->objects.elems->mat;

        nodeIndex=0;
        elemId=1;
        for ( j=0;
                j<p_class->qty;
                j++ )
        {

            /* Exclude hidden materials */
            if ( hide_material[mats[j]] )
            {
                elemId++;
                continue;
            }

            if ( p_class->labels_found )
                labelNum = get_class_label( p_class, elemId );
            else
                labelNum = elemId;

            fprintf( fp, "%8d%5d ", labelNum, mats[j]+1 );

            for ( k=0;
                    k<3;
                    k++ )
            {
                nodeNum = nodeNums[nodeIndex++];
                fprintf( fp, "%8d", nodeNum+1 );
            }
            fprintf( fp, "\n" );
            elemId++;
        }
    }
    fflush( fp );
    fclose( fp );
}

