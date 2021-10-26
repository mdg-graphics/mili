/*
 * partition.h
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
 *  I. R. Corey - August 18, 2006: Moved from Xmilics so we can convert
 *  partition files to TI files.
 *
 *  I. R. Corey - Augist 20, 2007: Added local to global mapping
 *  functions for elements and nodes.
 *
 *  I. R. Corey - October 28, 2009: Added local to global mapping
 *  variables for meshless class (ML).
 ************************************************************************
 */

#ifndef PARTITION_H
#define PARTITION_H

#include "misc.h"
#include "mili_internal.h"

/*****************************************************************
 * TAG( Partition_mili  )
 *
 * A local to global map structure.
 */
typedef struct _Partition
{
   /* Partition assignment file name. */
   FILE *partition_file;

   Bool_type is_TI_map;

   Bool_type comb_split;
   Bool_type have_disc_el;

   /* Global problem parameters. */
   int nnpg;
   int nelhg;
   int nelbg;
   int nelsg;
   int neltg;
   int neldg;
   int nummat;
   int nelmg; /* Meshless */

   /* labels */
   int neltrig;
   int neltetg;
   int neltrussg;
   int nelwedgeg;

   /* Number of processors. */
   int nproc;

   /* Processor local problem parameters. */
   int *nnp;
   int *ncso;
   int *nelh;
   int *nelb;
   int *nels;
   int *nelt;
   int *neld;
   int *nelm; /* Meshless */

   /* labels */
   int *neltri;
   int *neltet;
   int *neltruss;
   int *nelwedge;

   /* Local-to-Global mappings. */
   /* For partition file and conns with labels */
   int *npltg;
   int *elhltg;
   int *elbltg;
   int *elsltg;
   int *eldltg;
   int *elmltg; /* Meshless */

   /* labels */
   int *eltriltg;
   int *eltetltg;
   int *eltrussltg;
   int *elwedgeltg;

   /* Time History Local-to-Global mappings. */
   int *th_node_map;
   int *th_hex_map;
   int *th_beam_map;
   int *th_shell_map;
   int *th_disc_map;
   int *th_ml_map;  /* Meshless */

} Partition_mili;
/* Prototypes */

int read_partition_mili( char *partition_file_name,
                         Partition_mili **map ) ;

int read_partition_ti_global( Famid fam_id, int proc,
                              Bool_type alloc_mem,
                              Partition_mili **map ) ;

int read_partition_ti_local( Famid fam_id, int proc,
                             Bool_type alloc_mem,
                             Partition_mili **map ) ;

int calc_global_element_number( Partition_mili *c_map,
                                int obj_type,
                                int local_index,
                                int proc ) ;

int calc_global_node_number( Partition_mili *map,
                             int local_node, int proc ) ;

void free_partition_mili( Partition_mili **map ) ;

#endif
