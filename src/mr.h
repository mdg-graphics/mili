
/*
 * Include file for Mili Readers
 *
 */

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

************************************************************************
* Modifications:
*
*  I. R. Corey - October 10, 2008: Created.
*
************************************************************************
*/

#ifndef MR_H
#define MR_H

#include "misc.h"
#include "mili_internal.h"

Return_value mc_mr_get_geom(int dbid, int mesh_id, int element_type, int *qty_elems, int **conn, int **mat,
                            int *num_mats, int **mat_list, int **part, int **elem_ids, int **labels);

Return_value mc_mr_get_result_len(int dbid, int subrec_qty, Subrecord *p_subrec, char *class_name, char *subrec_name,
                                  char *svar_name, int *svar_result_len);

Return_value mc_mr_get_result(int dbid, int state, int subrec_qty, Subrecord *p_subrec, char *class_name,
                              char *var_name, int *len, int *veclen, int *type, int *block_qty, int **block_list,
                              void **result);

Return_value mc_mr_get_subrec_list(int dbid, int subrec_qty, Subrecord *p_subrec, char *class_name,
                                   int *subrec_names_len, char **subrec_names);

Return_value mc_mr_get_subrec_vars(int dbid, int subrec_qty, Subrecord *p_subrec, char *class_name, char *subrec_name,
                                   int *subrec_vars_len, char **subrec_vars);

Return_value mc_mr_get_param_list(int dbid, Bool_type ti_param, char *search_pattern, int *list_len, char **param_list);

Return_value mc_mr_get_param_attributes(int dbid, char *var_name, Bool_type ti_param, int *type, int *len);

Return_value mc_mr_get_param_data(int dbid, char *var_name, Bool_type ti_param, void **param);

Return_value mc_mr_open_input_dbase(char *fname, Mili_analysis *analy);

Return_value mc_mr_dump_subrecs(int dbid, char *fname, int subrec_qty, Subrecord *p_subrec);

Return_value mc_mr_is_valid_svar(Subrecord *p_subrec, char *svar, Bool_type *valid);

Return_value mc_mr_get_subrecs(int fid, int *subrec_total, Subrecord **p_subr);
#endif
