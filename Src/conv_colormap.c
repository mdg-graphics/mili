/* $Id$ */
/*
 * conv_colormap.c - Converts a colormap file into a C data type
 * for inclusion into Griz.
 *
 *      Ivan R. Corey
 *      Lawrence Livermore National Laboratory
 *      April 10, 2010
 *
 ************************************************************************
 * Modifications:
 *
 *  Ivan R. Corey - April 12, 2010: Created
 *
 ************************************************************************
 */

/* buildinfo.c */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>

int main(int argc, char *argv[])
{
    FILE *infile, *outfile;
    char out_fname[256], line[1000];
    int  num_count=0;
    int  i=0, j=0;

    if (!(infile = fopen(argv[1], "r")))
    {
        fprintf(stderr, "Failed to open %s for writing!\a\n", argv[1]);
        exit(1);
    }

    strcpy( out_fname, argv[1] );
    for ( i=0;
            i<strlen( out_fname );
            i++ )
    {
        if ( out_fname[i]=='.' )
        {
            for ( j=i;
                    j<strlen(out_fname);
                    j++ )
            {
                if ( out_fname[j] ==' ' )
                    break;
                out_fname[j] = '\0';
            }
        }
    }

    if (!(outfile = fopen(out_fname, "w")))
    {
        fprintf(stderr, "Failed to open %s for writing!\a\n", argv[1]);
        exit(1);
    }

    while ( fgets( line, 1000, infile ) != NULL )
    {
        num_count = 0;
        for ( i=0;
                i<strlen(line);
                i++ )
            if ( line[i]=='\n' )
                line[i] = ' ';

        for ( i=0;
                i<strlen(line);
                i++ )
        {

            if ( num_count<2 && line[i]!=' ' && line[i+1]==' ' && line[i]!=',' )
            {
                line[i+1]=',';
                num_count++;

            }
        }
        if ( num_count==2 )
        {
            fprintf( outfile, "\n{ %s }, ", line );
            continue;
        }
    }


    fclose( infile );
    fclose( outfile );

    exit(0);
}
