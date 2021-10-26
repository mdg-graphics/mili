#include "socitume.h"
#include "socitumeP.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <values.h>
#include "silo.h"

overlink_data*   MapStresslinkToOverlink(stresslink_data *);
stresslink_data* MapOverlinkToStresslink(overlink_data *);

/*
 * Convert the given overlink file to stresslink format
 */
int OverlinkToStresslink(const char* olinkfilename,
                         const char* slinkfilename,
                         int nummats, int *ignore_list)
{
   overlink_data   *o[1];
   stresslink_data *s[1];


   int err;

   o[0] = read_overlink(olinkfilename, &err);
   s[0] = MapOverlinkToStresslink( o[0] );

   write_stresslink(s[0], slinkfilename, nummats, ignore_list, &err);

   return OK;
}


/*
 * Convert the given stresslink file to overlink format
 */
int StresslinkToOverlink(const char* slinkfilename,
                         const char* olinkfilename,
                         int nummats, int *ignore_list)
{
   stresslink_data *s[1];
   overlink_data   *o[1];

   int err;

   s[0] = read_stresslink(slinkfilename, &err);
   o[0] = MapStresslinkToOverlink( s[0] );

   write_overlink(o[0], olinkfilename, nummats, ignore_list, &err);


   return OK;
}

/*
 * Stresslink reader/writer loop test - used for debugging
 */
int StresslinkToStresslink(const char* slinkfilename_in,
                           const char* slinkfilename_out,
                           int nummats, int *ignore_list)
{
   stresslink_data *s[1];

   int err;

   s[0] = read_stresslink(slinkfilename_in, &err);

   write_stresslink(s[0], slinkfilename_out, nummats, ignore_list, &err);

   return OK;
}

/*
 * Overlink reader/writer loop test - used for debugging
 */
int OverlinkToOverlink(const char* olinkfilename_in,
                       const char* olinkfilename_out,
                       int nummats, int *ignore_list)
{
   overlink_data *o[1];

   int err;

   o[0] = read_overlink(olinkfilename_in, &err);
   write_overlink(o[0], olinkfilename_out, nummats, ignore_list, &err);

   return OK;
}

/*
 * Stresslink dump function - used for debugging
 */
int DumpStresslink(const char* slinkfilename_in,
                   const char* slinkfilename_out)
{
   stresslink_data *s[1];
   int nummats=0, *ignore_list=NULL;
   int err;

   s[0] = read_stresslink(slinkfilename_in, &err);
   write_stresslink_text(s[0], slinkfilename_out, nummats, ignore_list, &err);

   return OK;
}

/*
 * Overlink dump function - used for debugging
 */
int DumpOverlink(const char* olinkfilename_in,
                 const char* olinkfilename_out)
{
   overlink_data *o[1];
   int nummats=0, *ignore_list=NULL;
   int err;

   o[0] = read_overlink(olinkfilename_in, &err);
   write_overlink_text(o[0], olinkfilename_out, nummats, ignore_list, &err);

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

   overlink_data *ol_out;

   char *tempVarNames[1000];

   double p; /* Pressure */

   /* Label Vars */
   int label_index, *label_map, label_min=0, label_max=0, label_offset=0, node_num=0;

   ol_out = overlink_data_new();

   ol_out->num_node_sets    = sl->num_node_sets;
   num_node_sets            = ol_out->num_node_sets;
   ol_out->node_sets        = (ol_node_set *) malloc(num_node_sets*sizeof(ol_node_set));

   num_element_sets         = sl->num_element_sets;

   ol_out->num_element_sets = 1; /* Only one element set in OL structure */
   ol_out->element_sets     = (ol_element_set *) malloc(1*sizeof(ol_element_set));

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

   ol_out->node_sets[0].numVars    = numNodalVars;
   ol_out->element_sets[0].numVars = numZonalVars;
   ol_out->numVars                 = numVars;

   ol_out->varNames                = (char **) malloc(numVars*2*sizeof(char *));
   for (i=0; i<numVars; i++)
      ol_out->varNames[i] = strdup(tempVarNames[i]);

   ol_out->node_sets[0].varSet    = (ol_var*)malloc(numNodalVars*2*sizeof(ol_var));
   ol_out->element_sets[0].varSet = (ol_var*)malloc(numZonalVars*2*sizeof(ol_var));


   /*************************/
   /* Map the Node Set data */
   /*************************/

   ol_out->node_sets[0].num_nodes = sl->node_sets[0].num_nodes;

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

   ol_out->node_sets[0].labels = (int *) malloc(num_nodes*sizeof(int));
   for ( i=0; i<num_nodes; i++ )
   {
      ol_out->node_sets[0].labels[i] = sl->node_sets[0].labels[i];
   }

   for (i=0; i<3; i++)
   {
      ol_out->node_sets[0].cur_coord[i] =
         (double*)malloc(num_nodes*sizeof(double));
   }

   ol_conn_index = 0;
   for (j=0; j<num_nodes; j++)
   {
      for (i=0; i<3; i++)
      {
         ol_out->node_sets[0].cur_coord[i][j] =
            (double)sl->node_sets[0].cur_coord[ol_conn_index++];
      }
   }

   if (sl->node_sets[0].cur_coord != NULL)
   {
      ol_out->node_sets[0].varSet[idxN].len_val = num_nodes*3;
      ol_out->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol_out->node_sets[0].varSet[idxN].varName         = strdup("cur_coord");
      ol_out->node_sets[0].varSet[idxN].datatype        = DB_FLOAT;
      ol_out->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol_out->node_sets[0].varSet[idxN].mixlen          = 0;
      ol_out->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol_out->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].t_disp[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].t_disp != NULL)
   {
      ol_out->node_sets[0].varSet[idxN].len_val = num_nodes*3;
      ol_out->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol_out->node_sets[0].varSet[idxN].varName         = strdup("t_disp");
      ol_out->node_sets[0].varSet[idxN].datatype        = DB_FLOAT;
      ol_out->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol_out->node_sets[0].varSet[idxN].mixlen          = 0;
      ol_out->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol_out->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].t_disp[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].r_disp != NULL)
   {
      ol_out->node_sets[0].varSet[idxN].len_val = num_nodes*3;
      ol_out->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol_out->node_sets[0].varSet[idxN].varName         = strdup("r_disp");
      ol_out->node_sets[0].varSet[idxN].datatype        = DB_FLOAT;
      ol_out->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol_out->node_sets[0].varSet[idxN].mixlen          = 0;
      ol_out->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol_out->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].r_disp[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].t_vel != NULL)
   {
      ol_out->node_sets[0].varSet[idxN].len_val = num_nodes*3;
      ol_out->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol_out->node_sets[0].varSet[idxN].varName         = strdup("t_vel");
      ol_out->node_sets[0].varSet[idxN].datatype        = DB_FLOAT;
      ol_out->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol_out->node_sets[0].varSet[idxN].mixlen          = 0;
      ol_out->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol_out->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].t_vel[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].r_vel != NULL)
   {
      ol_out->node_sets[0].varSet[idxN].len_val = num_nodes*3;
      ol_out->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol_out->node_sets[0].varSet[idxN].varName         = strdup("r_vel");
      ol_out->node_sets[0].varSet[idxN].datatype        = DB_FLOAT;
      ol_out->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol_out->node_sets[0].varSet[idxN].mixlen          = 0;
      ol_out->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol_out->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].r_vel[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].t_acc != NULL)
   {
      ol_out->node_sets[0].varSet[idxN].len_val = num_nodes*3;
      ol_out->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol_out->node_sets[0].varSet[idxN].varName         = strdup("t_acc");
      ol_out->node_sets[0].varSet[idxN].datatype        = DB_FLOAT;
      ol_out->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol_out->node_sets[0].varSet[idxN].mixlen          = 0;
      ol_out->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol_out->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].t_acc[i];
      }
      idxN++;
   }
   if (sl->node_sets[0].r_acc != NULL)
   {
      ol_out->node_sets[0].varSet[idxN].len_val = num_nodes*3;
      ol_out->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*3*sizeof(double));
      ol_out->node_sets[0].varSet[idxN].varName         = strdup("r_acc");
      ol_out->node_sets[0].varSet[idxN].datatype        = DB_FLOAT;
      ol_out->node_sets[0].varSet[idxN].annotation_flag = FALSE;
      ol_out->node_sets[0].varSet[idxN].mixlen          = 0;
      ol_out->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes*3; i++ )
      {
         ol_out->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].r_acc[i];
      }
      idxN++;
   }

   /* Nodal Annotation Variables */

   if (sl->node_sets[0].labels != NULL)
   {
      ol_out->node_sets[0].varSet[idxN].len_val = num_nodes;
      ol_out->node_sets[0].varSet[idxN].val  =
         (double*)malloc(num_nodes*sizeof(double));
      ol_out->node_sets[0].varSet[idxN].varName         = strdup("node_labels");
      ol_out->node_sets[0].varSet[idxN].datatype        = DB_INT;
      ol_out->node_sets[0].varSet[idxN].annotation_flag = TRUE;
      ol_out->node_sets[0].varSet[idxN].mixlen          = 0;
      ol_out->node_sets[0].varSet[idxN].mixvals         = NULL;
      for ( i=0; i<num_nodes; i++ )
      {
         ol_out->node_sets[0].varSet[idxN].val[i] = (double)sl->node_sets[0].labels[i];
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

   ol_out->element_sets[0].num_elements = total_elements;

   ol_out->element_sets[0].conn =
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
         ol_out->element_sets[0].conn[ol_conn_index] = node_num-1;
         save_conn[ol_conn_index]                    = node_num;
         ol_conn_index++;
      }
   }

   /* Update connectivity with labels */

   /* Translate connectivity to right-hand format */
   for ( i = 0; i < total_elements; i++ )
   {
      for ( j = 0; j<8; j++ )
         temp_conn[j] = ol_out->element_sets[0].conn[node_index+j];

      /* ol_out->element_sets[0].conn[node_index+0] = temp_conn[0];
           ol_out->element_sets[0].conn[node_index+1] = temp_conn[4];
           ol_out->element_sets[0].conn[node_index+2] = temp_conn[5];
           ol_out->element_sets[0].conn[node_index+3] = temp_conn[1];
           ol_out->element_sets[0].conn[node_index+4] = temp_conn[3];
           ol_out->element_sets[0].conn[node_index+5] = temp_conn[7];
           ol_out->element_sets[0].conn[node_index+6] = temp_conn[6];
           ol_out->element_sets[0].conn[node_index+7] = temp_conn[2]; */

      ol_out->element_sets[0].conn[node_index+0] = temp_conn[0];
      ol_out->element_sets[0].conn[node_index+1] = temp_conn[1];
      ol_out->element_sets[0].conn[node_index+2] = temp_conn[2];
      ol_out->element_sets[0].conn[node_index+3] = temp_conn[3];
      ol_out->element_sets[0].conn[node_index+4] = temp_conn[4];
      ol_out->element_sets[0].conn[node_index+5] = temp_conn[5];
      ol_out->element_sets[0].conn[node_index+6] = temp_conn[6];
      ol_out->element_sets[0].conn[node_index+7] = temp_conn[7];
      node_index+=8;
   }

   /* Map the materials (regions) */
   ol_out->element_sets[0].numRegs  = num_element_sets;

   ol_out->element_sets[0].numMixEntries  = 0;
   ol_out->element_sets[0].mixVF          = NULL;
   ol_out->element_sets[0].mixMat         = NULL;
   ol_out->element_sets[0].regionId       = (int*)malloc(num_element_sets*sizeof(int));
   for ( mat_index = 0; mat_index < num_element_sets; mat_index++ )
   {
      ol_out->element_sets[0].regionId[mat_index] = mat_index+1;
   }

   ol_out->element_sets[0].matList = (int*)malloc(total_elements*sizeof(int));

   /* Load  ol_out->element_sets[0].matList with region numbers */
   elem_index = 0;
   for ( mat_index = 0; mat_index < num_element_sets; mat_index++ )
   {
      for ( j = 0; j < sl->element_sets[mat_index].num_elements; j++ )
      {
         ol_out->element_sets[0].matList[elem_index++] = mat_index+1;
      }
   }

   ol_out->element_sets[0].numMixEntries  = 0;
   ol_out->element_sets[0].mixVF          = NULL;
   ol_out->element_sets[0].mixNext        = NULL;
   ol_out->element_sets[0].mixMat         = NULL;
   ol_out->element_sets[0].mixZone        = NULL;

   /* For now the Overlink side only uses one Element set - in the future
    * when we support multiple element types we will need more - IRC
    */
   ol_out->element_sets[0].nnodes    = sl->element_sets[0].nnodes;
   nnodes                            = sl->element_sets[0].nnodes;

   ol_out->element_sets[0].nips      = sl->element_sets[0].nips;
   nnips                             = sl->element_sets[0].nips;

   ol_out->element_sets[0].nhist     = sl->element_sets[0].nhist;
   nhist                             = sl->element_sets[0].nhist;

   ol_out->element_sets[0].type      = sl->element_sets[0].type;

   ol_out->element_sets[0].nhist     = sl->element_sets[0].nhist;
   nhist                             = sl->element_sets[0].nhist;

   /* Map the Zonal Variables */

   idxN = 0;

   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*nnodes*sizeof(double));
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("material");
   ol_out->element_sets[0].varSet[idxN].datatype        = DB_INT;
   ol_out->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   for ( i = 0; i < total_elements; i++ )
   {
      ol_out->element_sets[0].varSet[idxN].val[i] = (double) ol_out->element_sets[0].matList[i];
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements*nnodes;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*nnodes*sizeof(double));
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("init_conn");
   ol_out->element_sets[0].varSet[idxN].datatype        = DB_INT;
   ol_out->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   for ( i = 0; i < total_elements*nnodes; i++ )
   {
      ol_out->element_sets[0].varSet[idxN].val[i] = (double) save_conn[i];
   }
   idxN++;

   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("elem_labels");
   ol_out->element_sets[0].varSet[idxN].datatype        = DB_INT;
   ol_out->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   /* Since labels are for each element set we need to break up into
    * element set arrays.
    */
   elem_index=0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].labels[j];
      }
   }
   idxN++;

   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("vol_ref");
   ol_out->element_sets[0].varSet[idxN].datatype        = DB_FLOAT;
   ol_out->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   /* Since vol_ref are for each element set we need to break up into
    * element set arrays.
    */
   elem_index=0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].vol_ref[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("Pev");
   ol_out->element_sets[0].varSet[idxN].datatype        = DB_FLOAT;
   ol_out->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index=0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      if (sl->element_sets[i].mat11_Pev)
         for ( j=0; j<num_elements; j++ )
         {
            if (validMatModel(sl->element_sets[i].mat_model))
               ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_Pev[j];
         }
   }
   idxN++;

   ol_out->element_sets[0].varSet[idxN].len_val         = 15*num_element_sets;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(15*num_element_sets*sizeof(double));
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("elem_control");
   ol_out->element_sets[0].varSet[idxN].datatype        = DB_FLOAT;
   ol_out->element_sets[0].varSet[idxN].annotation_flag = TRUE; /* Set to TRUE if this  variable is to
								 * loaded into the Annotations section.
								 */
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   /* Since element control is for each element set we need to break up into
    * element set arrays.
    */
   elem_index=0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      for ( j=0; j<13; j++ )
      {
         ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].control[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("sx");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].sxx[j] +
                  (double)sl->element_sets[i].pressure[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i] = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("sy");
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].syy[j] +
                  (double)sl->element_sets[i].pressure[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i] = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("sz");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].szz[j] +
                  (double)sl->element_sets[i].pressure[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("txy");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].sxy[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("tyz");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].syz[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("tzx");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].szx[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("p");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].pressure[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("eps");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      if (sl->element_sets[i].mat11_eps)
         for ( j=0; j<num_elements; j++ )
         {
            if (validMatModel(sl->element_sets[i].mat_model))
               ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_eps[j];
         }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("q");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      if (sl->element_sets[i].mat11_Q)
         for ( j=0; j<num_elements; j++ )
         {
            if (validMatModel(sl->element_sets[i].mat_model))
               ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_Q[j];
         }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("E");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      if (sl->element_sets[i].mat11_E)
         for ( j=0; j<num_elements; j++ )
         {
            if (validMatModel(sl->element_sets[i].mat_model))
               ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_E[j];
         }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("vol");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_vol[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("pev");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_Pev[j];
      }
   }
   idxN++;


   ol_out->element_sets[0].varSet[idxN].len_val         = total_elements;
   ol_out->element_sets[0].varSet[idxN].val             = (double*)malloc(total_elements*sizeof(double));
   for (i=0; i<total_elements; i++)
      ol_out->element_sets[0].varSet[idxN].val[i]     = 0.0;
   ol_out->element_sets[0].varSet[idxN].varName         = strdup("e");
   ol_out->element_sets[0].varSet[idxN].annotation_flag = FALSE;
   ol_out->element_sets[0].varSet[idxN].mixlen          = 0;
   ol_out->element_sets[0].varSet[idxN].mixvals         = NULL;

   elem_index = 0;
   for ( i = 0; i < num_element_sets; i++ )
   {
      num_elements = sl->element_sets[i].num_elements;
      for ( j=0; j<num_elements; j++ )
      {
         if (validMatModel(sl->element_sets[i].mat_model))
            ol_out->element_sets[0].varSet[idxN].val[elem_index++] = (double)sl->element_sets[i].mat11_eos[j];
      }
   }

   /* Update num vars with true count */
   ol_out->element_sets[0].numVars = idxN;

   /* Release temporary storage */
   free(label_map);
   for (i=0; i<numVars; i++)
      free(tempVarNames[i]);

   return (ol_out);
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

   float *temp_control=NULL;
   int   temp_control_index=0;

   double **mix_element_mat_map;
   int    *mix_element_mat_count;
   int    mix_index=0, mix_next_index=0;
   int    *num_elem_region;
   double sum_vf=0.0, max_vf=0.0, p;
   int    max_elem=0;
   int    *elem_region;
   int    sl_conn_index=0, ol_conn_index=0;

   int   numNodalVars, numZonalVars, numVars;

   stresslink_data *sl_out;
   int              sl_hist_index=0;

   int   temp_cnt=0;

   float *x_temp, *y_temp, *z_temp, *rhon_temp;
   int   rhon_found=FALSE;

   char varName[64];

   int **mix_element_vals_map; /* Mixed values mapping (indexes): [numZones:numMats] */

   int *warning_count_per_mat; /* Used to limit number of Warning messages that print */

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

   mix_element_mat_map   = (double **) malloc(num_elements*sizeof(double *));
   mix_element_vals_map  = (int **)    malloc(num_elements*sizeof(int *));
   mix_element_mat_count = (int *)     malloc(num_elements*sizeof(int));
   elem_region           = (int *)     malloc(num_elements*sizeof(int));
   num_elem_region       = (int *)     malloc(num_mats*sizeof(int));

   warning_count_per_mat = (int *)     malloc(num_mats*sizeof(int));

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
      mix_element_vals_map[i] = (int *) malloc(num_mats*sizeof(int *));
   }

   for (i=0; i <num_elements; i++)
   {
      elem_region[i] = -1;
   }

   /* Allocate space for mix element volume-fraction (VF) array and initialize */
   for (i=0; i <num_elements; i++)
   {
      mix_element_mat_map[i]  = (double *) malloc(num_mats*sizeof(double *));
      mix_element_mat_count[i]  = 0;

      for (j=0; j<num_mats; j++)
      {
         mix_element_mat_map[i][j]  = 0.0;
         mix_element_vals_map[i][j] = -1;
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
         mix_element_mat_map[i][mat_index]  = ol->element_sets[0].mixVF[mix_index];
         mix_element_vals_map[i][mat_index] = mix_index;

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
               mix_element_mat_map[i][mat_index] = ol->element_sets[0].mixVF[mix_next_index];
               mix_element_vals_map[i][mat_index] = mix_next_index;
            }
         }
      }
   }

   /* Load  zone numbers for each element */
   for (i=0; i<num_elements; i++)
   {
      if (ol->element_sets[0].matList[i]>=0)
      {
         elem_region[i]           = ol->element_sets[0].matList[i];
         mix_element_mat_count[i] = 1;
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
            sum_vf += mix_element_mat_map[i][mat_index];
            if (mix_element_mat_map[i][mat_index] > max_vf)
            {
               max_vf   = mix_element_mat_map[i][mat_index];
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

   sl_out = stresslink_data_new(ol->num_node_sets, num_sl_mats);

   sl_out->num_element_sets = num_sl_mats;
   num_element_sets         = ol->num_element_sets;

   sl_out->num_node_sets    = ol->num_node_sets;
   num_node_sets            = ol->num_node_sets;

   sl_out->control          = (int *) malloc(20*sizeof(int));
   for (i=0; i<20; i++)
      sl_out->control[i]  = 0;

   sl_out->control[0]       = 1;
   sl_out->control[1]       = num_sl_mats;
   sl_out->control[2]       = 0;
   sl_out->control[3]       = 1; /* Analysis code = Dyna3D/ParaDyn */

   /* Map node set data from Overlink */
   sl_out->node_sets[0].control   = (int *) malloc(20*sizeof(int));
   for (i=0; i<20; i++)
      sl_out->node_sets[0].control[i] = 0;

   sl_out->node_sets[0].num_nodes = ol->node_sets[0].num_nodes;

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

   sl_out->node_sets[0].labels    = (int *) malloc(num_nodes*sizeof(int));
   for ( i=0; i<num_nodes; i++ )
      if (temp_node_labels)
         sl_out->node_sets[0].labels[i] = temp_node_labels[i];
      else
         sl_out->node_sets[0].labels[i] = i+1;

   sl_out->node_sets[0].control[0] = 1; /* Default sub-assembly label number */
   sl_out->node_sets[0].control[1] = 1; /* Default node-set     label number */
   sl_out->node_sets[0].control[2] = num_nodes;

   sl_out->node_sets[0].cur_coord  = (float*)malloc(num_nodes*3*sizeof(float));
   sl_out->node_sets[0].init_coord = (float*)malloc(num_nodes*3*sizeof(float));

   /* Load Current Coords */
   coord_index = 0;
   for (i=0; i<num_nodes; i++)
   {
      sl_out->node_sets[0].cur_coord[coord_index++] = (float)ol->node_sets[0].cur_coord[0][i];
      sl_out->node_sets[0].cur_coord[coord_index++] = (float)ol->node_sets[0].cur_coord[1][i];
      sl_out->node_sets[0].cur_coord[coord_index++] = (float)ol->node_sets[0].cur_coord[2][i];
   }

   /* Load Initial Coords  = cur_coord for now */
   coord_index = 0;
   for (i=0; i<num_nodes; i++)
   {
      sl_out->node_sets[0].init_coord[coord_index++] = (float)ol->node_sets[0].cur_coord[0][i];
      sl_out->node_sets[0].init_coord[coord_index++] = (float)ol->node_sets[0].cur_coord[1][i];
      sl_out->node_sets[0].init_coord[coord_index++] = (float)ol->node_sets[0].cur_coord[2][i];
   }

   sl_out->node_sets[0].t_vel = (float*)malloc(num_nodes*3*sizeof(float));
   x_temp    = (float*)malloc(num_nodes*sizeof(float));
   y_temp    = (float*)malloc(num_nodes*sizeof(float));
   z_temp    = (float*)malloc(num_nodes*sizeof(float));
   rhon_temp = (float*)malloc(num_nodes*sizeof(float));

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
         sl_out->node_sets[0].t_vel[vel_index++] = 0.0;
         sl_out->node_sets[0].t_vel[vel_index++] = 0.0;
         sl_out->node_sets[0].t_vel[vel_index++] = 0.0;
      }
      else
      {
         sl_out->node_sets[0].t_vel[vel_index++] = x_temp[j]/rhon_temp[j];
         sl_out->node_sets[0].t_vel[vel_index++] = y_temp[j]/rhon_temp[j];
         sl_out->node_sets[0].t_vel[vel_index++] = z_temp[j]/rhon_temp[j];
      }
   }

   free(x_temp);
   free(y_temp);
   free(z_temp);
   free(rhon_temp);

   sl_out->element_sets[0].nnodes = ol->element_sets[0].nnodes;

   /* Read element control variables */

   temp_control = (float*) malloc(20*num_sl_mats*sizeof(float));

   for ( i=0; i<ol->element_sets[0].numVars; i++ )
   {

      /* Annotation variable: elem_control */
      if ( strcmp(ol->element_sets[0].varSet[i].varName, "elem_control" ) == 0)
      {
         for ( j=0; j<13*num_sl_mats; j++ )
         {
            temp_control[j] = (float) ol->element_sets[0].varSet[i].val[j];
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

      sl_out->element_sets[mat_index].control = (int *) malloc(20*sizeof(int));

      for (i=0; i<20; i++)
         sl_out->element_sets[mat_index].control[i] = 0;

      sl_out->element_sets[mat_index].control[0]   = sub_assembly; /* Sub-assembly number */
      sl_out->element_sets[mat_index].control[1]   = mat_index+1;  /* Element set label number */
      sl_out->element_sets[mat_index].control[2]   = SOLID;        /* Solid Element */
      sl_out->element_sets[mat_index].control[4]   = nnodes;
      sl_out->element_sets[mat_index].control[5]   = num_elem_region[mat_index];
      sl_out->element_sets[mat_index].control[6]   = nips;        /* num integration points */
      sl_out->element_sets[mat_index].control[7]   = mat_model;   /* Mat Model number */
      sl_out->element_sets[mat_index].control[8]   = nhist;       /* Num Hist */

      sl_out->element_sets[mat_index].num_elements = num_elem_region[mat_index];
      sl_out->element_sets[mat_index].nnodes       = nnodes;
      sl_out->element_sets[mat_index].nhist        = nhist;
   }


   for (mat_index=0; mat_index<num_sl_mats; mat_index++)
   {
      num_elements_per_mat = num_elem_region[mat_index];
      mat_model            = sl_out->element_sets[mat_index].control[7];
      nhist                = sl_out->element_sets[mat_index].nhist;

      /* Initialize all element set fields */
      sl_out->element_sets[mat_index].type         = SOLID;
      sl_out->element_sets[mat_index].num_elements = num_elements_per_mat;
      sl_out->element_sets[mat_index].len_control  = 20;
      sl_out->element_sets[mat_index].nips         = 1;
      sl_out->element_sets[mat_index].mat_model    = mat_model;

      sl_out->element_sets[mat_index].labels       = (int *) malloc(num_elements_per_mat*sizeof(int));
      for (j=0; j<num_elements_per_mat; j++)
         sl_out->element_sets[mat_index].labels[j] = j+1; /* Initialize labels with 1:n */

      sl_out->element_sets[mat_index].vol_ref      = (float *) malloc(num_elements_per_mat*sizeof(float));
      for (j=0; j<num_elements_per_mat; j++)
      {
         sl_out->element_sets[mat_index].vol_ref[j] = 0; /* Initialize reference volume to 0. */
      }
      sl_out->element_sets[mat_index].pressure     = NULL;
      sl_out->element_sets[mat_index].sxx          = NULL;
      sl_out->element_sets[mat_index].syy          = NULL;
      sl_out->element_sets[mat_index].szz          = NULL;
      sl_out->element_sets[mat_index].sxy          = NULL;
      sl_out->element_sets[mat_index].syz          = NULL;
      sl_out->element_sets[mat_index].szx          = NULL;

      sl_out->element_sets[mat_index].conn =
         (int*)malloc(num_elements_per_mat*nnodes*sizeof(int));

      /* Load the connectivity */
      for (j=0; j<num_elements_per_mat*nnodes; j++)
      {
         sl_out->element_sets[mat_index].conn[j] = (int) ol->element_sets[0].conn[(total_elements*nnodes)+j];
      }

      for ( i=0; i<ol->element_sets[0].numVars; i++ )
      {

         /* Annotation variable: labels */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "elem_labels" ) == 0)
         {
            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl_out->element_sets[mat_index].labels[j] = (int) ol->element_sets[0].varSet[i].val[total_elements+j];
            }
         }

         /* Annotation variable: reference volume */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "vol_ref" ) == 0)
         {
            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl_out->element_sets[mat_index].vol_ref[j] = ol->element_sets[0].varSet[i].val[total_elements+j];
            }
         }

         /* Annotation variable: Pev */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "Pev" ) == 0)
         {
            sl_out->element_sets[mat_index].mat11_Pev = (float *) malloc(num_elements_per_mat*sizeof(float));

            for ( j=0; j<num_elements_per_mat; j++ )
            {
               sl_out->element_sets[mat_index].mat11_Pev[j] = ol->element_sets[0].varSet[i].val[total_elements+j];
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
            sl_out->element_sets[mat_index].conn[i] = temp_conn[i];
         }
      }
      else
         for ( i=0; i<num_elements_per_mat; i++ )
         {
            for ( j = 0; j<8; j++ )
            {
               temp_conn[j] = sl_out->element_sets[mat_index].conn[node_index+j]+1;

               /* Apply label mapping if labels are present */
               if (temp_node_labels)
               {
                  temp_conn[j] = temp_node_labels[temp_conn[j]-1];
               }
            }

            /*sl_out->element_sets[mat_index].conn[node_index+0] = temp_conn[0];
            sl_out->element_sets[mat_index].conn[node_index+1] = temp_conn[3];
            sl_out->element_sets[mat_index].conn[node_index+2] = temp_conn[7];
            sl_out->element_sets[mat_index].conn[node_index+3] = temp_conn[4];
            sl_out->element_sets[mat_index].conn[node_index+4] = temp_conn[1];
            sl_out->element_sets[mat_index].conn[node_index+5] = temp_conn[2];
            sl_out->element_sets[mat_index].conn[node_index+6] = temp_conn[6];
            sl_out->element_sets[mat_index].conn[node_index+7] = temp_conn[5];*/

            sl_out->element_sets[mat_index].conn[node_index+0] = temp_conn[0];
            sl_out->element_sets[mat_index].conn[node_index+1] = temp_conn[1];
            sl_out->element_sets[mat_index].conn[node_index+2] = temp_conn[2];
            sl_out->element_sets[mat_index].conn[node_index+3] = temp_conn[3];
            sl_out->element_sets[mat_index].conn[node_index+4] = temp_conn[4];
            sl_out->element_sets[mat_index].conn[node_index+5] = temp_conn[5];
            sl_out->element_sets[mat_index].conn[node_index+6] = temp_conn[6];
            sl_out->element_sets[mat_index].conn[node_index+7] = temp_conn[7];
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

   for (mat_index=0; mat_index<num_sl_mats; mat_index++)
   {
      num_elements_per_mat = num_elem_region[mat_index];

      sl_out->element_sets[mat_index].pressure = NULL;

      /* We need to get pressure field first since stress' are calculated from pressure */
      for ( i=0; i<ol->element_sets[0].numVars; i++ )
      {
         /* Pressure - p */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "p" ) == 0)
         {
            sl_out->element_sets[mat_index].pressure = (float *) malloc(num_elements*sizeof(float));
            elem_index = 0;

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].pressure[j] = ol->element_sets[0].varSet[i].val[j];
            }
         }
      }

      /* We first need to get sx & sy since they are used later to calculate szz */
      sl_out->element_sets[mat_index].sxx = NULL;
      sl_out->element_sets[mat_index].syy = NULL;

      for ( i=0; i<ol->element_sets[0].numVars; i++ )
      {

         /* Sxx */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "sx" ) == 0 &&
               sl_out->element_sets[mat_index].pressure )
         {
            sl_out->element_sets[mat_index].sxx = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].sxx[j] = ol->element_sets[0].varSet[i].val[j] -
                     sl_out->element_sets[mat_index].pressure[j];
            }
         }

         /* Syy */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "sy" ) == 0 &&
               sl_out->element_sets[mat_index].pressure )
         {
            sl_out->element_sets[mat_index].syy = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].syy[j] = ol->element_sets[0].varSet[i].val[j] -
                     sl_out->element_sets[mat_index].pressure[j];
            }
         }
      }

      /* Szz */
      sl_out->element_sets[mat_index].szz = NULL;
      if ( sl_out->element_sets[mat_index].pressure && sl_out->element_sets[mat_index].sxx && sl_out->element_sets[mat_index].syy )
      {
         sl_out->element_sets[mat_index].szz = (float *) malloc(num_elements*sizeof(float));

         for ( j=0; j<num_elements; j++ )
         {
            p = sl_out->element_sets[mat_index].pressure[j];
            sl_out->element_sets[mat_index].szz[j] =
               -(sl_out->element_sets[mat_index].sxx[j]) - (sl_out->element_sets[mat_index].syy[j]) - p;
         }
      }

      /* Now get the rest of the variables */
      sl_out->element_sets[mat_index].rhon      = NULL;
      sl_out->element_sets[mat_index].sxy       = NULL;
      sl_out->element_sets[mat_index].syz       = NULL;
      sl_out->element_sets[mat_index].szx       = NULL;
      sl_out->element_sets[mat_index].v         = NULL;
      sl_out->element_sets[mat_index].mat11_eps = NULL;
      sl_out->element_sets[mat_index].mat11_E   = NULL;
      sl_out->element_sets[mat_index].mat11_eos = NULL;
      sl_out->element_sets[mat_index].mat11_Q   = NULL;
      sl_out->element_sets[mat_index].mat11_vol = NULL;
      sl_out->element_sets[mat_index].mat11_Pev = NULL;

      for ( i=0; i<ol->element_sets[0].numVars; i++ )
      {

         strcpy(varName, ol->element_sets[0].varSet[i].varName);

         /* rhon */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "rhon" ) == 0)
         {
            sl_out->element_sets[mat_index].rhon = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].rhon[j] = ol->element_sets[0].varSet[i].val[j];
            }
         }

         /* Sxy */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "txy" ) == 0 &&
               sl_out->element_sets[mat_index].pressure )
         {
            sl_out->element_sets[mat_index].sxy = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].sxy[j] = ol->element_sets[0].varSet[i].val[j];
            }
         }

         /* Syz */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "tyz" ) == 0 &&
               sl_out->element_sets[mat_index].pressure )
         {
            sl_out->element_sets[mat_index].syz = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].syz[j] = ol->element_sets[0].varSet[i].val[j];
            }
         }

         /* Szx */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "txz" ) == 0 &&
               sl_out->element_sets[mat_index].pressure )
         {
            sl_out->element_sets[mat_index].szx = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].szx[j] = ol->element_sets[0].varSet[i].val[j];
            }
         }


         /**************************/
         /* Mat Model-11 Variables */
         /**************************/

         /* eps - Requires Reading Mixed component!! */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "eps" ) == 0 )
         {
            sl_out->element_sets[mat_index].mat11_eps = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
               sl_out->element_sets[mat_index].mat11_eps[j] = ol->element_sets[0].varSet[i].val[j];

            if (ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements; j++ )
               {
                  mix_index = mix_element_vals_map[j][0];
                  if (mix_index>=0)
                     sl_out->element_sets[mat_index].mat11_eps[j] = ol->element_sets[0].varSet[i].mixvals[mix_index];
               }
         }

         /* E - Requires Reading Mixed component!! */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "E" ) == 0 )
         {
            sl_out->element_sets[mat_index].mat11_E = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
               sl_out->element_sets[mat_index].mat11_E[j] = ol->element_sets[0].varSet[i].val[j];

            if (ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements; j++ )
               {
                  mix_index = mix_element_vals_map[j][0];
                  if (mix_index>=0)
                     sl_out->element_sets[mat_index].mat11_E[j] = ol->element_sets[0].varSet[i].mixvals[mix_index];
               }
         }

         /* eos - Requires Reading Mixed component!! */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "e" ) == 0 )
         {
            sl_out->element_sets[mat_index].mat11_eos = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
               sl_out->element_sets[mat_index].mat11_eos[j] = ol->element_sets[0].varSet[i].val[j];

            if (ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements; j++ )
               {
                  mix_index = mix_element_vals_map[j][0];
                  if (mix_index>=0)
                     sl_out->element_sets[mat_index].mat11_eos[j] = ol->element_sets[0].varSet[i].mixvals[mix_index];
               }
         }

         /* Q */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "q" ) == 0 )
         {
            sl_out->element_sets[mat_index].mat11_Q = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].mat11_Q[j] = ol->element_sets[0].varSet[i].val[j];
            }
         }

         /* v - relative vol  Requires Reading Mixed component!! */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "v" ) == 0 )
         {
            sl_out->element_sets[mat_index].v = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
               sl_out->element_sets[mat_index].v[j] = ol->element_sets[0].varSet[i].val[j];

            if (ol->element_sets[0].varSet[i].mixlen>0)
               for ( j=0; j<num_elements; j++ )
               {
                  mix_index = mix_element_vals_map[j][0];
                  if (mix_index>=0)
                     sl_out->element_sets[mat_index].v[j] = ol->element_sets[0].varSet[i].mixvals[mix_index];
               }
         }

         /* Pev */
         if ( strcmp(ol->element_sets[0].varSet[i].varName, "Pev" ) == 0 )
         {
            sl_out->element_sets[mat_index].mat11_Pev = (float *) malloc(num_elements*sizeof(float));

            for ( j=0; j<num_elements; j++ )
            {
               sl_out->element_sets[mat_index].mat11_Pev[j] = ol->element_sets[0].varSet[i].val[j];
            }
         }
      }

      /* Compute vol (copy from vol_ref) */

      sl_out->element_sets[mat_index].mat11_vol = (float *) malloc(num_elements*sizeof(float));
      for ( j=0; j<num_elements; j++ )
      {
         sl_out->element_sets[mat_index].mat11_vol[j] = sl_out->element_sets[mat_index].vol_ref[j];
      }

      /* Compute E */

      sl_out->element_sets[mat_index].mat11_E = (float *) malloc(num_elements*sizeof(float));
      if (sl_out->element_sets[mat_index].mat11_eos && sl_out->element_sets[mat_index].vol_ref &&
            sl_out->element_sets[mat_index].v )
      {
         for ( j=0; j<num_elements; j++ )
         {
            if (sl_out->element_sets[mat_index].v[j]>0.0)
               sl_out->element_sets[mat_index].mat11_E[j] = (sl_out->element_sets[mat_index].mat11_eos[j]/
                     sl_out->element_sets[mat_index].v[j]) *
                     sl_out->element_sets[mat_index].vol_ref[j];
            else
               sl_out->element_sets[mat_index].mat11_E[j] = 0.0;
         }
      }
      else
      {
         for ( j=0; j<num_elements; j++ )
            sl_out->element_sets[mat_index].mat11_E[j] = 0.0;
      }
   } /* mat loop */



   /*****************************************************/
   /* Copy result variables into history variable array */
   /*****************************************************/
   for (mat_index=0; mat_index<num_sl_mats; mat_index++)
   {
      num_elements_per_mat = num_elem_region[mat_index];
      nhist                = sl_out->element_sets[mat_index].nhist;

      sl_out->element_sets[mat_index].hist_var = (float *) malloc(num_elements_per_mat*nhist*sizeof(float));
      for ( j=0; j<num_elements_per_mat*nhist; j++ )
      {
         sl_out->element_sets[mat_index].hist_var[i] = 0.0;
      }

      sl_hist_index = 0;

      mat_model = sl_out->element_sets[mat_index].mat_model;

      for ( j=0; j<num_elements_per_mat; j++ )
      {
         if (sl_out->element_sets[mat_index].sxx)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+0] = sl_out->element_sets[mat_index].sxx[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+0] = 0.0;

         if (sl_out->element_sets[mat_index].syy)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+1] = sl_out->element_sets[mat_index].syy[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+1] = 0.0;

         if (sl_out->element_sets[mat_index].szz)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+2] = sl_out->element_sets[mat_index].szz[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+2] = 0.0;

         if (sl_out->element_sets[mat_index].sxy)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+3] = sl_out->element_sets[mat_index].sxy[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+3] = 0.0;

         if (sl_out->element_sets[mat_index].syz)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+4] = sl_out->element_sets[mat_index].syz[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+4] = 0.0;

         if (sl_out->element_sets[mat_index].szx)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+5] = sl_out->element_sets[mat_index].szx[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+5] = 0.0;

         /* Mat Model 11 History Variables */
         if (sl_out->element_sets[mat_index].mat11_eps && mat_model==11)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+6] = sl_out->element_sets[mat_index].mat11_eps[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+6] = 0.0;

         if (sl_out->element_sets[mat_index].mat11_E && mat_model==11)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+7] = sl_out->element_sets[mat_index].mat11_E[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+7] = 0.0;

         if (sl_out->element_sets[mat_index].mat11_Q && mat_model==11)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+8] = sl_out->element_sets[mat_index].mat11_Q[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+8] = 0.0;

         if (sl_out->element_sets[mat_index].vol_ref && mat_model==11)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+9] = sl_out->element_sets[mat_index].vol_ref[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+9] = 0.0;

         if (sl_out->element_sets[mat_index].mat11_Pev && mat_model==11)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+10] = sl_out->element_sets[mat_index].mat11_Pev[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+10] = 0.0;

         if (sl_out->element_sets[mat_index].mat11_eos && mat_model==11)
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+11] = sl_out->element_sets[mat_index].mat11_eos[j];
         else
            sl_out->element_sets[mat_index].hist_var[sl_hist_index+11] = 0.0;

         sl_hist_index+=nhist;
      }
   }


   /* Release temporary storage */
   for (i=0; i <num_elements; i++)
   {
      free(mix_element_mat_map[i]);
      free(mix_element_vals_map[i]);
   }
   free(mix_element_mat_map);
   free(mix_element_vals_map);

   free(elem_region);
   free(num_elem_region);

   return (sl_out);
}


int
validMatModel(int mat_model)
{
   if ( mat_model==11 )
      return (TRUE);
   else
      return (FALSE);
}

/* End of socitume.c */
