/* $Id: mc_silo_mesh.c,v 1.11 2011/05/26 21:43:13 durrenberger1 Exp $ */

/*
 * Mesh-Silo I/O Library source code - Silo API
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

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  25-Jul-2007 IRC Initial version.                            */
/*                                                              */
/****************************************************************/

/*  Include Files  */
#include "silo.h"
#include "mili_internal.h"
#include "MiliSilo_Internal.h"
#include "MiliSilo_Internal.h"

#include <sys/types.h>


/*****************************************************************
 * TAG( mc_silo_get_mesh_id ) PUBLIC
 *
 * Map a mesh name into a mesh identifier.
 */
int
mc_silo_get_mesh_id( Famid famid, char *mesh_name, int *p_mesh_id )
{
   MiliSilo_family *fam;
   int i;

   fam = &family[famid];

   for ( i = 0; i < fam->qty_meshes; i++ )
      if ( strcmp( fam->meshes[i].name, mesh_name ) == 0 )
      {
         *p_mesh_id = i;
         return (int) OK;
      }

   return (int) NO_MATCH;
}


/*****************************************************************
 * TAG( mc_silo_def_nodes ) PUBLIC
 *
 * Define coordinates for a continuously-numbered sequence of nodes.
 * Logic assumes the sequence is ascending.
 */
int
mc_silo_def_nodes( Famid famid, int mesh_id, char *short_name, int start_node,
                   int stop_node, float *coords )
{
   MiliSilo_family *fam;
   int  class_id;
   int  i;
   int  rval, status;
   char mesh_name[32];
   int  num_meshes, mesh_index;
   int  num_nodes, nl_index=0;;
   int  *var1;

   fam = &family[famid];

   if ( fam->meshes[mesh_id].superclass != M_NODE )
      return SUPERCLASS_CONFLICT;

   CHECK_WRITE_ACCESS( fam )

   class_id = mc_silo_get_class_id( famid, mesh_id, short_name, -1 );
   if ( class_id<0 )
      return NO_CLASS;

   /* Locate mesh index */
   num_meshes = fam->qty_meshes;
   strcpy( mesh_name, fam->meshes[mesh_id].name );

   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Metadata", "/") ;

   sprintf(mesh_name, "MESH_%d", mesh_id);
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, mesh_name, NULL) ;

   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "Classes", NULL) ;

   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, short_name);
   if ( status != 0 )
      return NO_CLASS;

   if (fam->meshes[mesh_id].p_nl==NULL )
      fam->meshes[mesh_id].p_nl = NEW( Block_list, "Nodes block list" );

   rval = insert_range( fam->meshes[mesh_id].p_nl, start_node, stop_node );
   if ( rval == OBJECT_RANGE_OVERLAP )
      return ( status );
   else
      return ( OK );

   fam->meshes[mesh_id].nodes_defined = TRUE;

   nl_index  = fam->meshes[mesh_id].num_nl;
   num_nodes = (stop_node - start_node) + 1;

   fam->meshes[mesh_id].nodes[nl_index].num_nodes  = num_nodes;
   fam->meshes[mesh_id].nodes[nl_index].start_node = start_node;
   fam->meshes[mesh_id].nodes[nl_index].stop_node  = stop_node;
   fam->meshes[mesh_id].nodes[nl_index].coords = (float *) malloc(num_nodes*sizeof(float));
   for (i=0;
         i<num_nodes;
         i++)
      fam->meshes[mesh_id].nodes[nl_index].coords[i] = coords[i];

   fam->meshes[mesh_id].total_nodes+=num_nodes;

   fam->meshes[mesh_id].num_nl++;
   return OK;
}



/*****************************************************************
 * TAG( mc_silo_make_umesh ) PUBLIC
 *
 * Create and initialize an unstructured mesh, making it the
 * current mesh.  Return its id in a pass-by-reference argument.
 */
int
mc_silo_make_umesh( Famid famid, char *mesh_name, int dim, int *p_mesh_id )
{
   MiliSilo_family *fam;
   int i;
   int num_meshes, mesh_id;

   int  status;
   int  num_classes;
   int  *var1;
   char *var_char;
   char temp_name[32];

   fam = &family[famid];

   if ( dim != 2 && dim != 3 )
      return INVALID_DIMENSIONALITY;
   else if ( dim != fam->dimensions && fam->dimensions != 0 )
      return INVALID_DIMENSIONALITY;

   CHECK_WRITE_ACCESS( fam )

   /* Check for a naming conflict */
   num_meshes = fam->qty_meshes;
   mesh_id    = num_meshes;

   for (i=0;
         i<num_meshes;
         i++)
      if ( strcmp( fam->meshes[i].name, mesh_name ) == 0 )
         return (int) NAME_CONFLICT;

   fam->meshes[mesh_id].name = strdup(mesh_name);
   fam->meshes[mesh_id].type = UNSTRUCTURED;

   if ( fam->dimensions == 0 )
      fam->dimensions = dim;

   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Metadata", "/") ;

   sprintf(temp_name, "MESH_%d", mesh_id);
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, temp_name, ".") ;

   /* Write out the mesh Metadata */
   var_char  = &fam->meshes[mesh_id].name[0];
   status    = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "mesh_name", 0., 1, 1, strlen(var_char), DB_CHAR, var_char);

   var1      = &dim;
   status    = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "mesh_dimensions", 0., 1, 1, 1, DB_INT, var1);

   var1      = &fam->meshes[i].type;
   status    = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "mesh_type", 0., 1, 1, 1, DB_INT, var1);

   *p_mesh_id = fam->qty_meshes;
   fam->qty_meshes++;

   return OK;
}


/*****************************************************************
 * TAG( mc_def_conn_seq ) PUBLIC
 *
 * Define connectivities, materials, and part numbers for a
 * globally numbered sequential list of elements of a given
 * type.
 */
mc_silo_def_conn_seq( Famid famid, int mesh_id, char *short_name,
                      int start_el, int stop_el, int *data )
{
   MiliSilo_family *fam;
   int class_id;
   int data_len;
   int i,j;
   int el_index, num_el;
   int status;
   int superclass;
   int data_index=0, temp_index=0;

   char mesh_name[32];

   fam = &family[famid];

   if ( mesh_id > fam->qty_meshes - 1 )
      return NO_MESH;

   CHECK_WRITE_ACCESS( fam )

   class_id = mc_silo_get_class_id( famid, mesh_id, short_name, -1 );
   if ( class_id<0 )
      return NO_CLASS;

   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Metadata", "/") ;

   sprintf(mesh_name, "MESH_%d", mesh_id);
   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, mesh_name) ;

   fam->meshes[mesh_id].connects_defined = TRUE;

   el_index   = fam->meshes[mesh_id].num_el;
   num_el     = (stop_el - start_el) + 1;
   superclass = fam->meshes[mesh_id].superclass;

   data_len = conn_words[superclass];

   fam->meshes[mesh_id].elements[el_index].num_el_points = 0;
   fam->meshes[mesh_id].elements[el_index].num_el        = num_el;
   fam->meshes[mesh_id].elements[el_index].start_el      = start_el;
   fam->meshes[mesh_id].elements[el_index].stop_el       = stop_el;
   if (data_len-2>0)
   {
      fam->meshes[mesh_id].elements[el_index].elements = (int *) malloc(num_el*(data_len-2)*sizeof(int));
      fam->meshes[mesh_id].elements[el_index].mats     = (int *) malloc(num_el*sizeof(int));
      fam->meshes[mesh_id].elements[el_index].parts    = (int *) malloc(num_el*sizeof(int));
   }

   data_index = 0;
   temp_index = 0;

   for (i=0;
         i<num_el;
         i++)
   {
      for (j=0;
            j<data_len-2;
            j++)
         fam->meshes[mesh_id].elements[el_index].elements[temp_index++] = data[data_index++];
      fam->meshes[mesh_id].elements[el_index].mats[i]                = data[data_index++];
      fam->meshes[mesh_id].elements[el_index].parts[i]               = data[data_index++];
   }

   fam->meshes[mesh_id].total_elements+=num_el;
   fam->meshes[mesh_id].elements[el_index].num_el_points = temp_index;

   fam->meshes[mesh_id].num_el++;
   return OK;
}



/*****************************************************************
 * TAG( mc_silo_def_conn_arb ) PUBLIC
 *
 * Define connectivities, materials, and part numbers for
 * an arbitrary list of elements of a given type.
 */
int
mc_silo_def_conn_arb( Famid famid, int mesh_id, char *short_name,
                      int el_qty, int *elem_ids, int *data )
{
   MiliSilo_family *fam;
   int class_id;
   int *p_int_blk, *p_iblk, *ibound;
   int data_len;
   int blk_qty;
   int el_index;
   int i, j;
   int rval, status;
   int superclass;

   char mesh_name[32];
   int data_index, temp_index;

   fam = &family[famid];

   CHECK_WRITE_ACCESS( fam )

   /* if ( mesh_id > fam->qty_meshes - 1 )
           return NO_MESH;
   */

   class_id = mc_silo_get_class_id( famid, mesh_id, short_name, -1 );
   if ( class_id<0 )
      return NO_CLASS;

   /* Convert 1D list to 2D block list. */
   status = list_to_blocks( el_qty, elem_ids, &p_int_blk, &blk_qty );
   if (status != OK)
   {
      return status;
   }

   /* Keep track of elements defined. */
   ibound = p_int_blk + blk_qty * 2;
   for ( p_iblk = p_int_blk; p_iblk < ibound; p_iblk += 2 )
   {
      status = insert_range(fam->meshes[mesh_id].p_el, *p_iblk, *(p_iblk + 1) );
      if ( rval == OBJECT_RANGE_OVERLAP )
         return ( status );
      else
         return ( OK );
   }

   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Metadata", "/") ;

   sprintf(mesh_name, "MESH_%d", mesh_id);
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, mesh_name, NULL) ;

   fam->meshes[mesh_id].connects_defined = TRUE;

   el_index   = fam->meshes[mesh_id].num_el;
   superclass = fam->meshes[mesh_id].superclass;

   data_len = conn_words[superclass];

   fam->meshes[mesh_id].elements[el_index].num_el   = el_qty;
   fam->meshes[mesh_id].elements[el_index].start_el = -1;
   fam->meshes[mesh_id].elements[el_index].stop_el  = -1;

   if (data_len-2>0)
   {
      fam->meshes[mesh_id].elements[el_index].elements = (int *) malloc(el_qty*(data_len-2)*sizeof(int));
      fam->meshes[mesh_id].elements[el_index].mats     = (int *) malloc(el_qty*sizeof(int));
      fam->meshes[mesh_id].elements[el_index].parts    = (int *) malloc(el_qty*sizeof(int));
      fam->meshes[mesh_id].elements[el_index].el_ids   = (int *) malloc(el_qty*sizeof(int));
   }

   data_index = 0;
   temp_index = 0;

   for (i=0;
         i<el_qty;
         i++)
   {
      for (j=0;
            j<data_len-2;
            j++)
         fam->meshes[mesh_id].elements[el_index].elements[temp_index++] = data[data_index++];
      fam->meshes[mesh_id].elements[el_index].mats[i]   = data[data_index++];
      fam->meshes[mesh_id].elements[el_index].parts[i]  = data[data_index++];
      fam->meshes[mesh_id].elements[el_index].el_ids[i] = elem_ids[data_index++];
   }

   fam->meshes[mesh_id].total_elements+=el_qty;

   fam->meshes[mesh_id].num_el++;

   return OK;
}



/*****************************************************************
 * TAG( mc_silo_write_umesh ) PRIVATE
 *
 * Once all of the data is gatheed to represent the umesh this
 * function will write it out to the Silo file.
 */
int
mc_silo_write_umesh( Famid famid, int mesh_id, int class_id )
{
   MiliSilo_family *fam;
   int coord_index=0, num_coord_points;
   int data_len;
   int group_num;
   int i,j,k;
   int num_nodes=0, num_els=0;
   int node_id=0, el_id=0, el_index;
   int el_type, tot_els;
   int local_id;
   double time=1.0, time_step ;

   char mesh_name[32];

   float  *coord_x, *coord_y, *coord_z;
   float  *coords[3];
   char   *coord_names[3] = {"X Coord", "Y Coord", "Z Coord"};

   /* Material Variables */
   int   *mat_zone_list, *part_zone_list;
   int   mat, matnum, num_mats;
   int   *mat_nums, *part_nums;
   int   mat_count=0;
   int   *node_list, nl_length=0, zl_length=0;
   char  *mat_names[20], matname[64];
   Bool_type mat_found = FALSE;


   /* Mesh variables */
   int    mm_count=0;
   int    num_meshes = 0, meshtypes[200], zonecounts[400];
   double extents[120];
   char   *meshnames[400];
   char   temp_string[400];
   int    *global_node_ids, *global_zone_ids;
   int    zl_number=0;
   int    superclass;

   int    status;

   fam = &family[famid];

   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, "/Metadata");

   sprintf(mesh_name, "/MESH_%d", mesh_id);
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, mesh_name, NULL) ;

   num_nodes  = fam->meshes[mesh_id].total_nodes;
   num_els    = fam->meshes[mesh_id].total_elements;
   superclass = fam->meshes[mesh_id].superclass;
   data_len = conn_words[superclass];

   coord_x   = (float *) calloc(num_nodes,sizeof(double));
   coord_y   = (float *) calloc(num_nodes,sizeof(double));
   coord_z   = (float *) calloc(num_nodes,sizeof(double));

   mat_zone_list   = (int *)   calloc(num_els,sizeof(int));
   part_zone_list  = (int *)   calloc(num_els,sizeof(int));
   mat_nums        = (int *)   calloc(1000,sizeof(int));
   part_nums       = (int *)   calloc(1000,sizeof(int));
   node_list       = (int *)   calloc(num_nodes,sizeof(int));
   global_node_ids =  (int *) calloc(num_nodes,sizeof(int));
   global_zone_ids =  (int *) calloc(num_els,sizeof(int));

   coords[X] = coord_x;
   coords[Y] = coord_y;
   coords[Z] = coord_z;


   data_len = conn_words[superclass];

   /* Look through the nodal data */
   local_id =0;
   for (i=0;
         i<fam->meshes[mesh_id].num_nl;
         i++)
   {
      num_coord_points = fam->meshes[mesh_id].dim * fam->meshes[mesh_id].nodes[i].num_nodes;
      node_id          = fam->meshes[mesh_id].dim * fam->meshes[mesh_id].nodes[i].start_node;

      for (j=0;
            j<num_coord_points;
            j++)
      {
         coord_x[coord_index] = fam->meshes[mesh_id].nodes[i].coords[j];
         coord_y[coord_index] = fam->meshes[mesh_id].nodes[i].coords[j+1];
         if ( fam->meshes[mesh_id].dim == 3 )
            coord_y[coord_index] = fam->meshes[mesh_id].nodes[i].coords[j+2];

         if ( fam->meshes[mesh_id].dim == 3 )
            j+=3;
         else
            j+=2;

         global_node_ids[nl_length++] = node_id++;
      }
   }

   /* Look through the element data */
   for (i=0;
         i<fam->meshes[mesh_id].num_el;
         i++)
   {
      num_els = fam->meshes[mesh_id].dim * fam->meshes[mesh_id].elements[i].num_el;
      el_id   = fam->meshes[mesh_id].dim * fam->meshes[mesh_id].elements[i].start_el;
      el_index = 0;

      for (j=0;
            j<num_els;
            j++)
      {
         mat_zone_list[zl_length]  = fam->meshes[mesh_id].dim * fam->meshes[mesh_id].elements[i].mats[j];
         part_zone_list[zl_length] = fam->meshes[mesh_id].dim * fam->meshes[mesh_id].elements[i].parts[j];
         el_index = 0;

         for ( k=0;
               k<data_len;
               k++)
         {
            node_list[node_id++] = fam->meshes[mesh_id].dim * fam->meshes[mesh_id].elements[i].elements[el_index++];
         }

         if  ( fam->meshes[mesh_id].dim * fam->meshes[mesh_id].elements[i].sequential )
            global_zone_ids[zl_length++] = el_id++;
         else
            global_zone_ids[zl_length++] = fam->meshes[mesh_id].elements[i].el_ids[j];
      }
   }


   /* Construct a list of unique material numbers */

   for (i=0;
         i<zl_length;
         i++)
   {
      mat_found = FALSE;
      for (j=0;
            j<num_mats;
            j++)
         if (mat_nums[j] = mat_zone_list[i])
            mat_found = TRUE;

      if (!mat_found)
         mat_nums[num_mats++] = mat_zone_list[i];
   }


   switch (fam->meshes[mesh_id].superclass)
   {
      case M_HEX:
         el_type = ELEM_BRICK;
         break;

      case M_BEAM:
         el_type = ELEM_BEAM;
         break;

      case M_TRUSS:
         el_type = ELEM_TRUSS;
         break;

      case M_QUAD:
         el_type = ELEM_SHELL;
         break;

      case M_TET:
         el_type = ELEM_TET;
         break;

      case M_WEDGE:
         el_type = ELEM_WEDGE;
         break;

      case M_PYRAMID:
         el_type = ELEM_PYRAMID;
         break;
   }

   group_num = 1;
   zl_number = 1;

   status = SiloLib_Write_UCD_Mesh(famid,
                                   SILO_PLOTFILE,
                                   mesh_name,
                                   el_type,
                                   group_num,
                                   3,
                                   node_list,
                                   nl_length,
                                   coord_names,
                                   (void *) coords,
                                   num_nodes,
                                   num_els,
                                   global_node_ids,
                                   global_zone_ids,
                                   zl_number,
                                   0.0,
                                   time,
                                   0,
                                   DB_FLOAT) ;

   /* Write out the materials */
   status = SiloLib_Write_Material(famid,
                                   SILO_PLOTFILE,
                                   mesh_name,
                                   "Material",
                                   &num_els,
                                   num_mats,
                                   mat_nums,
                                   mat_zone_list,
                                   0.0,
                                   time,
                                   0,
                                   DB_DOUBLE) ;
   return OK;
}

/* End of mc_silo_mesh.c */
