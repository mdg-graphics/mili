/*
 * combine_db.c
 *
 *      Lawrence Livermore National Laboratory
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - June 19, 2008: Fixed a problem with mapping contact
 *  segment (CSEG) class - QUADS.
 *  SCR# 541
 *
 *  I. R. Corey - February 28, 2009: Fixed a problem with combining
 *  multiple global variables of type vector and vector_array.
 *  SCR# 576
 *
 *  I. R. Corey - April 02, 2009: Fixed a problem with combining multiple
 *  classes for a superclass when no labels are present - this is specific
 *  to Diablo since no other code generates multiple class data for a
 *  superclass without label data. The element and node numbers are not
 *  getting set to global quantities so we need to create labels dynam-
 *  ically to pass to Griz.
 *  SCR# 583
 *
 *  I. R. Corey - November 16, 2009: Added combining and writing labels
 *     for meshless elements (ML).
 *
 *  I. R. Corey - February 18, 2010: Added capability to combine multiple
 *     node sub-records for TH databases (for non-TH databases we can
 *     combine any number of node sub-records).
 *     for meshless elements (ML).
 *     SCR# 662
 *
 *  I. R. Corey - February 24, 2010: Fixed bug with combining PSEGs that
 *  was introduced with changes to combine PSEGs and CSEGs.
 *
 *  I. R. Corey - February 25, 2010: Added another change to correct
 *  problem with PSEGs and CSEGs.
 *
 *  I. R. Corey - May 10, 2011: Fixed a bug with combine nodal TH data.
 *
 ************************************************************************
 */
#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <dirent.h>
#else
#include <direct.h>
#include <sys/stat.h>
#endif

#include "sarray.h"

#include "driver.h"
#include "mili_enum.h"

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;


/*****************************************************************
 * TAG( combine_umesh ) LOCAL
 *
 *  Combine mesh table information;
 */
void
combine_umesh( Mili_analysis *in_db, Mili_analysis *out_db )
{
   Mili_family *in_fam, *out_fam;
   int in_dbid,out_dbid;
   int i, j, mesh_qty;
   int dims, mesh_id;
   Bool_type found;
   char *mname;

   in_dbid = in_db->db_ident;
   in_fam = fam_list[in_dbid];

   out_dbid = out_db->db_ident;
   out_fam = fam_list[out_dbid];

   for ( i = 0; i < in_fam->qty_meshes; i++ ) {
      str_dup( &mname, in_fam->meshes[i]->name );
      found = FALSE;
      mesh_qty = out_fam->qty_meshes;
      for ( j = 0; j < mesh_qty; j++ ) {
         if ( strcmp( out_fam->meshes[j]->name, mname ) == 0 ) {
            found = TRUE;
         }
      }

      if( !found ) {
         dims = in_fam->dimensions;
         mc_make_umesh( out_dbid, mname, dims, &mesh_id );
      }
      free( mname );
   }

   return;
}


/*****************************************************************
 *
 * TAG( mc_combine_classes ) PUBLIC
 *
 */
Return_value
mc_combine_classes(Mili_family *in_fam,Mili_family *out_fam) {
   int class_qty_in,
       id_blocks,
       *ids,
       i,
       j,
       superclass;
   char short_name[128];
   char long_name[128];

   Return_value rval;
   int query[1];
   int modifiers[2];
   modifiers[1]=M_ALL;

   for(i=0 ; i< in_fam->qty_meshes; i++) {

      modifiers[0] = i;
      rval = get_class_qty(in_fam,modifiers,&class_qty_in);

      if(rval==OK) {
         for(j=0; j<class_qty_in; j++) {
            rval = mc_get_class_info_by_index( in_fam, &i, &j, &superclass,
                                               short_name, long_name);

            if(rval==OK) {
               rval = mc_query_family( out_fam->my_id, CLASS_SUPERCLASS,
                                       modifiers, short_name,query );

               if(rval==ENTRY_NOT_FOUND) {
                  rval = mc_def_class(out_fam->my_id,i,superclass,
                                      short_name, long_name);
                  if(superclass == M_UNIT  ||
                   superclass == M_MAT   ||
                   superclass == M_MESH  ||
                   superclass == M_SURFACE ){
                   
                     rval = (Return_value) mc_get_class_idents(in_fam->my_id,i,short_name, &id_blocks,&ids);
                     rval = mc_def_class_idents(out_fam->my_id,i,short_name, ids[0],ids[1]);
                     free(ids);
                  }
               }
            }
         }
      }
   }
   return rval;
}

Return_value
define_variable(Mili_family* in, Mili_family *out, char *variable_name)
{
  State_variable state_var;
  Return_value rval;
  int idx;
  int in_fam_id = in->my_id;
  int out_fam_id = out->my_id;
  Htable_entry *phte;
  
  rval = mc_get_svar_def(in_fam_id, variable_name, &state_var);
  
  if(rval != OK) {
    mc_print_error("mc_get_svar_def:" ,rval);
    return rval;
  }
  
  int types[1];

  switch (state_var.agg_type) {
    case (SCALAR):
      types[0] = state_var.num_type;
      rval = mc_def_svars(out_fam_id,1,
                          state_var.short_name,0,
                          state_var.long_name,0,
                          types);
      break;
    case (ARRAY):
      rval = mc_def_arr_svar(out_fam_id,
                             state_var.rank,
                             state_var.dims,
                             state_var.short_name,
                             state_var.long_name,
                             state_var.num_type);
      break;
    case (VECTOR):
      for( idx=0; idx < state_var.vec_size; idx++)
      {
        rval = define_variable(in, out, state_var.components[idx]);
        if(rval != OK)
        {
          return rval;
        }
      }
      rval = mc_def_vec_svar(out_fam_id,
                             state_var.num_type,
                             state_var.vec_size,
                             state_var.short_name,
                             state_var.long_name,
                             state_var.components,
                             state_var.component_titles);
      break;
    case(VEC_ARRAY):
      for( idx=0; idx < state_var.vec_size; idx++)
      {
        rval = define_variable(in, out, state_var.components[idx]);
        if(rval != OK)
        {
          return rval;
        }
      }
      rval = mc_def_vec_arr_svar(out_fam_id,
                                 state_var.rank,
                                 state_var.dims,
                                 state_var.short_name,
                                 state_var.long_name,
                                 state_var.vec_size,
                                 state_var.components,
                                 state_var.component_titles,
                                 state_var.num_type);
      break;
    default:
      break;
  }//End switch/case
  
  mc_cleanse_st_variable(&state_var);

  return rval;
}

Return_value 
mc_combine_svars(Mili_family* in, Mili_family *out) {
   int svar_count=0,
       cur_srec=0,
       cur_subrec=0,
       srec_qty = in->qty_srecs,
       subrec_qty=0,
       cur_svar=0,
       error = 0;
   Return_value rval=(Return_value)OK;
   Return_value status;
   Srec *base_psr;
   
   for(; cur_srec< srec_qty; cur_srec = cur_srec+1) {
      base_psr = in->srecs[cur_srec];
      subrec_qty = base_psr->qty_subrecs;
      cur_subrec = 0;

      for(; cur_subrec<subrec_qty; cur_subrec = cur_subrec+1) {
         Subrecord subrecord;
         rval = mc_get_subrec_def( in->my_id, cur_srec, cur_subrec, &subrecord );

         if(rval == OK) {
            for(cur_svar = 0; cur_svar < subrecord.qty_svars; cur_svar++) {
              rval = define_variable(in, out, subrecord.svar_names[cur_svar]);
              if(rval != OK) return rval;
            }
         } else {
            mc_print_error("Load Subrecord:" ,rval);
         }
         mc_cleanse_subrec(&subrecord);
      }
   }

   return rval;

}

/*****************************************************************
 * TAG( copy_metadata ) LOCAL
 *
 *
 */
void
mc_copy_metadata( Mili_family *fam , Mili_family * in_fam) {
   int rval;
   char title_bufr[128];
   char mili_version[64],
        xmilics_version[64],
        timestamp[64],
        host[64],
        arch[256],
        username[32];

   Famid outdbid = fam->my_id;
   Famid indbid = in_fam->my_id;
   xmilics_version[0] = '\0';
   sprintf(xmilics_version, "%s(%s)",PACKAGE_VERSION, PACKAGE_DATE);
   mc_wrt_string( outdbid, "xmilics version", xmilics_version );
   mc_wrt_string( outdbid, "xmilics mili version", MILI_VERSION );
   mc_wrt_scalar( outdbid, M_INT, "states per file", &in_fam->states_per_file);


   mili_version[0]    = '\0';
   host[0]            = '\0';
   arch[0]            = '\0';
   timestamp[0]       = '\0';
   xmilics_version[0] = '\0';

   rval = mc_get_metadata(indbid,mili_version,host,arch,timestamp,xmilics_version)  ;
   //mc_read_string( indbid, "lib version", mili_version );
   mc_wrt_string( outdbid, "origin lib version", mili_version );
   
   //mc_read_string( indbid, "arch name", arch);
   mc_wrt_string( outdbid, "origin arch name", arch );
   
   //mc_read_string( indbid, "host name", host );
   mc_wrt_string( outdbid, "origin host name", host);
   
   //mc_read_string( indbid, "date", timestamp );
   mc_wrt_string( outdbid, "origin date", timestamp);


   rval = mc_read_string( indbid, "title", title_bufr );
   rval = mc_wrt_string(outdbid,"title",title_bufr );
   rval = mc_read_string( indbid, "username", username );
   rval = mc_wrt_string(outdbid,"origin username", username);
   
   mc_print_error("write_string",rval);
   

   return;
}

/*****************************************************************
 * TAG( mc_ti_get_data_len ) PUBLIC
 *
 * Search a hash table for an entry and return the fields type and
 * length.
 */
Return_value
mc_get_data_len( Famid fam_id, char *name, int *datatype, long *datalength )
{
   Htable_entry *phte;
   Param_ref *p_pr;
   LONGLONG *entry;
   int entry_idx, file;
   int       len_bytes;
   Mili_family *fam;
   int order=1;
   Return_value status;

   fam = fam_list[fam_id];

   status = htable_search( fam->param_table, name, FIND_ENTRY, &phte );

   if ( phte == NULL )
   {
      return status;
   }

   p_pr = ( Param_ref * ) phte->data;

   file      = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get the type and length of the data. */
   entry        = fam->directory[file].dir_entries[entry_idx];
   len_bytes    = entry[LENGTH_IDX];
   *datatype    = entry[MODIFIER1_IDX];


   switch (*datatype)
   {
      case (M_INT):
         *datalength =  len_bytes/fam->external_size[M_INT];
         break;

      case (M_INT8):
         *datalength = len_bytes/fam->external_size[M_INT8];
         break;

      case (M_STRING):
         *datalength = len_bytes;
         break;

      case (M_FLOAT):
      case (M_FLOAT4):
         *datalength = len_bytes/fam->external_size[M_FLOAT];
         break;

      case (M_FLOAT8):
         *datalength = len_bytes/fam->external_size[M_FLOAT8];
         break;

      default:
         return INVALID_DATA_TYPE;
         break;
   }

   return (Return_value)OK;
}


Return_value
add_parameter(int fam_id,int out_fam_id,char* name) {
   
   Return_value status = (Return_value)OK;
   int datatype;
	long datalength;
   int dims[3];
   void *data;
   int *data_int = NULL;
   long *data_long = NULL;
   char *data_char = NULL;
   float *data_float;
   double *data_double; 
   
   mc_get_data_len(fam_id,name,&datatype,&datalength);
   
   switch (datatype) {
      case (M_INT):
         data_int = (void *) malloc( datalength*sizeof(int) );
         status = mc_read_scalar(fam_id, name,(void*)data_int);
         if(status == OK) {
            mc_wrt_scalar(out_fam_id,datatype,name,(void*)data_int);
         }
         free(data_int);
         break;

      case (M_INT8):
         data_long = (void *) malloc( datalength*sizeof(long) );
         status = mc_read_scalar(fam_id, name,(void*)data_long);
         if(status == OK) {
            mc_wrt_scalar(out_fam_id,datatype,name,(void*)data_long);
         }
         free(data_long);
         break;

      case (M_STRING):
         data_char = (void *) malloc( datalength*sizeof(char) );
         status = mc_read_string(fam_id,name,data_char);
         if (status == OK) {
            mc_wrt_string(out_fam_id, name, data_char);
         }
         free(data_char);
         break;

      case (M_FLOAT):
      case (M_FLOAT4):
         data_float = (void *) malloc( datalength*sizeof(float) );
         status = mc_read_scalar(fam_id, name,(void*)data_float);
         if(status == OK) {
            mc_wrt_scalar(out_fam_id,datatype,name,(void*)data_float);
         }
         free(data_float);
         break;

      case (M_FLOAT8):
         data_double = (void *) malloc( datalength*sizeof(double) );
         status = mc_read_scalar(fam_id, name,(void*)data_double);
         if(status == OK) {
            mc_wrt_scalar(out_fam_id,datatype,name,(void*)data_double);
         }
         free(data_double);
         break;
   } /* switch */
   
   return status;
}

Return_value
add_ti_parameter(int fam_id,int fam_id_out,char* name) {
   
   if ( strncmp( "Element Labels", name, strlen("Element Labels"))==0 ||
        strncmp( "Node Labels",  name, strlen("Node Labels"))==0 ) {
      return (Return_value)OK;
   }
   Return_value status;
   int    *data_int = NULL,    data_int_scalar;
   long   *data_long = NULL,   data_long_scalar;
   double *data_double= NULL, data_double_scalar;
   float  *data_float= NULL,  data_float_scalar;
   char   *data_char= NULL;
   
   int datatype = 0, 
       num_items_read=0;
   long datalength = 0;
            
   int dims[3];
            
   status = mc_get_data_len( fam_id, name, &datatype,  &datalength );
   
   if(datalength ==1 || datatype==M_STRING) {
      switch (datatype) {
         case (M_INT):
            status = mc_ti_read_scalar( fam_id,   name, &data_int_scalar ) ;
            if(status == OK) {
               status = mc_ti_wrt_scalar( fam_id_out, M_INT, name, (void *) &data_int_scalar ) ;
            }
            break;

         case (M_INT8):
            status = mc_ti_read_scalar( fam_id,   name, &data_long_scalar ) ;
            if(status == OK) {
               status = mc_ti_wrt_scalar( fam_id_out, M_INT8, name, (void *) &data_long_scalar ) ;
            }
            break;

         case (M_STRING):
            data_char = (char*)malloc(sizeof(char)*datalength);
            status = mc_ti_read_string( fam_id, name, data_char ) ;
            if(status == OK) {
               status = mc_ti_wrt_string( fam_id_out, name, data_char ) ;
            }
            break;

         case (M_FLOAT):
         case (M_FLOAT4):
            status = mc_ti_read_scalar( fam_id, name, &data_float_scalar ) ;
            if(status == OK) {
               status = mc_ti_wrt_scalar( fam_id_out, M_FLOAT, name, (void *) &data_float_scalar ) ;
            }
            break;

         case (M_FLOAT8):
            status = mc_ti_read_scalar( fam_id, name, &data_double_scalar ) ;
            if(status == OK) {
               status = mc_ti_wrt_scalar( fam_id_out, M_FLOAT8, name, (void *) &data_double_scalar ) ;
            }
            break;
      }
   }else {
      dims[0] = datalength;
      dims[1] = 0;
      dims[2] = 0;
      switch (datatype) {
         case (M_INT):
            status  = mc_ti_read_array( fam_id, name, (void **) &data_int, &num_items_read ) ;
            if(status == OK) {
               dims[0] = num_items_read;
               status  = mc_ti_wrt_array( fam_id_out,  datatype, name , 1, dims,
                                          (void *)  data_int );
            }
            
            break;
         case (M_INT8):
            status  = mc_ti_read_array( fam_id, name, (void **) &data_long, &num_items_read ) ;
            if(status == OK) {
               dims[0] = num_items_read;
               status  = mc_ti_wrt_array( fam_id_out,  datatype, name , 1, dims,
                                          (void *)  data_long );
            }
            break;

         case (M_FLOAT4):
         case (M_FLOAT):
            status  = mc_ti_read_array( fam_id, name, (void **) &data_float, &num_items_read ) ;
            if(status == OK) {
               dims[0] = num_items_read;
               status  = mc_ti_wrt_array( fam_id_out,  datatype, name , 1, dims,
                                          (void *)  data_float );
            }
            break;
         case (M_FLOAT8):
            status  = mc_ti_read_array( fam_id, name, (void **) &data_double, &num_items_read ) ;
            if(status == OK) {
               dims[0] = num_items_read;
               status  = mc_ti_wrt_array( fam_id_out,  datatype, name , 1, dims,
                                          (void *)  data_double );
            }
            break;

      } /* switch */
   }
   if ( data_int!= NULL) {
      free( data_int ) ;
   }
   if ( data_long!= NULL) {
      free( data_long ) ;
   }
   if ( data_char!= NULL) {
      free( data_char ) ;
   }
   if ( data_float!= NULL) {
      free( data_float ) ;
   }
   if ( data_double!= NULL) {
      free( data_double ) ;
   }
   return status;         
}

Return_value
mc_copy_geometry(Mili_analysis **in_db, Mili_analysis *out_db,TILabels *labels) {
   
   int cur_proc,i,j,
       dims = labels->dimensions,
       size,
       local_index,
       node_size=0,
       conn_size = -1,
       label_size = -1,
       conns,
       position,
       num_procs = labels->num_procs;
   LONGLONG current_offset,
         node_offset;
   Label *iter,*nodeLabel= NULL;
   float *temp_coords=NULL;
   int *temp_conns=NULL,
       *temp_mats=NULL,
       *temp_parts=NULL,
       *out_conns=NULL;
   Return_value status = (Return_value)OK;
   
   for(iter = labels->labels ;iter !=NULL; iter = iter->next) {
   
      size = iter->size;
      
      if(iter->sclass == M_NODE) {
         nodeLabel = iter;
         float *coords = (float*) malloc(sizeof(float)*size*labels->dimensions);
         current_offset = 0;
         for(cur_proc = 0 ; cur_proc <num_procs; cur_proc++) { 
         
            if(env.num_selected_procs>0 && !env.selected_proc_list[cur_proc]) {
               current_offset +=  iter->num_per_processors[cur_proc];
               continue;
            }
            if(iter->num_per_processors[cur_proc] ==0) {
               continue;
            }
            if(node_size < iter->num_per_processors[cur_proc]) {
               node_size = iter->num_per_processors[cur_proc];
               if(temp_coords != NULL) {
                  free(temp_coords);
               }
               temp_coords = (float*) malloc(sizeof(float)*labels->dimensions*node_size);
               if(temp_coords == NULL) {
                  if(coords != NULL) {
                     free(coords);
                     coords = NULL;
                  }
                  break;
               }
               
            }
            status = mc_load_nodes(in_db[cur_proc]->db_ident,iter->mesh_id,iter->sname,temp_coords);
            if(status != OK) {
               mc_print_error("mc_load_nodes: " , status);
               if(coords != NULL) {
                  free(coords);
               }
               if(temp_coords != NULL) {
                  free(temp_coords);
               }
               return status;
            }
            for(i=0;i< iter->num_per_processors[cur_proc]; i++){
               coords[dims *iter->map[current_offset+i]] = temp_coords[ dims*i];
               coords[dims *iter->map[current_offset+i]+1] = temp_coords[dims*i+1];
               coords[dims *iter->map[current_offset+i]+2] = temp_coords[dims*i+2];
            }
            current_offset += iter->num_per_processors[cur_proc];
            
         }
         if(temp_coords != NULL) {
            free(temp_coords);
            temp_coords = NULL;
         }
         status = mc_def_nodes(out_db->db_ident,iter->mesh_id,
                               iter->sname,1,iter->size,coords);
         mc_print_error("mc_def_nodes: " ,status);
         status = mc_def_node_labels(out_db->db_ident,iter->mesh_id,
                               iter->sname,iter->size,iter->labels);
         mc_print_error("mc_def_nodes: " ,status);
         if(coords != NULL) {
            free (coords);
            coords = NULL;
         }
      }
   }
   
   
   if(nodeLabel == NULL) {
      return NOT_OK;
   }
   
   
   for(iter = labels->labels ;iter !=NULL; iter = iter->next) {
      
      if(iter->sclass == M_NODE) {
         continue;
      }
      size = iter->size;
      
      conns = class_conns[iter->sclass];
      out_conns = (int*) calloc(size*(conns+2),sizeof(int));
      current_offset = 0;
      node_offset = 0;
      for(cur_proc = 0 ; cur_proc <num_procs; cur_proc++) { 
         
         node_offset = nodeLabel->offset_per_processor[cur_proc];
         if(env.num_selected_procs>0 && !env.selected_proc_list[cur_proc]) {
            current_offset +=  iter->num_per_processors[cur_proc];
            continue;
         }
         if(iter->num_per_processors[cur_proc] ==0) {
            continue;
         }
            
         if(conn_size < iter->num_per_processors[cur_proc]) {
            conn_size = iter->num_per_processors[cur_proc];
            if(temp_conns != NULL) {
               free(temp_conns);
               temp_conns = NULL;
            }
            if(temp_mats != NULL) {
               free(temp_mats);
               temp_mats = NULL;
            }
            if(temp_parts != NULL) {
               free(temp_parts);
               temp_parts = NULL;
            }
            temp_conns = (int*) calloc(conn_size*conns,sizeof(int));
            temp_mats = (int*) calloc(conn_size,sizeof(int));
            temp_parts = (int*) calloc(conn_size,sizeof(int));
            if(temp_conns == NULL || temp_mats == NULL || temp_parts == NULL) {
               if(out_conns != NULL ) {
                  free(out_conns);
                  out_conns = NULL;
               }
               if(temp_conns != NULL) {
                  free(temp_conns);
                  temp_conns = NULL;
               }
               if(temp_mats != NULL) {
                  free(temp_mats);
                  temp_mats = NULL;
               }
               if(temp_parts != NULL) {
                  free(temp_parts);
                  temp_parts = NULL;
               }
               break;
            }
               
         }
         status = mc_load_conns(in_db[cur_proc]->db_ident,iter->mesh_id,
                                iter->sname,temp_conns,temp_mats,temp_parts);
         if(status != OK) {
            mc_print_error("mc_load_conns: " , status);
            return status;
         }
         
         for(i=0;i< iter->num_per_processors[cur_proc]; i++){
            position = iter->map[current_offset+i]*(conns+2);
            for(j=0 ;j<conns;j++) {
               out_conns[position+j] = nodeLabel->map[node_offset+temp_conns[(i*conns)+j]]+1;
            }
            out_conns[position+j] = temp_mats[i]+1;
            out_conns[position+j+1] = temp_parts[i]+1;
            
         }
         current_offset += iter->num_per_processors[cur_proc];
      }
      if(temp_conns != NULL) {
         free(temp_conns);
         temp_conns = NULL;
      }
      if(temp_mats != NULL) {
         free(temp_mats);
         temp_mats = NULL;
      }
      if(temp_parts != NULL) {
         free(temp_parts);
         temp_parts = NULL;
      }
      conn_size= -1;
      status = mc_def_conn_seq_labels(out_db->db_ident,iter->mesh_id,
                                      iter->sname,1,iter->size, iter->labels,
                                      out_conns);
      if(out_conns != NULL) {
         free(out_conns);
         out_conns = NULL;
      }
   }
   return status;
}

/* TAG (mc_subrec_exists)
*
*/
Return_value
mc_subrec_exists(Famid fam_id , int srec_id, char* subrec_name,char* class_name){
   Return_value rval= (Return_value)OK;
   Mili_family *fam;
   Htable_entry *subrec_entry;
   
   fam = fam_list[fam_id];
   if ( fam->subrec_table == NULL )
   {
       return ENTRY_NOT_FOUND;
   }
   rval = htable_search( fam->subrec_table, subrec_name,
                           FIND_ENTRY, &subrec_entry );
    
   return rval;
}
/* TAG (convert_local_moids)
*
*/
Return_value
convert_local_moids(int **global_ids, int* in_count, Subrecord *subrec, int *map) {
   
   
   int *temp_list = (int*) calloc(subrec->qty_objects,sizeof(int));
   int count = *in_count;
   Return_value rval = (Return_value)OK;
   int *ids,i,j=0,k=0;
   int min, temp0, temp1;
   for(i=0;i<subrec->qty_blocks; i++) {
      for(j=subrec->mo_blocks[(i*2)]-1; j<subrec->mo_blocks[(i*2)+1];j++,k++) {
         temp_list[k] = map[j]+1;
      }
   }
   rval = list_to_blocks(subrec->qty_objects,temp_list,global_ids,in_count);
   count = *in_count;
   ids = global_ids[0];
   if(count>1) {
      
      for(i = 0 ; i<count ; i++) {
         min = i;
         for(j=i+1; j< count; j++) {
            if(ids[j*2] < ids[min*2]) {
               min=j;
            }
         }
         temp0 = ids[i*2];
         temp1 = ids[(i*2)+1];
         ids[i*2] = ids[min*2];
         ids[(i*2)+1] = ids[(min*2)+1];
         ids[min*2] = temp0;
         ids[(min*2)+1] = temp1;
      }      
   }
   free(temp_list);
   return rval;
}


Return_value
mc_merge_subrec_ids(Subrecord *out_subrec,Subrecord *in_subrec,int *in_ids,int count){
   
   Return_value rval = (Return_value)OK;
   int i=0,
       j=0,
       k=0,
       new_count=0,
       old_count=out_subrec->qty_blocks,
       last_value=0;
   int *combined;
   int *temp;
   combined = (int*) calloc ((count+out_subrec->qty_blocks)*2,sizeof(int));
   if(in_ids[0]<out_subrec->mo_blocks[0]) {
      combined[0] = in_ids[0];
      combined[1] = in_ids[1];
      i=1;
   }else {
      combined[0] = out_subrec->mo_blocks[0];
      combined[1] = out_subrec->mo_blocks[1];
      j=1;
   }
   for(;i<count && j<old_count;) {
      if( in_ids[i*2]<out_subrec->mo_blocks[j*2]) {
         if(in_ids[i*2] <combined[k*2+1] || in_ids[i*2] == combined[k*2+1]+1) {
            if(in_ids[i*2+1] > combined[k*2+1]){
               combined[k*2+1] = in_ids[i*2+1];
            }                
         }else if(in_ids[i*2] >combined[k*2+1]) {
            k++;
            combined[k*2] = in_ids[i*2];
            combined[k*2+1] = in_ids[i*2+1];
         }else {
            combined[k*2+1] = in_ids[i*2+1];
         }
         i++;
      }else if( in_ids[i*2]>out_subrec->mo_blocks[j*2]) {
         if(out_subrec->mo_blocks[j*2] <=combined[k*2+1] || 
            out_subrec->mo_blocks[j*2] ==combined[k*2+1]+1) {
            
            if(out_subrec->mo_blocks[j*2+1] >combined[k*2+1]) {
               combined[k*2+1] = out_subrec->mo_blocks[j*2+1];    
            }              
         }else if(out_subrec->mo_blocks[j*2] >combined[k*2+1]) {
            k++;
            combined[k*2] = out_subrec->mo_blocks[j*2];
            combined[k*2+1] = out_subrec->mo_blocks[j*2+1];
         }else {
            combined[k*2+1] = out_subrec->mo_blocks[i*2+1];
         }
         j++;
      }else {
         if(in_ids[i*2+1]>out_subrec->mo_blocks[j*2+1]){
            last_value = in_ids[i*2+1];
         }else{
            last_value = out_subrec->mo_blocks[j*2+1];
         }
         if(in_ids[i*2] <combined[k*2+1] || in_ids[i*2] == combined[k*2+1]+1) {
            if(last_value > combined[k*2+1]) {
               combined[k*2+1] = last_value;
            }             
         }else if(in_ids[i*2] >combined[k*2+1]) {
            k++;
            combined[k*2] = in_ids[i*2];
            combined[k*2+1] = last_value;
         }else {
            combined[k*2+1] = last_value;
         }
         i++;
         j++;
      }
   }
   int index;
   if(i<count) {
      temp = in_ids;
      index = i;
   }else {
      temp = out_subrec->mo_blocks;
      index = j;
      count = old_count;
   }
   while(index < count) {
      if(temp[index*2] <= combined[k*2+1] || temp[index*2] == combined[k*2+1]+1) {
         if(temp[index*2+1]>combined[k*2+1]) {
            combined[k*2+1] = temp[index*2+1];
         }
      }else {
         k++;
         combined[k*2+1] = temp[index*2+1];
         combined[k*2] = temp[index*2];
      }
      index++;
   }
   
   temp=NULL;
   free(out_subrec->mo_blocks);
   out_subrec->mo_blocks = (int*) calloc((k+1)*2,sizeof(int));
   out_subrec->qty_objects = 0;
   for(i=0; i<=k;i++) {
      out_subrec->mo_blocks[i*2] = combined[i*2];
      out_subrec->mo_blocks[(i*2)+1] = combined[(i*2)+1];
      out_subrec->qty_objects +=  out_subrec->mo_blocks[(i*2)+1]-out_subrec->mo_blocks[i*2]+1;
   }
   free(combined);
   out_subrec->qty_blocks = k+1;
   
   return rval;
}

/* TAG (get_subrec_contributions) PRIVATE
*
*/
Return_value
get_subrec_contributions(Mili_analysis **in_db,Mili_analysis *out_db,TILabels *in_labels){

   int num_procs = in_labels->num_procs;
   Mili_family *in_fam,*out_fam;
   SubrecContainer *subrec_container = NULL,
                   *working_container = NULL,
                   *last_container= NULL;
   Famid in_dbid;
        
   int i,j,k,l,
       srec_id,
       out_srec_id,
       cur_proc,
       srec_qty,
       out_srec_qty=0,
       count,
       mesh_id,
       out_subrec_qty=0,
       subrec_qty,
       stype,
       found,
       name_stride = M_MAX_NAME_LEN,
       max_qty_svars = -1;
       
   Subrecord subrec,
             out_subrec,
             test_subrec;
   
   Srec *p_sr, *out_psr;
   
   char *subrec_name, 
        *sv_name, 
        *title;
   
   Return_value rval;
   
   out_fam = fam_list[out_db->db_ident];             
   for(cur_proc = 0; cur_proc < num_procs; cur_proc++){
   
      if(env.num_selected_procs>0 && !env.selected_proc_list[cur_proc]) {
         continue;
      }
      
      in_dbid = in_db[cur_proc]->db_ident;
      in_fam = fam_list[in_dbid];
      /* Get state record format count for this database. */
      rval = mc_query_family( in_dbid, QTY_SREC_FMTS, NULL, NULL,
                              (void *) &srec_qty );
      if(out_srec_qty < srec_qty) {
         out_srec_qty = srec_qty;
      }
      for( srec_id = 0; srec_id < srec_qty; srec_id++ ) {
         rval = mc_query_family( in_dbid, SREC_MESH, &srec_id, NULL,
                              (void *) &mesh_id );         
         
         p_sr = in_fam->srecs[srec_id];
         subrec_qty = p_sr->qty_subrecs;

         for( i = 0; i < subrec_qty; i++ ) {
            
            rval = mc_get_subrec_def(in_dbid , srec_id, i, &subrec);
            rval = mc_query_family( in_dbid, CLASS_SUPERCLASS, &mesh_id,
                                 subrec.class_name, (void *)&stype );
            if( rval != OK ) {
               mc_print_error("Error: mc_query_family(CLASS_SUPERCLASS): ",rval);
               return rval;
            }
            
            found = 0;
            last_container = NULL;
            
            if(subrec_container != NULL){
               
               working_container = subrec_container;
               
               while(working_container != NULL) {
                  
                  last_container = working_container;
                  
                  if(strcmp(working_container->subrecord.name, subrec.name) ==0 &&
                     
                     strcmp(working_container->subrecord.class_name, subrec.class_name)==0 &&
                     working_container->srec_id == srec_id){
                     found = 1;
                     break;
                     
                  }
                  
                  working_container = working_container->next;
               
               }
            }
            
            if(!found){
               
               if(subrec_container == NULL){
                  
                  subrec_container = (SubrecContainer *) malloc(sizeof(SubrecContainer));
                  subrec_container->next = NULL;
                  working_container = subrec_container;
               
               }else{
                  
                  last_container->next = (SubrecContainer *) malloc(sizeof(SubrecContainer));
                  working_container = last_container->next;
                  working_container->next = NULL;
               
               }
               working_container->stype = stype;
               working_container->srec_id= srec_id;
               working_container->in_subrec_per_proc = (short*) calloc(num_procs,sizeof(short));
               working_container->subrec_offset_per_proc = 0;
            }
            
            working_container->in_subrec_per_proc[cur_proc]++;
            
            if(found) {
               
               mc_cleanse_subrec(&subrec);
               
            }else {
               
               out_subrec_qty++;
               
               working_container->subrecord.name = subrec.name; 
               working_container->subrecord.organization = subrec.organization;
               working_container->subrecord.qty_svars = 0;
               
               if(working_container->subrecord.qty_svars > max_qty_svars) {
                  max_qty_svars = working_container->subrecord.qty_svars;
               }
               
               working_container->subrecord.svar_names =NULL;
               working_container->subrecord.class_name = subrec.class_name;
               working_container->subrecord.qty_objects = subrec.qty_objects;
               
               working_container->subrecord.mo_blocks = NULL;
               working_container->subrecord.surface_variable_flag = NULL;
               
               subrec.mo_blocks = NULL;
            }
            
         }
      }  
   }
   
   if(out_srec_qty>0 && num_procs>0){
      in_labels->subrec_contributions = (short*) calloc(num_procs*out_subrec_qty, sizeof(short));      
   }
   
   for(srec_id=0; srec_id < out_srec_qty; srec_id++) {
      
      out_srec_id = out_fam->qty_srecs - 1;
       
      
      j=0;          
      
      for(working_container = subrec_container; 
         working_container != NULL; 
         working_container = working_container->next) {
         
         if( working_container->srec_id != out_srec_id){
            continue;
         }
      
         for(i=0;i<num_procs;i++) {
            in_labels->subrec_contributions[i*out_subrec_qty+j] = working_container->in_subrec_per_proc[i];
         }
         j++;
         
      }
   }
   
   working_container = subrec_container;
   subrec_container = NULL;
   
   while(working_container != NULL) {
      
      if(working_container->in_subrec_per_proc != NULL) {
         free(working_container->in_subrec_per_proc);
      }
      
      mc_cleanse_subrec(&(working_container->subrecord));
      
      last_container = working_container->next;
      
      free(working_container);
      
      working_container = last_container;
      
   }
   
   return rval;
}
/* TAG (mc_combine_srecs)
*
*/
Return_value
mc_combine_srecs(Mili_analysis **in_db,Mili_analysis *out_db,TILabels *in_labels){
   
   int num_procs = in_labels->num_procs;
   
   Mili_family *in_fam, 
               *out_fam;
               
   Famid in_dbid, 
         out_dbid;
   
   SubrecContainer *subrec_container = NULL,
                   *working_container = NULL,
                   *last_container= NULL;
                   
   int i,j,k,l,
       srec_id,
       out_srec_id,
       cur_proc,
       srec_qty,
       out_srec_qty=0,
       count,
       mesh_id,
       out_subrec_qty=0,
       subrec_qty,
       stype,
       found,
       name_stride = M_MAX_NAME_LEN,
       max_qty_svars = -1;
   
   Sub_srec *out_psubrec;
   
   Subrecord subrec,
             out_subrec,
             test_subrec;
   
   Srec *p_sr, *out_psr;
   
   char *subrec_name, 
        *sv_name, 
        *title;
   
   Return_value rval;
   
   Label *labels,
         *iter=NULL;
   
   labels = in_labels->labels;  
   
   out_dbid = out_db->db_ident;
   out_fam = fam_list[out_dbid];
   
   char *pi, 
        *temp_chars;
        
   int* global_ids = NULL;
#if TIMER
   clock_t start,stop;
   double cumalative=0.0;
   start = clock();
#endif    
   /* Create the subrecord hash table if it hasn't been. */
   if ( out_fam->subrec_table == NULL ) {
      out_fam->subrec_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
   }
   
   for(cur_proc = 0; cur_proc < num_procs; cur_proc++){
      
      if(env.num_selected_procs>0 && !env.selected_proc_list[cur_proc]) {
         continue;
      }
      
      in_dbid = in_db[cur_proc]->db_ident;
      in_fam = fam_list[in_dbid];
   
      /* Get state record format count for this database. */
      rval = mc_query_family( in_dbid, QTY_SREC_FMTS, NULL, NULL,
                              (void *) &srec_qty );
      if(out_srec_qty < srec_qty) {
         out_srec_qty = srec_qty;
      }
      
      for( srec_id = 0; srec_id < srec_qty; srec_id++ ) {
         rval = mc_query_family( in_dbid, SREC_MESH, &srec_id, NULL,
                              (void *) &mesh_id );         
         
         p_sr = in_fam->srecs[srec_id];
         subrec_qty = p_sr->qty_subrecs;

         for( i = 0; i < subrec_qty; i++ ) {
            
            rval = mc_get_subrec_def(in_dbid , srec_id, i, &subrec);
            rval = mc_query_family( in_dbid, CLASS_SUPERCLASS, &mesh_id,
                                 subrec.class_name, (void *)&stype );
            if( rval != OK ) {
               mc_print_error("Error: mc_query_family(CLASS_SUPERCLASS): ",rval);
               return rval;
            }
            
            found = 0;
            last_container = NULL;
            
            if(subrec_container != NULL){
               
               working_container = subrec_container;
               
               while(working_container != NULL) {
                  
                  last_container = working_container;
                  
                  if(strcmp(working_container->subrecord.name, subrec.name) ==0 &&
                     
                     strcmp(working_container->subrecord.class_name, subrec.class_name)==0 &&
                     working_container->srec_id == srec_id){
                     found = 1;
                     break;
                     
                  }
                  
                  working_container = working_container->next;
               
               }
            }
            
            if(!found){
               
               if(subrec_container == NULL){
                  
                  subrec_container = (SubrecContainer *) malloc(sizeof(SubrecContainer));
                  subrec_container->next = NULL;
                  working_container = subrec_container;
               
               }else{
                  
                  last_container->next = (SubrecContainer *) malloc(sizeof(SubrecContainer));
                  working_container = last_container->next;
                  working_container->next = NULL;
               
               }
               working_container->stype = stype;
               working_container->srec_id= srec_id;
               working_container->in_subrec_per_proc = (short*) calloc(num_procs,sizeof(short));
               /*
               working_container->subrec_entries_from_each_proc = (int*) calloc(num_procs,sizeof(int));
               
               working_container->subrecord_map_for_each_proc = (int **) malloc(num_procs*sizeof(int*));
               
               for(k=0; k<num_procs; k++) {
                  working_container->subrecord_map_for_each_proc[k] = NULL;
               }
               
               
               working_container->subrecord_offsets_per_proc = (int*) calloc(num_procs,sizeof(int));
               */  
               working_container->subrec_offset_per_proc = 0;
            }
            
            working_container->in_subrec_per_proc[cur_proc]++;
            
            if(found) {
               
               if( ( stype != M_MESH ) &&
                  ( stype != M_MAT  ) &&
                  ( stype != M_UNIT ) ) {
                  
                  for(iter = labels;iter != NULL; iter = iter->next) {
                     if(strcmp(iter->sname, subrec.class_name)==0) {
                        break;
                     }
                  }
                  
                  if(iter == NULL) {
                     mc_print_error("No labels found to map subrecord",5000);
                  }
                  
                  rval = convert_local_moids(&global_ids, &count ,&subrec, iter->map + iter->offset_per_processor[cur_proc]);
                  /*working_container->total_external_contributions+=count;*/
                  rval = mc_merge_subrec_ids(&(working_container->subrecord),&subrec,global_ids,count);
                  /*
                  if(working_container->subrecord_map_for_each_proc[cur_proc] == NULL) {
                     working_container->subrecord_map_for_each_proc[cur_proc] = global_ids;
                  }else {
                     working_container->subrecord_map_for_each_proc[cur_proc] =   
                        RENEW_N(int, working_container->subrecord_map_for_each_proc[cur_proc],
                                working_container->subrec_entries_from_each_proc[cur_proc]*2,
                                count*2, "subrec_map");
                     for(j=0,k=working_container->subrec_entries_from_each_proc[cur_proc]; j<count;k++,j++) {
                        working_container->subrecord_map_for_each_proc[cur_proc][(k*2)] =
                           global_ids[j];
                        working_container->subrecord_map_for_each_proc[cur_proc][(k*2)+1] =
                           global_ids[j+1];
                     }
                  }
                  
                  working_container->subrec_entries_from_each_proc[cur_proc] += count;
                  */
                  if(global_ids != NULL) {
                     free(global_ids);
                     global_ids = NULL;
                  }
               
               }
               
               mc_cleanse_subrec(&subrec);
               
            }else {
               
               out_subrec_qty++;
               
               working_container->subrecord.name = subrec.name; 
               working_container->subrecord.organization = subrec.organization;
               working_container->subrecord.qty_svars = subrec.qty_svars;
               
               if(working_container->subrecord.qty_svars > max_qty_svars) {
                  max_qty_svars = working_container->subrecord.qty_svars;
               }
               
               working_container->subrecord.svar_names =subrec.svar_names;
               working_container->subrecord.class_name = subrec.class_name;
               working_container->subrecord.qty_objects = subrec.qty_objects;
               
               if( ( stype != M_MESH ) &&
                  ( stype != M_MAT  ) &&
                  ( stype != M_UNIT ) ) {
                  
                  for(iter = labels;iter != NULL; iter = iter->next) {
                     if(strcmp(iter->sname, working_container->subrecord.class_name)==0) {
                        break;
                     }
                  }
                  
                  if(iter == NULL) {
                     mc_print_error("No labels found to map subrecord",5000);
                  }
                  
                  rval = convert_local_moids(&global_ids, &count ,&subrec, iter->map + iter->offset_per_processor[cur_proc]);
                  working_container->total_external_contributions+=count;/*Used in next release*/
                  
                  working_container->subrecord.qty_blocks = count;
                  working_container->subrecord.mo_blocks = (int*) calloc(count*2,sizeof(int));
                  for(j=0;j<count; j++) {
                     working_container->subrecord.mo_blocks[j*2] = global_ids[j*2];
                     working_container->subrecord.mo_blocks[(j*2)+1] = global_ids[(j*2)+1];
                  }
                  /*
                  working_container->subrecord_map_for_each_proc[cur_proc] = global_ids;
                  working_container->subrec_entries_from_each_proc[cur_proc] += count;
                  */
                  if(global_ids != NULL) {
                     free(global_ids);
                     global_ids = NULL;
                  }
                  
               }else {
                  
                  working_container->subrecord.qty_blocks = subrec.qty_blocks;
                  working_container->subrecord.mo_blocks = (int*) calloc(subrec.qty_blocks*2,sizeof(int));
                  count = subrec.qty_blocks;
                  working_container->total_external_contributions=0;
                  for(j=0;j<count; j++) {
                     working_container->subrecord.mo_blocks[j*2] = subrec.mo_blocks[j*2];
                     working_container->subrecord.mo_blocks[(j*2)+1] = subrec.mo_blocks[(j*2)+1];
                  }
                  
               }
               
               working_container->subrecord.surface_variable_flag = subrec.surface_variable_flag;
               
               free(subrec.mo_blocks);
               
               subrec.mo_blocks = NULL;
            }
            
         }
      }
   }
#if TIMER
   stop = clock();
   cumalative =((double)(stop - start))/CLOCKS_PER_SEC;
   fprintf(stderr,"First processor loop Time is: %f\n",cumalative);
#endif  
#if TIMER
   start = clock();
#endif   
   if(out_srec_qty>0 && num_procs>0){
      in_labels->subrec_contributions = (short*) calloc(num_procs*out_subrec_qty, sizeof(short));
      
   }
   
   
   /*
   Need to work this in at this point.  check to see if the subrec is subset of the domain it referes to 
   and if it is we need to create a sub-mapping, otherwise we can refer to the label->mapping.
   
   
   if(out_psubrec->mo_qty != iter->size || sub_contibutions[contribute_subrec] >1) { 
                        index_out = 0;
                        * There is a subset of the select object in this subrecord *
                        if(temp_array == NULL || temp_array_size < in_psubrec->mo_qty) {
                           if(temp_array != NULL) {
                              free(temp_array);
                              temp_array= NULL;
                           }
                           temp_array = (int *)calloc(in_psubrec->mo_qty, sizeof(int));
                           temp_array_size = in_psubrec->mo_qty;
                        }* else we just use what has already been created*
                        in_index= 0;
                        
                        for(  cur_in_moid_blck =0; 
                              cur_in_moid_blck< in_psubrec->qty_id_blks; 
                              cur_in_moid_blck++) {
                           for(  in_moid = in_psubrec->mo_id_blks[cur_in_moid_blck*2];
                                 in_moid <= in_psubrec->mo_id_blks[cur_in_moid_blck*2+1];
                                 in_moid++) {
                              match_moid = map[in_moid-1]+1;
                              index_out=0;
                              for( cur_out_moid_block =0 ;
                                    cur_out_moid_block< out_psubrec->qty_id_blks; 
                                    cur_out_moid_block++) {
                                 if( out_psubrec->mo_id_blks[cur_out_moid_block*2] <= match_moid  && match_moid <= out_psubrec->mo_id_blks[cur_out_moid_block*2+1]) {
                                    temp_array[in_index++] = index_out + match_moid - out_psubrec->mo_id_blks[cur_out_moid_block*2];
                                    break;
                                 }
                                 index_out += out_psubrec->mo_id_blks[cur_out_moid_block*2+1] -out_psubrec->mo_id_blks[cur_out_moid_block*2]+1;
                                 /*
                                 for(  out_moid= out_psubrec->mo_id_blks[cur_out_moid_block*2];
                                       out_moid <= out_psubrec->mo_id_blks[cur_out_moid_block*2+1]; 
                                       out_moid++){
                                    
                                    if(match_moid == out_moid){
                                       temp_array[in_index++] = index_out;
                                       
                                       break;
                                    }
                                    index_out++;
                                    
                                 }
                              }
                           }
                           
                        }
                        map = temp_array;
            
                     
                  }
   */
   for(srec_id=0; srec_id < out_srec_qty; srec_id++) {
   
   
      if( srec_id > out_fam->qty_srecs -1 ) {
         
         rval = mc_open_srec( out_dbid, mesh_id, &out_srec_id );
         
         if( rval != OK ) {
            return rval;
         }
         
      } else {
         out_srec_id = out_fam->qty_srecs - 1;
      } 
      
      j=0;          
      
      for(working_container = subrec_container; 
         working_container != NULL; 
         working_container = working_container->next) {
         
         if( working_container->srec_id != out_srec_id){
            continue;
         }
      
         rval = mc_def_subrec_from_subrec( out_dbid, out_srec_id,&(working_container->subrecord));
         mc_print_error("mc_def_subrec: ",rval);
         /*
         if(   ( working_container->stype != M_MESH ) &&
               ( working_container->stype != M_MAT  ) &&
               ( working_container->stype != M_UNIT ) ) {
            working_container->subrecord_map = (int*) calloc(working_container->total_external_contributions,sizeof(int));
         }*/
         for(i=0;i<num_procs;i++) {
            /*  This work will be in the next working release *//*
            if(   ( working_container->stype != M_MESH ) &&
                  ( working_container->stype != M_MAT  ) &&
                  ( working_container->stype != M_UNIT ) ) {
               if(i > 0) {
                  working_container->subrecord_offsets_per_proc[i] = working_container->subrecord_offsets_per_proc[i-1] +
                                                                     working_container->subrec_entries_from_each_proc[i-1];               
               }
               if(working_container->subrec_entries_from_each_proc[i]>0) {
                  j= working_container->working_container->subrecord_offsets_per_proc[i];
                  
               }
            }
            */
            in_labels->subrec_contributions[i*out_subrec_qty+j] = working_container->in_subrec_per_proc[i];
         }
         j++;
         
      }
      rval = mc_close_srec( out_dbid, srec_id  );
      mc_print_error("mc_close_srec: ",rval);
   }
   
   working_container = subrec_container;
   subrec_container = NULL;
   
   while(working_container != NULL) {
      
      if(working_container->in_subrec_per_proc != NULL) {
         free(working_container->in_subrec_per_proc);
      }
      
      mc_cleanse_subrec(&(working_container->subrecord));
      
      last_container = working_container->next;
      
      free(working_container);
      
      working_container = last_container;
      
   }
   
   return rval;
}

int 
get_param_type( int fam_id, char *param_name)
{   
   Mili_family *fam;
   int i,name_idx,fidx,qty_entries;
   fam = fam_list[fam_id];
   name_idx=0;
    
   for( fidx=0; fidx < fam->file_count; fidx++ )
   {
      qty_entries = fam->directory[fidx].qty_entries;
 		
      for (i=0; i<qty_entries; i++)
      {
			
         if ( !strcmp(fam->directory[fidx].names[name_idx], param_name) )
         {
				return fam->directory[fidx].dir_entries[i][TYPE_IDX];
         }
         name_idx += fam->directory[fidx].dir_entries[i][STRING_QTY_IDX];
      }
   }
   return -1;
}

Return_value
combine_non_state_definitions(Mili_analysis **in_db, Mili_analysis *out_db,TILabels *labels) {
   
   int num_procs = labels->num_procs;
   Return_value status = (Return_value)OK;
   int cur_proc;
   int processed_metadata = 0;
   int param_type=-1;
   
   for(cur_proc = 0 ; cur_proc <num_procs; cur_proc++) {
      if(env.num_selected_procs>0 && !env.selected_proc_list[cur_proc]) {
         continue;
      }
      if(!processed_metadata) {
         fam_list[out_db->db_ident]->precision_limit = fam_list[in_db[cur_proc]->db_ident]->precision_limit;
         mc_copy_metadata(fam_list[out_db->db_ident],fam_list[in_db[cur_proc]->db_ident]);
         processed_metadata = 1;
      }
      combine_umesh(in_db[cur_proc],out_db);
      
      char **list=NULL;
      int count,i;
      Htable_entry *entry;
      
      count = htable_search_wildcard( fam_list[in_db[cur_proc]->db_ident]->param_table,0,
                        FALSE,"*", "NULL", "NULL",list);
      list=(char**) malloc( count*sizeof(char *));
      count = htable_search_wildcard(fam_list[in_db[cur_proc]->db_ident]->param_table,
                                     0, FALSE,"*", "NULL", 
                                     "NULL",list );
                              
      for(i = 0; i< count ;i++) {
         param_type = get_param_type(in_db[cur_proc]->db_ident,list[i]);
         status = htable_search( fam_list[out_db->db_ident]->param_table, list[i], 
                                 FIND_ENTRY, &entry );
                                 
         if((param_type == MILI_PARAM || param_type == APPLICATION_PARAM) && status == ENTRY_NOT_FOUND )
         {
           add_parameter(in_db[cur_proc]->db_ident,out_db->db_ident,list[i]);  
         }
         else if((param_type == TI_PARAM &&status == ENTRY_NOT_FOUND) && 
            strncmp( "Element Labels", list[i], strlen("Element Labels"))!=0 &&
            strncmp( "Node Labels", list[i], strlen("Node Labels"))!=0  &&
            strncmp( "GLOBAL_IDS_LOCAL_MAP", list[i], strlen("GLOBAL_IDS_LOCAL_MAP")) !=0 &&
            strncmp( "GLOBAL_IDS", list[i], strlen("GLOBAL_IDS")) !=0 &&
            strncmp( "LOCAL_COUNT", list[i], strlen("LOCAL_COUNT")) !=0) 
         {
            add_ti_parameter(in_db[cur_proc]->db_ident,out_db->db_ident,list[i]);         
         }
      }
       
      htable_delete_wildcard_list( count, list ); 
      free(list); 
            
   }
   mc_read_scalar(out_db->db_ident, "mesh dimensions" , &(labels->dimensions));
   for(cur_proc = 0 ; cur_proc < num_procs; cur_proc++) {
      if(env.num_selected_procs>0 && !env.selected_proc_list[cur_proc]) {
         continue;
      }
      status = mc_combine_classes( fam_list[in_db[cur_proc]->db_ident], fam_list[out_db->db_ident] );
      mc_print_error("mc_combine_classes:", status);
      
      status = mc_combine_svars( fam_list[in_db[cur_proc]->db_ident], fam_list[out_db->db_ident] );
      mc_print_error("mc_combine_svars:", status);
   }
#if TIMER
   clock_t start,stop;
   double cumalative=0.0;
   start = clock();
#endif  
   mc_copy_geometry(in_db, out_db, labels);
#if TIMER
   stop = clock();
   cumalative =((double)(stop - start))/CLOCKS_PER_SEC;
   fprintf(stderr,"Copy geometry Time is: %f\n",cumalative);
   start = clock();
#endif 
   status = mc_combine_srecs(in_db,out_db,labels);
#if TIMER
   stop = clock();
   cumalative =((double)(stop - start))/CLOCKS_PER_SEC;
   fprintf(stderr,"Combine srecs Time is: %f\n",cumalative);
#endif
   mc_print_error("mc_combine_srecs:",status);
   mc_flush(out_db->db_ident,NON_STATE_DATA);
   return status;
}

Return_value 
merge_state_data(int proc, TILabels *in_labels, Mili_analysis *in_db, Mili_analysis *out_db )
{
   Return_value rval = (Return_value)OK;
   
   if(in_labels == NULL) {
      return NOT_OK;
   }
   
   Label *labels = in_labels->labels;
   short * sub_contibutions ;
   Mili_family *in_fam, *out_fam;
   Famid in_dbid, out_dbid;
   Hash_table *srec_table;
   Srec *p_sr, *out_psr;
   Sub_srec *out_psubrec, *in_psubrec;
   Htable_entry *subrec_entry, *class_entry;
   Mesh_object_class_data *p_mocd;
   Svar *out_svar, *in_svar;
   size_t state_size;
   LONGLONG out_offset,
            in_offset;
   int state_qty,
       i,j,k,ii,kk,jj,ll,
       srec_id,
       subrec_qty,
       contribute_subrec,
       precision,
       mo_qty,
       qty_svars,
       qty_out_svars,
       iorder,
       stype,
       num_type,
       atom_size,
       in_round, 
       out_round,
       agg_type,
       step,
       iprec;
   char *subrec_name,
        *class_name;
   char *out_buf, 
        *in_buf,
        *inp_c, 
        *outp_c;
   Label *iter;
   int * map = NULL,
       * temp_array = NULL;
   int index_out = 0, 
       in_index=0, 
       in_moid=0,
       out_moid=0,
       cur_in_moid_blck=0, 
       cur_out_moid_block=0,
       match_moid,
       temp_array_size=0;
   out_dbid = out_db->db_ident;
   out_fam = fam_list[out_dbid];
   srec_table = out_fam->subrec_table;
   if( srec_table == NULL ) {
       return ( NOT_OK );
   }
   in_dbid = in_db->db_ident;
   in_fam = fam_list[in_dbid];
   
   /* establish the timesteps */
   if( out_db->state_times == NULL || 
       (out_fam->state_qty < env.current_state_max && 
        env.current_state_array_size < env.current_state_max)) {
       
      state_qty = env.current_state_max;
      if(out_fam->state_qty < env.current_state_max){
         free(out_db->state_times);
      }
      out_db->state_times = NEW_N( float, state_qty, "State descriptor");
      for( i = 0; i < state_qty && i < in_fam->state_qty; i++ ) {
         out_db->state_times[i] = in_fam->state_map[i].time;
      }
      env.current_state_array_size = state_qty;
   }
   srec_id = in_fam->qty_srecs - 1;
   if(srec_id <0) {
      return NOT_OK;
   }
   p_sr = in_fam->srecs[srec_id];
   out_psr = out_fam->srecs[srec_id];
 
   precision = out_fam->precision_limit;
   state_size = out_psr->size / EXT_SIZE( out_fam, M_FLOAT4 ) + 1;

   switch( precision ) {
      case PREC_LIMIT_SINGLE:
         iprec = 1;
         if( out_db->result == NULL ) {
            out_db->result = NEW_N( float, state_size, "Results" );
         }
         break;

      case PREC_LIMIT_DOUBLE:
         iprec = 2;
         if( out_db->result == NULL ) {
            out_db->result = (float *)NEW_N( double, state_size, "Results" );
         }
         break;

      default:
         rval = UNKNOWN_PRECISION;
   }
   in_offset  =  0;
   out_offset =  0;
   in_round  =  0;
   out_round =  0;
   subrec_qty = p_sr->qty_subrecs;
   sub_contibutions = in_labels->subrec_contributions+(proc*out_psr->qty_subrecs);
   for( i = 0; i < subrec_qty; i++ ) {
      in_psubrec = p_sr->subrecs[i];
      class_name = in_psubrec->mclass;
      subrec_name = in_psubrec->name;
      for(contribute_subrec = 0; contribute_subrec < out_psr->qty_subrecs; contribute_subrec++){
         if(strcmp(subrec_name, out_psr->subrecs[contribute_subrec]->name) ==0 &&
            strcmp(class_name, out_psr->subrecs[contribute_subrec]->mclass) ==0) {
            break;  
         }
      }
      if(strcmp("pseg",class_name) == 0) {
         int kjfksdfk= 0;
      }
      /*
      rval = (Return_value) htable_search( srec_table, subrec_name,
                                           FIND_ENTRY, &subrec_entry );
      */
      mo_qty = in_psubrec->mo_qty;
      map = NULL;
      if( contribute_subrec <out_psr->qty_subrecs) {
         
         out_psubrec = out_psr->subrecs[contribute_subrec];/*(Sub_srec *)subrec_entry->data;*/
         
         if( rval != ENTRY_NOT_FOUND ) {
            
            
            rval = (Return_value)
                   htable_search( in_fam->srec_meshes[srec_id]->mesh_data.umesh_data,
                                  in_psubrec->mclass, FIND_ENTRY, &class_entry );
            p_mocd = (Mesh_object_class_data *) class_entry->data;
            stype = p_mocd->superclass;
            
            switch (stype) {
               case M_UNIT:
               case M_MAT:
               case M_MESH:
                  /* Global state variables */                  
                  break;
               default:
                  for(iter = labels; iter != NULL; iter = iter->next) {
                     if(strcmp(class_name, iter->sname) == 0) {
                        break;
                     }
                  }
                  if(iter == NULL) {
                     /*no map labels what are we going to do. I vote to punt.*/
                  }else{ 
                     map= iter->map + iter->offset_per_processor[proc];
                     if(out_psubrec->mo_qty != iter->size || sub_contibutions[contribute_subrec] >1) { 
                        index_out = 0;
                        /* There is a subset of the select object in this subrecord */
                        if(temp_array == NULL || temp_array_size < in_psubrec->mo_qty) {
                           if(temp_array != NULL) {
                              free(temp_array);
                              temp_array= NULL;
                           }
                           temp_array = (int *)calloc(in_psubrec->mo_qty, sizeof(int));
                           temp_array_size = in_psubrec->mo_qty;
                        }/* else we just use what has already been created*/
                        in_index= 0;
                        
                        for(  cur_in_moid_blck =0; 
                              cur_in_moid_blck< in_psubrec->qty_id_blks; 
                              cur_in_moid_blck++) {
                           for(  in_moid = in_psubrec->mo_id_blks[cur_in_moid_blck*2];
                                 in_moid <= in_psubrec->mo_id_blks[cur_in_moid_blck*2+1];
                                 in_moid++) {
                              match_moid = map[in_moid-1]+1;
                              index_out=0;
                              for( cur_out_moid_block =0 ;
                                    cur_out_moid_block< out_psubrec->qty_id_blks; 
                                    cur_out_moid_block++) {
                                 if( out_psubrec->mo_id_blks[cur_out_moid_block*2] <= match_moid  && match_moid <= out_psubrec->mo_id_blks[cur_out_moid_block*2+1]) {
                                    temp_array[in_index++] = index_out + match_moid - out_psubrec->mo_id_blks[cur_out_moid_block*2];
                                    break;
                                 }
                                 index_out += out_psubrec->mo_id_blks[cur_out_moid_block*2+1] -out_psubrec->mo_id_blks[cur_out_moid_block*2]+1;
                                 /*
                                 for(  out_moid= out_psubrec->mo_id_blks[cur_out_moid_block*2];
                                       out_moid <= out_psubrec->mo_id_blks[cur_out_moid_block*2+1]; 
                                       out_moid++){
                                    
                                    if(match_moid == out_moid){
                                       temp_array[in_index++] = index_out;
                                       
                                       break;
                                    }
                                    index_out++;
                                    
                                 }*/
                              }
                           }
                           
                        }
                        map = temp_array;
            
                     
                  }
                  break;
               }
            }
            
            in_offset = in_psubrec->offset/ EXT_SIZE( in_fam, M_FLOAT );
            qty_svars = in_psubrec->qty_svars;
            qty_out_svars = out_psubrec->qty_svars;

            
            if( out_psubrec->organization == RESULT_ORDERED) {
               for( j = 0; j < qty_svars; j++ ) {
                  out_offset = out_psubrec->offset/ EXT_SIZE( out_fam, M_FLOAT );
                  in_svar = in_psubrec->svars[j];
                  for(k = 0; k<qty_out_svars; k++)
                  {
                     out_svar = out_psubrec->svars[k];
                     num_type = *out_svar->data_type;
                     atom_size = EXT_SIZE( in_fam, num_type );
                     agg_type = *out_svar->agg_type;
                     
                     switch( num_type) {
                        case M_INT:
                        case M_INT4:
                        case M_FLOAT:
                        case M_FLOAT4:
                           iprec = 1;
                           break;
                        case M_INT8:
                        case M_FLOAT8:
                           iprec = 2;
                           break;
                     }

                     if(strcmp(in_svar->name,out_svar->name)==0){
                        break;
                     }
                     
                     switch( agg_type ) {
                        case SCALAR:
                           out_offset += out_psubrec->mo_qty*iprec;
                           break;
                        case ARRAY:
                           step = 1;
                           for( ii = 0; ii < *out_svar->order; ii++ ) {
                              step *=  out_svar->dims[ii];
                           }
                           out_offset += step*out_psubrec->mo_qty*iprec;
                           break;
                        case VECTOR:
                           out_offset += out_psubrec->mo_qty*(*out_svar->list_size)*iprec;
                           break;
                        case VEC_ARRAY:
                           
                           out_offset += out_psubrec->mo_qty*(*out_svar->list_size)*iprec;
                           
                           break;
                     }
                  }
                  if(k==qty_out_svars)
                  {
                     continue; 
                  }
                  
                  inp_c = (char *)(in_db->result  + in_offset);

                  outp_c = (char *)(out_db->result  + out_offset);

                  in_buf = (char *)inp_c;
                  out_buf = (char *)outp_c;
                  

               
                  switch( agg_type ) {
                     case SCALAR:
                        for( k = 0; k < mo_qty; k++ ) {
                           if( ( stype == M_UNIT ) ||
                                 ( stype == M_MAT  ) ||
                                 ( stype == M_MESH ) ||
                                 ( map   == NULL   )    ) {
                              index_out = k*atom_size;
                           } else {
                              index_out = (map[k])*atom_size;
                           }

                           for( kk = 0; kk < atom_size; kk++ )
                              *(out_buf + index_out + kk ) =
                                 *(in_buf + k*atom_size + kk );
                        }
                        in_offset += in_psubrec->mo_qty*iprec;
                        out_offset += out_psubrec->mo_qty*iprec;
                        break;
                     case ARRAY:
                        step = 1;
                        for( k = 0; k < *out_svar->order; k++ ) {
                           step *=  out_svar->dims[k];
                        }
                        for( k = 0; k < mo_qty; k++ ) {
                           if( ( stype == M_UNIT ) ||
                                 ( stype == M_MAT  ) ||
                                 ( stype == M_MESH ) ||
                                 ( map   == NULL   )    ) {
                              index_out = k;
                           } else {
                              index_out = step*(map[k]);
                           }

                           for( ii = 0; ii < step; ii++ ) {
                              jj = (index_out + ii)*atom_size;
                              for( kk = 0; kk < atom_size; kk++ ) {
                                 *(out_buf + jj + kk ) = *(in_buf +ii*atom_size + kk );
                              }
                           }
                           in_buf += step*atom_size;
                        }
                        in_offset  += step*in_psubrec->mo_qty*iprec;
                        out_offset += step*out_psubrec->mo_qty*iprec;
                        break;
                     case VECTOR:
                        for( k = 0; k < mo_qty; k++ ) {
                           if( ( stype == M_UNIT ) ||
                                 ( stype == M_MAT  ) ||
                                 ( stype == M_MESH ) ||
                                 ( map   == NULL   )    ) {
                              index_out = k;
                           } else {
                              index_out = (map[k]);
                           }

                           for( ii = 0; ii < *out_svar->list_size; ii++ ) {
                              jj = (index_out*(*out_svar->list_size) + ii)*atom_size;
                              ll = (k*(*in_svar->list_size) + ii)*atom_size;
                              for( kk = 0; kk < atom_size; kk++ ) {
                                 *(out_buf + jj + kk ) = *(in_buf + ll + kk );
                              }
                           }
                        }

                        in_offset += in_psubrec->mo_qty*(*in_svar->list_size)*iprec;
                        out_offset += out_psubrec->mo_qty*(*out_svar->list_size)*iprec;
                        break;
                     case VEC_ARRAY:
                        for( k = 0; k < mo_qty; k++ ) {
                           if( ( stype == M_UNIT ) ||
                                 ( stype == M_MAT  ) ||
                                 ( stype == M_MESH ) ||
                                 ( map   == NULL   )    ) {
                              index_out = k;
                           } else {
                              index_out = map[k] ;
                           }

                           for( ii = 0; ii < *out_svar->list_size; ii++ ) {
                              jj = ( index_out*(*out_svar->list_size) + ii)*atom_size;
                              ll = ( k*(*in_svar->list_size) +ii)*atom_size;
                              for( kk = 0; kk < atom_size; kk++ ) {
                                 *(out_buf + jj +  kk ) = *(in_buf + ll +kk );
                              }
                           }
                        }
                        in_offset += in_psubrec->mo_qty*(*in_svar->list_size)*iprec;
                        out_offset += out_psubrec->mo_qty*(*out_svar->list_size)*iprec;
                        break;
                     default:
                        break;
                  }
               }
            } else {
               /* OBJECT ORIENTED */

               for( j = 0; j < qty_svars; j++ ) {
                  
                  out_offset = out_psubrec->offset/ EXT_SIZE( out_fam, M_FLOAT );
                  int object_offset = 0;
                  in_svar = in_psubrec->svars[j];
                  for(k = 0; k<qty_out_svars; k++) {
                     out_svar = out_psubrec->svars[k];
                     num_type = *out_svar->data_type;
                     atom_size = EXT_SIZE( in_fam, num_type );
                     agg_type = *out_svar->agg_type;
                     
                     switch( num_type) {
                        case M_INT:
                        case M_INT4:
                        case M_FLOAT:
                        case M_FLOAT4:
                           iprec = 1;
                           break;
                        case M_INT8:
                        case M_FLOAT8:
                           iprec = 2;
                           break;
                     }
                        
                     if(strcmp(in_svar->name,out_svar->name)==0){
                        break;
                     }
                        
                     switch( agg_type ) {
                        case SCALAR:
                           object_offset += iprec;
                           break;
                        case ARRAY:
                           step = 1;
                           for( ii = 0; ii < *out_svar->order; ii++ ) {
                              step *=  out_svar->dims[ii];
                           }
                           object_offset += step*out_psubrec->mo_qty*iprec;
                           break;
                        case VECTOR:
                           object_offset += (*out_svar->list_size)*iprec;
                           break;
                        case VEC_ARRAY:
                           for( iorder = 0; iorder < *(out_svar->order); iorder++ ) {
                              object_offset += (out_svar->dims[iorder])*
                                              *(out_svar->list_size)*out_psubrec->mo_qty*iprec;
                           }
                           break;
                     }
                  }
                  if(k==qty_out_svars)
                  {
                     continue; 
                  }

                  inp_c = (char *)(in_db->result  + in_offset) + in_round;

                  outp_c = (char *)(out_db->result  + out_offset) + out_round;

                  in_buf = (void *)inp_c;
                  out_buf = (void *)outp_c;
                     
                  switch( agg_type ) {
                     case SCALAR:
                        for( k = 0; k < mo_qty; k++ ) {
                           if( ( stype == M_UNIT ) ||
                                 ( stype == M_MAT  ) ||
                                 ( stype == M_MESH ) ||
                                 ( map   == NULL   )    ) {
                              index_out = ((*out_psubrec->lump_sizes/atom_size)*k)+object_offset ;
                           } else {
                              index_out = ((*out_psubrec->lump_atoms)*(map[k] )) +object_offset;
                           }

                           jj = index_out  * atom_size;
                           for( kk = 0; kk <  atom_size; kk++ ) {
                              *(out_buf + jj + kk ) = *(in_buf +k*atom_size + kk );
                           }
                        }
                        in_offset += in_psubrec->mo_qty*iprec;
                        break;
                     case ARRAY:
                        step = 1;
                        for( k = 0; k < *out_svar->order; k++ ) {
                           step *=  out_svar->dims[k];
                        }
                        for( k = 0; k < mo_qty; k++ ) {
                           if( ( stype == M_UNIT ) ||
                                 ( stype == M_MAT  ) ||
                                 ( stype == M_MESH ) ||
                                 ( map   == NULL   )    ) {
                                 index_out = k*mo_qty;
                           } /* IRC - February 28, 20009
                              * This may need to be
                              * modified to multiply by
                              * list size - need to create
                              * a test problem.
                              */
                           else {
                              index_out = step*(map[k]);
                           }

                           for( ii = 0; ii < step; ii++ ) {
                              jj = (index_out + ii)*atom_size;
                              for( kk = 0; kk < atom_size; kk++ ) {
                                 *(out_buf + jj + kk ) = *(in_buf +ii*atom_size + kk );
                              }
                           }
                           in_buf += step*atom_size;
                        }
                        in_offset  += step*in_psubrec->mo_qty*iprec;
                        out_offset += step*out_psubrec->mo_qty*iprec;
                        break;
                     case VECTOR:
                        index_out=0;
                        for( k = 0; k < mo_qty; k++ ) {
                           if( ( stype == M_UNIT ) ||
                                 ( stype == M_MAT  ) ||
                                 ( stype == M_MESH ) ||
                                 ( map   == NULL   )    ) {
                              index_out = k*(*out_svar->list_size)*iprec;
                           } /* IRC: February 28, 2009
                              * Added multiply by list
                              * size.
                              */
                           else {
                              index_out = (*out_psubrec->lump_atoms)*(map[k])+object_offset;
                           }

                           for( ii = 0; ii < *out_svar->list_size; ii++ ) {
                              jj = (index_out + ii)*atom_size;
                              for(kk =  0; kk <  atom_size; kk++ ) {
                                 *(out_buf + jj + kk ) = *(in_buf + ii*atom_size + kk );
                              }
                           }
                           in_buf += (*in_svar->list_size * atom_size);
                        }
                        out_offset += (*out_svar->list_size)*iprec;
                        in_offset += (*in_svar->list_size)*in_psubrec->mo_qty*iprec;
                        break;

                     case VEC_ARRAY:
                        for( iorder = 0; iorder < *in_svar->order; iorder++ ) {
                           for ( k = 0; k < mo_qty; k++ ) {
                              if( ( stype == M_UNIT ) ||
                                    ( stype == M_MAT  ) ||
                                    ( stype == M_MESH ) ||
                                    ( map   == NULL   )    ) {
                                 index_out = k*out_svar->dims[iorder]*(*out_svar->list_size);
                              }
                              /* IRC: February 28, 2009
                               * Added multiply by list
                               * size and dims.
                               */
                              else
                                 index_out = (out_svar->dims[iorder])*
                                             (*out_svar->list_size)*(map[k] );

                              for( ii = 0; ii < (in_svar->dims[iorder])*(*in_svar->list_size); ii++ ) {
                                 jj = (index_out + ii)*atom_size;
                                 for( kk = 0; kk < atom_size; kk++ ) {
                                    *(out_buf +jj + kk ) = *(in_buf + ii*atom_size + kk );
                                 }
                              }
                              in_buf += in_psubrec->lump_atoms[iorder]*atom_size;
                           }
                           out_offset += (out_svar->dims[iorder])*
                                         (*out_svar->list_size)*out_psubrec->mo_qty*iprec;
                           in_offset  += (in_svar->dims[iorder])*
                                         (*in_svar->list_size)*in_psubrec->mo_qty*iprec;
                        }
                        break;
                     default:
                        break;
                  }
               }
            }
               
         }
            
      }
      map = NULL;
      if(temp_array != NULL){
         free(temp_array);
         temp_array = NULL;
      } 

   }
      
   
   return rval;
}






void
l2gnums(int proc, int *offsets, int *inmap, int count, int *loc_list, int *gbl_list)
{
   int offset;
   int i, j;

   offset = 0;
   for( i = 0; i < proc; i++ ) {
      offset += offsets[i];
   }

   for( j = 0; j < count; j++ ) {
      gbl_list[j] = inmap[ offset + loc_list[j] -1 ];
   }
   return;
}


/* End of combine_db.c */
