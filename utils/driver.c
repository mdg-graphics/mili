/*
 * driver.c
 *
 *      Lawrence Livermore National Laboratory
 *
 */

/*
*****************************************************************************
* Modifications:
*
*  I. R. Corey - October 2,  2006: Added functions for merging TI files.
*  I. R. Corey - June    20, 2008: Perform sanity check of input variables.
*
*  I. R. Corey - April   29, 2009: Fixed bug with combining TH databases
*                        when appending.
*                        SCR#: 600
*
*  K. Durrenberger - Sept 08, 2009: Changed the file checking to fix a bug
*                        where the person combines a file using a partition
*                        file in the local directory and then tries to combine
*                        again. The TI and A files were mismatched in count
*                        were failing.
*
*  I. R. Corey - November 16, 2009:  Added combining and writing labels
*                        for meshless elements (ML).
*
*  I. R. Corey - March 12, 2010: Added capability to combine selected
*                        processors or materials.
*                        SCR#: 673
*
*  K. Durrenberger - May 17, 2010: Added a check when we open the directory
*                        it is valid. If not we exit. This was causing a core
*                        dump.
****************************************************************************
*/

/* #define DEBUG 1 */

#include "driver.h"
/* Added to allow for more than 1020 open file descriptors */
#define NEW_MAX (1024*8)

extern Mili_family **fam_list;

Environ env;

Database_type db_type;

static int wait_time = 300;


int get_max_state( Mili_analysis *analy );
int load_ti_nprocs( Mili_analysis *in_db );
int load_ti_labels( Mili_analysis **in_db, int nproc, TILabels *labels );
int build_cmap_from_labels( Mili_analysis **in_db, int nproc, TILabels *labels );
void VersionInfo(void);
int file_select(struct dirent   *entry);
int ti_file_select(struct dirent   *entry);
void set_timesteps(Mili_analysis *in_db, Mili_analysis *out_db ,int *
                   start_state , int *stop_state);


Bool_type xmilics_labels_found = FALSE;

void delete_labels(TILabels *labels){
   if(labels->subrec_contributions != NULL) {
      free(labels->subrec_contributions);
   }
   Label* iter,*temp;
   iter = labels->labels;
   while(iter != NULL) {
      
      if(iter->num_per_processors!= NULL) {
         free(iter->num_per_processors);
      }
      if(iter->offset_per_processor!= NULL) {
         free(iter->offset_per_processor);
      }
      if(iter->labels!= NULL) {
         free(iter->labels);
      }
      if(iter->map!= NULL) {
         free(iter->map);
      }
      temp = iter->next;
      free(iter);
      iter = temp;
   }
}
/************************************************************
 * TAG( usage )
 *
 * Parse a list of processor or material numbers and load
 * proc or material array with list of selected items.
 */
Return_value
parse_procmat_select_list( char *list_string, Bool_type procmat,
                           int  select_len,   short *select_list )
{
   int i, j;
   int begin, end;
   int len_list_string=0;
   char num_string[32];
   Bool_type convert_num=FALSE, convert_num_range=FALSE;

   int  num_index=0, procmat_num_index=0;
   int *procmat_nums;
   char *list_string_temp;

   len_list_string  = strlen( list_string );
   list_string_temp =  malloc( (len_list_string+10) * sizeof( char ) );
   procmat_nums = (int *) malloc( len_list_string * sizeof( int ) );

   /* Add , at end of input string */
   for ( i=0;
         i<=len_list_string;
         i++ ) {
      list_string_temp[i] = list_string[i];
   }
   if ( list_string_temp[len_list_string-1] == ',' ) {
      list_string_temp[len_list_string] = ' ';
   } else {
      list_string_temp[len_list_string] = ',';
   }

   for ( i=0;
         i<=len_list_string;
         i++ ) {
      if (isdigit( list_string_temp[i] )) {
         for ( j=i;
               j<=len_list_string;
               j++ ) {
            if ( list_string_temp[j] == '-' ) {
               convert_num_range = TRUE;
               break;
            }
            if ( list_string_temp[j] == ',' ) {
               convert_num = TRUE;
               break;
            }
            num_string[num_index++] = list_string_temp[j];
            i++;

         }
         if ( convert_num_range && !convert_num ) {
            num_string[num_index] = '\0';
            num_index = 0;
            procmat_nums[procmat_num_index++] = atoi( num_string );
         }
         if ( convert_num_range && convert_num ) {
            num_string[num_index] = '\0';
            num_index = 0;
            procmat_nums[procmat_num_index++] = atoi( num_string );
            convert_num_range = FALSE;
            convert_num = FALSE;
         }
         if ( !convert_num_range && convert_num ) {
            num_string[num_index] = '\0';
            num_index = 0;
            procmat_nums[procmat_num_index++] = atoi( num_string );
            procmat_nums[procmat_num_index++] = atoi( num_string ); /* Convert a single number to a range
									  * i.e. 10 -> 10-10
									  */
            convert_num = FALSE;
         }
      }
   }

   /* Convert block to list of numbers */
   for ( i=0;
         i<procmat_num_index;
         i++ ) {

      if ( procmat==MAT ) {
         if ( procmat_nums[i]<0 || procmat_nums[i]>=MAX_MAT ) {
            fprintf(stderr, "\n\tError - Invalid material selected = %d",procmat_nums[i]);
            exit( 1 );
         }
      }
      if ( procmat==PROC ) {
         if ( procmat_nums[i]<0 || procmat_nums[i]>=MAX_PROC ) {
            fprintf(stderr, "\n\tError - Invalid processor selected = %d",procmat_nums[i]);
            exit( 1 );
         }
      }

      for ( j=procmat_nums[i]-1;
            j<=procmat_nums[i+1]-1;
            j++ ) {
         select_list[j] = 1;
         if ( procmat == PROC ) {
            env.num_selected_procs++;
         } else {
            env.num_selected_mats++;
         }
      }
      i++;
   }
   free (list_string_temp );
   free( procmat_nums );
   return ( NOT_OK );
}

int
main( int argc, char *argv[] )
{
#if TIMER
   clock_t start_time,stop_time;
   double cumalative=0.0;
   double total_time = 0.0;
   start_time = clock();
   static struct timeb wc1, wc2;
#endif
   Mili_analysis **in_db, **temp_db;
   Mili_analysis **tmp_db;
   Mili_analysis **out_db;
   int in_dbid;
   env.hsp_done=FALSE;
   int proc, nprocs;
   FILE *p_f;
   Bool_type done, file_exists, status;
   int num_states;
   int start;
   int max_num_states;
   int i,j;
   char answer[4] = " ";
   char file_name[100];
   char elorder[32];

   int ret;
   struct rlimit rl;
   /* TI variables */
   Bool_type alloc_mem=FALSE;

   TILabels labels;
   labels.subrec_contributions= NULL;
   labels.dimensions = 0;
   char ti_particle_name[256], particle_class[256];

   char select_list[512];

   int mat_id=0, qty_mats=0, qty_mats_found=0;

   /* Clear out the env struct just in case. */
   memset( &env, 0, sizeof( Environ ) );

   /* Print Header */
   fprintf(stderr, "\n\n");                                               ;
   fprintf(stderr, "\n\t Running Xmilics Version: %s(%s)", PACKAGE_VERSION,PACKAGE_DATE);
   fprintf(stderr, "\n\n");

   /* Added on 12/21/2009 to accomadate over 1024 file count */
   ret = getrlimit(RLIMIT_NOFILE, &rl);
   if(ret != 0) {
      fprintf(stderr, "Unable to read open file limit.\n"
              "(getrlimit(RLIMIT_NOFILE, &rl) failed)\n"
              "(%d, %s)", errno, strerror(errno));
      fprintf(stderr, "Running with default 1020 filedescriptors.\n");
   } else {
      rl.rlim_cur = rl.rlim_max = NEW_MAX;
      ret = setrlimit(RLIMIT_NOFILE, &rl);
      if(ret != 0) {
         fprintf(stderr, "Unable to set open file limit.\n"
                 "(setrlimit(RLIMIT_NOFILE, &rl) failed)\n"
                 "(%d, %s)", errno, strerror(errno));
         fprintf(stderr, "Running with default 1020 filedescriptors.\n");
      } else {
         ret = getrlimit(RLIMIT_NOFILE, &rl);
         if(ret != 0) {
            fprintf(stderr, "Unable to read new open file limit.\n"
                    "(getrlimit(RLIMIT_NOFILE, &rl) failed)\n"
                    "(%d, %s)", errno, strerror(errno));
            fprintf(stderr, "Running with default 1020 filedescriptors.\n");
         }
      }
   }

   /* Scan command-line arguments, other initialization. */
   env.selected_mat_list  = (short *) calloc( MAX_MAT ,  sizeof(short) );
   env.selected_proc_list = (short *) calloc( MAX_PROC , sizeof(short) );

   scan_args( argc, argv );
   if(env.wait) {
      wait_for_start(env.input_file_name);
      if(env.pad ==0) {
         find_pad_count(env.input_file_name);
      }
   }

   /* Start time */
   manage_timer( 0 );
   Bool_type known_type;
   pf_name( env.input_file_name, 0, file_name );
   if(strlen(file_name) == 0) {
      printf("\n\tXmilics was unable to locate any processor files\n\tfor %s\n",env.input_file_name );
      exit(1);
   }
   /*Wait for them to write their first state file*/
   if(env.wait) {
      if(!wait_for_first_state()) {
         printf("\n\tTimed out awaiting first state to be written.\n");
         exit(1);
      }
   }
   known_type = is_known_db(file_name,&db_type);

   if (!known_type) {
      fprintf(stderr, "\n\tFailed to find file:   %s\n",env.input_file_name);
      exit( 1 );
   }

   if( db_type== TAURUS_DB_TYPE) {
   
      fprintf(stderr, "\n\t****************************************************");
      fprintf(stderr, "\n\t*                                                  *");
      fprintf(stderr, "\n\t*          Taurus DataBase are deprecated.         *");
      fprintf(stderr, "\n\t*          If you need to use a taurus DB          *");
      fprintf(stderr, "\n\t*          please use a version of the combiner    *");
      fprintf(stderr, "\n\t*          prior to 12.1 .                         *");
      fprintf(stderr, "\n\t*                                                  *");
      fprintf(stderr, "\n\t****************************************************\n\n");
      
   } else if(db_type== MILI_DB_TYPE) {
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time prior to starting combine is: %f\n",cumalative);
   start_time = clock();
#endif   
      char  directory[MAXPATHLEN];
      char* loc = strrchr(env.input_file_name,'/');
      int stop_state,start_state;
      if (loc) {
         strcpy(env.plotfile_name, loc+1);

         int length = strlen(env.input_file_name) -strlen(loc);
         strncpy(directory,env.input_file_name,length);
         directory[length]='\0';
      } else {
         strcpy(env.plotfile_name, env.input_file_name);
         if (getcwd(directory,MAXPATHLEN) == NULL) {
            fprintf(stderr, "\n\t* Directory could not open current working directory" );
         }
      }

      struct dirent **namelist;
      struct dirent **ti_namelist;

      int an,tin;

      /* Scan for the "A" files */
      an = scandir(directory, &namelist, (void *)file_select, alphasort);
      if (an == 0) {
         fprintf(stderr, "\n\t**************************************************");
         fprintf(stderr, "\n\t* A file count =%d                              *", an);
         fprintf(stderr, "\n\t* Exiting due to error reading file      *" );
         fprintf(stderr, "\n\t**************************************************\n");
         exit(-1);
      } else if (an < 0) {
         fprintf(stderr, "\n\t****************************************************");
         fprintf(stderr, "\n\t* Error reading A files                            *");
         fprintf(stderr, "\n\t* Exiting due to error reading partition directory *" );
         fprintf(stderr, "\n\t**************************************************\n");
         exit(-1);
      }
      
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time checking ti and A files  is: %f\n",cumalative);
   start_time = clock();
#endif 
      /* Combine  n -> 1 */

         fprintf(stderr, "\n\t****************************************************");
         fprintf(stderr, "\n\t*                                                  *");
         fprintf(stderr, "\n\t*              DataBase Combiner                   *");
         fprintf(stderr, "\n\t*            /* Combine  n -> 1 */                 *");
         fprintf(stderr, "\n\t*                                                  *");
         fprintf(stderr, "\n\t****************************************************\n\n");

         /* Get partition assignments from the partion file if available */
         if ( !env.ti_enabled ) {
            fprintf(stderr, "\n\t***************************************");
            fprintf(stderr, "\n\t*                                      ");
            fprintf(stderr, "\n\t* WARNING: : Use of partition files is ");
            fprintf(stderr, "\n\t*            no longer supported as of ");
            fprintf(stderr, "\n\t*            version 10.1 of Xmilics.  ");
            fprintf(stderr, "\n\t*                                      ");
            fprintf(stderr, "\n\t***************************************");

            exit(100);
         }

         /* Get number of processors from TI datafile */
         nprocs = an;
         if ( env.stop_proc<0 ) {
            env.stop_proc = nprocs;
         }
         
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time to get processors from TI file is: %f\n",cumalative);
   start_time = clock();
#endif 
         env.nprocs = nprocs;
         labels.num_procs = nprocs;
         if ( env.stop_proc<0 ) {
            env.stop_proc = env.nprocs;
         }

         out_db = NEW( Mili_analysis *,  "Pointer to Mili_analysis struct" );
         *out_db = NEW( Mili_analysis, "Mili_analysis struct" );

         file_exists = FALSE;
         done = FALSE;

         sprintf( file_name, "%sA", env.output_file_name);
         p_f = fopen( file_name, "r" );

         if( p_f != NULL && !env.batch_overwrite) {
            /* Output file exits */
            file_exists = TRUE;
            fprintf(stderr, "\n\t****************************************************");
            fprintf(stderr, "\n\t*                                                  *");
            fprintf(stderr, "\n\t*            WARNING!  WARNING!                    *");
            fprintf(stderr, "\n\t*                                                  *");
            fprintf(stderr, "\n\t*      Output file %s exists                        ",
                    env.output_file_name                        );
            fprintf(stderr, "\n\t*                                                  *");
            fprintf(stderr, "\n\t*                                                  *");
            fprintf(stderr, "\n\t****************************************************\n\n");

            if( env.batch || env.restart) {
               env.append = TRUE;
            } else {
               if (!env.newfile) {
                  fprintf(stderr, "\n\n");
                  do {
                     fprintf(stderr, "\t Do you want to append to existing files? <yes> or <no> ");
                     fgets( answer, 4, stdin );

                     if((strncmp( answer, "y", 1 ) == 0 )||(strncmp( answer, "n", 1 ) == 0 )) {
                        done = TRUE;
                     }
                  } while(!done);

                  if( strncmp( answer, "y", 1 ) == 0 ) {
                     env.append = TRUE;
                  }
               } else {
                  env.append = TRUE;
                  fprintf(stderr, "\n\t Overwriting Output file %s\n", env.output_file_name);
               }
            }
         }

         in_db = NEW_N( Mili_analysis *, env.nprocs,  "Pointer to Mili_analysis struct" );
         
         for( proc = 0; proc < env.nprocs; proc++ ) {
            in_db[proc] = NULL;
         }

         if ( env.matproc_selected ) {
            env.matproc_table = (short **) malloc( env.nprocs*sizeof(short *) );
            env.num_mats_proc = (int *)   malloc( env.nprocs*sizeof(int) );
            for ( i=0;
                  i<env.nprocs;
                  i++ ) {
               env.num_mats_proc[i] = 0;
            }
         }
         if(env.wait) {
            if(!check_all_state_files()) {
               fprintf(stderr,"Not all state files created\n");
               exit(1);
            }
            fprintf(stderr,"\n\t* Found all state files        *\n");
         }
       
         for( proc = 0;
               proc < env.nprocs;
               proc++ ) {
            if ( env.num_selected_procs>0 && !env.selected_proc_list[proc] ) {
               in_db[proc] = NULL;
               continue;
            }

            in_db[proc] = NEW( Mili_analysis,  "Mili_analysis struct" );
            in_db[proc]->root_name= NULL;
            in_db[proc]->state_times= NULL;
            in_db[proc]->result= NULL;
            pf_name( env.input_file_name, proc,  file_name );
#if TIMER

   time_t mt, st;
   ftime( &wc1 );
   start_time = clock();
#endif  
            status = open_input_dbase( file_name, in_db[proc] );
#if TIMER
   ftime( &wc2 );
   mt = wc2.millitm - wc1.millitm;
   if ( mt < 0 )
   {
      mt += 1000;
      st = wc2.time - wc1.time - 1;
   }
   else
   {
      st = wc2.time - wc1.time;
   }
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Start time: %d    Finish time: %d   Clocks per sec: %d\n",start_time,stop_time,CLOCKS_PER_SEC);
   fprintf(stderr,"Opening file Time is %s: %d.%03d\n",file_name,st,mt);
   
#endif
            if ( !status ) {
               fprintf(stderr, "\n\t***************************************");
               fprintf(stderr, "\n\t*                                      ");
               fprintf(stderr, "\n\t* WARNING: File Not Found:  %s         ", file_name );
               fprintf(stderr, "\n\t*                                      ");
               fprintf(stderr, "\n\t***************************************");
               exit( 1 );
            }

            //read_non_state_data( in_db[proc] );

            if ( env.num_selected_mats>0 ) {
               env.num_mats_proc[proc] = get_mat_list_for_proc( in_db, proc, (short **) &env.matproc_table[proc] );
            }
         }

         /* If user requested specific materials to combine -
          * select the processors to combine based on the
          * materials selected.
          */
         if ( env.matproc_selected ) {
            for ( i=0; i<MAX_MAT; i++ ) {
               if ( env.selected_mat_list[i]>0 ) {
                  mat_id = i+1;

                  /* Locate all processors for this material */
                  for ( proc=0; proc<env.nprocs; proc++ ) {
                     for ( j=0; j<env.num_mats_proc[proc]; j++ ) {
                        if ( mat_id == env.matproc_table[proc][j] ) {
                           env.selected_proc_list[proc] = 1;
                           qty_mats_found++; /* Keep a count of number of mats found on processors of
							    * those materials that were selected. It is possible that
							    * a user could select a set of materials that are not
							    * used for any elements in the model. This would be an
							    * error as there would be no elements to combine.
							    */
                        }
                     }
                  }
               }
            }

            if ( env.num_selected_mats>0 && qty_mats_found==0 ) {
               fprintf(stderr, "\n\tError - Materials selected are not used for any elements in the model.");
               exit(1);
            }

            /* Total the number of procs selected */
            env.start_proc = -1;
            for ( proc=0; proc<env.nprocs; proc++ ) {
               env.num_selected_procs = 0;
               if ( env.selected_proc_list[proc]>0 ) {
                  env.num_selected_procs++;
                  env.stop_proc = proc+1;
               } else {
                  if ( in_db[proc] ) {
                     free( in_db[proc] );
                     in_db[proc] = NULL;
                  }
               }
               env.num_selected_procs++;
               if ( env.start_proc<0 && env.selected_proc_list[proc]>0 ) {
                  env.start_proc = proc;
               }
            }
         }

         if( env.append ) {
            /*mc_restart_at_state();*/
            /* Open output file in append mode. */
            stop_state=0;
            mc_open( env.output_file_name, "./", "ad", &out_db[0]->db_ident );
            num_states = get_max_state( in_db[env.start_proc] );
            

            set_timesteps(in_db[env.start_proc],out_db[0],&start_state,&stop_state);

            if(start_state < 0 && !env.wait) {
               fprintf(stderr, "\n\t***************************************");
               fprintf(stderr, "\n\t*                                     *");
               fprintf(stderr, "\n\t*     Exiting. Nothing to append      *");
               fprintf(stderr, "\n\t*                                     *");
               fprintf(stderr, "\n\t***************************************\n");
               exit( 1 );
               /* I should do some clean up here */
            }

            if( env.stop_state ==0) {
               env.stop_state = stop_state;
            }
            if (stop_state >0 && stop_state < env.stop_state) {
               env.stop_state = stop_state;
            }
            if (env.start_state < start_state ) {
               env.start_state = start_state;
            }
            
            if(env.start_state>env.stop_state && !env.wait) {
               fprintf(stderr, "\n\t***************************************");
               fprintf(stderr, "\n\t*                                     *");
               fprintf(stderr, "\n\t*     Exiting. Nothing to append      *");
               fprintf(stderr, "\n\t*                                     *");
               fprintf(stderr, "\n\t***************************************\n");
               exit( 1 );
            }
            if(!env.wait){
               fprintf(stderr, "\n        *       Appending database  *\n");
               fprintf(stderr, "        *        start state = %d   *\n",env.start_state);
               fprintf(stderr, "        *        stop state= %d     *\n\n",env.stop_state);
            }


         } else {
            /* Open output plotfile and initialize database data structure. */
            status = open_output_dbase( env.output_file_name, out_db[0] );
            if ( !status ) {
               exit( 1 );
            }
            start = 0;
         }
                                    
         
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time setting mats and procs to select is: %f\n",cumalative);
   start_time = clock();
#endif          
         if ( env.ti_enabled ) {
            labels.labels = NULL;
            status = load_ti_labels( in_db, nprocs, &labels );
            if(status !=OK)
            {
               fprintf(stderr, "There was an error loading labels that XmiliCS could not recover from. Exiting .....\n");
               //exit(201);
            }
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time loading ti labels: %f\n",cumalative);
   start_time = clock();
#endif  
            if (status==NO_LABELS) {
               xmilics_labels_found = FALSE;
				}
				else if( status == OK){            
               xmilics_labels_found = TRUE;
               status = build_cmap_from_labels( in_db, nprocs, &labels );
				
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time creating global mapping: %f\n",cumalative);
   start_time = clock();
#endif  
            }else {
					fprintf(stderr, "There was an error loading the labels. Exiting program now."); 
					exit(200);
				}
         }

         if( (!file_exists) || (strncmp( answer, "n", 1 ) == 0) ) {
            
            combine_non_state_definitions(in_db, out_db[0], &labels);
#ifdef DEBUG
            status = dump_geom_data( out_db[0] );
#endif

         } else {
            get_subrec_contributions(in_db, out_db[0], &labels);
         }
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Combine non-state data time is: %f\n",cumalative);
   fprintf(stderr,"Total pre_statecombine time is: %f\n",total_time);
   
#endif

         if( env.states_per_file ) {
            mc_limit_states( out_db[0]->db_ident, env.n_states  );
         } else if(env.flsize) {
            mc_limit_filesize( out_db[0]->db_ident, env.flsize );
         }
         if(env.restart){
           int restart = wait_for_restart(in_db, env.start_state);
           if(restart==0 || restart==2){
              exit(1);
           }
         }

         if(!env.wait) {
#if TIMER
   fprintf(stderr,"Starting State combine\n");
   start_time = clock();
#endif
            max_num_states = 0;
            for( proc = env.start_proc; proc < env.stop_proc; proc++ ) {
               if ( env.num_selected_procs>0 && !env.selected_proc_list[proc] ) {
                  continue;
               }

               num_states = get_max_state( in_db[proc] );
               if( num_states >= max_num_states ) {
                  max_num_states = num_states;
               }
            }
            if ( env.stop_state==0 ) {
               env.stop_state = max_num_states;
            }
            if ( env.stop_state>num_states ) {
               env.stop_state = num_states;
            }
         
         
            if (env.start_state == 0) {
               env.start_state = 1;
            }
            env.current_state_max = env.stop_state;
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time finding max states is: %f\n",cumalative);
#endif
            for( i = env.start_state; i <= env.stop_state; i++ ) {

               for( proc = env.start_proc; proc < env.stop_proc; proc++ ) {
                  if ( in_db[proc] ) {
                     read_state_data( i, in_db[proc] );
#if 0
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time to read in results for state %d processor %d: %f\n",i,proc,cumalative);
   start_time = clock();
#endif
                     merge_state_data( proc, &labels, in_db[proc], out_db[0] );
#if 0
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time to combine results for state %d processor %d: %f\n",i,proc,cumalative);
   start_time = clock();
#endif
                  }
               }
               fprintf(stderr, " State %6d: Time = %1.6e\n",
                       i , out_db[0]->state_times[i-1] );
               write_state_data( i, out_db[0] );
#if TIMER
   stop_time = clock();
   cumalative =((double)(stop_time - start_time))/CLOCKS_PER_SEC;
   total_time += cumalative;
   fprintf(stderr,"Time to write results for state:%d is: %f\n",i,cumalative);
   start_time = clock();
#endif
            }
         }else{
           if (env.start_state == 0) {
                env.start_state = 1;
           }
           int current_state =  env.start_state;
           int run_to_state =0;
	        int stop_time = env.wait_time*60;
           int current =0;
           do{
             run_to_state = get_next_state(in_db);
             if(env.stop_state >0 && env.stop_state < run_to_state) {
                run_to_state =  env.stop_state;
             }else { 
                env.current_state_max = run_to_state;
	             if(current_state >= run_to_state) {
	                sleep(wait_time);
		             current +=wait_time;
		             if(current >= stop_time) {
		                break;
		             }
	             }
             }   
             for( i = current_state; i < run_to_state ; i++ ) {
	            for( proc = env.start_proc; proc < env.stop_proc; proc++ ) {
                  if ( in_db[proc] ) {
                     read_state_data( i, in_db[proc] );
                     merge_state_data( proc, &labels, in_db[proc], out_db[0] );
                  }
               }
               fprintf(stderr, " State %6d: Time = %1.6e\n",
                       i, out_db[0]->state_times[i-1] );
               write_state_data( i, out_db[0] );
               current_state++;
             }
             if(run_to_state == env.stop_state){
               break;
             }
           }while(!end_of_run());
           
           if(!run_to_state == env.stop_state){            
              run_to_state = get_next_state(in_db);
              if(run_to_state >= current_state){
                 env.current_state_max = run_to_state;
                 if(env.stop_state >0 && env.stop_state < run_to_state) {
                    run_to_state =  env.stop_state;
                 } 
                 for( i = current_state-1; i < run_to_state ; i++ ) {
                 
                    for( proc = env.start_proc; proc < env.stop_proc; proc++ ) {
                       if ( in_db[proc] ) {
                          read_state_data( i, in_db[proc] );
                          merge_state_data( proc, &labels, in_db[proc], out_db[0] );
                       }
                    }
                    fprintf(stderr, " State %6d: Time = %1.6e\n",
                            i, out_db[0]->state_times[i-1] );
                    write_state_data( i, out_db[0] );
                 }
              }
           }
         }
         // Time to clean up 
         close_dbase( out_db[0] ,1);
         free( out_db[0] );
         for( proc = env.start_proc; proc < env.stop_proc; proc++ ) {
            if ( in_db[proc] ) {
               close_dbase( in_db[proc] ,1);
               free( in_db[proc] );
            }
         }
         delete_labels(&labels);
         free( in_db );
         free( out_db );

         /* Tracking using the AX tracker tool. */
         manage_timer( 1 );

         fprintf(stderr, "\n\t****************************************************");
         fprintf(stderr, "\n\t*                                                  *");
         fprintf(stderr, "\n\t*                 DataBase Combiner                *");
         fprintf(stderr, "\n\t*              Successful Completion               *");
         fprintf(stderr, "\n\t*                                                  *");
         fprintf(stderr, "\n\t****************************************************\n\n");
      
   } else {
      fprintf(stderr, "\n\t******************ERROR*************************");
      fprintf(stderr, "\n\t*                                              *");
      fprintf(stderr, "\n\t*          Unknown Database Type               *");
      fprintf(stderr, "\n\t*                                              *");
      fprintf(stderr, "\n\t************************************************\n");
      exit(1);
   }

   exit( 0 );
}


/************************************************************
 * TAG( find_pad_count )
 *
 * Find the count of integers to fill the file name for searching
 * for files.
 */
void
find_pad_count(char* input_file_name)
{
   env.pad=0;
   char  directory[MAXPATHLEN];
   char* loc = strrchr(input_file_name,'/');
   if (loc) {
      strcpy(env.plotfile_name, loc+1);
      int length = strlen(input_file_name) -strlen(loc);
      strncpy(directory,input_file_name,length);
      directory[length]='\0';
   } else {
      strcpy(env.plotfile_name, input_file_name);
      directory[0]='.';
      directory[1]='\0';
   }
   DIR *dir = opendir(directory);
   if(dir == NULL) {
      fprintf(stderr, "****** ERROR *******\n\n");
      fprintf(stderr, "Invalid directory: %s\nExiting.\n\n",directory);
      fprintf(stderr, "****** ERROR *******\n\n");
      exit(1);
   }
   struct dirent *dirp;
   while ((dirp = readdir(dir)) != NULL) {
      int last_pos = strlen(dirp->d_name)-1;
      if(strstr(dirp->d_name,env.plotfile_name) != NULL &&
            dirp->d_name[last_pos] == 'A' && dirp->d_name[last_pos-1] != '_') {
         int pos = strlen(env.plotfile_name);/*last_pos-1;*/
         while(isdigit(dirp->d_name[pos]) && pos<last_pos) {
            env.pad++;
            pos++;
         }
         if( env.pad >0 && dirp->d_name[pos] =='A') {
            break;
         }
      }
   }
   closedir(dir);
}
int wait_for_restart(Mili_analysis ** anal, int current_state){
   int state,status=1,return_state= 0;
   int stop_time = env.wait_time*60;
   int current =0;
   char file_name[128];
   file_name[0]='\0';
   pf_name( env.input_file_name, 0,  file_name );
   
   do{         
      Mili_family *fam = fam_list[anal[0]->db_ident];
      state = fam->state_qty;
      if(state == current_state-1){
      }
      else if(state > current_state ){
         break;      
      }else if(end_of_run()){
         status=2;
         break;
      }
         
      sleep(wait_time);
         
      close_dbase( anal[0] ,0); 
      current +=wait_time;
        
      status = open_input_dbase( file_name, anal[0] );
      if(!status){
         break;
      }
      
   }while(current<stop_time);
   
   if(status == 2){
      printf("\n\t* Run ended with no additional time steps added  *\n");
   }
   else if (state < current_state){
      fprintf(stderr, "\n\t* Timed out awaiting restart   *\n");
      status = 0;
   }
   return status;
      
}
int
get_next_state(Mili_analysis ** anal){
  int state,
      status,
      i,
      return_state;
  char file_name[128];
  file_name[0]='\0';
  Mili_family *fam = fam_list[anal[0]->db_ident];
  state = fam->state_qty;
  close_dbase( anal[0] , 0);
  
  pf_name( env.input_file_name, 0,  file_name );
  status = open_input_dbase( file_name, anal[0] );
  
  fam = fam_list[anal[0]->db_ident];
  return_state = fam->state_qty;
  if(state < fam->state_qty){
     /* We will need to cycle over all the file until they are 
        at least at the new state*/
     short *temp = (short*)calloc(env.nprocs, sizeof(short));
     int updated=0;
     state = fam->state_qty;
     while(!updated){
        updated = 1;
        for( i = 1; i<env.nprocs ; i++){
          
           if(!temp[i] && anal[i]){
             
              close_dbase(anal[i],0);
              pf_name( env.input_file_name, i,  file_name );
              status = open_input_dbase( file_name, anal[i] );
              fam = fam_list[anal[i]->db_ident];
              if(state <= fam->state_qty){
                
                temp[i] = 1;
              }else{
                
                 updated = 0;
              }               
           }
           
        }
     }
     
  }
  return return_state;
}

int 
end_of_run(){
  
  int term_count=0, restart_count=0;
  FILE * checkfile;
  char * pointer;
  char buffer[128];
  int length;
  int last_restart_position=-1, last_termination_position=0;
  int line = 0;
  
  if(!env.hsp_done){
     env.hsp_file_name[0] = '\0';
     char buffer[128];
     pointer = strstr(env.input_file_name,".plt");
     if(pointer>0){
        length = strlen(env.input_file_name) - strlen(pointer);
        strncat(env.hsp_file_name,env.input_file_name,length);
     }else{
        pointer = strstr(env.input_file_name,".th");
        if(pointer>0){
           length = strlen(env.input_file_name) - strlen(pointer);
           strncat(env.hsp_file_name,env.input_file_name,length);
        }else {
           strcat(env.hsp_file_name,env.input_file_name);
        }
     }
     strcat(env.hsp_file_name,".hsp");
     env.hsp_done=TRUE;
  }
  
  checkfile = fopen(env.hsp_file_name,"rt");
  if(checkfile != NULL){
     while(!feof(checkfile)){
        if(fgets(buffer,126,checkfile)){
          line +=1;
          if(strstr(buffer, "n o r m a l    t e r m i n a t i o n")){
             term_count +=1;
             last_termination_position= line;
          }
          if(strstr(buffer, "RESTART RUN")){
             last_restart_position =line;
          }
        }
     }
     fclose(checkfile);
  }
  
  if(last_restart_position>0){
    
    return(last_termination_position > last_restart_position);
  }
  
  return (term_count >= 1) ; 
}



int
check_all_state_files()
{
   char file_name[128];
   int stop_time = env.wait_time*60;
   int current =0;
   int proc;
   FILE *test_file;
   while(current < stop_time) {
      for(proc=0; proc<env.nprocs; proc++) {
         pf_name(env.input_file_name,proc,file_name);
         strcat(file_name,"00");
         if((test_file = fopen(file_name,"r")) == NULL) {
            break;
         }
         //fclose(test_file);
      }
      if(proc >= env.nprocs) {
         return 1;
      }
      sleep(wait_time);
      current +=wait_time;
   }
   return 0;
}
int
wait_for_first_state()
{
   char outfile_name[128];
   pf_name(env.input_file_name,0,outfile_name);
   strcat(outfile_name,"00");
   int stop_time = env.wait_time*60;
   int current =0;
   FILE * check_file;
   while(current < stop_time) {
      if((check_file = fopen(outfile_name,"r"))!= NULL) {
         break;
      }
      sleep(wait_time);
      current +=wait_time;
   }
   return (current < stop_time);

}
int
wait_for_start(char * input_file_name)
{
   int diff = 0;
   int pad=0;
   char  directory[MAXPATHLEN];
   char* loc = strrchr(input_file_name,'/');
   if (loc) {
      strcpy(env.plotfile_name, loc+1);
      int length = strlen(input_file_name) -strlen(loc);
      strncpy(directory,input_file_name,length);
      directory[length]='\0';
   } else {
      strcpy(env.plotfile_name, input_file_name);
      directory[0]='.';
      directory[1]='\0';
   }
   DIR *dir = NULL;//= opendir(directory);
   
   struct dirent *dirp;
   int wait_limit = env.wait_time * 60 ;
   while(diff <= wait_limit) {
      dir = opendir(directory);
      while ((dirp = readdir(dir)) != NULL) {
         int last_pos = strlen(dirp->d_name)-1;
         if(strstr(dirp->d_name,env.plotfile_name) != NULL &&
            dirp->d_name[last_pos] == 'A' && dirp->d_name[last_pos-1] != '_') {
            int pos = strlen(env.plotfile_name);/*last_pos-1;*/
            while(isdigit(dirp->d_name[pos]) && pos<last_pos) {
               pad++;
               pos++;
            }
            if( pad >0 && dirp->d_name[pos] =='A') {
               break;
            }
         }
      }
      if(pad) {
         break;
      }
      closedir(dir);
      dir = NULL;
      sleep(wait_time);
      diff += wait_time;
   }
   
   if(dir != NULL){
      closedir(dir);
   }
   if(diff > wait_limit){
      printf("\n\t* Timed out awaiting output of plot files *\n"); 
   }
   return pad;
}


/************************************************************
 * TAG( scan_args )
 *
 * Parse the program's command line arguments.
 */
static void
scan_args( int argc, char *argv[] )
{
   int i;
   int inname_set;
   int outname_set = 0;
   time_t time_int;
   struct tm *tm_struct;
   char *name;
   inname_set = FALSE;
   int start_stop = 0;
   char seqnum_char[32];

   Return_value rval;

   if (argc < 2 ) {
      usage();
      exit( 0 );
   }

   /* Set defaults */
   env.nprocs = 1;
   env.nmats  = 0;
   env.num_selected_procs = 0;
   env.num_selected_mats  = 0;

   env.combine = FALSE;
   env.split = FALSE;
   env.states_per_file = FALSE;

   /* default file size of 10 Mb */
   env.flsize = 0;

   env.n_states = 1;
   env.start_state = 0;
   env.stop_state  = 0;

   env.start_proc = 0;
   env.stop_proc  = -1;
   env.num_selected_procs = 0;

   env.start_mat  = -1;
   env.stop_mat   = -1;
   env.num_selected_mats = 0;

   env.force_partition = FALSE;
   env.ti_enabled  = TRUE;
   env.append  = FALSE;
   env.newfile = FALSE;
   env.batch   = FALSE;
   env.proc_seqnum  = -1;
   env.matproc_selected = FALSE;
   env.wait_time = 30;
   env.restart = 0;

   for ( i = 1; i < argc; i++ ) {
      if ( strcmp( argv[i], "-i" ) == 0 ) {
         /* Olga's very first bug fix */
         /* If i >= argc, then Griz just dumps core    */
         /* User's don't like this and so now we check */
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         strcpy( env.input_file_name, argv[i] );
         find_pad_count(env.input_file_name);
         inname_set = TRUE;
      } else if ( strcmp( argv[i], "-o" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         strcpy( env.output_file_name, argv[i] );
         outname_set = TRUE;
      }

      else if ( strcmp( argv[i], "-n" ) == 0 ) {
         /* This will be deprecated and not needed in the future */

         fprintf(stderr, "The -n option is deprecated and is not needed.");
         i++;

         if(i >= argc) {
            usage();
            exit(0);
         }
         env.nprocs = atoi( argv[i] );
      }

      else if ( strcmp( argv[i], "-c" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         strcpy( env.partfile_name_c, argv[i] );
         fprintf(stderr, "\tThe -c option is deprecated as of Version 10.1.\n\tXmilics will ignore -c input.\n");
         env.combine = FALSE;
         env.force_partition = FALSE;

      } else if ( strcmp( argv[i], "-l" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.flsize = atoi( argv[i] );
      } else if ( strcmp( argv[i], "-s" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.states_per_file = TRUE;
         env.n_states = atoi( argv[i] );
      } else if ( strcmp( argv[i], "-start" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.start_state = atoi( argv[i] );
         if(env.start_state < 0) {
            env.start_state=1;
         }
         start_stop = 1;
      } else if ( strcmp( argv[i], "-stop" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.stop_state = atoi( argv[i] );
         if(env.stop_state <0) {
            env.stop_state=0;
         }
         start_stop = 1;
      } else if ( strcmp( argv[i], "-proc" ) == 0 || strcmp( argv[i], "-procs" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.matproc_selected = TRUE;
         rval = parse_procmat_select_list( argv[i], PROC, MAX_PROC, env.selected_proc_list );
      } else if ( strcmp( argv[i], "-pstart" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.start_proc = atoi( argv[i] )-1;

         if(env.start_proc < 0) {
            env.start_proc=1;
         }
      } else if ( strcmp( argv[i], "-pstop" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.stop_proc = atoi( argv[i] )-1;

         if(env.stop_proc <0) {
            env.stop_proc = 0;
         }
      } else if ( strcmp( argv[i], "-mat" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.matproc_selected = TRUE;
         rval = parse_procmat_select_list( argv[i], MAT, MAX_MAT, env.selected_mat_list );
      } else if ( strcmp( argv[i], "-mstart" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.start_mat = atoi( argv[i] )-1;
         env.stop_mat  = env.start_mat;

         if(env.start_mat < 0) {
            env.start_mat=1;
         }
      } else if ( strcmp( argv[i], "-mstop" ) == 0 ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.stop_mat = atoi( argv[i] )-1;
         env.stop_mat = env.stop_mat;

         if(env.stop_mat <0) {
            env.stop_mat = 0;
         }
      } else if ( (strcmp( argv[i], "-append" ) == 0) && !start_stop ) {
         i++;
         if(i >= argc) {
            usage();
            exit(0);
         }
         env.stop_state = atoi( argv[i] );
         if(env.stop_state <0) {
            env.stop_state=0;
         }
         env.start_state=env.stop_state;
      } else if ( strcmp( argv[i], "-batch_overwrite" ) == 0 ) {
         /* Enable batch mode */
         env.batch_overwrite = TRUE;
      } else if ( strcmp( argv[i], "-batch" ) == 0 ) {
         /* Enable batch mode */
         env.batch = TRUE;
      } else if ( strcmp( argv[i], "-wait" ) == 0 ) {
         /* Enable batch mode */
         env.wait = TRUE;
         if(i<argc-1 && argv[i+1][0] != '-') {
            i++;
            env.wait_time = atoi(argv[i]);
         }
      } else if ( strcmp( argv[i], "-pt" ) == 0 ) {
         /* Enable batch mode */
         env.wait = TRUE;
         if(i<argc-1 && argv[i+1][0] != '-') {
            i++;
            wait_time = atoi(argv[i]);
         }
      } else if ( strcmp( argv[i], "-restart" ) == 0 ) {
         /* Enable restart ode */
         env.restart = TRUE;
         
      } else if ( strcmp( argv[i], "-newfile" ) == 0 ) {
         env.newfile = TRUE;
      } else if ( strcmp( argv[i], "-seqnum" ) == 0 ) {
         i++;
         env.proc_seqnum = atoi( argv[i] );
      } else if ( strcmp( argv[i], "-V" ) == 0 ) {
         VersionInfo();
         if( argc == 2 ) {
            exit(0);
         }
      }
   }

   /* Perform sanity check of input variables */
   if (env.force_partition && !env.combine && !env.split) {
      printf("You need to include a partition file if you are using -c \n" );
      usage();
      exit(1);
   }
   if (!inname_set ) {
      usage();
      exit( 0 );
   }
   if (env.nprocs==0 && !env.combine) {
      usage();
      exit( 0 );
   }

   /* Get today's date. */
   time_int = time( NULL );
   tm_struct = gmtime( &time_int );
   strftime( env.date, 19, "%D", tm_struct );

   /* Get user's name. */
   name = getenv( "LOGNAME" );
   strcpy( env.user_name, name );
   if (!outname_set) {
      char temp[80]="" ;
      char* loc = strrchr(env.input_file_name,'/');
      if (loc>0) {
         strcpy(temp, loc+1);
      } else {
         strcpy(temp, env.input_file_name);
      }
      strcat( temp,"_c");
      strcpy( env.output_file_name, temp);
   }
   /* Perform sanity check of input variables */

   /* Append sequence number to output filename if requested */
   if ( env.proc_seqnum >=0 ) {
      sprintf( seqnum_char, "%03d", env.proc_seqnum );
      strcat( env.output_file_name, seqnum_char );
   }
}

/************************************************************
 * TAG( file_select )
 *
 * Used by scandir to detect the Mili "A" files in a directory.
 */
int
file_select(struct dirent   *entry)
{
   int start_check;
	int found_A = 0;
	if ((strcmp(entry->d_name, ".")== 0) ||
         (strcmp(entry->d_name, "..") == 0)) {
      return (FALSE);
   }

   /* Check for filename extensions */
   int cptr = strlen(env.plotfile_name);
   int ptr = strlen(entry->d_name);
   if ((strncmp(env.plotfile_name,entry->d_name,strlen(env.plotfile_name)) ==0)
         &&(entry->d_name[ptr-1]== 'A') 
         && (entry->d_name[ptr-2] >='0' && entry->d_name[ptr-2] <='9')
         && !(entry->d_name[ptr-2] == '_')
         && !((entry->d_name[cptr] == '_') && (entry->d_name[cptr+1] == 'c'))) {
		
		
		for(start_check = strlen(env.plotfile_name); start_check< strlen(entry->d_name) && !found_A; start_check++){
			if(entry->d_name[start_check] == 'A' && start_check == strlen(entry->d_name)-1 ){
				found_A=1;
				continue;
			}
			
			if(entry->d_name[start_check]<'0' || entry->d_name[start_check]>'9'){
				break;
			}
		}
		if(found_A){
      	return (TRUE);
		}else{
			return (FALSE);
		}
   } else {
      return(FALSE);
   }
}


/************************************************************
 * TAG( ti_file_select )
 *
 * Used by scandir to detect the ti files in a directory.
 */
int
ti_file_select(struct dirent   *entry)
{
   if ((strcmp(entry->d_name, ".")== 0) ||
         (strcmp(entry->d_name, "..") == 0)) {
      return (FALSE);
   }

   /* Check for filename extensions */
   int cptr = strlen(env.plotfile_name);
   int ptr = strlen(entry->d_name);
   if ((strncmp(env.plotfile_name,entry->d_name,strlen(env.plotfile_name)) ==0)
         && !((entry->d_name[cptr] == '_') && (entry->d_name[cptr+1] == 'c'))
         && (entry->d_name[ptr-1]== 'A')
         && (entry->d_name[ptr-2] == '_')
         && (entry->d_name[ptr-3] == 'I')
         && (entry->d_name[ptr-4] == 'T')
         && !((entry->d_name+strlen(env.plotfile_name))[0] =='_'
              && (entry->d_name+strlen(env.plotfile_name))[1] =='c')) {
      return (TRUE);
   } else {
      return(FALSE);
   }
}


/*****************************************************************
 * TAG( pf_name )
 *
 * Get the plot file name for the given file number.
 *
 *****************************************************************/
static void
pf_name( char *root_name, int fnum, char *fname )
{
   if(env.pad == 0) {
      fname[0] ='\0';
   } else {
      sprintf( fname, "%s%0*d", root_name, env.pad, fnum );
   }
   return;
}


/************************************************************
 * TAG( usage )
 *
 * Write out command-line syntax.
 */
static void
usage( void )
{

   printf("\n" );
   printf("Usage: \n" );
   printf("  xmilics -i <input_base_name> [-o <output_base_name>]\n\n");
   printf("  ###    Read serial Taurus or parallel Mili database, ###\n");
   printf("  ###    Write serial Mili database                    ###\n");
   printf("\n");
   printf("OPTIONS:\n\n");
   printf("  [-o]  <output file name>\n");
   printf("  ###   Create output file using this name.           ###\n");
   printf("  ###   default output file name: {input_base_name}_c ###\n\n");
   printf("\n");
   /*
   printf("  ****deprecated as of version 10.1 ***\n");
   printf("  [-c]  <input_partition_file_name>\n");
   printf("  ###    Use specified partition file to combine  ###\n");
   printf("  ###    plot files. Disables using the TI files  ###\n");
   printf("  ###    Write Mili database                      ###\n");
   printf("  ****deprecated as of version 10.1 ***\n");
   printf("\n");
   */
   printf("  [-l]  <file size in megabytes>\n");
   printf("  ###    Limit file size of combined database     ###\n");
   printf("\n");
   printf("  [-s]  <number of states per file>\n");
   printf("  ###    Limit number of states per file in       ###\n");
   printf("  ###    combined database                        ###\n");
   printf("  ###    NOTE:  the -s option will override       ###\n");
   printf("  ###    the -l option                            ###\n");
   printf("\n");
   printf("  [-start]  <state to begin at>\n");
   printf("  ###    Limit states per file with non-zero      ###\n");
   printf("  ###    starting state. When appending this is   ###\n");
   printf("  ###    set to the last time of output file if   ###\n");
   printf("  ###    starting state is less than the end      ###\n");
   printf("  ###    state of the output database.            ###\n");
   printf("\n");
   printf("  [-stop]  <state to end at>\n");
   printf("  ###    Limit states per file with an ending     ###\n");
   printf("  ###    states. This is ignored if less the last  ###\n");
   printf("  ###    time in the output file when appending   ###\n");
   printf("\n");
   printf("  [-proc] <processor list to combine>\n");
   printf("  ###       Single Processor Format = -proc 10 \n");
   printf("  ###       OR Tuple Format (NO SPACES!) = -proc 1-5,10 n\n");
   printf("\n");
   printf("  [-pstart]  <processor to begin at>\n");
   printf("  ###    Limit procs per file with non-zero       ###\n");
   printf("  ###    starting proc.                           ###\n");
   printf("\n");
   printf("  [-pstop]  <processor to end at>\n");
   printf("  ###    Limit  procs per file with an ending     ###\n");
   printf("  ###    proc.                                    ###\n");
   printf("\n");
   printf("  [-wait] <minutes to wait>\n");
   printf("  ###    Wait for the files to be written.        ###\n");
   printf("  ###    default is 30 minutes.                   ###\n");
   printf("\n");
   printf("  [-pt] <check file wait time in seconds>\n");
   printf("  ###    Value te pass to wait() function.        ###\n");
   printf("  ###    default is 300 seconds.                  ###\n");
   printf("\n");
   printf("  [-restart] \n");
   printf("  ###    This tell Xmilics that it is awaiting    ###\n");
   printf("  ###    a restart by paradyn. It will wait the   ###\n");
   printf("  ###    time specified by the -wait flag.        ###\n");
   printf("\n");
#ifdef PROC_MAT_SELECT
   printf("  [-mat] <material list to combine>\n");
   printf("  ###       Single Material Format = -mat 10 \n");
   printf("  ###       OR Tuple Format (NO SPACES!) = -mat 1-5,10 n\n");
   printf("\n");
   printf("  [-mstart]  <material to begin at>\n");
   printf("  ###    Limit mats per file with non-zero       ###\n");
   printf("  ###    starting mat.                           ###\n");
   printf("\n");
   printf("  [-mstop]  <material to end at>\n");
   printf("  ###    Limit  mats per file with an ending     ###\n");
   printf("  ###    mat.                                    ###\n");
   printf("\n");
#endif

   printf("  [-append]  <state to end at>\n");
   printf("  ###    Append a timestep to an existing         ###\n");
   printf("  ###    database. This is overriden if a start   ###\n");
   printf("  ###    or stop state are defined.               ###\n");
   printf("\n");
   printf("  [-batch]\n");
   printf("  ###    Run in batch mode append to file.        ###\n");
   printf("\n");
   printf("  [-batch_overwrite]\n");
   printf("  ###    Run in batch mode but overwrite the      ###\n");
   printf("  ###    existing file.                           ###\n");
   printf("\n");
   printf("  [-newfile]\n");
   printf("  ###    Do not append - create new file          ###\n");
   printf("  [-seqnum] <n output sequence number to append to output file> \n");
   printf("  ###  This is used to create unique files for output ###\n");
   printf("  ###  when running parallel Xmilics jobs.           ###\n");
   printf("\n");
   printf("  [-V]   Display build stats (version-info)\n ");
}

