/* $Id$ */
/*
 * explode.c - Routines for material exploded views.
 *
 *      Doug Speck
 *      Lawrence Livermore National Laboratory
 *      Oct 20 1994
 *
 * Copyright (c) 1994 Regents of the University of California
 */

#include <string.h>
#include "viewer.h"

#define BAD_CMD (-1)

/************************************************************
 * TAG( Matl_exp_obj )
 *
 * Associate a set of materials with a point or line about which
 * the materials can be exploded.
 */
typedef struct _Matl_exp_obj
{
    struct _Matl_exp_obj *next;
    struct _Matl_exp_obj *prev;
    Exploded_view_type ev_type;
    int qty_matls;
    int *matls;
    float p1[3];
    float p2[3];
    float *dirvecs;
    float *distances;
    char *name;
} Matl_exp_obj;

static Matl_exp_obj *meo_list = NULL;

void find_matl_means();
int parse_num_spec();
Bool_type is_integer_string();
void free_matl_exp();
static Matl_exp_obj *delete_meo();


/************************************************************
 * TAG( associate_matl_exp )
 *
 * Initialize an association between a list of materials
 * and a point or line for material exploded views.
 */
int
associate_matl_exp( token_cnt, tokens, analy, exp )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
Analysis *analy;
Exploded_view_type exp;
{
    Matl_exp_obj *p_meo, *p_m;
    int i, j, tcnt, cnt, toks_read, stat;
    int *tmp_nums;
    float tmp[3], tmp2[3], line_dir[3], vec_start[3], pt[3];
    float len, scale, eval;
    float *tmp_means, *matl_cen;

    if ( analy->dimension != 3 )
    {
	wrt_text( "Exploded material views available only for 3D meshes.\n" );
	return ( 0 );
    }
    else if ( token_cnt < 5 )
	return ( BAD_CMD );

    tcnt = 1;
    p_meo = NEW( Matl_exp_obj, "Matl explode obj" );

    p_meo->ev_type = exp;

    /* There should always be at least one point defined. */
    p_meo->p1[0] = atof( tokens[1] );
    p_meo->p1[1] = atof( tokens[2] );
    p_meo->p1[2] = atof( tokens[3] );
    tcnt += 3;

    /* For line sources, there should be another point defined. */
    if ( exp == CYLINDRICAL || exp == AXIAL )
    {
	if ( token_cnt >= 8 )
        {
            p_meo->p2[0] = atof( tokens[4] );
            p_meo->p2[1] = atof( tokens[5] );
            p_meo->p2[2] = atof( tokens[6] );
            tcnt += 3;
        }
	else
	{
	    free( p_meo );
	    return ( BAD_CMD );
	}
    }

    /* Extract specified material numbers. */
    if ( strcmp( tokens[tcnt], "all" ) == 0 )
    {
	p_meo->matls = NEW_N( int, analy->num_materials, "Exp all materials" );

        for ( i = 0; i < analy->num_materials; i++ )
	    p_meo->matls[i] = i;

	tcnt++;

        p_meo->qty_matls = analy->num_materials;
    }
    else
    {
	/* Materials listed as explicit numbers and/or hyphenated ranges. */
        tmp_nums = NEW_N( int, analy->num_materials, "Temp matl nums" );
        stat = parse_num_spec( tokens + tcnt, token_cnt - tcnt, tmp_nums, &cnt,
		           &toks_read );
        if ( stat != 0 )
        {
	    free( p_meo );
	    free( tmp_nums );
	    return ( BAD_CMD );
        }

        tcnt += toks_read;

        /* Translate material numbers to zero-base and put in struct. */
        p_meo->matls = NEW_N( int, cnt, "Exp materials" );
        for ( i = 0; i < cnt; i++ )
	     p_meo->matls[i] = tmp_nums[i] - 1;

        free( tmp_nums );

        p_meo->qty_matls = cnt;
    }

    /* If name included, copy it into struct. */
    if ( token_cnt > tcnt )
    {
	p_meo->name = NEW_N( char, strlen( tokens[tcnt + 1] ) + 1, "MEO name" );
	strcpy( p_meo->name, tokens[token_cnt - 1] );

	/* Search for and replace old duplicates. */
	for ( p_m = meo_list; p_m != NULL; p_m = p_m->next )
	{
	    if ( p_m->name != NULL )
		if ( strcmp( p_m->name, p_meo->name ) == 0 )
		{
		    wrt_text( "Material exp name conflict; old replaced.\n" );
		    DELETE( p_m, meo_list );
		    break;
		}
	}
    }
    else
	p_meo->name = NULL;

    /* Calculate the geometric centers of each material at current state. */
    tmp_means = NEW_N( float, 3 * p_meo->qty_matls, "Temp matl means" );
    find_matl_means( analy, p_meo->matls, p_meo->qty_matls, tmp_means );

    /*
     * Calculate the material explosion direction vector based on
     * the exploded view source and type and the material center.
     */
    p_meo->dirvecs = NEW_N( float, 3 * p_meo->qty_matls, "MEO dirvecs" );
    p_meo->distances = NEW_N( float, p_meo->qty_matls, "MEO distances" );

    switch ( exp )
    {
	case SPHERICAL:
	    for ( i = 0, j = 0; i < p_meo->qty_matls; i++, j += 3 )
	    {
		VEC_SUB( tmp, tmp_means + j, p_meo->p1 );
		len = VEC_LENGTH( tmp );
		if ( len == 0.0 )
		{
		    p_meo->dirvecs[j] = 1.0;
		    p_meo->dirvecs[j + 1] = 0.0;
		    p_meo->dirvecs[j + 2] = 0.0;
		}
		else
		{
		    scale = 1.0 / len;
		    VEC_SCALE( p_meo->dirvecs + j, scale, tmp );
		}
		
		p_meo->distances[i] = len;
	    }
	    break;
	
	case CYLINDRICAL:
	    VEC_SUB( line_dir, p_meo->p2, p_meo->p1 );
	    if ( VEC_LENGTH( line_dir ) == 0.0 )
	    {
		free( p_meo->dirvecs );
		free( p_meo->distances );
		free( p_meo->matls );
		free( p_meo->name );
		free( p_meo );
		wrt_text( "Zero length source line for exploded view.\n" );
		return( BAD_CMD );
	    }

	    for ( i = 0, j = 0; i < p_meo->qty_matls; i++, j += 3 )
	    {
		matl_cen = tmp_means + j;
		near_pt_on_line( matl_cen, p_meo->p1, line_dir, vec_start );
		VEC_SUB( tmp, matl_cen, vec_start );
		len = VEC_LENGTH( tmp );
		if ( len == 0.0 )
		{
		    /*
		     * Yuk. Create any ol' vector perpendicular to the
		     * source line and use it. Yes, it's ugly.
		     */
		    VEC_COPY( tmp2, line_dir );
		    if ( tmp2[0] == 0.0 )
			tmp2[0] = 1.0;
		    else
			tmp2[1] = ( tmp2[1] == 0.0 ) ? 1.0 : tmp2[1] * 2.0;
		    
		    VEC_CROSS( tmp, line_dir, tmp2 );

		    scale = 1.0 / VEC_LENGTH( tmp );
		}
		else
		    scale = 1.0 / len;

		p_meo->distances[i] = len;
	        VEC_SCALE( p_meo->dirvecs + j, scale, tmp );
	    }
	    break;
	
	case AXIAL:
	    VEC_SUB( line_dir, p_meo->p2, p_meo->p1 );
	    len = VEC_LENGTH( line_dir );
	    if ( len == 0.0 )
	    {
		free( p_meo->dirvecs );
		free( p_meo->distances );
		free( p_meo->matls );
		free( p_meo->name );
		free( p_meo );
		wrt_text( "Zero length source line for exploded view.\n" );
		return( BAD_CMD );
	    }
	    else
	    {
		VEC_SCALE( line_dir, 1.0 / len, line_dir );

		/* find source line midpoint */
		pt[0] = (p_meo->p2[0] + p_meo->p1[0]) / 2.0;
		pt[1] = (p_meo->p2[1] + p_meo->p1[1]) / 2.0;
		pt[2] = (p_meo->p2[2] + p_meo->p1[2]) / 2.0;

	        for ( i = 0, j = 0; i < p_meo->qty_matls; i++, j += 3 )
	        {
		    /* 
		     * Materials centered in positive half-space of plane
		     * perpendicular to source line and through midpoint
		     * get line_dir, else -line_dir.
		     */
		    matl_cen = tmp_means + j;
		    eval = line_dir[0] * (matl_cen[0] - pt[0])
			   + line_dir[1] * (matl_cen[1] - pt[1])
			   + line_dir[2] * (matl_cen[2] - pt[2]);
		    
		    VEC_COPY( p_meo->dirvecs + j, line_dir );
		    if ( eval < 0.0 )
		    {
			VEC_NEGATE( p_meo->dirvecs + j );
		        p_meo->distances[i] = -eval;
		    }
		    else
		        p_meo->distances[i] = eval;
		}
	    }
	    break;

	default:
	    wrt_text( "Unknown exploded view type; ignored.\n" );
	    free( p_meo->dirvecs );
	    free( p_meo->distances );
	    free( p_meo->matls );
	    free( p_meo->name );
	    free( p_meo );
	    free( tmp_means );
	    return ( BAD_CMD );
    }

    free( tmp_means );

    /*
     * Put at end of list so that newer defs will be evaluated last
     * in traversals, replacing translations for same materials
     * from any previous defs.
     */
    APPEND( p_meo, meo_list );

    return ( 0 );
}


/************************************************************
 * TAG( find_matl_means )
 *
 * Compute the average x, y, and z of the nodes referenced by all
 * elements of each specified material type. Nodes referenced by
 * multiple elements are summed into average with EACH reference.
 */
void
find_matl_means( analy, matls, qty_matls, means )
Analysis *analy;
int matls[];
int qty_matls;
float means[][3];
{
    Hex_geom *bricks;
    Shell_geom *shells;
    Beam_geom *beams;
    Nodal *nodes;
    int i, j, matl, nd;
    int *cnt;
    float *sumx, *sumy, *sumz, *activity;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    beams = analy->geom_p->beams;
    nodes = analy->state_p->nodes;

    sumx = NEW_N( float, analy->num_materials, "Matl sum x" );
    sumy = NEW_N( float, analy->num_materials, "Matl sum y" );
    sumz = NEW_N( float, analy->num_materials, "Matl sum z" );
    cnt = NEW_N( int, analy->num_materials, "Matl sum counts" );

    /*
     * Sum the coordinates for all elements for ALL materials.
     * This avoids testing every element's material against the
     * requested material list, but will be inefficient if
     * the exploded view doesn't operate on a majority of the 
     * materials in the mesh.
     */

    /*
     * Split cases to avoid evaluating activity at every iteration
     * when possible.
     */
    if ( !analy->state_p->activity_present )
    {
        if ( bricks )
        {
            for ( i = 0; i < bricks->cnt; i++ )
            {
                matl = bricks->mat[i];
                for ( j = 0; j < 8; j++ )
                {
            	    nd = bricks->nodes[j][i];
            	    sumx[matl] += nodes->xyz[0][nd];
            	    sumy[matl] += nodes->xyz[1][nd];
            	    sumz[matl] += nodes->xyz[2][nd];
                }
                cnt[matl] += 8;
            }
        }

	if ( shells )
        {
            for ( i = 0; i < shells->cnt; i++ )
            {
                matl = shells->mat[i];
                for ( j = 0; j < 4; j++ )
                {
            	    nd = shells->nodes[j][i];
            	    sumx[matl] += nodes->xyz[0][nd];
            	    sumy[matl] += nodes->xyz[1][nd];
            	    sumz[matl] += nodes->xyz[2][nd];
                }
                cnt[matl] += 4;
            }
        }
        
	if ( beams )
        {
            for ( i = 0; i < beams->cnt; i++ )
            {
                matl = beams->mat[i];
                for ( j = 0; j < 2; j++ )
                {
            	    nd = beams->nodes[j][i];
            	    sumx[matl] += nodes->xyz[0][nd];
            	    sumy[matl] += nodes->xyz[1][nd];
            	    sumz[matl] += nodes->xyz[2][nd];
                }
                cnt[matl] += 2;
            }
        }
    }
    else
    {
	if ( bricks )
        {
	    activity = analy->state_p->bricks->activity;
            for ( i = 0; i < bricks->cnt; i++ )
            {
		if ( activity[i] != 0.0 )
		{
                    matl = bricks->mat[i];
                    for ( j = 0; j < 8; j++ )
                    {
            	        nd = bricks->nodes[j][i];
            	        sumx[matl] += nodes->xyz[0][nd];
            	        sumy[matl] += nodes->xyz[1][nd];
            	        sumz[matl] += nodes->xyz[2][nd];
                    }
                    cnt[matl] += 8;
		}
            }
        }

        if ( shells )
        {
	    activity = analy->state_p->shells->activity;
            for ( i = 0; i < shells->cnt; i++ )
            {
		if ( activity[i] != 0.0 )
		{
                    matl = shells->mat[i];
                    for ( j = 0; j < 4; j++ )
                    {
            	        nd = shells->nodes[j][i];
            	        sumx[matl] += nodes->xyz[0][nd];
            	        sumy[matl] += nodes->xyz[1][nd];
            	        sumz[matl] += nodes->xyz[2][nd];
                    }
                    cnt[matl] += 4;
		}
            }
        }
    
	if ( beams )
        {
	    activity = analy->state_p->beams->activity;
            for ( i = 0; i < beams->cnt; i++ )
            {
		if ( activity[i] != 0.0 )
		{
                    matl = beams->mat[i];
                    for ( j = 0; j < 2; j++ )
                    {
            	        nd = beams->nodes[j][i];
            	        sumx[matl] += nodes->xyz[0][nd];
            	        sumy[matl] += nodes->xyz[1][nd];
            	        sumz[matl] += nodes->xyz[2][nd];
                    }
                    cnt[matl] += 2;
		}
            }
        }
    }

    /*
     * Now calculate the average coordinates for each of the
     * requested materials.
     */
    for ( i = 0; i < qty_matls; i++ )
    {
	matl = matls[i];
	means[i][0] = sumx[matl] / (float) cnt[matl];
	means[i][1] = sumy[matl] / (float) cnt[matl];
	means[i][2] = sumz[matl] / (float) cnt[matl];
    }

    free( sumx );
    free( sumy );
    free( sumz );
    free( cnt );
}


/************************************************************
 * TAG( parse_num_spec )
 *
 * Parse command line tokens specifying individual numbers and
 * ranges of numbers into an array explicitly listing each number.
 */
int
parse_num_spec( tokens, token_cnt, nums, p_cnt, p_toks_used )
char tokens[][TOKENLENGTH];
int token_cnt;
int nums[];
int *p_cnt;
int *p_toks_used;
{
    int i, j, k, start_num, end_num, num_tok_cnt, len;
    char start[16], end[16];
    char *hyph_pos;

    for ( num_tok_cnt = 0, j = 0, i = 0; i < token_cnt; i++ )
    {
	/* Case - token composed of all numerals is a single number. */
	if ( is_integer_string( tokens[i] ) )
	{
	    nums[j++] = atof( tokens[i] );
	    num_tok_cnt++;
	}
	/* Case - token contains a hyphen; may be a range. */
	else if ( (hyph_pos = strchr( tokens[i], (int) '-' )) != NULL )
	{
	    /* Extract strings preceding and following hyphen. */
	    len = (int) (hyph_pos - tokens[i]);
	    strncpy( start, tokens[i], len );
	    start[len] = '\0';

	    len = strlen( hyph_pos + 1 );
	    strncpy( end, hyph_pos + 1, len );
	    end[len] = '\0';

	    /* Have valid range if both strings are numeric. */
	    if ( is_integer_string( start ) && is_integer_string( end ) )
	    {
		start_num = atof( start );
		end_num = atof( end );
		if ( start_num > end_num )
		{
		    k = start_num;
		    start_num = end_num;
		    end_num = k;
		}
		for ( k = start_num; k <= end_num; k++ )
		    nums[j++] = k;
	    }
	    else
		wrt_text( "Malformed numeric range \"%s\" ignored.\n",
			  tokens[i] );
	    
	    num_tok_cnt++;
	}
	/* Else not number or range, ignore */
    }

    if ( j > 0 )
    {
        *p_cnt = j;
	*p_toks_used = num_tok_cnt;

	return (0);
    }
    else
	return (-1);
}


/*****************************************************************
 * TAG( is_integer_string )
 * 
 * Evaluate a string and return TRUE if each character is a member
 * of 0-9.
 */
Bool_type
is_integer_string( num_string )
char *num_string;
{
    unsigned char *p_c;

    for ( p_c = (unsigned char *) num_string;
	  *p_c != (unsigned char ) 0;
	  p_c++ )
	if ( !isdigit( (int) *p_c ) )
	    return FALSE;
    
    return TRUE;
}


/*****************************************************************
 * TAG( explode_materials )
 * 
 * Translate materials for an exploded view.
 */
void
explode_materials( token_cnt, tokens, analy, scaled )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
Analysis *analy;
Bool_type scaled;
{
    Matl_exp_obj *p_meo;
    int i, j, matl, veci;
    float scale;

    /* Extract and convert the explosion distance. */
    scale = atof( tokens[token_cnt - 1] );
    
    /* If no names, apply distance to all exploded view defs. */
    if ( token_cnt == 2 )
    {
	for ( p_meo = meo_list; p_meo != NULL; p_meo = p_meo->next )
	{
	    for ( i = 0; i < p_meo->qty_matls; i++ )
	    {
		matl = p_meo->matls[i];
		veci = 3 * i;
		if ( scaled )
		{
	            analy->mtl_trans[0][matl] = p_meo->dirvecs[veci]
						* (scale - 1.0)
						* p_meo->distances[i];
	            analy->mtl_trans[1][matl] = p_meo->dirvecs[veci + 1]
						* (scale - 1.0)
						* p_meo->distances[i];
	            analy->mtl_trans[2][matl] = p_meo->dirvecs[veci + 2]
						* (scale - 1.0)
						* p_meo->distances[i];
		}
		else
		{
	            analy->mtl_trans[0][matl] = p_meo->dirvecs[veci] * scale;
	            analy->mtl_trans[1][matl] = p_meo->dirvecs[veci+1] * scale;
	            analy->mtl_trans[2][matl] = p_meo->dirvecs[veci+2] * scale;
		}
	    }
	}
    }
    else
    {
	/* For each name given... */
	for ( j = 1; j < token_cnt - 1; j++ )
	{
	    /* Traverse the associate-material-exploded view list. */
            for ( p_meo = meo_list; p_meo != NULL; p_meo = p_meo->next )
            {
		/* If name matches token, explode those materials. */
		if ( p_meo->name && strcmp( p_meo->name, tokens[j] ) == 0 )
		{
                    for ( i = 0; i < p_meo->qty_matls; i++ )
                    {
            	        matl = p_meo->matls[i];
			veci = 3 * i;
			if ( scaled )
			{
                            analy->mtl_trans[0][matl] = p_meo->dirvecs[veci]
						        * (scale - 1.0)
							* p_meo->distances[i];
                            analy->mtl_trans[1][matl] = p_meo->dirvecs[veci + 1]
						        * (scale - 1.0)
							* p_meo->distances[i];
                            analy->mtl_trans[2][matl] = p_meo->dirvecs[veci + 2]
						        * (scale - 1.0)
							* p_meo->distances[i];
			}
			else
			{
                            analy->mtl_trans[0][matl] = p_meo->dirvecs[veci]
						        * scale;
                            analy->mtl_trans[1][matl] = p_meo->dirvecs[veci + 1]
						        * scale;
                            analy->mtl_trans[2][matl] = p_meo->dirvecs[veci + 2]
						        * scale;
			}
                    }
		    break;
		}
            }
	}
    }
}


/************************************************************
 * TAG( free_matl_exp )
 *
 * Free all allocated memory in the associate-material-exploded view
 * list.
 */
void
free_matl_exp()
{
    Matl_exp_obj *p_meo;

    p_meo = meo_list; 
    while ( p_meo != NULL )
	p_meo = delete_meo( p_meo, &meo_list );
}


/************************************************************
 * TAG( remove_exp_assoc )
 *
 * Remove some or all of the material/exploded view associations.
 */
void
remove_exp_assoc( token_cnt, tokens )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
{
    Matl_exp_obj *p_meo;
    int i;

    if ( token_cnt < 2 )
    {
	popup_dialog( USAGE_POPUP, "emrm <name> [<name>...]\nOR\nemrm all" );
	return;
    }

    if ( strcmp( tokens[1], "all" ) == 0 )
	free_matl_exp();
    else
    {
	/*
	 * Traverse the meo_list for each name, deleting the list
	 * node when a match is found.
	 */
	for ( i = 1; i < token_cnt; i++ )
	{
	    p_meo = meo_list; 
	    while ( p_meo != NULL )
	    {
		if ( p_meo->name != NULL
		     && strcmp( p_meo->name, tokens[i] ) == 0 )
		    p_meo = delete_meo( p_meo, &meo_list );
		else
		    p_meo = p_meo->next;
	    }
	}
    }
}


/************************************************************
 * TAG( delete_meo )
 *
 * Free memory associated with a list-resident Matl_exp_obj struct,
 * returning a pointer to the next list struct.
 */
static Matl_exp_obj *
delete_meo( p_meo, p_list )
Matl_exp_obj *p_meo;
Matl_exp_obj **p_list;
{
    Matl_exp_obj *p_next;

    /* Manage the list. */
    if ( p_meo == *p_list )
        *p_list = p_meo->next;
    else
    {
        p_meo->prev->next = p_meo->next;
        if ( p_meo->next != NULL )
            p_meo->next->prev = p_meo->prev;
    }

    /* Free allocated contents. */
    free( p_meo->matls );
    free( p_meo->dirvecs );
    free( p_meo->distances );
    if ( p_meo->name ) free( p_meo->name );

    /* Free the struct. */
    p_next = p_meo->next;
    free( p_meo );

    return p_next;
}


/************************************************************
 * TAG( report_exp_assoc )
 *
 * Write a summary of the current material/exploded view
 * associations to the monitor window.
 */
#define MATL_WID (4)

void
report_exp_assoc()
{
    Matl_exp_obj *p_meo;
    static char *ev_type_names[] = { "SPHERICAL", "CYLINDRICAL", "AXIAL" };
    char *name, *lead;
    char *matl_str = "Materials:";
    char *pad_str =  "          "; /* same len as "matl_str" */
    int win_width, per_line, num_chars, j, i, line_out, remaining, qty;

    /*
     * Initialize vars to adapt the material number output rows to the
     * current monitor window width.
     */
    num_chars = MATL_WID - 1;
    win_width = get_monitor_width();
    per_line = (win_width - strlen( matl_str )) / MATL_WID;
    if ( per_line <= 0 ) per_line = 10; /* Stingy? They can scroll. */

    wrt_text( "\n* * Material/Exploded View Associations * *\n\n" );
    if ( meo_list == NULL ) wrt_text( "(none)\n" );

    /* Traverse the associate-material-exploded view list. */
    for ( p_meo = meo_list; p_meo != NULL; p_meo = p_meo->next )
    {
	/* Output name, type, and associated material quantity. */
	name = ( p_meo->name == NULL ) ? "(unnamed)" : p_meo->name;
	wrt_text( "Name: %-10.10s Type: %s   Qty materials: %d\n",
		  name, ev_type_names[p_meo->ev_type], p_meo->qty_matls );

	/* Output reference point/line. */
	switch ( p_meo->ev_type )
	{
	    case SPHERICAL:
		wrt_text( "Reference point: %.2f %.2f %.2f\n",
			  p_meo->p1[0], p_meo->p1[1], p_meo->p1[2] );
		break;
	
	    case CYLINDRICAL:
	    case AXIAL:
		wrt_text( "Reference line: %.2f %.2f %.2f    %.2f %.2f %.2f\n",
			  p_meo->p1[0], p_meo->p1[1], p_meo->p1[2],
			  p_meo->p2[0], p_meo->p2[1], p_meo->p2[2] );
		break;
	
	    default:
		wrt_text( "Unknown exploded view type.\n" );
		return;
	}

	/* Output the material numbers. */
	qty = p_meo->qty_matls;
	j = 0;
	while ( j < qty )
	{
	    remaining = qty - j;
	    line_out = ( remaining < per_line ) ? remaining : per_line;
	    lead = ( j < per_line ) ? matl_str : pad_str;
	    wrt_text( "%s", lead );
	    for ( i = 0; i < line_out; i++ )
	    {
	        wrt_text( " %-*d", num_chars, p_meo->matls[j] + 1 );
		j++;
	    }
	    wrt_text( "\n" );
	}
	wrt_text( "\n" );
    }
}


