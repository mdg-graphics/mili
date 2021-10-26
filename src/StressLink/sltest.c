/**************************************************************************
 * test1.c
 * @author Rob Neely
 *
 * @purpose Used to perform testing of OverlinkToStresslink and
 * StresslinkToOverlink functions.
 *
 *
 * @param argc/argv
 *
 * @param
 *
 * @return status
 *
 *-------------------------------------------------------------------------
 *
 * Revision History
 *  ----------------
 *
 *  25-May-2007 JRN Initial version.
 **************************************************************************/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "socitume.h"
#include "string.h"

static void scan_args( int argc, char *argv[] );
static void get_input_options( char * );
static void usage( void );

/* Set default file names */
char overlink_file[128]="olinkfile", stresslink_file[128]="slinkfile";
char stresslink_file_in[128]="", stresslink_file_out[128]="";
char overlink_file_in[128]="",   overlink_file_out[128]="";

int  sl_loop_test=FALSE;
int  ol_loop_test=FALSE;
int  sl_ol       = FALSE;
int  ol_sl       = FALSE;
int  d_ol        = FALSE;
int  d_sl        = FALSE;

int main(int argc, char *argv[])
{
   int status;

   printf("\nRunning Pasit version: %s (%s)\n\n", PASIT_VERSION, PASIT_DATE);

   /* Parse command line arguments */
   scan_args( argc, argv );

   /* Get input options */
   get_input_options("PasitInit");

   /* Loop test for Stresslink - used for debugging Stresslink reader/writer */
   if ( sl_loop_test )
   {
      status = StresslinkToStresslink(stresslink_file_in, stresslink_file_out);
      return status;
   }

   /* Loop test for Overlink - used for debugging Overlink reader/writer */
   if ( ol_loop_test )
   {
      status = OverlinkToOverlink(overlink_file_in, overlink_file_out);
      return status;
   }

   /* Test one direction */
   if ( ol_sl )
   {
      status = OverlinkToStresslink(overlink_file_in, stresslink_file_out);
   }

   /* Test the other direction */
   if ( sl_ol )
   {
      status = StresslinkToOverlink(stresslink_file_in, overlink_file_out );
   }

   /* Dump SL file to textfile */
   if ( d_sl )
   {
      status = DumpStresslink(stresslink_file_in, "SLTEXT" );
   }

   /* Dump OL file to textfile */
   if ( d_ol )
   {
      status = DumpOverlink(overlink_file_in, "OLTEXT" );
   }
   return status;
}


/**************************************************************************
 * @Function: get_input_options
 * @author Bob Corey
 *
 * @purpose Read in any special options. Valid options include:
 *
 *   exclude-mat   1 2 3 4 end
 *   exclude-model 1 2 3 4 end
 *
 **************************************************************************/
static void
get_input_options( char *filename )
{
   char input_line[256], next_token[64];
   int  mat_num=0;
   int  i,j;
   int  len=0, line_index=0;

   FILE *fp=NULL;

   int exclude_mat=FALSE, exclude_model=FALSE;

   for (i=0;
         i<1000;
         i++)
      passit_mat_ignore_list[i]=FALSE;


   fp = fopen(filename, "r");
   if (!fp)
      return;

   printf("\t** Reading input options found in file '%s'\n\n", filename);

   while ( fgets( input_line, 256,  fp) )
   {
      len = strlen( input_line );
      if ( input_line[0]=='#' || input_line[0]=='*' )
         len=0;

      j=0;
      for (i=0;
            i<len;
            i++)
      {
         if (input_line[i]==' ')
         {
            next_token[j]='\0';

            if (!strcmp(next_token,"end"))
            {
               i=len;
               break;
            }

            if (!strcmp(next_token,"exclude_mat"))
            {
               exclude_mat=TRUE;
               j=0;
            }
            else
            {
               mat_num = atoi(next_token);
               if (mat_num>0 && mat_num<1000)
                  passit_mat_ignore_list[mat_num]=TRUE;
               j=0;
            }
         }
         next_token[j++]=input_line[i];
      }

   }
   fclose(fp);
}


/**************************************************************************
 * @Function: scan_args
 * @author Bob Corey
 *
 * @purpose Parse the driver program's command line arguments.
 **************************************************************************/
static void
scan_args( int argc, char *argv[] )
{
   int i;
   char *fontlib;
   time_t time_int;
   struct tm *tm_struct;
   int loop_test_selected=FALSE;

   char *name;

   for ( i = 1;
         i < argc;
         i++ )

      /* Test */
      if ( strcmp( argv[i], "-ol_loop" ) == 0 ||
            strcmp( argv[i], "-ol_ol" )   == 0 )
      {
         ol_loop_test = TRUE;
         loop_test_selected=TRUE;
      }
      else if ( strcmp( argv[i], "-sl_loop" ) == 0 ||
                strcmp( argv[i], "-sl_sl" )   == 0 )
      {
         sl_loop_test = TRUE;
         loop_test_selected=TRUE;
      }
      else if ( strcmp( argv[i], "-ol_sl" )   == 0 )
      {
         ol_sl = TRUE;
         loop_test_selected=FALSE;
      }
      else if ( strcmp( argv[i], "-sl_ol" ) == 0 )
      {
         sl_ol = TRUE;
         loop_test_selected=FALSE;
      }
      else if ( strcmp( argv[i], "-d_ol" ) == 0 )
      {
         d_ol = TRUE;
      }
      else if ( strcmp( argv[i], "-d_sl" ) == 0 )
      {
         d_sl = TRUE;
      }

   /* Overlink */
      else if ( strcmp( argv[i], "-O" ) == 0 )    /* Overlink */
      {
         i++;
         strcpy( overlink_file, argv[i] );
      }
      else if ( strcmp( argv[i], "-S" ) == 0 )    /* Stresslink */
      {
         i++;
         strcpy( stresslink_file, argv[i] );
      }
      else if ( strcmp( argv[i], "-Si" ) == 0 )   /* Stresslink In */
      {
         i++;
         strcpy( stresslink_file_in, argv[i] );
         sl_ol = TRUE;
      }
      else if ( strcmp( argv[i], "-So" ) == 0 )   /* Stresslink Out */
      {
         i++;
         strcpy( stresslink_file_out, argv[i] );
      }
      else if ( strcmp( argv[i], "-Oi" ) == 0 )   /* Overlink In */
      {
         i++;
         strcpy( overlink_file_in, argv[i] );
         ol_sl = TRUE;
      }
      else if ( strcmp( argv[i], "-Oo" ) == 0 )   /* Overlink Out */
      {
         i++;
         strcpy( overlink_file_out, argv[i] );
      }
      else if ( strcmp( argv[i], "-a" ) == 0 )   /* Ignore - used for Totalview */
      {
      }
      else if ( (strcmp( argv[i], "-h" ) == 0 ||
                 strcmp( argv[i], "-help" ) == 0 ) ||
                argc==1                           ||
                !loop_test_selected
              )                                  /* Help */
      {
         usage();
         exit(1);
      }
}



/**************************************************************************
 * @Function: usage
 * @author Bob Corey
 *
 * @purpose Displays driver's command line options and syntax.
 **************************************************************************/
static void
usage( void )
{
   static char *usage_text[] =
   {
      "\nUsage:  pasit_test1 Options -O overlink_filename -S stresslink_filename \n\n",
      " Loop Test Options (REQUIRED):\n",
      "            [-ol_loop | -ol_ol]        # Test OL->OL                #\n",
      "            [-sl_loop | -sl_sl]        # Test SL->SL                #\n",
      "            [-sl_ol]                   # Test SL->OL Mapping        #\n",
      "            [-ol_sl]                   # Test OL->SL Mapping        #\n",
      "            [-d_ol]                    # Dump OL to Text file       #\n",
      "            [-d_sl]                    # Dump SL to Text file       #\n",
      "\n",
      "            ---------------------------------------------------------\n",
      "\n",
      " File Options:\n",
      "            [-O]                       # name of Overlink file      #\n",
      "            [-S]                       # name of Stresslink file    #\n",
      "            [-Si]                      # name of Stresslink input file #\n",
      "            [-So]                      # name of Stresslink output file #\n",
      "            [-Oi]                      # name of Overlink input file dir #\n",
      "            [-Oo]                      # name of Overlink output file dir #\n",
      "            [-h]                       # print help - usage         #\n",
      "\n",
      "            ---------------------------------------------------------\n",
      " Examples:\n",
      "            pasit_test1 -sl_loop -Si mapping_demo2.HDFlink -So SLOUT\n",
      "            pasit_test1 -sl_loop -Si mapping_demo2.HDFlink -So SLOUT\n",

   };

   int i, line_cnt;

   line_cnt = sizeof( usage_text ) / sizeof( usage_text[0] );
   for ( i = 0;
         i < line_cnt;
         i++ )
      printf( "%s", usage_text[i] );

   return;
}
