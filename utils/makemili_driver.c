#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "mili.h"
#include "mili_config.h"

char directory[512];
char plotfile_name[512];

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
   int cptr = strlen(plotfile_name);
   int ptr = strlen(entry->d_name);
   if ((strncmp(plotfile_name,entry->d_name,strlen(plotfile_name)) ==0)
         &&(entry->d_name[ptr-1]== 'A') 
         && (entry->d_name[ptr-2] >='0' && entry->d_name[ptr-2] <='9')
         && !(entry->d_name[ptr-2] == '_')
         && !((entry->d_name[cptr] == '_') && (entry->d_name[cptr+1] == 'c'))) {
		
		
		for(start_check = strlen(plotfile_name); start_check< strlen(entry->d_name) && !found_A; start_check++)
        {
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
 * TAG( find_pad_count )
 *
 * Find the count of integers to fill the file name for searching
 * for files.
 */
int
find_pad_count(char* input_file_name)
{
   int pad=0;
   int found=0;
   char* loc = strrchr(input_file_name,'/');
   struct dirent *dirp;
   DIR *dir;
   
   
   if (loc) {
      strcpy(plotfile_name, loc+1);
      int length = strlen(input_file_name) -strlen(loc);
      strncpy(directory,input_file_name,length);
      directory[length]='\0';
   } else {
      strcpy(plotfile_name, input_file_name);
      directory[0]='.';
      directory[1]='\0';
   }
   
   
   dir = opendir(directory);
   
   if(dir == NULL) {
      fprintf(stderr, "****** ERROR *******\n\n");
      fprintf(stderr, "Invalid directory: %s\nExiting.\n\n",directory);
      fprintf(stderr, "****** ERROR *******\n\n");
      return(-1);
   }
   
   
   
   while ((dirp = readdir(dir)) != NULL) {
      int last_pos = strlen(dirp->d_name)-1;
      if(strstr(dirp->d_name,plotfile_name) != NULL &&
            dirp->d_name[last_pos] == 'A' && dirp->d_name[last_pos-1] != '_') {
         found = 1;
         int pos = strlen(plotfile_name);/*last_pos-1;*/
         while(isdigit(dirp->d_name[pos]) && pos<last_pos) {
            pad++;
            pos++;
         }
         if( pad >0 && dirp->d_name[pos] =='A') {
            break;
         }
      }
   }
   closedir(dir);
   
   if (!found) 
   {
        pad = -1;
   }
   return pad;
}
/**************************************************************
 *  TAG(processSingleFile)
 *  Function to process a single mili database to create a .mili file
 * @param char* file_name 
 * @return  int  
 */
int 
processSingleFile(char* file_name , int close, int* fam_id)
{
    Famid famid;
    int status;
    
    status = mc_open( file_name, directory,"Ar",&famid);
    
    if(status != OK)
    {
        mc_print_error("mc_open()",status);
        return status; 
    }
    
    mc_activate_visit_file(famid, 1);
    
    status = mc_write_mili_metadata(famid);
    *fam_id = famid;
    if(close)
    {
        status = mc_close(famid);
    }
    
    return status;
    
}
/*
 *   TAG(write_multi_processor_files)
 *   Function to write out multiple processor files for the same
 *   simulation run.
 *   @param int padding the  padding needed to create the name
 */
 

int 
write_multi_processor_files(int padding)
{
    int a_file_count,i;
    struct dirent **namelist;
    char temp_name[512];
    int name_length;
    int status = OK;
    Famid famid, *ids;
    a_file_count = scandir(directory, &namelist, (void *)file_select, alphasort);
    ids = calloc(sizeof(int), a_file_count);
    // Run over each named file discovered by scandir function
    for(i=0;i<a_file_count;i++)
    {
        name_length = strlen(namelist[i]->d_name)-1;
        strncpy(temp_name, namelist[i]->d_name,name_length);
        status = processSingleFile( temp_name, 0, &famid);
        if(status != OK)
        {
            fprintf(stderr, "Error processing file %s.\n",temp_name);
            return status; 
        }
        ids[i] = famid;
        
    }
    // We any file to write the global information
    //status = mc_open(temp_name,directory,"Ar",&famid);
    if(status != OK)
    {
        mc_print_error("mc_open for globalfile write.\n",status);
        return status;
    }
    
    status = mc_activate_visit_file(ids[0], 1);
    if(status != OK)
    {
        mc_print_error("Could not activate visit file writing.\n",status);
        return status;
    }
    
    status = mc_write_global_metadata(ids[0]);
    if(status != OK)
    {
        mc_print_error("Failed to write globalfile for plotfile.\n",status);
        return status;
    }
    for(i=0; i<a_file_count; i++)
    {
         status = mc_close(ids[i]);
         if(status != OK)
         {
              mc_print_error("mc_close to single files in global write.\n",status);
              return status;
         }
    }
    //status = mc_close(famid);
    if(status != OK)
    {
        mc_print_error("mc_close to close in global write.\n",status);
        return status;
    }
    return status;
}

int
main( int argc, char *argv[] )
{
    
    int pad;
    int status;
    int famid;
    
    if(argc != 2)
    {
        fprintf(stderr, "Usage: makemili_driver base_filename\n");
        return 101;
    }
    fprintf(stderr, "\n\n");                                               ;
   fprintf(stderr, "\n\t Running Makemili_driver Version: %s(%s)", PACKAGE_VERSION,PACKAGE_DATE);
   fprintf(stderr, "\n\n");
    pad = find_pad_count(argv[1]);
    
    // The pad will tell us what kind of file this is.
    switch(pad)
    {
        case -1:
            fprintf(stderr, "File %s was not located.\n",argv[1]);
            break;
        case 0:
            //Generate for a single plot file.
            fprintf(stderr, "Single processor %s.\n", plotfile_name);
            status = processSingleFile(plotfile_name,1,&famid);
            break;
        default:
            //Must have multiple processors
            fprintf(stderr, "Multiple processors.  Base Name: %s\n", 
                    plotfile_name);
            status = write_multi_processor_files(pad);
            break;
        
    }
    if(status == OK)
    {
        fprintf(stderr, "Successfully completed generation of metadate file for %s\n",
                plotfile_name);
    }
}
