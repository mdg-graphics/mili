/**************************************************************************
 * readstresslink.c
 * @author Bob Corey
 *
 * @purpose Reads a stresslink file in HDF5 format and populates a
 *  a stresslink data structure.
 *
 *
 * @param
 *
 * @param
 *
 * @return stresslink_data the data required to write a stresslink file
 *
 *-------------------------------------------------------------------------
 *
 * Revision History
 *  ----------------
 *
 *  05-May-2008 IRC  Initial version.
 **************************************************************************/

#include "pasitP.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hdf5.h"

extern int passit_mat_input_ignore_list[1000];

int* read_hdf5_native_int( hid_t fid,
                           char *group_name,
                           char *var_name,
                           hsize_t *dims,
                           int *errorCode);

float* read_hdf5_native_float( hid_t fid,
                               char *group_name,
                               char *var_name,
                               hsize_t *dims,
                               int *errorCode);


double* read_hdf5_native_double( hid_t fid,
                                 char *group_name,
                                 char *var_name,
                                 hsize_t *dims,
                                 int *errorCode);


stresslink_data* read_stresslink(const char *slinkfilename,
                                 int *errorCode)
{
   hid_t       fid, dataset;
   hid_t       datatype;
   hid_t       dataspace, memspace;
   hid_t       gid;

   hsize_t     dims[4];
   size_t      size;
   herr_t      status=OK;

   int         i, j;

   int ii;

   int         openError=FALSE;

   stresslink_data *sl;

   char        set_name[64];
   char        new_filename[64];
   int         status_int;

   int num_nodes=0, num_elements=0;
   int num_node_sets=0, num_element_sets=0;
   int hist_var_index=0;
   int *control;

   int element_set_count=0;

   status = H5Eset_auto(0,0);
   /* DBSetDeprecateWarnings(0); */

   /***************************/
   /* Start of ReadStresslink */
   /***************************/

   /* Open the HDF file - try several extension types */

   fid = H5Fopen (slinkfilename, H5F_ACC_RDONLY, H5P_DEFAULT); /* Try with no extension first */

   if (fid<0)
   {
      openError = TRUE;
      strcpy( new_filename, slinkfilename );
      strcat( new_filename, ".hdf" );
      fid = H5Fopen (new_filename, H5F_ACC_RDONLY, H5P_DEFAULT);
   }
   else
   {
      openError = FALSE;
   }

   if (fid<0)
   {
      openError = TRUE;
      strcpy( new_filename, slinkfilename );
      strcat( new_filename, ".HDF" );
      fid = H5Fopen (new_filename, H5F_ACC_RDONLY, H5P_DEFAULT);
   }
   else
   {
      openError = FALSE;
   }

   if (fid<0)
   {
      openError = TRUE;
      strcpy( new_filename, slinkfilename );
      strcat( new_filename, ".HDFlink" );
      fid = H5Fopen (new_filename, H5F_ACC_RDONLY, H5P_DEFAULT);
   }
   else
   {
      openError = FALSE;
   }

   if (openError)
   {
      printf("\n** Error. Cannot open file: [%s]. Aborting!!\n\n.", slinkfilename);
      exit(1);
   }


   /* Read control data */
   control = read_hdf5_native_int( fid, NULL, "control",
                                   dims, &status_int );

   num_node_sets    = control[0];
   num_element_sets = control[1];

   sl = stresslink_data_new( num_node_sets, num_element_sets );

   sl->control = read_hdf5_native_int( fid, NULL, "control",
                                       dims, &status_int );

   /* Read misc data */
   sl->state_data = (double *) read_hdf5_native_double( fid, NULL, "state_data",
                    dims, &status_int );

   printf("Reading stresslink file '%s'\n", slinkfilename);


   /**************************/
   /* Read the node set data */
   /**************************/

   for (i=0;
         i< num_node_sets;
         i++)
   {
      /* Construct the set name */
      sprintf( set_name, "/node_set%d", i+1 );

      sl->node_sets[i].control = read_hdf5_native_int( fid, set_name, "control",
                                 dims, &status_int );
      sl->node_sets[i].len_control = dims[0];

      sl->node_sets[i].num_nodes = sl->node_sets[i].control[2];

      /* Read in the nodal labels for the next node set */
      sl->node_sets[i].labels = read_hdf5_native_int( fid, set_name, "label",
                                dims, &status_int );

      /* Read in the coordinates for the next node set */
      sl->node_sets[i].init_coord = NULL;
      sl->node_sets[i].init_coord = read_hdf5_native_double( fid, set_name, "init_coord",
                                    dims, &status_int );

      sl->node_sets[i].cur_coord = NULL;
      sl->node_sets[i].cur_coord  = read_hdf5_native_double( fid, set_name, "cur_coord",
                                    dims, &status_int );

      /* Read in the displacements for the next node set */
      sl->node_sets[i].t_disp = NULL;
      sl->node_sets[i].t_disp  = read_hdf5_native_double( fid, set_name, "t_disp",
                                 dims, &status_int );
      if (status_int!=0)
         sl->node_sets[i].t_disp = NULL;

      /* Read in the velocities for the next node set */
      sl->node_sets[i].t_vel = NULL;
      sl->node_sets[i].t_vel  = read_hdf5_native_double( fid, set_name, "t_vel",
                                dims, &status_int );
      if (status_int!=0)
         sl->node_sets[i].t_vel = NULL;

      /* Read in the accelerations for the next node set */
      sl->node_sets[i].t_acc = NULL;
      sl->node_sets[i].t_acc  = read_hdf5_native_double( fid, set_name, "t_acc",
                                dims, &status_int );
      if (status_int!=0)
         sl->node_sets[i].t_acc = NULL;
   }


   /*****************************/
   /* Read the element set data */
   /*****************************/

   for (i=0;
         i< num_element_sets;
         i++)
   {

      if ( passit_mat_input_ignore_list[i+1] )
      {
         continue;
      }

      /* Construct the set name */
      sprintf( set_name, "/element_set%d", element_set_count+1 );

      sl->element_sets[element_set_count].control = read_hdf5_native_int( fid, set_name, "control",
            dims, &status_int );

      sl->element_sets[element_set_count].type          = sl->element_sets[element_set_count].control[2];
      sl->element_sets[element_set_count].nnodes        = sl->element_sets[element_set_count].control[4];
      sl->element_sets[element_set_count].num_elements  = sl->element_sets[element_set_count].control[5];
      sl->element_sets[element_set_count].nips          = sl->element_sets[element_set_count].control[6];
      sl->element_sets[element_set_count].mat_model     = sl->element_sets[element_set_count].control[7];
      sl->element_sets[element_set_count].nhist         = sl->element_sets[element_set_count].control[8];

      /* Read in the element labels for the next element set */
      sl->element_sets[element_set_count].labels = read_hdf5_native_int( fid, set_name, "label",
            dims, &status_int );
      if (status_int<0)
         sl->element_sets[element_set_count].labels = NULL;

      /* Read in the connectivity for the next element set */
      sl->element_sets[element_set_count].conn = read_hdf5_native_int( fid, set_name, "conn",
            dims, &status_int );

      /* Read in the reference volume for the next element set */
      sl->element_sets[element_set_count].vol_ref = read_hdf5_native_double( fid, set_name, "vol_ref",
            dims, &status_int );
      if (status_int<0)
         sl->element_sets[element_set_count].vol_ref = NULL;

      /* Read in the history variables for the next element set */
      sl->element_sets[element_set_count].hist_var = read_hdf5_native_double( fid, set_name, "hist_var",
            dims, &status_int );

      /* Break out the components of the history variables */

      /* We will always have stresses and pressure no matter what material models
      * are present.
      */
      sl->element_sets[element_set_count].pressure = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
      sl->element_sets[element_set_count].sxx      = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
      sl->element_sets[element_set_count].syy      = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
      sl->element_sets[element_set_count].szz      = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
      sl->element_sets[element_set_count].sxy      = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
      sl->element_sets[element_set_count].syz      = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
      sl->element_sets[element_set_count].szx      = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));

      sl->element_sets[element_set_count].mat11_eps  = NULL;
      sl->element_sets[element_set_count].mat11_E    = NULL;
      sl->element_sets[element_set_count].mat11_Q    = NULL;
      sl->element_sets[element_set_count].mat11_vol  = NULL;
      sl->element_sets[element_set_count].mat11_Pev  = NULL;
      sl->element_sets[element_set_count].mat11_eos  = NULL;

      if (sl->element_sets[element_set_count].mat_model==11)
      {
         sl->element_sets[element_set_count].mat11_eps  = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
         sl->element_sets[element_set_count].mat11_E    = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
         sl->element_sets[element_set_count].mat11_Q    = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
         sl->element_sets[element_set_count].mat11_vol  = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
         sl->element_sets[element_set_count].mat11_Pev  = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
         sl->element_sets[element_set_count].mat11_eos  = (double *) malloc(sl->element_sets[element_set_count].num_elements*sizeof(double));
      }

      hist_var_index = 0;

      for (j=0;
            j<sl->element_sets[element_set_count].num_elements;
            j++)
      {

         /* Always compute the stresses and pressure */

         sl->element_sets[element_set_count].sxx[j]       = sl->element_sets[element_set_count].hist_var[hist_var_index+0];
         sl->element_sets[element_set_count].syy[j]       = sl->element_sets[element_set_count].hist_var[hist_var_index+1];
         sl->element_sets[element_set_count].szz[j]       = sl->element_sets[element_set_count].hist_var[hist_var_index+2];
         sl->element_sets[element_set_count].sxy[j]       = sl->element_sets[element_set_count].hist_var[hist_var_index+3];
         sl->element_sets[element_set_count].syz[j]       = sl->element_sets[element_set_count].hist_var[hist_var_index+4];
         sl->element_sets[element_set_count].szx[j]       = sl->element_sets[element_set_count].hist_var[hist_var_index+5];
         sl->element_sets[element_set_count].pressure[j] = (1.0/3.0)*(sl->element_sets[element_set_count].sxx[j] +
               sl->element_sets[element_set_count].syy[j] +
               sl->element_sets[element_set_count].szz[j]);

         /* Mat model 11 */
         if (sl->element_sets[element_set_count].mat_model==11)
         {
            sl->element_sets[element_set_count].mat11_eps[j]  = sl->element_sets[element_set_count].hist_var[hist_var_index+6];
            sl->element_sets[element_set_count].mat11_E[j]    = sl->element_sets[element_set_count].hist_var[hist_var_index+7];
            sl->element_sets[element_set_count].mat11_Q[j]    = sl->element_sets[element_set_count].hist_var[hist_var_index+8];
            sl->element_sets[element_set_count].mat11_vol[j]  = sl->element_sets[element_set_count].hist_var[hist_var_index+9];
            sl->element_sets[element_set_count].mat11_Pev[j]  = sl->element_sets[element_set_count].hist_var[hist_var_index+10];
            sl->element_sets[element_set_count].mat11_eos[j]  = sl->element_sets[element_set_count].hist_var[hist_var_index+11];
         }

         hist_var_index+=sl->element_sets[element_set_count].nhist;
      }

      element_set_count++;
   }

   sl->control[1]       = element_set_count;
   sl->num_element_sets = element_set_count;

   status = H5Fclose (fid);
   return sl;
}


/**************************************************************************
 * read_hdf5_native_float()
 *
 * @author Bob Corey
 *
 * @purpose Reads an H5 native float var from a specified group.
 *
 *
 * @param
 *   hid_t fid:
 *   char *group_name:
 *   char *var_name:
 *   hsize_t *dims:
 *   float **var,
 *   int *errorCode:
 *
 * @return *float
 *
 *-------------------------------------------------------------------------
 *
 * Revision History
 *  ----------------
 *
 *  15-May-2008  IRC  Initial version.
 **************************************************************************/
float* read_hdf5_native_float( hid_t fid,
                               char *group_name,
                               char *var_name,
                               hsize_t *dims,
                               int *errorCode)
{
   hid_t       dataset;
   hid_t       datatype;
   hid_t       dataspace, memspace;
   hid_t       gid;

   herr_t      status=OK;
   int         status_int=OK;

   int         datasize;
   float       *var;

   if ( group_name )
      gid = H5Gopen(fid, group_name);
   else
      gid = fid;

   dataset   = H5Dopen(gid, var_name);
   datatype  = H5Dget_type(dataset);
   datasize  = H5Dget_storage_size(dataset)/sizeof(float);

   dataspace = H5Dget_space(dataset);

   var = (float *) malloc( (datasize+2)*sizeof(float) );

   status = H5Sget_simple_extent_dims(dataspace, dims, NULL);

   status_int = H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, dataspace,
                        H5P_DEFAULT, var);

   H5Dclose(dataset);

   if ( group_name )
      H5Gclose(gid);

   *errorCode = status_int;

   return ( var );
}

/**************************************************************************
 * read_hdf5_native_double()
 *
 * @author Bob Corey
 *
 * @purpose Reads an H5 native double var from a specified group.
 *
 *
 * @param
 *   hid_t fid:
 *   char *group_name:
 *   char *var_name:
 *   hsize_t *dims:
 *   double **var,
 *   int *errorCode:
 *
 * @return *double
 *
 *-------------------------------------------------------------------------
 *
 * Revision History
 *  ----------------
 *
 *  15-May-2008  IRC  Initial version.
 **************************************************************************/
double* read_hdf5_native_double( hid_t fid,
                                 char *group_name,
                                 char *var_name,
                                 hsize_t *dims,
                                 int *errorCode)
{
   hid_t       dataset;
   hid_t       datatype;
   hid_t       dataspace, memspace;
   hid_t       gid;

   herr_t      status=OK;
   int         status_int=OK;

   int         datasize;
   double       *var;

   if ( group_name )
      gid = H5Gopen(fid, group_name);
   else
      gid = fid;

   dataset   = H5Dopen(gid, var_name);
   datatype  = H5Dget_type(dataset);
   datasize  = H5Dget_storage_size(dataset)/sizeof(double);

   dataspace = H5Dget_space(dataset);

   var = (double *) malloc( (datasize+2)*sizeof(double) );

   status = H5Sget_simple_extent_dims(dataspace, dims, NULL);

   status_int = H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, dataspace,
                        H5P_DEFAULT, var);

   H5Dclose(dataset);

   if ( group_name )
      H5Gclose(gid);

   *errorCode = status_int;

   return ( var );
}

/**************************************************************************
 * read_hdf5_native_int()
 *
 * @author Bob Corey
 *
 * @purpose Reads an H5 native integer var from a specified group.
 *
 *
 * @param
 *   hid_t fid:
 *   char *group_name:
 *   char *var_name:
 *   hsize_t *dims:
 *   float **var,
 *   int *errorCode:
 *
 * @return *int
 *
 *-------------------------------------------------------------------------
 *
 * Revision History
 *  ----------------
 *
 *  15-May-2008  IRC  Initial version.
 **************************************************************************/
int* read_hdf5_native_int( hid_t fid,
                           char *group_name,
                           char *var_name,
                           hsize_t *dims,
                           int *errorCode)
{
   hid_t       dataset;
   hid_t       datatype;
   hid_t       dataspace, memspace;
   hid_t       gid;

   herr_t      status=OK;

   int         datasize;
   int         *var;

   int         status_int;

   if ( group_name )
      gid = H5Gopen(fid, group_name);
   else
      gid = fid;

   dataset   = H5Dopen(gid, var_name);
   datatype  = H5Dget_type(dataset);
   datasize  = H5Dget_storage_size(dataset)/sizeof(int);

   dataspace = H5Dget_space(dataset);

   var = (int *) malloc( (datasize+2)*sizeof(int)*2 );

   status_int = H5Sget_simple_extent_dims(dataspace, dims, NULL);

   status = H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, dataspace,
                    H5P_DEFAULT, var);

   H5Dclose(dataset);

   if ( group_name )
      H5Gclose(gid);

   *errorCode = status_int;

   return ( var );
}

