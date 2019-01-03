#include "pasit.h"
#include "pasitP.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <values.h>
#include "silo.h"

overlink_data*   MapStresslinkToOverlink(stresslink_data *);
stresslink_data* MapOverlinkToStresslink(overlink_data *);

int              get_mixed_val( int, int, int *, int **, double **,
                                double *, double *,
                                double *, double *, double *);


/*
 * Convert the given overlink file to stresslink format
 */
int OverlinkToStresslink(const char* olinkfilename,
                         const char* slinkfilename)
{
   overlink_data   *ol[1];
   stresslink_data *sl[1];


   int err;

   ol[0] = read_overlink(olinkfilename, &err);
   sl[0] = MapOverlinkToStresslink( ol[0] );

   write_stresslink(sl[0], slinkfilename, &err);

   stresslink_data_destroy( sl[0] );
   overlink_data_destroy( ol[0] );

   return OK;
}


/*
 * Convert the given stresslink file to overlink format
 */
int StresslinkToOverlink(const char* slinkfilename,
                         const char* olinkfilename)
{
   stresslink_data *sl[1];
   overlink_data   *ol[1];

   int err;

   sl[0] = read_stresslink(slinkfilename, &err);
   ol[0] = MapStresslinkToOverlink( sl[0] );

   write_overlink(ol[0], olinkfilename, &err);

   stresslink_data_destroy( sl[0] );
   overlink_data_destroy( ol[0] );

   return OK;
}

/*
 * Stresslink reader/writer loop test - used for debugging
 */
int StresslinkToStresslink(const char* slinkfilename_in,
                           const char* slinkfilename_out)
{
   stresslink_data *sl[1];

   int err;

   sl[0] = read_stresslink(slinkfilename_in, &err);

   write_stresslink(sl[0], slinkfilename_out, &err);

   stresslink_data_destroy( sl[0] );

   return OK;
}

/*
 * Overlink reader/writer loop test - used for debugging
 */
int OverlinkToOverlink(const char* olinkfilename_in,
                       const char* olinkfilename_out)
{
   overlink_data *ol[1];

   int err;

   ol[0] = read_overlink(olinkfilename_in, &err);

   write_overlink(ol[0], olinkfilename_out, &err);

   overlink_data_destroy( ol[0] );

   return OK;
}

/*
 * Stresslink dump function - used for debugging
 */
int DumpStresslink(const char* slinkfilename_in,
                   const char* slinkfilename_out)
{
   stresslink_data *sl[1];

   int err;

   sl[0] = read_stresslink(slinkfilename_in, &err);

   write_stresslink_text(sl[0], slinkfilename_out, &err);

   stresslink_data_destroy( sl[0] );

   return OK;
}

/*
 * Overlink dump function - used for debugging
 */
int DumpOverlink(const char* olinkfilename_in,
                 const char* olinkfilename_out)
{
   overlink_data *ol[1];

   int err;

   ol[0] = read_overlink(olinkfilename_in, &err);

   write_overlink_text(ol[0], olinkfilename_out, &err);

   overlink_data_destroy( ol[0] );

   return OK;
}

/*********************************************************
 * Stresslink data -> Overlink data
 * Copies a Stresslink data structure to an Overlink data
 * structure.
 *********************************************************/
overlink_data* MapStresslinkToOverlink(stresslink_data *sl)
{
   int i, j, idxN=0;
   int numNodalVars=0;
   int numZonalVars=0;
   int numVars=0;

   int num_node_sets=0, num_element_sets=0;
   int num_nodes=0,nnodes=0, nnips=0,
       num_elements=0, total_elements=0;

   int nips=0, nhist=0;

   int mat_index=0;
   int hist_index=0, nip_index=0, elem_index=0, node_index=0;
   int ol_conn_index=0, sl_conn_index=0;

   int temp_conn[8], *save_conn;

   overlink_data *ol;

   char *tempVarNames[1000];

   double p; /* Pressure */

   /* Label Vars */
   int label_index, *label_map, label_min=0, label_max=0, label_offset=0, node_num=0;

   ol = overlink_data_new();

   ol->num_node_sets    = sl->num_node_sets;
   num_node_sets            = ol->num_node_sets;
   ol->node_sets        = (ol_node_set *) malloc(num_node_sets*sizeof(ol_node_set));

   num_element_sets         = sl->num_element_sets;

   ol->num_element_sets = 1; /* Only one element set in OL structure */
   ol->element_sets     = (ol_element_set *) malloc(1*sizeof(ol_element_set));

   /*******************************************/
   /* Map the Node+Element Set Variable Names */
   /*******************************************/

   /* Copy Variables that are allocated into overlink structure */

   /* Count the Nodal Vars */
   idxN=0;
   numNodalVars=0;
   numZonalVars=0;

   tempVarNames[idxN++] = strdup("cur_coord");
   numNodalVars++;

   if (sl->node_sets[0].t_disp != NULL)
   {
      tempVarNames[idxN++] = strdup("t_disp");
      numNodalVars++;
   }
   if (sl->node_sets[0].r_disp != NULL)
   {
      tempVarNames[idxN++] = strdup("r_disp");
      numNodalVars++;
   }
   if (sl->node_sets[0].t_vel != NULL)
   {
      tempVarNames[idxN++] = strdup("t_vel");
      numNodalVars++;
   }
   if (sl->node_sets[0].r_vel != NULL)
   {
      tempVarNames[idxN++] = strdup("r_vel");
      numNodalVars++;
   }
   if (sl->node_sets[0].t_acc != NULL)
   {
      tempVarNames[idxN++] = strdup("t_acc");
      numNodalVars++;
   }
   if (sl->node_sets[0].r_acc!= NULL)
   {
      tempVarNames[idxN++] = strdup("r_acc");
      numNodalVars++;
   }

   /* Nodal Annotation Vars */
   tempVarNames[idxN++] = strdup("node_labels");
   numNodalVars++;

   /* Count the Zonal Vars */

   /* Zonal Annotation Vars */
   tempVarNames[idxN++] = strdup("material");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("init_conn");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("elem_labels");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("vol_ref");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("Pev");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("elem_control");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("sx");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("sy");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("sz");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("sxy");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("syz");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("szx");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("pressure");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("eps");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("E");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("q");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("vol");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("Pev");
   numZonalVars++;

   tempVarNames[idxN++] = strdup("e");
   numZonalVars++;

   numVars = numNodalVars + numZonalVars;

   ol->node_sets[0].numVars    = numNodalVars;
   ol->element_sets[0].numVars = numZonalVars;
   ol->numVars                 = numVars;

   ol->varNames                = (char **) malloc(numVars*2*sizeof(char *));
   for (i=0; i<numVars; i++)
      ol->varNames[i] = strdup(tempVarNames[i]);

   ol->node_sets[0].varSet    = (ol_var*)malloc(numNodalVars*2*sizeof(ol_var));
   ol->element_sets[0].varSet = (ol_var*)malloc(numZonalVars*2*sizeof(ol_var));


   /*************************/
   /* Map the Node Set data */
   /*************************/

   ol->node_sets[0].num_nodes = sl->node_sets[0].num_nodes;

   num_nodes = sl->node_sets[0].num_nodes;

   /* Build the label map */
   label_map = (int *) malloc(num_nodes*sizeof(int));

   /* First find label range */
   label_max = -1;
   label_min = MAXINT;
   for (i=0;
         i<num_nodes;
         i++)
   {
      if ( sl->node_sets[0].labels[i]>label_max)
         label_max = sl->node_sets[0].labels[i];
      if ( sl->node_sets[0].labels[i]<label_min)
         label_min = sl->node_sets[0].labels[i];
   }
   label_offset = label_min-1;

   for (i=0;
         i<num_nodes;
         i++)
   {
      label_index = sl->node_sets[0].labels[i] - label_offset;
      label_map[label_index-1] = i+1;
   }


   idxN = 0;

   ol->node_sets[0].labels = (int *) malloc(num_nodes*sizeof(int));
   for ( i=0; i<num_nodes; i++ )
   {
      ol->node_sets[0].labels[i] = sl->node_sets[0].labels[i];
   }

   for (i=0; i<3; i++)
   {
      ol->node_sets[0].cur_coord[i] =
         (double*)malloc(num_nodes*sizeof(double));
   }

   ol_conn_index = 0;
   for (j=0; j<num_nodes; j++)
   {
      for (i=0; i<3; i++)
      {
         ol->node_sets[0].cur_coord[i][j] =
            (double)sl->node_sets[0].cur_coord[ol_conn_index++];
      }
   }

   if (sl->node_sets[0].cur_coord != NULL)
   {
      ol->node_sets[0].varSet[idxN].lenval = num_nodes*3;
      ol->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol->node_sets[0].varSet[idxN].varName         = strdup("cur_coord");
      ol->node_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
      ol->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol->node_sets[0].varSet[idxN].mixlen          = 0;
      ol->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].cur_coord[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].t_disp != NULL)
   {
      ol->node_sets[0].varSet[idxN].lenval = num_nodes*3;
      ol->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol->node_sets[0].varSet[idxN].varName         = strdup("t_disp");
      ol->node_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
      ol->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol->node_sets[0].varSet[idxN].mixlen          = 0;
      ol->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].t_disp[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].r_disp != NULL)
   {
      ol->node_sets[0].varSet[idxN].lenval = num_nodes*3;
      ol->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol->node_sets[0].varSet[idxN].varName         = strdup("r_disp");
      ol->node_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
      ol->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol->node_sets[0].varSet[idxN].mixlen          = 0;
      ol->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].r_disp[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].t_vel != NULL)
   {
      ol->node_sets[0].varSet[idxN].lenval = num_nodes*3;
      ol->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol->node_sets[0].varSet[idxN].varName         = strdup("t_vel");
      ol->node_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
      ol->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol->node_sets[0].varSet[idxN].mixlen          = 0;
      ol->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].t_vel[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].r_vel != NULL)
   {
      ol->node_sets[0].varSet[idxN].lenval = num_nodes*3;
      ol->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol->node_sets[0].varSet[idxN].varName         = strdup("r_vel");
      ol->node_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
      ol->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol->node_sets[0].varSet[idxN].mixlen          = 0;
      ol->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].r_vel[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].t_acc != NULL)
   {
      ol->node_sets[0].varSet[idxN].lenval = num_nodes*3;
      ol->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol->node_sets[0].varSet[idxN].varName         = strdup("t_acc");
      ol->node_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
      ol->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol->node_sets[0].varSet[idxN].mixlen          = 0;
      ol->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].t_acc[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].r_acc != NULL)
   {
      ol->node_sets[0].varSet[idxN].lenval = num_nodes*3;
      ol->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol->node_sets[0].varSet[idxN].varName         = strdup("r_acc");
      ol->node_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
      ol->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol->node_sets[0].varSet[idxN].mixlen          = 0;
      ol->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].r_acc[i];
      }
      idxN++;
   }

   /* Nodal Annotation Variables */

   if (sl->node_sets[0].labels != NULL)
   {
      ol->node_sets[0].varSet[idxN].lenval = num_nodes;
      ol->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*sizeof(double));
      ol->node_sets[0].varSet[idxN].varName         = strdup("node_labels");
      ol->node_sets[0].varSet[idxN].datatype        = DB_INT;
      ol->node_sets[0].varSet[idxN].annotation_flag = TRUE;
      ol->node_sets[0].varSet[idxN].mixlen          = 0;
      ol->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes; i++ )
      {
         ol->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].labels[i];
      }
      idxN++;
   }

   /****************************/
   /* Map the Element Set data */
   /****************************/

   /* Map the connectivity */

   nnodes = 8;
   for ( i = 0; i < num_element_sets; i++ )
   {
      total_elements += sl->element_sets[i].num_elements;
   }

   ol->element_sets[0].num_elements = total_elements;

   ol->element_sets[0].conn =
      (int*)malloc(total_elements*nnodes*sizeof(int));
   save_conn =
      (int*)malloc(total_elements*nnodes*sizeof(int));

   ol_conn_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      for ( j = 0; j < sl->element_sets[i].num_elements*nnodes; j++ )
      {

         /* Update connectivity with labels */
         node_num = label_map[sl->element_sets[i].conn[j]-1];
         ol->element_sets[0].conn[ol_conn_index] = node_num-1;
         save_conn[ol_conn_index]                    = node_num;
         ol_conn_index++;
      }
   }

   /* Update connectivity with labels */

   /* Translate connectivity to right-hand format */
   for ( i = 0; i < total_elements; i++ )
   {
      for ( j = 0; j<8; j++ )
         temp_conn[j] = ol->element_sets[0].conn[node_index+j];

      /* ol->element_sets[0].conn[node_index+0] = temp_conn[0];
           ol->element_sets[0].conn[node_index+1] = temp_conn[4];
           ol->element_sets[0].conn[node_index+2] = temp_conn[5];
           ol->element_sets[0].conn[node_index+3] = temp_conn[1];
           ol->element_sets[0].conn[node_index+4] = temp_conn[3];
           ol->element_sets[0].conn[node_index+5] = temp_conn[7];
           ol->element_sets[0].conn[node_index+6] = temp_conn[6];
           ol->element_sets[0].conn[node_index+7] = temp_conn[2]; */

      ol->element_sets[0].conn[node_index+0] = temp_conn[0];
      ol->element_sets[0].conn[node_index+1] = temp_conn[1];
      ol->element_sets[0].conn[node_index+2] = temp_conn[2];
      ol->element_sets[0].conn[node_index+3] = temp_conn[3];
      ol->element_sets[0].conn[node_index+4] = temp_conn[4];
      ol->element_sets[0].conn[node_index+5] = temp_conn[5];
      ol->element_sets[0].conn[node_index+6] = temp_conn[6];
      ol->element_sets[0].conn[node_index+7] = temp_conn[7];
      node_index+=8;
   }

   /* Map the materials (regions) */
   ol->element_sets[0].numRegs  = num_element_sets;

   ol->element_sets[0].numMixEntries  = 0;
   ol->element_sets[0].mixVF          = NULL;
   ol->element_sets[0].mixMat         = NULL;
   ol->element_sets[0].regionId       = (int*)malloc(num_element_sets*sizeof(int));
   for ( mat_index = 0; mat_index < num_element_sets; mat_index++ )
   {
      ol->element_sets[0].regionId[mat_index] = mat_index+1;
   }

   ol->element_sets[0].matList = (int*)malloc(total_elements*sizeof(int));

   /* Load  ol->element_sets[0].matList with region numbers */
   elem_index = 0;
   for ( mat_index = 0; mat_index < num_element_sets; mat_index++ )
   {
      for ( j = 0; j < sl->element_sets[mat_index].num_elements; j++ )
      {
         ol->element_sets[0].matList[elem_index++] = mat_index+1;
      }
   }

   ol->element_sets[0].numMixEntries  = 0;
   ol->element_sets[0].mixVF          = NULL;
   ol->element_sets[0].mixNext        = NULL;
   ol->element_sets[0].mixMat         = NULL;
   ol->element_sets[0].mixZone        = NULL;

   /* For now the Overlink side only uses one Element set - in the future
    * when we support multiple element types we will need more - IRC
    */
   ol->element_sets[0].nnodes    = sl->element_sets[0].nnodes;
   nnodes                            = sl->element_sets[0].nnodes;

   ol->element_sets[0].nips      = sl->element_sets[0].nips;
   nnips                             = sl->element_sets[0].nips;

   ol->element_sets[0].nhist     = sl->element_sets[0].nhist;
   nhist                             = sl->element_sets[0].nhist;

   ol->element_sets[0].type      = sl->element_sets[0].type;

   ol->element_sets[0].nhist     = sl->element_sets[0].nhist;
   nhist                             = sl->element_sets[0].nhist;

   /* Map the Zonal Variables */

   idxN = 0;

   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*nnodes*sizeof(double));
   ol->element_sets[0].varSet[idxN].varName         = strdup("material");
   ol->element_sets[0].varSet[idxN].datatype        = DB_INT;
   ol->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   for ( i = 0; i < total_elements; i++ )
   {
      ol->element_sets[0].varSet[idxN].val[i] = (double) ol->element_sets[0].matList[i];
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements*nnodes;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*nnodes*sizeof(double));
   ol->element_sets[0].varSet[idxN].varName         = strdup("init_conn");
   ol->element_sets[0].varSet[idxN].datatype        = DB_INT;
   ol->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   for ( i = 0; i < total_elements*nnodes; i++ )
   {
      ol->element_sets[0].varSet[idxN].val[i] = (double) save_conn[i];
   }
   idxN++;

   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   ol->element_sets[0].varSet[idxN].varName         = strdup("elem_labels");
   ol->element_sets[0].varSet[idxN].datatype        = DB_INT;
   ol->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   /* Since labels are for each element set we need to break up into
    * element set arrays.
    */
   elem_index=0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].labels[j];
      }
   }
   idxN++;

   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("vol_ref");
   ol->element_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
   ol->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   /* Since vol_ref are for each element set we need to break up into
    * element set arrays.
    */
   elem_index=0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].vol_ref[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("Pev");
   ol->element_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
   ol->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index=0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      if (sl->element_sets[i].mat11_Pev)
         for ( j=0; j<num_elements; j++ )
         {
            if (validMatModel(sl->element_sets[i].mat_model))
               ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_Pev[j];
         }
   }
   idxN++;

   ol->element_sets[0].varSet[idxN].lenval          = 15*num_element_sets;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(15*num_element_sets*sizeof(double));
   ol->element_sets[0].varSet[idxN].varName         = strdup("elem_control");
   ol->element_sets[0].varSet[idxN].datatype        = DB_DOUBLE;
   ol->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   /* Since element control is for each element set we need to break up into
    * element set arrays.
    */
   elem_index=0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      for ( j=0; j<13; j++ )
      {
         ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].control[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("sx");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].sxx[j] +
                  (double)sl->element_sets[i].pressure[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i] = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("sy");
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].syy[j] +
                  (double)sl->element_sets[i].pressure[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i] = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("sz");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].szz[j] +
                  (double)sl->element_sets[i].pressure[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("txy");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].sxy[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("tyz");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].syz[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("tzx");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].szx[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("p");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].pressure[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("eps");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      if (sl->element_sets[i].mat11_eps)
         for ( j=0; j<num_elements; j++ )
         {
            if (validMatModel(sl->element_sets[i].mat_model))
               ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_eps[j];
         }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("q");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      if (sl->element_sets[i].mat11_Q)
         for ( j=0; j<num_elements; j++ )
         {
            if (validMatModel(sl->element_sets[i].mat_model))
               ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_Q[j];
         }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("E");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      if (sl->element_sets[i].mat11_E)
         for ( j=0; j<num_elements; j++ )
         {
            if (validMatModel(sl->element_sets[i].mat_model))
               ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_E[j];
         }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("vol");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_vol[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("pev");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_Pev[j];
      }
   }
   idxN++;


   ol->element_sets[0].varSet[idxN].lenval          = total_elements;
   ol->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol->element_sets[0].varSet[idxN].varName         = strdup("e");
   ol->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol->element_sets[0].varSet[idxN].mixlen          = 0;
   ol->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_eos[j];
      }
   }

   /* Update num vars with true count */
   ol->element_sets[0].numVars = idxN;

   /* Release temporary storage */
   free(label_map);
   for (i=0; i<numVars; i++)
      free(tempVarNames[i]);

   return (ol);
}


/***********************************************************
 * Overlink data -> Stresslink data
 * Copies a Overlink data structure to an Stresslink data
 * structure.
 */
stresslink_data* MapOverlinkToStresslink(overlink_data *ol)
{
   int i, j, coord_index;

   int num_node_sets=0, num_element_sets=0;
   int num_nodes=0, element_number=0, num_elements=0, total_elements=0,
       num_elements_per_mat=0;
   int *material_numbers=NULL;

   int elem_index, vel_index=0;
   int num_mats=0, num_sl_mats=0, mat_index=0, mat_index_temp=0;
   int nthick=0;

   int node_index=0;

   int *temp_conn, init_conn_found=FALSE;
   int *temp_node_labels=NULL, *temp_elem_labels=NULL;

   double *temp_control=NULL;
   int   temp_control_index=0;

   double **mixed_element_vf;
   int    *mixed_element_mat_count;
   int    mix_index=0, mix_next_index=0;
   int    *num_elem_region;
   double sum_vf=0.0, max_vf=0.0, p,
          mixed_val=0.0, avg_val=0.0, weight_val=0.0;
   int    max_elem=0;
   int    *elem_region;
   int    sl_conn_index=0, ol_conn_index=0;

   int   numNodalVars, numZonalVars, numVars;

   stresslink_data *sl;
   int              sl_hist_index=0;

   float f1=0.0, f2=0.0;

   int   temp_cnt=0;

   double *x_temp, *y_temp, *z_temp, *rhon_temp;
   int    rhon_found=FALSE;

   char varName[64];

   int **mixed_element_vals_index; /* Mixed values mapping (indexes): [numZones:numMats] */

   int *warning_count_per_mat; /* Used to limit number of Warning messages that print */

   int status = OK;

   int use_mixed=TRUE;

   int mat_model    = 11; /* Default Mat Model */
   int nips         = 1;  /* Default NIPS */
   int nhist        = 12; /* Default nhist */
   int nnodes       = 8;  /* Num nodes per element */
   int sub_assembly = 1;
   num_mats         = ol->element_sets[0].numRegs;
   num_nodes        = ol->node_sets[0].num_nodes;
   num_elements     = ol->element_sets[0].num_elements;

   /* Load Annotation variables */
   temp_conn =
      (int*)malloc(num_elements*nnodes*sizeof(int));

   material_numbers =
      (int*)malloc(num_elements*sizeof(int));

   for ( i=0; i<ol->element_sets[0].numVars; i++ )
   {

      /* Annotation variable: material */
      if ( strcmp(ol->element_sets[0].varSet[i].varName, "material" ) == 0)
      {
         for ( j=0; j<num_elements; j++ )
         {
            material_numbers[j] = (int) ol->element_sets[0].varSet[i].val[j];
         }
      }

      /* Annotation variable: init_conn */
      if ( strcmp(ol->element_sets[0].varSet[i].varName, "init_conn" ) == 0)
      {
         for ( j=0; j<num_elements*nnodes; j++ )
         {
            temp_conn[j] = (int) ol->element_sets[0].varSet[i].val[j];
         }
      }
   }

   /* First convert all mixed zones to clean zones and count number
     * of result materials.
     */

   mixed_element_vf         = (double **) malloc(num_elements*sizeof(double *));
   mixed_element_vals_index = (int **)    malloc(num_elements*sizeof(int *));
   mixed_element_mat_count  = (int *)     malloc(num_elements*sizeof(int));
   elem_region              = (int *)     malloc(num_elements*sizeof(int));
   num_elem_region          = (int *)     malloc(num_mats*sizeof(int));

   warning_count_per_mat    = (int *)     malloc(num_mats*sizeof(int));

   for (i=0; i <num_mats; i++)
   {
      num_elem_region[i]       = 0;
      warning_count_per_mat[i] = 0;
   }

   for (i=0; i <num_elements; i++)
   {
      elem_region[i] = -1;
   }
   for (i=0; i <num_elements; i++)
   {
      mixed_element_vals_index[i] = (int *) malloc(num_mats*sizeof(int *));
   }

   for (i=0; i <num_elements; i++)
   {
      elem_region[i] = -1;
   }

   /* Allocate space for mix element volume-fraction (VF) array and initialize */
   for (i=0; i <num_elements; i++)
   {
      mixed_element_vf[i]        = (double *) malloc(num_mats*sizeof(double *));
      mixed_element_mat_count[i] = 1;

      for (j=0; j<num_mats; j++)
      {
         mixed_element_vf[i][j]   = 0.0;
         mixed_element_vals_index[i][j] = -1;
      }
   }

   /* Load mixed element mapping VF array */
   for (i=0; i<num_elements; i++)
   {
      if (ol->element_sets[0].matList[i]<0)
      {
         mix_index = -(ol->element_sets[0].matList[i])-1;

         /* Get first material VF */
         mat_index = ol->element_sets[0].mixMat[mix_index]-1;
         mixed_element_vf[i][mat_index]  = ol->element_sets[0].mixVF[mix_index];
         mixed_element_vals_index[i][mat_index] = mix_index;

         for (j=mix_index; j<ol->element_sets[0].numMixEntries; j++)
         {
            mix_next_index = ol->element_sets[0].mixNext[j]-1;
            if  (mix_next_index<0)
            {
               j=ol->element_sets[0].numMixEntries;
               break;
            }
            else
            {
               mat_index = ol->element_sets[0].mixMat[mix_next_index]-1;
               mixed_element_vf[i][mat_index]   = ol->element_sets[0].mixVF[mix_next_index];
               mixed_element_vals_index[i][mat_index] = mix_next_index;
            }
         }
      }
   }

   /* Load  zone numbers for each element */
   for (i=0; i<num_elements; i++)
   {
      if (ol->element_sets[0].matList[i]>=0)
      {
         elem_region[i] = ol->element_sets[0].matList[i];
      }
   }

   /* Now look for the MAX VF for each element */
   for (i=0; i<num_elements; i++)
   {
      if (ol->element_sets[0].matList[i]<0)
      {
         max_vf = 0.0;
         sum_vf = 0.0;
         for (mat_index=0; mat_index<num_mats; mat_index++)
         {
            mixed_element_mat_count[i]++;
            sum_vf += mixed_element_vf[i][mat_index];
            if (mixed_element_vf[i][mat_index] > max_vf)
            {
               max_vf   = mixed_element_vf[i][mat_index];
               max_elem = mat_index+1;
            }
         }

         /* Now perform a sanity check of VF before turning into a clean zone */

         elem_region[i] = max_elem;

         if (sum_vf<=.99)
            printf ("\n\t *** Warning - Element %8d Total Volume VF is not 1.0 @ %e", i+1, sum_vf);

         warning_count_per_mat[max_elem]++;

         if (warning_count_per_mat[max_elem]<MAX_WARNINGS)
         {
            if (max_elem!=1)
               printf ("\n *** Warning - Element %8d Max VF is %e for Material %4d", i+1, max_vf, max_elem);

            if (sum_vf>0 && max_vf<.5)
               printf ("\n *** Warning - Element %8d Max VF is %e for Material %4d", i+1, max_vf, max_elem);
         }
      }
   }

   if (material_numbers)
   {
      for (i=0; i<num_elements; i++)
      {
         elem_region[i] = material_numbers[i];
      }
      free(material_numbers);
   }


   for (mat_index=0; mat_index<num_mats; mat_index++)
   {
      for (i=0; i<num_elements; i++)
      {
         mat_index_temp=mat_index+1;
         if (mat_index_temp==elem_region[i])
            num_elem_region[mat_index]++;
      }
   }

   /* Count actual number of materials after  removing mixed zones */
   for (mat_index=0; mat_index<num_mats; mat_index++)
   {
      if (num_elem_region[mat_index]>0)
      {
         num_sl_mats++;
      }
   }

   /* Initialize element set and map connectivity */

   sl = stresslink_data_new(ol->num_node_sets, num_sl_mats);

   sl->num_element_sets = num_sl_mats;
   num_element_sets     = ol->num_element_sets;

   sl->num_node_sets    = ol->num_node_sets;
   num_node_sets            = ol->num_node_sets;

   sl->control          = (int *) malloc(20*sizeof(int));
   for (i=0; i<20; i++)
      sl->control[i]  = 0;

   sl->control[0]       = 1;
   sl->control[1]       = num_sl_mats;
   sl->control[2]       = 0;
   sl->control[3]       = 4; /* Analysis code = Dyna3D/ParaDyn-From Pasit */

   /* Map node set data from Overlink */
   sl->node_sets[0].control   = (int *) malloc(20*sizeof(int));
   for (i=0; i<20; i++)
      sl->node_sets[0].control[i] = 0;

   sl->node_sets[0].num_nodes = ol->node_sets[0].num_nodes;

   /* Read Node Set labels as an annotation */
   for ( i=0; i<ol->node_sets[0].numVars; i++ )
   {

      /* Annotation variable: labels */
      if ( strcmp(ol->node_sets[0].varSet[i].varName, "node_labels" ) == 0)
      {
         temp_node_labels  = (int *) malloc(num_nodes*sizeof(int));
         for ( j=0; j<num_nodes; j++ )
         {
            temp_node_labels[j] = (int) ol->node_sets[0].varSet[i].val[j];
         }
      }
   }

   sl->node_sets[0].labels = (int *) malloc(num_nodes*sizeof(int));
   for ( i=0; i<num_nodes; i++ )
      if (temp_node_labels)
         sl->node_sets[0].labels[i] = temp_node_labels[i];
      else
         sl->node_sets[0].labels[i] = i+1;

   sl->node_sets[0].control[0] = 1; /* Default sub-assembly label number */
   sl->node_sets[0].control[1] = 1; /* Default node-set     label number */
   sl->node_sets[0].control[2] = num_nodes;

   sl->node_sets[0].cur_coord  = (double*)malloc(num_nodes*3*sizeof(double));
   sl->node_sets[0].init_coord = (double*)malloc(num_nodes*3*sizeof(double));

   /* Load Current Coords */
   coord_index = 0;
   for (i=0; i<num_nodes; i++)
   {
      sl->node_sets[0].cur_coord[coord_index++] = (double)ol->node_sets[0].cur_coord[0][i];
      sl->node_sets[0].cur_coord[coord_index++] = (double)ol->node_sets[0].cur_coord[1][i];
      sl->node_sets[0].cur_coord[coord_index++] = (double)ol->node_sets[0].cur_coord[2][i];
   }

   /* Load Initial Coords  = cur_coord for now */
   coord_index = 0;
   for (i=0; i<num_nodes; i++)
   {
      sl->node_sets[0].init_coord[coord_index++] = (double)ol->node_sets[0].cur_coord[0][i];
      sl->node_sets[0].init_coord[coord_index++] = (double)ol->node_sets[0].cur_coord[1][i];
      sl->node_sets[0].init_coord[coord_index++] = (double)ol->node_sets[0].cur_coord[2][i];
   }

   sl->node_sets[0].t_vel = (double*)malloc(num_nodes*3*sizeof(double));
   x_temp    = (double*)malloc(num_nodes*sizeof(double));
   y_temp    = (double*)malloc(num_nodes*sizeof(double));
   z_temp    = (double*)malloc(num_nodes*sizeof(double));
   rhon_temp = (double*)malloc(num_nodes*sizeof(double));

   /* First load RHON since we need to divide the momentum by RHON to get
    * the velocity.
    */
   for ( j=0; j<num_nodes; j++ )
      rhon_temp[i] = 1.0;

   for ( i=0; i<ol->node_sets[0].numVars; i++ )
   {
      /* xd */
      if ( strcmp(ol->node_sets[0].varSet[i].varName, "rhon" ) == 0)
      {
         rhon_found = TRUE;
         for ( j=0; j<num_nodes; j++ )
         {
            rhon_temp[j] = ol->node_sets[0].varSet[i].val[j];
         }
      }
   }

   if (!rhon_found)
      printf("\n** Warning - RHON not found xd, yd, and zd are momemtums NOT velocities.");

   /* Load Nodal variables - Nodal velocities (really momemtums): xd, yd, zd */
   for ( i=0; i<ol->node_sets[0].numVars; i++ )
   {
      /* xd */
      if ( strcmp(ol->node_sets[0].varSet[i].varName, "xd" ) == 0)
      {
         for ( j=0; j<num_nodes; j++ )
         {
            x_temp[j] = ol->node_sets[0].varSet[i].val[j];
         }
      }
   }
   for ( i=0; i<ol->node_sets[0].numVars; i++ )
   {
      /* xd */
      if ( strcmp(ol->node_sets[0].varSet[i].varName, "yd" ) == 0)
      {
         for ( j=0; j<num_nodes; j++ )
         {
            y_temp[j] = ol->node_sets[0].varSet[i].val[j];
         }
      }
   }
   for ( i=0; i<ol->node_sets[0].numVars; i++ )
   {
      /* xd */
      if ( strcmp(ol->node_sets[0].varSet[i].varName, "zd" ) == 0)
      {
         for ( j=0; j<num_nodes; j++ )
         {
            z_temp[j] = ol->node_sets[0].varSet[i].val[j];
         }
      }
   }

   vel_index = 0;
   for ( j=0; j<num_nodes; j++ )
   {
      if (rhon_temp[j]==0.0)
      {
         sl->node_sets[0].t_vel[vel_index++] = 0.0;
         sl->node_sets[0].t_vel[vel_index++] = 0.0;
         sl->node_sets[0].t_vel[vel_index++] = 0.0;
      }
      else
      {
         sl->node_sets[0].t_vel[vel_index++] = x_temp[j]/rhon_temp[j];
         sl->node_sets[0].t_vel[vel_index++] = y_temp[j]/rhon_temp[j];
         sl->node_sets[0].t_vel[vel_index++] = z_temp[j]/rhon_temp[j];
      }
   }

   free(x_temp);
   free(y_temp);
   free(z_temp);
   free(rhon_temp);

   sl->element_sets[0].nnodes = ol->element_sets[0].nnodes;

   /* Read element control variables */

   temp_control = (double*) malloc(20*num_sl_mats*sizeof(double));

   for ( i=0; i<ol->element_sets[0].numVars; i++ )
   {

      /* Annotation variable: elem_control */
      if ( strcmp(ol->element_sets[0].varSet[i].varName, "elem_control" ) == 0)
      {
         for ( j=0; j<13*num_sl_mats; j++ )
         {
            temp_control[j] = (double) ol->element_sets[0].varSet[i].val[j];
         }
      }
   }


   /* Now construct the Stress Link Element Structure */
   total_elements = 0;
   for (mat_index=0; mat_index<num_sl_mats; mat_index++)
   {

      /* If the element control was passed in as an annotation then
      * extract some key parameters.
      */
      if (temp_control)
      {
         temp_control_index = mat_index*13;
         sub_assembly = temp_control[temp_control_index+0];
         nnodes       = temp_control[temp_control_index+4];
         nips         = temp_control[temp_control_index+6];
         mat_model    = temp_control[temp_control_index+7];
         nhist        = temp_control[temp_control_index+8];
      }
      else
      {
         mat_model = 11; /* Default Mat Model */
         nips      = 1;  /* Default NIPS */
         nhist     = 12; /* Default nhist */
         nnodes    = 8;  /* Num nodes per element */
      }

      sl->element_sets[mat_index].control = (int *) malloc(20*sizeof(int));

      for (i=0; i<20; i++)
         sl->element_sets[mat_index].control[i] = 0;

      sl->element_sets[mat_index].control[0]   = sub_assembly; /* Sub-assembly number */
      sl->element_sets[mat_index].control[1]   = mat_index+1;  /* Element set label number */
      sl->element_sets[mat_index].control[2]   = SOLID;        /* Solid Element */
      sl->element_sets[mat_index].control[4]   = nnodes;
      sl->element_sets[mat_index].control[5]   = num_elem_region[mat_index];
      sl->element_sets[mat_index].control[6]   = nips;        /* num integration points */
      sl->element_sets[mat_index].control[7]   = mat_model;   /* Mat Model number */
      sl->element_sets[mat_index].control[8]   = nhist;       /* Num Hist */

      sl->element_sets[mat_index].num_elements = num_elem_region[mat_index];
      sl->element_sets[mat_index].nnodes       = nnodes;
      sl->element_sets[mat_index].nhist        = nhist;
   }


   for (mat_index=0; mat_index<num_sl_mats; mat_index++)
   {
      num_elements_per_mat = num_elem_region[mat_index];
      mat_model            = sl->element_sets[mat_index].control[7];
      nhist                = sl->element_sets[mat_index].nhist;

      /* Initialize all element set fields */
      sl->element_sets[mat_index].type         = SOLID;
      sl->element_sets[mat_index].num_elements = num_elements_per_mat;
      sl->element_sets[mat_index].len_control  = 20;
      sl->element_sets[mat_index].nips         = 1;
      nips                                     = sl->element_sets[mat_index].nips;
      sl->element_sets[mat_index].mat_model    = mat_model;

      sl->element_sets[mat_index].labels       = (int *) malloc(num_elements_per_mat*sizeof(int));
      for (j=0; j<num_elements_per_mat; j++)
         sl->element_sets[mat_index].labels[j] = j+1; /* Initialize labels with 1:n */

      sl->element_sets[mat_index].vol_ref         = (double *) malloc(nips*num_elements_per_mat*sizeof(double));
      sl->element_sets[mat_index].vol_ref_over_v  = (double *) malloc(nips*num_elements_per_mat*sizeof(double));

      for (j=0; j<nips*num_elements_per_mat; j++)
      {
         sl->element_sets[mat_index].vol_ref[j]        = 0; /* Initialize reference volume to 0. */
         sl->element_sets[mat_index].vol_ref_over_v[j] = 0; /* Initialize reference volume/v to 0. */
      }
      sl->element_sets[mat_index].pressure     = NULL;
      sl->element_sets[mat_index].sxx          = NULL;
      sl->element_sets[mat_index].syy          = NULL;
      sl->element_sets[mat_index].szz          = NULL;
      sl->element_sets[mat_index].sxy          = NULL;
      sl->element_sets[mat_index].syz          = NULL;
      sl->element_sets[mat_index].szx          = NULL;

      sl->element_sets[mat_index].sx           = NULL;
      sl->element_sets[mat_index].sy           = NULL;

      sl->element_sets[mat_index].conn =
         (int*)malloc(num_elements_per_mat*nnodes*sizeof(int));

      /* Load the connectivity */
      for (j=0; j<num_elements_per_mat*nnodes; j++)
      {
         sl->element_sets[mat_index].conn[j] = (int) ol->element_sets[0].conn[(total_elements*nnodes)+j];
      }

      for ( i=0; i<ol->element_sets[0].numVars; i++ )
      {

         /* Annotation variable: labels */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "elem_labels" ) == 0)
         {
            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl->element_sets[mat_index].labels[j] = (int) ol->element_sets[0].varSet[i].val[total_elements+j];
            }
         }

         /* Annotation variable: reference volume */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "vol_ref" ) == 0)
         {
            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl->element_sets[mat_index].vol_ref[j] = ol->element_sets[0].varSet[i].val[total_elements+j];
            }
         }

         /* Annotation variable: Pev */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "Pev" ) == 0)
         {
            sl->element_sets[mat_index].mat11_Pev = (double *) malloc(num_elements_per_mat*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl->element_sets[mat_index].mat11_Pev[j] = ol->element_sets[0].varSet[i].val[total_elements+j];
            }
         }
      }

      /* Translate connectivity from right-hand format back to SL ordering */

      node_index = 0;
      if (init_conn_found)
      {
         sl_conn_index = 0;
         for ( i=0; i<num_elements_per_mat*nnodes; i++ )
         {
            sl->element_sets[mat_index].conn[i] = temp_conn[i];
         }
      }
      else
         for ( i=0; i<num_elements_per_mat; i++ )
         {
            for ( j = 0; j<8; j++ )
            {
               temp_conn[j] = sl->element_sets[mat_index].conn[node_index+j]+1;

               /* Apply label mapping if labels are present */
               if (temp_node_labels)
               {
                  temp_conn[j] = temp_node_labels[temp_conn[j]-1];
               }
            }

            /*sl->element_sets[mat_index].conn[node_index+0] = temp_conn[0];
            sl->element_sets[mat_index].conn[node_index+1] = temp_conn[3];
            sl->element_sets[mat_index].conn[node_index+2] = temp_conn[7];
            sl->element_sets[mat_index].conn[node_index+3] = temp_conn[4];
            sl->element_sets[mat_index].conn[node_index+4] = temp_conn[1];
            sl->element_sets[mat_index].conn[node_index+5] = temp_conn[2];
            sl->element_sets[mat_index].conn[node_index+6] = temp_conn[6];
            sl->element_sets[mat_index].conn[node_index+7] = temp_conn[5];*/

            sl->element_sets[mat_index].conn[node_index+0] = temp_conn[0];
            sl->element_sets[mat_index].conn[node_index+1] = temp_conn[1];
            sl->element_sets[mat_index].conn[node_index+2] = temp_conn[2];
            sl->element_sets[mat_index].conn[node_index+3] = temp_conn[3];
            sl->element_sets[mat_index].conn[node_index+4] = temp_conn[4];
            sl->element_sets[mat_index].conn[node_index+5] = temp_conn[5];
            sl->element_sets[mat_index].conn[node_index+6] = temp_conn[6];
            sl->element_sets[mat_index].conn[node_index+7] = temp_conn[7];
            node_index+=8;
         }

      total_elements+=num_elements_per_mat;
   }


   if (temp_conn)
      free(temp_conn);


   /* Map the history variables - by region */

   numNodalVars = ol->node_sets[0].numVars;
   numZonalVars = ol->element_sets[0].numVars;
   numVars      = ol->numVars;

   total_elements = 0;

   for (mat_index=0; mat_index<num_sl_mats; mat_index++)
   {
      num_elements_per_mat = num_elem_region[mat_index];

      sl->element_sets[mat_index].pressure = NULL;

      /* We need to get pressure field first since stress' are calculated from pressure */
      for ( i=0; i<ol->element_sets[0].numVars; i++ )
      {
         /* Pressure - p */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "p" ) == 0)
         {
            sl->element_sets[mat_index].pressure = (double *) malloc(num_elements_per_mat*sizeof(double));
            elem_index = 0;

            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl->element_sets[mat_index].pressure[j] = ol->element_sets[0].varSet[i].val[total_elements+j];
            }

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].pressure[j] = mixed_val;
               }
         }
      }

      /* We first need to get sx & sy since they are used later to calculate szz */
      sl->element_sets[mat_index].sxx = NULL;
      sl->element_sets[mat_index].syy = NULL;
      sl->element_sets[mat_index].sx  = NULL;
      sl->element_sets[mat_index].sy  = NULL;

      for ( i=0; i<ol->element_sets[0].numVars; i++ )
      {

         /* Sx & Sxx */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "sx" ) == 0 &&
               sl->element_sets[mat_index].pressure )
         {
            sl->element_sets[mat_index].sxx = (double *) malloc(num_elements_per_mat*sizeof(double));
            sl->element_sets[mat_index].sx  = (double *) malloc(num_elements_per_mat*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl->element_sets[mat_index].sx[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

               sl->element_sets[mat_index].sxx[j] = ol->element_sets[0].varSet[i].val[total_elements+j] -
                                                    sl->element_sets[mat_index].pressure[j];
            }

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                  {
                     sl->element_sets[mat_index].sx[j] = mixed_val;

                     sl->element_sets[mat_index].sxx[j] = mixed_val -
                                                          sl->element_sets[mat_index].pressure[j];
                  }
               }
         }

         /* Sy & Syy */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "sy" ) == 0 &&
               sl->element_sets[mat_index].pressure )
         {
            sl->element_sets[mat_index].syy = (double *) malloc(num_elements_per_mat*sizeof(double));
            sl->element_sets[mat_index].sy  = (double *) malloc(num_elements_per_mat*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].sy[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].sy[j] = mixed_val;
               }


            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].syy[j] = ol->element_sets[0].varSet[i].val[total_elements+j] -
                                                    sl->element_sets[mat_index].pressure[j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].syy[j] = mixed_val -
                                                          sl->element_sets[mat_index].pressure[j];
               }
         }
      }

      /* Szz */
      sl->element_sets[mat_index].szz = NULL;
      if ( sl->element_sets[mat_index].pressure && sl->element_sets[mat_index].sx && sl->element_sets[mat_index].sy )
      {
         sl->element_sets[mat_index].szz = (double *) malloc(num_elements_per_mat*sizeof(double));

         for ( j=0; j<num_elements_per_mat; j++ )
         {
            p = sl->element_sets[mat_index].pressure[j];
            sl->element_sets[mat_index].szz[j] =
               -(sl->element_sets[mat_index].sx[j]) - (sl->element_sets[mat_index].sy[j]) - p;
         }
      }

      /* Now get the rest of the variables */
      sl->element_sets[mat_index].rhon      = NULL;
      sl->element_sets[mat_index].sxy       = NULL;
      sl->element_sets[mat_index].syz       = NULL;
      sl->element_sets[mat_index].szx       = NULL;
      sl->element_sets[mat_index].v         = NULL;
      sl->element_sets[mat_index].mat11_eps = NULL;
      sl->element_sets[mat_index].mat11_E   = NULL;
      sl->element_sets[mat_index].mat11_eos = NULL;
      sl->element_sets[mat_index].mat11_Q   = NULL;
      sl->element_sets[mat_index].mat11_vol = NULL;
      sl->element_sets[mat_index].mat11_Pev = NULL;

      for ( i=0; i<ol->element_sets[0].numVars; i++ )
      {

         strcpy(varName, ol->element_sets[0].varSet[i].varName);

         /* rhon */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "rhon" ) == 0)
         {
            sl->element_sets[mat_index].rhon = (double *) malloc(num_elements_per_mat*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].rhon[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].rhon[j] = mixed_val;
               }
         }

         /* Sxy */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "txy" ) == 0 &&
               sl->element_sets[mat_index].pressure )
         {
            sl->element_sets[mat_index].sxy = (double *) malloc(num_elements_per_mat*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].sxy[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].sxy[j] = mixed_val;
               }
         }

         /* Syz */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "tyz" ) == 0 &&
               sl->element_sets[mat_index].pressure )
         {
            sl->element_sets[mat_index].syz = (double *) malloc(num_elements_per_mat*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].syz[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].syz[j] = mixed_val;
               }
         }

         /* Szx */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "txz" ) == 0 &&
               sl->element_sets[mat_index].pressure )
         {
            sl->element_sets[mat_index].szx = (double *) malloc(num_elements_per_mat*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].szx[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].szx[j] = mixed_val;
               }
         }


         /**************************/
         /* Mat Model-11 Variables */
         /**************************/

         /* eps - Requires Reading Mixed component!! */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "eps" ) == 0 )
         {
            sl->element_sets[mat_index].mat11_eps = (double *) malloc(num_elements_per_mat*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl->element_sets[mat_index].mat11_eps[j] = ol->element_sets[0].varSet[i].val[total_elements+j];
            }

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].mat11_eps[j] = mixed_val;
               }
         }

         /* E - Requires Reading Mixed component!! */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "E" ) == 0 )
         {
            sl->element_sets[mat_index].mat11_E = (double *) malloc(num_elements*sizeof(double));

            if (ol->element_sets[0].varSet[i].mixlen==0)
               for ( j=0; j<num_elements_per_mat; j++ )
                  sl->element_sets[mat_index].mat11_E[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].mat11_E[j] = mixed_val;
               }
         }

         /* eos - Requires Reading Mixed component!! */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "e" ) == 0 )
         {
            sl->element_sets[mat_index].mat11_eos = (double *) malloc(num_elements*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].mat11_eos[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].mat11_eos[j] = mixed_val;
               }
         }

         /* Q */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "q" ) == 0 )
         {
            sl->element_sets[mat_index].mat11_Q = (double *) malloc(num_elements*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl->element_sets[mat_index].mat11_Q[j] = ol->element_sets[0].varSet[i].val[total_elements+j];
            }

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].mat11_Q[j] = mixed_val;
               }
         }

         /* v - relative vol  Requires Reading Mixed component!! */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "v" ) == 0 )
         {
            sl->element_sets[mat_index].v = (double *) malloc(num_elements*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].v[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].v[j] = mixed_val;
               }
         }

         /* Pev */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "Pev" ) == 0 )
         {
            sl->element_sets[mat_index].mat11_Pev = (double *) malloc(num_elements*sizeof(double));

            for ( j=0; j<num_elements_per_mat; j++ )
               sl->element_sets[mat_index].mat11_Pev[j] = ol->element_sets[0].varSet[i].val[total_elements+j];

            if (use_mixed && ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements_per_mat; j++ )
               {
                  status = get_mixed_val( (total_elements+j), num_mats, elem_region,
                                          mixed_element_vals_index, mixed_element_vf,
                                          ol->element_sets[0].varSet[i].val,
                                          ol->element_sets[0].varSet[i].mixvals,
                                          &mixed_val, &avg_val, &weight_val );
                  if (status>0)
                     sl->element_sets[mat_index].mat11_Pev[j] = mixed_val;
               }
         }
      }

      /* Compute vol (copy from vol_ref) */

      sl->element_sets[mat_index].mat11_vol = (double *) malloc(num_elements*sizeof(double));
      for ( j=0; j<num_elements_per_mat; j++ )
      {
         sl->element_sets[mat_index].mat11_vol[j] = sl->element_sets[mat_index].vol_ref[j];
      }

      /* Compute vol_ref divided by v */
      for ( j=0; j<num_elements_per_mat; j++ )
      {
         if ( sl->element_sets[mat_index].v[j] != 0.0 )
            sl->element_sets[mat_index].vol_ref_over_v[j] =
               sl->element_sets[mat_index].vol_ref[j]/sl->element_sets[mat_index].v[j];
         else
            sl->element_sets[mat_index].vol_ref_over_v[j] = sl->element_sets[mat_index].vol_ref[j];
      }

      /* Compute E */

      sl->element_sets[mat_index].mat11_E = (double *) malloc(num_elements*sizeof(double));
      if (sl->element_sets[mat_index].mat11_eos && sl->element_sets[mat_index].vol_ref &&
            sl->element_sets[mat_index].v )
      {
         for ( j=0; j<num_elements_per_mat; j++ )
         {
            if (sl->element_sets[mat_index].v[j]>0.0)
               sl->element_sets[mat_index].mat11_E[j] = (sl->element_sets[mat_index].mat11_eos[j]/
                     sl->element_sets[mat_index].v[j]) *
                     sl->element_sets[mat_index].vol_ref[j];
            else
               sl->element_sets[mat_index].mat11_E[j] = 0.0;
         }
      }
      else
      {
         for ( j=0; j<num_elements_per_mat; j++ )
            sl->element_sets[mat_index].mat11_E[j] = 0.0;
      }

      total_elements+=num_elements_per_mat;

   } /* mat loop */


   /*****************************************************/
   /* Copy result variables into history variable array */
   /*****************************************************/
   for (mat_index=0; mat_index<num_sl_mats; mat_index++)
   {
      num_elements_per_mat = num_elem_region[mat_index];
      nhist                = sl->element_sets[mat_index].nhist;

      sl->element_sets[mat_index].hist_var = (double *) malloc(num_elements_per_mat*nhist*sizeof(double));
      for ( j=0; j<num_elements_per_mat*nhist; j++ )
      {
         sl->element_sets[mat_index].hist_var[i] = 0.0;
      }

      sl_hist_index = 0;

      mat_model = sl->element_sets[mat_index].mat_model;

      for ( j=0; j<num_elements_per_mat; j++ )
      {
         if (sl->element_sets[mat_index].sxx)
            sl->element_sets[mat_index].hist_var[sl_hist_index+0] = sl->element_sets[mat_index].sxx[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+0] = 0.0;

         if (sl->element_sets[mat_index].syy)
            sl->element_sets[mat_index].hist_var[sl_hist_index+1] = sl->element_sets[mat_index].syy[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+1] = 0.0;

         if (sl->element_sets[mat_index].szz)
            sl->element_sets[mat_index].hist_var[sl_hist_index+2] = sl->element_sets[mat_index].szz[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+2] = 0.0;

         if (sl->element_sets[mat_index].sxy)
            sl->element_sets[mat_index].hist_var[sl_hist_index+3] = sl->element_sets[mat_index].sxy[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+3] = 0.0;

         if (sl->element_sets[mat_index].syz)
            sl->element_sets[mat_index].hist_var[sl_hist_index+4] = sl->element_sets[mat_index].syz[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+4] = 0.0;

         if (sl->element_sets[mat_index].szx)
            sl->element_sets[mat_index].hist_var[sl_hist_index+5] = sl->element_sets[mat_index].szx[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+5] = 0.0;

         /* Mat Model 11 History Variables */
         if (sl->element_sets[mat_index].mat11_eps && mat_model==11)
            sl->element_sets[mat_index].hist_var[sl_hist_index+6] = sl->element_sets[mat_index].mat11_eps[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+6] = 0.0;

         if (sl->element_sets[mat_index].mat11_E && mat_model==11)
            sl->element_sets[mat_index].hist_var[sl_hist_index+7] = sl->element_sets[mat_index].mat11_E[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+7] = 0.0;

         if (sl->element_sets[mat_index].mat11_Q && mat_model==11)
            sl->element_sets[mat_index].hist_var[sl_hist_index+8] = sl->element_sets[mat_index].mat11_Q[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+8] = 0.0;

         if (sl->element_sets[mat_index].vol_ref && mat_model==11)
            sl->element_sets[mat_index].hist_var[sl_hist_index+9] = sl->element_sets[mat_index].vol_ref[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+9] = 0.0;

         if (sl->element_sets[mat_index].mat11_Pev && mat_model==11)
            sl->element_sets[mat_index].hist_var[sl_hist_index+10] = sl->element_sets[mat_index].mat11_Pev[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+10] = 0.0;

         if (sl->element_sets[mat_index].mat11_eos && mat_model==11)
            sl->element_sets[mat_index].hist_var[sl_hist_index+11] = sl->element_sets[mat_index].mat11_eos[j];
         else
            sl->element_sets[mat_index].hist_var[sl_hist_index+11] = 0.0;

         sl_hist_index+=nhist;
      }
   }


   /* Release temporary storage */
   for (i=0; i <num_elements; i++)
   {
      free(mixed_element_vf[i]);
      free(mixed_element_vals_index[i]);
   }
   free(mixed_element_vf);
   free(mixed_element_vals_index);

   free(elem_region);
   free(num_elem_region);

   return (sl);
}

int get_mixed_val( int elem_num, int num_mats,
                   int *elem_region, int **mixed_vals_index, double **mixed_vals_vf,
                   double *clean_vals, double *mixed_vals,
                   double *mixed_val, double *avg_val, double *weight_val )
{
   int    i;
   int    mat_num;
   int    val_index;
   int    status=OK;

   int    num_mats_found=0;
   double vf_total=0.0, val_total=0.0, val_weight_total=0.0;
   double clean_val, mixed_diff=0.0;

   double mixed_components_vals[1000], mixed_components_vf[1000];

   int    consis_error=FALSE;

   mat_num   = elem_region[elem_num];
   val_index = mixed_vals_index[elem_num][mat_num-1];
   if ( val_index<0 )
      return ( -1 );

   clean_val  = clean_vals[elem_num];
   *mixed_val = mixed_vals[val_index];

   for (i=0;
         i<1000;
         i++)
   {
      mixed_components_vals[i] = 0.0;
      mixed_components_vf[i]   = 0.0;
   }

   for (i=0;
         i<num_mats;
         i++)
   {
      val_index = mixed_vals_index[elem_num][i];
      if ( val_index>=0 )
      {
         num_mats_found++;
         vf_total         += mixed_vals_vf[elem_num][i];
         val_weight_total += mixed_vals[val_index]*mixed_vals_vf[elem_num][i];
         val_total        += mixed_vals[val_index];

         mixed_components_vals[i] = mixed_vals[val_index];
         mixed_components_vf[i]   = mixed_vals_vf[elem_num][i];
      }
   }

   if ( vf_total<0.9999999999)
      vf_total+=1.0;

   /* Compute average and weighted average values */
   *avg_val    = val_total/num_mats_found;
   *weight_val = val_weight_total;

   /* Consistency check */
   mixed_diff = fabs( clean_val-*weight_val );
   if ( mixed_diff > .000001 )
      consis_error = TRUE;

   return ( 1 );
}


int
validMatModel(int mat_model)
{
   if ( mat_model==11 )
      return (TRUE);
   else
      return (FALSE);
}

/* End of pasit.c */
