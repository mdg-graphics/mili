/* $Id $ */

/*
 * tipart: TI Partition file loader utility
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - August 16, 2007: Added field for TI vars to note if
 *                nodal or element result.
 *
 ************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include "mili.h"
#include "mili_internal.h"
#include "partition.h"
#include <sys/stat.h>

extern Mili_family **fam_list;

/* Prototypes */
static void usage();
static void create_mili_filename(int nproc, int procnum, char *mili_file_by_processor, char *rootname);

static char mili_file[128];

int main(int argc, char *argv[])
{
    int i;
    int matid = 0, meshid = 1;
    int state = 0;

    Famid fid;

    int dims[3];
    int num_entries, tsize;

    Mili_family *fam;

    char **wildcard_list = NULL;

    /* Metadata fields */

    char mili_version[32], host[64], arch[32], timestamp[64], ti_user[32], xmilics_version[64], code_name[64];

    char ti_mili_version[32], ti_host[64], ti_arch[32], ti_timestamp[64];

    /* Offsets into local to global mapping array */
    int offset_plt = 0;
    int offset_elhlt = 0;
    int offset_elblt = 0;
    int offset_elslt = 0;
    int offset_eldlt = 0;

    char var_name[64];

    Partition_mili *map;
    char partition_file[128], mili_file_by_processor[128];

    int status;

    int ti_int;

    if ( argc < 2 )
    {
        usage();
    }

    /* Parse the command line options */

    strcpy(mili_file, "PART");
    for ( i = 0; i < argc; i++ )
    {
        if ( !strcmp("-i", argv[i]) )
        {
            strcpy(partition_file, argv[++i]);
        }

        if ( !strcmp("-o", argv[i]) )
        {
            strcpy(mili_file, argv[++i]);
        }
    }

    /* Disable all file locking */
    mc_filelock_disable();

    /*
     * Read in the partition file
     */
    map = (Partition_mili *)malloc(1 * sizeof(Partition_mili));
    status = read_partition_mili(partition_file, &map);

    for ( i = 0; i < map->nproc; i++ )
    {
        /* Create a processor specific mili filename */

        create_mili_filename(map->nproc, i, mili_file_by_processor, mili_file);

        /*
         * Create the database.
         */

        /* Enable TI functions */
        fid = mc_get_next_fid();

        mc_ti_enable(fid);

        status = mc_open(mili_file_by_processor, "", "AwPdEn", &fid);
        if ( status != 0 )
        {
            mc_print_error("mc_open", status);
            exit(-1);
        }

        fam = fam_list[fid];

        /* Only write out TI files */
        mc_ti_enable_only(fid);

        status = mc_ti_def_class(fid, meshid, i, matid, "M_NODE", TRUE, TRUE, "Node", "Node");
        if ( status != 0 )
        {
            mc_close(fid);
            mc_print_error("mc_def_ti_class", status);
            exit(-1);
        }

        status = mc_ti_wrt_scalar(fid, M_INT, "nproc", &map->nproc);
        if ( status != 0 )
        {
            mc_print_error("mc_ti_wrt_scalar", status);
        }

        /* Write out the global problem parameters */
        status = mc_ti_wrt_scalar(fid, M_INT, "numnp_global", &map->nnpg);
        status = mc_ti_wrt_scalar(fid, M_INT, "nummat_global", &map->nummat);
        status = mc_ti_wrt_scalar(fid, M_INT, "numnp_global", &map->nnpg);
        status = mc_ti_wrt_scalar(fid, M_INT, "numelh_global", &map->nelhg);
        status = mc_ti_wrt_scalar(fid, M_INT, "numelb_global", &map->nelbg);
        status = mc_ti_wrt_scalar(fid, M_INT, "numels_global", &map->nelsg);
        status = mc_ti_wrt_scalar(fid, M_INT, "numelt_global", &map->neltg);
        status = mc_ti_wrt_scalar(fid, M_INT, "numeld_global", &map->neldg);

        /* Write out the local problem parameters */
        status = mc_ti_wrt_scalar(fid, M_INT, "numnp_local", &map->nnp[i]);
        status = mc_ti_wrt_scalar(fid, M_INT, "numelh_local", &map->nelh[i]);
        status = mc_ti_wrt_scalar(fid, M_INT, "numelb_local", &map->nelb[i]);
        status = mc_ti_wrt_scalar(fid, M_INT, "numels_local", &map->nels[i]);
        status = mc_ti_wrt_scalar(fid, M_INT, "numelt_local", &map->nelt[i]);
        status = mc_ti_wrt_scalar(fid, M_INT, "numeld_local", &map->neld[i]);

        /* Write out the global to local mappings */

        status = mc_ti_wrt_scalar(fid, M_INT, "npltg_map", &map->npltg[i]);
        status = mc_ti_wrt_scalar(fid, M_INT, "elhltg_map", &map->elhltg[i]);

        /* Nodal Point Local-to-Global Map */
        dims[0] = map->nnp[i];
        dims[1] = 0;
        dims[2] = 0;

        strcpy(var_name, "Nodal Points");
        status = mc_ti_wrt_array(fid, M_INT, (char *)var_name, 1, dims, (void *)&map->npltg[offset_plt]);

        /* Hexahedral elements assigned to each processor */
        dims[0] = map->nelh[i];
        strcpy(var_name, "Hexahedral elements");
        status = mc_ti_wrt_array(fid, M_INT, (char *)var_name, 1, dims, (void *)&map->elhltg[offset_elhlt]);

        /* Beam elements assigned to each processor */
        dims[0] = map->nelb[i];
        strcpy(var_name, "Beam elements");
        status = mc_ti_wrt_array(fid, M_INT, (char *)var_name, 1, dims, (void *)&map->elbltg[offset_elblt]);

        /* Shell elements assigned to each processor */
        dims[0] = map->nels[i];
        strcpy(var_name, "Shell elements");
        status = mc_ti_wrt_array(fid, M_INT, (char *)var_name, 1, dims, (void *)&map->elsltg[offset_elslt]);

        /* Thick shell elements assigned to each processor */
        dims[0] = map->nelt[i];
        strcpy(var_name, "Thick shell elements");
        status = mc_ti_wrt_array(fid, M_INT, (char *)var_name, 1, dims, (void *)&map->elhltg[offset_elhlt]);

        /* Update offsets into mapping arrays */
        offset_plt += map->nnp[i];
        offset_elhlt += map->nelh[i];
        offset_elblt += map->nelb[i];
        offset_elslt += map->nels[i];
        offset_eldlt += map->neld[i];
        offset_elhlt += map->nelt[i];

        /* Check for variables in the ti hash table */

        tsize = fam->ti_param_table->size;

        wildcard_list = (char **)malloc(tsize * sizeof(char *));
        num_entries =
            htable_search_wildcard(fam->ti_param_table, 0, FALSE, "Nodal Points", "NULL", "NULL", wildcard_list);

        num_entries = htable_search_wildcard(fam->ti_param_table, 0, FALSE, "M_NODE", "NULL", "NULL", wildcard_list);

        htable_delete_wildcard_list(num_entries, wildcard_list);

        mc_close(fid);
    }

    /* DEBUG Code
     * Test read of the output TI file.
     */

#ifdef DEBUG
    printf("\n\nBegin TI Test Reads");

    fid = mc_get_next_fid();

    mc_ti_enable(fid);

    create_mili_filename(map->nproc, 0, mili_file_by_processor, mili_file);

    mc_ti_enable(0);

    status = mc_open(mili_file_by_processor, "", "ArPdEn", &fid);
    if ( status != 0 )
    {
        mc_print_error("mc_open(Read)", status);
        exit(-1);
    }

    status = mc_get_metadata(fid, mili_version, host, arch, timestamp, xmilics_version);
    if ( status != OK )
    {
        exit(1);
    }
    status =
        mc_ti_get_metadata(fid, ti_mili_version, ti_host, ti_arch, ti_timestamp, ti_user, xmilics_version, code_name);
    if ( status != OK )
    {
        exit(1);
    }

    status = mc_ti_check_arch(fid);
    if ( status != OK )
    {
        exit(1);
    }

    /* Define TI class search criteria */
    status = mc_ti_undef_class(fid);
    status = mc_ti_def_class(fid, meshid, state, matid, "M_NODE", TRUE, TRUE, "Node", "Node");

    /* Read an integer scalar */
    status = mc_ti_read_scalar(fid, "nproc", &ti_int);
    printf("\n\t nproc = \t%d", ti_int);

    status = mc_ti_read_scalar(fid, "numnp_global", &ti_int);
    printf("\n\t numnp_global = \t%d", ti_int);

#endif

    return 0;
}

/*****************************************************************
 * TAG( usage ) LOCAL
 *
 * Print usage line and exit.
 */
static void usage()
{
    printf("\nUsage: tipart -i partition_file_name -o mili_database_name\n");
    exit(1);
}

/*****************************************************************
 * TAG( create_mili_filename ) LOCAL
 *
 * Creates a Mili filename with the correct processor number.
 */
static void create_mili_filename(int nproc, int procnum, char *mili_file_by_processor, char *rootname)
{
    char proc_number[16];

    strcpy(mili_file_by_processor, mili_file);

    if ( nproc > 9999 )
    {
        sprintf(proc_number, "%5.5d", procnum);
    }
    else if ( nproc > 999 )
    {
        sprintf(proc_number, "%4.4d", procnum);
    }
    else
    {
        sprintf(proc_number, "%3.3d", procnum);
    }

    strcat(mili_file_by_processor, proc_number);
}
