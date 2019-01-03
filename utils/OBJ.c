/* $Id: OBJ.c,v 1.6 2011/05/26 19:41:32 durrenberger1 Exp $ */

/*
 * OBJ file I/O Routines.
 * 

 * 
 */

 /*
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - November 21, 2008: Created.
 *                See SCR#480
 *
 ************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mili_internal.h"


/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open 
 * MILI families.
 */
extern Mili_family **fam_list;

/*****************************************************************
 * TAG( fam_qty )
 *
 * The actual number of all currently open MILI families.
 */
extern int fam_qty;

static int obj_count_tokens(char *, int *);
static int obj_parse_token(char *token, char *delimiter,
                           int *var1, int *var2, int *var3);

int
mc_OBJ_read(         /* Load an OBJ file into memory */
    char   *fname,   /* Name of OBJ file to load */
    int    group,    /* Group number (-1 if all groups) */
    ObjDef **obj)
{
    FILE *fp;
    char input_line[4000], next_token[64];

    int num_groups=0, group_index=0;
    int vCnt=0,       vnCnt=0, faceCnt=0,    lineCnt=0, pointCnt=0;

    int vertex_index=0, normal_index=0, face_index=0, 
        line_index=0, point_index=0;

    float x,y,z;

    int *group_vCnt=NULL,    *group_vnCnt=NULL, *group_faceCnt=NULL,
        *group_lineCnt=NULL, *group_pointCnt=NULL;
    
    char *group_names[10000], one_name[258];
    Bool_type group_found=FALSE, first_group=FALSE;

    int smoothing_group=0;

    Bool_type newToken=FALSE;
    char face_token[64]; 
    int  tokenCnt=0, tokenIndexes[100];
    int  varCnt=0;

    int i,j,k;

    ObjDef *p_obj;

    int status=OK;

    fp = fopen(fname, "r");
    if (!fp) {
        return (!OK);
    }

    /* First count number of groups for memory allocation */
    lineCnt=0;
    while ( fgets( input_line, 256,  fp) )
    {
        lineCnt++;

        /* Ignore comment lines */
        if ( input_line[0]=='#' ) {
            continue; 
        }

        if (input_line[0]=='g' && input_line[1]==' ')
        {
            sscanf(input_line,"g %256s", one_name);

            /* Check if group_name is unique */
            group_found = FALSE;
            for (j=0; j<num_groups; j++) {
                if (!strcmp(group_names[j],one_name))
                {
                    group_found = TRUE;
                    continue;
                }
            }

            if (!group_found)
            {
                group_names[num_groups] = strdup(one_name); 
                num_groups++;
            }

            if (num_groups==0)
            {
                group_names[num_groups] = strdup(one_name);
                num_groups++;
            }
        }
    }

    if (num_groups==0)
    {
        num_groups=1;
        group_names[0] = strdup("group1");
    }

    /* Allocate space for all group summary data */

    rewind ( fp ); 
    group_vCnt     = (int *)  malloc( num_groups*sizeof(int) );
    group_vnCnt    = (int *)  malloc( num_groups*sizeof(int) );
    group_faceCnt  = (int *)  malloc( num_groups*sizeof(int) );
    group_lineCnt  = (int *)  malloc( num_groups*sizeof(int) );
    group_pointCnt = (int *)  malloc( num_groups*sizeof(int) );


    for (i=0; i<num_groups; i++)
    {
        group_vCnt[group_index]     = 0;
        group_vnCnt[group_index]    = 0;
        group_faceCnt[group_index]  = 0;
        group_lineCnt[group_index]  = 0;
        group_pointCnt[group_index] = 0;
    }


    /* First count number of groups for memory allocation */
    group_index=-1;
    while ( fgets( input_line, 256,  fp) )
    {
        /* Ignore comment lines */
        if ( input_line[0]=='#' ) {
            continue;
        }

        if (input_line[0]=='s' && input_line[1]==' ' && !first_group)
        {
            sscanf(input_line,"g %d", &smoothing_group);
        }

        if (input_line[0]=='g' && input_line[1]==' ' && !first_group)
        {

            sscanf(input_line,"g %256s", one_name);

            /* Locate the group name in list of groups */
            for (i=0; i<num_groups; i++) {
                if (!strcmp(group_names[i] ,one_name)) {
                    group_index=i;
                }
            }

            if ( group_index>=0 )
            {
                group_names[group_index]     = strdup(one_name);
                group_vCnt[group_index]     += vCnt;
                group_vnCnt[group_index]    += vnCnt;
                group_faceCnt[group_index]  += faceCnt;
                group_lineCnt[group_index]  += lineCnt;
                group_pointCnt[group_index] += pointCnt;

                vCnt     = 0;
                vnCnt    = 0;
                faceCnt  = 0;
                lineCnt  = 0;
                pointCnt = 0;
            }
        }

        if (input_line[0]=='g' && input_line[1]==' ') {
            first_group = TRUE;
        }

        if (input_line[0]=='v' && input_line[1]==' ') {
            vCnt++;
        }
        if (input_line[0]=='v' && input_line[1]=='n' && input_line[2]==' ') {
            vnCnt++;
        }
        if (input_line[0]=='f' && input_line[1]==' ')
        {
            tokenCnt = obj_count_tokens( &input_line[1], NULL );
            faceCnt += tokenCnt/4;
        }
        if (input_line[0]=='l' && input_line[1]==' ')
        {
            tokenCnt = obj_count_tokens( &input_line[1], NULL );
            lineCnt += tokenCnt/2;
        }
        if (input_line[0]=='p' && input_line[1]==' ') {
            pointCnt++;
        }
    }

    if ( group_index<=1 )
    {
        group_vCnt[0]     = vCnt;
        group_vnCnt[0]    = vnCnt;
        group_faceCnt[0]  = faceCnt;
        group_lineCnt[0]  = lineCnt;
        group_pointCnt[0] = pointCnt;
    }    
    else
    {
        group_vCnt[group_index]     += vCnt;
        group_vnCnt[group_index]    += vnCnt;
        group_faceCnt[group_index]  += faceCnt;
        group_lineCnt[group_index]  += lineCnt;
        group_pointCnt[group_index] += pointCnt;
    }    

    /* Now load all of the face, point, and line data */
    rewind ( fp ); 
    p_obj = (ObjDef *) malloc( sizeof(ObjDef));

    p_obj->num_groups = num_groups;
    p_obj->groups     = (ObjGroup *) malloc( num_groups*sizeof(ObjGroup));

    /* Allocate storage for group specific data */
    for (i=0; i<num_groups; i++)
    {
        p_obj->groups[i].group_name  = NULL;

        p_obj->groups[i].numVertex   = group_vCnt[i];     
        p_obj->groups[i].numLineSegs = group_lineCnt[i];
        p_obj->groups[i].numFaces    = group_faceCnt[i];
        p_obj->groups[i].numPoints   = group_pointCnt[i];

        p_obj->groups[i].vertex = (float **) malloc(group_vCnt[i]*sizeof(float *));   
        p_obj->groups[i].normal = (float **) malloc(group_vnCnt[i]*sizeof(float *));   

        for (j=0; j<group_vCnt[i]; j++)
        {
            p_obj->groups[i].vertex[j] = (float *) malloc(3*sizeof(float));
        }

        for (j=0; j<group_vnCnt[i]; j++)
        {
            p_obj->groups[i].normal[j] = (float *) malloc(3*sizeof(float));
        }

        p_obj->groups[i].vertex_id = (int *) malloc(group_vCnt[i]*sizeof(int));  
        p_obj->groups[i].normal_id = (int *) malloc(group_vnCnt[i]*sizeof(int));  

        p_obj->groups[i].facesVertex = (int **) malloc(group_faceCnt[i]*sizeof(int *));  
        for (j=0; j<group_faceCnt[i]; j++) 
        {
            p_obj->groups[i].facesVertex[j] =(int *) malloc(12*sizeof(int));
            for (k=0; k<24; k++) {
                 p_obj->groups[i].facesVertex[j][k] = -1;
            }
        }

        p_obj->groups[i].linesVertex = (int **) malloc(group_lineCnt[i]*sizeof(int *));  
        for (j=0; j<group_lineCnt[i]; j++) 
        {
            p_obj->groups[i].linesVertex[j] =(int *) malloc(6*sizeof(int));
            for (k=0; k<6; k++) {
                 p_obj->groups[i].linesVertex[j][k] = -1;
            }
        }

        p_obj->groups[i].pointsVertex = (int **) malloc(group_pointCnt[i]*sizeof(int *));  
        for (j=0; j<group_lineCnt[i]; j++) 
        {
            p_obj->groups[i].pointsVertex[j] =(int *) malloc(3*sizeof(int));
            for (k=0; k<3; k++) {
                 p_obj->groups[i].pointsVertex[j][k] = -1;
            }
        }

    }

    rewind ( fp ); 
    group_index=0;
    vertex_index=0;
    normal_index=0;
    face_index=0;
    line_index=0;
    point_index=0;

    lineCnt=0;

    while ( fgets( input_line, 256,  fp) )
    {
        /* Ignore comment lines */
        if ( input_line[0]=='#' ) {
            continue; 
        }

        if (input_line[0]=='g' && input_line[1]==' ')
        {
            /* Locate the group name in list of groups */
            for (i=0; i<num_groups; i++) {
                 if (!strcmp(group_names[i] ,&input_line[2])) {
                     group_index=i;
                 }
            }
        }

        if (input_line[0]=='s')
        {
            sscanf(input_line,"g %d", &smoothing_group);
        }

        if (!p_obj->groups[group_index].group_name) {
            p_obj->groups[group_index].group_name = strdup(group_names[i]);
        }

        if (input_line[0]=='v' && input_line[1]==' ')
        {
            sscanf(input_line, "v %f %f %f", &x, &y, &z);
            p_obj->groups[group_index].vertex[vertex_index][0] = x;
            p_obj->groups[group_index].vertex[vertex_index][1] = y;
            p_obj->groups[group_index].vertex[vertex_index][2] = z;

            p_obj->groups[group_index].vertex_id[vertex_index] = lineCnt;
            vertex_index++;
        }

        if (input_line[0]=='v' && input_line[1]=='n' && input_line[2]==' ')
        {
            sscanf(input_line, "vn %f %f %f", &x, &y, &z);
            p_obj->groups[group_index].normal[normal_index][0] = x;
            p_obj->groups[group_index].normal[normal_index][1] = y;
            p_obj->groups[group_index].normal[normal_index][2] = z;

            p_obj->groups[group_index].normal_id[normal_index] = lineCnt;
            normal_index++;
        }

        /* Faces */
        if (input_line[0]=='f' && input_line[1]==' ')
        {
            tokenCnt = obj_count_tokens( input_line, tokenIndexes );
            tokenCnt /=4;

            /* Determine number of numbers in a token which can be in the 
             * form of: (1) number
             *          (2) f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3]
             */

            /* Now process the set of tokens */
            for (i=0; i<tokenCnt; i++)
            {
              /* varCnt = obj_parse_token(token, "/" , int *var1, int *var2, int *var3) */
            }
        }

        /* Lines */
        if (input_line[0]=='l' && input_line[1]==' ')
        {
            tokenCnt = obj_count_tokens( input_line, tokenIndexes );
            tokenCnt /=2;
        }

        /* Points */
        if (input_line[0]=='p' && input_line[1]==' ')
        {
            pointCnt += obj_count_tokens( input_line, tokenIndexes );
            lineCnt++;
        }
    }

    fclose( fp );

    *obj = p_obj; /* Return the filled in geometry */

    /* Clean up allocated local storage. */
    free(group_faceCnt);
    free(group_lineCnt);
    free(group_pointCnt);
    free(group_vCnt);
    free(group_vnCnt);

    return (OK);
}

static int obj_count_tokens(char *input_line, int *tokenIndexes)
{
    Bool_type newToken=FALSE;
    int       tokenCnt=0;
    int       i=0;

    for (i=0; i<strlen(input_line); i++)
    {
         if (newToken && input_line[i]!=' ')
         {
             if (tokenIndexes) {
                 tokenIndexes[tokenCnt] = i;
             }

             newToken=TRUE;
             tokenCnt++;
         }
         if (input_line[i]==' ') {
             newToken=FALSE;
         }
    }
    return( tokenCnt );
}

static int obj_parse_token(char *token, char *delimiter,
                           int *var1, int *var2, int *var3)
{
    Bool_type newToken=FALSE;
    int       tokenCnt=1;
    int       firstChar=0;
    int       i=0;

    *var1 = 0;
    *var2 = 0;
    *var3 = 0; 
   
    for (i=0; i<strlen(token); i++)
    {
         if (token[i]!=' ')
         {
             firstChar=i;
             break;
         }
    }

    if (firstChar>=(strlen(token)-2)) {
        return 0;
    }

    for (i=0; i<strlen(token); i++)
    {
         if (token[i]==delimiter[0])
         {
             token[i]=' ';
             tokenCnt++;
         }
    }

    sscanf( token, "%d %d %d", var1, var2, var3);
    
    return( tokenCnt );
}


/* End of OBJ.c */



#ifdef NEW1
/* Obj Group */
/* Obj Group */
typedef struct _objgroup
{
    char   *group_name;

    int    numVertex;
    int    numLineSegs;
    int    numFaces;
    int    numPoints;

    float  *vertex[3];
    float  *normal[3];
    int    *vertex_id, *normal_id;

    int    **facesVertex;
    int    **linesVertex;
    int    **pointsVertex;
} ObjGroup;


/* Obj top-level */
typedef struct _objdef
{
    char *obj_name;
    int  num_groups;
    ObjGroup *groups;
} ObjDef;
#endif
