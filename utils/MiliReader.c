/* $Id: MiliReader.c,v 1.19 2012/10/23 17:55:54 corey3 Exp $ */

/*
 * Mili Database Reader
 *
 *  This utility was developed to exercise the various Mili read functions
 *  in the Mili Library (mc_mr*) and to serve as an example of how to
 *  extract the various fields from a Mili Database.
 *
 *************************************************************************
 *
 * Modifications:
 *
 *  I. R. Corey - October 8, 2008: Created
 *
 *  I. R. Corey - July 1, 2009: Enhancements for 9.1 release.
 *
 *  I. R. Corey - March 1, 2010: Modified to read all available svars
 *                               for Shells, Beams and Bricks.
 *
 *  I. R. Corey - October 18, 2012: Modified to read all node class
 *                                  svars.
 *
 *************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "mili.h"
#include "mili_internal.h"
#include "mr.h"

static void dump_result(int dbid, FILE *fp, int state, char *svar_name, char *class_name, float *result, int qty_result,
                        int result_veclen, char **field_names, int block_qty, int *block_list);
static void scan_args(int argc, char *argv[], int last_state);
static void usage(void);

extern Mili_family **fam_list;

static char fname_in[M_MAX_NAME_LEN], fname_out[M_MAX_NAME_LEN], fname_results[M_MAX_NAME_LEN];

static char *result_class = NULL, *result_var = NULL;

static int result_class_set = FALSE, result_var_set = FALSE;
static int start_state = 1, stop_state = 1, single_state = 1;
static int max_states_per_file = 1000000;

int main(int argc, char *argv[])
{
    int i, j, k;
    int dbid = 0;

    /* Mili file variables */
    Mili_family *fam_in;
    
    /* Mili file metadata variables */
    char mili_version[64], host[64], arch[64], timestamp[64];

    /* Node variables */
    int mesh_id, class_id;
    int qty_node;
    int *node_labels;
    float *coords;

    char class_name[M_MAX_NAME_LEN], long_name[M_MAX_NAME_LEN];
    char file_num[64];

    FILE *fp;
    int states_file_output = 0;

    int status;

    /* For each element type: quantity, connectivity, materials, parts,
     * element ids, and element labels */

    int qty_beam = 0, *conn_beam = NULL, *mat_beam = NULL, num_mats_beam, *mat_list_beam, *part_beam = NULL,
        *elemIds_beam = NULL, *labels_beam = NULL;
    int qty_quad = 0, *conn_quad = NULL, *mat_quad = NULL, num_mats_quad, *mat_list_quad, *part_quad = NULL,
        *elemIds_quad = NULL, *labels_quad = NULL;
    int qty_hex = 0, *conn_hex = NULL, *mat_hex = NULL, num_mats_hex, *mat_list_hex, *part_hex = NULL,
        *elemIds_hex = NULL, *labels_hex = NULL;
    
    int qty = 0;

    int total_zones = 0;
    int state, qty_states = 1;

    /* Variables used for labels */
    int block_qty = 0, *block_list;

    /* State record variables */
    int *subrec_qtys;
    int staterec_qty = 0, subrec_count = 0;
    Subrecord *p_subrec;
    int subrec_names_len = 0;

    /* State variable data */
    char svar_name[256];
    State_variable p_sv;
    int field_qty = 0;
    char *field_names[256];

    int subrec_vars_len_node = 0, subrec_vars_len_beam = 0, subrec_vars_len_brick = 0, subrec_vars_len_shell = 0;

    char **subrec_names_node = NULL, **subrec_vars_node = NULL;
    char **subrec_names_beam = NULL, **subrec_vars_beam = NULL;
    char **subrec_names_brick = NULL, **subrec_vars_brick = NULL;
    char **subrec_names_shell = NULL, **subrec_vars_shell = NULL;

    /* Result variables */
    int result_veclen = 0, result_type = 0;
    float *results_flt = NULL; 

    /* Mili parameter variables - constants and non time dependent variables */
    int num_params = 0, num_params_ti = 0;
    char **param_list, **param_list_ti;
    char one_param[M_MAX_NAME_LEN];

    int *param_lens, *param_lens_ti;
    int *param_types, *param_types_ti;

    int conn_count = 0;

    /* Element label variables */
    
    fprintf(stderr, "\n\n\n\n\t Running MiliReader Version: %s\n\n", MILI_VERSION);

    /* Scan the command-line arguments, other initialization. */
    scan_args(argc, argv, 0);

    mc_ti_enable(dbid);

    /* Open the input database */
    status = mc_open(fname_in, NULL, "r", &dbid);
    if ( status != 0 )
    {
        mc_print_error("mc_open", status);
        exit(-1);
    }
    fam_in = fam_list[dbid];

    /* Read Mili File Metadata */
    mc_read_string(dbid, "lib version", mili_version);
    mc_read_string(dbid, "host name", host);
    mc_read_string(dbid, "arch name", arch);
    mc_read_string(dbid, "date", timestamp);

    /* Get the number of states */
    status = mc_query_family(dbid, QTY_STATES, NULL, NULL, &qty_states);

    /* Scan the command-line arguments again to look for last state option that
     * requires qty_states.
     */
    scan_args(argc, argv, qty_states);

    /* Get the list for all the non-TI params */
    status = mc_mr_get_param_list(dbid, FALSE, NULL, &num_params, (char **)&param_list);

    param_lens = (int *)malloc(num_params * sizeof(int));
    param_types = (int *)malloc(num_params * sizeof(int));

    /* Get the data attributes for all param variables */
    for ( i = 0; i < num_params; i++ )
    {
        status = mc_mr_get_param_attributes(dbid, param_list[i], FALSE, &param_types[i], &param_lens[i]);
    }

    /* Get the list for all the TI params */
    status = mc_mr_get_param_list(dbid, TRUE, NULL, &num_params_ti, (char **)&param_list_ti);

    param_lens_ti = (int *)malloc(num_params_ti * sizeof(int));
    param_types_ti = (int *)malloc(num_params_ti * sizeof(int));

    /* Get the data attributes for all param variables */
    for ( i = 0; i < num_params_ti; i++ )
    {
        status = mc_mr_get_param_attributes(dbid, param_list_ti[i], TRUE, &param_types_ti[i], &param_lens_ti[i]);
    }

    /* Load node data - coords x,y,z */
    mesh_id = 0;
    class_id = 0;
    status = mc_get_class_info(dbid, mesh_id, M_NODE, class_id, class_name, long_name, &qty_node);

    coords = (float *)malloc(qty_node * 3 * sizeof(float));
    status = mc_load_nodes(dbid, mesh_id, class_name, (void *)coords);

    /* Read the nodal labels */
    node_labels = (int *)malloc(qty_node * sizeof(int));
    status = mc_load_node_labels(dbid, mesh_id, class_name, &block_qty, &block_list, node_labels);

    /* Read the element connectivity */

    /*********/
    /* HEXES */
    /*********/
    conn_count = mc_get_conn_count(M_HEX);
    status = mc_mr_get_geom(dbid, mesh_id, M_HEX, &qty_hex, &conn_hex, &mat_hex, &num_mats_hex, &mat_list_hex,
                            &part_hex, &elemIds_hex, &labels_hex);

    /**********/
    /* SHELLS */
    /**********/
    conn_count = mc_get_conn_count(M_QUAD);
    status = mc_mr_get_geom(dbid, mesh_id, M_QUAD, &qty_quad, &conn_quad, &mat_quad, &num_mats_quad, &mat_list_quad,
                            &part_quad, &elemIds_quad, &labels_quad);

    /*********/
    /* BEAMS */
    /*********/
    conn_count = mc_get_conn_count(M_BEAM);
    status = mc_mr_get_geom(dbid, mesh_id, M_BEAM, &qty_beam, &conn_beam, &mat_beam, &num_mats_beam, &mat_list_beam,
                            &part_beam, &elemIds_beam, &labels_beam);

    /* Example of how to read a String parameter from the TI file */
    /*  Search for part names - "part_title*" */
    status = mc_ti_savedef_class(dbid);
    mc_ti_undef_class(dbid);
    for ( i = 0; i < num_params_ti; i++ )
    {
        if ( param_types_ti[i] == M_STRING && !strncmp(param_list_ti[i], "part_title", 10) )
        {
            status = mc_ti_read_string(dbid, param_list_ti[i], one_param);
        }
    }
    status = mc_ti_restoredef_class(dbid);

    total_zones = qty_hex + qty_quad + qty_beam;

    /* Get state record format count for this database. */
    status = mc_query_family(dbid, QTY_SREC_FMTS, NULL, NULL, (void *)&staterec_qty);

    subrec_qtys = (int *)malloc(staterec_qty * sizeof(int));
    for ( i = 0; i < staterec_qty; i++ )
    {
        /* Get subrecord count for this state record. */
        status = mc_query_family(dbid, QTY_SUBRECS, (void *)&i, NULL, (void *)&subrec_qtys[i]);
        subrec_count += subrec_qtys[i];
    }

    /* Load the Subrecord definitions into an array of Subrecords */
    p_subrec = (Subrecord *)malloc(subrec_count * sizeof(Subrecord));

    k = 0;
    for ( i = 0; i < staterec_qty; i++ )
    {
        for ( j = 0; j < subrec_qtys[i]; j++ )
        {
            /* Get binding */
            status = mc_get_subrec_def(dbid, i, j, &p_subrec[k++]);
        }
    }
    free(subrec_qtys);

    /* Dump the Subrecord definitions (TOC) to a file */

    /* Set a default output file name */
    strcpy(fname_out, fname_in);
    strcat(fname_out, "-OUTPUT");
    printf("\nDumping table of contents (Subrecords) to file [%s-SUBRECS]\n", fname_out);
    status = mc_mr_dump_subrecs(dbid, fname_out, subrec_count, p_subrec);

    /* Get the subrecord list for all brick element types */

    strcpy(class_name, "node");

    status =
        mc_mr_get_subrec_list(dbid, subrec_count, p_subrec, class_name, &subrec_names_len, (char **)&subrec_names_node);

    if ( subrec_names_len > 0 )
    {
        status = mc_mr_get_subrec_vars(dbid, subrec_count, p_subrec, "node", subrec_names_node[0],
                                       &subrec_vars_len_node, (char **)&subrec_vars_node);
    }

    strcpy(class_name, "brick");

    status = mc_mr_get_subrec_list(dbid, subrec_count, p_subrec, class_name, &subrec_names_len,
                                   (char **)&subrec_names_brick);

    if ( subrec_names_len > 0 )
    {
        status = mc_mr_get_subrec_vars(dbid, subrec_count, p_subrec, "brick", subrec_names_brick[0],
                                       &subrec_vars_len_brick, (char **)&subrec_vars_brick);
    }

    /* Get the subrecord list for all shell element types */

    strcpy(class_name, "shell");

    status = mc_mr_get_subrec_list(dbid, subrec_count, p_subrec, class_name, &subrec_names_len,
                                   (char **)&subrec_names_shell);

    if ( subrec_names_len > 0 )
    {
        status = mc_mr_get_subrec_vars(dbid, subrec_count, p_subrec, "shell", subrec_names_shell[0],
                                       &subrec_vars_len_shell, (char **)&subrec_vars_shell);
    }

    /* Get the subrecord list for all beam element types */

    strcpy(class_name, "beam");

    status =
        mc_mr_get_subrec_list(dbid, subrec_count, p_subrec, class_name, &subrec_names_len, (char **)&subrec_names_beam);

    if ( subrec_names_len > 0 )
    {
        status = mc_mr_get_subrec_vars(dbid, subrec_count, p_subrec, "beam", subrec_names_beam[0],
                                       &subrec_vars_len_beam, (char **)&subrec_vars_beam);
    }

    /***********************************************
     * Read results per state and write to a file
     *    - from start_state to stop_state
     ***********************************************/

    if ( stop_state < 0 )
    {
        stop_state = qty_states;
    }

    strcpy(fname_results, fname_out);
    strcat(fname_results, "-RESULTS.");
    sprintf(file_num, "%d", start_state);
    strcat(fname_results, file_num);
    printf("\nDumping Results to file [%s]", fname_results);

    fp = fopen(fname_results, "w+");

    for ( i = start_state - 1; i < stop_state; i++ )
    {
        state = i + 1; /* State numbers are 1 based */

        /************************************/
        /* Read in all of the Nodal results */
        /************************************/
        if ( qty_node > 0 && !result_class_set )
        {
            for ( k = 0; k < subrec_vars_len_node; k++ )
            {
                strcpy(class_name, "node");
                strcpy(svar_name, subrec_vars_node[k]);

                /* Get a list of the variable components if the variable
                 * has components */
                status = mc_get_svar_def(dbid, svar_name, &p_sv);
                field_qty = 0;
                if ( p_sv.vec_size > 1 )
                {
                    field_qty = p_sv.vec_size;
                    for ( j = 0; j < p_sv.vec_size; j++ )
                    {
                        field_names[j] = strdup(p_sv.components[j]);
                    }
                }

                status = mc_mr_get_result(dbid, state, subrec_count, p_subrec, class_name, svar_name, &qty,
                                          &result_veclen, &result_type, &block_qty, &block_list, (void **)&results_flt);
                if ( status == OK && qty > 0 )
                {
                    printf("\n\tDumping Class=%s \tSvar=%s   \tat State %d", class_name, svar_name, state);
                    dump_result(dbid, fp, state, svar_name, class_name, results_flt, qty, result_veclen, field_names,
                                block_qty, block_list);

                    free(results_flt);

                    /* Deallocate the space for the svar fields */
                    if ( field_qty > 0 )
                    {
                        for ( j = 0; j < field_qty; j++ )
                        {
                            free(field_names[j]);
                            field_names[j] = NULL;
                        }
                    }
                }
            }
        }

        /************************************/
        /* Read in all of the Shell results */
        /************************************/
        if ( qty_quad > 0 && !result_class_set )
        {
            for ( k = 0; k < subrec_vars_len_shell; k++ )
            {
                strcpy(class_name, "shell");
                strcpy(svar_name, subrec_vars_shell[k]);

                /* Get a list of the variable components if the variable
                 * has components */
                status = mc_get_svar_def(dbid, svar_name, &p_sv);
                field_qty = 0;
                if ( p_sv.vec_size > 1 )
                {
                    field_qty = p_sv.vec_size;
                    for ( j = 0; j < p_sv.vec_size; j++ )
                    {
                        field_names[j] = strdup(p_sv.components[j]);
                    }
                }

                status = mc_mr_get_result(dbid, state, subrec_count, p_subrec, class_name, svar_name, &qty,
                                          &result_veclen, &result_type, &block_qty, &block_list, (void **)&results_flt);
                if ( status == OK && qty > 0 )
                {
                    printf("\n\tDumping Class=%s \tSvar=%s   \tat State %d", class_name, svar_name, state);
                    dump_result(dbid, fp, state, svar_name, class_name, results_flt, qty, result_veclen, field_names,
                                block_qty, block_list);

                    free(results_flt);

                    /* Deallocate the space for the svar fields */
                    if ( field_qty > 0 )
                    {
                        for ( j = 0; j < field_qty; j++ )
                        {
                            free(field_names[j]);
                            field_names[j] = NULL;
                        }
                    }
                }
            }
        }

        /************************************/
        /* Read in all of the Beam results */
        /************************************/
        if ( qty_beam > 0 && !result_class_set )
        {
            for ( k = 0; k < subrec_vars_len_beam; k++ )
            {
                strcpy(class_name, "beam");
                strcpy(svar_name, subrec_vars_beam[k]);

                /* Get a list of the variable components if the variable
                 * has components */
                status = mc_get_svar_def(dbid, svar_name, &p_sv);
                field_qty = 0;
                if ( p_sv.vec_size > 1 )
                {
                    field_qty = p_sv.vec_size;
                    for ( j = 0; j < p_sv.vec_size; j++ )
                    {
                        field_names[j] = strdup(p_sv.components[j]);
                    }
                }

                status = mc_mr_get_result(dbid, state, subrec_count, p_subrec, class_name, svar_name, &qty,
                                          &result_veclen, &result_type, &block_qty, &block_list, (void **)&results_flt);
                if ( status == OK && qty > 0 )
                {
                    printf("\n\tDumping Class=%s \tSvar=%s   \tat State %d", class_name, svar_name, state);
                    dump_result(dbid, fp, state, svar_name, class_name, results_flt, qty, result_veclen, field_names,
                                block_qty, block_list);

                    free(results_flt);

                    /* Deallocate the space for the svar fields */
                    if ( field_qty > 0 )
                    {
                        for ( j = 0; j < field_qty; j++ )
                        {
                            free(field_names[j]);
                            field_names[j] = NULL;
                        }
                    }
                }
            }
        }

        /************************************/
        /* Read in all of the Brick results */
        /************************************/
        if ( qty_hex > 0 && !result_class_set )
        {
            for ( k = 0; k < subrec_vars_len_brick; k++ )
            {
                strcpy(class_name, "brick");
                strcpy(svar_name, subrec_vars_brick[k]);

                /* Get a list of the variable components if the variable
                 * has components */
                status = mc_get_svar_def(dbid, svar_name, &p_sv);
                field_qty = 0;
                if ( p_sv.vec_size > 1 )
                {
                    field_qty = p_sv.vec_size;
                    for ( j = 0; j < p_sv.vec_size; j++ )
                    {
                        field_names[j] = strdup(p_sv.components[j]);
                    }
                }

                status = mc_mr_get_result(dbid, state, subrec_count, p_subrec, class_name, svar_name, &qty,
                                          &result_veclen, &result_type, &block_qty, &block_list, (void **)&results_flt);
                if ( status == OK && qty > 0 )
                {
                    printf("\n\tDumping Class=%s \tSvar=%s   \tat State %d", class_name, svar_name, state);
                    dump_result(dbid, fp, state, svar_name, class_name, results_flt, qty, result_veclen, field_names,
                                block_qty, block_list);

                    free(results_flt);

                    /* Deallocate the space for the svar fields */
                    if ( field_qty > 0 )
                    {
                        for ( j = 0; j < field_qty; j++ )
                        {
                            free(field_names[j]);
                            field_names[j] = NULL;
                        }
                    }
                }
            }
        }

        /* Read and output a user requested result */
        if ( result_class_set && result_var_set )
        {
            strcpy(class_name, result_class);
            strcpy(svar_name, result_var);

            /* Get a list of the variable components if the variable has
             * components */
            status = mc_get_svar_def(dbid, svar_name, &p_sv);
            field_qty = 0;
            if ( p_sv.vec_size > 1 )
            {
                field_qty = p_sv.vec_size;
                for ( j = 0; j < p_sv.vec_size; j++ )
                {
                    field_names[j] = strdup(p_sv.components[j]);
                }
            }
            status = mc_mr_get_result(dbid, state, subrec_count, p_subrec, class_name, svar_name, &qty, &result_veclen,
                                      &result_type, &block_qty, &block_list, (void **)&results_flt);
            if ( status == OK && qty > 0 )
            {
                printf("\n\tDumping Class=%s \tSvar=%s   \tat State %d", class_name, svar_name, state);
                dump_result(dbid, fp, state, svar_name, class_name, results_flt, qty, result_veclen, field_names,
                            block_qty, block_list);

                free(results_flt);

                /* Deallocate the space for the svar fields */
                if ( field_qty > 0 )
                {
                    for ( j = 0; j < p_sv.vec_size; j++ )
                    {
                        free(field_names[j]);
                        field_names[j] = NULL;
                    }
                }
            }
        }

        /* Check to see if we need to write a new Result file */
        states_file_output++;

        if ( states_file_output >= max_states_per_file && i < stop_state - 1 )
        {
            states_file_output = 0;
            fclose(fp);
            strcpy(fname_results, fname_out);
            strcat(fname_results, "-RESULTS.");
            sprintf(file_num, "%d", i + 2);
            strcat(fname_results, file_num);
            printf("\n\nDumping Results to file [%s]", fname_results);

            fp = fopen(fname_results, "w+");
        }
        printf("\n");
    } /* End for on state */

    fclose(fp);
    status = mc_close(dbid);
    if ( status != 0 )
    {
        mc_print_error("mc_close", status);
        exit(-1);
    }

    printf("\n\n");

    return OK;
}

/************************************************************
 * TAG( dump_result ) LOCAL
 *
 * Writes result data to a specified file.
 */
static void dump_result(int dbid, FILE *fp, int state, char *svar_name, char *class_name, float *result, int qty_result,
                        int result_veclen, char **field_names, int block_qty, int *block_list)
{
    int i, j, k;
    int elem_id = 0;
    int obj_index, block_index;
    int result_len = 0;
    float result_flt = 0.0;

    char field_name[256];

    float time = 0.0;
    int status = OK;

    /* Get the State Time */

    status = mc_query_family(dbid, STATE_TIME, (void *)&state, NULL, (void *)&time);

    fprintf(fp, "\n\tState=[%d] @ Time=[%14.10f]", state, time);

    fprintf(fp, "\n\tResult_Class=[%s]  Result_Name=[%s]  Vec_Len=[%d]", class_name, svar_name, result_veclen);

    obj_index = 0;
    block_index = 0;

    for ( i = 0; i < block_qty; i++ )
    {
        fprintf(fp, "\n\n\tBlock Length = [%d] Block ids = [%d:%d] ",
                (block_list[block_index + 1] - block_list[block_index]) + 1, block_list[block_index],
                block_list[block_index + 1]);

        result_len = (block_list[block_index + 1] - block_list[block_index] + 1) * result_veclen;

        elem_id = block_list[block_index];
        obj_index = (block_list[block_index] - 1) * result_veclen;
        block_index += 2;

        for ( j = 0; j < result_len; j += result_veclen )
        {
            fprintf(fp, "\n\t[%d]\t ", elem_id++);
            for ( k = 0; k < result_veclen; k++ )
            {
                result_flt = (float)result[obj_index];

                if ( result_veclen > 1 )
                {
                    if ( field_names )
                    {
                        sprintf(field_name, "%d", k + 1);
                        if ( field_names[k] )
                        {
                            strcpy(field_name, field_names[k]);
                        }
                        fprintf(fp, " \tComponent=[%s] %25.18e ", field_name, result_flt);
                    }
                }
                else
                {
                    fprintf(fp, " \t%25.18e ", result_flt);
                }
                obj_index++;
            }
        }
    }
    fprintf(fp, "\n\n");
}

/************************************************************
 * TAG( scan_args ) LOCAL
 *
 * Parse the reader program's command line arguments.
 */
static void scan_args(int argc, char *argv[], int last_state)
{
    int i;
    int inname_set = FALSE;

    start_state = 1;
    stop_state = -1;

    /* */
    for ( i = 1; i < argc; i++ )
    {
        if ( strcmp(argv[i], "-state") == 0 )
        {
            i++;

            if ( !strcmp(argv[i], "last") || !strcmp(argv[i], "LAST") )
            {
                single_state = last_state;
            }
            else
            {
                single_state = atoi(argv[i]);
            }
            start_state = single_state;
            stop_state = single_state;
        }

        if ( strcmp(argv[i], "-start") == 0 )
        {
            i++;
            start_state = atoi(argv[i]);
        }

        if ( strcmp(argv[i], "-states-file") == 0 )
        {
            i++;
            max_states_per_file = atoi(argv[i]);
        }

        if ( strcmp(argv[i], "-stop") == 0 )
        {
            i++;
            if ( !strcmp(argv[i], "last") || !strcmp(argv[i], "LAST") )
            {
                stop_state = last_state;
            }
            else
            {
                stop_state = atoi(argv[i]);
            }
        }

        if ( strcmp(argv[i], "-i") == 0 )
        {
            i++;
            strcpy(fname_in, argv[i]);
            inname_set = TRUE;
        }

        if ( strcmp(argv[i], "-result-class") == 0 )
        {
            i++;
            result_class = strdup(argv[i]);
            result_class_set = TRUE;
        }

        if ( strcmp(argv[i], "-result-var") == 0 )
        {
            i++;
            result_var = strdup(argv[i]);
            result_var_set = TRUE;
        }

        if ( strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-o") == 0 )
        {
            i++;
            strcpy(fname_out, argv[i]);
        }
    }

    if ( !inname_set || argc == 1 )
    {
        usage();
        exit(1);
    }
}

/************************************************************
 * TAG( usage ) LOCAL
 *
 * Write out command-line syntax.
 */
static void usage(void)
{
    printf("\n");
    printf("Usage: milireader -i <input_base_name>\n");
    printf("\n");
    printf("OPTIONS:\n");
    printf("                [-d | -o]           \t<dumpfile name>                    \n");
    printf("\n");
    printf("                [-states-file n]    \t<states to output per file>\n");
    printf("\n");
    printf("                [-state n]          \t<read a single state n | 'last'>    \n");
    printf("\n");
    printf("                [-start n]           \t<start state n>                    \n");
    printf("                [-stop n]            \t<stop state  n | 'last' >          \n");
    printf("\n");
    printf("                [-result-class class] \t<user selected result class>      \n");
    printf("                [-result-var var]     \t<user selected result variable>   \n");
    printf("\n");
}

/* End of MiliReader.c */
