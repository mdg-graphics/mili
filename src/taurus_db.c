/*
 Copyright (c) 2016, Lawrence Livermore National Security, LLC.
 Produced at the Lawrence Livermore National Laboratory. Written
 by Kevin Durrenberger: durrenberger1@llnl.gov. CODE-OCEC-16-056.
 All rights reserved.

 This file is part of Mili. For details, see <URL describing code
 and how to download source>.

 Please also read this link-- Our Notice and GNU Lesser General
 Public License.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License (as published by
 the Free Software Foundation) version 2.1 dated February 1999.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms
 and conditions of the GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

 *
 *
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *
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

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include "mili.h"
#include "mili_internal.h"
#include "taurus_db.h"

/*
 * Control words in plotfile header:
 *
 *   ndim    = ctl[ 0]     Dimension of data
 *   numnp   = ctl[ 1]     Number of nodes
 *   icode   = ctl[ 2]     Flag for old or new database
 *   nglbv   = ctl[ 3]     Number of global variables for each state
 *   it      = ctl[ 4]     Temperatures included flag
 *   iu      = ctl[ 5]     Current geometry included flag
 *   iv      = ctl[ 6]     Velocities included flag
 *   ia      = ctl[ 7]     Accelerations included flag
 *   nel8    = ctl[ 8]     Number of brick elements
 *   nummat8 = ctl[ 9]     Number of materials used by brick elements
 *   nv3d    = ctl[12]     Number of variables for each brick element
 *   nel2    = ctl[13]     Number of beam elements
 *   nummat2 = ctl[14]     Number of materials used by beam elements
 *   nv1d    = ctl[15]     Number of variables for each beam element
 *   nel4    = ctl[16]     Number of shell elements
 *   nummat4 = ctl[17]     Number of materials used by shell elements
 *   nv2d    = ctl[18]     Number of variables for each shell element
 *   activ   = ctl[20]     Element activity included flag
 *   ixd     = ctl[24]     Flag word for extra data
 */

Return_value taurus_make_umesh(Famid fam_id, char *mesh_name, int dim, int *p_mesh_id);
Return_value taurus_make_srec(Mili_family *fam, int mesh_id, int *p_srec_id);
Return_value taurus_build_state_map(Mili_family *fam, Bool_type initial_build, int ctl[], int srec_id);
Return_value taurus_def_subrec(Mili_family *fam, int srec_id, char *subrec_name, int org, int qty_svars,
                               char **svar_names, char *class, int format, int cell_qty, int *mo_ids, Bool_type first);
Return_value taurus_def_class(Famid fam_id, int mesh_id, int superclass, char *short_name, char *long_name);
Return_value taurus_def_class_idents(Famid fam_id, int mesh_id, char *short_name, int start, int stop);
Return_value taurus_def_nodes(Famid fam_id, int mesh_id, char *short_name, int start_node, int stop_node);
Return_value taurus_def_conn_seq(Mili_family *fam, int mesh_id, char *short_name, int start_el, int stop_el);
Return_value taurus_def_svars(Mili_family *fam, int qty, char **names, char **titles, int types[]);
Return_value taurus_def_vec_svar(Mili_family *fam, int type, int size, char *name, char *title, char **field_names,
                                 char **field_titles);
extern Return_value taurus_commit_svars(Mili_family *fam);
extern Return_value taurus_commit_srecs(Mili_family *fam);
extern void taurus_delete_mesh(Mili_family *fam);
extern Return_value taurus_reset_class_data(Mili_family *, int, Bool_type);

/* Local routines. */
static int taurus_open_family(Mili_family *fam, Famid fam_id, char *mode);
Return_value taurus_load_direc(Mili_family *fam, int ctl[]);
Return_value taurus_load_descriptors(Mili_family *fam, Famid fam_id, int ctl[], int *p_srec_id);
static void taurus_cleanse(Mili_family *fam);

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;

/*****************************************************************
 * TAG( fam_qty )
 *
 * The actual number of all currently open MILI families.
 */
extern int fam_qty;

/*****************************************************************
 * TAG( fam_array_length )
 *
 * The length of fam_list.
 */
extern int fam_array_length;

/*****************************************************************
 * TAG( Class names )
 *
 * Short and long class names assigned to Taurus db data.
 */

static char *glob_long_name = "Global";
static char *glob_short_name = "glob";

static char *mat_long_name = "Material";
static char *mat_short_name = "mat";

static char *wall_long_name = "RigidWall";
static char *wall_short_name = "wall";

static char *nike_long_name = "NikeSlide";
static char *nike_short_name = "nike";

static char *hex_long_name = "Bricks";
static char *hex_short_name = "brick";

static char *shell_long_name = "Shells";
static char *shell_short_name = "shell";

static char *beam_long_name = "Beams";
static char *beam_short_name = "beam";

static char *node_long_name = "Nodal";
static char *node_short_name = "node";

/*****************************************************************
 * TAG( taurus_open )
 *
 * Open a family of MDG plot files.  Returns -1 if unsuccessful,
 * otherwise returns the number of states in the family.  Note
 * that if the plot file has valid geometry but no states, the
 * routine returns 0.
 */
int taurus_open(char *root_name, char *path, char *mode, Famid *fam_id)
{
    Mili_family *fam;
    int root_len, path_len;
    char *lpath;
    Return_value rval;
    
    root_len = (int)strlen(root_name);

    if ( path != NULL && *path != '\0' )
    {
        path_len = (int)strlen(path);
        lpath = path;
    }
    else
    {
        lpath = ".";
        path_len = 1;
    }

    fam = NEW(Mili_family, "Taurus family");

    fam_list = RENEW_N(Mili_family *, fam_list, fam_qty, 1, "Big Kahuna pointers");
    fam_list[fam_qty] = fam;
    *fam_id = fam_qty;
    fam_qty++;
    fam_array_length++;

    /*  Need to let the world know that this is a Taurus data base */
    fam->db_type = TAURUS_DB_TYPE;

    /* Build the family root string. */
    fam->root_len = path_len + root_len + 1;
    fam->root = (char *)calloc(fam->root_len + 1, sizeof(char));
    sprintf(fam->root, "%s/%s", lpath, root_name);

    /* Save the separate components. */
    str_dup(&fam->path, lpath);
    str_dup(&fam->file_root, root_name);

    /* Regardless of mode argument, only permit read access on Taurus db's. */
    fam->access_mode = 'r';

    /* Initialize file indices. */
    fam->cur_index = -1;
    fam->cur_st_index = -1;

    fam->commit_max = -1;

    /* State data file pointer. */
    fam->cur_st_file = NULL;

    /* Open the family. */
    rval = (Return_value)taurus_open_family(fam, *fam_id, "r");

    if ( rval != OK )
    {
        cleanse(fam);
        free(fam_list[fam_qty]);
        fam_qty--;
        fam_array_length--;
        *fam_id = ID_FAIL;
        return (int)rval;
    }
    else
        return (int)OK;
}

/*****************************************************************
 * TAG( taurus_open_family ) LOCAL
 *
 * Open a taurus family.
 */
static int taurus_open_family(Mili_family *fam, Famid fam_id, char *mode)
{
    FILE *p_f;
    char fname[128];
    int read_cnt;
    long int offset;
    int ctl[50];
    int p_srec_id;
    struct stat statbuf;
    int i;
    int ndim;
    int nel2;
    int nel4;
    int nel8;
    int numnp;
    int rval;

    /**/
    /* Need to initialize all of fam */

    fam->precision_limit = PREC_LIMIT_SINGLE;
#ifdef OLD_STUFF
    /* Set appropriate I/O routines for family. */
    rval = set_default_io_routines(fam);
    if ( rval != OK )
        return rval;
#endif
    /* Open file and read in control data. */

    i = 0;
    make_fnam(TAURUS_DATA, fam, i, fname);
    p_f = fopen(fname, "rb");
    if ( p_f == NULL )
        return OPEN_FAILED;
    fam->cur_file = p_f;
    fam->file_count = 1;

    /* Seek past the title bytes. */
    offset = 15 * sizeof(int);
    rval = (Return_value)fseek(p_f, offset, SEEK_SET);
    if ( rval != OK )
        return rval;

    /* Just read the first control word, ndim, to check endianness. */
    read_cnt = (int)fread(ctl, 4, 1, p_f);
    if ( read_cnt != 1 )
        return SHORT_READ;

    /*
     * Use ndim control word to infer endianness of db.  Possible values
     * are non-zero but require only the low-order byte, the other bytes
     * should all be zero. Therefore if host endianness and db endianness
     * are the same, ndim will be less than 256 (if there's a mis-match,
     * ndim will be greater than (2**24 - 1) ).
     */
    if ( ctl[0] < 256 )
        fam->swap_bytes = FALSE;
    else
        fam->swap_bytes = TRUE;

    /* Now we have everything to set appropriate I/O routines for family. */
    rval = set_default_io_routines(fam);
    if ( rval != OK )
        return rval;

    /*
     * Now reset the file pointer and read the whole header,
     * byte-swapping if necessary.
     */
    rval = (Return_value)fseek(p_f, offset, SEEK_SET);
    if ( rval != OK )
        return rval;

    read_cnt = (int)fam->read_funcs[M_INT](p_f, ctl, 49);
    if ( read_cnt != 49 )
        return SHORT_READ;

    rval = taurus_load_direc(fam, ctl);
    if ( rval != OK || fam->file_count == 0 )
        return rval;

    /* Load entries into hash tables. */
    rval = taurus_load_descriptors(fam, fam_id, ctl, &p_srec_id);
    if ( rval != OK )
        return rval;

    /* Load entries into hash tables. */
    rval = taurus_build_state_map(fam, TRUE, ctl, p_srec_id);
    if ( rval != OK )
        return rval;
    /*
     * Determine the number of files containing geometry or
     * 'non-state' data
     */

    if ( ctl[0] == 4 )
        ctl[0] = 3;

    ndim = ctl[0];
    numnp = ctl[1];
    nel8 = ctl[8];
    nel2 = ctl[13];
    nel4 = ctl[16];

    offset = 64 * EXT_SIZE(fam, M_INT) + (ndim * numnp + 9 * nel8 + 5 * nel4 + 6 * nel2) * EXT_SIZE(fam, M_FLOAT);

    stat(fname, &statbuf);
    offset -= statbuf.st_size;
    while ( offset >= 0 )
    {
        i++;
        make_fnam(TAURUS_DATA, fam, i, fname);
        stat(fname, &statbuf);
        offset -= statbuf.st_size;
        /*****
                fam->file_count += 1;
        ****/
    }

    return rval;
}

/*****************************************************************
 * TAG( taurus_load_direc ) PRIVATE
 *
 * Traverse non-state files in family and read in directories.
 *
 * Should only be called at initialization of an existing family.
 *
 * This routine is hard-coded to the current (initial) design
 * of directories.  A new design would require abstraction of the
 * directory handling routines.
 */
Return_value taurus_load_direc(Mili_family *fam, int ctl[])
{
    LONGLONG c_qty;
    Param_ref *ppr;
    int fidx;
    char *title;
    Return_value rval;

    /* Allocate storage for current non-state file's directory. */
    fam->directory = NEW_N(File_dir, 1, "Load dir File_dir");

    /* Create the param table if necessary. */
    if ( fam->param_table == NULL )
        fam->param_table = htable_create(DEFAULT_HASH_TABLE_SIZE);

    /* Add directory entry. Modifier is Mili data type. */
    c_qty = 15 * sizeof(int);
    str_dup(&title, "title");
    rval = add_dir_entry(fam, APPLICATION_PARAM, M_STRING, DONT_CARE, 1, &title, 0, c_qty);
    if ( rval != OK )
        return rval;

    free(title);

    fam->next_free_byte += c_qty;

    /* Now create entry for the param table. */
    ppr = NEW(Param_ref, "Param table entry - string");
    fidx = 0;
    ppr->file_index = fidx;
    ppr->entry_index = fam->directory[fidx].qty_entries - 1;
    rval = htable_add_entry_data(fam->param_table, "title", ENTER_ALWAYS, ppr);
    if ( rval != OK )
    {
        free(ppr);
        return rval;
    }

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( taurus_load_svars ) PRIVATE
 */
Return_value taurus_load_svars(Mili_family *fam, int ctl[])
{
    int activity;
    int ia;
    int it;
    int iu;
    int iv;
    int ixd;
    int icode;
    int ndim;
    int nel2;
    int nel4;
    int nel8;
    int nglbv;
    int nummat2;
    int nummat4;
    int nummat8;
    int nv2d;
    int nv3d;
    int nummat;
    int numrw;
    Return_value rval = (Return_value)OK;

    if ( ctl[0] == 4 )
    {
        ctl[0] = 3;
    }

    ndim = ctl[0];
    icode = ctl[2];
    nglbv = ctl[3];
    it = ctl[4];
    iu = ctl[5];
    iv = ctl[6];
    ia = ctl[7];
    nel8 = ctl[8];
    nummat8 = ctl[9];
    nv3d = ctl[12];
    nel2 = ctl[13];
    nummat2 = ctl[14];
    nel4 = ctl[16];
    nummat4 = ctl[17];
    nv2d = ctl[18];
    activity = ctl[20];
    ixd = ctl[24];

    if ( nglbv > 0 )
    {
        if ( activity != 2000 ) /* ... nike3d serial number ... */
                                /* See Nike3d source code nik3di.f */
        {
            rval = taurus_def_svars(fam, 1, global_primal_shorts, global_primal_longs, types);

            rval = taurus_def_svars(fam, 1, global_primal_shorts + 1, global_primal_longs + 1, types);

            rval = taurus_def_svars(fam, 1, global_primal_shorts + 2, global_primal_longs + 2, types);

            rval = taurus_def_svars(fam, 3, global_primal_shorts + 3, global_primal_longs + 3, types);

            nummat = nummat8 + nummat2 + nummat4;
            if ( nummat > 0 )
            {
                /* Material Variables */
                rval = taurus_def_svars(fam, 1, mat_primal_shorts, mat_primal_longs, types);

                rval = taurus_def_svars(fam, 1, mat_primal_shorts + 1, mat_primal_longs + 1, types);

                rval = taurus_def_svars(fam, 1, mat_primal_shorts + 2, mat_primal_longs + 2, types);

                rval = taurus_def_vec_svar(fam, types[0], 3, mat_primal_shorts[3], mat_primal_longs[3],
                                           mat_primal_shorts + 4, mat_primal_longs + 4);
            }

            /* Rigid Wall Data */
            numrw = nglbv - (6 + 6 * nummat);
            if ( numrw > 0 )
            {
                rval = taurus_def_svars(fam, 1, wall_primal_shorts, wall_primal_longs, types);
            }
        }
        else
        {
            /* Nike3d 'Global' data */
            rval = taurus_def_svars(fam, 1, nike_primal_shorts, nike_primal_longs, types);
        }
    }

    /* Nodal Positions */
    if ( iu == 1 )
    {
        rval = taurus_def_vec_svar(fam, M_FLOAT, ndim, node_primal_shorts[0], node_primal_longs[0], node_posit_shorts,
                                   node_posit_longs);
    }

    /* Nodal Velocities */
    if ( iv == 1 )
    {
        rval = taurus_def_vec_svar(fam, M_FLOAT, ndim, node_primal_shorts[1], node_primal_longs[1], node_vel_shorts,
                                   node_vel_longs);
    }

    /* Nodal Accelerations */
    if ( ia == 1 )
    {
        rval = taurus_def_vec_svar(fam, M_FLOAT, ndim, node_primal_shorts[2], node_primal_longs[2], node_acc_shorts,
                                   node_acc_longs);
    }

    /* Nodal temperatures. */
    if ( it || icode == 1 )
    {
        rval = taurus_def_svars(fam, 1, temp_short, temp_long, types);
    }

    /* Nodal k and epsilon. */
    if ( ixd & K_EPSILON_MASK )
    {
        rval = taurus_def_svars(fam, 2, flow_short, flow_long, types);

        /* Nodal A2. */
        if ( ixd & A2_MASK )
            rval = taurus_def_svars(fam, 1, flow_short + 2, flow_long + 2, types);
    }

    /* Brick data. */
    if ( nel8 > 0 )
    {
        if ( nv3d > 0 )
        {
            rval = taurus_def_vec_svar(fam, M_FLOAT, 6, hex_primal_shorts[0], hex_primal_longs[0], hex_stress_shorts,
                                       hex_stress_longs);

            rval = taurus_def_svars(fam, 1, hex_primal_shorts + 1, hex_primal_longs + 1, types);
        }
    }

    /* Beam data. */
    if ( nel2 > 0 )
    {
        rval = taurus_def_svars(fam, 6, beam_primal_shorts, beam_primal_longs, types);
    }

    /* Shell data. */
    if ( nel4 > 0 )
    {
        rval = taurus_def_vec_svar(fam, M_FLOAT, 6, shell_primal_shorts[0], shell_primal_longs[0], stress_comp_shorts,
                                   stress_comp_longs);

        rval = taurus_def_svars(fam, 1, shell_primal_shorts + 1, shell_primal_longs + 1, types);

        rval = taurus_def_vec_svar(fam, M_FLOAT, 6, shell_primal_shorts[2], shell_primal_longs[2], stress_comp_shorts,
                                   stress_comp_longs);

        rval = taurus_def_svars(fam, 1, shell_primal_shorts + 3, shell_primal_longs + 3, types);

        rval = taurus_def_vec_svar(fam, M_FLOAT, 6, shell_primal_shorts[4], shell_primal_longs[4], stress_comp_shorts,
                                   stress_comp_longs);

        rval = taurus_def_svars(fam, 1, shell_primal_shorts + 5, shell_primal_longs + 5, types);

        rval = taurus_def_vec_svar(fam, M_FLOAT, 3, shell_primal_shorts[6], shell_primal_longs[6], bend_comp_shorts,
                                   bend_comp_longs);

        rval = taurus_def_vec_svar(fam, M_FLOAT, 2, shell_primal_shorts[7], shell_primal_longs[7], shear_comp_shorts,
                                   shear_comp_longs);

        rval = taurus_def_vec_svar(fam, M_FLOAT, 3, shell_primal_shorts[8], shell_primal_longs[8], normal_comp_shorts,
                                   normal_comp_longs);

        rval = taurus_def_svars(fam, 3, shell_primal_shorts + 9, shell_primal_longs + 9, types);

        if ( nv2d == 33 )
        {
            rval = taurus_def_svars(fam, 1, shell_primal_shorts + 12, shell_primal_longs + 12, types);
        }

        if ( nv2d == 44 || nv2d == 45 || nv2d == 46 )
        {
            rval = taurus_def_vec_svar(fam, M_FLOAT, 6, shell_primal_shorts[13], shell_primal_longs[13],
                                       strain_comp_short, strain_comp_long);

            rval = taurus_def_vec_svar(fam, M_FLOAT, 6, shell_primal_shorts[14], shell_primal_longs[14],
                                       strain_comp_short, strain_comp_long);

            rval = taurus_def_svars(fam, 1, shell_primal_shorts + 12, shell_primal_longs + 12, types);

            rval = taurus_def_svars(fam, 1, shell_primal_shorts + 15, shell_primal_longs + 15, types);
        }
    }

    if ( activity >= 1000 && activity <= 1005 )
    {
        if ( nel8 > 0 )
        {
            rval = taurus_def_svars(fam, 1, sand_db_short, sand_db_long, types);
        }

        if ( nel2 > 0 )
        {
            rval = taurus_def_svars(fam, 1, sand_db_short, &sand_db_long[1], types);
        }

        if ( nel4 > 0 )
        {
            rval = taurus_def_svars(fam, 1, sand_db_short, &sand_db_long[2], types);
        }
    }

    taurus_commit_svars(fam);

    return rval;
}

/*****************************************************************
 * TAG( taurus_init_meshes ) LOCAL
 *
 */
static Return_value taurus_init_meshes(Mili_family *fam, Famid fam_id, int ctl[], int *p_mesh_id)
{
    Return_value stat;
    int ndim;
    int numnp;
    int nglbv;
    int nel8;
    int nummat8;
    int nel2;
    int nummat2;
    int nel4;
    int nummat4;
    int activity;
    int nummat;
    int numrw;

    if ( ctl[0] == 4 )
        ctl[0] = 3;

    ndim = ctl[0];
    numnp = ctl[1];
    nglbv = ctl[3];
    nel8 = ctl[8];
    nummat8 = ctl[9];
    nel2 = ctl[13];
    nummat2 = ctl[14];
    nel4 = ctl[16];
    nummat4 = ctl[17];
    activity = ctl[20];

    stat = taurus_make_umesh(fam_id, "Basic mesh", ndim, p_mesh_id);
    if ( stat != OK )
        return stat;

    stat = taurus_def_class(fam_id, *p_mesh_id, M_MESH, glob_short_name, glob_long_name);
    if ( stat != OK )
        return stat;

    stat = taurus_def_class_idents(fam_id, *p_mesh_id, glob_short_name, 1, 1);
    if ( stat != OK )
        return stat;

    nummat = nummat8 + nummat2 + nummat4;
    if ( nummat > 0 )
    {
        stat = taurus_def_class(fam_id, *p_mesh_id, M_MAT, mat_short_name, mat_long_name);
        if ( stat != OK )
            return stat;
        stat = taurus_def_class_idents(fam_id, *p_mesh_id, mat_short_name, 1, nummat);
        if ( stat != OK )
            return stat;
    }

    if ( nglbv > 0 )
    {
        if ( activity != 2000 )
        {
            numrw = nglbv - (6 + 6 * nummat);
            if ( numrw > 0 )
            {
                stat = taurus_def_class(fam_id, *p_mesh_id, M_UNIT, wall_short_name, wall_long_name);
                if ( stat != OK )
                    return stat;
                stat = taurus_def_class_idents(fam_id, *p_mesh_id, wall_short_name, 1, numrw);
                if ( stat != OK )
                    return stat;
            }
        }
        else
        {
            stat = taurus_def_class(fam_id, *p_mesh_id, M_UNIT, nike_short_name, nike_long_name);
            if ( stat != OK )
                return stat;
            stat = taurus_def_class_idents(fam_id, *p_mesh_id, nike_short_name, 1, nglbv);
            if ( stat != OK )
                return stat;
        }
    }

    stat = taurus_def_class(fam_id, *p_mesh_id, M_NODE, node_short_name, node_long_name);
    if ( stat != OK )
        return stat;
    stat = taurus_def_nodes(fam_id, *p_mesh_id, node_short_name, 1, numnp);
    if ( stat != OK )
        return stat;

    if ( nel8 > 0 )
    {
        stat = taurus_def_class(fam_id, *p_mesh_id, M_HEX, hex_short_name, hex_long_name);
        if ( stat != OK )
            return stat;
        stat = taurus_def_conn_seq(fam_list[fam_id], *p_mesh_id, hex_short_name, 1, nel8);
        if ( stat != OK )
            return stat;
    }

    if ( nel2 > 0 )
    {
        stat = taurus_def_class(fam_id, *p_mesh_id, M_BEAM, beam_short_name, beam_long_name);
        if ( stat != OK )
            return stat;
        stat = taurus_def_conn_seq(fam_list[fam_id], *p_mesh_id, beam_short_name, 1, nel2);
        if ( stat != OK )
            return stat;
    }

    if ( nel4 > 0 )
    {
        stat = taurus_def_class(fam_id, *p_mesh_id, M_QUAD, shell_short_name, shell_long_name);
        if ( stat != OK )
            return stat;
        stat = taurus_def_conn_seq(fam_list[fam_id], *p_mesh_id, shell_short_name, 1, nel4);
        if ( stat != OK )
            return stat;
    }

    return stat;
}

/*****************************************************************
 * TAG( taurus_load_subrec ) PRIVATE
 */
Return_value taurus_load_subrec(Mili_family *fam, int ctl[], int mesh_id, int *p_srec_id)
{
    int mo_ids[2];
    int qty_svars;
    int activity;
    int it;
    int iu;
    int iv;
    int ia;
    int iresults;
    int ixd;
    int icode;
    int nel2;
    int nel4;
    int nel8;
    int nglbv;
    int nummat2;
    int nummat4;
    int nummat8;
    int numnp;
    int nv1d;
    int nv2d;
    int nv3d;
    int i;
    int nummat;
    Bool_type first;
    Return_value rval = (Return_value)OK;

    if ( ctl[0] == 4 )
    {
        ctl[0] = 3;
    }

    numnp = ctl[1];
    icode = ctl[2];
    nglbv = ctl[3];
    it = ctl[4];
    iu = ctl[5];
    iv = ctl[6];
    ia = ctl[7];
    nel8 = ctl[8];
    nummat8 = ctl[9];
    nv3d = ctl[12];
    nel2 = ctl[13];
    nummat2 = ctl[14];
    nv1d = ctl[15];
    nel4 = ctl[16];
    nummat4 = ctl[17];
    nv2d = ctl[18];
    activity = ctl[20];
    ixd = ctl[24];

    rval = taurus_make_srec(fam, mesh_id, p_srec_id);

    first = TRUE;

    if ( nglbv > 0 )
    {
        if ( activity != 2000 ) /* ... nike3d serial number ... */
                                /* See Nike3d source code nik3di.f */
        {
            int nglbv_avail = nglbv;
            int qty_mtls;

            /*
             * To accommodate DYNA3D hacks of the Taurus db, we must
             * implement a priority scheme in defining subrecords out of
             * the Taurus "global" data:
             *   1. First six will be defined as the true global vars.
             *   2. If #1 is taken care of and more remain, six per material
             *      will be alotted to define material variables, up to as
             *      many materials as are defined by summing nummat8/2/4.
             *   3. If there are slots remaining after #1 and #2 (with all
             *      materials having variables available), the rest will be
             *      alotted as Rigid Wall variables.
             * The DYNA hacks, as described, will cause the nummat sum to
             * increase without providing additional set-of-six material
             * variables in the actual data.  There may still be rigid wall
             * data even if we're shorted on space for material data, but
             * there is _no_ way of resolving this (we only identify that
             * rigid wall data exists if "6 + 6*nummat" is less than nglbv).
             * So, in the case where we're shorted on actual material variables
             * in the data, if there are rigid wall variables they will be
             * mis-identified as material variables in groups of six.  Since
             * the material subrecord is RESULT_ORDERED, this will screw up
             * each material's data, so the analyst will need to know that
             * the material data is incorrect when (1) there are six or more
             * rigid walls _and_ (2) the nummat sum is greater than the number
             * of materials for which data is actually provided.
             */

            if ( nglbv >= 6 )
            {
                mo_ids[0] = 1;
                mo_ids[1] = 1;
                rval = taurus_def_subrec(fam, *p_srec_id, glob_short_name, RESULT_ORDERED, 6, global_primal_shorts,
                                         glob_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
                first = FALSE;

                nglbv_avail -= 6;
            }

            nummat = nummat8 + nummat2 + nummat4;
            if ( nummat > 0 && nglbv > 6 && nglbv_avail > 0 )
            {
                qty_mtls = nglbv_avail / 6;
                if ( qty_mtls > nummat )
                    qty_mtls = nummat;

                mo_ids[0] = 1;
                mo_ids[1] = qty_mtls;
                rval = taurus_def_subrec(fam, *p_srec_id, mat_short_name, RESULT_ORDERED, 4, mat_primal_subrec_shorts,
                                         mat_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
                first = FALSE;

                nglbv_avail -= 6 * qty_mtls;
            }

            if ( nglbv_avail > 0 )
            {
                mo_ids[0] = 1;
                mo_ids[1] = nglbv_avail;
                rval = taurus_def_subrec(fam, *p_srec_id, wall_short_name, OBJECT_ORDERED, 1, wall_primal_shorts,
                                         wall_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
                first = FALSE;

                nglbv_avail = 0;
            }
        }
        else
        {
            mo_ids[0] = 1;
            mo_ids[1] = nglbv;
            rval = taurus_def_subrec(fam, *p_srec_id, nike_short_name, RESULT_ORDERED, 1, nike_primal_shorts,
                                     nike_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
            first = FALSE;
        }
    }

    if ( iu == 1 )
    {
        iresults = iu + iv + ia;
        mo_ids[0] = 1;
        mo_ids[1] = numnp;
        rval = taurus_def_subrec(fam, *p_srec_id, node_short_name, RESULT_ORDERED, iresults, node_primal_shorts,
                                 node_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
        first = FALSE;
    }
    else if ( iv + ia > 0 )
    {
        iresults = iv + ia;
        mo_ids[0] = 1;
        mo_ids[1] = numnp;
        rval = taurus_def_subrec(fam, *p_srec_id, node_short_name, RESULT_ORDERED, iresults, &node_primal_shorts[1],
                                 node_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
        first = FALSE;
    }

    /* Nodal temperatures. */
    if ( it || icode == 1 )
    {
        mo_ids[0] = 1;
        mo_ids[1] = numnp;
        rval = taurus_def_subrec(fam, *p_srec_id, "Temperature", OBJECT_ORDERED, 1, temp_short, node_short_name,
                                 M_BLOCK_OBJ_FMT, 1, mo_ids, first);
        first = FALSE;
    }

    /* Nodal turbulence, a2. */
    if ( ixd )
    {
        mo_ids[0] = 1;
        mo_ids[1] = numnp;

        if ( ixd & A2_MASK )
            rval = taurus_def_subrec(fam, *p_srec_id, "Flow vars", OBJECT_ORDERED, 3, flow_short, node_short_name,
                                     M_BLOCK_OBJ_FMT, 1, mo_ids, first);
        else if ( ixd & K_EPSILON_MASK )
            rval = taurus_def_subrec(fam, *p_srec_id, "Flow vars", OBJECT_ORDERED, 2, flow_short, node_short_name,
                                     M_BLOCK_OBJ_FMT, 1, mo_ids, first);
        first = FALSE;
    }

    /* Brick data. */
    if ( nel8 > 0 )
    {
        if ( nv3d > 0 )
        {
            mo_ids[0] = 1;
            mo_ids[1] = nel8;
            rval = taurus_def_subrec(fam, *p_srec_id, hex_short_name, OBJECT_ORDERED, 2, hex_primal_shorts,
                                     hex_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
            first = FALSE;
        }
    }

    /* Beam data. */
    if ( nel2 > 0 )
    {
        if ( nv1d > 0 )
        {
            mo_ids[0] = 1;
            mo_ids[1] = nel2;
            rval = taurus_def_subrec(fam, *p_srec_id, beam_short_name, OBJECT_ORDERED, 6, beam_primal_shorts,
                                     beam_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
            first = FALSE;
        }
    }

    /* Shell data. */
    if ( nel4 > 0 )
    {
        if ( nv2d > 0 )
        {
            /*
             * For the shell subrecord, the first 12 var's are the same
             * regardless of how many var's are bound.  If the taurus db
             * declares 33 var's, the 13th (and last) var for Mili/libtaurus
             * is "inteng".  If the taurus db declares 45 vars, the 13th
             * for Mili/libtaurus is "inner strain" (vector), the 14th is
             * "outer strain" (vector), and the 15th (and last) is "inteng".
             * Below, we copy the short name pointers into a new array to
             * provide the correct ordering in the subrecord.
             *
             * Not even sure why nv2d = 46 is an option, as neither dyna
             * nor nike use it, but we'll allow it as var "unused_var".
             */
            char *p_shell_shorts[16];

            for ( i = 0; i < 12; i++ )
                p_shell_shorts[i] = shell_primal_shorts[i];

            mo_ids[0] = 1;
            mo_ids[1] = nel4;
            if ( nv2d == 32 )
                qty_svars = 12;
            if ( nv2d == 33 )
            {
                qty_svars = 13;
                p_shell_shorts[12] = shell_primal_shorts[12];
            }
            if ( nv2d > 33 )
            {
                qty_svars = 14;
                p_shell_shorts[12] = shell_primal_shorts[13];
                p_shell_shorts[13] = shell_primal_shorts[14];
            }
            if ( nv2d > 44 )
            {
                qty_svars = 15;
                p_shell_shorts[14] = shell_primal_shorts[12];
            }
            if ( nv2d > 45 )
            {
                qty_svars = 16;
                p_shell_shorts[15] = shell_primal_shorts[15];
            }
            rval = taurus_def_subrec(fam, *p_srec_id, shell_short_name, OBJECT_ORDERED, qty_svars, p_shell_shorts,
                                     shell_short_name, M_BLOCK_OBJ_FMT, 1, mo_ids, first);
            first = FALSE;
        }
    }

    if ( activity >= 1000 && activity <= 1005 )
    {
        /* Brick activity data. */
        if ( nel8 > 0 )
        {
            mo_ids[0] = 1;
            mo_ids[1] = nel8;
            rval = taurus_def_subrec(fam, *p_srec_id, "sand", OBJECT_ORDERED, 1, sand_db_short, hex_short_name,
                                     M_BLOCK_OBJ_FMT, 1, mo_ids, first);
            first = FALSE;
        }

        /* Beam activity data. */
        if ( nel2 > 0 )
        {
            mo_ids[0] = 1;
            mo_ids[1] = nel2;
            rval = taurus_def_subrec(fam, *p_srec_id, "sand", OBJECT_ORDERED, 1, sand_db_short, beam_short_name,
                                     M_BLOCK_OBJ_FMT, 1, mo_ids, first);
            first = FALSE;
        }
        /* Shell activity data. */
        if ( nel4 > 0 )
        {
            mo_ids[0] = 1;
            mo_ids[1] = nel4;
            rval = taurus_def_subrec(fam, *p_srec_id, "sand", OBJECT_ORDERED, 1, sand_db_short, shell_short_name,
                                     M_BLOCK_OBJ_FMT, 1, mo_ids, first);
            first = FALSE;
        }
    }

    rval = taurus_commit_srecs(fam);

    return rval;
}

/*****************************************************************
 * TAG( taurus_load_descriptors ) LOCAL
 *
 * Load descriptive information for state variables,
 * meshes, and state record formats.
 *
 * Should only be called at start-up, but after load_directories().
 */
Return_value taurus_load_descriptors(Mili_family *fam, Famid fam_id, int ctl[], int *p_srec_id)
{
    Return_value stat;
    int p_mesh_id;

    stat = taurus_init_meshes(fam, fam_id, ctl, &p_mesh_id);
    if ( stat != OK )
        return stat;

    stat = taurus_load_svars(fam, ctl);
    if ( stat != OK )
        return stat;

    stat = taurus_load_subrec(fam, ctl, p_mesh_id, p_srec_id);
    if ( stat != OK )
        return stat;

    return stat;
}

/*****************************************************************
 * TAG( taurus_non_state_file_open ) PRIVATE
 *
 * Mili-equivalent non_state_file_open() functionality for Taurus
 * db's.
 */
extern Return_value taurus_non_state_file_open(Mili_family *fam, int index, char mode)
{
    char fname[128];
    int fcnt;
    Return_value rval;
    char access[4];

    make_fnam(NON_STATE_DATA, fam, index, fname);

    rval = OK;

    if ( fam->cur_index != index )
    {
        fcnt = fam->file_count;

        if ( fam->cur_index != -1 )
            non_state_file_close(fam);

        set_file_access(mode, TRUE, access);

        rval = open_buffered(fname, access, &fam->cur_file, &fam->cur_file_size);
        /**/
        /* "next_free_byte" is managed as "cur_file_size" should be, and the two
         * should probably be consolidated, but need to check libtaurus's
         * dependencies on "cur_file_size" first.
         */
        fam->next_free_byte = fam->cur_file_size;

        /* Set current file index and manage directory. */
        if ( rval == OK )
            fam->cur_index = index;
        else
            fam->cur_index = -1;
    }

    return rval;
}

/*****************************************************************
 * TAG( taurus_close ) PUBLIC
 *
 * Close the indicated file family.
 */
Return_value taurus_close(Famid fam_id)
{
    Mili_family *fam;
    Return_value rval;

    rval = validate_fam_id(fam_id);
    if ( rval != OK )
        return rval;

    fam = fam_list[fam_id];

    if ( fam->cur_file != NULL )
    {
        fclose(fam->cur_file);
        fam->cur_file = NULL;
        fam->cur_index = -1;
    }

    if ( fam->cur_st_file != NULL )
    {
        fclose(fam->cur_st_file);
        fam->cur_st_file = NULL;

        fam->cur_st_index = -1;
        fam->cur_st_file_mode = '\0';
    }

    taurus_cleanse(fam);
    free(fam);
    fam_list[fam_id] = NULL;

    rval = taurus_reset_class_data(NULL, 0, TRUE);
    if ( rval != OK )
    {
        return rval;
    }

    /* Call Mili's version too as some queries initialize it.  Needs fixing! */
    reset_class_data(NULL, 0, TRUE);

    return OK;
}

/*****************************************************************
 * TAG( taurus_cleanse ) LOCAL
 *
 * Free all memory associated with a Mili_family struct.
 */
static void taurus_cleanse(Mili_family *fam)
{
    free(fam->root);
    free(fam->path);
    free(fam->file_root);

    if ( fam->state_map != NULL )
        free(fam->state_map);

    if ( fam->directory != NULL )
        delete_dir(fam);

    if ( fam->meshes != NULL )
        taurus_delete_mesh(fam);

    if ( fam->srecs != NULL )
        delete_srecs(fam);

    if ( fam->param_table != NULL )
        htable_delete(fam->param_table, NULL, TRUE);

    if ( fam->svar_table != NULL )
    {
        htable_delete(fam->svar_table, delete_svar, TRUE);
        ios_destroy(fam->svar_c_ios);
        ios_destroy(fam->svar_i_ios);
        fam->svar_hdr = NULL;
    }
}

/*****************************************************************
 * TAG( fix_title )
 *
 * Fixes the screwy title from absolute files.
 */
void fix_title(char *title)
{
    int j, loc;

    for ( loc = 0, j = 0; j <= 5; j++, loc++ )
        title[loc] = title[j];
    for ( j = 8; j <= 13; j++, loc++ )
        title[loc] = title[j];
    for ( j = 16; j <= 21; j++, loc++ )
        title[loc] = title[j];
    for ( j = 24; j <= 29; j++, loc++ )
        title[loc] = title[j];
    for ( j = 32; j <= 37; j++, loc++ )
        title[loc] = title[j];
    for ( j = 40; j <= 45; j++, loc++ )
        title[loc] = title[j];

    while ( isspace(title[loc - 1]) || !isprint(title[loc - 1]) )
        loc--;
    title[loc] = '\0';
}
