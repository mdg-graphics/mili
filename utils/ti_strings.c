#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "mili.h"
#include "misc.h"
#include "list.h"
#include "gahl.h"
#include "mili_internal.h"
extern Mili_family **fam_list;
int main(int argc, char *argv[])
{
    if ( argc != 2 )
    {
        printf("%s", "Usage: ti_strings <path/file_name>\n");
        exit(100);
    }
    char **wildcard_list, *out_data_char, path[1028], filename[512];
    int wildcard_count = 0, datatype, datalength, dbid, status = OK, i, count = 0;

    Mili_family *fam;
    filename[0] = '\0';
    path[0] = '\0';
    char *loc = strrchr(argv[1], '/');
    if ( loc )
    {
        strcpy(filename, loc + 1);
        int length = strlen(argv[1]) - strlen(loc);
        strncpy(path, argv[1], length);
        path[length] = '\0';
    }
    else
    {
        strcpy(filename, argv[1]);
        path[0] = '.';
        path[1] = '\0';
    }
    status = mc_open(filename, path, "r", &dbid);

    if ( status != OK )
    {
        mc_print_error("mc_open", status);
        exit(100);
    }
    fam = fam_list[dbid];
    wildcard_count = htable_search_wildcard(fam->ti_param_table, wildcard_count, FALSE, "*", "NULL", "NULL", 0);
    wildcard_list = (char **)malloc(wildcard_count * sizeof(char *));
    wildcard_count = 0;
    wildcard_count =
        htable_search_wildcard(fam->ti_param_table, wildcard_count, FALSE, "*", "NULL", "NULL", wildcard_list);

    for ( i = 0; i < wildcard_count; i++ )
    {
        /* Here we do our best to filter some common paramters from the TI files
         */
        if ( strncmp("Node Labels", wildcard_list[i], strlen("Node Labels")) == 0 ||
             (strncmp("Element Labels", wildcard_list[i], strlen("Element Labels")) == 0) ||
             (strncmp("nproc", wildcard_list[i], strlen("nproc")) == 0) ||
             (strncmp("lib version", wildcard_list[i], strlen("lib version")) == 0) ||
             (strncmp("date", wildcard_list[i], strlen("date")) == 0) ||
             (strncmp("username", wildcard_list[i], strlen("username")) == 0) ||
             (strncmp("arch name", wildcard_list[i], strlen("arch name")) == 0) ||
             (strncmp("host name", wildcard_list[i], strlen("host name")) == 0) )
        {
            continue;
        }

        mc_ti_get_data_len(dbid, wildcard_list[i], &datatype, &datalength);

        if ( datatype == M_STRING )
        {
            out_data_char = (char *)malloc(datalength * sizeof(char) + 1);
            status = mc_ti_read_string(dbid, wildcard_list[i], out_data_char);
            if ( status == OK )
            {
                printf("\n%-15s : %s", wildcard_list[i], out_data_char);
            }
            count++;
            free(out_data_char);
        }
    }
    printf("\n\nLocated %d Time Invariant strings.\n", count);
    htable_delete_wildcard_list(wildcard_count, wildcard_list);
    exit(0);
}
