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
 */
 
 /**
 File generator formated files for VisIt use.
 */
 
#include "mili.h"
#include "mili_internal.h"
#include "mili_enum.h"
#include "misc.h"
#include "parson.h"
#include <math.h>
#include <stdio.h>
#include <errno.h>



 
#define OUT_VERSION 2
// From avtTypes.h
#define     AVT_SCALAR_VAR      1
#define     AVT_VECTOR_VAR      2
#define     AVT_UNKNOWN_TYPE    7
#define     AVT_NODECENT        0
#define     AVT_ZONECENT        1
 //These are the families that are open at the moment. 
extern Mili_family **fam_list;
 
 //Bring in the current family quantity count 
extern int fam_qty;

typedef struct
{
   State_variable *state_var;   // The Mili defintion of this variable
   int subrec_count;            // The number of subrecords containing this variable
   int *subrec_ids;             // The actual subrecords that this contained in.
   char **subrec_name;          // The associated subrecord names.
   int center;
   int vtk_type;
   int realNameCount;
   char *realNames[10];
   char *realLongName;
} Variable;

typedef struct
{
   Hash_table *variables;   // Variables associated with this mili class
   char name[128];          // This is the short name of this class(i.e. brick)
   char longName[256];      // Long name.  We want this be what is seen in the menu.
   int superClass;          // This is our "inherited" class type (i.e. M_HEX)
   int elementCount;
}Mili_Class;

typedef struct
{
  int materialId;            // Material that this element set belongs to.
  int *integrationPoints;    // The actual integration point written out to database
  int totalIntegrationPoints;// Total integration point in the simulation
  int count;
  char name[100];                 // Number of integration point written to the database.
} ElementSet;

ElementSet *elementSets;
int elementSetCount;
int numProcessors;
int xmilicsFile = 0;
int subrecord_count = 0;
char *subrec_string = NULL;
int
mc_activate_visit_file(Famid database_id, int on)
{
    Mili_family* fam;  //This is the Mili database plot file
    if(database_id >= fam_qty || database_id <0)
    {
       return BAD_FAMILY;
    }
    
    fam = fam_list[database_id];
    
    fam->visit_file_on = on;
    
    return OK;
}

/**
*   TAG (writeVariable_json)
*   Write a single variable to the json structure.
*
*   @param char* var_elem   Variable name 
*   @param Varaible* variable Variable struct that contain information.
*   @param JSON_Object* Pointer the element in the Json structure that we \
*                       are adding the variable to.  
*/
static void
writeVariable_json(char* var_elem, Variable *variable,JSON_Object *root_object)
{
   char variable_string[2048];
   char components[1024];
   char number[10];
   char *numbers = NULL;
   int i;
   State_variable *sv;
   variable_string[0] = '\0';
   components[0] = '\0';
   number[0] = '\0';
   
   sv=variable->state_var;
   if(var_elem == NULL || sv == NULL)
   {
      return;
   }
   
   sprintf(variable_string,"%s.%s",var_elem,"LongName");
   if(variable->realLongName)
   {
      
      json_object_dotset_string(root_object,variable_string,variable->realLongName);
   }else
   {
      json_object_dotset_string(root_object,variable_string,sv->long_name);
   }
   
   sprintf(variable_string,"%s.%s",var_elem,"num_type");
   json_object_dotset_number(root_object,variable_string,sv->num_type);
   sprintf(variable_string,"%s.%s",var_elem,"agg_type");
   json_object_dotset_number(root_object,variable_string,sv->agg_type);
   sprintf(variable_string,"%s.%s",var_elem,"vector_size");
   json_object_dotset_number(root_object,variable_string,sv->vec_size);
   sprintf(variable_string,"%s.%s",var_elem,"dims");
   if(sv->dims)
   {
      json_object_dotset_number(root_object,variable_string,sv->dims[0]);
   }else
   {
      json_object_dotset_number(root_object,variable_string,0);
   }
   if(variable->realNameCount>0)
   {
      components[0] = '\0';
      sprintf(variable_string,"%s.%s",var_elem,"real_names");
      sprintf(components,"%s","[");
      for(i=0; i<variable->realNameCount;i++)
      {
         sprintf(components,"%s\"%s\"",components,variable->realNames[i]);
         if(i < variable->realNameCount-1)
         {
            strcat(components,",");
         }
      }
      strcat(components,"]");
      json_object_dotset_value(root_object,variable_string,
                                  json_parse_string(components));
   }
   
   components[0]='\0';
   sprintf(variable_string,"%s.%s",var_elem,"rank");
   json_object_dotset_number(root_object,variable_string,sv->rank);
   sprintf(variable_string,"%s.%s",var_elem,"Center");
   json_object_dotset_number(root_object,variable_string,variable->center);
   sprintf(variable_string,"%s.%s",var_elem,"VTK_TYPE");
   json_object_dotset_number(root_object,variable_string,variable->vtk_type);
   switch (sv->agg_type){
      case VECTOR:
      case VEC_ARRAY:
         sprintf(variable_string,"%s.%s",var_elem,"vector_components");
         sprintf(components,"%s","[");
         for( i=0; i<sv->vec_size; i++)
		   {
            strcat(components,"\"");
            strcat(components,sv->components[i]);
            strcat(components,"\"");
               
            if(i < sv->vec_size-1)
            {
               strcat(components,",");
            }
               
         }
         strcat(components,"]");
         json_object_dotset_value(root_object,variable_string,
                                  json_parse_string(components));
         break;
         
      case ARRAY:
               
         numbers = NEW_N(char , sv->rank*10,"Numbers");
         numbers[0]='\0';
         sprintf(variable_string,"%s.%s",var_elem,"dimensions");
         
         strcat(numbers,"[");
         for( i=0; i<sv->rank;i++)
         {
            sprintf(number,"%d",sv->dims[i]);
            strcat(numbers,number);
            if(i < sv->rank-1)
            {
               strcat(numbers,",");
            }
         }
         strcat(numbers,"]");
         json_object_dotset_value(root_object,variable_string,
                            json_parse_string(numbers));
         free (numbers);
         break;
      default:
         break;
   }
   
   sprintf(variable_string,"%s.%s",var_elem,"subrecords");
   //fprintf(stderr, "%d %d\n\n", variable->subrec_count,((int)log10( variable->subrec_count+1))+1);
   if(!subrec_string  && subrecord_count >0)
   {
      int sub_size = subrecord_count*(((int)log10( variable->subrec_count+1))+4);
      subrec_string = NEW_N(char ,sub_size,"subrecs in writeVariable_json" );
   }   
   if(variable->subrec_count > 0)
   {
      subrec_string[0] = '\0';
      for(i=0;i<variable->subrec_count;i++)
      {
         if(i==0)
         {
            strcat(subrec_string, "[");
         }
      
      
         if(i < variable->subrec_count-1)
         {
            sprintf(number ,"%d,", variable->subrec_ids[i]);
         }else
         {
            sprintf(number ,"%d", variable->subrec_ids[i]);
         }
         strcat(subrec_string,number);
      }
      strcat(subrec_string, "]");
   }
   
   json_object_dotset_value(root_object,variable_string,
                            json_parse_string(subrec_string));
    
}
/**
 *  This function goes adds the variables to the hashtable for additional processing later.
 *  If this variable is a vector or vector array it will add the component to the variable 
 *  hashtable also.
 *
 *  @param Famid database_id  Identifier for the specified database.
 *  @param Hash_table* variable_ht   Hashtable to hold the variables definitions. The key for each is the variable name
 *  @param Variable* variable 
 *  @param char* class_name The class name this variable is within for this definition.
 */

static void
add_variables(Famid database_id, Hash_table *variable_ht, 
               Variable *variable, char *class_name)
{
      State_variable *sv;
      int i;
      char variable_string[1024];
      char components[512];
      Htable_entry *entry;
      Variable *component_sv;
      Return_value rval = OK;
      sv=variable->state_var;
      
      components[0] = '\0';
      variable_string[0] = '\0';
      
      strcat(variable_string,"[");
      strcat(variable_string,class_name);
      strcat(variable_string,"]");
      strcat(variable_string,sv->short_name);
      
      if(htable_search( variable_ht, variable_string , FIND_ENTRY, &entry )==OK)
      {
         
         mc_cleanse_st_variable(variable->state_var);
      }
      else
      {
        rval = htable_search( variable_ht, variable_string , ENTER_UNIQUE, &entry );
                          
        if(rval == OK)
        {
          entry->data = (void*)variable;
          sv=variable->state_var;
          switch (sv->agg_type)
          {
            case VECTOR:
            case VEC_ARRAY:
         
               for( i=0; i<sv->vec_size; i++)
		         {
               
                  strcat(components,sv->components[i]);
                  
                  component_sv = NEW(Variable,"Variable");
                  component_sv->state_var = NEW(State_variable, "State_variable");
                  rval = mc_get_svar_def(database_id, sv->components[i], component_sv->state_var);
                  
                  add_variables(database_id, variable_ht, component_sv, 
                                class_name);
               
               }
               
               break;
            default:
               break;
          }
         
        }
      }
}


/**
*   TAG(process_state_variables)
*   Get the state variables,  We only want to get a single definition so we do not process 
*   The variables listed in a Vector or Vector Array.
*   
*   Famid database_id  Identifier for the database
*   Hash_table *classTable  Table containing class information
*/
static Hash_table *
process_state_variables(Famid database_id,Hash_table *classTable )
{
   int i,j,k,l;
   int srec_qty =0;
   Return_value rval = OK;
   Htable_entry *entry;
   Hash_table *variable_ht = htable_create(5009);
   Mili_Class *mili_class;
   Variable *component_var;
   char components[512];
   rval = mc_query_family(database_id, QTY_SREC_FMTS, NULL, NULL,
                          (void*) &srec_qty);
   
   //If there is a state record format
   if(rval == OK)
   {
     //Process each state record format.  Usually just 1...
     for(i=0;i < srec_qty ; i++)
     {
       //Get the subrecord count
       rval = mc_query_family(database_id, QTY_SUBRECS, (void *) &i, NULL,
                              (void *) &subrecord_count);
         
       // Succesful?  good process the subrecords
       // We want to process variables based on subrecords
       // thus eliminating extra variables declared by the
       // various codes.
       if(rval == OK)
       {
         if(!subrec_string)
         {
           int sub_size = subrecord_count*(((int)log10( subrecord_count+1))+4);
           subrec_string = NEW_N(char ,sub_size,
                                 "subrec_string in process_state_variables" );
         }
         Subrecord sr;
         for (j = 0 ; j < subrecord_count ; j++)
         {
           
               
           rval = mc_get_subrec_def(database_id, i, j, &sr);
           if(rval==OK)
           {
             Variable *var;
             for(k=0;k<sr.qty_svars;k++)
             {
               State_variable temp; 
               rval = mc_get_svar_def(database_id, sr.svar_names[k], 
                                      &temp);
               if(htable_search( variable_ht, temp.short_name , FIND_ENTRY, &entry )==OK)
               {
                  var = (Variable*) entry->data;
                  //Lets run over what we have already to put out the subrecord 
                  //id only once.
                  if(var->subrec_ids[var->subrec_count-1]!=j)
                  {
                    var->subrec_ids[var->subrec_count]=j;
                    var->subrec_name[var->subrec_count] = (char*)calloc(sizeof(char), 64);
                    strcpy(var->subrec_name[var->subrec_count], sr.name);
                    var->subrec_count++;
                  }
                  
                  if(htable_search( classTable, sr.class_name , FIND_ENTRY, &entry )==OK)
                  {
                    mili_class = (Mili_Class*) entry->data;
                    entry = NULL;
                    if(htable_search( mili_class->variables, var->state_var->short_name , 
                                      FIND_ENTRY, &entry )!=OK)
                    {
                        rval = htable_search( mili_class->variables, var->state_var->short_name , 
                                              ENTER_UNIQUE, &entry );
                        entry->data = (void*) var;
                    }
                  }
                  
                  mc_cleanse_st_variable(&temp);
                  
                  continue;
               }
               mc_cleanse_st_variable(&temp);
               var = NEW(Variable,"Variable");
               var->subrec_ids = (int*)calloc((sizeof(int)), subrecord_count );
               var->subrec_ids[0] = j;
               var->subrec_name = (char**)calloc(sizeof(char*),subrecord_count);
               var->subrec_name[0] = (char*)calloc(sizeof(char), 64);
               strcpy(var->subrec_name[0], sr.name);
               var->subrec_count = 1;
               var->realNames[0]=NULL;
               var->realNameCount = 0;
               
               if(sr.superclass == M_NODE)
               {
                  var->center = AVT_NODECENT;
               }else
               {
                  var->center = AVT_ZONECENT;
               }
               
               if (strcmp(sr.class_name, "glob") == 0 || 
                   strcmp(sr.class_name, "mat") == 0)
               { 
                  var->center = AVT_ZONECENT; 
               }
               var->state_var  = NEW(State_variable,"State Variable");
               
               rval = mc_get_svar_def(database_id, sr.svar_names[k], 
                                      var->state_var);
               if(rval != OK)
               {
                  continue;
               }
               if (var->state_var->agg_type == SCALAR)
               { 
                  var->vtk_type = (AVT_SCALAR_VAR); 
               }
               else if (var->state_var->agg_type == VECTOR)
               { 
                  var->vtk_type = (AVT_VECTOR_VAR); 
               }
               else
               { 
                  var->vtk_type = (AVT_UNKNOWN_TYPE); 
               }
               rval = htable_search( variable_ht, var->state_var->short_name , ENTER_UNIQUE, &entry );
               entry->data = (void*) var;
               
               entry = NULL;
               
               if(htable_search( classTable, sr.class_name , FIND_ENTRY, &entry )==OK)
               {
                  mili_class = (Mili_Class*) entry->data;
                  entry = NULL;
                  if(htable_search( mili_class->variables, var->state_var->short_name , FIND_ENTRY, &entry )!=OK)
                  {
                     rval = htable_search( mili_class->variables, var->state_var->short_name , ENTER_UNIQUE, &entry );
                     entry->data = (void*) var;
                  }
               }
               
               int agg_type = var->state_var->agg_type;
               
               switch (agg_type)
               {
                   case VECTOR:
                   case VEC_ARRAY:

                       var->realNames[0] = mc_determine_naming( var->state_var->short_name,var->state_var);
                       
                       if(var->realNames[0])
                       {
                          var->realNameCount++;
                          if(strcmp(var->realNames[0],"stress")==0)
                          {
                             var->realLongName = calloc(1, 7);
                             strcpy(var->realLongName,"Stress");
                          }else if(strcmp(var->realNames[0],"strain")==0)
                          {
                             var->realLongName = calloc(1, 7);
                             strcpy(var->realLongName,"Strain");
                          }else
                          {
                             var->realLongName = NULL;
                          }
                       }
                       
                       for( l=0; l<var->state_var->vec_size; l++)
		                 {
               
                           strcpy(components,var->state_var->components[l]);
                  
                           rval = mc_get_svar_def(database_id, components, &temp);
                           
                           if(htable_search( variable_ht, temp.short_name , FIND_ENTRY, &entry )==OK)
                           {
                              component_var = (Variable*) entry->data;
                              if(component_var->subrec_ids[component_var->subrec_count-1]!=j)
                              {
                                component_var->subrec_ids[component_var->subrec_count]=j;
                                component_var->subrec_name[component_var->subrec_count] = (char*)calloc(sizeof(char), 64);
                                strcpy(component_var->subrec_name[component_var->subrec_count], sr.name);
                                component_var->subrec_count++;
                                if(var->realNames[0])
                                {
                                   if(strcmp(var->realNames[0],"stress")==0)
                                   {
                                      if(!(strcmp(temp.short_name,"sx") == 0 ||
                                         strcmp(temp.short_name,"sy") == 0 ||
                                         strcmp(temp.short_name,"sz") == 0 ||
                                         strcmp(temp.short_name,"sxy") == 0 ||
                                         strcmp(temp.short_name,"syz") == 0 ||
                                         strcmp(temp.short_name,"szx") == 0 ))
                                      {
                                         var->realNames[var->realNameCount] = calloc(1,strlen(temp.short_name)+1);
                                         strcpy(var->realNames[var->realNameCount],temp.short_name);
                                         var->realNameCount++;
                                      }
                                   }
                                   if(strcmp(var->realNames[0],"strain")==0)
                                   {
                                      if(!(strcmp(temp.short_name,"ex") == 0 ||
                                         strcmp(temp.short_name,"ey") == 0 ||
                                         strcmp(temp.short_name,"ez") == 0 ||
                                         strcmp(temp.short_name,"exy") == 0 ||
                                         strcmp(temp.short_name,"eyz") == 0 ||
                                         strcmp(temp.short_name,"ezx") == 0 ))
                                      {
                                         var->realNames[var->realNameCount] = calloc(1,strlen(temp.short_name)+1);
                                         strcpy(var->realNames[var->realNameCount],temp.short_name);
                                         var->realNameCount++;
                                      }
                                   }
                                }
                                mc_cleanse_st_variable(&temp);
                              }
                              
                              continue;
                           }
                           
                           mc_cleanse_st_variable(&temp);
                           component_var = NEW(Variable,"Variable");
                           component_var->subrec_ids = (int*)calloc((sizeof(int)), subrecord_count );
                           component_var->subrec_ids[0] = j;
                           component_var->subrec_name = (char**)calloc(sizeof(char*),subrecord_count);
                           component_var->subrec_name[0] = (char*)calloc(sizeof(char), 64);
                           strcpy(component_var->subrec_name[0], sr.name);
                           component_var->subrec_count=1;
                           component_var->state_var  = NEW(State_variable,"State Variable");
                           if(sr.superclass == M_NODE)
                           {
                              component_var->center = AVT_NODECENT;
                           }else
                           {
                              component_var->center = AVT_ZONECENT;
                           }
               
                           if (strcmp(sr.class_name, "glob") == 0 || 
                               strcmp(sr.class_name, "mat") == 0)
                           { 
                              component_var->center = AVT_ZONECENT; 
                           }
                           
               
                           rval = mc_get_svar_def(database_id, components, 
                                                 component_var->state_var);
                           if (component_var->state_var->agg_type == SCALAR)
                           { 
                              component_var->vtk_type = (AVT_SCALAR_VAR); 
                           }
                           else if (component_var->state_var->agg_type == VECTOR)
                           { 
                              component_var->vtk_type = (AVT_VECTOR_VAR); 
                           }
                           else
                           { 
                              component_var->vtk_type = (AVT_UNKNOWN_TYPE); 
                           }
                           entry = NULL;
                           rval = htable_search( variable_ht, components , ENTER_UNIQUE, &entry );
                           entry->data = (void*) component_var;
                           if(var->realNames[0])
                           {
                               if(strcmp(var->realNames[0],"stress")==0)
                               {
                                   if(!(strcmp(component_var->state_var->short_name,"sx") == 0 ||
                                      strcmp(component_var->state_var->short_name,"sy") == 0 ||
                                      strcmp(component_var->state_var->short_name,"sz") == 0 ||
                                      strcmp(component_var->state_var->short_name,"sxy") == 0 ||
                                      strcmp(component_var->state_var->short_name,"syz") == 0 ||
                                      strcmp(component_var->state_var->short_name,"szx") == 0 ))
                                   {
                                      var->realNames[var->realNameCount] = 
                                         calloc(1,strlen(component_var->state_var->short_name)+1);
                                      strcpy(var->realNames[var->realNameCount],
                                             component_var->state_var->short_name);
                                      var->realNameCount++;
                                   }
                                }
                                if(strcmp(var->realNames[0],"strain")==0)
                                {
                                   if(!(strcmp(component_var->state_var->short_name,"ex") == 0 ||
                                      strcmp(component_var->state_var->short_name,"ey") == 0 ||
                                      strcmp(component_var->state_var->short_name,"ez") == 0 ||
                                      strcmp(component_var->state_var->short_name,"exy") == 0 ||
                                      strcmp(component_var->state_var->short_name,"eyz") == 0 ||
                                      strcmp(component_var->state_var->short_name,"ezx") == 0 ))
                                   {
                                      var->realNames[var->realNameCount] = 
                                         calloc(1,strlen(component_var->state_var->short_name)+1);
                                      strcpy(var->realNames[var->realNameCount],
                                             component_var->state_var->short_name);
                                      var->realNameCount++;
                                   }
                                }
                             
                           }
               
                       }
               
                       break;
                    default:
                       break;
               }
               
                      
             }
           }
           mc_cleanse_subrec(&sr);
         }
       }
     }
   }
   return variable_ht;
}




static void 
cleanVariable(Variable *variable)
{
   /*
   typedef struct
   {
      State_variable *state_var;   // The Mili defintion of this variable
      int subrec_count;            // The number of subrecords containing this variable
      int *subrec_ids;             // The actual subrecords that this contained in.
      char **subrec_name;          // The associated subrecord names.
   } Variable;
   */
   int i;
   mc_cleanse_st_variable(variable->state_var);
   free(variable->state_var);
   free(variable->subrec_ids );
   for(i=0;i<variable->subrec_count;i++)
   {
      free(variable->subrec_name[i]);
   }
   free(variable->subrec_name);
   
   
}


static void
cleanVariableTable(Hash_table *table)
{
   int current_index = 0;
   Htable_entry *current_entry;
   
   for(current_index=0; current_index < table->size; current_index++)
   {
      current_entry = table->table[current_index];
      
      if(current_entry == NULL)
      {
         continue;
      }
      
      while(current_entry != NULL)
      {
         cleanVariable((Variable*)current_entry->data);
         free((Variable*)current_entry->data);
         current_entry = current_entry->next;
      }
   }
   htable_delete(table,NULL,0);
}
/**
*    TAG(gather_parameters)
*    This function gathers any additional paramters that are defined.  
*    This will include the Element sets
*  
*    Famid database_id Identifier for the database
*/
static int 
gather_parameters(Famid database_id )
{
   char **parameters_next;
   char title[36];
   int i,j,parameter_count = 0;
   Mili_family* fam;  //This is the Mili database plot file
   Htable_entry *entry;
   int status;
   int* es_ids;
   int items_read = 0;
   elementSetCount = 0;
   
   if(database_id >= fam_qty || database_id <0)
   {
      return BAD_FAMILY;
   }
    
   fam = fam_list[database_id];
   
   parameter_count = mc_get_app_parameter_count(database_id);
   
   parameters_next = NEW_N(char*, parameter_count, "Parameter_list");
   
   mc_get_app_parameter_names(database_id, parameters_next, parameter_count);
   
   for(i=0;i<parameter_count;i++)
   {
      if(strstr(parameters_next[i],"_es_"))
      {
         elementSetCount++;
        
      }else if(strncmp(parameters_next[i],"MAT_NAME_",9) )
      {
         
         if(!strncmp(parameters_next[i], "part_title", 10))
         {
            mc_ti_read_string(database_id, parameters_next[i], title);
         }
         status = htable_search( fam->param_table, parameters_next[i], 
                               FIND_ENTRY, &entry );
         
         free(parameters_next[i]);
         parameters_next[i] = NULL;
         
      }
   }
   int currentElementSet = 0;
   int mat_id;
   if(elementSetCount>0)
   {
      elementSets = NEW_N(ElementSet,elementSetCount,"");
      for(i=0;i<parameter_count;i++)
      {
         if(parameters_next[i]!=NULL && strstr(parameters_next[i],"_es_"))
         {
            strcpy(elementSets[currentElementSet].name,parameters_next[i]);
            mat_id = atoi(parameters_next[i]+12);
            elementSets[currentElementSet].materialId = mat_id;
            
            status = mc_ti_read_array(database_id,parameters_next[i],
                                     (void**)&es_ids, &items_read);
            elementSets[currentElementSet].integrationPoints = 
               NEW_N(int, items_read,"integrationPoints");
            
            elementSets[currentElementSet].count = items_read-1;
            
            for(j=0;j<items_read - 1;j++)
            {
               elementSets[currentElementSet].integrationPoints[j] = es_ids[j];
            }
            
            elementSets[currentElementSet].totalIntegrationPoints = es_ids[j];
            free(es_ids);
            currentElementSet++;
         }
      }
   }
   for(i=0;i<parameter_count;i++)
   {
      if(parameters_next[i]!=NULL)
      {
         free(parameters_next[i]);
      }
   }
   free (parameters_next);
   
   return parameter_count;
   
}

/**
*    TAG(write_state_variables_json)
*
*    Hash_table *variable_ht    Hash table holding Variable structs
*    JSON_Object *root_object   JSON element to write information
*/
static void
write_state_variables_json(Hash_table *variable_ht, JSON_Object *root_object)
{
   int cur_table_entry;
   Htable_entry * entry;
   Variable *sv;
   int header_written = 0;
   char var_elem[64];
   
   for(cur_table_entry=0; 
       cur_table_entry<variable_ht->size;
       cur_table_entry++)
   {
      
      if(variable_ht->table[cur_table_entry] == NULL)
      {
         continue;
      }
      if(!header_written)
      {
         json_object_dotset_number(root_object, "Variables.count",variable_ht->qty_entries);
         header_written= 1;
      }
      entry = variable_ht->table[cur_table_entry];
      
      sv = (Variable*) entry->data;
      sprintf(var_elem,"Variables.%s",entry->key);
      writeVariable_json(var_elem, sv,root_object);
      while(entry->next != NULL)
      {
         entry = entry->next;
         sprintf(var_elem,"Variables.%s",entry->key);
         sv = (Variable*) entry->data;
         writeVariable_json(var_elem, sv,root_object);
      }
   }
   
}
/**
*    TAG(countClasses)
*    Get the number of classes in the database.
*
*    Famid database_id Identifier for the database to open
*/
static int 
countClasses(Famid database_id)
{
   int qty_classes = 0;
   int current_class_count = 0;
   int mesh_id;
   int SUPERCLASS;
   int int_args[2];
   int status;
   
   Mili_family* fam;  //This is the Mili database plot file
   
   if(database_id >= fam_qty || database_id <0)
   {
      return BAD_FAMILY;
   }
    
   fam = fam_list[database_id];
   
   for(mesh_id=0; mesh_id<fam->qty_meshes; mesh_id++)
   {
      for(SUPERCLASS=0;SUPERCLASS<M_QTY_SUPERCLASS;SUPERCLASS++)
      {
         int_args[0] = mesh_id;
         int_args[1] = SUPERCLASS;
   
         status = mc_query_family( database_id, QTY_CLASS_IN_SCLASS, 
                                   (void *) int_args, NULL,
                                   (void *) &current_class_count );
         if(status != OK)
         {
            return -1;
         }
         qty_classes += current_class_count;
      }
   }
   return qty_classes;
} 

/**
*    TAG(readClasses)
*
*    @param Famid database_id database id for this processor
*    @param Hash_table *classes Hash table to store class information
*    @param Mili_Class *classes_array  Array of mili class structs
*/
static int 
readClasses(Famid database_id, Hash_table *classes, Mili_Class *classes_array )
{
   int classes_count = 0;
   int mesh_id;
   int SUPERCLASS;
   Htable_entry *entry = NULL;
   int i;
   int status = OK;
   int stop_ident = 0, start_ident = 0;
   int int_args[2];
   char short_name[255];
	char long_name[255];
   int qty_classes;
   
   
   Mili_family* fam;  //This is the Mili database plot file
   
   if(database_id >= fam_qty || database_id <0)
   {
      return BAD_FAMILY;
   }
    
   fam = fam_list[database_id];
   
   for(mesh_id=0; mesh_id<fam->qty_meshes; mesh_id++)
   {
      for(SUPERCLASS=0;SUPERCLASS<M_QTY_SUPERCLASS;SUPERCLASS++)
      {
         int_args[0] = mesh_id;
         int_args[1] = SUPERCLASS;
   
         status = mc_query_family( database_id, QTY_CLASS_IN_SCLASS, 
                                   (void *) int_args, NULL,
                                   (void *) &qty_classes );
         if(status != OK)
         {
            return status;
         }                     
         for (i=0; i<qty_classes; i++)
	      {
	         status = mc_get_simple_class_info( database_id, mesh_id, 
                                               SUPERCLASS, i, short_name, 
                                               long_name, &start_ident, 
                                               &stop_ident );
                                            
            if(status != OK)
            {
               classes_array[classes_count].variables = NULL;
               classes_count++;
               continue;
            }
            

            classes_array[classes_count].variables = htable_create(1009);
            
            classes_array[classes_count].name[0] ='\0';
            strcpy(classes_array[classes_count].name,short_name);
            
            classes_array[classes_count].longName[0] = '\0';
            strcpy(classes_array[classes_count].longName, long_name);
            
            classes_array[classes_count].superClass = SUPERCLASS;
            
            if(strcmp(short_name,"mat")==0 || strcmp(short_name,"glob")==0)
            {
               classes_array[classes_count].elementCount = 0;
            }else
            {
               classes_array[classes_count].elementCount = stop_ident -start_ident +1;
            }
            
            status = htable_search( classes, short_name , ENTER_UNIQUE, &entry );
            
            if(status != OK)
            {
               return status;
            }
            
            entry->data = (void*)&classes_array[classes_count];
            classes_count++;
	      }
      }
   }
   return OK;
}

/**
*  TAG (writeClasses_json)
*  Function to write out the class info.
*  @param Hash_table classTable  Hash Table containing the Mili Class structs
*  
*  @param JSON_Object *root_object  pointer to the root json object
*/
static void
writeClasses_json(Hash_table *classTable,Mili_Class *miliClasses, JSON_Object *root_object)
{
   int qty_classes = 0;
   int qty_written = 0;
   char class_elem[64];
   char class_elem_variable[72];
   char class_elem_variables[2048];
   char class_elem_count[100];
   int i,j;
   Hash_table *variables;
   Htable_entry *next;
   qty_classes = classTable->qty_entries;
   json_object_dotset_number(root_object, "Classes.count",qty_classes);
               
   for(i=0;i<qty_classes;i++)
   {
      sprintf(class_elem, "Classes.%s",miliClasses[i].name);
      sprintf(class_elem_variable,"%s.%s",class_elem,"LongName");
      json_object_dotset_string(root_object,class_elem_variable,miliClasses[i].longName);
      sprintf(class_elem_count,"%s.%s",class_elem,"ElementCount");
      json_object_dotset_number(root_object,class_elem_count,miliClasses[i].elementCount);
      
      sprintf(class_elem_count,"%s.%s",class_elem,"SuperClass");
      json_object_dotset_number(root_object,class_elem_count,miliClasses[i].superClass);
      variables = miliClasses[i].variables;
      qty_written=0;
      
      if(variables->qty_entries >0)
      {
         sprintf(class_elem_variables,"["); 
         
         for(j=0;j<variables->size;j++)
         {
            if(variables->table[j] == NULL)
            {
               continue;
            }
            strcat(class_elem_variables,"\"");
            strcat(class_elem_variables,variables->table[j]->key);
            strcat(class_elem_variables,"\"");
            if(qty_written < variables->qty_entries-1)
            {
               strcat(class_elem_variables,",");
            }
            
            qty_written++;
            
            next = variables->table[j]->next;
            while(next != NULL)
            {
               strcat(class_elem_variables,"\"");
               strcat(class_elem_variables,next->key);
               strcat(class_elem_variables,"\"");
               if(qty_written < variables->qty_entries-1)
               { 
                  strcat(class_elem_variables,",");
               }
               qty_written++;
               next = next->next;
            }
            
         }
          
         strcat(class_elem_variables,"]");
         sprintf(class_elem_variable,"%s.%s",class_elem,"variables");
         
         json_object_dotset_value(root_object,class_elem_variable,
                                  json_parse_string(class_elem_variables));
      }
   }
   
}

/**
*    TAG(writeElementSets_json)
*    Write the global element set to the JSON file.
*
*    @param JSON_Object *root_object JSON element to write the Element Sets to.
*/

static void
writeElementSets_json(JSON_Object *root_object)
{
   int i,j; //Just counters
   char pathway[100];
   char var_pathway[110];
   char integration_points[1028];
   if(elementSetCount<=0)
   {
      return;
   }
   json_object_dotset_number(root_object, "Element_Sets.count",elementSetCount);
   
   for(i=0;i<elementSetCount;i++)
   {
      pathway[0]='\0';
      sprintf(pathway, "Element_Sets.%s",elementSets[i].name);
      sprintf(var_pathway, "%s.material_id",pathway);
      json_object_dotset_number(root_object,var_pathway,elementSets[i].materialId);
      
      sprintf(var_pathway, "%s.TotalIntegrationPoints",pathway);
      json_object_dotset_number(root_object,var_pathway,elementSets[i].totalIntegrationPoints);
      
      sprintf(var_pathway, "%s.OutputCountIntegrationPoints",pathway);
      json_object_dotset_number(root_object,var_pathway,elementSets[i].count);
      
      integration_points[0] = '\0';
      sprintf(integration_points,"[%d",elementSets[i].integrationPoints[0]);
      for(j=1;j<elementSets[i].count;j++)
      {
         sprintf(integration_points,"%s,%d",integration_points,elementSets[i].integrationPoints[j]);
      }
      sprintf(integration_points,"%s]",integration_points);
      sprintf(var_pathway, "%s.IntegrationPoints",pathway);
      json_object_dotset_value(root_object,var_pathway,json_parse_string(integration_points));
      
   } 
}

/**
*  TAG (writeMaterials_json)
*  Function to write out the materials retriveing the names if they exist.
*  @param Famid database_id  database id for this processor
*  @param JSON_Object* root_object  pointer to the Json root object
*/
static int
writeMaterials_json(Famid database_id, JSON_Object *root_object)
{
    int qty_classes = 0;
    int i;
    int start_ident;
    int stop_ident;
    int highestMat =0;
    char short_name[255];
    char long_name[255];
    int int_args[2];
    int mesh_id;
    int cur_material;
    char base_name[20];
    char result_name[100];
    char color_string[100];
    int items_read;
    int status,status2;
    float *colors;
   
   Mili_family* fam;  //This is the Mili database plot file
    
    
   if(database_id >= fam_qty || database_id <0)
   {
      return BAD_FAMILY;
   }
    
   fam = fam_list[database_id];
   
   
   for(mesh_id=0; mesh_id<fam->qty_meshes; mesh_id++)
   {
      int_args[0] = mesh_id;
      int_args[1] = M_MAT;
   
      status = mc_query_family( database_id, QTY_CLASS_IN_SCLASS, (void *) int_args, 
                              NULL, (void *) &qty_classes );
      if(status != OK)
      {
         return status;
      }                     
      for (i=0; i<qty_classes; i++)
	   {
	      status = mc_get_simple_class_info( database_id, mesh_id, M_MAT, i, short_name, 
                                          long_name, &start_ident, 
                                          &stop_ident );
         if(status == OK && stop_ident > highestMat)
         {
		      highestMat = stop_ident;
         }
	   }
   }
   
   if(highestMat > 0 )
   {
      json_object_dotset_number(root_object, "Materials.count",highestMat);
   }
   
   
   for(cur_material=1; cur_material<=highestMat; cur_material++)
   {
      base_name[0] = '\0';
      items_read = 0;
      // Check for a name
      sprintf(base_name,"MAT_NAME_%d",cur_material);
      result_name[0]='\0';
      status = mc_ti_read_string(database_id, base_name, (void*) &result_name);
      base_name[0]='\0';
      color_string[0]='\0';
      // Check for colors 
      sprintf(base_name,"SetRGB_%d",cur_material);
      status2 = mc_ti_read_array(database_id,base_name,(void**)&colors, &items_read);
      
      //If we found a name then use it otherwise just assign the number as the name
      if(status == OK)
      {
         base_name[0]='\0';
         
         sprintf(base_name,"Materials.%d.name",cur_material);
         json_object_dotset_string(root_object,base_name,result_name);
      }else
      {
         base_name[0]='\0';
         
         sprintf(base_name,"Materials.%d.%d",cur_material,cur_material);
         json_object_dotset_string(root_object,base_name,result_name);
      }
      //If we found colors then use them, otherwise just end the line.
      if(status2 == OK)
      {
         base_name[0]='\0';
         
         sprintf(base_name,"Materials.%d.COLOR",cur_material);
         sprintf(color_string,"[%.3f,%.3f,%.3f]", colors[0],colors[1],colors[2]);
         json_object_dotset_value(root_object,base_name,json_parse_string(color_string));
         free(colors);
      }
      
   }
   return OK;

}

/**
*    TAG(write_steps_json)
*    Write the time steps to the JSON object
*
*    @param Famid database_id  The indentifier for this database
*    @param JSON_Object  JSON element which to write timesteps
*/
static void
write_steps_json(Famid database_id)
{
  Mili_family* fam;  //This is the Mili database plot file
  char time_string[512];
  char* timesteps;
  int i;
  JSON_Object *root_object;
  float time; 
  if(database_id >= fam_qty || database_id <0)
  {
    return;
  }
  
  fam = fam_list[database_id];
  
  root_object = fam->root_object;
  if(JSONFailure == json_object_dotset_number(root_object, "States.count",fam->state_qty))
  {
      return;
  }
  if(!fam->state_map)
  {
      return;
  }
  timesteps = (char*)malloc(fam->state_qty*20);
  i=0;
  sprintf(timesteps,"[%1.7g", fam->state_map[i].time);
  for(i=1; i<fam->state_qty;i++)
  {
      strcat(timesteps,",");
      time_string[0]='\0';
      time = fam->state_map[i].time;
      
      sprintf(time_string,"%1.7g",time);
      strcat(timesteps,time_string);
      
  }
  strcat(timesteps,"]");
  
  json_object_dotset_value(root_object,"States.times",json_parse_string(timesteps));
  free(timesteps);
}


static void
getGlobalName(char fileName[256], Mili_family* fam)
{
   int end_of_string = strlen(fam->root)-1;
   if(end_of_string == 0)
   {
      return;
   } 
   
   while(fam->root[end_of_string] >= '0' && fam->root[end_of_string] <= '9')
   {
      end_of_string--;
   }
   
   strncat(fileName, fam->root, end_of_string+1);
   
}
/**
*   TAG(create_outfile)
*   Create the file to write out the data to.
*   @param Famid database_id The indentifier for this database
*   @return FILE* pointer to the newly created file.
*/
static FILE *
create_outfile(Famid database_id, int global)
{
  char MetadataFile[256];
  FILE *OUTFILE;
  
  Mili_family* fam;  //This is the Mili database plot file
  
  if(database_id >= fam_qty || database_id <0)
  {
    return NULL;
  }
    
  fam = fam_list[database_id];
  
  MetadataFile[0]='\0';
  
  if(global)
  {
      getGlobalName(MetadataFile,fam);
  }else
  {
      strcpy(MetadataFile,fam->root);
  }
  
  strcat(MetadataFile,".mili");
    
  OUTFILE = fopen(MetadataFile,"w+");
    
  if(!OUTFILE)
  {
      return NULL;
  }
  return OUTFILE;
  
}


/**
*   TAG(write_database_metadata_json)
*   This is general information about the database that is defined as a MILI_PARAM for the most part.
*   
*   @param Famid database_id  The indentifier for this database
*   @param JSON_Object* root_object  Json element that the data will be written to. 
*   @return void 
*/
static int
write_database_metadata_json(Famid database_id, JSON_Object *root_object)
{
  int rval = OK;
  numProcessors =1;
  char xmilics_version[100];
  xmilics_version[0] = '\0';
  Mili_family* fam;  //This is the Mili database plot file
  
  if(database_id >= fam_qty || database_id <0)
  {
    return BAD_FAMILY;
  }
    
  fam = fam_list[database_id];
  
  // Write the version of this output to the json object
  json_object_set_number( root_object,"Out_Version",OUT_VERSION);
  // Write the path to the json object
  json_object_set_string( root_object,"Path",fam->path);
  
  rval = mc_read_string( database_id, "xmilics version", xmilics_version ); 
  
  if(rval == OK && strlen(xmilics_version) >0)
  {
     // We set this due to the fact that the combined file will 
     // list the number of processors that the simulation used 
     // prior to combining.
     numProcessors = 1;
     xmilicsFile = 1;
  }else
  {
     //Get the number of processors from the mili database 
     numProcessors = fam->num_procs ; 
     if(numProcessors == 0 )
     {
         numProcessors = 1;
     }
  }
  
  json_object_set_number( root_object,"Domains",numProcessors);
      
  // Write the mesh dimensions
  json_object_set_number( root_object,"Dimensions",fam->dimensions);
    
  // Write the number of meshes.
  json_object_set_number( root_object,"Number_of_Meshes",fam->qty_meshes);
  
  return OK;
}


/**
*  TAG (mc_writeMiliMetaData)
*  Function to write out the files needed for VisIt.
*  @param Famid database   database id for this processor
*/
Return_value 
write_mili_metadata(Famid database_id, int global)
{
    
    Return_value rval = OK;  // This hopefully will return OK.  
                             //If not there is a problem.
    Mili_family* fam;  //This is the Mili database plot file
  
    if(database_id >= fam_qty || database_id <0)
    {
      return BAD_FAMILY;
    }
    
    fam = fam_list[database_id];
    
    int parameter_count = 0;
    int class_count = 0;
    int i;
    Hash_table *table = NULL;
    Hash_table *classTable = htable_create(1009);
    Mili_Class *miliClasses;
    FILE *OUTFILE;
    char *serialized_string = NULL;
    char database_name[128];
    int status;
    
    OUTFILE = create_outfile(database_id,global);
    
    if(!fam->visit_file_on)
    {
        if(OUTFILE == NULL)
        {
            return OK;
        }else
        {
            fclose(OUTFILE);
        }
        strcpy(database_name,fam->root);
        strcat(database_name,".mili");
        
        status = remove(database_name);
        
        if(status != 0)
        {
		  fprintf(stderr, "Error: %d deleting the file %s\n", errno,database_name);
        }
        return OK;
    }
    if(OUTFILE == NULL)
    {
      return NOT_OK;
    }
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value); 
    
    if(root_value == NULL || root_object == NULL)
    {
        return OK;
    }
    
    fam->root_object = root_object;
    fam->root_value = root_value;
    
    json_object_set_string(root_object,"fileType","processor");
    
    status = write_database_metadata_json(database_id, root_object);
    
    if(status != OK)
    {
        mc_print_error("Function: write_database_metadata_json()",status);
        return status;
    }
    
    writeMaterials_json(database_id, root_object);
    
    parameter_count = gather_parameters(database_id);
    if (parameter_count == 0)
    {
        return OK;
    }
    writeElementSets_json(root_object);
    
    class_count = countClasses(database_id);
    
    if(class_count <1)
    {
       return NO_CLASS;
    }
    
    miliClasses = NEW_N(Mili_Class, class_count, "Mili_Class");
     
    status = readClasses(database_id, classTable, miliClasses);
    
    table = process_state_variables(database_id, classTable);
    
    write_state_variables_json(table,root_object);
    
    writeClasses_json(classTable, miliClasses,root_object); 
    
    write_steps_json(database_id);
    
    serialized_string = json_serialize_to_string_pretty(root_value);
    
    fprintf(OUTFILE,"%s\n", serialized_string);
    // Time to clean up
    fclose(OUTFILE);
    free(serialized_string);
    cleanVariableTable(table);
    root_object = NULL;
    for(i=0;i<class_count;i++)
    {
       if(miliClasses[i].variables != NULL)
       {
         htable_delete(miliClasses[i].variables,NULL,0);
       }
    }
    free(miliClasses);
    if(subrec_string)
    {
       free (subrec_string);
       subrec_string = NULL;
    }
    htable_delete(classTable,NULL,0);
    for(i=0;i<elementSetCount;i++)
    {
      free(elementSets[i].integrationPoints);
    }
    free(elementSets);
    return rval;
 }

/*
 *   mc_create_single_processor_metadata(char* db_name)
 *   Take a file assume that the Mili database exists and 
 *   create a .mili from this single file.
 */

int
mc_create_metadata(char* db_name)
{
    char *path_index;
    Famid fam_id;
    int rval = OK;
    int index;
    char file_path[256];
    char *file_name;
    //We will need to search the string for a path.
    file_path[0]='\0';
    path_index = strrchr(db_name,'/');
    index = path_index - db_name;
    if(path_index)
    {
        strncat(file_path,path_index,index);
        file_name = path_index+1;
    }else
    {
        strcat(file_path,".");
        file_name = db_name;
    } 
    mc_open(file_name,file_path,"Ar",&fam_id);
    rval = write_mili_metadata(fam_id,0); 
    return rval;
}



Return_value 
mc_write_mili_metadata(Famid database_id)
{
   return write_mili_metadata(database_id,0);
}
/*
 *   TAG (update_visit_file)
 *   
 */
int 
update_visit_file(char * database_name, Famid database_id)
{
   Mili_family* fam;  //This is the Mili database plot file
  
   if(database_id >= fam_qty || database_id <0)
   {
     return BAD_FAMILY;
   }
    
   fam = fam_list[database_id];
   
   FILE *OUTFILE;
   
   JSON_Value *user_data = fam->root_value;
   JSON_Object *root_object = fam->root_object;
   
   char *serialized_string;
   
   if (json_object_remove(root_object,"States") != JSONSuccess)
   {
      fprintf(stderr,"Unable to remove timesteps\n");
   }
   
   write_steps_json(database_id);
   
   OUTFILE = fopen(database_name, "w+");
   
   serialized_string = json_serialize_to_string_pretty(user_data);
    
   fprintf(OUTFILE,"%s\n", serialized_string);
   
   fclose(OUTFILE);
   free(serialized_string);
   user_data = NULL;
   return OK;
}
/**
*  TAG (mc_update_visit_file)
*  Function to update an existing file json file. 
*  @param Famid database   database id for this processor
*/
int 
mc_update_visit_file(Famid database_id)
{
   Return_value rval = OK;
   char MetaDataFile[256];
   
   Mili_family* fam;  //This is the Mili database plot file
  
   if(database_id >= fam_qty || database_id <0)
   {
     return BAD_FAMILY;
   }
    
   fam = fam_list[database_id];
   if(database_id == 0)
   {
       rval = OK;
   }
   if(!fam->visit_file_on)
   {
       return OK;
   }
   // Make sure we start with a clean name
   MetaDataFile[0]='\0';
   strcpy(MetaDataFile,fam->root);
   
   strcat(MetaDataFile,".mili");
   
   rval = update_visit_file(MetaDataFile, database_id);
   
   return rval;

}

static void 
merge_element_sets(JSON_Object *base, const JSON_Object *incoming)
{
    int has_element_sets = 0;
    int i;
    const char *element_set_name;
    JSON_Object *base_object = NULL;
    char path[64];
    char *base_path = "Element_Sets.";
    JSON_Value *adding_value;
    JSON_Status status;
    JSON_Object *test, *in_object;
    JSON_Value *in_element_sets = json_object_get_value(incoming, "Element_Sets");
    
    //If the incoming JSON has no Element sets the just return
    if(!in_element_sets)
    {
        return;
    }
    
    JSON_Value *base_element_sets = json_object_get_value(base,"Element_Sets");
    
    if(base_element_sets)
    {
        has_element_sets = 1;
        base_object = json_object(base_element_sets);
    }
    
    in_object = json_object(in_element_sets);
    
    LONGLONG count = json_object_get_count(in_object);
    
    for(i=0; i<count; i++)
    {
        element_set_name =  json_object_get_name(in_object, i);
        if(!strcmp(element_set_name,"count"))
        {
            continue;
        }
        if(has_element_sets)
        {
            test =  json_object_get_object(base_object,
                                                        element_set_name);
            if(test)
            {
                continue;
            }
            adding_value = json_value_deep_copy(json_object_get_value_at(in_object,i));
            status = json_object_set_value(base_object,
                                           element_set_name,
                                           adding_value);
        }else
        {
            sprintf(path,"%s%s", base_path, element_set_name);
            adding_value = json_value_deep_copy(json_object_get_value_at(in_object,i));
            
            status = json_object_dotset_value(base,path,adding_value);
        }
    }
}

static void 
remove_subrecords(JSON_Object *base)
{
    int i;
    const char *var_name;
    JSON_Status rval;
    
    JSON_Value *base_variables = json_object_get_value(base,"Variables");
    JSON_Object *base_object = json_object(base_variables); 
    
    LONGLONG count = json_object_get_count(base_object);
    
    for(i=0; i< count; i++)
    {
        var_name =  json_object_get_name(base_object, i);
        
        if(!strcmp(var_name,"count"))
        {
            continue;
        }
        
        JSON_Object *test =  json_object_get_object(base_object,var_name);
        
        rval = json_object_remove(test, "subrecords");
        
        if(!rval)
        {
            // Did not delete subrecords object, but it is not the 
            // that crucial.
        }
    }
    
}


static void
merge_variables( JSON_Object *base, const JSON_Object *incoming)
{
    int i;
    const char *var_name;
    JSON_Status status;
    JSON_Value *in_variables = json_object_get_value(incoming, "Variables");
    JSON_Value *base_variables = json_object_get_value(base,"Variables");
    
    JSON_Object *in_object = json_object(in_variables); 
    JSON_Object *base_object = json_object(base_variables); 
    
    LONGLONG count = json_object_get_count(in_object);
    
    for(i=0; i<count; i++)
    {
        var_name =  json_object_get_name(in_object, i);
        
        if(!strcmp(var_name,"count"))
        {
            continue;
        }
        
        JSON_Object *test =  json_object_get_object(base_object,var_name);
        
        if(test)
        {
            continue;
        }
        
        
        JSON_Value *adding_value = json_value_deep_copy(json_object_get_value_at(in_object,i));
        json_object_remove(json_object(adding_value),"subrecords");
        status = json_object_set_value(base_object,var_name,adding_value);
        if(!status)
        {
            //Failed to add variable to json.  Will need to revisit and 
            //determine how we want to handle this situation.
        }
    }
    
}

static int 
get_processor_id( char* file_name, int *num_length)
{
    int pos=0;
    int process_number;
    *num_length = 0;
    char digits[6]; digits[0] = '\0';
    
    pos = strlen(file_name)-1 ;
    
    while( file_name[pos] >= '0' && file_name[pos] <= '9')
    {
        *num_length = *num_length + 1 ;
        pos--;
    }
    
    if(pos == strlen(file_name) -1)
    {
        return -1;
    }
    
    strcpy(digits, file_name+pos+1);
    process_number = atoi(digits);
    
    return process_number;
}


static void
remove_element_counts(JSON_Object *base)
{
    int i;
    const char *var_name;
    JSON_Status rval;
    JSON_Object *test;
    JSON_Value *base_variables = json_object_get_value(base,"Variables");
    JSON_Object *base_object = json_object(base_variables); 
    
    LONGLONG count = json_object_get_count(base_object);
    
    for(i=0; i< count; i++)
    {
        var_name =  json_object_get_name(base_object, i);
        
        //We need to ignore the count object
        if(!strcmp(var_name,"count"))
        {
            continue;
        }
        
        test =  json_object_get_object(base_object,var_name);
        
        rval = json_object_remove(test, "ElementCount");
        
        if (!rval)
        {
            // We did not remove the ElementCount object, 
            // however it is not crucial that it is deleted.
        }
    }
}


static void
merge_classes( JSON_Object *base, const JSON_Object *incoming)
{
    int i,j,k;
    const char *var_name;
    JSON_Status status;
    JSON_Value *in_variables = json_object_get_value(incoming, "Classes");
    JSON_Value *base_variables = json_object_get_value(base,"Classes");
    
    JSON_Object *in_object = json_object(in_variables); 
    JSON_Object *base_object = json_object(base_variables);
    
    LONGLONG count = json_object_get_count(in_object);
    
    for(i=0; i<count; i++)
    {
        var_name =  json_object_get_name(in_object, i);
        
        if(!strcmp(var_name,"count"))
        {
            continue;
        }
        
        JSON_Object *test =  json_object_get_object(base_object,var_name);
        
        if(!test)
        {
            JSON_Value *adding_value = json_value_deep_copy(json_object_get_value_at(in_object,i));
            if(adding_value)
            {
                status = json_object_remove(json_object(adding_value), "ElementCount");
                
                status = json_object_set_value(base_object,var_name,adding_value);
            }
        }else
        {
            JSON_Object *class_object = json_object_get_object(in_object,var_name);
            status = json_object_remove(test, "ElementCount");
            JSON_Array  *array = json_object_get_array(class_object, "variables");
            int var_count = json_array_get_count(array);
            
            JSON_Array  *base_array = json_object_get_array(test, "variables");
            int base_var_count = json_array_get_count(base_array);
            
            for(j=0;j<var_count;j++)
            {
                const char *incoming_var = json_array_get_string(array,j);
                for(k=0; k<base_var_count; k++)
                {
                    if(!strcmp(incoming_var, json_array_get_string(base_array,k)))
                    {
                        break;
                    }
                }
                if(k==base_var_count)
                {
                    char *entry = calloc( 1, strlen(incoming_var)+1);
                    strcpy(entry, incoming_var);
                    json_array_append_string( base_array, entry);
                }
            }

            //JSON_Value *adding_value = json_value_deep_copy(json_object_get_value_at(in_object,i));
            //JSON_Status status = json_object_set_value(base_object,var_name,adding_value);
        }
    }
}


/*
 *   TAG (fix_count)
 *   Used in the global file to correct the counts of select objects in the JSON files
 */
static void
fix_count(JSON_Object *base_object, char *target)
{
    LONGLONG count;
    JSON_Object *temp_object = json_object_get_object(base_object,target);
    JSON_Value *count_object = json_object_get_value(temp_object,"count");
    if(count_object)
    {
        count = json_object_get_count(temp_object) -1 ;
    }else
    {
        count = json_object_get_count(temp_object);
    }
    
    json_object_set_number(temp_object, "count",count);
}

/*
 *  TAG(mc_write_global_metadata)
 *  write the global visit file information.  You should call this
 *  from the zero processor but it will work otherwise also. 
 */
int
mc_write_global_metadata(Famid database_id)
{
    FILE *OUTFILE = NULL, 
         *INFILE  = NULL;
    
    char *serialized_string = NULL;
    int processors;
    int i;
    char database_name[256];
    char base_name_without_path[128];
    char base_name[128];
    char db_with_path[512];
    char *domain_base = "Domain_files.";
    char domain_path[64]; 
    JSON_Value *user_data;
    JSON_Value *next_data;
    int file_id;
    int *files_processed;
    int num_length; 
    JSON_Object *base_object;
    JSON_Object *incoming_object;
    int status;  // Used for the removing of the .mili file
    Return_value rval;
    database_name[0] = '\0';
    base_name_without_path[0] = '\0';
    base_name[0] = '\0';
    Mili_family* fam;  //This is the Mili database plot file.
  
    if(database_id >= fam_qty || database_id <0)
    {
        return BAD_FAMILY;
    }
    
    fam = fam_list[database_id];
    
    if(!fam->visit_file_on)
    {
        // Clean up old .mili files as VisIt file generation is turned off.
        getGlobalName(database_name,fam);
        strcat(database_name, ".mili");
        //We do this as it may be open in some capacity and 
        //it needs to be closed to remove.
        INFILE = fopen(database_name,"r");
        if(INFILE)
        {
            fclose(INFILE);
        }else
        {
            //It did not exist. Do nothing
            return OK;
        }
        
        status = remove(database_name);
        
        if(status != 0)
        {
		  fprintf(stderr, "Error: %d deleting the file %s\n", errno,database_name);
        }
        return OK;
    }
    
    rval = mc_read_scalar(database_id,"nproc", (void*)(&processors));
    
    if(rval == OK && processors == 1)
    {
        //This is not a "global" file as it only had one processor 
        return OK;
    }
    OUTFILE = create_outfile(database_id,1);
    //Allocate the file checks
    files_processed = calloc( sizeof(int), processors);
    // Lets read the first processor file.  
    strcpy(db_with_path,fam->root);
    strcat(db_with_path,".mili");
    
    user_data = json_parse_file(db_with_path);
    
    file_id = get_processor_id(fam->root,&num_length);
    
    strncpy(base_name, fam->root, strlen(fam->root)-num_length);
    
    files_processed[file_id] = 1;
    char * start_pos = strrchr(base_name,'/')+1;
    sprintf(domain_path, "%s%d", domain_base,file_id);
    sprintf(base_name_without_path,"%s.mili",fam->file_root);
    base_object = json_object(user_data);
    
    json_object_dotset_string(base_object, domain_path,base_name_without_path);
    json_object_set_string(base_object,"fileType","global");
    for(i=0; i<processors; i++)
    {
        if(files_processed[i])
        {
            continue;
        }
        
        sprintf ( database_name, "%s%0*d.mili",  base_name, num_length,i);
        sprintf(domain_path, "%s%d", domain_base,i);
        
        sprintf(base_name_without_path,"%s%0*d.mili",start_pos,num_length,i);
        json_object_dotset_string(base_object, domain_path,base_name_without_path);
        next_data = json_parse_file(database_name);
        
        if(next_data)
        {
            incoming_object = json_object(next_data);
            remove_subrecords(base_object);
            merge_element_sets(base_object, incoming_object);
            merge_variables(base_object, incoming_object);
            remove_element_counts(base_object);
            merge_classes(base_object, incoming_object);
        }
        
        
        
    }
    // Lets clean up the counts for some of the data
    
    fix_count(base_object,"Classes");
    fix_count(base_object,"Variables");
    fix_count(base_object,"Element_Sets");
    fix_count(base_object,"Materials");
    
    
    serialized_string = json_serialize_to_string_pretty(user_data);
    
    fprintf(OUTFILE,"%s\n", serialized_string);
   
    fclose(OUTFILE);
    
    json_value_free(user_data);
    return OK;
}


/**
*  TAG (mc_update_visit_file)
*  Function to update an existing file json file. 
*  @param Famid database   database id for this processor
*  @param int process_id the parallel process identifier.
*/
int 
mc_update_global_times(Famid database_id)
{
   Return_value rval = OK;
   char GlobalMetaDataFile[256];
   
   Mili_family* fam;  //This is the Mili database plot file
  
   if(database_id >= fam_qty || database_id <0)
   {
     return BAD_FAMILY;
   }
    
   fam = fam_list[database_id];
   if(!fam->visit_file_on)
   {
       return OK;
   }
   GlobalMetaDataFile[0]='\0';
   
   getGlobalName(GlobalMetaDataFile, fam);
   
   
   strcat(GlobalMetaDataFile,".mili");
   update_visit_file(GlobalMetaDataFile, database_id);
   
   return rval;
}
