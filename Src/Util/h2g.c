
/* $Id$ */

/*
 * h2g - HDF Palette to Griz colormap translator.
 * 
 * This program takes as input an HDF file which contains ONLY a
 * single palette and produces a Griz text colormap on stdout.
 * 
 * cc -o h2g h2g.c
 */

#include <stdio.h>

#define HDF_HDR_OFFSET ((long) 202)
#define RGB_DATA_LEN (768)

main( int argc, char *argv[] )
{
    FILE *hfile;
    int count;
    unsigned char dbuf[RGB_DATA_LEN];
    unsigned char *p_c;
    float red, green, blue;
    
    if ( argc != 2 )
    {
	fprintf( stderr, "Usage: h2g HDF_palette_file\n" );
	exit( 1 );
    }
    else
    {
	/* Open the HDF palette file. */
	hfile = fopen( argv[1], "rb" );
	if ( hfile == NULL )
	{
	    fprintf( stderr, "Unable to open file \"%s\".\n", argv[1] );
	    exit( 2 );
	}
    }
   
    /* 
     * Position the file pointer past the header to the start of rgb
     * data.  HDF_HDR_OFFSET was determined by inspecting hex dumps
     * of several HDF palette files.  They were at least a couple
     * years old at the time, and it's conceivable that the format
     * could have changed, necessitating a new offset for more recent
     * HDF files...
     */
    if ( fseek( hfile, HDF_HDR_OFFSET, SEEK_SET ) )
    {
	fprintf( stderr, "Unable to seek HDF file to color data (byte %d).\n", 
	         (int) HDF_HDR_OFFSET );
        exit( 3 );
    }
   
    /* Read the 256 rgb triples (one byte/component -> 768 bytes). */
    count = fread( dbuf, 1, RGB_DATA_LEN, hfile );
    if ( count != RGB_DATA_LEN )
    {
	fprintf( stderr, "Only read %d of %d bytes (attempted) of rgb data.\n",
		 count, RGB_DATA_LEN );
	fprintf( stderr, "Exiting.\n" );
	exit( 4 );
    }
   
    /* Don't need the input file anymore... */
    fclose( hfile );

    /*
     * Scale each component onto [0.0, 1.0] and write to stdout
     * one rgb triple at a time.
     */
    for ( p_c = dbuf; p_c < dbuf + RGB_DATA_LEN; )
    {
        red = (float) *p_c++ / 255.0;
        green = (float) *p_c++ / 255.0;
        blue = (float) *p_c++ / 255.0;
	
	printf( "%8.6f %8.6f %8.6f\n", red, green, blue );
    }
    
    exit( 0 );
}
