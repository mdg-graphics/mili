/* $Id$ */
/* 
 * video.c - Video driver routines.
 * 
 *      Mark Christon
 *      Lawrence Livermore National Laboratory
 *      Oct 1 1991
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

#ifdef VIDEO_FRAMER

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "vframer.h"

/* Define the NTSC x,y resolution for rendering. */
#define IXRES 720
#define IYRES 486

/* V-LAN control commands for initialization and recording. */
static char *init_cmds[8] = {"ND1","SR","CL","TSV","GT0","SD1","PF","LR" };
static char *rec_cmds[5] = {"SI","SD1","PF","ES","SR"};

/* Global variables. */
static int frame_cnt = 0;
static int disc_frame;
static int vlan_initialized = FALSE;
static int framer_selected = FALSE;
static VFR_DEV *Vfr;

/* Local routines. */
static void vid_move_frame();
static void vid_convert();


/*****************************************************************
 * TAG( tcode_to_frame )
 *
 * Convert a vlan timecode to the actual frame number.
 */
int
tcode_to_frame( tcode )
char *tcode;
{
    int hour, min, sec, frame;

    sscanf( tcode, "%d:%d:%d:%d", &hour, &min, &sec, &frame );
    return( 60*60*30*hour + 60*30*min + 30*sec + frame );
}


/*****************************************************************
 * TAG( frame_to_tcode )
 *
 * Convert a frame number to a vlan timecode string.
 */
void
frame_to_tcode( frame_num, tcode )
int frame_num;
char *tcode;
{
    int hour, min, sec;

    hour = frame_num / (60*60*30);
    frame_num = frame_num % (60*60*30);
    min = frame_num / (60*30);
    frame_num = frame_num % (60*30);
    sec = frame_num / 30;
    frame_num = frame_num % 30;

    sprintf( tcode, "%02d:%02d:%02d:%02d", hour, min, sec, frame_num );
}


/*****************************************************************
 * TAG( vid_select )
 *
 * Sets the video framer output type and initializes the frame buffer.
 * The select parameter should be
 *      1 - NTSC
 *      2 - Beta
 * This routine should only be called once.
 */
void
vid_select( select )
int select;
{
    char mode_name[30];
    int setup_mode;

    /* Only want to execute this command once. */
    if ( framer_selected )
    {
        wrt_text( "Video framer already selected.\n" );
        return;
    }
    framer_selected = TRUE;
 
    wrt_text( "Initializing the video framer.\n" );

    setup_mode = VFR_CUSTOM_SETUP;

    /* 
     * If select is
     *     1 - initialize for NTSC.
     *     2 - initialize for Beta.
     */
    if ( select == 1 )
        strcpy( mode_name, "vfr_ntsc" );
    else if ( select == 2 )
        strcpy( mode_name, "vfr_rgb_525" );
    else
        wrt_text( "Video output type unrecognized.\n" );

    if ( !(Vfr = vfr_open( DEFAULT_VFR_ADDR, setup_mode, mode_name )) )
    {  
        vfr_perror( "Video framer error: " );
        return;
    }

    /* Get the target VideoFramer resolution. */
    /*
    xres = vfr_map_mode_to_active_width( VFR_DEVICE_MODE(*Vfr) );
    */

    /* Send out some color bars. */
    vfr_rgb_bars( Vfr );
    vfr_convert_from_rgb( Vfr, RGB_ACTIVE_LINE_LENGTH );
    /* vfr_rgb_to_beta( Vfr, RGB_ACTIVE_LINE_LENGTH ); */ 

    wrt_text( "Done initializing the video framer.\n" );
}


/*****************************************************************
 * TAG( vid_move_frame )
 *
 * Copy an image from the screen display to the video framer shadow 
 * buffer.
 */
static void
vid_move_frame()
{
    unsigned char *ipdat;

    ipdat = (unsigned char *)calloc( IXRES*IYRES*4, sizeof( unsigned char ) );

    /* Read the pixel data into ipdat. */
    screen_to_memory( TRUE, IXRES, IYRES, ipdat );

    /*
     * Copy the pixels from memory to
     * the VideoFramer Shadow Buffer.
     */
    vid_convert( ipdat, VFR_DEVICE_PSHADOW(*Vfr) );
    vfr_convert_from_rgb( Vfr, RGB_ACTIVE_LINE_LENGTH );

    free( ipdat );
}


/*****************************************************************
 * TAG( vid_convert )
 *
 * Convert the image for video.  Transfers the image from 
 * memory to the video framer shadow buffer.
 */
static void
vid_convert( membuf, vidbuf )
unsigned char *membuf;
long *vidbuf;
{
    unsigned char *ibuf;
    long *obuf;
    int a, r, g, b;
    int iend, jend, i, j;

    iend = IXRES - 1;
    jend = IYRES - 1;
    obuf = vidbuf;

    for ( j = jend; j >= 0; j-- )
    { 
        ibuf = membuf + j*IXRES*4;

        for ( i = 0; i <= 1023; i++ )
        { 
            if ( i <= iend )
            { 
                r = *ibuf;
                ibuf++;
                g = *ibuf;
                ibuf++;
                b = *ibuf;
                ibuf++;
                a = *ibuf;
                ibuf++;

                *obuf = a;
                *obuf = *obuf << 8;
                *obuf = *obuf | b;
                *obuf = *obuf << 8;
                *obuf = *obuf | g;
                *obuf = *obuf << 8;
                *obuf = *obuf | r;
            }
            obuf++;
        }
    }
}


/*****************************************************************
 * TAG( vid_cbars )
 *
 * Put color bars into the video framer's frame buffer.
 */
void
vid_cbars()
{
   vfr_rgb_bars( Vfr );
   vfr_convert_from_rgb( Vfr, RGB_ACTIVE_LINE_LENGTH );
}


/*****************************************************************
 * TAG( vid_vlan_init )
 *
 * Initializes the V-LAN.
 * 
 */
int
vid_vlan_init()
{
    int i, ns, synch, ndot;
    char *resp;

    /* The V-LAN code returns 12-character errors. */

    /* Reset the frame count. */
    frame_cnt = 0;

    wrt_text( "Initializing the V-LAN.\n" );

    if( !vfr_vlan_init( Vfr, VFR_VLAN_525 ) )
    {  
        vfr_perror( "Video framer error: " );
        return( 0 );
    }

    for ( i = 0; i <= 7; i++ )
    {
        if ( (resp = vfr_vlan_cmd( Vfr, init_cmds[i] )) == NULL )
        {  
            wrt_text( "V-LAN Command Failed.\n" );
            return( 0 );
        }

        if ( i == 1 && strcmp(resp,"OFF LINE    ") == 0 )
        { 
            wrt_text( "Video Transport is OFF LINE.\n" );
            wrt_text( "Check status of VTR & try again.\n" );
            return( 0 );
        }
        else if ( i == 1 && strcmp(resp,"EJECTED     ") == 0 )
        {  
            wrt_text( "Video Transport is EJECTED.\n" );
            wrt_text( "Check status of VTR & try again.\n" );
            return( 0 );
        }
        else if ( i == 7 )
            disc_frame = tcode_to_frame( resp ) + 1;
    }

    wrt_text( "Starting at frame: %d\n", disc_frame );
    wrt_text( "\nDone initializing the V-LAN.\n" );
    vlan_initialized = TRUE;
 
    return( 1 );
}


/*****************************************************************
 * TAG( vid_vlan_rec )
 *
 * Do a frame accurate record.  Returns 1 if successful and 0 if
 * failed.
 */
int 
vid_vlan_rec( move_frame, record_n_frames )
int move_frame;
int record_n_frames;
{
    int synchup;
    char *resp;
    char tcode[50], com[50];

    if ( !vlan_initialized && record_n_frames > 0 )
    {
        wrt_text( "Use vidinit to initialize the VLAN first!\n" );
        return( 0 );
    }

    if ( record_n_frames > 0 )
        wrt_text( "Recording %d frame(s) (count = %d).\n",
                  record_n_frames, frame_cnt+record_n_frames );
    else
        wrt_text( "Moving a frame.\n", frame_cnt+1 );
 
    /* Move the image from screen memory to the frame buffer. */
    if( move_frame )
        vid_move_frame();
 
    if ( record_n_frames > 0 )
    {
        /* Set the inpoint register to the current position. */
        sprintf( com, "SI " );
        frame_to_tcode( disc_frame, tcode );
        strcat( com, tcode );
        wrt_text( "Recording frame: %d  Timecode: %s\n", disc_frame, tcode );
        if ( (resp = vfr_vlan_cmd( Vfr, com )) == NULL )
        {  
            wrt_text( "V-LAN command failed: %s\n", com );
            return( 0 );
        }

        /* Set the duration register to number of frames to record. */
        sprintf( com, "SD%d", record_n_frames );
        if ( (resp = vfr_vlan_cmd( Vfr, com )) == NULL )
        {  
            wrt_text( "V-LAN command failed: %s\n", com );
            return( 0 );
        }
 
        /* Now record the frame, and synch up! */
        if ( (resp = vfr_vlan_cmd( Vfr, rec_cmds[2] )) == NULL )
        { 
            wrt_text( "V-LAN command failed: %s\n", rec_cmds[2] );
            return( 0 );
        }
        /* wrt_text( "Performed edit (PF)\n" ); */

        sleep( 1 ); 
        synchup = 1;
        while( synchup )
        {
            if ( (resp = vfr_vlan_cmd( Vfr, rec_cmds[3] )) == NULL )
            {  
                wrt_text( "V-LAN command failed: %s\n", rec_cmds[3] );
                return( 0 );
            }
            /* wrt_text( "Edit status (ES) is: %s\n", resp ); */

            if ( strcmp( resp, "ABORTED     " ) == 0 )
            {
                wrt_text( "Edit aborted, leaving record frame.\n" );
                if( (resp = vfr_vlan_cmd( Vfr, "AB" )) == NULL )
                    wrt_text( "V-LAN command failed: AB\n" );
                return( 0 );
            }

            if ( (resp = vfr_vlan_cmd( Vfr, rec_cmds[4] )) == NULL )
            {  
                wrt_text( "V-LAN command failed: %s\n", rec_cmds[4] );
                return( 0 );
            }
            /* wrt_text( "Status request 1 (SR) is: %s\n", resp ); */

            if ( strcmp( resp, "PAUSE       " ) == 0 )
                synchup = 0;
        }
        disc_frame += record_n_frames;
        frame_cnt += record_n_frames;
    }

    if ( record_n_frames > 0 )
        wrt_text( "Done recording frame(s).\n" );
    else
        wrt_text( "Done moving a frame.\n" );

    return( 1 );
}


#endif VIDEO_FRAMER

