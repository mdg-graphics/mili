/* $Id$ */
/* 
 * traction.c:  procedures relating to the computation of traction force(s) 
 * 
 *  Lawrence A. Sanford
 *  Lawrence Livermore National Laboratory
 *  Nuvember 13, 2000
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
/****
#include <unistd.h>
#include <ctype.h>
****/
#include <stdlib.h>
#include "draw.h"
#include "viewer.h"


#define TRACTION_ALLOCATE
#include "traction_force.h"

#include <errno.h>

int surface_command_processed = FALSE;
int traction_data_table_initialized = FALSE;


extern int tokenize( char *p_input, char token_list[][TOKENLENGTH], size_t maxtoken );

/*
 * Establish table of surface function pointers
 */

typedef struct {
                const char         *name;
                p_surface_function  function;
               } Surface_function_table;


/*
 * NOTE:  surface_function_table MUST be established in alphabetical order
 *        for use in binary searching
 */

Surface_function_table
                       surface_function_table[] = {
                                                   "poly", poly
                                                  ,"rect", rect
                                                  ,"ring", ring
                                                  ,"spot", spot
                                                  ,"tube", tube
                                                  };




/*
 * Local parameters -- surface; traction
 */

int    n;
int    validity_status;



/*****************************************************************
 * TAG( parse_surface_command )
 *
 * parse and validate "surface" command parameters
 */
int
parse_surface_command( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt )
{
    int
        validity_status;


    /* */

    if      ( strcmp( tokens[1], "poly" ) == 0 )
    {
        validity_status = parse_poly( tokens, token_cnt );
    }
    else if ( strcmp( tokens[1], "ring" ) == 0 )
    {
        validity_status = parse_ring( tokens, token_cnt );
    }
    else if ( strcmp( tokens[1], "rect" ) == 0 )
    {
        validity_status = parse_rect( tokens, token_cnt );
    }
    else if ( strcmp( tokens[1], "spot" ) == 0 )
    {
        validity_status = parse_spot( tokens, token_cnt );
    }
    else if ( strcmp( tokens[1], "tube" ) == 0 )
    {
        validity_status = parse_tube( tokens, token_cnt );
    }
    else
    {
        popup_dialog( USAGE_POPUP, "surface {spot | ring | rect | tube | poly }" );
        return( invalid_state );
    }


    return( validity_status );
}



/*****************************************************************
 * TAG( parse_traction_command )
 * 
 */
int
parse_traction_command( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH], int token_count )
{
    List_head
              *p_lh;

    MO_class_data
                  **p_classes;

    int
        i
       ,total_traction_materials
       ,traction_material
       ,traction_material_list_established = FALSE
       ,validity_status;


    /* */

    if ( 2 == token_count )
    {
        if ( strcmp( tokens[1], "all" ) == 0 )
        {
            /*
             * get all materials and place into bit array of traction materials
             */

            p_lh = MESH_P( analy )->classes_by_sclass;
            p_classes = (MO_class_data **)p_lh[G_MAT].list;

            total_traction_materials = p_classes[0]->qty;

            if ( NULL == (traction_material_list = NEW_N( char, total_traction_materials, "traction_material_list" )) )
            {
                popup_dialog( WARNING_POPUP, "parse_traction_command:  storage allocation failure." );
                return( invalid_state );
            }


            /*
             * NOTE:  traction_material_list contains "real-world" material numbers
             */

            for ( i = 1; i <= total_traction_materials; i++ )
                 BITSET( traction_material_list, i );

            traction_material_list_established = TRUE;
            validity_status = valid_state;
        }
        else
        {
            popup_dialog( USAGE_POPUP, "traction {all | <qty materials> "
                          "<mat_1>...<mat_n>}" );
            traction_data_table_initialized = FALSE;

            validity_status = invalid_state;
        }
    }
    else if ( 3 <= token_count )
    {
             /*
              * get selected materials and place into bit array of traction materials
              */

             n = (int)is_valid_number( tokens[1], &validity_status );

             if ( invalid_state == validity_status )
             {
                 popup_dialog( WARNING_POPUP, "parse_traction_command:  Invalid qty. of materials." );
                 return( invalid_state );
             }


             p_lh = MESH_P( analy )->classes_by_sclass;
             p_classes = (MO_class_data **)p_lh[G_MAT].list;

             total_traction_materials = p_classes[0]->qty;

             if ( NULL == (traction_material_list = NEW_N( char, total_traction_materials, "traction_material_list" )) )
             {
                 popup_dialog( WARNING_POPUP, "parse_traction_command:  storage allocation failure." );
                 return( invalid_state );
             }


             /*
              * test to determine if traction material id is a valid number and a valid material id
              */

             for ( i = 2; i < token_count; i++ )
             {
                 traction_material = (int)is_valid_number( tokens[ i ], &validity_status );

                 if ( valid_state == validity_status )
                 {
                     if ( (1 <= traction_material)  &&  (traction_material <= total_traction_materials) )
                     {
                         /*
                          * NOTE:  traction_material_list contains "real-world" material numbers
                          */

                         BITSET( traction_material_list, traction_material );

                         traction_material_list_established = TRUE;
                         validity_status = valid_state;
                     }
                 }
                 else
                 {
                     popup_dialog( WARNING_POPUP, "parse_traction_command:  Invalid material id number." );
                     validity_status = invalid_state;
                 }
             }
    }
    else
    {
        popup_dialog( USAGE_POPUP, "traction {all | <qty materials> "
                      "<mat_1>...<mat_n>}" );
        traction_data_table_initialized = FALSE;

        validity_status = invalid_state;
    }

    if ( FALSE == traction_material_list_established )
        validity_status = invalid_state;


    return( validity_status );
}



/*****************************************************************
 * TAG( parse_poly )
 *
 * parse and validate command "poly" parameters
 *
 * NOTE:  "poly" data is placed directly in the traction_data_table
 *        because it requires no intermediate computations and/or modifications
 *
 */
int
parse_poly( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt )
{
    FILE
         *fp_surface_poly_file;

    Vector
           p;

    char
          buffer[TOKENLENGTH]
        , poly_tokens[MAXTOKENS][TOKENLENGTH]
        ,*surface_poly_file;

    float
          area = 0.0;

    int
        i
       ,validity_status;


    /* */

        if ( 3 == token_cnt )
        {
            griz_str_dup( &surface_parameter_table.type, tokens[1] );

            griz_str_dup( &surface_poly_file, tokens[2] );


            if ( NULL != (fp_surface_poly_file = fopen( surface_poly_file, "r" )) )
            {
                if ( NULL == fgets( buffer, sizeof( buffer ), fp_surface_poly_file ) )
                {
                    popup_dialog( WARNING_POPUP, "parse_poly:  Unable to read surface poly file:  %s\n", surface_poly_file );
                    return( invalid_state );
                }


                if ( 1 != tokenize( buffer, poly_tokens, (MAXTOKENS - 1) ) )
                {
                    popup_dialog( WARNING_POPUP, "parse_poly:  Invalid quantity of points OR quantity of points format.\n" );
                    return( invalid_state );
                }


                surface_parameter_table.n = (int)is_valid_number( poly_tokens[0], &validity_status );

                if ( invalid_state == validity_status )
                {
                    popup_dialog( WARNING_POPUP, "parse_poly:  Unable to convert n." );
                    return( invalid_state );
                }

                /*
                 * NOTE:  "poly" does NOT provide for an auto_compute_n mode
                 *
                 *        Do NOT allow a value of surface_parameter_table.n ==> 0
                 */

                if ( 0 >= surface_parameter_table.n )
                {
                    popup_dialog( WARNING_POPUP, "parse_poly:  n MUST be > 0" );
                    return( invalid_state );
                }

                surface_parameter_table.n = MIN( surface_parameter_table.n, MAXIMUM_SURFACE_POINTS );

                /*
                 * NOTE:  traction_data_table is established here rather than in "poly"
                 *
                 *        free the allocated memory in "poly"
                 */

                if ( NULL != (traction_data_table = NEW_N( Traction_data, surface_parameter_table.n, "traction_data_table" )) )
                {
                    traction_data_table_initialized = TRUE;
                }
                else
                {
                    traction_data_table_initialized = FALSE;
                    return( invalid_state );
                }


                p.x = 0.0;
                p.y = 0.0;
                p.z = 0.0;

                for ( i = 0; i < surface_parameter_table.n; i++ )
                {
                    if ( NULL == fgets( buffer, sizeof( buffer ), fp_surface_poly_file ) )
                    {
                        popup_dialog( WARNING_POPUP, "parse_poly:  Unable to read surface poly file:  %s\n", surface_poly_file );
                        return( invalid_state );
                    }


                    if ( 7 == tokenize( buffer, poly_tokens, (MAXTOKENS - 1) ) )
                    {
                        traction_data_table[ i ].point.x = is_valid_number( poly_tokens[0], &validity_status );

                        if ( invalid_state == validity_status )
                        {
                            popup_dialog( WARNING_POPUP, "parse_poly:  Unable to convert px." );
                            return( invalid_state );
                        }

                        traction_data_table[ i ].point.y = is_valid_number( poly_tokens[1], &validity_status );

                        if ( invalid_state == validity_status )
                        {
                            popup_dialog( WARNING_POPUP, "parse_poly:  Unable to convert py." );
                            return( invalid_state );
                        }

                        traction_data_table[ i ].point.z = is_valid_number( poly_tokens[2], &validity_status );

                        if ( invalid_state == validity_status )
                        {
                            popup_dialog( WARNING_POPUP, "parse_poly:  Unable to convert pz." );
                            return( invalid_state );
                        }

                        traction_data_table[ i ].normal.x = is_valid_number( poly_tokens[3], &validity_status );

                        if ( invalid_state == validity_status )
                        {
                            popup_dialog( WARNING_POPUP, "parse_poly:  Unable to convert nx." );
                            return( invalid_state );
                        }

                        traction_data_table[ i ].normal.y = is_valid_number( poly_tokens[4], &validity_status );

                        if ( invalid_state == validity_status )
                        {
                            popup_dialog( WARNING_POPUP, "parse_poly:  Unable to convert ny." );
                            return( invalid_state );
                        }

                        traction_data_table[ i ].normal.z = is_valid_number( poly_tokens[5], &validity_status );

                        if ( invalid_state == validity_status )
                        {
                            popup_dialog( WARNING_POPUP, "parse_poly:  Unable to convert nz." );
                            return( invalid_state );
                        }

                        traction_data_table[ i ].area = is_valid_number( poly_tokens[6], &validity_status );

                        if ( invalid_state == validity_status )
                        {
                            popup_dialog( WARNING_POPUP, "parse_poly:  Unable to convert dA." );
                            return( invalid_state );
                        }
                    }
                    else
                    {
                        popup_dialog( WARNING_POPUP, "parse_poly:  Invalid quantity of data -- px py pz vx vy vz dA" );
                        return( invalid_state );
                    }


                    p.x += (traction_data_table[ i ].point.x * traction_data_table[ i ].area);
                    p.y += (traction_data_table[ i ].point.y * traction_data_table[ i ].area);
                    p.z += (traction_data_table[ i ].point.z * traction_data_table[ i ].area);

                    area += traction_data_table[ i ].area;
                } /* for i */
            }
            else
            {
                popup_dialog( WARNING_POPUP, "parse_poly:  Unable to open poly input file:  %s\n", surface_poly_file );
                return( invalid_state );
            }

            p.x /= area;
            p.y /= area;
            p.z /= area;

            surface_parameter_table.p = p;


            free( surface_poly_file );

            fclose( fp_surface_poly_file );
        }
        else
        {
            popup_dialog( USAGE_POPUP, "poly <file_name>" );
            return( invalid_state );
        }


    return( valid_state );
}



/*****************************************************************
 * TAG( parse_rect )
 *
 * parse and validate command "rect" parameters
 */
int
parse_rect( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt )
{
    int
        validity_status;


    /* */

        if ( 11 == token_cnt )
        {
            surface_parameter_table.n = (int)is_valid_number( tokens[2], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert n." );
                return( invalid_state );
            }

            if ( 0 > surface_parameter_table.n )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  n MUST be >= 0" );
                return( invalid_state );
            }

            surface_parameter_table.n = MIN( surface_parameter_table.n, MAXIMUM_SURFACE_POINTS );


            surface_parameter_table.p.x = is_valid_number( tokens[3], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert px." );
                return( invalid_state );
            }

            surface_parameter_table.p.y = is_valid_number( tokens[4], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert py." );
                return( invalid_state );
            }

            surface_parameter_table.p.z = is_valid_number( tokens[5], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert pz." );
                return( invalid_state );
            }

            surface_parameter_table.v.x = is_valid_number( tokens[6], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert vx." );
                return( invalid_state );
            }

            surface_parameter_table.v.y = is_valid_number( tokens[7], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert vy." );
                return( invalid_state );
            }

            surface_parameter_table.v.z = is_valid_number( tokens[8], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert vz." );
                return( invalid_state );
            }

            surface_parameter_table.a = is_valid_number( tokens[9], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert a." );
                return( invalid_state );
            }

            if ( 0.0 >= surface_parameter_table.a )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  a MUST be > 0" );
                return( invalid_state );
            }

            surface_parameter_table.b = is_valid_number( tokens[10], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  Unable to convert b." );
                return( invalid_state );
            }

            if ( 0.0 >= surface_parameter_table.b )
            {
                popup_dialog( WARNING_POPUP, "parse_rect:  b MUST be > 0" );
                return( invalid_state );
            }
        }
        else
        {
            popup_dialog( USAGE_POPUP, "rect <n> <px> <py> <pz> <vx> <vy> <vz> <a> <b>" );
            return( invalid_state );
        }


    return( valid_state );
}



/*****************************************************************
 * TAG( parse_ring )
 *
 * parse and validate command "ring" parameters
 */
int
parse_ring( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt )
{
    int
        validity_status;


    /* */

        if ( 11 == token_cnt )
        {
            griz_str_dup( &surface_parameter_table.type, tokens[1] );

            surface_parameter_table.n = (int)is_valid_number( tokens[2], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert n." );
                return( invalid_state );
            }

            if ( surface_parameter_table.n < 0 )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  n MUST be >= 0" );
                return( invalid_state );
            }

            surface_parameter_table.n = MIN( surface_parameter_table.n, MAXIMUM_SURFACE_POINTS );


            surface_parameter_table.p.x = is_valid_number( tokens[3], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert px." );
                return( invalid_state );
            }

            surface_parameter_table.p.y = is_valid_number( tokens[4], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert py." );
                return( invalid_state );
            }

            surface_parameter_table.p.z = is_valid_number( tokens[5], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert pz." );
                return( invalid_state );
            }

            surface_parameter_table.v.x = is_valid_number( tokens[6], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert vx." );
                return( invalid_state );
            }

            surface_parameter_table.v.y = is_valid_number( tokens[7], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert vy." );
                return( invalid_state );
            }

            surface_parameter_table.v.z = is_valid_number( tokens[8], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert vz." );
                return( invalid_state );
            }

            surface_parameter_table.delta_a = is_valid_number( tokens[9], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert delta_a." );
                return( invalid_state );
            }

            if ( 0.0 >= surface_parameter_table.delta_a )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  delta_a MUST be > 0" );
                return( invalid_state );
            }


            surface_parameter_table.delta_b = is_valid_number( tokens[10], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  Unable to convert delta_b." );
                return( invalid_state );
            }

            if ( 0.0 >= surface_parameter_table.delta_b )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  delta_b MUST be > 0" );
                return( invalid_state );
            }

            if ( surface_parameter_table.delta_b <= surface_parameter_table.delta_a )
            {
                popup_dialog( WARNING_POPUP, "parse_ring:  delta_b MUST be > delta_a." );
                return( invalid_state );
            }
        }
        else
        {
            popup_dialog( USAGE_POPUP, "ring <n> <px> <py> <pz> <vx> <vy> <vz> <delta_a> <delta_b>" );
            return( invalid_state );
        }


    return( valid_state );
}



/*****************************************************************
 * TAG( parse_spot )
 *
 * parse and validate command "spot" parameters
 */
int
parse_spot( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt )
{
    int
        validity_status;


    /* */

        if ( 10 == token_cnt )
        {
            griz_str_dup( &surface_parameter_table.type, tokens[1] );

            surface_parameter_table.n = (int)is_valid_number( tokens[2], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  Unable to convert n." );
                return( invalid_state );
            }

            if ( 0 > surface_parameter_table.n )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  n MUST be >= 0" );
                return( invalid_state );
            }

            surface_parameter_table.n = MIN( surface_parameter_table.n, MAXIMUM_SURFACE_POINTS );


            surface_parameter_table.p.x = is_valid_number( tokens[3], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  Unable to convert px." );
                return( invalid_state );
            }

            surface_parameter_table.p.y = is_valid_number( tokens[4], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  Unable to convert py." );
                return( invalid_state );
            }

            surface_parameter_table.p.z = is_valid_number( tokens[5], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  Unable to convert pz." );
                return( invalid_state );
            }

            surface_parameter_table.v.x = is_valid_number( tokens[6], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  Unable to convert vx." );
                return( invalid_state );
            }

            surface_parameter_table.v.y = is_valid_number( tokens[7], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  Unable to convert vy." );
                return( invalid_state );
            }

            surface_parameter_table.v.z = is_valid_number( tokens[8], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  Unable to convert vz." );
                return( invalid_state );
            }

            surface_parameter_table.delta_a = is_valid_number( tokens[9], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  Unable to convert delta." );
                return( invalid_state );
            }

            if ( 0.0 >= surface_parameter_table.delta_a )
            {
                popup_dialog( WARNING_POPUP, "parse_spot:  delta MUST be > 0" );
                return( invalid_state );
            }
        }
        else
        {
            popup_dialog( USAGE_POPUP, "spot <n> <px> <py> <pz> <vx> <vy> <vz> <delta>" );
            return( invalid_state );
        }


    return( valid_state );
}



/*****************************************************************
 * TAG( parse_tube )
 *
 * parse and validate command "tube" parameters
 */
int
parse_tube( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt )
{
    int
        validity_status;


    /* */

        if ( (11 == token_cnt)  ||  (12 == token_cnt) )
        {
            griz_str_dup( &surface_parameter_table.type, tokens[1] );

            surface_parameter_table.n = (int)is_valid_number( tokens[2], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert n." );
                return( invalid_state );
            }

            if ( 0 > surface_parameter_table.n )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  n MUST be >= 0" );
                return( invalid_state );
            }

            surface_parameter_table.n = MIN( surface_parameter_table.n, MAXIMUM_SURFACE_POINTS );


            surface_parameter_table.p.x = is_valid_number( tokens[3], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert px." );
                return( invalid_state );
            }

            surface_parameter_table.p.y = is_valid_number( tokens[4], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert py." );
                return( invalid_state );
            }

            surface_parameter_table.p.z = is_valid_number( tokens[5], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert pz." );
                return( invalid_state );
            }

            surface_parameter_table.v.x = is_valid_number( tokens[6], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert vx." );
                return( invalid_state );
            }

            surface_parameter_table.v.y = is_valid_number( tokens[7], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert vy." );
                return( invalid_state );
            }

            surface_parameter_table.v.z = is_valid_number( tokens[8], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert vz." );
                return( invalid_state );
            }

            surface_parameter_table.delta_a = is_valid_number( tokens[9], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert delta." );
                return( invalid_state );
            }

            if ( 0.0 >= surface_parameter_table.delta_a )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  delta_a MUST be > 0" );
                return( invalid_state );
            }

            surface_parameter_table.h = is_valid_number( tokens[10], &validity_status );

            if ( invalid_state == validity_status )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert delta." );
                return( invalid_state );
            }

            if ( 0.0 >= surface_parameter_table.h )
            {
                popup_dialog( WARNING_POPUP, "parse_tube:  h MUST be > 0" );
                return( invalid_state );
            }


            if ( 12 == token_cnt )
            {
                surface_parameter_table.delta_b = is_valid_number( tokens[11], &validity_status );

                if ( invalid_state == validity_status )
                {
                    popup_dialog( WARNING_POPUP, "parse_tube:  Unable to convert delta." );
                    return( invalid_state );
                }

                /*
                 * NOTE:  delta_b MAY BE 0.0 in order to define a cone
                 */

                if ( 0.0 > surface_parameter_table.delta_b )
                {
                    popup_dialog( WARNING_POPUP, "parse_tube:  delta_b MUST be >= 0" );
                    return( invalid_state );
                }
            }
            else
                surface_parameter_table.delta_b = surface_parameter_table.delta_a;
        }
        else
        {
            popup_dialog( USAGE_POPUP, "tube <n> <px> <py> <pz> <vx> <vy> <vz> <delta_a> <h> {<delta_b>}" );
            return( invalid_state );
        }


    return( valid_state );
}



/*****************************************************************
 * TAG( poly )
 *
 * [Place description here.]
 *
 */
void
poly()
{
    int
        i;


    /* */

    qty_tdt = surface_parameter_table.n;


    /*
     * establishment of u0, u1, and u2 vectors for use in "poly"
     */

    u0.x = 0.0;
    u0.y = 1.0;
    u0.z = 0.0;

    u1.x = 0.0;
    u1.y = 1.0;
    u1.z = 0.0;

    u2.x = 0.0;
    u2.y = 0.0;
    u2.z = 1.0;



    /*
     * Compute traction data tables
     */

    surface_area = 0.0;

    for ( i = 0; i < qty_tdt; i++ )
        surface_area += traction_data_table[ i ].area;


    surface_command_processed = TRUE;


    return;
}



/*****************************************************************
 * TAG( rect )
 *
 * [Place description here.]
 */
void
rect()
{
    Vector
           p
          ,v;

    float
            a
         ,  b
         ,  dA
         ,  dx
         ,  x_finish
         ,  x_start
         ,  x_offset
         ,  x_term
         ,  x
         ,  xl
         ,  xr
         ,**xyz_table
         ,  y_finish
         ,  y_start
         ,  y_offset
         ,  y_term
         ,  y
         ,  yb
         ,  yt
         ,  z;

    int
         factor
        ,i
        ,j
        ,k
        ,qty_a_divisions
        ,qty_b_divisions;


    enum {
          xyz_columns = 3
         };

    enum {
          x_coordinate
         ,y_coordinate
         ,z_coordinate
         };


    /* */

    /*
     * Establishment of local parameters
     */

    n = surface_parameter_table.n;

    p = surface_parameter_table.p;
    v = surface_parameter_table.v;

    a = surface_parameter_table.a;
    b = surface_parameter_table.b;


    /*
     * Compute quantity of rectangles in "x" direction
     * and "y" direction such that an aspect ratio of 1:1 is maintained
     */

    dx = (float)MIN( a, b ) / (float)n;


    qty_a_divisions = ROUND( a / dx );
    qty_b_divisions = ROUND( b / dx );


    qty_tdt = qty_a_divisions * qty_b_divisions;

    if ( NULL != (traction_data_table = NEW_N( Traction_data, qty_tdt, "traction_data_table" )) )
    {
        traction_data_table_initialized = TRUE;
    }
    else
    {
        traction_data_table_initialized = FALSE;
        return;
    }


    compute_u0_u1_and_u2_vectors( v, &u0, &u1, &u2 );



    /*
     * Compute traction_data_table data
     */

    /*
     * Preliminary operations:
     *       dA
     *       x-y coordinates of rectangle centered about origin
     */

    factor = MAX( ROUND( a / b ), ROUND( b / a) );

    dA = (a * b) / (float)( factor * SQR( n ) );


    /*
     * Center rectangle about origin
     */

    xl = -a / 2.0;
    yb = -b / 2.0;
    xr =  a / 2.0;
    yt =  b / 2.0;


    /*
     * Compute coordinates of subdivision center points
     */

    if ( NULL == (xyz_table = NEW_N( float *, qty_tdt, "xyz_table" )) )
    {
        popup_dialog( WARNING_POPUP, "rect():  storage allocation fail." );
        return;
    }

    for ( i = 0; i < qty_tdt; i++ )
    {
        if ( NULL == (xyz_table[ i ] = NEW_N( float, xyz_columns, "xyz_table row" )) )
        {
            popup_dialog( WARNING_POPUP, "rect():  storage allocation fail." );
            return;
        }
    }


    x_offset = a / ((float)qty_a_divisions * 2.0);

    x_start  = xl + x_offset;
    x_finish = xr - x_offset;


    y_offset = b / ((float)qty_b_divisions * 2.0);

    y_start  = yb + y_offset;
    y_finish = yt - y_offset;


    if ( 1 != qty_a_divisions )
        x_term = (x_finish - x_start) / (float)(qty_a_divisions - 1);
    else
        x_term = 0.0;

    if ( 1 != qty_b_divisions )
        y_term = (y_finish - y_start) / (float)(qty_b_divisions - 1);
    else
        y_term = 0.0;


    k = 0;

    for ( i = 0; i < qty_a_divisions; i++ )
    {
        x = x_start + ((float)i * x_term);

        for ( j = 0; j < qty_b_divisions; j++ )
        {
            y = y_start + ((float)j * y_term);

            xyz_table[ k ][ x_coordinate ] = x;
            xyz_table[ k ][ y_coordinate ] = y;
            xyz_table[ k ][ z_coordinate ] = 0.0;

            k++;
        } /* for j */
    } /* for i */


    /*
     * Compute traction data tables
     */

    surface_area = 0.0;

    for ( i = 0; i < qty_tdt; i++ )
    {
        x = xyz_table[ i ][ x_coordinate ];
        y = xyz_table[ i ][ y_coordinate ];
        z = xyz_table[ i ][ z_coordinate ];

        traction_data_table[ i ].area    = dA;
        traction_data_table[ i ].point.x = p.x + (u1.x * x) + (u2.x * y) + (u0.x * z);
        traction_data_table[ i ].point.y = p.y + (u1.y * x) + (u2.y * y) + (u0.y * z);
        traction_data_table[ i ].point.z = p.z + (u1.z * x) + (u2.z * y) + (u0.z * z);
        traction_data_table[ i ].normal  = u0;

        surface_area += dA;
    }


    for ( i = 1; i < qty_tdt; i++ )
        free( xyz_table[ i ] );

    free( xyz_table[ 0 ] );


    surface_command_processed = TRUE;


    return;
}



/*****************************************************************
 * TAG( ring )
 *
 * [Place description here.]
 */
void
ring()
{
    Vector
           p
          ,v;

    double
           theta_p;

    float
            dA
         ,  delta_a
         ,  delta_b
         ,  delta_delta
         ,  r
         ,  r_prior
         ,**r_table
         ,  term
         ,  x
         ,  y
         ,  z;

    int
        i
       ,k
       ,l
       ,m
       ,n
       ,qty_points_per_ring
       ,r_columns
       ,r_rows;

    enum {
          minimum_points = 6
         };


    /* */

    n       = surface_parameter_table.n;

    p       = surface_parameter_table.p;
    v       = surface_parameter_table.v;

    delta_a = surface_parameter_table.delta_a;
    delta_b = surface_parameter_table.delta_b;



    /*
     * Compute traction_data_table data
     */

    /*
     * Preliminary operations:
     *
     *       radius_table
     *       total quantity of points
     *       normalize u vectors
     */

    r_rows    = n + 1;
    r_columns = 2;

    if ( NULL == (r_table = NEW_N( float *, r_rows, "r_table" )) )
    {
        popup_dialog( WARNING_POPUP, "ring():  storage allocation fail." );
        return;
    }

    for ( i = 0; i < r_rows; i++ )
    {
        if ( NULL == (r_table[ i ] = NEW_N( float, r_columns, "r_table row" )) )
        {
            popup_dialog( WARNING_POPUP, "ring():  storage allocation fail." );
            return;
        }
    }

    if ( 2 <= n )
    {
        r                       = delta_a;
        r_table[ 0 ][ squared ] = SQR( r );
        r_table[ 0 ][ cubed   ] = CUBE( r );


        delta_delta = (delta_b - delta_a) / (float)n;

        for ( m = 1; m < n; m++ )
        {
            r_prior                 = r;

            r                       = r_prior + delta_delta;
            r_table[ m ][ squared ] = SQR( r );
            r_table[ m ][ cubed   ] = CUBE( r );
        }

        r                       = delta_b;
        r_table[ m ][ squared ] = SQR( r );
        r_table[ m ][ cubed   ] = CUBE( r );
    }



    term    = TWO_PI / delta_delta;

    qty_tdt = 0;

    for ( m = 0; m < n; m++ )
    {
        r = TWO_THIRDS * ( (r_table[ m + 1 ][ cubed ] - r_table[ m ][ cubed ]) / (r_table[ m + 1 ][ squared ] - r_table[ m ][ squared ]) );

        qty_tdt += MAX( minimum_points, ROUND( r * term ) );
    }

    if ( NULL != (traction_data_table = NEW_N( Traction_data, qty_tdt, "traction_data_table" )) )
    {
        traction_data_table_initialized = TRUE;
    }
    else
    {
        traction_data_table_initialized = FALSE;
        return;
    }


    compute_u0_u1_and_u2_vectors( v, &u0, &u1, &u2 );



    /*
     * Compute traction data tables
     */

    surface_area = 0.0;

    if ( 2 <= n )
    {
        k = 0;

        for ( m = 0; m < n; m++ )
        {
            r = TWO_THIRDS * ( (r_table[ m + 1 ][ cubed ] - r_table[ m ][ cubed ]) / (r_table[ m + 1 ][ squared ] - r_table[ m ][ squared ]) );

            qty_points_per_ring = MAX( minimum_points, ROUND( r * term ) );

            dA = ( PI * (r_table[ m + 1 ][ squared ] - r_table[ m ][ squared ]) ) / qty_points_per_ring;

            for ( l = 0; l < qty_points_per_ring; l++ )
            {
                theta_p = (TWO_PI * (double)l) / (double)qty_points_per_ring;

                x = r * cos( theta_p );
                y = r * sin( theta_p );
                z = 0.0;

                traction_data_table[ k ].area    = dA;
                traction_data_table[ k ].point.x = p.x + (u1.x * x) + (u2.x * y) + (u0.x * z);
                traction_data_table[ k ].point.y = p.y + (u1.y * x) + (u2.y * y) + (u0.y * z);
                traction_data_table[ k ].point.z = p.z + (u1.z * x) + (u2.z * y) + (u0.z * z);
                traction_data_table[ k ].normal  = u0;

                surface_area += dA;

                k++;
            }
        }
    }


    for ( i = 1; i < r_rows; i++ )
        free( r_table[ i ] );

    free( r_table[ 0 ] );


    surface_command_processed = TRUE;


    return;
}



/*****************************************************************
 * TAG( spot )
 *
 * [Place description here.]
 */
void
spot()
{
    Vector
           p
          ,v;

    double
           denominator
          ,theta_p;

    float
            d_theta
         ,  dA
         ,  delta
         ,  r
         ,  rp
         ,**r_table
         ,  x
         ,  y
         ,  z;

    int
        i
       ,k
       ,l
       ,m
       ,n
       ,nl
       ,r_columns
       ,r_rows;


    /* */

    n     = surface_parameter_table.n;

    p     = surface_parameter_table.p;
    v     = surface_parameter_table.v;

    delta = surface_parameter_table.delta_a;


    qty_tdt = ( (3 * SQR( n )) - (3 * n) + 2 ) / 2;

    if ( NULL != (traction_data_table = NEW_N( Traction_data, qty_tdt, "traction_data_table" )) )
    {
        traction_data_table_initialized = TRUE;
    }
    else
    {
        traction_data_table_initialized = FALSE;
        return;
    }


    compute_u0_u1_and_u2_vectors( v, &u0, &u1, &u2 );



    /*
     * Compute traction_data_table data
     */

    /*
     * Preliminary operations:
     *       dA
     *       radius_table
     */

    dA = (PI * SQR( delta )) / (float)qty_tdt;


    r_rows    = n;
    r_columns = 2;

    if ( NULL == (r_table = NEW_N( float *, r_rows, "r_table" )) )
    {
        popup_dialog( WARNING_POPUP, "spot():  storage allocation fail." );
        return;
    }

    for ( i = 0; i < r_rows; i++ )
    {
        if ( NULL == (r_table[ i ] = NEW_N( float, r_columns, "r_table row" )) )
        {
            popup_dialog( WARNING_POPUP, "spot():  storage allocation fail." );
            return;
        }
    }

    if ( 2 <= n )
    {
        denominator = (double)qty_tdt * 2.0;

        for ( m = 0; m < n - 1; m++ )
        {
            r                       = sqrt( ( (3.0 * SQR( m + 1 )) - (3.0 * (double)(m + 1)) + 2.0 ) / denominator ) * delta;
            r_table[ m ][ squared ] = SQR( r );
            r_table[ m ][ cubed   ] = CUBE( r );
        }

        r                       = delta;
        r_table[ m ][ squared ] = SQR( r );
        r_table[ m ][ cubed   ] = CUBE( r );
    }

    /*
     * Compute traction data tables
     */

    traction_data_table[ 0 ].area   = dA;
    traction_data_table[ 0 ].point  = p;
    traction_data_table[ 0 ].normal = u0;


    surface_area = 0.0;

    if ( 2 <= n )
    {
        k = 1;

        for ( m = 0; m < n - 1; m++ )
        {
            rp = TWO_THIRDS * ( (r_table[ m + 1 ][ cubed ] - r_table[ m ][ cubed ]) / (r_table[ m + 1 ][ squared ] - r_table[ m ][ squared ]) );

            nl = 3 * (m + 1);

            d_theta = TWO_PI / (float)nl;

            for ( l = 0; l < nl; l++ )
            {
                theta_p = (double)(l * d_theta);

                x = rp * cos( theta_p );
                y = rp * sin( theta_p );
                z = 0.0;

                traction_data_table[ k ].area    = dA;
                traction_data_table[ k ].point.x = p.x + (u1.x * x) + (u2.x * y) + (u0.x * z);
                traction_data_table[ k ].point.y = p.y + (u1.y * x) + (u2.y * y) + (u0.y * z);
                traction_data_table[ k ].point.z = p.z + (u1.z * x) + (u2.z * y) + (u0.z * z);
                traction_data_table[ k ].normal  = u0;

                surface_area += dA;

                k++;
            }
        }
    }


    for ( i = 1; i < r_rows; i++ )
        free( r_table[ i ] );

    free( r_table[ 0 ] );


    surface_command_processed = TRUE;


    return;
}



/*****************************************************************
 * TAG( tube )
 *
 * [Place description here.]
 */
void
tube()
{
    Vector
           p
          ,v;

    double
           gamma
          ,theta;

    float
          dA
         ,cosine_theta
         ,d_theta
         ,delta_a
         ,delta_b
         ,h
         ,l_magnitude
         ,lx
         ,ly
         ,lz
         ,r
         ,sine_theta
         ,term_1
         ,term_2
         ,term_3
         ,term_4
         ,term_5
         ,x
         ,y
         ,z;

    int
        i
       ,j
       ,k
       ,n
       ,nr;

    enum {
          minimum_points = 6
         };


    /* */

    n       = surface_parameter_table.n;

    p       = surface_parameter_table.p;
    v       = surface_parameter_table.v;

    h       = surface_parameter_table.h;
    delta_a = surface_parameter_table.delta_a;
    delta_b = surface_parameter_table.delta_b;



    /*
     * Compute traction_data_table data
     */

    /*
     * Preliminary operations:
     *
     *       total quantity of points
     *       normalize u and v vectors
     */

    term_1 = h / ( n * 2.0 );
    term_2 = h / n;
    term_3 = ( delta_a - delta_b ) / h;
    term_4 = ( TWO_PI * n ) / h;
    term_5 = TWO_PI * h;

    qty_tdt = 0;

    for ( i = 0; i < n; i++ )
    {
        z = term_1 + ( (float)i * term_2 );

        r = delta_b + (( h - z ) * term_3);

        qty_tdt += MAX( minimum_points, ROUND( r * term_4 ) );
    }


    if ( NULL != (traction_data_table = NEW_N( Traction_data, qty_tdt, "traction_data_table" )) )
    {
        traction_data_table_initialized = TRUE;
    }
    else
    {
        traction_data_table_initialized = FALSE;
        return;
    }


    compute_u0_u1_and_u2_vectors( v, &u0, &u1, &u2 );



    /*
     * Compute traction data tables
     */

    surface_area = 0.0;

    gamma       = atan( (double)term_3 );
    l_magnitude = sqrt( 1.0 + SQR( sin( gamma ) ) );
    lz          = sin( gamma ) / l_magnitude;

    k = 0;

    for ( i = 0; i < n; i++ )
    {
        z = term_1 + ((float)i * term_2);

        r = delta_b + ((h - z) * term_3);

        nr = MAX( minimum_points, ROUND( r * term_4 ) );

        d_theta = TWO_PI / (float)nr;

        dA = (r * term_5) / (float)(n * nr);

        for ( j = 0; j < nr; j++ )
        {
            theta = (float)j * d_theta;

            cosine_theta = cos( theta );
            sine_theta   = sin( theta );

            x = r * cosine_theta;
            y = r * sine_theta;

            lx = cosine_theta / l_magnitude;
            ly = sine_theta / l_magnitude;


            traction_data_table[ k ].area     = dA;
            traction_data_table[ k ].point.x  = p.x + (u1.x * x) + (u2.x * y) + (u0.x * z);
            traction_data_table[ k ].point.y  = p.y + (u1.y * x) + (u2.y * y) + (u0.y * z);
            traction_data_table[ k ].point.z  = p.z + (u1.z * x) + (u2.z * y) + (u0.z * z);
            traction_data_table[ k ].normal.x = (u1.x * lx) + (u2.x * ly) + (u0.x * lz);
            traction_data_table[ k ].normal.y = (u1.y * lx) + (u2.y * ly) + (u0.y * lz);
            traction_data_table[ k ].normal.z = (u1.z * lx) + (u2.z * ly) + (u0.z * lz);

            surface_area += dA;

            k++;
        } /* for j */
    } /* for i */


    surface_command_processed = TRUE;


    return;
}



/*****************************************************************
 * TAG( auto_compute_n )
 *
 * "Automatically" compute number of requested points in surface geometry commands
 *
 * NOTE:  "poly" does NOT provide for an auto_compute_n mode
 */
int
auto_compute_n( Analysis *analy )
{
    Bool_type
              convergence_achieved = FALSE;

    List_head
              *p_lh;

    MO_class_data
                  **p_classes;

    static p_surface_function
                              surface_target;

    float
          convergence_term
         ,convergence_tolerance            =  1.0
         ,prior_total_traction_area
         ,total_traction_area;

    int
        i
       ,total_traction_materials;

    enum {
          minimum_n          = 40
         ,increment_n        = 10
         ,qty_tdt_limit = 1000000
         };


    /* */

    surface_parameter_table.n = minimum_n;

    /*
     * get all materials and place into bit array of traction materials
     */

    p_lh = MESH_P( analy )->classes_by_sclass;
    p_classes = (MO_class_data **)p_lh[G_MAT].list;

    total_traction_materials = p_classes[0]->qty;

    if ( NULL == (traction_material_list = NEW_N( char, total_traction_materials, "traction_material_list" )) )
    {
        popup_dialog( WARNING_POPUP, "auto_compute_n:  storage allocation failure." );
        return( invalid_state );
    }

    /*
     * NOTE:  traction_material_list contains "real-world" material numbers
     */

    for ( i = 1; i <= total_traction_materials; i++ )
        BITSET( traction_material_list, i );


    if ( NULL != (surface_target = surface_function_lookup( surface_parameter_table.type )) )
        surface_target();
    else
    {
        popup_dialog( USAGE_POPUP, "auto_compute_n:  Invalid surface type:  %s\n", surface_parameter_table.type );

        free( traction_material_list );
        return( invalid_state );
    }


    wrt_text( "\n\n" );
    wrt_text( "Beginning automatic computation of surface 'n' convergence . . .\n" );
    XFlush( dpy_copy );


    total_traction_area = traction_area( analy, traction_material_list );

    while ( FALSE == convergence_achieved )
    {
        prior_total_traction_area = total_traction_area;


        surface_parameter_table.n += increment_n;

        wrt_text( "   n:  %d\n", surface_parameter_table.n );
        XFlush( dpy_copy );

        surface_target();

        total_traction_area = traction_area( analy, traction_material_list );


        /*
         * DEBUGGING ONLY -- KEEP THIS STATEMENT
         *
        surface_area_coverage_efficiency = (total_traction_area / surface_area ) * 100.0;
         */


        convergence_term = (200.0 * fabs( total_traction_area - prior_total_traction_area )) /
                           fabs( total_traction_area + prior_total_traction_area );

        if ( (convergence_term  <=  convergence_tolerance)  ||  qty_tdt > qty_tdt_limit )
            convergence_achieved = TRUE;
    }

    wrt_text( ". . . convergence complete\n" );
    XFlush( dpy_copy );


    wrt_text( "\n\n" );
    wrt_text( "Number of points:  %d\n", qty_tdt );
    wrt_text( "surface area:  %f\n", surface_area );
    wrt_text( "p  ==>  %f, %f, %f\n",  surface_parameter_table.p.x,  surface_parameter_table.p.y,  surface_parameter_table.p.z );
    wrt_text( "u0 ==>  %f, %f, %f\n", u0.x, u0.y, u0.z );
    wrt_text( "u1 ==>  %f, %f, %f\n", u1.x, u1.y, u1.z );
    wrt_text( "u2 ==>  %f, %f, %f\n", u2.x, u2.y, u2.z );

    /*
     * DEBUGGING ONLY -- KEEP THESE STATEMENTS
     *
    wrt_text( "\n\n" );
    wrt_text( "Surface area efficiency:  %f\n", surface_area_coverage_efficiency );
     */


    free( traction_material_list );


    return( valid_state );
}



/*****************************************************************
 * TAG( compute_u0_u1_and_u2_vectors )
 *
 */
void
compute_u0_u1_and_u2_vectors( Vector v, Vector *u0, Vector *u1, Vector *u2 )
{
    Vector
           fabs_u0;

    float
          u_magnitude;


    /* */

    u0->x = v.x;
    u0->y = v.y;
    u0->z = v.z;

    normalize_vector( u0 );


    fabs_u0.x = fabs( (double)u0->x );
    fabs_u0.y = fabs( (double)u0->y );
    fabs_u0.z = fabs( (double)u0->z );

    if ( (fabs_u0.x >= fabs_u0.y)  &&  (fabs_u0.x >= fabs_u0.z) )
    {
        u1->x = 0.0;

        if ( APX_EQ( u0->z, 0.0 ) )
        {
            u1->y = 0.0;
            u1->z = 1.0;
        }
        else
        {
            u_magnitude = sqrt( 1.0 + SQR( u0->y / u0->z ) );

            u1->y = 1.0 / u_magnitude;
            u1->z = -(u0->y / u0->z) / u_magnitude;

        }
    }

    if ( (fabs_u0.y >= fabs_u0.z)  &&  (fabs_u0.y > fabs_u0.x) )
    {
        u1->y = 0.0;

        if ( APX_EQ( u0->z, 0.0 ) )
        {
            u1->x = 0.0;
            u1->z = 1.0;
        }
        else
        {
            u_magnitude = sqrt( 1.0 + SQR( u0->x / u0->z ) );

            u1->x = 1.0 / u_magnitude;
            u1->z = -(u0->x / u0->z) / u_magnitude;
        }
    }

    if ( (fabs_u0.z > fabs_u0.x)  &&  (fabs_u0.z > fabs_u0.y) )
    {
        u1->z = 0.0;

        if ( APX_EQ( u0->y, 0.0 ) )
        {
            u1->x = 0.0;
            u1->y = 1.0;
        }
        else
        {
            u_magnitude = sqrt( 1.0 + SQR( u0->x / u0->y ) );

            u1->x = 1.0 / u_magnitude;
            u1->y = -(u0->x / u0->y) / u_magnitude;
        }
    }


    u2->x = (u0->y * u1->z) - (u0->z * u1->y);
    u2->y = (u0->z * u1->x) - (u0->x * u1->z);
    u2->z = (u0->x * u1->y) - (u0->y * u1->x);


    return;
}



/*****************************************************************
 * TAG( surface_function_lookup )
 *
 */
p_surface_function
surface_function_lookup( char *name )
{
    const Surface_function_table
                                 *result;


    /* */

    result = bsearch(
                     name
                    ,surface_function_table
                    ,sizeof( surface_function_table ) / sizeof( surface_function_table[ 0 ] )
                    ,sizeof( surface_function_table[ 0 ] )
                    ,sft_compare
                    );


    return( result == NULL  ?  NULL : result->function );
}



/*****************************************************************
 * TAG( initialize_surface_parameter_table )
 *
 */
void
initialize_surface_parameter_table()
{
    /* */

    surface_parameter_table.p.x     = 0.0;
    surface_parameter_table.p.y     = 0.0;
    surface_parameter_table.p.z     = 0.0;

    surface_parameter_table.v.x     = 0.0;
    surface_parameter_table.v.y     = 0.0;
    surface_parameter_table.v.z     = 0.0;

    surface_parameter_table.a       = 0.0;
    surface_parameter_table.b       = 0.0;
    surface_parameter_table.delta_a = 0.0;
    surface_parameter_table.delta_b = 0.0;
    surface_parameter_table.h       = 0.0;

    surface_parameter_table.n       = 0;

    surface_parameter_table.type    = NULL;


    return;
}



/*****************************************************************
 * TAG( is_valid_number )
 *
 * test to determine if a string represents a valid number:
 *
 * procedure attempts to convert string into double precision numerical representation
 *
 * NOTE:  ALWAYS test conversion "validity_status"
 */


float is_valid_number( const char *string, int *validity_status )
{
    char
         *p_tail;

    double
           result;


    /* */

    errno = 0;

    result = strtod( string, &p_tail );


    if ( (0 == errno)  &&  (0 == strcmp( p_tail, "" )) )
    {
        *validity_status = valid_state;
    }
    else
    {
        *validity_status = invalid_state;
    }


    return( result );
}



/*****************************************************************
 * TAG( normalize_vector )
 *
 * normalize the specified vector
 */
void normalize_vector( Vector *a )
{
    float
          magnitude;


    /* */


    magnitude = sqrt( SQR( a->x ) + SQR( a->y ) + SQR( a->z ) );

    a->x /= magnitude;
    a->y /= magnitude;
    a->z /= magnitude;


    return;
}



/*****************************************************************
 * TAG( sft_compare )
 *
 * comparison procedure for use in binary search routine to determine
 * validity/presence of surface function name/type within surface_function_table
 */

int
sft_compare( const void *key, const void *sft )
{
    return( strcmp( (const char *)key, ((const Surface_function_table *)sft)->name ) );
}



/*****************************************************************
 * TAG( traction )
 *
 * Description goes here!!
 */
void
traction( Analysis *analy, char *traction_material_list )
{
    Bool_type
              valid_elements_found = FALSE;

    Htable_entry
                 *p_hash_table_entry;

    MO_class_data
                  *p_mo_class
                 ,*p_class;

    Subrec_obj
               *p_subrecord;

    Vector
           f
          ,f_hat
          ,m
          ,mp
          ,mp_hat
          ,n
          ,p;

    Vector_pt_obj
                  *point_data_list
                 ,*point_data_node;
    
    char
         class[]                        = "brick"
        ,primal_specification__stress[] = "stress"
        ,primal_specification__sxx[]    = "stress[sx]"
        ,primal_specification__syy[]    = "stress[sy]"
        ,primal_specification__szz[]    = "stress[sz]"
        ,primal_specification__sxy[]    = "stress[sxy]"
        ,primal_specification__syz[]    = "stress[syz]"
        ,primal_specification__szx[]    = "stress[szx]"
        ,sign_term_2[2]
        ,sign_term_3[2];

    char
         *primal_values[7];

    float
          area = 0.0
         ,dA
         ,fo_magnitude
         ,fp_magnitude
         ,fu0
         ,fu1
         ,fu2
         ,h[8]
         ,mo_magnitude
         ,mp_magnitude
         ,mu0
         ,mu1
         ,mu2
         ,r
         ,s
         ,t
         ,shape_function
         ,sigma__xx
         ,sigma__yy
         ,sigma__zz
         ,sigma__xy
         ,sigma__yz
         ,sigma__zx
         ,sigma__yx
         ,sigma__xz
         ,sigma__zy
         ,x
         ,y
         ,z;

    float
          **nodal_stress_data
         , *p_sigmas__Form_II
         ,(*sigmas__Form_I)[6]
         , *sigmas__Form_II;

    int
        connectivity_qty
       ,element_idx
       ,i
       ,j
       ,node_idx
       ,subrecord_index
       ,total_brick_elements
       ,total_nodes;

    int
        *element_nodal_connectivities
       ,*materials;

    static Result
                  new_result;


    enum {
           r_coordinate
          ,s_coordinate
          ,t_coordinate
         };

    enum {
           x_coordinate
          ,y_coordinate
          ,z_coordinate
         };

    enum {
           sxx
          ,syy
          ,szz
          ,sxy
          ,syz
          ,szx
         };

    enum {
          qty_stress_types = 6
         };


    /* */

    p = surface_parameter_table.p;



    /*
     * Initialization:
     *
     * establish total elements and total nodes in entire mesh
     */

    /*
     * see "tell_coordinates"
     */

    if ( OK != htable_search( MESH( analy ).class_table, class, FIND_ENTRY, &p_hash_table_entry ) )
    {
        popup_dialog( USAGE_POPUP, "traction:  invalid class name" );
        return;
    }
    
    p_mo_class = (MO_class_data *) p_hash_table_entry->data;

    total_brick_elements = p_mo_class->qty;

    total_nodes = MESH_P( analy )->node_geom->qty;



    /*
     * "init_vec_points_hex" will determine the identification of the element containing each point
     * and the <rst> coordinates of the point within the element
     *
     * if a point does NOT lie within an element, the element identification number is "null_element"
     *
     * NOTE:  all element numbers returned in point_data_list->elnum are the actual, real-world
     *        element numbers
     */

    point_data_list = NULL;

    for ( i = 0; i < qty_tdt; i++ )
    {
        point_data_node = NEW( Vector_pt_obj, "point_data_node" );

        point_data_node->pt[ x_coordinate ] = traction_data_table[ i ].point.x;
        point_data_node->pt[ y_coordinate ] = traction_data_table[ i ].point.y;
        point_data_node->pt[ z_coordinate ] = traction_data_table[ i ].point.z;

        point_data_node->elnum              = null_element;

        APPEND( point_data_node, point_data_list );
    }


    for ( i = 0; i < analy->mesh_qty; i++ )
        init_vec_points_hex( &point_data_list, i, analy, FALSE );


    if ( NO_INTERP == analy->interp_mode )
    {
        /*
         * I.  Update traction data table information:
         *
         *     Assign valid element number;
         *
         *
         *     Remove nodes containing a point that does NOT lie within a brick element, i.e., element # == null_element,
         *          AND
         *     Remove nodes containing an element that is NOT in the traction material list
         */

        materials = p_mo_class->objects.elems->mat;

        for ( point_data_node = point_data_list, i = 0; point_data_node != NULL; i++ )
        {
            element_idx = point_data_node->elnum;

            /*
             * NOTE:  offset materials[ element_idx ] to convert to "real-world" material number
             */

            if ( (null_element != element_idx)  &&  (BITTEST( traction_material_list, materials[ element_idx ] + 1 )) )
            {
                traction_data_table[ i ].element = element_idx;

                valid_elements_found = TRUE;
            }
            else
            {
                /*
                 * "nullify" all points located in:
                 *
                 * a.) "space", or
                 * b.) elements whose material is NOT in the requested traction material list
                 *
                 * NOTE:  Updating traction data table materials and <rst> coordinate system
                 *        is NOT required for NULL elements.
                 */

                traction_data_table[ i ].element = null_element;
            }

            NEXT( point_data_node );
        }

        if ( FALSE == valid_elements_found )
        {
            popup_dialog( WARNING_POPUP, "traction:  No points lie within valid mesh elements.\n" );
            DELETE_LIST( point_data_list );

            return;
        }



        /*
         * II.  Collect 6 stress values for ALL brick elements of entire mesh
         *
         *      Stress values are returned/assigned in a data structure of the form:
         *
         *           element  sxx  syy  szz  sxy  syz  szx
         *              0      #    #    #    #    #    #
         *              1      #    #    #    #    #    #
         *                              ...
         *             n-1     #    #    #    #    #    #
         *
         *
         *      Stress values may be accessed:
         *
         *      sigmas__Form_I[ element_idx ][ sxx | syy | szz | sxy | syz | szx ]
         */

        find_result( analy, PRIMAL, TRUE, &new_result, "stress" );

        p_subrecord = analy->srec_tree[ new_result.srec_id ].subrecs;

        primal_values[ 0 ] = primal_specification__stress;
        primal_values[ 1 ] = NULL;

        for ( j = 0; j < new_result.qty; j++ )
        {
            subrecord_index = new_result.subrecs[ j ];

            p_class = p_subrecord[ subrecord_index ].p_object_class;

            if ( p_mo_class == p_class )
                break;
        }

        analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrecord_index, 1, primal_values, analy->tmp_result[ 0 ] );


        sigmas__Form_I = (float (*)[6])analy->tmp_result[ 0 ];



        /*
         * III.  Compute traction force vector components
         */

        f.x = 0.0;
        f.y = 0.0;
        f.z = 0.0;

        m.x = 0.0;
        m.y = 0.0;
        m.z = 0.0;

        mp.x = 0.0;
        mp.y = 0.0;
        mp.z = 0.0;

        for ( i = 0; i < qty_tdt; i++ )
        {
            if ( null_element != traction_data_table[ i ].element )
            {
                element_idx = traction_data_table[ i ].element;

                x = traction_data_table[ i ].point.x;
                y = traction_data_table[ i ].point.y;
                z = traction_data_table[ i ].point.z;

                dA = traction_data_table[ i ].area;

                n = traction_data_table[ i ].normal;

                sigma__xx = sigmas__Form_I[ element_idx ][ sxx ];
                sigma__yy = sigmas__Form_I[ element_idx ][ syy ];
                sigma__zz = sigmas__Form_I[ element_idx ][ szz ];
                sigma__xy = sigmas__Form_I[ element_idx ][ sxy ];
                sigma__yz = sigmas__Form_I[ element_idx ][ syz ];
                sigma__zx = sigmas__Form_I[ element_idx ][ szx ];

                sigma__yx = sigma__xy;
                sigma__xz = sigma__zx;
                sigma__zy = sigma__yz;


                area += dA;

                f_hat.x = ( (sigma__xx * n.x) + (sigma__yx * n.y) + (sigma__zx * n.z) ) * dA;
                f_hat.y = ( (sigma__xy * n.x) + (sigma__yy * n.y) + (sigma__zy * n.z) ) * dA;
                f_hat.z = ( (sigma__xz * n.x) + (sigma__yz * n.y) + (sigma__zz * n.z) ) * dA;


                f.x += f_hat.x;
                f.y += f_hat.y;
                f.z += f_hat.z;

                m.x += (y * f_hat.z) - (z * f_hat.y);
                m.y += (z * f_hat.x) - (x * f_hat.z);
                m.z += (x * f_hat.y) - (y * f_hat.x);


                mp_hat.x = ((y - p.y) * f_hat.z) - ((z - p.z) * f_hat.y);
                mp_hat.y = ((z - p.z) * f_hat.x) - ((x - p.x) * f_hat.z);
                mp_hat.z = ((x - p.x) * f_hat.y) - ((y - p.y) * f_hat.x);

                mp.x += mp_hat.x;
                mp.y += mp_hat.y;
                mp.z += mp_hat.z;
            } /* if */
        } /* for i */

        fo_magnitude = sqrt( SQR( f.x ) + SQR( f.y ) + SQR( f.z ) );

        fu0 = (u0.x * f.x) + (u0.y * f.y) + (u0.z * f.z);
        fu1 = (u1.x * f.x) + (u1.y * f.y) + (u1.z * f.z);
        fu2 = (u2.x * f.x) + (u2.y * f.y) + (u2.z * f.z);

        fp_magnitude = sqrt( SQR( fu0 ) + SQR( fu1 ) + SQR( fu2 ) );

        mo_magnitude = sqrt( SQR( m.x ) + SQR( m.y ) + SQR( m.z ) );

        mu0 = (u0.x * mp.x) + (u0.y * mp.y) + (u0.z * mp.z);
        mu1 = (u1.x * mp.x) + (u1.y * mp.y) + (u1.z * mp.z);
        mu2 = (u2.x * mp.x) + (u2.y * mp.y) + (u2.z * mp.z);

        mp_magnitude = sqrt( SQR( mu0 ) + SQR( mu1 ) + SQR( mu2 ) );



        /*
         * Cleanup
         */

        DELETE_LIST( point_data_list );
    } /* if NO_INTERP */



    if ( (REG_INTERP == analy->interp_mode)  ||  (GOOD_INTERP == analy->interp_mode) )
    {
        /*
         * I.  Update traction data table information:
         *
         *     Assign valid element number, material, and <rst> coordinates of valid points;
         *
         *
         *     Remove nodes containing a point that does NOT lie within a brick element, i.e., element # == null_element,
         *          AND
         *     Remove nodes containing an element that is NOT in the traction material list
         */

        materials = p_mo_class->objects.elems->mat;

        for ( point_data_node = point_data_list, i = 0; 
              point_data_node != NULL; i++ )
        {
            element_idx = point_data_node->elnum;

            /*
             * NOTE:  offset materials[ element_idx ] to convert to 
             *        "real-world" material number
             */

            if ( (null_element != element_idx)  
                 && (BITTEST( traction_material_list, 
                              materials[ element_idx ] + 1 )) )
            {
                traction_data_table[ i ].element = element_idx;

                traction_data_table[ i ].element_centered_coordinates.r = 
                    point_data_node->xi[ r_coordinate ];
                traction_data_table[ i ].element_centered_coordinates.s = 
                    point_data_node->xi[ s_coordinate ];
                traction_data_table[ i ].element_centered_coordinates.t = 
                    point_data_node->xi[ t_coordinate ];

                valid_elements_found = TRUE;
            }
            else
            {
                /*
                 * "nullify" all points located in:
                 *
                 * a.) "space", or
                 * b.) elements whose material is NOT in the requested traction
                 *     material list
                 *
                 * NOTE:  Updating traction data table materials and <rst> 
                 *        coordinate system is NOT required for NULL elements.
                 */

                traction_data_table[ i ].element = null_element;
            }

            NEXT( point_data_node );
        }

        if ( FALSE == valid_elements_found )
        {
            popup_dialog( WARNING_POPUP, "traction:  No points lie within "
                          "valid mesh elements.\n" );
            DELETE_LIST( point_data_list );

            return;
        }



        /*
         * II.  Collect 6 stress values for ALL brick elements of entire mesh
         *
         *      Stress values are returned/assigned in a data structure of the 
         *      form:
         *
         *                 elements
         *           sxx:  0  1...n - 1, syy:  0...n - 1, ... ,szx:  0...n - 1
         *
         *      within a single-dimension array.
         *
         *
         *      Stress values may be accessed:
         *
         *      sxx:  sigmas__Form_II[ total_brick_elements * 0  ...  
         *                             (total_brick_elements * 1) - 1 ]
         *      syy:  sigmas__Form_II[ total_brick_elements * 1  ...  
         *                             (total_brick_elements * 2) - 1 ]
         *      szz:  sigmas__Form_II[ total_brick_elements * 2  ...  
         *                             (total_brick_elements * 3) - 1 ]
         *      sxy:  sigmas__Form_II[ total_brick_elements * 3  ...  
         *                             (total_brick_elements * 4) - 1 ]
         *      syz:  sigmas__Form_II[ total_brick_elements * 4  ...  
         *                             (total_brick_elements * 5) - 1 ]
         *      szx:  sigmas__Form_II[ total_brick_elements * 5  ...  
         *                             (total_brick_elements * 6) - 1 ]
         *
         *      e.g.,
         *
         *      total_brick_elements = 36
         *
         *      sigmas__Form_II[ 75 ]  =
         *                             =  sigmas__Form_II[ total_brick_elements
         *                                                 * 2) + 3 ]
         *                             =  szz[ element_idx #3 ]
         *                             =  szz for element (real world ) #4
         */

        find_result( analy, PRIMAL, TRUE, &new_result, "stress" );

        p_subrecord = analy->srec_tree[ new_result.srec_id ].subrecs;

        primal_values[ 0 ] = primal_specification__sxx;
        primal_values[ 1 ] = primal_specification__syy;
        primal_values[ 2 ] = primal_specification__szz;
        primal_values[ 3 ] = primal_specification__sxy;
        primal_values[ 4 ] = primal_specification__syz;
        primal_values[ 5 ] = primal_specification__szx;
        primal_values[ 6 ] = NULL;

        for ( j = 0; j < new_result.qty; j++ )
        {
            subrecord_index = new_result.subrecs[ j ];

            p_class = p_subrecord[ subrecord_index ].p_object_class;

            if ( p_mo_class == p_class )
                break;
        }

        analy->db_get_results( analy->db_ident, analy->cur_state + 1, 
                               subrecord_index, 6, primal_values, 
                               analy->tmp_result[ 0 ] );


        sigmas__Form_II = analy->tmp_result[ 0 ];



        /*
         * III.  Collect 6 stress values for ALL nodes of entire mesh
         *
         *      Stress values are returned/assigned in a data structure of the 
         *      form:
         *
         *                     element
         *                 0  1  ...  n - 1
         *           sxx   #  #   #     #
         *           syy   #  #   #     #
         *           szz   #  #   #     #
         *           sxy   #  #   #     #
         *           syz   #  #   #     #
         *           szx   #  #   #     #
         *
         *
         *      Stress values may be accessed:
         *
         *      nodal_stress_data[sxx | syy | szz | sxy | syz | szx][node_idx]
         *
         *      e.g.,
         *
         *      The stress "syz" for node 142 may be found:
         *
         *      nodal_stress_data[ syz ][ 141 ]
         */


        if ( NULL == (nodal_stress_data = NEW_N( float *, qty_stress_types, 
                                                 "nodal_stress_data" )) )
        {
            popup_dialog( WARNING_POPUP, 
                          "traction:  storage allocation fail." );
            return;
        }

        p_sigmas__Form_II = sigmas__Form_II;

        for ( i = 0; i < qty_stress_types; i++ )
        {
            if ( NULL == (nodal_stress_data[ i ] = NEW_N( float, total_nodes, 
                                                        "nodal_stress_data" )) )
            {
                popup_dialog( WARNING_POPUP, 
                              "traction:  storage allocation fail." );
                return;
            }

            hex_to_nodal( p_sigmas__Form_II, nodal_stress_data[ i ], 
                          p_mo_class, total_brick_elements, NULL, analy );

            p_sigmas__Form_II += total_brick_elements;
        }



        /*
         * IV.  Compute traction force vector components
         */

        connectivity_qty = qty_connects[ p_mo_class->superclass ];

        f.x = 0.0;
        f.y = 0.0;
        f.z = 0.0;

        m.x = 0.0;
        m.y = 0.0;
        m.z = 0.0;

        mp.x = 0.0;
        mp.y = 0.0;
        mp.z = 0.0;

        for ( i = 0; i < qty_tdt; i++ )
        {
            if ( null_element != traction_data_table[ i ].element )
            {
                element_idx = traction_data_table[ i ].element;

                x = traction_data_table[ i ].point.x;
                y = traction_data_table[ i ].point.y;
                z = traction_data_table[ i ].point.z;

                r = traction_data_table[ i ].element_centered_coordinates.r;
                s = traction_data_table[ i ].element_centered_coordinates.s;
                t = traction_data_table[ i ].element_centered_coordinates.t;

                dA = traction_data_table[ i ].area;

                n = traction_data_table[ i ].normal;


                /*
                 * Interpolate nodal stresses to point stresses
                 */

                element_nodal_connectivities = p_mo_class->objects.elems->nodes
                                               + element_idx * connectivity_qty;

                shape_fns_hex( r, s, t, h );
               
                sigma__xx = 0.0;
                sigma__yy = 0.0;
                sigma__zz = 0.0;
                sigma__xy = 0.0;
                sigma__yz = 0.0;
                sigma__zx = 0.0;

                for ( j = 0; j < connectivity_qty; j++ )
                {
                    node_idx = element_nodal_connectivities[ j ];

                    shape_function = h[ j ];

                    sigma__xx += shape_function * nodal_stress_data[ sxx ][ node_idx ];
                    sigma__yy += shape_function * nodal_stress_data[ syy ][ node_idx ];
                    sigma__zz += shape_function * nodal_stress_data[ szz ][ node_idx ];
                    sigma__xy += shape_function * nodal_stress_data[ sxy ][ node_idx ];
                    sigma__yz += shape_function * nodal_stress_data[ syz ][ node_idx ];
                    sigma__zx += shape_function * nodal_stress_data[ szx ][ node_idx ];
                } /* for j */

                sigma__yx = sigma__xy;
                sigma__xz = sigma__zx;
                sigma__zy = sigma__yz;


                area += dA;

                f_hat.x = ( (sigma__xx * n.x) + (sigma__yx * n.y) + (sigma__zx * n.z) ) * dA;
                f_hat.y = ( (sigma__xy * n.x) + (sigma__yy * n.y) + (sigma__zy * n.z) ) * dA;
                f_hat.z = ( (sigma__xz * n.x) + (sigma__yz * n.y) + (sigma__zz * n.z) ) * dA;


                f.x += f_hat.x;
                f.y += f_hat.y;
                f.z += f_hat.z;

                m.x += (y * f_hat.z) - (z * f_hat.y);
                m.y += (z * f_hat.x) - (x * f_hat.z);
                m.z += (x * f_hat.y) - (y * f_hat.x);


                mp_hat.x = ((y - p.y) * f_hat.z) - ((z - p.z) * f_hat.y);
                mp_hat.y = ((z - p.z) * f_hat.x) - ((x - p.x) * f_hat.z);
                mp_hat.z = ((x - p.x) * f_hat.y) - ((y - p.y) * f_hat.x);

                mp.x += mp_hat.x;
                mp.y += mp_hat.y;
                mp.z += mp_hat.z;
            } /* if */
        } /* for i */

        fo_magnitude = sqrt( SQR( f.x ) + SQR( f.y ) + SQR( f.z ) );

        fu0 = (u0.x * f.x) + (u0.y * f.y) + (u0.z * f.z);
        fu1 = (u1.x * f.x) + (u1.y * f.y) + (u1.z * f.z);
        fu2 = (u2.x * f.x) + (u2.y * f.y) + (u2.z * f.z);

        fp_magnitude = sqrt( SQR( fu0 ) + SQR( fu1 ) + SQR( fu2 ) );

        mo_magnitude = sqrt( SQR( m.x ) + SQR( m.y ) + SQR( m.z ) );

        mu0 = (u0.x * mp.x) + (u0.y * mp.y) + (u0.z * mp.z);
        mu1 = (u1.x * mp.x) + (u1.y * mp.y) + (u1.z * mp.z);
        mu2 = (u2.x * mp.x) + (u2.y * mp.y) + (u2.z * mp.z);

        mp_magnitude = sqrt( SQR( mu0 ) + SQR( mu1 ) + SQR( mu2 ) );



        /*
         * Cleanup
         */

        DELETE_LIST( point_data_list );

        for ( i = 0; i < qty_stress_types; i++ )
            free( nodal_stress_data[ i ] );

        free( nodal_stress_data );
    } /* if REG_INTERP */



    /*
     * Display results (for both interpolated and non-interpolated modes)
     */

    wrt_text( "\n" );
    wrt_text( "Traction area:  %g\n", area );


    if ( f.y >= 0 )
        strcpy( sign_term_2, "+" );
    else
        strcpy( sign_term_2, "-" );

    if ( f.z >= 0 )
        strcpy( sign_term_3, "+" );
    else
        strcpy( sign_term_3, "-" );

    wrt_text( "Fo = %gx %s %gy %s %gz\n", f.x, sign_term_2, fabs( (double)f.y ), sign_term_3, fabs( (double)f.z ) );
    wrt_text( "Fo magnitude:  %g\n", fo_magnitude );


    if ( fu1 >= 0 )
        strcpy( sign_term_2, "+" );
    else
        strcpy( sign_term_2, "-" );

    if ( fu2 >= 0 )
        strcpy( sign_term_3, "+" );
    else
        strcpy( sign_term_3, "-" );

    wrt_text( "Fp = %gv %s %gu1 %s %gu2\n", fu0, sign_term_2, fabs( (double)fu1 ), sign_term_3, fabs( (double)fu2 ) );
    wrt_text( "Fp magnitude:  %g\n", fp_magnitude );


    if ( m.y >= 0 )
        strcpy( sign_term_2, "+" );
    else
        strcpy( sign_term_2, "-" );

    if ( m.z >= 0 )
        strcpy( sign_term_3, "+" );
    else
        strcpy( sign_term_3, "-" );

    wrt_text( "Mo = %gx %s %gy %s %gz\n", m.x, sign_term_2, fabs( (double)m.y ), sign_term_3, fabs( (double)m.z ) );
    wrt_text( "Mo magnitude:  %g\n", mo_magnitude );


    if ( mu1 >= 0 )
        strcpy( sign_term_2, "+" );
    else
        strcpy( sign_term_2, "-" );

    if ( mu2 >= 0 )
        strcpy( sign_term_3, "+" );
    else
        strcpy( sign_term_3, "-" );

    wrt_text( "Mp = %gv %s %gu1 %s %gu2\n", mu0, sign_term_2, fabs( (double)mu1 ), sign_term_3, fabs( (double)mu2 ) );
    wrt_text( "Mp magnitude:  %g\n", mp_magnitude );


    return;
}



/*****************************************************************
 * TAG( traction_area )
 *
 * Description goes here!!
 */
float
traction_area( Analysis *analy, char *traction_material_list )
{
    Htable_entry
                 *p_hash_table_entry;

    MO_class_data
                  *p_mo_class;

    Vector_pt_obj
                  *point_data_list
                 ,*point_data_node;
    
    char
         class[] = "brick";

    float
          total_traction_area = 0.0;

    int
        element_idx
       ,i;

    int
        *materials;


    enum {
           x_coordinate
          ,y_coordinate
          ,z_coordinate
         };


    /* */

    total_traction_area = 0.0;


    if ( OK != htable_search( MESH( analy ).class_table, class, FIND_ENTRY, &p_hash_table_entry ) )
    {
        popup_dialog( USAGE_POPUP, "traction_area:  invalid class name" );
        return( (float) invalid_state );
    }
    
    p_mo_class = (MO_class_data *) p_hash_table_entry->data;



    /*
     * "init_vec_points_hex" will determine the identification of the element containing each point
     * and the <rst> coordinates of the point within the element
     *
     * if a point does NOT lie within an element, the element identification number is "null_element"
     *
     * NOTE:  all element numbers returned in point_data_list->elnum are the actual, real-world
     *        element numbers
     */

    point_data_list = NULL;

    for ( i = 0; i < qty_tdt; i++ )
    {
        point_data_node = NEW( Vector_pt_obj, "point_data_node" );

        point_data_node->pt[ x_coordinate ] = traction_data_table[ i ].point.x;
        point_data_node->pt[ y_coordinate ] = traction_data_table[ i ].point.y;
        point_data_node->pt[ z_coordinate ] = traction_data_table[ i ].point.z;

        point_data_node->elnum              = null_element;

        APPEND( point_data_node, point_data_list );
    }


    for ( i = 0; i < analy->mesh_qty; i++ )
        init_vec_points_hex( &point_data_list, i, analy, FALSE );


    /*
     * Compute total traction area based upon those points contained within "valid" elements
     *
     * NOTE:  ALL materials are valid
     */

    materials = p_mo_class->objects.elems->mat;

    for ( point_data_node = point_data_list, i = 0; point_data_node != NULL; i++ )
    {
        element_idx = point_data_node->elnum;

        /*
         * NOTE:  offset materials[ element_idx ] to convert to "real-world" material number
         */

        if ( (null_element != element_idx)  &&  (BITTEST( traction_material_list, materials[ element_idx ] + 1 )) )
            total_traction_area += traction_data_table[ i ].area;

        NEXT( point_data_node );
    }


    DELETE_LIST( point_data_list );


    return( total_traction_area );
}
