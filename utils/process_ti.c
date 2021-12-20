/*
 * process_ti.c
 *
 *      Lawrence Livermore National Laboratory
 *
 * Copyright (c) 1991 Regents of the University of California
 */

/*
 *                        L E G A L   N O T I C E
 *
 *                          (C) Copyright 1991
 */

/*
************************************************************************
* Modifications:
*
*  I. R. Corey - October 2, 2006: Added functions for merging TI files.
*  E. M. Pierce - August 24, 2007:  Moved ti processing from main driver
*  E. M. Pierce - January 10, 2008:  Added combining and writing labels.
*  I. R. Corey - November 16, 2009:  Added combining and writing labels
*     for meshless elements (ML).
*
************************************************************************
*/

/* #define DEBUG 1 */

#include "driver.h"


/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;

Label* create_label(char* shortname, int nproc,int sclass, int mesh_id,
                     Bool_type isMeshVar, Bool_type isNodal)
{
   int proc;
   Label *label = NEW(Label,"Creating Label");
   label->next = NULL;
   label->sname[0]= '\0';
   label->sclass = sclass;
   label->mesh_id = mesh_id;
   label->isMeshVar = isMeshVar;
   label->isNodal = isNodal;
   label->size = 0;
   label->combiner_created = 0;
   strcat(label->sname,shortname);
   label->num_per_processors = 
                  (long *) calloc( nproc,sizeof(long) );
   label->offset_per_processor = (LONGLONG*) calloc(nproc,sizeof(LONGLONG) );

   

   return label;
}
/************************************************************
 * TAG( load_ti_labels )
 *
 * Read in nodal and element labels for all processors from
 * the Mili TI files.
 */

int
load_ti_labels( Mili_analysis **in_db, int nproc, TILabels *labels )
{
   Mili_family *fam;
   Hash_table *table;
   int proc;
   int i, j,k;
   int    fam_id;
   int mesh_id;
   char short_name[M_MAX_NAME_LEN], long_name[M_MAX_NAME_LEN];
   Bool_type status;
   Label * working_label, *last_label;
   /* TI variables */
   char **wildcard_list;
   int   wildcard_count = 0;
   int   datatype,datalength;
   Bool_type labels_found=FALSE;

   int  *temp_int=NULL,
	     *label_temp_int = NULL;
   long *temp_long=NULL,
	     *label_temp_long= NULL;

   int num_items_read;

   char labels_class[128];
   int labels_meshid, 
       labels_state, 
       labels_matid, 
       labels_superclass;
   
   int    *data_int,    data_int_scalar;
   long   *data_long,   data_long_scalar;
   double *data_double, data_double_scalar;
   float  *data_float,  data_float_scalar;
   char   *data_char,*out_data_char;
   
      
   Bool_type labels_isMeshvar, 
             labels_isNodal;

   /*
    * Assume all processors have the same number of materials?
    * NO
    */
   fam_id     = in_db[env.start_proc]->db_ident;
   fam        = fam_list[fam_id];
   labels->dimensions = fam->dimensions;
   mesh_id = fam->qty_meshes - 1;
   /*initialize the new label struct to NULL*/
   labels->labels == NULL;
   
   
   for( proc = env.start_proc; proc < env.stop_proc; proc++ ) {
   
      if ( !in_db[proc] ) {
         continue;
      }
      fam_id     = in_db[proc]->db_ident;
      fam        = fam_list[fam_id];

      /* Collect all the TI variables on this processor */
      mesh_id = fam->qty_meshes - 1;
      if(fam->char_header[DIR_VERSION_IDX] >1){
         table = fam->param_table;
      }else{
         table = fam->ti_param_table;
      }
      wildcard_count = htable_search_wildcard(table,0, FALSE,"*", 
                                              "NULL", "NULL",0);
                                              
      wildcard_list=(char**) malloc( wildcard_count*sizeof(char *));
      
      wildcard_count = htable_search_wildcard(table,0, FALSE,"*", 
                                              "NULL", "NULL",wildcard_list );


      for (i=0; i<wildcard_count; i++) {
         if ( strncmp( "Element Labels-ElemIds", wildcard_list[i], strlen("Element Labels-ElemIds"))==0 ||
               strncmp( "Node Labels",    wildcard_list[i], strlen("Node Labels"))==0 ) {

            labels_found=TRUE;
            datalength = 0;

            status = mc_ti_get_metadata_from_name( wildcard_list[i], labels_class,
                                                   &labels_meshid, &labels_state, &labels_matid,
                                                   &labels_superclass,
                                                   &labels_isMeshvar, &labels_isNodal );

            if(fam->char_header[DIR_VERSION_IDX] >1){
               status = mc_read_param_array_len( fam_id, wildcard_list[i],
                                                 &datatype, &datalength );

            }else{
               status = mc_ti_get_data_len( fam_id, wildcard_list[i],
                                            &datatype,  &datalength );
            }
            if( status != OK ) {
               continue;
            }
            
            if (labels->labels == NULL)
            {
               labels->labels = create_label(labels_class,nproc,
                                             labels_superclass,
                                             labels_meshid, 
                                             labels_isMeshvar, 
                                             labels_isNodal);
               working_label = labels->labels;
            }else
            {
               working_label = labels->labels;
               while(working_label != NULL) {
                  if(strcmp(labels_class,working_label->sname) == 0) {
                     break;
                  }
                  last_label = working_label;
                  working_label = working_label->next;
               }
            }
            if(working_label== NULL) {
               last_label->next = create_label(labels_class,nproc,
                                                labels_superclass,
                                                labels_meshid, 
                                                labels_isMeshvar, 
                                                labels_isNodal);
               working_label = last_label->next;
            }
            working_label->size += datalength;
            working_label->num_per_processors[proc] += datalength;           
         }
      }
      for(i = 0 ; i< wildcard_count; i++) {
         free(wildcard_list[i]);
      } 
      free(wildcard_list);
      
      for(working_label = labels->labels; working_label != NULL; working_label = working_label->next) {
         if(proc ==0) {
            working_label->offset_per_processor[0]=0;
         }else {
            for(k=0;k<proc;k++) {
               working_label->offset_per_processor[proc] += working_label->num_per_processors[k];
            }
         }
      }
   }
   
   if(labels->labels == NULL) {
      return(NO_LABELS);
   }
   /* Exit now if no labels present */
   if ( !labels_found ) {
      return(NO_LABELS);
   }

   /* Allocate space in the Labels structure for the
    * different classes of labels.
    */
   for(working_label = labels->labels; working_label != NULL; working_label = working_label->next) {
      working_label->labels = (int*) malloc(sizeof(int)*working_label->size);
   }
   

   /* Process the labels for each processor */
   for( proc = env.start_proc; proc < env.stop_proc; proc++ ) {
      
      if ( !in_db[proc] ) {
         continue;
      }
      
      fam_id     = in_db[proc]->db_ident;
      fam        = fam_list[fam_id];
      
      if(fam->char_header[DIR_VERSION_IDX] >1){
         table = fam->param_table;
      }else{
         table = fam->ti_param_table;
      }
      
      /* Collect the remainder of all TI variables on this processor */
      wildcard_count = htable_search_wildcard(table, 0, FALSE,"*", 
                                              "NULL", "NULL",0);
      
      wildcard_list=(char**) malloc( wildcard_count*sizeof(char *));
      
      wildcard_count = 
         htable_search_wildcard(table,0, FALSE,"*", "NULL",
                                "NULL",wildcard_list );

      /* Check to see if we have any labels - if not then exit */
      for (i=0; i<wildcard_count; i++) {
      
         if ( strncmp( "Element Labels-ElemIds", wildcard_list[i], strlen("Element Labels-ElemIds"))==0 ||
              strncmp( "Node Labels",    wildcard_list[i], strlen("Node Labels"))==0 ) {
            
            labels_found=TRUE;
            datalength = 0;
            
            if(fam->char_header[DIR_VERSION_IDX] >1){
               status = mc_read_param_array_len( fam_id, wildcard_list[i],
                                                 &datatype,&datalength );
                                              
            }else{
               status = mc_ti_get_data_len( fam_id, wildcard_list[i],
                                            &datatype,  &datalength );
            }
            temp_int = NULL;
            temp_long= NULL;
            if( status != OK ) {
               continue;
            }
            
            status = mc_ti_get_metadata_from_name( wildcard_list[i], labels_class,
                                                   &labels_meshid, &labels_state,
                                                   &labels_matid,&labels_superclass,
                                                   &labels_isMeshvar, &labels_isNodal );
            if( status != OK ) {
               continue;
            }
            
            for(working_label=labels->labels; working_label != NULL ; working_label = working_label->next) {
               if(strcmp(working_label->sname,labels_class)==0) {
                  break;
               }
            }
            if(working_label==NULL){
               return(NO_LABELS);
            }
            
            int start_offset = working_label->offset_per_processor[proc];
            switch (datatype) {
               case (M_INT):
                  /* Allocate temporary memory for this processor's TI variable */
                  /*temp_int = (int *) malloc( datalength*sizeof(int) ) ;*/

                  status = mc_ti_read_array( fam_id, wildcard_list[i], (void **) &temp_int, &num_items_read ) ;
                  break;

               case (M_INT8):
                  /* Allocate temporary memory for this processor's TI variable */
                  /*temp_long = (long *) malloc( datalength*sizeof(long) ) ;*/

                  status = mc_ti_read_array( fam_id, wildcard_list[i], (void **) &temp_long, &num_items_read ) ;
                  break;
            } /* end switch */
				
				if ( strncmp( "Node Labels", wildcard_list[i], strlen("Node Labels"))!=0) {
					
					char * label_info = strchr(wildcard_list[i], '[');
					for(j=0; j<wildcard_count;j++) {
						if(strstr(wildcard_list[j],label_info)) {
                     if(strncmp("Element Labels[",
                        wildcard_list[j],strlen("Element Labels[")) == 0) {
                        break;
                     }
                  }
					}
					
					if (j >= wildcard_count) {
                  fprintf(stderr, "No data for label id mismatch for %s\n", wildcard_list[i]);
                  return !OK;
               }
					
               switch (datatype) {
                  case (M_INT):
							if(label_temp_int != NULL){
                     	free(label_temp_int);
								label_temp_int = NULL;
							}
                     status = mc_ti_read_array( fam_id, wildcard_list[j], (void **) &label_temp_int, &num_items_read ) ;
                     break;

                  case (M_INT8):
                     if(label_temp_long != NULL) {
								free(label_temp_long);
								label_temp_long=NULL;
							}
                     status = mc_ti_read_array( fam_id, wildcard_list[j], (void **) &label_temp_long, &num_items_read ) ;
                     break;
               } /* end switch */
               for(j=0; j<datalength; j++) {
                  switch (datatype) {
                     case (M_INT):
                        if(working_label->size <= start_offset+temp_int[j]-1)
                        {
                           fprintf(stderr, "Trying to write to a out of bounds memory address.\n");
                           htable_delete_wildcard_list( wildcard_count, wildcard_list ) ;
                           return 0;
                        }
                        working_label->labels[start_offset+temp_int[j]-1] = label_temp_int[j];
                        break;
                     case (M_INT8):
                        if(working_label->size <= start_offset+temp_int[j]-1)
                        {
                           fprintf(stderr, "Trying to write to a out of bounds memory address.\n");
                           htable_delete_wildcard_list( wildcard_count, wildcard_list ) ;
                           return 0;
                        }
                        working_label->labels[start_offset+temp_long[j]-1] = label_temp_long[j];
                        break;
                  } 
               }
					
				}else{
					for(j=0 ; j< datalength; j++) {
               	switch (datatype) {
                  	case (M_INT):
                     	working_label->labels[start_offset+j] = temp_int[j];
                     	break;
                  	case (M_INT8):
                     	working_label->labels[start_offset+j] = temp_long[j];
                     	break;
               	} 
            	}
				}
				
				
				
            if(temp_int != NULL) {
               free(temp_int);
               temp_int = NULL;
            }
            if( temp_long != NULL) {
               free(temp_long);
               temp_long=NULL;
            } 
				if(label_temp_int != NULL) {
               free(temp_int);
               temp_int = NULL;
            }
            if( label_temp_long != NULL) {
               free(temp_long);
               temp_long=NULL;
            }                       
         } /* if Elem Labels */
      } /* for wildcard_count */

      /* Release temporary allocated memory */
      htable_delete_wildcard_list( wildcard_count, wildcard_list ) ;
      wildcard_count = 0;
      int count = 0;
      int out_count = 0;
      char** out_names;
      for(mesh_id= 0 ; mesh_id < fam->qty_meshes; mesh_id++){
         get_elem_conn_classes_names( fam, mesh_id ,&count,&out_names);
      }
      int con_count;
      int found,args =0;
      char temp[128][1];
      for (con_count=0 ; con_count<count; con_count++) {
         found = 0;
         for(working_label=labels->labels; working_label != NULL ; working_label = working_label->next) {
            if(strcmp(working_label->sname,out_names[con_count])==0) {
               found=1;
              break;
            }
            last_label = working_label;
            /* We found an issue */
         }
         if(found && !working_label->combiner_created){
            continue;
         }
         out_count = count_elem_conn_defs(fam,0,out_names[con_count]);
         if(!found ) {
             
             strcpy(temp[0],out_names[con_count]);
             mc_query_family(fam_id,CLASS_SUPERCLASS,(void*)&args,out_names[con_count] ,(void*)&labels_superclass);
             last_label->next = create_label(out_names[con_count],nproc,
                                             labels_superclass,
                                             0, 
                                             1, 
                                             0);
            working_label = last_label->next;
            working_label->combiner_created = 1;
            working_label->size = out_count;
            working_label->labels = (int*) malloc(sizeof(int)*working_label->size); 
                                            
         }else {
            working_label->labels=RENEW_N( int, working_label->labels,working_label->size,out_count,"labels");
            working_label->size += out_count;
         }
         if(proc ==0) {
            working_label->offset_per_processor[0]=0;
            working_label->num_per_processors[0]=out_count;
         }else {
            working_label->num_per_processors[proc]+=out_count;
            working_label->offset_per_processor[proc] = working_label->size-out_count;
            
         }
         
         for(j=0;j<out_count;j++) {
            working_label->labels[working_label->offset_per_processor[proc]+j]=working_label->offset_per_processor[proc]+j+1;
         }
         
      }
      for(i = 0 ; i <count; i++) {
         free(out_names[i]);
      }
      free(out_names);
      for(i = 0 ; i< wildcard_count; i++) {
         free(wildcard_list[i]);
      } 
      free(wildcard_list);
   } /* for nproc */
   
   
   return(OK);
}


/************************************************************
 * TAG( shift_array )
 * This is used to shift the array down during insertion sorting
 ************************************************************/
void shift_array(int start,int stop,int* labels) {
   int i;

   for(i=stop-1; i>start; i--) {
      labels[i] = labels[i-1];
   }

}
/*
sorter* 
create_sorter(int proc, int position, int label) {
   sorter *rsort = (sorter*) malloc(sizeof(sorter));
   rsort->parent = NULL;
   rsort->left = NULL;
   rsort->right = NULL;
   rsort->same = NULL;
   rsort->proc = proc;
   rsort->proc_position = position;
   rsort->label_id = label;
   rsort->child_count=0;
   return rsort;
}
sorter *
bsort_insert(sorter *head, sorter *insert) {

   sorter *comparator = head;
   sorter *parent;
   short finished = 0; 
   short duplicate =0;       
   while(!finished){
      if(insert->label_id == comparator->label_id) {
         duplicate = 1;
         if(comparator->same != NULL) {
            insert->same = comparator->same;            
         }
         comparator->same = insert;
         finished = 1;
      }else if(insert->label_id > comparator->label_id) {
         comparator->child_count=comparator->child_count+insert->child_count +1;
         if(comparator->right == NULL) {
            comparator->right = insert;
            insert->parent = comparator;
            finished = 1 ;                    
         }else{
            
            comparator = comparator->right;
         }
      }else {
         comparator->child_count=comparator->child_count+insert->child_count +1;
         if(comparator->left == NULL) {
            comparator->left = insert;
            insert->parent = comparator;
            finished = 1 ;                    
         }else{
            comparator = comparator->left;
         }
      }
   }
   if(!duplicate){
      if((insert->parent->left == NULL || insert->parent->right == NULL) 
         && insert->parent->parent != NULL) {
         parent = insert->parent;
         if(insert->parent->left == NULL) {
            parent->left = parent->parent;
            parent->left->child_count = 0;
            parent->child_count = insert->child_count +2;
            parent->parent = parent->left->parent;
            parent->left->parent = parent;
            parent->left->right = NULL;
         }else {
            parent->right = parent->parent;
            parent->right->child_count = 0;
            parent->child_count = insert->child_count +2;
            parent->parent = parent->right->parent;
            parent->right->parent = parent;
            parent->right->left = NULL;
         }
         if(parent->parent == NULL) {
            head = parent;
         }
      } 
   }
   return head;
}
*/
void
create_new_sorter(int proc, int position, int label,struct sorter *target,int index) {
   
   target[index].label_id = label;
   target[index].proc= proc;
   target[index].proc_position= position;
}
/************************************************************
 * TAG( filter_count )
 * This is used to filter out duplicate counts on nodes
 ************************************************************/
void
filter_count(Label *label,int proc_count) {
   int limit =2;
   int proc = 0,
       num_items,
       status,
       index=0,
       size=0;
   int i,j,k;
   short finished = 0;
   int child; 
   int last_element_index= 1;
   int last_label;
   int unit_size = sizeof(int);
   int label_index = 0;
   
   size=0;
   int heap_size=0,now;
   
   struct sorter *arr = NULL;
   arr =  (struct sorter*)calloc(label->size+2,sizeof(struct sorter));
   struct sorter temp_sorter;
   size = label->size;
   for(proc=0; proc<proc_count; proc++) {
      for(i=0;i<label->num_per_processors[proc];i++){
         heap_size++;
         if(heap_size > label->size || heap_size <1) {
            fprintf(stderr, "Too large proc: %d, offset: %d, heap size: %d, size: %d",
                    proc,(int)label->offset_per_processor[proc]+i,heap_size,label->size);
         }
         create_new_sorter(proc,i,
                           label->labels[label->offset_per_processor[proc]+i],
                           arr,heap_size);
         if(heap_size>1) {
            now= heap_size;
            while(arr[now/2].label_id > arr[now].label_id){
               temp_sorter.label_id= arr[now/2].label_id;
               temp_sorter.proc= arr[now/2].proc;
               temp_sorter.proc_position= arr[now/2].proc_position;
               
               arr[now/2].label_id = arr[now].label_id;
               arr[now/2].proc = arr[now].proc;
               arr[now/2].proc_position = arr[now].proc_position;
               
               arr[now].label_id = temp_sorter.label_id;
               arr[now].proc = temp_sorter.proc;
               arr[now].proc_position = temp_sorter.proc_position;
               
               now = now/2;
               if(now == 1) {
                  break;
               }
            }
            
         }
      }
   }
   
   now = 0;
   
   size = label->size;
   
   label->map = (int*) calloc(label->size,sizeof(int));
   
   label->labels[0]= (-1);
   size = 0;
   while(heap_size){     
      
      if(arr[1].label_id != label->labels[label_index]) {
         if((label_index == 0 && label->labels[label_index] != -1) || label_index != 0) {
            label_index++;
         }
         label->labels[label_index] = arr[1].label_id;
      } 
      label->map[label->offset_per_processor[arr[1].proc]+arr[1].proc_position] = label_index;
       
      arr[1].label_id = arr[heap_size].label_id;
      arr[1].proc = arr[heap_size].proc;
      arr[1].proc_position = arr[heap_size].proc_position;
      
      last_element_index = 1;
      heap_size--; 
      for(now = 1; now*2 <= heap_size; now = child) {
         child = now*2;
         if(child != heap_size && arr[child+1].label_id < arr[child].label_id ) 
         {
            child++;
         }

         if(arr[last_element_index].label_id > arr[child].label_id)
         {
            temp_sorter.label_id= arr[last_element_index].label_id;
            temp_sorter.proc= arr[last_element_index].proc;
            temp_sorter.proc_position= arr[last_element_index].proc_position;
               
            arr[last_element_index].label_id = arr[child].label_id;
            arr[last_element_index].proc = arr[child].proc;
            arr[last_element_index].proc_position = arr[child].proc_position;
            
            arr[child].label_id = temp_sorter.label_id;
            arr[child].proc = temp_sorter.proc;
            arr[child].proc_position = temp_sorter.proc_position;
            
            last_element_index = child;            
         }
         else 
         {
            break;
         }
       }
   }
   
   label->size=label_index+1;
   if(arr != NULL) {
      free(arr);
   }

}


void
filter_count_simple(Label *label) {
   int i;
     
   label->map = (int*) calloc(label->size,sizeof(int));
   
   for(i = 0; i < label->size; i++) {
       label->map[i] = i;
   }
}


/****************************************************************
 *  Function:: Constructs the partition map from nodal and
 *  element labels.
 ***************************************************************
 */
int
build_cmap_from_labels( Mili_analysis **in_db, int nproc, TILabels *labels )
{
   Label *working_label;
   for(working_label=labels->labels;working_label != NULL;working_label= working_label->next) {
      filter_count(working_label,nproc);
/*      if ((strcmp(working_label->sname, "node")) == 0) {
          filter_count(working_label,nproc);
      }
      else {
          filter_count_simple(working_label);
      } 
*/          
   }
   return OK;
   
}

/****************************************************************
 *  Function:load_ti_nprocs: Reads the number of procs that
 *  was used in the initial partition.
 ***************************************************************
 */
int
load_ti_nprocs( Mili_analysis *in_db  )
{
   Famid fam_id;
   Mili_family *fam;

   int status;
   int num_entries=0, nprocs=0;
   char *wildcard_list[100];

   fam_id = in_db->db_ident;
   fam    = fam_list[fam_id];

   num_entries = htable_search_wildcard(fam->ti_param_table, 0, FALSE,
                                        "nproc", "NULL", "NULL",
                                        wildcard_list );

   status = mc_ti_read_scalar( fam_id, wildcard_list[0], &nprocs ) ;

   htable_delete_wildcard_list( num_entries, wildcard_list ) ;

   return nprocs;
}


int
get_mat_list_for_proc( Mili_analysis **in_db, int proc,
                       short **mat_list )
{
   int i,j,k;
   int mesh_qty=0, qty_classes=0, obj_qty=0, qty_mats=0;
   int int_args[2];

   Famid fam_id;
   
   int mat_list_index = 0;

   int *nodes=NULL, *mats=NULL, *parts=NULL;

   int   num_sclasses=4, class, superclass, sclasses[4] = {M_HEX, M_QUAD, M_BEAM, M_TET};
   int                                      conn_qty[4] = {8,     4,      3,      4};
   short temp_mat[MAX_MAT], *local_mat_list;

   char short_name[256], long_name[256];
   
   Return_value status;
   if ( !in_db[proc] ) {
      return( 0 );
   }

   fam_id = in_db[proc]->db_ident;
   
   for ( i=0;
         i<MAX_MAT;
         i++ ) {
      temp_mat[i] = -1;
   }

   /* Get number of meshes in family. */
   mc_query_family( fam_id, QTY_MESHES, NULL, NULL, (void *) &mesh_qty );

   for ( i = 0;
         i < mesh_qty;
         i++ ) {
      for ( class=0;
            class<num_sclasses;
            class++ ) {
         superclass = sclasses[class];

         int_args[0] = i;
         int_args[1] = superclass;
         mc_query_family( fam_id, QTY_CLASS_IN_SCLASS, (void *) int_args,
                          NULL, (void *) &qty_classes );

         if ( qty_classes==0 ) {
            continue;
         }

         for ( j = 0;
               j < qty_classes;
               j++ ) {
            status = mc_get_class_info( fam_id, i, superclass, j,
                                        short_name, long_name, &obj_qty );

            nodes = (int *) malloc( obj_qty * conn_qty[class] * sizeof(int) );
            mats  = (int *) malloc( obj_qty *                   sizeof(int) );
            parts = (int *) malloc( obj_qty *                   sizeof(int) );

            status = mc_load_conns( fam_id, i, short_name, nodes,
                                    mats, parts );
            for ( k = 0;
                  k < obj_qty;
                  k++ ) {
               temp_mat[ mats[k] ] = 1;
            }

            free( nodes );
            free( mats );
            free( parts );
         }
      } /* for class */
   } /* for mesh_qty */

   for ( i=0;
         i<MAX_MAT;
         i++ )
      if ( temp_mat[i]>0 ) {
         qty_mats++;
      }

   local_mat_list = (short *) malloc( qty_mats * sizeof(short) );

   /* Collect the list of material numbers for this processor */
   for ( i=0;
         i<MAX_MAT;
         i++ )
      if ( temp_mat[i]>0 ) {
         local_mat_list[ mat_list_index++ ] = i+1;
      }

   *mat_list = local_mat_list;

   return( qty_mats );
}
