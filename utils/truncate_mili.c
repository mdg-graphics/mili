#include "mili.h"

int usage()
{
    fprintf(stderr, "Usage:  truncate_mili <file_name_base> <state>\n");

}

int main(int argc , char **argv )
{
    if (argc != 3)
        return usage(); 
    
    char *file_name, directory[256];
    int state=0;
    int status = OK, fid;
    char *sep_index = rindex(argv[1], '/');
    if(!sep_index)
    {
        directory[0] = '.';
        directory[1] = '\0';
        file_name = argv[1];
    }else
    {
        file_name = sep_index+1;
        strncpy(directory, argv[1], sep_index - argv[1]);
    }
    
    state = atoi(argv[2]);
    
    if(state <=0)
    {
        fprintf(stderr, "Invalid state number: %s\n",argv[2]);
        return (0);
    }
    
    status = mc_open(file_name,directory,"AaPd",&fid);
    
    mc_print_error("mc_open()",status);
    
    status = mc_restart_at_state(fid, -1, state);
    
    mc_print_error("restart_at_state:",status);

    mc_close(fid);
    return(0);
}
