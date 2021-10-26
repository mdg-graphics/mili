/*
 * $Id $
 *
*/


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
/*  18-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/

/*  Include Files  */
#include <stdlib.h>
#include "silo.h"
#include "string.h"
#include "SiloLib_Internal.h"

static int silolib_init=FALSE;

int SiloLib_Open_Restart_File(int  famid,
                              char *root,
                              char *path,
                              int  cycle,
                              int  processor,
                              int  seqnum,
                              int  filetype,
                              int  run_mode)


/****************************************************************/
/*  Routine SiloLib_Open_Restart_File opens the Silo restart    */
/*  file.                                                       */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  12-Apr-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/
{
   char error_message[SILOLIB_MAX_OUTPUT_STRING_LENGTH] = "";
   char restart_file_name[SILOLIB_MAX_FILE_NAME_LENGTH] = "";
   char restart_file_dir[SILOLIB_MAX_FILE_NAME_LENGTH] = "";
   char restart_file[2*SILOLIB_MAX_FILE_NAME_LENGTH] = "";
   char command[2*SILOLIB_MAX_FILE_NAME_LENGTH] = "";

   DBfile *db;
   int status=OK;

   char *temp_filename, fullpath[128];

   /* Initialize SiloLib if we have not done so */
   if ( !silolib_init )
   {
      status = SiloLib_Init();
   }


   /* First make a filename */
   temp_filename = SiloLib_Create_Filename(root, filetype, cycle, processor, seqnum);

   strcpy(fullpath, temp_filename);
   strcat(fullpath, "/");
   strcat(fullpath, temp_filename);

   system("pwd;ls");

   /* Open the SILO restart file. */
   if (run_mode == FILE_CREATE)
   {
      /* Create the directory the files will be stored in */

      if ( filetype==SILO_ROOTFILE )
      {
         sprintf( command, "mkdir -p %s", root );
         system ( command );
         sprintf( command, "cd %s", root );
         system ( command );
      }

      /* Create a new restart file. */
      db = DBCreate(fullpath,
                    DB_CLOBBER,
                    DB_LOCAL,
                    NULL,
                    DB_PDB);
   }
   else if (run_mode == FILE_APPEND)
   {
      if ( filetype==SILO_ROOTFILE )
      {
         sprintf( command, "cd %s", path );
         system ( command );
      }

      /* Open an existing restart file. */
      db = DBOpen(fullpath,
                  DB_PDB,
                  DB_APPEND);
   }

   else if (run_mode == FILE_READ)
   {
      if ( filetype==SILO_ROOTFILE )
      {
         sprintf( command, "cd %s", path );
         system ( command );
      }

      /* Open an existing restart file. */
      db = DBOpen(root,
                  DB_PDB,
                  DB_READ);

   }

   if (db == NULL)
   {
      sprintf(error_message,
              "Open of Graphics file %s failed!\n",
              restart_file);

      printf("\n%s", error_message);
      return(NOT_OK);
   }

   if ( filetype==SILO_PLOTFILE )
      silo_family[silo_family_qty].restart_file_pointer = db;
   else
      silo_family[silo_family_qty].root_file_pointer    = db;

   silo_family[silo_family_qty].famid     = famid;
   silo_family[silo_family_qty].root      = strdup(root);
   silo_family[silo_family_qty].cycle     = cycle;
   silo_family[silo_family_qty].processor = processor;
   silo_family[silo_family_qty].seqnum    = seqnum;

   silo_family_qty++;

   free(temp_filename);
   return(OK);
}


char *SiloLib_Create_Filename(char *filename,
                              int  filetype,
                              int  cycle,
                              int  processor,
                              int  sequence_number)

/****************************************************************/
/*  Routine SiloLib_Create_Filename: Creates a valid domain     */
/*  sequenced                                                   */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  12-Jun-2005  IRC  Initial version.                          */
/*                                                              */
/****************************************************************/
{
   char *new_filename;
   char seq_num_ascii[10], cycle_string[20], processor_string[20];

   new_filename = (char *) malloc(256*sizeof(char));

   strcpy(new_filename, filename) ;

   if ( cycle>0 )
   {
      strcat(new_filename, "_") ;

      sprintf(cycle_string,    "%05d", cycle);
      sprintf(processor_string,"%05d", processor);

      strcat(new_filename, cycle_string) ;
      strcat(new_filename, "_") ;

      strcat(new_filename, processor_string) ;
      strcat(new_filename, "_") ;

      sprintf(seq_num_ascii,"%c_",sequence_number+40);
   }

   return (new_filename);
}

/* End SiloLib_Open_Restart_File */
