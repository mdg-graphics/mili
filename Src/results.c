/* $Id$ */
/*
 * results.c - Routines for generating analysis result variables.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Mar 19 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include <values.h>
#include "viewer.h"


static void update_min_max();
static void load_hex_result();
static void load_shell_result();
static void load_nodal_result();


/*****************************************************************
 * TAG( trans_result )
 *
 * Translation table which associates a result type with the command
 * string used to identify that result type.
 *
 * Each row in the table contains:
 *
 *     Result ID (int)
 *     Result title (string)
 *     Function which computes the result (function)
 *         its arguments are: (analy, result_array)
 *     Command string for displaying result (string)
 */
char *trans_result[][4] =
{
    { (char *) VAL_NONE,                        "No Result",
      (char *) NULL,                            "mat" },

    { (char *) VAL_NODE_DISPX,                  "X Displacement",
      (char *) compute_node_displacement,       "dispx" },
    { (char *) VAL_NODE_DISPY,                  "Y Displacement",
      (char *) compute_node_displacement,       "dispy" },
    { (char *) VAL_NODE_DISPZ,                  "Z Displacement",
      (char *) compute_node_displacement,       "dispz" },
    { (char *) VAL_NODE_DISPMAG,                "Displacement Magnitude",
      (char *) compute_node_displacement,       "dispmag" },
    { (char *) VAL_NODE_VELX,                   "X Velocity",
      (char *) compute_node_velocity,           "velx" },
    { (char *) VAL_NODE_VELY,                   "Y Velocity",
      (char *) compute_node_velocity,           "vely" },
    { (char *) VAL_NODE_VELZ,                   "Z Velocity",
      (char *) compute_node_velocity,           "velz" },
    { (char *) VAL_NODE_VELMAG,                 "Velocity Magnitude",
      (char *) compute_node_velocity,           "velmag" },
    { (char *) VAL_NODE_ACCX,                   "X Acceleration",
      (char *) compute_node_acceleration,       "accx" },
    { (char *) VAL_NODE_ACCY,                   "Y Acceleration",
      (char *) compute_node_acceleration,       "accy" },
    { (char *) VAL_NODE_ACCZ,                   "Z Acceleration",
      (char *) compute_node_acceleration,       "accz" },
    { (char *) VAL_NODE_ACCMAG,                 "Acceleration Magnitude",
      (char *) compute_node_acceleration,       "accmag" },
    { (char *) VAL_NODE_TEMP,                   "Temperature",
      (char *) compute_node_temperature,        "temp" },
    { (char *) VAL_NODE_PINTENSE,               "Pressure Intensity",
      (char *) compute_node_pr_intense,         "pint" },
    { (char *) VAL_NODE_HELICITY,		"Helicity",
      (char *) compute_node_helicity,		"hel" },
    { (char *) VAL_NODE_ENSTROPHY,		"Enstrophy",
      (char *) compute_node_enstrophy,		"ens" },
    { (char *) VAL_NODE_K,			"k",
      (char *) compute_node_in_data,		"k" },
    { (char *) VAL_NODE_EPSILON,		"Epsilon",
      (char *) compute_node_in_data,		"eps" },
    { (char *) VAL_NODE_A2,			"A2",
      (char *) compute_node_in_data,		"a2" },

    { (char *) VAL_HEX_SIG_PD1,                 "Prin Dev Stress 1",
      (char *) compute_hex_principal_stress,    "pdev1" },
    { (char *) VAL_HEX_SIG_PD2,                 "Prin Dev Stress 2",
      (char *) compute_hex_principal_stress,    "pdev2" },
    { (char *) VAL_HEX_SIG_PD3,                 "Prin Dev Stress 3",
      (char *) compute_hex_principal_stress,    "pdev3" },
    { (char *) VAL_HEX_SIG_MAX_SHEAR,           "Maximum Shear Stress",
      (char *) compute_hex_principal_stress,    "maxshr" },
    { (char *) VAL_HEX_SIG_P1,                  "Principal Stress 1",
      (char *) compute_hex_principal_stress,    "prin1" },
    { (char *) VAL_HEX_SIG_P2,                  "Principal Stress 2",
      (char *) compute_hex_principal_stress,    "prin2" },
    { (char *) VAL_HEX_SIG_P3,                  "Principal Stress 3",
      (char *) compute_hex_principal_stress,    "prin3" },
    { (char *) VAL_HEX_EPS_PD1,                 "Prin Dev Strain 1",
      (char *) compute_hex_strain,              "pdstrn1" },
    { (char *) VAL_HEX_EPS_PD2,                 "Prin Dev Strain 2",
      (char *) compute_hex_strain,              "pdstrn2" },
    { (char *) VAL_HEX_EPS_PD3,                 "Prin Dev Strain 3",
      (char *) compute_hex_strain,              "pdstrn3" },
    { (char *) VAL_HEX_EPS_MAX_SHEAR,           "Maximum Shear Strain",
      (char *) compute_hex_strain,              "pshrstr" },
    { (char *) VAL_HEX_EPS_P1,                  "Principal Strain 1",
      (char *) compute_hex_strain,              "pstrn1" },
    { (char *) VAL_HEX_EPS_P2,                  "Principal Strain 2",
      (char *) compute_hex_strain,              "pstrn2" },
    { (char *) VAL_HEX_EPS_P3,                  "Principal Strain 3",
      (char *) compute_hex_strain,              "pstrn3" },
    { (char *) VAL_HEX_RELVOL,                  "Relative Volume",
      (char *) compute_hex_relative_volume,     "relvol" },

    { (char *) VAL_SHELL_RES1,                  "M_xx Bending Resultant",
      (char *) compute_shell_in_data,           "res1" },
    { (char *) VAL_SHELL_RES2,                  "M_yy Bending Resultant",
      (char *) compute_shell_in_data,           "res2" },
    { (char *) VAL_SHELL_RES3,                  "M_zz Bending Resultant",
      (char *) compute_shell_in_data,           "res3" },
    { (char *) VAL_SHELL_RES4,                  "Q_xx Shear Resultant",
      (char *) compute_shell_in_data,           "res4" },
    { (char *) VAL_SHELL_RES5,                  "Q_yy Shear Resultant",
      (char *) compute_shell_in_data,           "res5" },
    { (char *) VAL_SHELL_RES6,                  "N_xx Normal Resultant",
      (char *) compute_shell_in_data,           "res6" },
    { (char *) VAL_SHELL_RES7,                  "N_yy Normal Resultant",
      (char *) compute_shell_in_data,           "res7" },
    { (char *) VAL_SHELL_RES8,                  "N_zz Normal Resultant",
      (char *) compute_shell_in_data,           "res8" },
    { (char *) VAL_SHELL_THICKNESS,             "Thickness",
      (char *) compute_shell_in_data,           "thick" },
    { (char *) VAL_SHELL_INT_ENG,               "Internal Energy",
      (char *) compute_shell_in_data,           "inteng" },

    { (char *) VAL_SHELL_SURF1,                 "Surface Stress 1",
      (char *) compute_shell_surface_stress,    "surf1" },
    { (char *) VAL_SHELL_SURF2,                 "Surface Stress 2",
      (char *) compute_shell_surface_stress,    "surf2" },
    { (char *) VAL_SHELL_SURF3,                 "Surface Stress 3",
      (char *) compute_shell_surface_stress,    "surf3" },
    { (char *) VAL_SHELL_SURF4,                 "Surface Stress 4",
      (char *) compute_shell_surface_stress,    "surf4" },
    { (char *) VAL_SHELL_SURF5,                 "Surface Stress 5",
      (char *) compute_shell_surface_stress,    "surf5" },
    { (char *) VAL_SHELL_SURF6,                 "Surface Stress 6",
      (char *) compute_shell_surface_stress,    "surf6" },
    { (char *) VAL_SHELL_EFF1,          "Effective Upper Surface Stress",
      (char *) compute_shell_surface_stress,    "eff1" },
    { (char *) VAL_SHELL_EFF2,          "Effective Lower Surface Stress",
      (char *) compute_shell_surface_stress,    "eff2" },
    { (char *) VAL_SHELL_EFF3,          "Maximum Effective Surface Stress",
      (char *) compute_shell_surface_stress,    "effmax" },

    { (char *) VAL_SHARE_SIGX,                  "X Stress",
      (char *) compute_share_stress,            "sx" },
    { (char *) VAL_SHARE_SIGY,                  "Y Stress",
      (char *) compute_share_stress,            "sy" },
    { (char *) VAL_SHARE_SIGZ,                  "Z Stress",
      (char *) compute_share_stress,            "sz" },
    { (char *) VAL_SHARE_SIGXY,                 "XY Stress",
      (char *) compute_share_stress,            "sxy" },
    { (char *) VAL_SHARE_SIGYZ,                 "YZ Stress",
      (char *) compute_share_stress,            "syz" },
    { (char *) VAL_SHARE_SIGZX,                 "ZX Stress",
      (char *) compute_share_stress,            "szx" },

    { (char *) VAL_SHARE_SIG_EFF,               "Effective Stress",
      (char *) compute_share_effstress,         "seff" },

    { (char *) VAL_SHARE_EPSX,                  "X Strain",
      (char *) compute_share_strain,            "ex" },
    { (char *) VAL_SHARE_EPSY,                  "Y Strain",
      (char *) compute_share_strain,            "ey" },
    { (char *) VAL_SHARE_EPSZ,                  "Z Strain",
      (char *) compute_share_strain,            "ez" },
    { (char *) VAL_SHARE_EPSXY,                 "XY Strain",
      (char *) compute_share_strain,            "exy" },
    { (char *) VAL_SHARE_EPSYZ,                 "YZ Strain",
      (char *) compute_share_strain,            "eyz" },
    { (char *) VAL_SHARE_EPSZX,                 "ZX Strain",
      (char *) compute_share_strain,            "ezx" },

    { (char *) VAL_SHARE_EPS_EFF,               "Eff Plastic Strain",
      (char *) compute_share_eff_strain,        "eeff" },

    { (char *) VAL_SHARE_PRESS,                 "Pressure",
      (char *) compute_share_press,             "press" },

    { (char *) VAL_BEAM_AX_FORCE,               "Axial Force",
      (char *) compute_beam_in_data,            "axfor" },
    { (char *) VAL_BEAM_S_SHEAR,                "S Shear Resultant",
      (char *) compute_beam_in_data,            "sshear" },
    { (char *) VAL_BEAM_T_SHEAR,                "T Shear Resultant",
      (char *) compute_beam_in_data,            "tshear" },
    { (char *) VAL_BEAM_S_MOMENT,               "S Moment",
      (char *) compute_beam_in_data,            "smom" },
    { (char *) VAL_BEAM_T_MOMENT,               "T Moment",
      (char *) compute_beam_in_data,            "tmom" },
    { (char *) VAL_BEAM_TOR_MOMENT,             "Torsional Resultant",
      (char *) compute_beam_in_data,            "tor" },
    { (char *) VAL_BEAM_S_AX_STRN_P,		"S Axial Strain (+)",
      (char *) compute_beam_axial_strain,	"saep"},
    { (char *) VAL_BEAM_S_AX_STRN_M,		"S Axial Strain (-)",
      (char *) compute_beam_axial_strain,	"saem"},
    { (char *) VAL_BEAM_T_AX_STRN_P,		"T Axial Strain (+)",
      (char *) compute_beam_axial_strain,	"taep"},
    { (char *) VAL_BEAM_T_AX_STRN_M,		"T Axial Strain (-)",
      (char *) compute_beam_axial_strain,	"taem"},

    { (char *) NULL,                    NULL,
      (char *) NULL,                    NULL }   /* Dummy to signal end. */
};


/*****************************************************************
 * TAG( resultid_to_index )
 *
 * Table which gives the index in the trans_result table for each
 * result id.
 */
int *resultid_to_index;


/*****************************************************************
 * TAG( init_resultid_table )
 *
 * Create the resultid_to_index table.
 */
void
init_resultid_table()
{
    int i;

    resultid_to_index = NEW_N( int, VAL_ALL_END, "Resultid table" );

    /* Initialize to -1 to keep track of unmapped IDs. */
    for ( i = 0; i < VAL_ALL_END; i++ )
        resultid_to_index[i] = -1;

    for ( i = 0; trans_result[i][3] != NULL; i++ )
        resultid_to_index[(int)trans_result[i][0]] = i;
}


/*****************************************************************
 * TAG( lookup_result_id )
 *
 * Returns the result ID number for the given result command
 * name.  If the name is not found in the results table, the
 * routine returns -1.
 */
int
lookup_result_id( com_name )
char *com_name;
{
    int i;

    /* Get the result type from the command using the translation table. */
    for ( i = 0; trans_result[i][3] != NULL; i++ )
    {
        /* For efficiency, match the first characters first. */
        if ( com_name[0] == trans_result[i][3][0] &&
             strcmp( com_name, trans_result[i][3] ) == 0 )
        {
            return( (int)trans_result[i][0] );
        }
    }

    return( -1 );
}


/*****************************************************************
 * TAG( is_derived )
 *
 * Returns TRUE if the specified result type can be computed from
 * the variables in the plotfile database, otherwise FALSE.
 */
int
is_derived( result_id )
Result_type result_id;
{
    if ( resultid_to_index[result_id] < 0 )
        return FALSE;
    else
        return TRUE;
}


/*****************************************************************
 * TAG( is_hex_result )
 *
 * Returns TRUE if the specified result type applies to hex
 * elements.
 */
int
is_hex_result( result_id )
Result_type result_id;
{
    if ( result_id > VAL_HEX_BEGIN && result_id < VAL_HEX_END )
        return TRUE;
    else
        return FALSE;
}


/*****************************************************************
 * TAG( is_shell_result )
 *
 * Returns TRUE if the specified result type applies to shell
 * elements.
 */
int
is_shell_result( result_id )
Result_type result_id;
{
    if ( result_id > VAL_SHELL_BEGIN && result_id < VAL_SHELL_END )
        return TRUE;
    else
        return FALSE;
}


/*****************************************************************
 * TAG( is_shared_result )
 *
 * Returns TRUE if the specified result type applies to brick and
 * shell elements.
 */
int
is_shared_result( result_id )
Result_type result_id;
{
    if ( result_id > VAL_SHARE_BEGIN && result_id < VAL_SHARE_END )
        return TRUE;
    else
        return FALSE;
}


/*****************************************************************
 * TAG( is_beam_result )
 *
 * Returns TRUE if the specified result type applies to beam
 * elements.
 */
int
is_beam_result( result_id )
Result_type result_id;
{
    if ( result_id > VAL_BEAM_BEGIN && result_id < VAL_BEAM_END )
        return TRUE;
    else
        return FALSE;
}


/*****************************************************************
 * TAG( is_nodal_result )
 *
 * Returns TRUE if the specified result type is nodal data.
 */
int
is_nodal_result( result_id )
Result_type result_id;
{
    if ( result_id > VAL_NODE_BEGIN && result_id < VAL_NODE_END )
        return TRUE;
    else
        return FALSE;
}


/*****************************************************************
 * TAG( get_result_modifiers )
 *
 * Determine which possible result modifiers affect the
 * current result.
 */
int
get_result_modifiers( analy, modifiers )
Analysis *analy;
Result_modifier_type modifiers[];
{
    Result_type res_id;
    int cnt;

    res_id = analy->result_id;
    cnt = 0;
	
    /*
     * Strain variety needed for shared strains and principal strains
     * on hex elements.
     */
    if ( analy->geom_p->bricks != NULL )
    {
        if ( ( res_id >= VAL_SHARE_EPSX && res_id <= VAL_SHARE_EPSZX )
             || ( res_id >= VAL_HEX_EPS_PD1 && res_id <= VAL_HEX_EPS_P3 ) )
            modifiers[cnt++] = STRAIN_TYPE;
    }
    
    if ( is_shared_result( res_id ) && analy->geom_p->shells != NULL )
    {
        /* Reference surface needed for all shared shell results. */
        modifiers[cnt++] = REFERENCE_SURFACE;
	
        /* 
         * Reference frame needed for all but effective stress/strain
         * and pressure.
         */
        if ( ( res_id >= VAL_SHARE_SIGX && res_id <= VAL_SHARE_SIGZX )
             || ( res_id >= VAL_SHARE_EPSX && res_id <= VAL_SHARE_EPSZX ) )
	    modifiers[cnt++] = REFERENCE_FRAME;
    }
    
    return cnt;
}


/*****************************************************************
 * TAG( update_min_max )
 *
 * Update the state and global min/max for the current result
 * data.
 */
static void
update_min_max( analy )
Analysis *analy;
{
    float *state_mm, *result;
    int cnt, i;
    int *mm_nodes;

    state_mm = analy->state_mm;
    mm_nodes = analy->state_mm_nodes;
    result = analy->result;
    cnt = analy->geom_p->nodes->cnt;

    /* Update the state min/max. */
    state_mm[0] = result[0];
    mm_nodes[0] = 1;
    state_mm[1] = result[0];
    mm_nodes[1] = 1;

    for ( i = 1; i < cnt; i++ )
    {
        if ( result[i] < state_mm[0] )
	{
            state_mm[0] = result[i];
	    mm_nodes[0] = i + 1;
	}
        else if ( result[i] > state_mm[1] )
	{
            state_mm[1] = result[i];
	    mm_nodes[1] = i + 1;
	}
    }

    /* Update the global min/max. */
    if ( analy->result_mod )
    {
        analy->global_mm[0] = state_mm[0];
	analy->global_mm_nodes[0] = mm_nodes[0];
        analy->global_mm[1] = state_mm[1];
	analy->global_mm_nodes[1] = mm_nodes[1];
    }
    else
    {
        if ( state_mm[0] < analy->global_mm[0] )
	{
            analy->global_mm[0] = state_mm[0];
	    analy->global_mm_nodes[0] = mm_nodes[0];
	}
        if ( state_mm[1] > analy->global_mm[1] )
	{
            analy->global_mm[1] = state_mm[1];
	    analy->global_mm_nodes[1] = mm_nodes[1];
	}
    }

    /* Update the current min/max. */
    for ( i = 0; i < 2; i++ )
        if ( !analy->mm_result_set[i] )
        {
            if ( analy->use_global_mm )
                analy->result_mm[i] = analy->global_mm[i];
            else
                analy->result_mm[i] = analy->state_mm[i];
        }
}


/*****************************************************************
 * TAG( hex_vol )
 *
 * Compute the approximate volume of a hex element using 1-point
 * integration.  (Taken from DYNA code, subroutine vlmass().)
 */
float
hex_vol( x, y, z )
float x[8];
float y[8];
float z[8];
{
    float aj[9], vol;

    /*
     * Jacobian matrix.
     */
    aj[0] = -x[0]-x[1]+x[2]+x[3]-x[4]-x[5]+x[6]+x[7];
    aj[3] = -x[0]-x[1]-x[2]-x[3]+x[4]+x[5]+x[6]+x[7];
    aj[6] = -x[0]+x[1]+x[2]-x[3]-x[4]+x[5]+x[6]-x[7];
    aj[1] = -y[0]-y[1]+y[2]+y[3]-y[4]-y[5]+y[6]+y[7];
    aj[4] = -y[0]-y[1]-y[2]-y[3]+y[4]+y[5]+y[6]+y[7];
    aj[7] = -y[0]+y[1]+y[2]-y[3]-y[4]+y[5]+y[6]-y[7];
    aj[2] = -z[0]-z[1]+z[2]+z[3]-z[4]-z[5]+z[6]+z[7];
    aj[5] = -z[0]-z[1]-z[2]-z[3]+z[4]+z[5]+z[6]+z[7];
    aj[8] = -z[0]+z[1]+z[2]-z[3]-z[4]+z[5]+z[6]-z[7];

    /*
     * Jacobian.
     */
    vol = aj[0]*aj[4]*aj[8]+aj[1]*aj[5]*aj[6]+aj[2]*aj[3]*aj[7]-
          aj[2]*aj[4]*aj[6]-aj[1]*aj[3]*aj[8]-aj[0]*aj[5]*aj[7];
    vol = 0.0156250 * vol;

    return vol;
}
 
 
/***************************************************************** 
 * TAG( hex_vol_exact ) 
 *
 * Compute the exact volume of a hex element using 2-point integration.
 * (Taken from DYNA code, subroutine vlmffb().)
 */ 
float 
hex_vol_exact( x, y, z )
float x[8];
float y[8];
float z[8];
{ 
    float a45, a24, a52, a16, a31, a63, a27, a74, a38, a81, a86, a57;
    float a6345, a5238, a8624, a7416, a5731, a8127;
    float px1, px2, px3, px4, px5, px6, px7, px8;
    float vol;

    a45 = z[3]-z[4];
    a24 = z[1]-z[3];
    a52 = z[4]-z[1];
    a16 = z[0]-z[5];
    a31 = z[2]-z[0];
    a63 = z[5]-z[2];
    a27 = z[1]-z[6];
    a74 = z[6]-z[3];
    a38 = z[2]-z[7];
    a81 = z[7]-z[0];
    a86 = z[7]-z[5];
    a57 = z[4]-z[6];
    a6345 = a63-a45;
    a5238 = a52-a38;
    a8624 = a86-a24;
    a7416 = a74-a16;
    a5731 = a57-a31;
    a8127 = a81-a27;
    px1 = y[1]*a6345+y[2]*a24-y[3]*a5238+y[4]*a8624+y[5]*a52+y[7]*a45;
    px2 = y[2]*a7416+y[3]*a31-y[0]*a6345+y[5]*a5731+y[6]*a63+y[4]*a16;
    px3 = y[3]*a8127-y[0]*a24-y[1]*a7416-y[6]*a8624+y[7]*a74+y[5]*a27;
    px4 = y[0]*a5238-y[1]*a31-y[2]*a8127-y[7]*a5731+y[4]*a81+y[6]*a38;
    px5 = -y[7]*a7416+y[6]*a86+y[5]*a8127-y[0]*a8624-y[3]*a81-y[1]*a16;
    px6 = -y[4]*a8127+y[7]*a57+y[6]*a5238-y[1]*a5731-y[0]*a52-y[2]*a27;
    px7 = -y[5]*a5238-y[4]*a86+y[7]*a6345+y[2]*a8624-y[1]*a63-y[3]*a38;
    px8 = -y[6]*a6345-y[5]*a57+y[4]*a7416+y[3]*a5731-y[2]*a74-y[0]*a45;
    vol = (px1*x[0]+px2*x[1]+px3*x[2]+px4*x[3]+px5*x[4]+
          px6*x[5]+px7*x[6]+px8*x[7])/12.0;
 
    return vol;
}


/*****************************************************************
 * TAG( hex_to_nodal )
 *
 * Convert brick element result data to nodal data.
 */
void
hex_to_nodal( val_hex, val_nodal, analy )
float *val_hex;
float *val_nodal;
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    float xx[8], yy[8], zz[8];
    float *hex_vols, *vol_sums, *activity, *mm_val;
    Bool_type volume_average;
    int i, j, nd;
    int *el_type, *el_id;

    /* Set the next line to FALSE to compare with TAURUS output. */
    volume_average = TRUE;
    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    activity = analy->state_p->activity_present ?
               analy->state_p->bricks->activity : NULL;

    bzero( val_nodal, nodes->cnt * sizeof( float ) );
    hex_vols = NEW_N( float, bricks->cnt, "Hex volumes" );
    vol_sums = NEW_N( float, nodes->cnt, "Volume sums" );

    /*
     * Each node result is a volume-weighted average of the neighboring
     * element results.  The proper way to do this would be to interpolate
     * using the shape functions.
     */

    /* First loop to get the summed volumes. */
    for ( i = 0; i < bricks->cnt; i++ )
    {
        /* Ignore inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( analy->disable_material[ bricks->mat[i] ] )
            continue;

        for ( j = 0; j < 8; j++ )
        {
            nd = bricks->nodes[j][i];
            xx[j] = nodes->xyz[0][nd];
            yy[j] = nodes->xyz[1][nd];
            zz[j] = nodes->xyz[2][nd];
        }

        hex_vols[i] = hex_vol( xx, yy, zz );

        if ( hex_vols[i] <= 0.0 )
            wrt_text( "Warning: element volume is zero or negative!\n");

        for ( j = 0; j < 8; j++ )
        {
            nd = bricks->nodes[j][i];

            if ( volume_average )
                vol_sums[nd] += hex_vols[i];
            else
                vol_sums[nd] += 1.0;
        }
    }
    
    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.minmax;
    el_type = analy->tmp_elem_mm.el_type;
    el_id = analy->tmp_elem_mm.mesh_object;

    /* Loop to get the averaged values and extract state element min/max. */
    for ( i = 0; i < bricks->cnt; i++ )
    {
        /* Ignore inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( analy->disable_material[ bricks->mat[i] ] )
            continue;

	/* Test for new min or max. */
	if ( val_hex[i] < mm_val[0] )
	{
	    mm_val[0] = val_hex[i];
	    el_id[0] = i + 1;
            el_type[0] = BRICK_T;
	}
	
	if ( val_hex[i] > mm_val[1] )
	{
	    mm_val[1] = val_hex[i];
	    el_id[1] = i + 1;
            el_type[1] = BRICK_T;
	}

        for ( j = 0; j < 8; j++ )
        {
            nd = bricks->nodes[j][i];

            /*
             * Divide hex_vol by vol_sum first to avoid precision errors
             * for models with very small dimensions.
             */
            if ( volume_average )
                val_nodal[nd] += val_hex[i] * ( hex_vols[i] / vol_sums[nd] );
            else
                val_nodal[nd] += val_hex[i] / vol_sums[nd];
        }
    }

    free( hex_vols );
    free( vol_sums );
}


/*****************************************************************
 * TAG( shell_to_nodal )
 *
 * Convert shell element result data to nodal data.  If the merge
 * flag is TRUE, nodes that don't belong to shell elements aren't
 * zeroed.
 */
void
shell_to_nodal( val_shell, val_nodal, analy, merge )
float *val_shell;
float *val_nodal;
Analysis *analy;
Bool_type merge;
{
    Shell_geom *shells;
    Nodal *nodes;
    float *activity, *mm_val;
    int *adj_cnt;
    int i, j, nd;
    int *el_type, *el_id;

    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;
    activity = analy->state_p->activity_present ?
               analy->state_p->shells->activity : NULL;

    adj_cnt = NEW_N( int, nodes->cnt, "Tmp shell result cnts" );

    if ( merge )
    {
        /* Zero selectively. */
        for ( i = 0; i < shells->cnt; i++ )
        {
            if ( (activity && activity[i] == 0.0) ||
                 analy->disable_material[ shells->mat[i] ] )
                continue;

            for ( j = 0; j < 4; j++ )
                val_nodal[shells->nodes[j][i]] = 0.0;
        }
    }
    else
        bzero( val_nodal, nodes->cnt * sizeof( float ) );
    
    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.minmax;
    el_type = analy->tmp_elem_mm.el_type;
    el_id = analy->tmp_elem_mm.mesh_object;

    /*
     * Each node result is an average of the neighboring
     * element results.  Extract element min/max.
     */
    for ( i = 0; i < shells->cnt; i++ )
    {
        /* Ignore inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( analy->disable_material[ shells->mat[i] ] )
            continue;

	/* Test for new min or max. */
	if ( val_shell[i] < mm_val[0] )
	{
	    mm_val[0] = val_shell[i];
	    el_id[0] = i + 1;
            el_type[0] = SHELL_T;
	}
	
	if ( val_shell[i] > mm_val[1] )
	{
	    mm_val[1] = val_shell[i];
	    el_id[1] = i + 1;
            el_type[1] = SHELL_T;
	}

        for ( j = 0; j < 4; j++ )
        {
            nd = shells->nodes[j][i];
            val_nodal[nd] += val_shell[i];
            adj_cnt[nd]++;
        }
    }

    for ( i = 0; i < nodes->cnt; i++ )
        if ( adj_cnt[i] != 0 )
            val_nodal[i] = val_nodal[i] / adj_cnt[i];

    free( adj_cnt );
}


/*****************************************************************
 * TAG( beam_to_nodal )
 *
 * Convert beam element result data to nodal data.
 */
void
beam_to_nodal( val_beam, val_nodal, analy )
float *val_beam;
float *val_nodal;
Analysis *analy;
{
    Beam_geom *beams;
    Nodal *nodes;
    float *activity, *mm_val;
    int *adj_cnt;
    int i, j, nd;
    int *el_type, *el_id;

    beams = analy->geom_p->beams;
    nodes = analy->state_p->nodes;
    activity = analy->state_p->activity_present ?
               analy->state_p->beams->activity : NULL;

    adj_cnt = NEW_N( int, nodes->cnt, "Tmp beam result cnts" );

    bzero( val_nodal, nodes->cnt * sizeof( float ) );
    
    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.minmax;
    el_type = analy->tmp_elem_mm.el_type;
    el_id = analy->tmp_elem_mm.mesh_object;

    /*
     * Each node result is an average of the neighboring
     * element results.
     */
    for ( i = 0; i < beams->cnt; i++ )
    {
        /* Ignore inactive elements. */
        if ( activity && activity[i] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( analy->disable_material[ beams->mat[i] ] )
            continue;

	/* Test for new min or max. */
	if ( val_beam[i] < mm_val[0] )
	{
	    mm_val[0] = val_beam[i];
	    el_id[0] = i + 1;
            el_type[0] = BEAM_T;
	}
	
	if ( val_beam[i] > mm_val[1] )
	{
	    mm_val[1] = val_beam[i];
	    el_id[1] = i + 1;
            el_type[1] = BEAM_T;
	}

        for ( j = 0; j < 2; j++ )
        {
            nd = beams->nodes[j][i];
            val_nodal[nd] += val_beam[i];
            adj_cnt[nd]++;
        }
    }

    for ( i = 0; i < nodes->cnt; i++ )
        if ( adj_cnt[i] != 0 )
            val_nodal[i] = val_nodal[i] / adj_cnt[i];

    free( adj_cnt );
}


/*****************************************************************
 * TAG( load_result )
 *
 * Load the result variable for display.  The update parameter
 * should be TRUE if result min/max is to be updated, FALSE if
 * not.
 */
void
load_result( analy, update )
Analysis *analy;
Bool_type update;
{
    void (*compute_func)();
    int r_idx;

    if ( analy->result_id != VAL_NONE )
    {
        /* Initialize temporary element min/max values. */
	analy->tmp_elem_mm.minmax[0] = MAXFLOAT;
	analy->tmp_elem_mm.minmax[1] = -MAXFLOAT;
	
        /* Get the compute function from the result table and call it. */
        r_idx = resultid_to_index[analy->result_id];
        compute_func = (void (*)()) trans_result[r_idx][2];
        compute_func( analy, analy->result );
    }

    if ( update && analy->result_id != VAL_NONE )
    {
        analy->elem_state_mm = analy->tmp_elem_mm;
        update_min_max( analy );
    }
}


