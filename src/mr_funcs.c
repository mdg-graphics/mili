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

 * Mili Reader Funcions
 *
 ************************************************************************
 * Modifications:
 *  I. R. Corey - October 8, 2008: Created
 *
 *  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
 *                with Visit.
 *                See SCR #587.
 *
 *  I. R. Corey - July 10, 2009: Cleaned-up and added element ids to
 *                get geom function. Also added a function to calculate
 *                the result length. Renamed functions from
 *                mr_ -> mc_mr_.
 *
 *  I. R. Corey - February 26, 2010: Fixed problem with mc_mr_get_result
 *                related to reading TH data and milti-dim fields..
 *
 *  I. R. Corey - October 18, 2012: Fixed problem with computing result
 *                length for TH data. Removed duplicates from svar list.
 *
 ************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "mili.h"
#include "mr.h"
#include "mili_internal.h"

extern Mili_family **fam_list;

/************************************************************
 * TAG( mc_mr_get_geom )
 *
 * Get the geometry for a specified element type.
 */
Return_value mc_mr_get_geom(int dbid, int mesh_id, int superclass, int *qty_elems, int **conn, int **mat, int *num_mats,
                            int **mat_list, int **part, int **elem_ids, int **labels)
{
    int i, j;
    int int_args[2];
    int *conn_ptr = NULL, *mat_ptr = NULL, *part_ptr = NULL, *elems_ptr = NULL, *labels_ptr = NULL;
    int block_qty = 0, *block_range = NULL;
    int obj_index, qty_object = 0, obj_qty = 0;
    int qty_classes = 0;
    int conn_ct;
    char short_name[M_MAX_NAME_LEN], long_name[M_MAX_NAME_LEN];

    Return_value status = OK;

    int_args[0] = mesh_id;
    int_args[1] = superclass;
    status = mc_query_family(dbid, QTY_CLASS_IN_SCLASS, (void *)int_args, NULL, (void *)&qty_classes);
    if ( status != OK )
    {
        return status;
    }
    qty_object = 0;
    for ( i = 0; i < qty_classes; i++ )
    {
        status = mc_get_class_info(dbid, mesh_id, superclass, i, short_name, long_name, &obj_qty);
        if ( status != OK )
        {
            return status;
        }
        qty_object += obj_qty;
    }
    conn_ct = mc_get_conn_count(superclass);

    *conn = (int *)malloc(qty_object * conn_ct * sizeof(int));
    if ( qty_object > 0 && *conn == NULL )
    {
        return ALLOC_FAILED;
    }
    *mat = (int *)malloc(qty_object * sizeof(int));
    if ( qty_object > 0 && *mat == NULL )
    {
        free(*conn);
        return ALLOC_FAILED;
    }
    *part = (int *)malloc(qty_object * sizeof(int));
    if ( qty_object > 0 && *part == NULL )
    {
        free(*conn);
        free(*mat);
        return ALLOC_FAILED;
    }
    *elem_ids = (int *)malloc(qty_object * sizeof(int));
    if ( qty_object > 0 && *elem_ids == NULL )
    {
        free(*conn);
        free(*mat);
        free(*part);
        return ALLOC_FAILED;
    }
    *labels = (int *)malloc(qty_object * sizeof(int));
    if ( qty_object > 0 && *labels == NULL )
    {
        free(*conn);
        free(*mat);
        free(*part);
        free(*elem_ids);
        return ALLOC_FAILED;
    }

    conn_ptr = *conn;
    mat_ptr = *mat;
    part_ptr = *part, labels_ptr = *labels;
    elems_ptr = *elem_ids;

    obj_index = 0;
    for ( i = 0; i < qty_classes; i++ )
    {
        status = mc_get_class_info(dbid, mesh_id, superclass, i, short_name, long_name, &obj_qty);
        if ( status != OK )
        {
            return status;
        }

        status = mc_load_conns(dbid, mesh_id, short_name, conn_ptr + conn_ct * obj_index, mat_ptr + obj_index,
                               part_ptr + obj_index);
        if ( status != OK )
        {
            return status;
        }

        status = mc_load_conn_labels(dbid, mesh_id, short_name, obj_qty, &block_qty, &block_range,
                                     elems_ptr + obj_index, labels_ptr + obj_index);
        if ( status != OK )
        {
            return status;
        }
        obj_index += obj_qty;
    }

    *qty_elems = qty_object;

    return status;
}

Return_value mc_mr_get_subrecs(int fid, int *subrec_total, Subrecord **p_subr)
{
    Subrecord *p_subrec;
    int srec_qty = 0;
    int *subrec_qty = NULL;
    int i, j;
    Return_value stat;

    stat = mc_query_family(fid, QTY_SREC_FMTS, NULL, NULL, (void *)&srec_qty);
    if ( stat != 0 )
    {
        mc_print_error("mc_query_family(QTY_SREC_FMTS)", stat);
        return stat;
    }

    subrec_qty = (int *)malloc(srec_qty * sizeof(int));

    for ( i = 0; i < srec_qty; i++ )
    {
        stat = mc_query_family(fid, QTY_SUBRECS, (void *)&i, NULL, (void *)&subrec_qty[i]);

        if ( stat != 0 )
        {
            mc_print_error("mc_query_family(QTY_SUBRECS)", stat);
            return stat;
        }
        *subrec_total += subrec_qty[i];
    }

    /* Load the Subrecord definitions into an array of Subrecords */
    p_subrec = (Subrecord *)malloc(*subrec_total * sizeof(Subrecord));
    for ( i = 0; i < srec_qty; i++ )
    {
        for ( j = 0; j < subrec_qty[i]; j++ )
        {
            /* Get binding */
            stat = mc_get_subrec_def(fid, i, j, &p_subrec[j]);
            if ( stat != 0 )
            {
                mc_print_error("mc_get_subrec_def", stat);
                free(p_subrec);
                return stat;
            }
        }
    }
    *p_subr = p_subrec;
    free(subrec_qty);
    return OK;
}
/************************************************************
 * TAG( mc_mr_get_result_len )
 *
 * Returns the length of a result for a single data record
 * or all data records in a class if "*" is specified as
 * the data record name. The number of data items is
 * returned as ther length.  If the result is an array or
 * vector the vector fields are counted in the length.
 */
Return_value mc_mr_get_result_len(int dbid, int subrec_qty, Subrecord *p_subr, char *class_name, char *subrec_name,
                                  char *svar_name, int *svar_result_len)
{
    int i, j, k;

    int dims_len = 1, vec_len = 1;
    Subrecord *subrec_ptr;

    State_variable p_sv;

    Return_value status = OK;

    int result_len = 0;

    /* Compute the result length */
    status = mc_get_svar_def(dbid, svar_name, &p_sv);
    if ( status != OK )
    {
        return status;
    }

    for ( i = 0; i < subrec_qty; i++ )
    {
        subrec_ptr = &p_subr[i];
        if ( (!strcmp(subrec_ptr->name, subrec_name) || !strcmp("*", subrec_name)) &&
             !strcmp(subrec_ptr->class_name, class_name) )
        {
            for ( j = 0; j < subrec_ptr->qty_svars; j++ )
            {
                if ( !strcmp(subrec_ptr->svar_names[j], svar_name) )
                {
                    if ( (p_sv.agg_type == ARRAY || p_sv.agg_type == VEC_ARRAY) && p_sv.rank > 0 && p_sv.dims )
                    {
                        for ( k = 0; k < p_sv.rank; k++ )
                        {
                            if ( p_sv.dims[k] > 0 )
                            {
                                dims_len *= p_sv.dims[k];
                            }
                        }
                    }

                    if ( (p_sv.agg_type == VEC_ARRAY || p_sv.agg_type == VECTOR) && p_sv.vec_size > 1 )
                    {
                        vec_len = p_sv.vec_size;
                    }

                    result_len += subrec_ptr->qty_objects * vec_len * dims_len;
                }
            }
        }
    }
    mc_cleanse_st_variable(&p_sv);
    *svar_result_len = result_len;
    return status;
}

/************************************************************
 * TAG( mc_mr_get_result )
 *
 * Read and return a result for a specified class and variable.
 */
Return_value mc_mr_get_result(int dbid, int state, int subrec_qty, Subrecord *p_subrec, char *class_name,
                              char *svar_name, int *len, int *veclen, int *type, int *block_qty, int **block_list,
                              void **result)
{
    int i, j, k;
    int block_cnt = 0, total_blocks = 0, temp_block_cnt = 0, temp_len = 0;

    int elem_id_start = 0, elem_id_stop = 0;

    int result_index = 0;
    float *result_buff_flt = NULL, *temp_result_buff_flt;
    double *result_buff_dbl = NULL, *temp_result_buff_dbl;
    int *result_buff_int = NULL, *temp_result_buff_int;
    LONGLONG *result_buff_long = NULL, *temp_result_buff_long;

    int result_len = 0, result_vec_len = 1;
    void *result_buff, *temp_result_buff;

    char *results[2];
    Bool_type valid_result = FALSE;
    State_variable p_sv;
    Subrecord *subrec_ptr;

    int *temp_block_list, *block_list_ptr;
    int num_names = 0;
    int max_id = 0;

    Return_value status;

    status = mc_get_svar_def(dbid, svar_name, &p_sv);

    if ( status != OK )
    {
        return (status);
    }

    /* Count length of results for all records of this type */

    *block_qty = 0;
    temp_block_cnt = 0;
    *len = 0;

    for ( i = 0; i < subrec_qty; i++ )
    {
        if ( !strcmp(p_subrec[i].class_name, class_name) )
        {
            for ( j = 0; j < p_subrec[i].qty_svars; j++ )
            {
                if ( !strcmp(p_subrec[i].svar_names[j], svar_name) )
                {
                    for ( k = 0; k < p_subrec[i].qty_blocks; k++ )
                    {
                        temp_block_cnt += 2;
                    }
                }
            }
        }
    }

    if ( temp_block_cnt == 0 )
    {
        return (NOT_OK);
    }

    temp_block_list = (int *)malloc(temp_block_cnt * sizeof(int));
    if ( temp_block_cnt > 0 && temp_block_list == NULL )
    {
        return ALLOC_FAILED;
    }

    temp_block_cnt = 0;
    for ( i = 0; i < subrec_qty; i++ )
    {
        if ( !strcmp(p_subrec[i].class_name, class_name) )
        {
            for ( j = 0; j < p_subrec[i].qty_svars; j++ )
            {
                if ( !strcmp(p_subrec[i].svar_names[j], svar_name) )
                {
                    block_cnt = 0;
                    for ( k = 0; k < p_subrec[i].qty_blocks; k++ )
                    {
                        subrec_ptr = &p_subrec[i];

                        temp_block_list[temp_block_cnt] = (int)p_subrec[i].mo_blocks[block_cnt];
                        temp_block_list[temp_block_cnt + 1] = (int)p_subrec[i].mo_blocks[block_cnt + 1];
                        temp_len += (temp_block_list[temp_block_cnt + 1] - temp_block_list[temp_block_cnt]) + 1;
                        if ( temp_block_list[temp_block_cnt + 1] > max_id )
                            max_id = temp_block_list[temp_block_cnt + 1];
                        block_cnt += 2;
                        temp_block_cnt += 2;
                    }
                }
            }
        }
    }

    if ( temp_len == 0 )
    {
        free(temp_block_list);
        return (NOT_OK);
    }

    *len = temp_len;
    total_blocks = temp_block_cnt / 2;

    block_list_ptr = (int *)malloc((temp_block_cnt) * sizeof(int));

    if ( temp_block_cnt > 0 && block_list_ptr == NULL )
    {
        free(temp_block_list);
        return ALLOC_FAILED;
    }

    for ( i = 0; i < temp_block_cnt; i++ )
    {
        block_list_ptr[i] = temp_block_list[i];
    }

    free(temp_block_list);

    status = mc_mr_get_result_len(dbid, subrec_qty, p_subrec, class_name, "*", svar_name, &result_len);
    if ( status != OK )
    {
        free(block_list_ptr);
        return status;
    }

    result_vec_len = result_len / (*len);
    results[0] = strdup(svar_name);
    results[1] = NULL;

    /* Need to use max_id for computing the max result len to
     * account for TH files that may have less results than
     * the max id number.
     */
    result_len = max_id * result_vec_len;

    /* Allocate space for the result by type */

    if ( p_sv.num_type == M_FLOAT )
    {
        result_buff_flt = NEW_N(float, result_len, "Result buffer - flt[mc_mr_get_result[");
        result_buff = (void *)result_buff_flt;
        temp_result_buff_flt = NEW_N(float, result_len, "Temp result buffer - flt[mc_mr_get_result[");
        temp_result_buff = (void *)temp_result_buff_flt;

        for ( i = 0; i < result_len; i++ )
        {
            result_buff_flt[i] = (float)0.0;
            temp_result_buff_flt[i] = (float)0.0;
        }
    }
    else if ( p_sv.num_type == M_FLOAT8 )
    {
        result_buff_dbl = NEW_N(double, result_len, "Result buffer - dbl[mc_mr_get_result[");
        result_buff = (void *)result_buff_dbl;
        temp_result_buff_dbl = NEW_N(double, result_len, "Temp result buffer - flt[mc_mr_get_result[");
        temp_result_buff = (void *)temp_result_buff_dbl;

        for ( i = 0; i < result_len; i++ )
        {
            result_buff_dbl[i] = (double)0.0;
            temp_result_buff_dbl[i] = (double)0.0;
        }
    }
    else if ( p_sv.num_type == M_INT )
    {
        result_buff_int = NEW_N(int, result_len, "Result buffer - int[mc_mr_get_result[");
        result_buff = (void *)result_buff_int;
        temp_result_buff_int = NEW_N(int, result_len, "Temp result buffer - int[mc_mr_get_result[");
        temp_result_buff = (void *)temp_result_buff_int;

        for ( i = 0; i < result_len; i++ )
        {
            result_buff_int[i] = (int)0.0;
            temp_result_buff_int[i] = (int)0.0;
        }
    }
    else if ( p_sv.num_type == M_INT8 )
    {
        result_buff_long = NEW_N(LONGLONG, result_len, "Result buffer - long[mc_mr_get_result[");
        result_buff = (void *)result_buff_long;
        temp_result_buff_long = NEW_N(LONGLONG, result_len, "Temp result buffer - long[mc_mr_get_result[");
        temp_result_buff = (void *)temp_result_buff_long;

        for ( i = 0; i < result_len; i++ )
        {
            result_buff_long[i] = (LONGLONG)0.0;
            temp_result_buff_long[i] = (LONGLONG)0.0;
        }
    }
    else
    {
        free(block_list_ptr);
        return INVALID_DATA_TYPE;
    }
    if ( result_len != 0 && (result_buff == NULL || temp_result_buff == NULL) )
    {
        if ( result_buff != NULL )
        {
            free(result_buff);
        }
        if ( temp_result_buff != NULL )
        {
            free(temp_result_buff);
        }
        free(block_list_ptr);
        return ALLOC_FAILED;
    }

    /* Read the requested result for the specified class */

    for ( i = 0; i < subrec_qty; i++ )
    {
        subrec_ptr = &p_subrec[i];
        status = mc_mr_is_valid_svar(subrec_ptr, svar_name, &valid_result);
        if ( !strcmp(p_subrec[i].class_name, class_name) && valid_result )
        {
            status = mc_read_results(dbid, state, i, 1, results, (void *)temp_result_buff);

            if ( status != OK )
            {
                free(result_buff);
                free(temp_result_buff);
                free(block_list_ptr);
                return status;
            }
            result_index = 0;

            /* Load the result by element id */
            for ( j = 0; j < subrec_ptr->qty_blocks * 2; j++ )
            {
                elem_id_start = subrec_ptr->mo_blocks[j] - 1;
                elem_id_stop = subrec_ptr->mo_blocks[j + 1];
                j++;

                for ( k = elem_id_start * result_vec_len; k < elem_id_stop * result_vec_len; k++ )
                {
                    switch ( p_sv.num_type )
                    {
                        case M_INT:
                            result_buff_int[k] = temp_result_buff_int[result_index];
                            break;
                        case M_INT8:
                            result_buff_long[k] = temp_result_buff_long[result_index];
                            break;
                        case M_FLOAT:
                            result_buff_flt[k] = temp_result_buff_flt[result_index];
                            break;
                        case M_FLOAT8:
                            result_buff_dbl[k] = temp_result_buff_dbl[result_index];
                            break;
                    }
                    result_index++;
                }
            }
        }
    }

    *type = p_sv.num_type;
    *veclen = result_vec_len;
    *result = result_buff;
    *block_qty = total_blocks;
    *block_list = block_list_ptr;

    if ( temp_result_buff )
    {
        free(temp_result_buff);
    }
    free(results[0]);
    mc_cleanse_st_variable(&p_sv);
    return (OK);
}

/************************************************************
 * TAG( mc_mr_get_subrec_list )
 *
 * Returns a list of subrecord names for a specified class
 * type.
 */
Return_value mc_mr_get_subrec_list(int dbid, int subrec_qty, Subrecord *p_subrec, char *class_name,
                                   int *subrec_names_len, char **subrec_names)
{
    int i, j;
    int list_index = 0, name_index = 0;
    int qty_names = 0;
    char **temp_names;
    Bool_type name_found;
    *subrec_names_len = 0;

    /* Count number of Subrecord vars */
    for ( i = 0; i < subrec_qty; i++ )
    {
        if ( !strcmp(p_subrec[i].class_name, class_name) )
        {
            (*subrec_names_len)++;
        }
    }
    temp_names = (char **)malloc(*subrec_names_len * sizeof(char *));
    if ( *subrec_names_len > 0 && temp_names == NULL )
    {
        return ALLOC_FAILED;
    }

    for ( i = 0; i < subrec_qty; i++ )
    {
        name_found = FALSE;

        if ( !strcmp(p_subrec[i].class_name, class_name) )
        {
            /* Make sure that we are not creating duplicate svar entries */
            for ( j = 0; j < name_index; j++ )
                if ( !strcmp(temp_names[j], p_subrec[i].name) )
                {
                    name_found = TRUE;
                    break;
                }
            if ( !name_found )
            {
                temp_names[name_index++] = strdup(p_subrec[i].name);
            }
        }
    }

    if ( *subrec_names_len > 0 )
    {
        *subrec_names = (char *)temp_names;
    }
    else
    {
        *subrec_names = (char *)NULL;
    }
    return OK;
}

/************************************************************
 * TAG( mc_mr_get_subrec_vars )
 *
 * Returns a list of state variables for a specified record
 * name.
 */
Return_value mc_mr_get_subrec_vars(int dbid, int subrec_qty, Subrecord *p_subrec, char *class_name, char *subrec_name,
                                   int *subrec_vars_len, char **subrec_vars)
{
    int i, j, k;
    int var_index = 0;
    char temp_var_name[128];
    char **temp_vars;

    Bool_type name_found;
    *subrec_vars_len = 0;

    /* Count number of Subrecord vars */
    for ( i = 0; i < subrec_qty; i++ )
    {
        if ( !strcmp(p_subrec[i].class_name, class_name) )
        {
            strcpy(temp_var_name, p_subrec[i].name);
            (*subrec_vars_len) += p_subrec[i].qty_svars;
        }
    }
    temp_vars = (char **)malloc(*subrec_vars_len * sizeof(char *));
    if ( *subrec_vars_len > 0 && temp_vars == NULL )
    {
        return ALLOC_FAILED;
    }

    for ( i = 0; i < subrec_qty; i++ )
    {
        if ( subrec_name != NULL )
            if ( strcmp(p_subrec[i].name, subrec_name) )
                continue;

        if ( !strcmp(p_subrec[i].class_name, class_name) )
        {
            for ( j = 0; j < p_subrec[i].qty_svars; j++ )
            {
                name_found = FALSE;

                /* Make sure that we are not creating duplicate svar entries */
                for ( k = 0; k < var_index; k++ )
                    if ( !strcmp(temp_vars[k], p_subrec[i].svar_names[j]) )
                    {
                        name_found = TRUE;
                        break;
                    }
                if ( !name_found )
                {
                    temp_vars[var_index++] = strdup(p_subrec[i].svar_names[j]);
                }
            }
        }
    }

    *subrec_vars_len = var_index;
    if ( *subrec_vars_len > 0 )
    {
        *subrec_vars = (char *)temp_vars;
    }
    else
    {
        *subrec_vars = (char *)NULL;
    }
    return OK;
}

/************************************************************
 * TAG( mc_mr_is_valid_svar )
 *
 * Tests if this svar is in this srecord. Returns TRUE if so.
 */
Return_value mc_mr_is_valid_svar(Subrecord *p_subrec, char *svar, Bool_type *valid)
{
    int j = 0;
    *valid = FALSE;
    for ( j = 0; j < p_subrec->qty_svars; j++ )
    {
        if ( !strcmp(p_subrec->svar_names[j], svar) )
            *valid = TRUE;
    }
    return OK;
}

/************************************************************
 * TAG( mc_mr_dump_subrecs )
 *
 * Dump the Subrecord definitions to a file
 */
Return_value mc_mr_dump_subrecs(int dbid, char *fname, int subrec_qty, Subrecord *p_subrec)
{
    int i, j;
    int block_id, block_qty = 0;
    int qty_states = 0;
    FILE *fp;
    char fname_subrec[M_MAX_NAME_LEN];
    Return_value status;

    status = mc_query_family(dbid, QTY_STATES, NULL, NULL, &qty_states);
    if ( status != OK )
    {
        return status;
    }

    strcpy(fname_subrec, fname);
    strcat(fname_subrec, "-SUBRECS");

    fp = fopen(fname_subrec, "wb+");
    if ( fp == NULL )
    {
        return OPEN_FAILED;
    }

    fprintf(fp, "\nTotal States = [%d]\n", qty_states);

    /* Loop over subrecs */
    for ( i = 0; i < subrec_qty; i++ )
    {
        block_qty = p_subrec[i].qty_blocks;
        fprintf(fp, "\n\nSubrecord %d  Name=\"%s\" - [Class=\"%s\"] ", i, p_subrec[i].name, p_subrec[i].class_name);
        fprintf(fp, "\n\tState Variables:");
        for ( j = 0; j < p_subrec[i].qty_svars; j++ )
        {
            fprintf(fp, " [\"%s\"] ", p_subrec[i].svar_names[j]);
        }

        fprintf(fp, "\n\tQty Blocks: [%d]", block_qty);

        block_id = 0;
        for ( j = 0; j < block_qty; j++ )
        {
            fprintf(fp, "\n\t\tBlock Ids [%d-%d]", p_subrec[i].mo_blocks[block_id],
                    p_subrec[i].mo_blocks[block_id + 1]);
            block_id += 2;
        }
    }

    fclose(fp);
    return OK;
}

/************************************************************
 * TAG( mc_mr_get_param_list )
 *
 * Get the parameters for a Mili database (either standard or TI parameters).
 */
Return_value mc_mr_get_param_list(int dbid, Bool_type ti_param, char *search_pattern, int *list_len, char **param_list)
{
    int num_entries;
    char **temp_list;
    char search_string[512];

    search_string[0] = '\0';
    if ( search_pattern != NULL )
    {
        strncat(search_string, search_pattern, 511);
    }
    else
    {
        strncat(search_string, "*", 511);
    }

    if ( !ti_param )
    {
        /* First just return the number of entries so we can allocate space */

        num_entries = mc_htable_search_wildcard(dbid, 0, FALSE, search_string, "NULL", "NULL", NULL);

        temp_list = (char **)malloc(num_entries * sizeof(char *));
        if ( num_entries > 0 && temp_list == NULL )
        {
            return ALLOC_FAILED;
        }

        num_entries = mc_htable_search_wildcard(dbid, 0, FALSE, search_string, "NULL", "NULL", temp_list);
    }
    else
    {
        num_entries = mc_ti_htable_search_wildcard(dbid, 0, FALSE, "*", "NULL", "NULL", NULL);

        temp_list = (char **)malloc(num_entries * sizeof(char *));
        if ( num_entries > 0 && temp_list == NULL )
        {
            return ALLOC_FAILED;
        }

        num_entries = mc_ti_htable_search_wildcard(dbid, 0, FALSE, "*", "NULL", "NULL", temp_list);
    }

    *param_list = (char *)temp_list;
    *list_len = (int)num_entries;

    return OK;
}

/************************************************************
 * TAG( mc_mr_get_param_attributes )
 *
 * Get the type and length for a specified param.
 */
Return_value mc_mr_get_param_attributes(int dbid, char *param_name, Bool_type ti_param, int *type, int *len)
{
    int byte_count;
    int i;
    int fidx, name_idx;
    int qty_entries;
    Mili_family *fam;
    File_dir *directory;
    if ( strlen(param_name) == 0 )
    {
        return INVALID_NAME;
    }
    name_idx = 0;
    fam = fam_list[dbid];

    for ( fidx = 0; fidx < fam->file_count; fidx++ )
    {
        if ( fam->char_header[DIR_VERSION_IDX] == 1 && !ti_param )
        {
            directory = fam->directory;
        }
        else if ( fam->char_header[DIR_VERSION_IDX] == 1 && ti_param )
        {
            directory = fam->ti_directory;
        }
        else
        {
            directory = fam->directory;
        }
        qty_entries = directory[fidx].qty_entries;

        for ( i = 0; i < qty_entries; i++ )
        {
            if ( !strcmp(directory[fidx].names[name_idx], param_name) )
            {
                *type = (int)directory[fidx].dir_entries[i][MODIFIER1_IDX];

                /* Calculate length in units of data type */
                byte_count = mc_calc_bytecount(*type, 1);
                if ( byte_count == 0 )
                {
                    return INVALID_DATA_TYPE;
                }

                *len = (int)directory[fidx].dir_entries[i][LENGTH_IDX] / byte_count;
                return OK;
            }
            name_idx += directory[fidx].dir_entries[i][STRING_QTY_IDX];
        }
    }

    return NOT_OK;
}

/************************************************************
 * TAG( mc_mr_get_param_data )
 *
 * Read data for a specified parameter - either standard or TI .
 */
Return_value mc_mr_get_param_data(int dbid, char *param_name, Bool_type ti_param, void **param_data)
{
    int byte_count;
    int i;
    int fidx;
    int qty_entries;
    Mili_family *fam;

    if ( strlen(param_name) == 0 )
    {
        return INVALID_NAME;
    }

    fam = fam_list[dbid];

#ifndef NEW
    if ( !ti_param )
    {
        for ( fidx = 0; fidx < fam->file_count; fidx++ )
        {
            qty_entries = fam->directory[fidx].qty_entries;

            for ( i = 0; i < qty_entries; i++ )
            {
                if ( !strcmp(fam->directory[fidx].names[i], param_name) )
                {
                    *type = fam->directory[fidx].dir_entries[i][MODIFIER1_IDX];

                    /* Calculate length in units of data type */
                    byte_count = mc_calc_bytecount(*type, 1);
                    if ( byte_count == 0 )
                    {
                        return INVALID_DATA_TYPE;
                    }

                    *len = fam->directory[fidx].dir_entries[i][LENGTH_IDX] / byte_count;
                    return (OK);
                }
            }
        }
    }
    else
    {
        for ( fidx = 0; fidx < fam->file_count; fidx++ )
        {
            qty_entries = fam->ti_directory[fidx].qty_entries;

            for ( i = 0; i < qty_entries; i++ )
            {
                if ( !strcmp(fam->ti_directory[fidx].names[i], param_name) )
                {
                    *type = fam->ti_directory[fidx].dir_entries[i][MODIFIER1_IDX];

                    /* Calculate length in units of data type */
                    byte_count = mc_calc_bytecount(*type, 1);
                    if ( byte_count == 0 )
                    {
                        return INVALID_DATA_TYPE;
                    }

                    *len = fam->ti_directory[fidx].dir_entries[i][LENGTH_IDX] / byte_count;
                    return (OK);
                }
            }
        }
    }
#endif

    return NOT_OK;
}

/************************************************************
 * TAG( mc_mr_open_input_dbase )
 *
 * Checks if this is a valid Mii database type.
 */
Return_value mc_mr_open_input_dbase(char *fname, Mili_analysis *analy)
{
    Famid fid;
    Return_value status = OK;

    /* This should be put in a utility function, the ability to parse
     * a filename for the path and root name
     */
    char root_name[M_MAX_NAME_LEN];
    char path[M_MAX_NAME_LEN];
    int pos = 0;
    int rootlen = 0;
    int len = strlen(fname);

    for ( pos = len - 1; fname[pos] != '\\' && fname[pos] != '/' && pos >= 0; pos-- )
        ;

    if ( pos > 0 )
    {
        strncpy(path, fname, pos);
    }
    else /* no path */
    {
        pos = 0;
    }

    path[pos] = '\0';
    rootlen = len - pos - 1;
    strncpy(root_name, fname + pos + 1, rootlen);
    root_name[rootlen] = '\0';

    if ( pos > 0 )
    {
        status = mc_open(root_name, path, "r", &fid);
    }
    else
    {
        status = mc_open(fname, path, "r", &fid);
    }
    if ( status != OK )
    {
        return status;
    }

    analy->db_ident = fid;
    return status;
}

/* End of mr_funcs.c */
