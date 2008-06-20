/* $Id$ */

/* Header for testing procedures
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

#ifndef _INCLUDED_TEST_H_
#define _INCLUDED_TEST_H_

#ifndef ANSI_ARGS
#if defined(__STDC__) || defined(__cplusplus) || defined(HAVE_PROTOTYPES)
#define ANSI_ARGS(a) a
#else
#define ANSI_ARGS(a) ()
#endif
#endif

#if defined(NEEDS_STDLIB_PROTOTYPES)
#include "protofix.h"
#endif

void Test_Init ANSI_ARGS((char *, int));
#ifdef USE_STDARG
void Test_Printf ANSI_ARGS((char *, ...));
#else
/* No prototype */
void Test_Printf();
#endif
void Test_Message ANSI_ARGS((char *));
void Test_Failed ANSI_ARGS((char *));
void Test_Passed ANSI_ARGS((char *));
int Summarize_Test_Results ANSI_ARGS((void));
void Test_Finalize ANSI_ARGS((void));
void Test_Waitforall ANSI_ARGS((void));

typedef struct _gpixel
{
   unsigned char R;
   unsigned char G;
   unsigned char B;
   unsigned char A;
   float 	 z;
} GPixel;

/*****************************************************************
 * TAG( Name String type )
 *
 * Struct which contains all the derived/primal result name strings
 * which are collected from the slaves.  
 *
 * NOTE: 
 *    The following figures of the structure nstring is a little
 *    ugly. Please don't change them. this is the max size I can get 
 *    with no malloc buffer allocation trouble. I'll fixed this
 *    malloc problem later or someone else fixes it later if there
 *    is a need.  Yuen L. Lee
 */

typedef struct _nstring
{
   char class[24];		/* Class Name	*/
   char firstlname[32];		/* first level long name */
   char firstsname[16];		/* first level short name */
   char secondlname[32];	/* second level long name */
   char secondsname[16];	/* second level short name */
   char thirdlname[32];		/* third level long name */
   char thirdsname[8];		/* third level short name */
} NString;

#endif
