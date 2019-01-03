/*
 * $Log: SiloLib_dir.c,v $
 * Revision 1.9  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.8  2008/11/14 01:03:45  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.6  2008/09/24 18:36:02  corey3
 * New update of Passit source files
 *
 * Revision 1.5  2007/09/14 22:56:03  corey3
 * Added more functionality to Mili-Silo API
 *
 * Revision 1.4  2007/08/31 22:38:25  corey3
 * Added new functionality to Mili-Silo
 *
 * Revision 1.3  2007/08/24 19:54:07  corey3
 * Added more functions for support writing Silo files
 *
 * Revision 1.2  2007/07/05 22:03:50  corey3
 * Adding more functionality to Silo Library Interface
 *
 * Revision 1.1  2007/06/25 17:58:33  corey3
 * New Mili/Silo interface
 *
 *
 */

char *lastDir[64]; /* Holds the name of the last 'n' directories
		    * accessed for doing a cd '..'
		    */

int lastDir_index = 0;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                                                              */
/*                   Copyright (c) 2007                         */
/*         The Regents of the University of California          */
/*                     All Rights Reserved                      */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/

/*  Include Files  */
#include <string.h>
#include <malloc.h>
#include "silo.h"
#include "SiloLib_Internal.h"
#include "SiloLib.h"

int SiloLIB_mkdir(int  famid,
                  int  filetype,
                  char *dirname)
/****************************************************************/
/*  Routine SiloLib_mkdir creates a new directory level in      */
/*  a Silo database.                                            */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Apr-2007  IRC  Initial version.                          */
/****************************************************************/
{
   int status, ignore;
   char curr_dir[128];
   DBfile *db=NULL;

   if (famid != 0)
   {
      db     = SiloLIB_getDBid(famid, filetype);
      ignore = SiloLIB_Disable_Errors( );
      status = DBMkDir(db, dirname);
      ignore = SiloLIB_Enable_Errors( );
   }

   return (status);
}


int SiloLIB_mkcddir(int    famid,
                    int    filetype,
                    char   *pathname,
                    char   *dirname)
/****************************************************************/
/*  Routine SiloLib_mkcddir creates a new directory level in    */
/*  a Silo database if it does not exist then changes to that   */
/*  new directory..                                             */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  12-Sep-2007  IRC  Initial version.                          */
/****************************************************************/
{
   DBfile *db=NULL;
   int status;

   if ( pathname )
      if ( !strcmp("/", pathname) ) /* CD to the root */
         status = SiloLIB_cddir(db, "/") ;


   if (famid != 0)
   {
      db     = SiloLIB_getDBid(famid, filetype);
      status = SiloLIB_cddir(famid, filetype, dirname) ;

      if ( status!= 0 )
      {
         status = SiloLIB_mkdir(famid, filetype, dirname) ;
         status = SiloLIB_cddir(famid, filetype, dirname) ;
      }
   }

   return (status);
}


int SiloLIB_cddir(int    famid,
                  int    filetype,
                  char   *pathname)
/****************************************************************/
/*  Routine SiloLib_mkdir CDs to a new directory level in       */
/*  a Silo database.                                            */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Apr-2007  IRC  Initial version.                          */
/****************************************************************/
{
   DBfile *db=NULL;
   int    ignore;
   char   curr_dir[128];
   int    status=OK;

   ignore = SiloLIB_Disable_Errors( );
   status = SiloLIB_getdir(famid, filetype, curr_dir) ;

   /* Return if we are already in the requested directory */
   if ( !strcmp(curr_dir, pathname) )
      return (status);

   if (famid != 0)
   {
      if (!strcmp(pathname, ".."))
      {
         db     = SiloLIB_getDBid(famid, filetype);
         status = SiloLIB_getdir(famid, filetype, curr_dir) ;
         if (!strcmp(curr_dir, "/"))
            return(OK); /* Already at the Root */

         if (lastDir_index>0)
         {
            status = DBSetDir(db, "..");
            free(lastDir[lastDir_index--]);
         }
      }
      else
      {
         status = DBSetDir(db, pathname);
         lastDir_index++;
         lastDir[lastDir_index] = malloc(sizeof(char)*strlen(pathname)+1);
         strcpy(lastDir[lastDir_index], pathname);
      }
   }

   ignore = SiloLIB_Enable_Errors( );
   return (status);
}


int SiloLIB_getdir(int    famid,
                   int    filetype,
                   char   *pathname)
/****************************************************************/
/*  Routine SiloLib_mkdir CDs to a new directory level in       */
/*  a Silo database.                                            */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Apr-2007  IRC  Initial version.                          */
/****************************************************************/
{
   DBfile *db=NULL;
   int status;

   if (famid != 0)
   {
      db     = SiloLIB_getDBid(famid, filetype);
      status = DBGetDir(db, pathname);
   }

   return (status);
}


int SiloLIB_getTOC(int famid,         int filetype,
                   int *num_dirs,     char **dir_names,
                   int *num_vars,     char **var_names,
                   int *num_meshvars, char **mesh_vars,
                   int *num_ptvars,   char **pt_vars)
/****************************************************************/
/*  Routine SiloLib_getTOC lists the contents of the current    */
/*  Silo directory.                                             */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Sep-2007  IRC  Initial version.                          */
/****************************************************************/
{
   int i;
   DBfile *db=NULL;
   DBtoc *toc;
   int status;

   if (famid != 0)
   {
      db  = SiloLIB_getDBid(famid, filetype);
      toc = DBGetToc(db);
   }
   else
      return(!OK);

   /* Parse fields of interest in th TOC structure */

   *num_dirs = toc->ndir;
   if ( *num_dirs>0 && dir_names!=NULL )
   {
      for (i=0;
            i<*num_dirs;
            i++)
         dir_names[i] = strdup(toc->dir_names[i]);
   }

   *num_vars = toc->nvar;
   if ( *num_vars>0 && var_names!=NULL )
   {
      for (i=0;
            i<*num_vars;
            i++)
         var_names[i] = strdup(toc->var_names[i]);
   }

   *num_meshvars = toc->nucdvar;
   if ( *num_meshvars>0 && mesh_vars!=NULL )
   {
      for (i=0;
            i<*num_meshvars;
            i++)
         mesh_vars[i] = strdup(toc->ucdvar_names[i]);
   }

   *num_ptvars = toc->nptvar;
   if ( *num_ptvars>0 && pt_vars!=NULL )
   {
      for (i=0;
            i<*num_ptvars;
            i++)
         mesh_vars[i] = strdup(toc->ptvar_names[i]);
   }

   return (status);
}


DBfile *SiloLIB_getDBid(int famid,
                        int filetype )
/****************************************************************/
/* Routine SiloLib_getDBid - Returns the Silo database id       */
/* for the family id. */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Sep-2007  IRC  Initial version.                          */
/****************************************************************/
{
   int i;
   int status;

   for (i=0;
         i<silo_family_qty;
         i++)
      if ( silo_family[i].famid == famid )
      {
         if ( filetype==SILO_PLOTFILE )
            return ( silo_family[i].restart_file_pointer );
         else
            return ( silo_family[i].root_file_pointer ) ;
      }
   return (NULL);
}

/* End SiloLib_dir.c */
