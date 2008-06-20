/* $Id$ */
/* 
 * offscreen.c - Routine(s) to support offscreen rendering via
 * the Mesa offscreen context.
 * 
 *      Larry Sanford
 *      Lawrence Livermore National Laboratory
 *      Nov 2000
 *
 * 
 * This work was produced at the University of California, Lawrence 
 * Livermore National Laboratory (UC LLNL) under contract no. 
 * W-7405-ENG-48 (Contract 48) between the U.S. Department of Energy 
 * (DOE) and The Regents of the University of California (University) 
 * for the operation of UC LLNL. Copyright is reserved to the University 
 * for purposes of controlled dissemination, commercialization through 
 * formal licensing, or other disposition under terms of Contract 48; 
 * DOE policies, regulations and orders; and U.S. statutes. The rights 
 * of the Federal Government are reserved under Contract 48 subject to 
 * the restrictions agreed upon by the DOE and University as allowed 
 * under DOE Acquisition Letter 97-1.
 * 
 * 
 * DISCLAIMER
 * 
 * This work was prepared as an account of work sponsored by an agency 
 * of the United States Government. Neither the United States Government 
 * nor the University of California nor any of their employees, makes 
 * any warranty, express or implied, or assumes any liability or 
 * responsibility for the accuracy, completeness, or usefulness of any 
 * information, apparatus, product, or process disclosed, or represents 
 * that its use would not infringe privately-owned rights.  Reference 
 * herein to any specific commercial products, process, or service by 
 * trade name, trademark, manufacturer or otherwise does not necessarily 
 * constitute or imply its endorsement, recommendation, or favoring by 
 * the United States Government or the University of California. The 
 * views and opinions of authors expressed herein do not necessarily 
 * state or reflect those of the United States Government or the 
 * University of California, and shall not be used for advertising or 
 * product endorsement purposes.
 * 
 */

#ifdef SERIAL_BATCH

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <GL/osmesa.h>


extern OSMesaContext OSMesa_ctx;

int OffscreenContext( void *bfp, int width, int height, int ID );


#define RGBA_BUFFER_SIZE 4


/*****************************************************************
 * TAG( OffscreenContext )
 *
 * Create an application context and rendering buffer for the off
 * screen rendering function.     
 */
int
OffscreenContext( void *bfp, int width, int height, int ID )
{
    int
        rc = 0;

 
    /* */

    /*
    if (ID != GuiServer)
	init_mesh_window(env.curr_analy);
    */

    /*
     * allocate rendering buffer
     */

    if ( NULL == (bfp = (void *)malloc( width * height * RGBA_BUFFER_SIZE )) )
    {
         fprintf(stderr, "Offscreen() Error: Offscreen buffer allocation\n");
         rc = -2;
    }
    else
    {
         if ( NULL == (OSMesa_ctx = OSMesaCreateContext( GL_RGBA, NULL )) )
         {
             fprintf( stderr, "gui.c(OSMesa_ctx):  offscreen context allocation error\n" );
             rc = -1;
         }

         if ( GL_FALSE == OSMesaMakeCurrent( OSMesa_ctx, bfp, GL_UNSIGNED_BYTE, width, height ) )
         {
            fprintf(stderr, "gui.c(OSMesa_ctx): offscreen context current status error\n");
	    rc = -3;
         }
    }


    return( rc );
}

#endif /* SERIAL_BATCH */
