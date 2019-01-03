/* $Id: mc_silo_class.c,v 1.10 2011/05/26 21:43:13 durrenberger1 Exp $ */

/*
 * Routines for managing unstructured mesh geometry.
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

#include <string.h>
#include <sys/time.h>
#include "mili_internal.h"
#include "mili.h"
#include "MiliSilo_Internal.h"
#include "SiloLib.h"

/*****************************************************************
 * TAG( mc_silo_def_class ) PUBLIC
 *
 * Creates an entry or a new Mili class definition
 */
int
mc_silo_def_class( Famid famid, int mesh_id, int superclass, char *short_name,
                   char *long_name )
{
   MiliSilo_family *fam;
   int  class_id;
   int  status;
   int  num_classes;
   int  *var1;

   char mesh_name[32];

   fam = &family[famid];
   num_classes = fam->meshes[mesh_id].qty_classes;

   /* if ( mesh_id > fam->qty_meshes - 1 )
      return NO_MESH; */

   /* Create the class in the silo file if it does not already exist */

   class_id = mc_silo_get_class_id( famid, mesh_id, short_name, -1 );
   if ( class_id>0 )
      return ENTRY_EXISTS;

   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Metadata", "/") ;

   sprintf(mesh_name, "MESH_%d", mesh_id);
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, mesh_name, NULL) ;

   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "Classes", NULL) ;

   /* Create a subrirectory for this class */
   status = SiloLIB_mkdir(ROOT, SILO_PLOTFILE, short_name) ;
   if ( status != 0 )
      return ((int) ENTRY_EXISTS);

   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, short_name) ;
   if ( status != 0 )
      return (status);

   var1   = &fam->meshes[mesh_id].qty_classes;
   status = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "id", 0., 1, 1, 1, DB_INT, var1);

   var1   = &fam->meshes[mesh_id].superclass;
   status = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "superclass", 0., 1, 1, 1, DB_INT, var1);

   var1   = &fam->meshes[mesh_id].class[num_classes].num_parents;
   status = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "num_parents", 0., 1, 1, 1, DB_INT, var1);

   if ( fam->meshes[mesh_id].class[num_classes].num_parents>0 )
   {
      var1   =  &fam->meshes[mesh_id].class[num_classes].parent_ids[0];
      status = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "parent_ids", 0., 1, 1, fam->meshes[mesh_id].class[num_classes].num_parents, DB_INT, var1);
   }

   var1   = &fam->meshes[mesh_id].class[num_classes].num_children;
   status = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "num_children", 0., 1, 1, 1, DB_INT, var1);

   if ( fam->meshes[mesh_id].class[num_classes].num_children>0 )
   {
      var1   =  &fam->meshes[mesh_id].class[num_classes].child_ids[0];
      status = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "child_ids", 0., 1, 1, fam->meshes[mesh_id].class[num_classes].num_children, DB_INT, var1);
   }

   fam->meshes[mesh_id].classList[num_classes]        = strdup( short_name );
   fam->meshes[mesh_id].superclass                    = superclass;
   fam->meshes[mesh_id].class[num_classes].short_name = strdup( short_name );
   fam->meshes[mesh_id].class[num_classes].long_name  = strdup( long_name );
   fam->meshes[mesh_id].qty_classes++;

   return OK;
}



/*****************************************************************
 * TAG( mc_silo_def_class_idents ) PUBLIC
 *
 * Define object identifiers for objects in a particular class.
 * The class should be derived only from one of the superclasses
 * which represent objects of which there may be more than one
 * and which are not otherwise defined in another mc_def...()
 * call (as of this writing, M_UNIT or M_MAT).
 */
int
mc_silo_def_class_idents( Famid famid, int mesh_id, char *short_name, int start,
                          int stop )
{
   int class_id;
   int qty_classes;
   int object_id;
   int rval;
   int status;
   int superclass;
   int  *var1;
   MiliSilo_family *fam;
   char mesh_name[32];

   fam = &family[famid];

   /*    if ( fam->access_mode != DB_READ )
        return BAD_ACCESS_TYPE;

   if ( mesh_id > fam->qty_meshes - 1 )
       return NO_MESH;
   */

   class_id = mc_silo_get_class_id( famid, mesh_id, short_name, -1 );
   if ( class_id<0 )
      return NO_CLASS;

   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, "/") ;
   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, "/Metadata") ;
   if ( status!= 0 )
      return ENTRY_NOT_FOUND;

   sprintf(mesh_name, "MESH_%d", mesh_id);
   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, mesh_name) ;

   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, short_name) ;
   if ( status!= 0 )
      return NO_CLASS;

   qty_classes = fam->meshes[mesh_id].qty_classes;

   object_id = fam->meshes[mesh_id].class[class_id].qty_objects;
   var1      = &fam->meshes[mesh_id].class[class_id].parent_ids[0];
   status    = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "parent_ids", 0., 1, 1, fam->meshes[mesh_id].class[class_id].num_parents, DB_INT, var1);

   if (fam->meshes[mesh_id].class[class_id].p_bl==NULL )
      fam->meshes[mesh_id].class[class_id].p_bl = NEW( Block_list, "Class block list" );

   status = insert_range( fam->meshes[mesh_id].class[class_id].p_bl, start, stop );
   if ( status!=OK )
      return status;

   fam->meshes[mesh_id].class[class_id].obj_ids_start[object_id] = start;
   var1    = &fam->meshes[mesh_id].class[class_id].obj_ids_start[object_id];
   status  = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "obj_ids_start", 0., 1, 1, fam->meshes[mesh_id].class[qty_classes].qty_objects, DB_INT, var1);

   fam->meshes[mesh_id].class[class_id].obj_ids_start[object_id] = stop;
   var1    = &fam->meshes[mesh_id].class[class_id].obj_ids_stop[object_id];
   status  = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "obj_ids_stop", 0., 1, 1, fam->meshes[mesh_id].class[qty_classes].qty_objects, DB_INT, var1);

   /* Update the object qualtity */
   fam->meshes[mesh_id].class[class_id].qty_objects++;
   var1   = &fam->meshes[mesh_id].class[class_id].qty_objects;
   status = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "qty_objects", 0., 1, 1, 1, DB_INT, var1);

   return OK;
}


/*****************************************************************
 * TAG( mc_silo_get_class_info ) PUBLIC
 *
 * Get the name(s) and number of objects in a class identified by
 * an index number.
 */
int
mc_silo_get_class_info( Famid famid, int mesh_id, int superclass,
                        int class_index, char *short_name, char *long_name,
                        int *object_qty )
{
   int class_id;
   MiliSilo_family *fam;


   /*    if ( fam->access_mode != DB_READ )
         return BAD_ACCESS_TYPE;

    if ( mesh_id > fam->qty_meshes - 1 )
        return NO_MESH;
    */

   class_id = mc_silo_get_class_id( famid, mesh_id, short_name, -1 );
   if ( class_id<0 )
      return NO_CLASS;

   *object_qty = fam->meshes[mesh_id].class[class_id].qty_objects;
   strcpy( short_name, fam->meshes[mesh_id].class[class_id].short_name );
   strcpy( long_name, fam->meshes[mesh_id].class[class_id].long_name );

   return (int) OK;
}


/*****************************************************************
 * TAG( mc_silo_get_simple_class_info ) PUBLIC
 *
 * Get the name(s) and start/stop identifiers in a class identified by
 * an index number.
 */
int
mc_silo_get_simple_class_info( Famid famid, int mesh_id, int superclass,
                               int class_index, char *short_name, char *long_name,
                               int *start_ident, int *stop_ident )
{
   MiliSilo_family *fam;
   int class_num;
   int i, j;

   if ( validate_fam_id( famid ) == OK )
      fam = &family[famid];
   else
      return (int) BAD_FAMILY;

   /*  if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
       return (int) NO_MESH;*/

   class_num = mc_silo_get_class_id( famid, mesh_id, NULL, class_index );
   if ( class_index < 0 )
      return (int) NO_CLASS;

   strcpy( short_name, fam->meshes[mesh_id].class[class_num].short_name );
   strcpy( long_name,  fam->meshes[mesh_id].class[class_num].long_name );

   *start_ident = fam->meshes[mesh_id].class[class_num].p_bl->blocks->start;
   *stop_ident  = fam->meshes[mesh_id].class[class_num].p_bl->blocks->stop;

   return (int) OK;
}


/*****************************************************************
 * TAG( mc_silo_get_class_id ) PRIVATE
 *
 * Return the id number of the class named 'short_name'.  If short_name is
 * NULL then look for the index of the 'nth' class identified by class_num.
 *
 */
int
mc_silo_get_class_id( Famid famid, int mesh_id, char *short_name, int class_num )
{
   int i;
   MiliSilo_family *fam;
   int class_id = -1, num_classes = 0, num_classes_found = 0;

   fam = &family[famid];

   num_classes = fam->meshes[mesh_id].qty_classes;
   if ( strlen(short_name)>0 )
   {
      for ( i=0;
            i<num_classes;
            i++)

         if ( !strcmp(fam->meshes[mesh_id].class[i].short_name, short_name) )
            class_id = i;
   }
   else
   {
      for ( i=0;
            i<num_classes;
            i++)
         if ( num_classes_found+1 == class_num )
         {
            num_classes_found++;
            class_id = i;
            return class_id;
         }
   }

   return class_id ;
}
