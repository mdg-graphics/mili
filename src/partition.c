/*
 * partition.c - File for handling reading, closing and cleaning up memory
 *               used for partition files.
 *
 *
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *
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

 */

/*
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Augist 18, 2006: Moved from Xmilics so we can convert
 *  partition files to TI files.
 *  See SCR#480
 *
 *  I. R. Corey - Augist 20, 2007: Added local to global mapping
 *  functions for elements and nodes.
 *  See SCR#480
 *
 *  I. R. Corey - May 19, 2010: Incorporated more changes from
 *                Sebastian at IABG to support building under Windows.
 *                SCR #679.
 *
 ************************************************************************
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mili.h"
#include "partition.h"

extern Mili_family **fam_list;

static void read_lines(int nline, int offset, int base_size, int last_line, int *data, char *line_buffer, FILE *file);

/****************************************************************
  READ_PARTITION reads the partition assignment file.
  Returns 1 if successful and -1 if unsuccessful.
 ****************************************************************/
static void read_lines(int nline, int offset, int base_size, int last_line, int *data, char *line_buffer, FILE *file)
{
    int linesize = base_size;
    int current_pos, line;
    for ( line = 0; line < nline; line++ )
    {
        if ( (line == nline - 1) && last_line > 0 )
        {
            linesize = last_line;
        }

        for ( current_pos = 0; current_pos < linesize; current_pos++ )
        {
            fgets(line_buffer, 9, file);
            data[offset + (10 * line + current_pos)] = atoi(line_buffer);
        }
        fgets(line_buffer, 100 - linesize * 8, file);
    }
    return;
}

/****************************************************************
  READ_PARTITION reads the partition assignment file.
  Returns 1 if successful and -1 if unsuccessful.
 ****************************************************************/
int read_partition_mili(char *partition_file_name, Partition_mili **mapin)
{
    int line, last_line;
    int nelhtot;
    int nelbtot;
    int nelstot;
    int nline;
    int nnptot;
    int offset;
    int proc;
    char cdum[100];
    char line_buffer[100];
    char line_save[100];
    long int position = 0;

    Partition_mili *map;

    map = *mapin;

    /* Open the partition assignment file. */
    if ( (map->partition_file = fopen(partition_file_name, "rb")) == NULL )
    {
        fprintf(stderr, "Unable to open partition file: %s\n", partition_file_name);
        return -1;
    }

    /* Default parameters */
    map->is_TI_map = FALSE;
    map->comb_split = FALSE;
    map->neldg = 0;
    map->nproc = 0;
    map->th_node_map = NULL;
    map->th_hex_map = NULL;
    map->th_beam_map = NULL;
    map->th_shell_map = NULL;
    map->th_disc_map = NULL;

    /* Read the summary section of */
    /* the partition assignment file. */

    /* Read the global problem parameters. */
    fgets(line_buffer, 100, map->partition_file);
    position = ftell(map->partition_file);
    fgets(line_buffer, 100, map->partition_file);
    sscanf(line_buffer, "%s", cdum);

    while ( strncmp(cdum, "#", 1) == 0 )
    {
        position = ftell(map->partition_file);
        fgets(line_buffer, 100, map->partition_file);
        strcpy(line_save, line_buffer);
        sscanf(line_buffer, "%s", cdum);
    }

    if ( strncmp(cdum, "-", 1) != 0 )
    {
        fseek(map->partition_file, position, SEEK_SET);
        fgets(line_buffer, 9, map->partition_file);
        map->nnpg = atoi(line_buffer);
        if ( map->nnpg < 0 || map->nnpg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->nelhg = atoi(line_buffer);
        if ( map->nelhg < 0 || map->nelhg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->nelbg = atoi(line_buffer);
        if ( map->nelbg < 0 || map->nelbg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->nelsg = atoi(line_buffer);
        if ( map->nelsg < 0 || map->nelsg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->neltg = atoi(line_buffer);
        if ( map->neltg < 0 || map->neltg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->nproc = atoi(line_buffer);
        if ( map->nproc < 0 || map->nproc >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->neldg = atoi(line_buffer);
        if ( map->neldg < 0 || map->neldg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
    }
    else
    {
        fgets(line_buffer, 100, map->partition_file);
        fgets(line_buffer, 100, map->partition_file);
        sscanf(line_buffer, "%d", &map->neldg);
        if ( map->neldg < 0 || map->neldg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 100, map->partition_file);
        fgets(line_buffer, 9, map->partition_file);
        map->nnpg = atoi(line_buffer);
        if ( map->nnpg < 0 || map->nnpg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->nelhg = atoi(line_buffer);
        if ( map->nelhg < 0 || map->nelhg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->nelbg = atoi(line_buffer);
        if ( map->nelbg < 0 || map->nelbg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->nelsg = atoi(line_buffer);
        if ( map->nelsg < 0 || map->nelsg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->neltg = atoi(line_buffer);
        if ( map->neltg < 0 || map->neltg >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 9, map->partition_file);
        map->nproc = atoi(line_buffer);
        if ( map->nproc < 0 || map->nproc >= 1000000000 )
        {
            fclose(map->partition_file);
            return -1;
        }
        fgets(line_buffer, 100, map->partition_file);
    }

    if ( map->nproc == 0 )
    {
        map->nproc = map->neltg;
        map->neltg = 0;
    }

    if ( map->nproc > 1 )
    {
        map->nnp = NEW_N(int, map->nproc, "Number of Nodal Points (local)");
        map->ncso = NEW_N(int, map->nproc, "Number of Contact-surface-only nodes (local)");
        map->nelh = NEW_N(int, map->nproc, "Number of Hexahedral Elements (local)");
        map->nelb = NEW_N(int, map->nproc, "Number of Beam Elements (local)");
        map->nels = NEW_N(int, map->nproc, "Number of Shell Elements (local)");
        map->nelt = NEW_N(int, map->nproc, "Number of Thick-Shell Elements (local)");
        map->neld = NEW_N(int, map->nproc, "Number of Discrete Elements (local)");

        /* For multiple processors - data combining and possibly data
         * conversion. */

        nline = (int)(map->nproc / 10);
        if ( (last_line = map->nproc % 10) != 0 )
        {
            nline++;
        }

        /* Read the number of nodal points */
        /* assigned to each processor. */
        if ( map->nnpg != 0 )
        {
            fgets(line_buffer, 100, map->partition_file);
            read_lines(nline, 0, 10, last_line, map->nnp, line_buffer, map->partition_file);
        }

        /* Read the number of hexahedral elements */
        /* assigned to each processor. */
        if ( map->nelhg != 0 )
        {
            fgets(line_buffer, 100, map->partition_file);
            read_lines(nline, 0, 10, last_line, map->nelh, line_buffer, map->partition_file);
        }

        /* Read the number of beam elements */
        /* assigned to each processor. */
        if ( map->nelbg != 0 )
        {
            fgets(line_buffer, 100, map->partition_file);
            read_lines(nline, 0, 10, last_line, map->nelb, line_buffer, map->partition_file);
        }

        /* Read the number of shell elements */
        /* assigned to each processor. */
        if ( map->nelsg != 0 )
        {
            fgets(line_buffer, 100, map->partition_file);
            read_lines(nline, 0, 10, last_line, map->nels, line_buffer, map->partition_file);
        }

        /* Read the number of thick-shell elements */
        /* assigned to each processor. */
        if ( map->neltg != 0 )
        {
            fgets(line_buffer, 100, map->partition_file);
            read_lines(nline, 0, 10, last_line, map->nelt, line_buffer, map->partition_file);
        }

        /* Skip over the number of shared nodal points */
        /* assigned to each processor. */
        fgets(line_buffer, 100, map->partition_file);
        for ( line = 0; line < nline; line++ )
        {
            fgets(line_buffer, 100, map->partition_file);
        }

        /* Skip over the number of processors */
        /* that are adjacent each processor. */
        fgets(line_buffer, 100, map->partition_file);
        for ( line = 0; line < nline; line++ )
        {
            fgets(line_buffer, 100, map->partition_file);
        }

        /* Read the assignment section of */
        /* the partition assignment file. */

        /*****************************************************/
        /* NODAL POINTS                                      */
        /* Read the nodal points assigned to each processor. */
        /*****************************************************/
        if ( map->nnpg != 0 )
        {
            nnptot = 1;
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                nnptot += map->nnp[proc];
            }
            map->npltg = NEW_N(int, nnptot, "Nodal Point Local-to-Global Map");
            offset = 0;
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                if ( map->nnp[proc] != 0 )
                {
                    nline = (map->nnp[proc] / 10);
                    if ( (last_line = map->nnp[proc] % 10) != 0 )
                    {
                        nline++;
                    }
                    fgets(line_buffer, 100, map->partition_file);

                    map->ncso[proc] = 0;
                    read_lines(nline, offset, 10, last_line, map->npltg, line_buffer, map->partition_file);
                }
                offset += map->nnp[proc];
            }
        }

        /************************************************************/
        /* HEX ELEMENTS                                             */
        /* Read the hexahedral elements assigned to each processor. */
        /************************************************************/
        if ( map->nelhg != 0 )
        {
            nelhtot = 1;
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                nelhtot += map->nelh[proc] + map->nelt[proc];
            }
            map->elhltg = NEW_N(int, nelhtot, "Hexahedral Element Local-to-Global Map");
            offset = 0;
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                if ( map->nelh[proc] != 0 )
                {
                    nline = (map->nelh[proc] / 10);

                    if ( (last_line = map->nelh[proc] % 10) != 0 )
                    {
                        nline++;
                    }

                    fgets(line_buffer, 100, map->partition_file);
                    read_lines(nline, offset, 10, last_line, map->elhltg, line_buffer, map->partition_file);
                }
                offset += (map->nelh[proc] + map->nelt[proc]);
            }
        }

        /* Read the beam elements assigned to each processor. */
        if ( map->nelbg != 0 )
        {
            nelbtot = 1;
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                nelbtot += map->nelb[proc];
            }
            map->elbltg = NEW_N(int, nelbtot, "Beam Element Local-to-Global Map");
            offset = 0;
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                if ( map->nelb[proc] != 0 )
                {
                    nline = (map->nelb[proc] / 10);
                    if ( (last_line = map->nelb[proc] % 10) != 0 )
                    {
                        nline++;
                    }
                    fgets(line_buffer, 100, map->partition_file);
                    read_lines(nline, offset, 10, last_line, map->elbltg, line_buffer, map->partition_file);
                }
                offset += map->nelb[proc];
            }
        }

        /* Read the shell elements assigned to each processor. */
        if ( map->nelsg != 0 )
        {
            nelstot = 1;
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                nelstot += map->nels[proc];
            }
            map->elsltg = NEW_N(int, nelstot, "Shell Element Local-to-Global Map");
            offset = 0;
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                if ( map->nels[proc] != 0 )
                {
                    nline = (map->nels[proc] / 10);
                    if ( (last_line = map->nels[proc] % 10) != 0 )
                    {
                        nline++;
                    }
                    fgets(line_buffer, 100, map->partition_file);
                    read_lines(nline, offset, 10, last_line, map->elsltg, line_buffer, map->partition_file);
                }
                offset += map->nels[proc];
            }
        }

        /* Read the thick-shell elements assigned to each processor. */
        if ( map->neltg != 0 )
        {
            offset = map->nelh[0];
            for ( proc = 0; proc < map->nproc; proc++ )
            {
                if ( map->nelt[proc] != 0 )
                {
                    nline = (map->nelt[proc] / 10);
                    if ( (last_line = map->nelt[proc] % 10) != 0 )
                    {
                        nline++;
                    }
                    fgets(line_buffer, 100, map->partition_file);
                    read_lines(nline, offset, 10, last_line, map->elhltg, line_buffer, map->partition_file);
                }
                offset += map->nelt[proc] + map->nelh[proc + 1];
            }
        }

        /* Allocate space for the mapping for discrete elements */
        if ( map->neldg != 0 )
        {
            map->eldltg = NEW_N(int, map->neldg, "Discrete Element Local-to-Global Map");
        }

        /* Set the number of hexahedral elements per processor to the */
        /* sum of the number of hexahedral and thick-shell elements. */
        for ( proc = 0; proc < map->nproc; proc++ )
        {
            map->nelh[proc] += map->nelt[proc];
        }
    }

    /* Close the partition assignment file. */
    fclose(map->partition_file);

    /* Successfully read the partition assignment file. */
    return 1;
}

/*****************************************************************
 * TAG( free_partition_mili )
 *
 *  Free memory used in reading the partition file.
 */
void free_partition_mili(Partition_mili **mapin)
{
    Partition_mili *part_map;

    part_map = mapin[0];

    if ( part_map == NULL )
    {
        return;
    }

    if ( part_map->nnp != NULL )
    {
        free(part_map->nnp);
        part_map->nnp = NULL;
        free(part_map->npltg);
        part_map->npltg = NULL;
        part_map->nnpg = 0;
    }
    if ( part_map->ncso != NULL )
    {
        free(part_map->ncso);
        part_map->ncso = NULL;
    }
    if ( part_map->nelh != NULL )
    {
        free(part_map->nelh);
        part_map->nelh = NULL;
        free(part_map->elhltg);
        part_map->elhltg = NULL;
        part_map->nelhg = 0;
    }
    if ( part_map->nelb != NULL )
    {
        free(part_map->nelb);
        part_map->nelb = NULL;
        free(part_map->elbltg);
        part_map->elbltg = NULL;
        part_map->nelbg = 0;
    }
    if ( part_map->nels != NULL )
    {
        free(part_map->nels);
        part_map->nels = NULL;
        free(part_map->elsltg);
        part_map->elsltg = NULL;
        part_map->nelsg = 0;
    }
    if ( part_map->nelt != NULL )
    {
        free(part_map->nelt);
        part_map->nelt = NULL;
    }
}

/****************************************************************
  READ_PARTITION reads the local partition assignment data from
  a Mili database using the TI inerface.

  Returns 1 if successful and -1 if unsuccessful.
 ****************************************************************/
int read_partition_ti_local(Famid fam_id, int proc, Bool_type alloc_mem, Partition_mili **mapin)

{
    int num_entries = 0;
    char *wildcard_list[100];
    Mili_family *fam;

    int i;
    int nnptot = 1;
    int nelhtot = 1;
    int num_regular_hex = 0;
    int nelstot = 1;
    int offset;
    int *temp_ptr, temp_idx;

    Partition_mili *map;
    int num_items_read = 0;

    int status;

    map = *mapin;

    fam = fam_list[fam_id];

    offset = 0;
    for ( i = 0; i < map->nproc; i++ )
    {
        nnptot += map->nnp[i];
        if ( i < proc )
        {
            offset = nnptot;
        }
    }

    offset--;
    if ( offset <= 0 )
    {
        offset = 0;
    }

    if ( alloc_mem )
    {
        map->npltg = NEW_N(int, nnptot, "Nodal Point Local-to-Global Map");
    }

    num_entries = htable_search_wildcard(fam->ti_param_table, 0, FALSE, "node.map", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_array(fam_id, wildcard_list[0], (void **)&temp_ptr, &num_items_read);

        temp_idx = 0;
        for ( i = 0; i < map->nnp[proc]; i++ )
        {
            map->npltg[i + offset] = (int)temp_ptr[temp_idx++];
        }

        free(temp_ptr);
    }
    htable_delete_wildcard_list(num_entries, wildcard_list);

    offset = 0;
    for ( i = 0; i < map->nproc; i++ )
    {
        nelhtot += map->nelh[i] + map->nelt[i];
        /*              Hexes             Thick shells */
        num_regular_hex += map->nelh[i];

        if ( i < proc )
        {
            offset = nelhtot;
        }
    }

    offset--;
    if ( offset < 0 )
    {
        offset = 0;
    }

    if ( alloc_mem )
    {
        map->elhltg = NEW_N(int, nelhtot, "Hexahedral Element Local-to-Global Map");
    }

    num_entries = htable_search_wildcard(fam->ti_param_table, 0, FALSE, "brick.map", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_array(fam_id, wildcard_list[0], (void **)&temp_ptr, &num_items_read);

        temp_idx = 0;
        for ( i = 0; i < map->nelh[proc]; i++ )
        {
            map->elhltg[i + offset] = (int)temp_ptr[temp_idx++];
        }

        free(temp_ptr);
    }
    htable_delete_wildcard_list(num_entries, wildcard_list);

    for ( i = 0; i < map->nproc; i++ )
    {
        nelstot += map->nels[i];
        if ( i < proc )
        {
            offset = nelstot;
        }
    }

    offset--;
    if ( offset <= 0 )
    {
        offset = 0;
    }

    if ( alloc_mem )
    {
        map->elsltg = NEW_N(int, nelstot, "Shell Element Local-to-Global Map");
    }

    num_entries = htable_search_wildcard(fam->ti_param_table, 0, FALSE, "shell.map", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_array(fam_id, wildcard_list[0], (void **)&temp_ptr, &num_items_read);

        temp_idx = 0;
        for ( i = 0; i < map->nels[proc]; i++ )
        {
            map->elsltg[i + offset] = (int)temp_ptr[temp_idx++];
        }

        free(temp_ptr);
    }
    htable_delete_wildcard_list(num_entries, wildcard_list);

    offset--;
    if ( offset <= 0 )
    {
        offset = 0;
    }

    if ( alloc_mem )
    {
        map->elbltg = NEW_N(int, nelstot, "Beam Element Local-to-Global Map");
    }

    num_entries = htable_search_wildcard(fam->ti_param_table, 0, FALSE, "beam.map", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_array(fam_id, wildcard_list[0], (void **)&temp_ptr, &num_items_read);

        temp_idx = 0;
        for ( i = 0; i < map->nels[proc]; i++ )
        {
            map->elbltg[i + offset] = (int)temp_ptr[temp_idx++];
        }

        free(temp_ptr);
    }
    htable_delete_wildcard_list(num_entries, wildcard_list);

    /* Thick shells are added to the end of the Hexes array (elhltg) */
    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "Thick shell elements", "State-0", "NULL", wildcard_list);
    offset = num_regular_hex + offset;
    if ( num_entries > 0 )
    {
        status = mc_ti_read_array(fam_id, wildcard_list[0], (void **)&temp_ptr, &num_items_read);

        temp_idx = 0;
        for ( i = 0; i < map->nels[proc]; i++ )
        {
            map->elsltg[i + offset] = (int)temp_ptr[temp_idx++];
        }

        free(temp_ptr);
    }

    htable_delete_wildcard_list(num_entries, wildcard_list);

    return OK;
}

/****************************************************************
  READ_PARTITION reads the global partition assignment data from
  a Mili database using the TI inerface.

  Returns 1 if successful and -1 if unsuccessful.
 ****************************************************************/
int read_partition_ti_global(Famid fam_id, int proc, Bool_type alloc_mem, Partition_mili **mapin)

{
    int status;

    int num_entries = 0;
    char *wildcard_list[100];
    Mili_family *fam;

    Partition_mili *map;
    map = *mapin;

    fam = fam_list[fam_id];

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numnp_global", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nnpg);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "nummat_global", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nummat);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numelh_glogal", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nelhg);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numelb_global", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nelbg);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numels_global", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nelsg);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numelt_global", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->neltg);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numeld_global", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->neldg);
    }

    /* Allocate and read proc length arrays */

    if ( alloc_mem )
    {
        map->nnp = NEW_N(int, map->nproc, "Number of Nodal Points (local)");
        map->ncso = NEW_N(int, map->nproc, "Number of Contact-surface-only nodes (local)");
        map->nelh = NEW_N(int, map->nproc, "Number of Hexahedral Elements (local)");
        map->nelb = NEW_N(int, map->nproc, "Number of Beam Elements (local)");
        map->nels = NEW_N(int, map->nproc, "Number of Shell Elements (local)");
        map->nelt = NEW_N(int, map->nproc, "Number of Thick-Shell Elements (local)");
        map->neld = NEW_N(int, map->nproc, "Number of Discrete Elements (local)");
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numnp_local", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nnp[proc]);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numelh_local", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nelh[proc]);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numelb_local", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nelb[proc]);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numels_local", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->nels[proc]);
    }

    num_entries =
        htable_search_wildcard(fam->ti_param_table, 0, FALSE, "numeld_local", "State-0", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        status = mc_ti_read_scalar(fam_id, wildcard_list[0], &map->neld[proc]);
    }

    /* All done reading partition mapping data - cleanup and exit */

    htable_delete_wildcard_list(num_entries, wildcard_list);

    return OK;
}

/****************************************************************
  UPDATE PARTITION This function will update the global sums
  which are not initialized by adding the local values for each
  processor.

 ****************************************************************/
int update_partition_ti_global(Partition_mili **mapin)
{
    int i;

    Partition_mili *map;
    map = *mapin;

    if ( map->nnpg == 0 && map->nnp != NULL )
    {
        for ( i = 0; i < map->nproc; i++ )
        {
            map->nnpg += map->nnp[i];
        }
    }

    if ( map->nelbg == 0 && map->nelb != NULL )
    {
        for ( i = 0; i < map->nproc; i++ )
        {
            map->nelbg += map->nelb[i];
        }
    }

    if ( map->nelhg == 0 && map->nelh != NULL )
    {
        for ( i = 0; i < map->nproc; i++ )
        {
            map->nelhg += map->nelh[i];
        }
    }

    if ( map->nelsg == 0 && map->nels != NULL )
    {
        for ( i = 0; i < map->nproc; i++ )
        {
            map->nelsg += map->nels[i];
        }
    }

    if ( map->neltg == 0 && map->nelt != NULL )
    {
        for ( i = 0; i < map->nproc; i++ )
        {
            map->neltg += map->nelt[i];
        }
    }

    if ( map->neldg == 0 && map->neld != NULL )
    {
        for ( i = 0; i < map->nproc; i++ )
        {
            map->neldg += map->neld[i];
        }
    }

    return OK;
}

/************************************************************
 * TAG( dump_partition )
 *
 * I. R. Corey - Jan 03, 2006: Created
 *
 * This function will dump the contents of the partition map to an
 * ASCII file for debugging purposes.
 *
 */

int dump_partition(char *partition_file_name, Partition_mili *map)
{
    int i;
    int offset;
    int proc;

    int numPoints = 0, total = 0;
    int num_ts = 0;

    FILE *dump_fp;

    /* Create the partition dump file. */
    if ( (dump_fp = fopen(partition_file_name, "wb+")) == NULL )
    {
        fprintf(stderr, "Unable to create partition dump file: %s\n", partition_file_name);
        return -1;
    }

    /* Dump the global problem parameters. */

    fprintf(dump_fp, "\nNumber Processors = %d", map->nproc);

    fprintf(dump_fp, "\nNNPG=%d NELHG=%d NELBG=%d NELSG=%d NELTG=%d NELDG=%d\n", map->nnpg, map->nelhg, map->nelbg,
            map->nelsg, map->neltg, map->neldg);

    /* Dump the number of nodal points */
    /* assigned to each processor.     */
    if ( map->nnp != NULL )
    {
        /* Count total number of nodal points */

        for ( i = 0; i < map->nproc; i++ )
        {
            total += map->nnp[i];
        }

        /* Print the nodal points per processor */
        if ( total > 0 )
        {
            fprintf(dump_fp, "\nNODAL:\t");
            for ( i = 0; i < map->nproc; i++ )
            {
                fprintf(dump_fp, " %d", map->nnp[i]);
            }
            fprintf(dump_fp, "\n");
        }
    }

    /* Dump the number of hexahedral elements */
    /* assigned to each processor.            */
    if ( map->nelh != NULL )
    {
        /* Count total number of HEX elements */

        total = 0;
        for ( i = 0; i < map->nproc; i++ )
        {
            total += map->nnp[i];
        }

        /* Print the HEX elements processor */
        if ( total > 0 )
        {
            fprintf(dump_fp, "\nHEX:\t");
            for ( i = 0; i < map->nproc; i++ )
            {
                fprintf(dump_fp, " %d", map->nelh[i]);
            }
            fprintf(dump_fp, "\n");
        }
    }

    /* Dump the number of beam elements */
    /* assigned to each processor.      */
    if ( map->nelb != NULL )
    {
        /* Count total number of BEAM elements */

        total = 0;
        for ( i = 0; i < map->nproc; i++ )
        {
            total += map->nelb[i];
        }

        /* Print the HEX elements processor */
        if ( total > 0 )
        {
            fprintf(dump_fp, "\nBEAM:\t");
            for ( i = 0; i < map->nproc; i++ )
            {
                fprintf(dump_fp, " %d", map->nelb[i]);
            }
            fprintf(dump_fp, "\n");
        }
    }

    /* Dump the number of shell elements */
    /* assigned to each processor.       */
    if ( map->nels != NULL )
    {
        /* Count total number of SHELL elements */

        total = 0;
        for ( i = 0; i < map->nproc; i++ )
        {
            total += map->nels[i];
        }

        /* Print the SHELL elements processor */
        if ( total > 0 )
        {
            fprintf(dump_fp, "\nSHELL:\t");
            for ( i = 0; i < map->nproc; i++ )
            {
                fprintf(dump_fp, " %d", map->nels[i]);
            }
            fprintf(dump_fp, "\n");
        }
    }

    /* Dump the number of thick-shell elements */
    /* assigned to each processor.             */
    if ( map->nelt != NULL )
    {
        /* Count total number of Thick-SHELL elements */

        total = 0;
        for ( i = 0; i < map->nproc; i++ )
        {
            total += map->nelt[i];
        }

        /* Print the Thick-SHELL elements processor */
        if ( total > 0 )
        {
            fprintf(dump_fp, "\nThick SHELL:\t");
            for ( i = 0; i < map->nproc; i++ )
            {
                fprintf(dump_fp, " %d", map->nelt[i]);
            }
            fprintf(dump_fp, "\n");
        }
    }

    /**********************************/
    /* Dump the assignment section of */
    /* the partition assignment file. */
    /**********************************/

    fprintf(dump_fp, "\n\n");

    /* Dump the nodal points assigned to each processor. */
    if ( map->npltg != NULL )
    {
        offset = 0;
        for ( proc = 0; proc < map->nproc; proc++ )
        {
            numPoints = map->nnp[proc];
            if ( numPoints > 0 )
            {
                fprintf(dump_fp, "\nAssigned Nodes[%d]: ", proc);

                /* Print the nodal points per processor */
                for ( i = 0; i < numPoints; i++ )
                {
                    fprintf(dump_fp, " %d", map->npltg[i + offset]);
                }
                fprintf(dump_fp, "\n");
                offset += numPoints;
            }
        }
    }

    /* Dump the hexahedral elements assigned to each processor. */
    if ( map->elhltg != NULL )
    {
        offset = 0;
        for ( proc = 0; proc < map->nproc; proc++ )
        {
            numPoints = (map->nelh[proc] + map->nelt[proc]);
            if ( numPoints > 0 )
            {
                fprintf(dump_fp, "\nAssigned HEX[%d]: ", proc);

                /* Print the HEX elements per processor */
                for ( i = 0; i < numPoints; i++ )
                {
                    fprintf(dump_fp, " %d", map->npltg[i + offset]);
                }
                fprintf(dump_fp, "\n");
                offset += numPoints;
            }
        }
    }

    /* Dump the beam elements assigned to each processor. */
    if ( map->elbltg != NULL )
    {
        offset = 0;
        for ( proc = 0; proc < map->nproc; proc++ )
        {
            numPoints = map->nelb[proc];
            fprintf(dump_fp, "\nAssigned BEAM[%d]: ", proc);

            /* Print the BEAM elements per processor */
            for ( i = 0; i < numPoints; i++ )
            {
                fprintf(dump_fp, " %d", map->elbltg[i + offset]);
            }
            fprintf(dump_fp, "\n");
            offset += numPoints;
        }
    }

    /* Dump the shell elements assigned to each processor. */
    if ( map->elsltg != NULL )
    {
        offset = 0;
        for ( proc = 0; proc < map->nproc; proc++ )
        {
            numPoints = map->nels[proc];
            if ( numPoints > 0 )
            {
                fprintf(dump_fp, "\nAssigned SHELL[%d]: ", proc);

                /* Print the SHELL elements per processor */
                for ( i = 0; i < numPoints; i++ )
                {
                    fprintf(dump_fp, " %d", map->elsltg[i + offset]);
                }
                fprintf(dump_fp, "\n");
                offset += numPoints;
            }
        }
    }

    /* Dump the thick-shell elements assigned to each processor. */
    num_ts = 0;
    for ( proc = 0; proc < map->nproc; proc++ )
    {
        num_ts += map->nelt[proc];
    }

    if ( num_ts > 0 && map->elhltg != NULL )
    {
        offset = 0;
        for ( proc = 0; proc < map->nproc; proc++ )
        {
            numPoints = (map->nelt[proc] + map->nelh[proc]);
            if ( numPoints > 0 )
            {
                fprintf(dump_fp, "\nAssigned Thick-SHELL[%d]: ", proc);

                /* Print the Thick-SHELL elements per processor */
                for ( i = 0; i < numPoints; i++ )
                {
                    fprintf(dump_fp, " %d", map->elhltg[i + offset]);
                }
                fprintf(dump_fp, "\n");
                offset += numPoints;
            }
        }
    }

    /* Close the partition assignment file. */
    fclose(dump_fp);

    /* Successfully read the partition assignment file. */
    return OK;
}

/************************************************************
 * TAG( calc_global_element_number ) DEPRECATE
 *
 * Given a local element number, this function will return
 * the global number for the requested object.
 *
 */
int calc_global_element_number(Partition_mili *map, int obj_type, int local_element, int proc)
{
    int i, offset = 0;
    int global_element = 0;

    switch ( obj_type )
    {
        case (M_HEX):
            /* Calculate processor offset into the local to global map */
            for ( i = 1; i <= proc; i++ )
            {
                offset += map->nelh[i];
            }
            global_element = map->elhltg[offset + local_element];

            if ( global_element < 0 )
            {
                i = 0;
            }
            break;

        case (M_QUAD):
            /* Calculate processor offset into the local to global map */
            for ( i = 1; i <= proc; i++ )
            {
                offset += map->nels[i];
            }
            global_element = map->elsltg[offset + local_element] - 1;
            if ( global_element < 0 )
            {
                i = 0;
            }
            break;

        case (M_BEAM):
            /* Calculate processor offset into the local to global map */
            for ( i = 1; i <= proc; i++ )
            {
                offset += map->nelb[i];
            }
            global_element = map->elbltg[offset + local_element] - 1;
            if ( global_element < 0 )
            {
                i = 0;
            }
            break;

        case (M_TRUSS): /* Discrete elements */
            /* Calculate processor offset into the local to global map */
            for ( i = 1; i <= proc; i++ )
            {
                offset += map->neld[i];
            }
            global_element = map->eldltg[offset + local_element] - 1;
            if ( global_element < 0 )
            {
                i = 0;
            }
            break;
    } /* switch */

    return (global_element);
}

/************************************************************
 * TAG( calc_global_node_number )
 *
 * Given a local node number, this function will return
 * the global number for the requested object.
 *
 */
int calc_global_node_number(Partition_mili *map, int local_node, int proc)
{
    int i, offset = 0;
    int global_node = 0;

    /* Calculate processor offset into the local to global map */
    for ( i = 1; i < proc; i++ )
    {
        offset += map->nnp[i];
    }
    global_node = map->npltg[offset + local_node] - 1;

    if ( global_node < 0 )
    {
        i = 0;
    }

    return (global_node);
}
