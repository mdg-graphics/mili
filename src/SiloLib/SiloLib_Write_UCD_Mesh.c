/*
 * $Log: SiloLib_Write_UCD_Mesh.c,v $
 * Revision 1.5  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.4  2008/11/14 00:08:53  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.3  2008/09/24 18:36:02  corey3
 * New update of Passit source files
 *
 * Revision 1.2  2007/08/24 19:54:07  corey3
 * Added more functions for support writing Silo files
 *
 * Revision 1.1  2007/06/25 17:58:32  corey3
 * New Mili/Silo interface
 *
 *
*/


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                                                              */
/*                   Copyright (c) 2005                         */
/*         The Regents of the University of California          */
/*                     All Rights Reserved                      */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  18-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/

/*  Include Files  */

#include "silo.h"
#include <string.h>
#include "SiloLib_Internal.h"

/* Element types */
#define ELEM_BRICK   0
#define ELEM_SHELL   1
#define ELEM_TSHELL  2
#define ELEM_BEAM    3
#define ELEM_TET     4
#define ELEM_WEDGE   5
#define ELEM_PYRAMID 6
#define ELEM_TRUSS   7
#define ELEM_SLIDE  10


int SiloLib_Write_UCD_Mesh(int    famid,
                           int    filetype,
                           char   *mesh_name,
                           int    elem_type,
                           int    group_num,
                           int    ndims,
                           int    *nodelist,
                           int    nl_length,
                           char   **coord_names,
                           void   **coords,
                           int    nnodes,
                           int    nzones,
                           int    *global_node_ids,
                           int    *global_zone_ids,
                           int    zl_number,
                           double time,
                           double time_step,
                           int    cycle,
                           int    datatype)

/****************************************************************/
/* This function will write out a UCD mesh for all element      */
/* types to the Silo file.                                      */
/*                                                              */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  04-Jun-2007  IRC  Initial version.                          */
/****************************************************************/
{
   DBfile  *db=NULL;
   int     i ;
   int     status=OK;

   double  coord_init;

   float   *coord_arrays_flt[3];
   double  *coord_arrays_dbl[3];

   /* optlist variables */
   int        origin = 0 ;
   int        majororder = 0 ;
   int        hi[3]  = {0,0,0} ;
   int        low[3] = {0,0,0} ;
   int        coordsys;
   int        planar;
   DBoptlist *optlist;

   /* Facelist variables */

   DBfacelist *fl;

   /* Zonelist variables */
   int shapetype[2], shapesize[2], shapecnt[2];

   char elem_zonelist_name[32], elem_facelist_name[32];

   db = SiloLIB_getDBid(famid, filetype);

   /* Set element type specific variables */

   switch (elem_type)
   {
      case ELEM_BRICK:
         shapetype[0] = DB_ZONETYPE_HEX;
         shapesize[0] = 8;
         strcpy(elem_zonelist_name, "Zones_Brick");
         strcpy(elem_facelist_name, "Faces_Brick");
         break;

      case ELEM_SHELL:
         shapetype[0] = DB_ZONETYPE_QUAD;
         shapesize[0] = 4;
         strcpy(elem_zonelist_name, "Zones_Shell");
         strcpy(elem_facelist_name, "Faces_Shell");
         break;

      case ELEM_TSHELL:
         shapetype[0] = DB_ZONETYPE_HEX;
         shapesize[0] = 8;
         strcpy(elem_zonelist_name, "Zones_TShell");
         strcpy(elem_facelist_name, "Faces_TShell");
         break;

      case ELEM_WEDGE:
         shapetype[0] = DB_ZONETYPE_PRISM;
         shapesize[0] = 6;
         strcpy(elem_zonelist_name, "Zones_Wedge");
         strcpy(elem_facelist_name, "Faces_Wedge");
         break;

      case ELEM_PYRAMID:
         shapetype[0] = DB_ZONETYPE_PYRAMID;
         shapesize[0] = 5;
         strcpy(elem_zonelist_name, "Zones_Pyramid");
         strcpy(elem_facelist_name, "Faces_Pyramid");
         break;

      case ELEM_TET:
         shapetype[0] = DB_ZONETYPE_TET;
         shapesize[0] = 4;
         strcpy(elem_zonelist_name, "Zones_Tet");
         strcpy(elem_facelist_name, "Faces_Tet");
         break;

      case ELEM_BEAM: /* No actual 'BEAM' type in Silo, so need to represent with
		     * a triangle.
		     */
         shapetype[0] = DB_ZONETYPE_TRIANGLE;
         shapesize[0] = 3;
         strcpy(elem_zonelist_name, "Zones_Beam");
         strcpy(elem_facelist_name, "Faces_Beam");
         break;

      case ELEM_TRUSS:
         shapetype[0] = DB_ZONETYPE_BEAM;
         shapesize[0] = 2;
         strcpy(elem_zonelist_name, "Zones_Truss");
         strcpy(elem_facelist_name, "Faces_Truss");
         break;

      case ELEM_SLIDE:
         shapetype[0] = DB_ZONETYPE_QUAD;
         shapesize[0] = 4;
         sprintf(elem_zonelist_name, "Zones_Slide_%04d", zl_number);
         sprintf(elem_facelist_name, "Faces_Slide_%04d", zl_number);
         break;
   }

   if ( datatype==DB_FLOAT )
   {
      coord_arrays_flt[X] = (float *) coords[X];
      coord_arrays_flt[Y] = (float *) coords[Y];
      coord_arrays_flt[Z] = (float *) coords[Z];
   }
   else
   {
      coord_arrays_dbl[X] = (double *) coords[X];
      coord_arrays_dbl[Y] = (double *) coords[Y];
      coord_arrays_dbl[Z] = (double *) coords[Z];
   }

   /* Make quadvar option list */

   optlist = DBMakeOptlist(20) ;

   DBAddOption(optlist, DBOPT_GROUPNUM, (void *)&group_num) ;
   DBAddOption(optlist, DBOPT_CYCLE, (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_TIME, (void *)&time) ;
   DBAddOption(optlist, DBOPT_DTIME, (void *)&time_step) ;
   DBAddOption(optlist, DBOPT_MAJORORDER, (void *)&majororder) ;
   DBAddOption(optlist, DBOPT_ORIGIN, (void *)&origin) ;
   DBAddOption(optlist, DBOPT_HI_OFFSET, (void *)&hi) ;
   DBAddOption(optlist, DBOPT_LO_OFFSET, (void *)&low) ;
   DBAddOption(optlist, DBOPT_XUNITS, "cm");
   DBAddOption(optlist, DBOPT_YUNITS, "cm");
   DBAddOption(optlist, DBOPT_ZUNITS, "cm");

   planar   = DB_NONE ;
   coordsys = DB_OTHER ;
   DBAddOption(optlist, DBOPT_PLANAR, (void *)&planar) ;
   DBAddOption(optlist, DBOPT_COORDSYS, (void *)&coordsys) ;
   DBAddOption(optlist, DBOPT_XLABEL, "X Axis") ;
   DBAddOption(optlist, DBOPT_YLABEL, "Y Axis") ;
   DBAddOption(optlist, DBOPT_ZLABEL, "Z Axis") ;

   if ( global_node_ids != NULL )
      DBAddOption(optlist, DBOPT_NODENUM, (void *)global_node_ids) ;

   /* Write the Nodal Mesh */
   if ( datatype==DB_FLOAT )
   {
      status = DBPutUcdmesh(db,
                            mesh_name,
                            ndims,
                            coord_names,
                            (void *) coord_arrays_flt,
                            nnodes, nzones,
                            elem_zonelist_name,
                            NULL,
                            datatype,
                            optlist) ;
   }
   else
   {
      status = DBPutUcdmesh(db,
                            mesh_name,
                            ndims,
                            coord_names,
                            (void *) coord_arrays_dbl,
                            nnodes, nzones,
                            elem_zonelist_name,
                            NULL,
                            datatype,
                            optlist) ;
   }

   shapecnt[0]  = nzones;

   /* Create the Zonelist */
   if ( global_zone_ids != NULL )
      DBAddOption(optlist, DBOPT_ZONENUM, (void *)global_zone_ids) ;

   DBPutZonelist2(db,
                  elem_zonelist_name,
                  nzones,
                  ndims,
                  nodelist,
                  nl_length,
                  0,
                  0,
                  0,
                  shapetype,
                  shapesize,
                  shapecnt,
                  1,
                  NULL);


   /* Create the Facelist  - not really needed since VisIt will calculate but leave code here
    * as a reference
    */

#ifdef FACELIST
   fl = SiloLib_Create_External_Facelist(nodelist,
                                         nl_length,
                                         shapesize,
                                         shapecnt,
                                         1,
                                         NULL);

   status = DBPutFacelist(db,
                          elem_facelist_name,
                          fl->nfaces,
                          3,
                          fl->nodelist,
                          fl->lnodelist,
                          0,
                          fl->zoneno,
                          shapesize,
                          shapecnt,
                          1,
                          NULL,
                          NULL,
                          0);
#endif

   DBFreeOptlist(optlist);
   return(status) ;
}

/* End SiloLib_Write_UCD_Mesh.c */
