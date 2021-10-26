
#include "pasitP.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hdf5.h"

#define CREATE_GROUP 1
#define OPEN_GROUP   2

herr_t write_hdf5_native_int( hid_t fid,
                              char *group_name,
                              int  group_create,
                              char *var_name,
                              hsize_t rank,
                              hsize_t *dims,
                              int *var );

herr_t write_hdf5_native_double( hid_t fid,
                                 char *group_name,
                                 int  group_create,
                                 char *var_name,
                                 hsize_t rank,
                                 hsize_t *dims,
                                 double *var );

/**************************************************************************
 * writestresslink.c
 * @author Elsie Pierce
 *
 * @purpose Writes a stresslink file in HDF5 format
 *
 * @param
 *
 * @param
 *
 *
 *-------------------------------------------------------------------------
 *
 * Revision History
 *  ----------------
 *
 *  12 May 2008 EMP  Initial version.
 **************************************************************************/
void write_stresslink(stresslink_data *sl,
                      const char      *slinkfilename,
                      int             *errorCode)
{
   printf("\nWriting stresslink file '%s'\n", slinkfilename);

   hid_t       property_id;
   hid_t       fid, dataset;
   hid_t       datatype;
   hid_t       dataspace, memspace;

   hsize_t     rank;
   hsize_t     dims[4];
   size_t      size;
   herr_t      status=OK;

   int         i;

   char        set_name[64];
   char        new_filename[64];
   int         status_int;

   int num_node_sets, num_nodes;
   sl_node_set *node_set;

   int num_element_sets, num_elements;
   sl_element_set *element_set;

   int control[10], limit[4];

   strcpy(new_filename, slinkfilename);
   strcat(new_filename, ".hdf");
   property_id = H5Pcreate( H5P_FILE_CREATE );
   fid = H5Fcreate (new_filename, H5F_ACC_TRUNC, property_id, H5P_DEFAULT );

   /* Write node set data */

   num_node_sets = sl->num_node_sets;

   for (i=0;
         i< num_node_sets;
         i++)
   {
      node_set  = &sl->node_sets[i];
      num_nodes = node_set->num_nodes;

      /* Construct the set name */
      sprintf( set_name, "node_set%d", i+1 );

      /* Write the control info for the next node set */
      rank    = 1;
      dims[0] = 3;
      status = write_hdf5_native_int( fid, set_name, CREATE_GROUP,
                                      "control",
                                      rank, dims, node_set->control );

      /* Write in the nodal labels for the next node set */
      if( node_set->labels )
      {
         rank    = 1;
         dims[0] = num_nodes;
         status = write_hdf5_native_int( fid, set_name, OPEN_GROUP,
                                         "label", rank, dims,
                                         node_set->labels );
      }

      /* Write in the coordinates for the next node set */
      if( node_set->init_coord )
      {
         rank    = 2;
         dims[0] = num_nodes;
         dims[1] = 3;
         status =  write_hdf5_native_double( fid, set_name, OPEN_GROUP,
                                             "init_coord", rank, dims,
                                             node_set->init_coord );
      }

      if( node_set->cur_coord )
      {
         rank = 2;
         dims[0] = num_nodes;
         dims[1] = 3;
         status =  write_hdf5_native_double( fid, set_name, OPEN_GROUP,
                                             "cur_coord", rank, dims,
                                             node_set->cur_coord );
      }

      /* Write in the displacements for the next node set */
      if( node_set->t_disp )
      {
         rank = 2;
         dims[0] = num_nodes;
         dims[1] = 3;
         status = write_hdf5_native_double( fid, set_name, OPEN_GROUP,
                                            "t_disp", rank, dims,
                                            node_set->t_disp );
      }

      /* Write in the velocities for the next node set */
      if( node_set->t_vel )
      {
         rank    = 2;
         dims[0] = num_nodes;
         dims[1] = 3;
         status = write_hdf5_native_double( fid, set_name, OPEN_GROUP,
                                            "t_vel", rank, dims,
                                            node_set->t_vel );
      }

      /* Write in the accelerations for the next node set */
      if( node_set->t_acc )
      {
         rank = 2;
         dims[0] = num_nodes;
         dims[1] = 3;
         status = write_hdf5_native_double( fid, set_name, OPEN_GROUP,
                                            "t_acc", rank, dims,
                                            node_set->t_acc );
      }
   }

   num_element_sets = sl->num_element_sets;

   for (i=0;
         i< num_element_sets;
         i++)
   {
      element_set = &sl->element_sets[i];

      /* Do not write out element sets that have no element data */
      if (element_set->num_elements==0 || !element_set->conn)
      {
         sl->num_element_sets--; /* Found an empty element set - reduce count */
         sl->control[1] = sl->num_element_sets;
         continue;
      }

      /* Construct the set name */
      sprintf( set_name, "element_set%d", i+1 );

      rank    = 1;
      dims[0] = 12;
      status = write_hdf5_native_int( fid, set_name, CREATE_GROUP,
                                      "control",
                                      rank, dims, element_set->control );

      /* Write in the element labels for the next element set */
      if( element_set->labels )
      {
         rank    = 1;
         dims[0] = element_set->num_elements;
         status = write_hdf5_native_int( fid, set_name, OPEN_GROUP,
                                         "label", rank, dims,
                                         element_set->labels );
      }

      /* Write in the connectivity for the next element set */
      if( element_set->conn )
      {
         rank    = 2;
         dims[0] = element_set->num_elements;
         dims[1] = element_set->nnodes;
         status = write_hdf5_native_int( fid, set_name, OPEN_GROUP,
                                         "conn", rank, dims,
                                         element_set->conn );
      }

      /* Write the volume  reference for the next element set */
      if( element_set->vol_ref_over_v )
      {
         rank    = 2;
         dims[0] = element_set->nips;
         dims[1] = element_set->num_elements;
         status = write_hdf5_native_double( fid, set_name, OPEN_GROUP,
                                            "vol_ref", rank, dims,
                                            element_set->vol_ref_over_v );
      }

      /* Write in the history variables for the next element set */
      if( element_set->hist_var )
      {
         rank    = 3;
         dims[0] = element_set->num_elements;
         dims[1] = element_set->nips;
         dims[2] = element_set->nhist;
         status = write_hdf5_native_double( fid, set_name, OPEN_GROUP,
                                            "hist_var", rank, dims,
                                            element_set->hist_var );
      }
   }

   /* Write control data */
   rank    = 1;
   dims[0] = 4;

   status = write_hdf5_native_int( fid, "/", OPEN_GROUP, "control",
                                   rank, dims, sl->control );
   rank    = 1;
   dims[0] = 2;

   status = write_hdf5_native_double( fid, "/", OPEN_GROUP, "state_data",
                                      rank, dims, sl->state_data );

   status = H5Fclose (fid);

   return;
}


void write_stresslink_text(stresslink_data *sl,
                           const char      *slinkfilename,
                           int             *errorCode)
{
   printf("\nWriting stresslink text file '%s'\n", slinkfilename);


   FILE        *fp;
   int         i, j, k;
   int         field_count=0;

   int num_node_sets, num_nodes;
   sl_node_set *node_set;

   int num_element_sets, num_elements;
   sl_element_set *element_set;

   int control[10], limit[4];

   /* Open the output text file */
   fp = fopen(slinkfilename, "w+");

   /* Write node set data */

   fprintf(fp, "Node Set Data:");

   num_node_sets = sl->num_node_sets;


   for (i=0;
         i< num_node_sets;
         i++)
   {
      node_set  = &sl->node_sets[i];
      num_nodes = node_set->num_nodes;

      fprintf(fp, "\tNum Nodes=%d", num_nodes);
      fprintf(fp, "\tControl=%d/%d/%d",
              node_set->control[0], node_set->control[1], node_set->control[2]);

      /* Print 10 labels per line */
      fprintf(fp, "\n\tLabels=\n\t");
      field_count=0;
      for (j=0; j<num_nodes; j+=10)
      {
         for (k=0; k<10; k++)
         {
            fprintf(fp, "%10d ", node_set->labels[field_count]);
            field_count++;
            if (field_count==num_nodes) break;
         }
         fprintf(fp, "\n\t");
      }

      /* Print 12 coords per line */
      fprintf(fp, "\n\tCurr Coords=\n\t");
      field_count=0;
      for (j=0; j<num_nodes*3; j+=3)
      {
         for (k=0; k<3; k++)
         {
            fprintf(fp, "\t\t%f ", node_set->cur_coord[field_count]);
            field_count++;
            if (field_count==num_nodes*3) break;
         }
         fprintf(fp, "\n\t");
      }
   }


   num_element_sets = sl->num_element_sets;
   fprintf(fp, "\n\tElement Set Data:");

   for (i=0;
         i< num_element_sets;
         i++)
   {
      element_set  = &sl->element_sets[i];
      num_elements = element_set->num_elements;

      fprintf(fp, "\n\tElement Set Data - Set %d:", i+1);
      fprintf(fp, "\n\tNum Elements=%d", num_elements);
      fprintf(fp, "\n\tControl= ");
      for (j=0; j<12; j++)
         fprintf(fp, "%d ", element_set->control[j]);

      /* Element Labels */
      /* Print 10 labels per line */
      fprintf(fp, "\n\n\tElement Labels=\n\t");
      field_count=0;
      for (j=0; j<num_elements; j+=10)
      {
         for (k=0; k<10; k++)
         {
            fprintf(fp, "%10d ", element_set->labels[field_count]);
            field_count++;
            if (field_count==num_nodes) break;
         }
         fprintf(fp, "\n\t");
      }

      /* Element Connectivity */
      fprintf(fp, "\n\n\tElement Connectivity=\n\t");
      field_count=0;
      for (j=0; j<num_elements*8; j+=8)
      {
         fprintf(fp, "[%8d] -> ", (field_count/8)+1);
         for (k=0; k<8; k++)
         {
            fprintf(fp, "%10d ", element_set->conn[field_count]);
            field_count++;
            if (field_count==num_elements) break;
         }
         fprintf(fp, "\n\t");
      }

      /* Element Reference Volume */
      if (element_set->vol_ref)
      {
         fprintf(fp, "\n\n\tElement Ref Vol=\n\t");
         field_count=0;
         for (j=0; j<num_elements; j+=8)
         {
            fprintf(fp, "[%8d] -> ", (field_count)+1);
            for (k=0; k<8; k++)
            {
               fprintf(fp, "%f ", element_set->vol_ref[field_count]);
               field_count++;
               if (field_count==num_elements) break;
            }
            fprintf(fp, "\n\t");
         }
      }
   }

   fclose(fp);
}


/**************************************************************************
 * write_hdf5_native_double()
 *
 * @author Elsie M Pierce
 *
 * @purpose Writes an H5 native double var from a specified group.
 *
 *
 * @param
 *   hid_t fid:
 *   char *group_name:
 *   char *var_name:
 *   hsize_t rank:
 *   hsize_t *dims:
 *   double **var,
 *   int *errorCode);
 *
 * @return
 *   herr_t status:
 *
 *
 *-------------------------------------------------------------------------
 *
 * Revision History
 *  ----------------
 *
 *  20-May-2008  EMP
 **************************************************************************/
herr_t write_hdf5_native_double( hid_t fid,
                                 char *group_name,
                                 int   create_group,
                                 char *var_name,
                                 hsize_t rank,
                                 hsize_t *dims,
                                 double *var )
{
   hid_t       dataset;
   hid_t       datatype;
   hid_t       dataspace, memspace;
   hid_t       gid;
   herr_t      status=OK;

   if ( group_name==CREATE_GROUP )
      gid = H5Gcreate(fid, group_name, 0);
   else
      gid = H5Gopen(fid, group_name);

   dataspace = H5Screate_simple( rank, dims, NULL );
   dataset   = H5Dcreate(gid, var_name, H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT);

   status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, dataspace,
                     H5P_DEFAULT, (void *)var);

   H5Dclose(dataset);
   H5Sclose(dataspace);

   if ( group_name )
      H5Gclose(gid);

   return status;
}


/**************************************************************************
 * write_hdf5_native_int()
 *
 * @author Elsie M Pierce
 *
 * @purpose Writes an H5 native integer var from a specified group.
 *
 *
 * @param
 *   hid_t fid:
 *   char *group_name:
 *   char *var_name:
 *   hsize_t rank:
 *   hsize_t *dims:
 *   int *var:
 *
 * @return herr_t:
 *
 *-------------------------------------------------------------------------
 *
 * Revision History
 *  ----------------
 *
 *  20-May-2008  EMP  Initial version.
 **************************************************************************/
herr_t write_hdf5_native_int( hid_t fid,
                              char *group_name,
                              int  group_create,
                              char *var_name,
                              hsize_t rank,
                              hsize_t *dims,
                              int *var)
{
   hid_t       dataset;
   hid_t       datatype;
   hid_t       dataspace, memspace;
   hid_t       gid;
   herr_t      status=OK;

   int         datasize;

   if ( group_create==CREATE_GROUP )
      gid = H5Gcreate(fid, group_name, 0 );
   else
      gid = H5Gopen(fid, group_name );

   dataspace = H5Screate_simple( rank, dims, NULL );
   dataset   = H5Dcreate(gid, var_name, H5T_NATIVE_INT, dataspace, H5P_DEFAULT);

   status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, dataspace,
                     H5P_DEFAULT, (void *)var);

   H5Dclose(dataset);
   H5Sclose(dataspace);

   if ( group_name )
      H5Gclose(gid);

   return status;
}

