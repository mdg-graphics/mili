/* $Id$ */
/* 
 * poly.c - Read in and write out polygon data.
 * 
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Jan 2 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */


#include "viewer.h"
#include "draw.h"


/* Local routines. */
static float obj_color_lookup();
static void output_obj_vertex();
static void output_obj_face();
static void output_hid_vertex();
static void output_hid_face();
static void output_hid_beam();


/************************************************************
 * TAG( read_slp_file )
 *
 * Read in polygons from an slp file.
 */
void
read_slp_file( fname, analy )
char *fname;
Analysis *analy;
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
        wrt_text( "Couldn't open file %s.\n", fname );
        return;
    }

    wrt_text( "\n" );

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
                if ( poly_cnt % 500 == 0 )
                    wrt_text( "." );
            }

            read_token( infile, token, 80 );
        }

        /* Get endsolid's name. */
        read_token( infile, token, 80 );

        /* Get "solid". */
        read_token( infile, token, 80 );
    }

    wrt_text( "\n\n" );
    wrt_text( "Read %d polygons from SLP file.\n", poly_cnt );

    fclose( infile );
}


/************************************************************
 * TAG( read_ref_file )
 *
 * Read in a reference surface file.
 */
void
read_ref_file( fname, analy )
char *fname;
Analysis *analy;
{
    FILE *infile;
    Ref_poly *poly;
    char token[80];
    int poly_cnt;
    int nd, i, j;

    /*
     * The ref surface file gives us a way to refer to particular
     * surfaces on hex volume elements.  The file is organized as
     * follows:
     *
     *     N_srf
     *     n1 n2 n3 n4
     *     n1 n2 n3 n4
     *     .
     *     .
     *     .
     *
     * This routine reads in the ref surface file and puts the
     * ref surface polygons on the external surface list.  This
     * way, display of these surface polygons can be turned on
     * and off separately from the rest of the mesh.
     */

    if ( ( infile = fopen(fname, "r") ) == NULL )
    {
        wrt_text( "Couldn't open file %s.\n", fname );
        return;
    }

    read_token( infile, token, 80 );
    sscanf( token, "%d", &poly_cnt );

    for ( i = 0; i < poly_cnt; i++ )
    {
        poly = NEW( Ref_poly, "Reference polygon" );
        poly->mat = 0;

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


/************************************************************
 * TAG( gen_ref_from_coord )
 *
 * Generate a reference surface by finding all element faces
 * that lie in a particular axis-aligned plane.
 */
void
gen_ref_from_coord( analy, axis, coord )
Analysis *analy;
int axis;
float coord;
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
get_ref_average_area( analy )
Analysis *analy;
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


/* Wavefront .obj format output routines. */

/************************************************************
 * TAG( obj_color_lookup )
 *
 * Return the colormap value for a node in the mesh using the
 * same lookup procedure as in "color_lookup()".  The colormap
 * value is a real number in the range [0.0, 1.0].
 */
static float
obj_color_lookup( val, result_min, result_max, threshold )
float val;
float result_min;
float result_max;
float threshold;
{
    int idx;

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
        idx = (int)( 253.99*(val-result_min)/(result_max-result_min) ) + 1;
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
write_obj_file( fname, analy )
char *fname;
Analysis *analy;
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
    int num_matls, *matl_cnts;
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
            el = analy->face_el[j];
            fc = analy->face_fc[j];

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


/************************************************************
 * TAG( output_obj_vertex )
 *
 * Output a vertex to an .obj file.  This routine performs
 * any reflections required by reflection planes.
 */
static void
output_obj_vertex( nd, nd_pos, num_instances, outfile, analy )
int nd;
float nd_pos[3];
int num_instances;
FILE *outfile;
Analysis *analy;
{
    Refl_plane_obj *plane;
    float inst_pos[64][3];
    float cmap_idx;
    int cnt, i;

    /* Output the original vertex. */
    fprintf( outfile, "v %f %f %f\n", nd_pos[0], nd_pos[1], nd_pos[2] );

    if ( analy->show_hex_result || analy->show_shell_result )
    {
        cmap_idx = obj_color_lookup( analy->result[nd], analy->result_mm[0],
                                     analy->result_mm[1], analy->zero_result );
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

        if ( analy->show_hex_result || analy->show_shell_result )
            fprintf( outfile, "vt %f %f %f\n", cmap_idx, cmap_idx, cmap_idx );
    }
}


/************************************************************
 * TAG( output_obj_face )
 *
 * Output a face to an .obj file.  This routine performs
 * any reflections required by reflection planes.
 */
static void
output_obj_face( vert_nums, num_instances, reverse_order, outfile )
int vert_nums[4];
int num_instances;
Bool_type *reverse_order;
FILE *outfile;
{
    int i;

    /* Output the original face. */
    fprintf( outfile, "f %d/%d %d/%d %d/%d %d/%d\n",
             vert_nums[0], vert_nums[0],
             vert_nums[1], vert_nums[1],
             vert_nums[2], vert_nums[2],
             vert_nums[3], vert_nums[3] );

    /* We need to reverse node ordering for each reflection. */
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


/* HIDDEN .hid output routines. */

/************************************************************
 * TAG( output_hid_view )
 *
 * Write the current view parameters to a .hid file.
 */
static void
output_hid_view( outfile, analy )
FILE *outfile;
Analysis *analy;
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
void
write_hid_file( fname, analy )
char *fname;
Analysis *analy;
{
    FILE *outfile;
    Hex_geom *bricks;
    Shell_geom *shells;
    Beam_geom *beams;
    Nodal *nodes, *onodes;
    Refl_plane_obj *plane;
    char str[80];
    Bool_type *reverse_order;
    float *activity;
    float orig;
    float vert[3];
    int vert_idx[4];
    int node_cnt, face_cnt, shell_cnt, beam_cnt;
    int num_instances;
    int *node_nums;
    int i, j, el, fc, nd, cnt;

    /*
     * Open the output file.
     */
    if ( ( outfile = fopen(fname, "w") ) == NULL )
    {
        wrt_text( "Couldn't open file %s.\n", fname );
        return;
    }

    /*
     * Output the current view parameters.
     */
    output_hid_view( outfile, analy );


    /*
     * Output nodes, faces, shells, beams.
     */

    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;
    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    beams = analy->geom_p->beams;

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
    node_nums = NEW_N( int, nodes->cnt, "Hidden output tmp array" );

    face_cnt = analy->face_cnt;
    if ( bricks != NULL )
    {
        for ( i = 0; i < face_cnt; i++ )
        {
            el = analy->face_el[i];
            fc = analy->face_fc[i];

            for ( j = 0; j < 4; j++ )
            {
                nd = bricks->nodes[ fc_nd_nums[fc][j] ][el];
                node_nums[nd] = 1;
            }
        }
    }

    shell_cnt = 0;
    if ( shells != NULL )
    {
        activity = analy->state_p->activity_present ?
                   analy->state_p->shells->activity : NULL;

        for ( i = 0; i < shells->cnt; i++ )
        {
            /* Skip if this material is invisible. */
            if ( analy->hide_material[shells->mat[i]] )
                 continue;

            if ( activity && activity[i] == 0.0 )
                continue;

            shell_cnt++;

            for ( j = 0; j < 4; j++ )
            {
                nd = shells->nodes[j][i];
                node_nums[nd] = 1;
            }
        }
    }

    beam_cnt = 0;
    if ( beams != NULL )
    {
        activity = analy->state_p->activity_present ?
                   analy->state_p->beams->activity : NULL;

        for ( i = 0; i < beams->cnt; i++ )
        {
            /* Skip if this material is invisible. */
            if ( analy->hide_material[beams->mat[i]] )
                 continue;

            if ( activity && activity[i] == 0.0 )
                continue;

            beam_cnt++;

            node_nums[ beams->nodes[0][i] ] = 1;
            node_nums[ beams->nodes[1][i] ] = 1;
        }
    }

    /*
     * Count the nodes.
     */
    for ( node_cnt = 0, i = 0; i < nodes->cnt; i++ )
    {
        if ( node_nums[i] == 1 )
            node_cnt++;
    }

    /*
     * Output the nodes.
     */
    fprintf( outfile, "NODES %d\n", node_cnt*num_instances );

    for ( i = 0, cnt = 0; i < nodes->cnt; i++ )
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

            output_hid_vertex( vert, num_instances, outfile, analy );

            node_nums[i] = cnt;
            cnt += num_instances;
        }
    }

    /*
     * Output brick faces.
     */
    fprintf( outfile, "ONE_SIDED_POLYS %d\n", face_cnt*num_instances );

    if ( bricks != NULL )
    {
        for ( i = 0; i < face_cnt; i++ )
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

            for ( j = 0; j < 4; j++ )
            {
                nd = bricks->nodes[ fc_nd_nums[fc][j] ][el];
                vert_idx[j] = node_nums[nd];
            }

            output_hid_face( vert_idx, num_instances,
                             reverse_order, outfile );
        }
    }

    /*
     * Output shell faces.
     */
    fprintf( outfile, "TWO_SIDED_POLYS %d\n", shell_cnt*num_instances );

    if ( shells != NULL )
    {
        activity = analy->state_p->activity_present ?
                   analy->state_p->shells->activity : NULL;

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
                vert_idx[j] = node_nums[nd];
            }

            output_hid_face( vert_idx, num_instances, reverse_order, outfile );
        }
    }

    /*
     * Output beams.
     */
    fprintf( outfile, "LINES %d\n", beam_cnt*num_instances );

    if ( beams != NULL )
    {
        activity = analy->state_p->activity_present ?
                   analy->state_p->beams->activity : NULL;

        for ( i = 0; i < beams->cnt; i++ )
        {
            /* Check for inactive elements. */
            if ( activity && activity[i] == 0.0 )
                continue;

            /* Skip if this material is invisible. */
            if ( analy->hide_material[beams->mat[i]] )
                 continue;

            for ( j = 0; j < 2; j++ )
            {
                nd = beams->nodes[j][i];
                vert_idx[j] = node_nums[nd];
            }

            output_hid_beam( vert_idx, num_instances, outfile );
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
output_hid_vertex( nd_pos, num_instances, outfile, analy )
float nd_pos[3];
int num_instances;
FILE *outfile;
Analysis *analy;
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
output_hid_face( vert_nums, num_instances, reverse_order, outfile )
int vert_nums[4];
int num_instances;
Bool_type *reverse_order;
FILE *outfile;
{
    int i;

    /* Output the original face. */
    fprintf( outfile, "f %d %d %d %d\n",
             vert_nums[0], vert_nums[1], vert_nums[2], vert_nums[3] );

    /* We need to reverse node ordering for each reflection. */
    for ( i = 1; i < num_instances; i++ )
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
}


/************************************************************
 * TAG( output_hid_beam )
 *
 * Output a beam to an .hid file.  This routine performs
 * any reflections required by reflection planes.
 */
static void
output_hid_beam( vert_nums, num_instances, outfile )
int vert_nums[4];
int num_instances;
FILE *outfile;
{
    int i;

    /* Output the original beam. */
    fprintf( outfile, "b %d %d\n", vert_nums[0], vert_nums[1] );

    /* We need to reverse node ordering for each reflection. */
    for ( i = 1; i < num_instances; i++ )
        fprintf( outfile, "b %d %d\n", vert_nums[0]+i, vert_nums[1]+i );
}


/* Barry Becker's volume visualization output routines. */


/************************************************************
 * TAG( write_vv_connectivity_file )
 *
 * Write the mesh connectivity for use in Barry Becker's
 * volume visualization code.
 */
void
write_vv_connectivity_file( fname, analy )
char *fname;
Analysis *analy;
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
        wrt_text( "Couldn't open file %s.\n", fname );
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
write_vv_state_file( fname, analy )
char *fname;
Analysis *analy;
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
        wrt_text( "Couldn't open file %s.\n", fname );
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


