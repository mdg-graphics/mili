#include "pasitP.h"
#include "silo.h"

#if PARALLEL
#include "mpi.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

/*******************
 * Local Prototypes
 *******************/
static void ReadOverlinkDomain(DBfile *dbfile,
                               const char *submeshname,
                               overlink_data *ol_data);
static void ReadOverlinkFieldvars(DBfile *domfile,
                                  overlink_data *ol_data);

static void ReadOverlinkRegions(DBfile *dbfile,
                                overlink_data *ol_data);

char* CenteringStr(int centering);
char* DataTypeStr(int datatype);


/**************************************************************************
 * read_overlink
 * @author Rob Neely, Gary Friedman
 * @purpose Top level driver for reading a multipart overlink file
 * @param olinkdirname The name of the directory holding the multipart
 *        overlnk file
 * @param errorCode If 0, then the read was successful, otherwise the
 *        error code representing the problem with the read
 * @return overlink_data the data required to write a stresslink file
 **************************************************************************/
overlink_data* read_overlink(const char *olinkdirname,
                             int *errorCode)
{
   DBfile *dbfile, *tfile ;
   int silodriver = DB_HDF5;
   DBmultimesh *multimesh ;
   int me, numDomains, i ;
   overlink_data *ol_data;
   int multiPartOverlink = 1; /* For now, assume multipart */
   int multiDomainPerFile;
   int baseId;

#if PARALLEL
   MPI_Rank(MPI_COMM_WORLD, &me);
#else
   me = 0;
#endif

   /* Only processor 0 need do this work (for now) */
   if (me > 0)
      return NULL ;

   printf("Reading overlink file '%s'\n", olinkdirname);

   /* Create overlink data space */
   ol_data = overlink_data_new();

   /*******************************
    * Read Top Level Overlink File
    *******************************/
   /* Change the working directory into the top level overlink directory */
   if (chdir(olinkdirname) != 0)
   {
      perror("chdir in readoverlink");
   }

   /* Open the OvlTop.silo file, which contains metadata about the problem */
   if ((dbfile = DBOpen("OvlTop.silo", DB_UNKNOWN, DB_READ)) == NULL)
   {
      *errorCode = PASIT_ERR_OPEN_OVL;
      return NULL ;
   }

   multimesh = DBGetMultimesh(dbfile, "MMESH");
   numDomains = multimesh->nblocks;
   DBFreeMultimesh(multimesh) ;

   /* See if it's the one domain per file format, or possibly many
    * domain per file format */
   DBShowErrors(DB_NONE, NULL);
   tfile = DBOpen("domfile0", DB_UNKNOWN, DB_READ) ;
   if (tfile != NULL)
   {
      multiDomainPerFile = 1 ;
      DBClose(tfile) ;
   }
   else
   {
      multiDomainPerFile = 0 ;
   }
   DBShowErrors(DB_TOP, NULL);

   /*****************************
    * Read Domain Overlink Files
    *****************************/
   for(i=0 ; i<numDomains ; i++)   /* Domain loop */
   {
      char domname[40];
      DBfile *dbdomfile ;

      sprintf(domname, "domain%d.silo", i);
      if ((dbdomfile = DBOpen(domname, DB_UNKNOWN, DB_READ)) == NULL)
      {
         *errorCode = PASIT_ERR_OPEN_OVL;
         return NULL;
      }

      ReadOverlinkDomain(dbdomfile, "MESH", ol_data);
   } /* End Domain loop */

   /* Reset the working directory */
   if (chdir("..") != 0)
   {
      perror("chdir in readoverlink");
   }

   /* Combine the individual domains into a single data stream for
    * handing to stresslink */

   return ol_data;
}



/**************************************************************************
 * ReadOverlinkDomain
 * @author Rob Neely, Gary Friedman
 * @purpose Reads one domain's worth of data from the overlink file
 * @param dbfile the silo file pointer
 * @param submeshname The name of the submesh in the overlink file
 *        we're going to read
 **************************************************************************/
static void ReadOverlinkDomain(DBfile *domfile,
                               const char *submeshname,
                               overlink_data *ol_data)
{
   int i, j, idx;
   double* coordD;
   DBcompoundarray *annotations;
   DBucdmesh *mesh ;
   DBzonelist *zones;
   float *x, *y, *z;
   int numNodeSets, numElemSets;
   int numNodes, numElem;
   int numNodesPerElem = 8;
   int lnodelist;

   int status = OK;

   DBShowErrors(DB_NONE, NULL);
   status = DBSetDir(domfile, "domain0");
   DBShowErrors(DB_TOP, NULL);

   /* Read Mesh Data (Coordinates) */
   if ((mesh = DBGetUcdmesh(domfile, submeshname)) == NULL)
   {
      return ;
   }


   /****************
    * Load Node Set
    ****************/
   numNodeSets = 1;

   ol_data->num_node_sets = numNodeSets;
   numNodes = mesh->nnodes;

   ol_data->node_sets              = (ol_node_set*)malloc(numNodeSets*sizeof(ol_node_set));
   ol_data->node_sets[0].num_nodes = numNodes;
   ol_data->node_sets[0].labels    = NULL;

   /* Coordinates */
   for (i=0; i<3; i++)
   {
      ol_data->node_sets[0].cur_coord[i] = (double*)malloc(numNodes*sizeof(double));
      coordD = (double*)mesh->coords[i];
      for (j=0; j<numNodes; j++)
      {
         ol_data->node_sets[0].cur_coord[i][j] = (double)coordD[j];
      }
   }


   /*******************
    * Load Element Set
    *******************/
   numElemSets = 1;
   zones = mesh->zones;
   ol_data->num_element_sets = numElemSets;
   ol_data->element_sets = (ol_element_set*)malloc(numElemSets*
                           sizeof(ol_element_set));
   ol_data->element_sets[0].num_elements = zones->nzones;
   ol_data->element_sets[0].nnodes       = 8;
   ol_data->element_sets[0].nips         = 1;
   ol_data->element_sets[0].numVars      = 12;

   /* Read Zone List */
   lnodelist = zones->nzones*numNodesPerElem;
   ol_data->element_sets[0].conn = (int*)malloc(lnodelist*sizeof(int));
   for (i = 0; i < lnodelist; i++)
   {
      ol_data->element_sets[0].conn[i] = zones->nodelist[i];
   }


   /*******************
    * Load Annotations
    *******************/
   /* Get the annotations we may have passed along */
   /* Currently, nothing done with this. This is where Global node maps reside */
   DBShowErrors(DB_NONE, NULL);
   annotations = DBGetCompoundarray(domfile, "ANNOTATION_INT");
   DBShowErrors(DB_TOP, NULL);

   if (annotations != NULL)
   {
      if ((!strcmp(annotations->elemnames[0], "GLOBAL_NODE_MAP") == 0) ||
            (!strcmp(annotations->elemnames[1], "GLOBAL_ELEM_MAP") == 0))
      {
      }
   }


   ReadOverlinkFieldvars(domfile, ol_data);

   ReadOverlinkRegions(domfile, ol_data);

   if (annotations)
      DBFreeCompoundarray(annotations);
} /* read_overlink_domain */




/**************************************************************************
 * ReadOverlinkFieldvars
 * @author Rob Neely, Gary Friedman
 * @purpose Reads the field data from the file
 * @param domfile The silo file pointer
 **************************************************************************/
static void ReadOverlinkFieldvars(DBfile *domfile,
                                  overlink_data *ol_data)
{
   int i, j;
   int centering;
   int sane = 0;
   DBtoc *toc = DBGetToc(domfile);
   int numNodalVars = 0;
   int numZonalVars = 0;
   int numAnnotations = 0,  numAnnotationsNodal = 0,  numAnnotationsZonal = 0;
   int valIndex = 0;

   DBcompoundarray *annotations;

   int idxN = 0;
   int idxZ = 0;

   int numNodes = ol_data->node_sets[0].num_nodes;
   int numElems = ol_data->element_sets[0].num_elements;

   int idx;
   char tempName[64];

   int    *intPtr;
   float  *fltPtr=NULL;
   double *dblPtr=NULL;

   int    trueType=NULL;

   /* Count Number of Nodal & Zonal Variables */
   numNodalVars = numZonalVars = 0;
   for(i=0 ; i<toc->nucdvar ; i++)
   {
      const char *tocName = toc->ucdvar_names[i];
      DBucdvar *ucdvar = DBGetUcdvar(domfile, tocName);

      /*printf("%s %s %s\n", tocName, CenteringStr(ucdvar->centering),
        DataTypeStr(ucdvar->datatype));*/

      sane = 0;
      switch (ucdvar->centering)
      {
         case DB_NODECENT:
            numNodalVars++;
            break;
         case DB_ZONECENT:
            numZonalVars++;
            break;
      }
   }

   /* Count number of annotation variables */
   DBShowErrors(DB_NONE, NULL);
   annotations = DBGetCompoundarray(domfile, "ANNOTATION_INT");
   if (annotations != NULL)
   {
      numAnnotations += annotations->nelems;

      for (i=0; i<annotations->nelems; i++)
      {
         if (strstr(annotations->elemnames[i], "/NODAL/"))
            numAnnotationsNodal++;
         if (strstr(annotations->elemnames[i], "/ZONAL/"))
            numAnnotationsZonal++;
      }
   }


   annotations = DBGetCompoundarray(domfile, "ANNOTATION_FLOAT");
   if (annotations != NULL)
   {
      numAnnotations += annotations->nelems;

      for (i=0; i<annotations->nelems; i++)
      {
         if (strstr(annotations->elemnames[i], "/NODAL/"))
            numAnnotationsNodal++;
         if (strstr(annotations->elemnames[i], "/ZONAL/"))
            numAnnotationsZonal++;
      }
   }

   annotations = DBGetCompoundarray(domfile, "ANNOTATION_DOUBLE");
   if (annotations != NULL)
   {
      numAnnotations += annotations->nelems;

      for (i=0; i<annotations->nelems; i++)
      {
         if (strstr(annotations->elemnames[i], "/NODAL/"))
            numAnnotationsNodal++;
         if (strstr(annotations->elemnames[i], "/ZONAL/"))
            numAnnotationsZonal++;
      }
   }

   ol_data->node_sets[0].numVars    = numNodalVars + numAnnotationsNodal;
   ol_data->element_sets[0].numVars = numZonalVars + numAnnotationsZonal;
   ol_data->numVars = numNodalVars  + numZonalVars + numAnnotations;

   ol_data->node_sets[0].varSet    = (ol_var*)malloc((numNodalVars+numAnnotationsNodal)*sizeof(ol_var));
   ol_data->element_sets[0].varSet = (ol_var*)malloc((numZonalVars+numAnnotationsZonal)*sizeof(ol_var));


   /* First load all Int and Float Annotations */
   ol_var* varSet; /* set of numVars */

   valIndex = 0;
   annotations = DBGetCompoundarray(domfile, "ANNOTATION_INT");
   if (annotations)
      intPtr = (int *) annotations->values;

   idxN = 0;
   idxZ = 0;
   if (annotations)
      for (i=0; i<annotations->nelems; i++)
      {
         if (strstr(annotations->elemnames[i], "/NODAL/"))
         {
            varSet = ol_data->node_sets[0].varSet; /*set to node varSet*/
            strcpy(tempName, &annotations->elemnames[i][7]);
            varSet[idxN].varName         = strdup(tempName);
            varSet[idxN].annotation_flag = TRUE;
            varSet[idxN].datatype        = DB_INT;
            varSet[idxN].lenval          = annotations->elemlengths[i];
            varSet[idxN].val             = (double*)malloc(varSet[idxN].lenval*sizeof(double));
            for (j = 0; j < varSet[idxN].lenval; j++)
            {
               varSet[idxZ].val[j] =  (double) intPtr[valIndex++];
            }

            idxN++;
         }

         if (strstr(annotations->elemnames[i], "/ZONAL/"))
         {
            varSet = ol_data->element_sets[0].varSet; /*set to element varSet*/
            strcpy(tempName, &annotations->elemnames[i][7]);
            varSet[idxZ].varName         = strdup(tempName);
            varSet[idxZ].annotation_flag = TRUE;
            varSet[idxZ].datatype        = DB_INT;
            varSet[idxZ].lenval          = annotations->elemlengths[i];
            varSet[idxZ].val             = (double*)malloc(varSet[idxZ].lenval*sizeof(double));
            for (j = 0; j < varSet[idxZ].lenval; j++)
            {
               varSet[idxZ].val[j] =  (double) intPtr[valIndex++];
            }

            idxZ++;
         }
      }

   valIndex = 0;
   annotations = DBGetCompoundarray(domfile, "ANNOTATION_FLOAT");

   /* Determine the true data type of the vars - DB_FLOAT or DB_DOUBLE */
   fltPtr=NULL;
   dblPtr=NULL;
   if (annotations)
   {

      if ( annotations->datatype==DB_FLOAT )
         fltPtr = (float *) annotations->values;
      else
         dblPtr = (double *) annotations->values;
   }

   /* Currently we need to just support doubles for the Annotations so force to
    * double type.
    */

   if (annotations)
      for (i=0; i<annotations->nelems; i++)
      {
         if (strstr(annotations->elemnames[i], "/NODAL/"))
         {
            varSet = ol_data->node_sets[0].varSet; /*set to node varSet*/
            strcpy(tempName, &annotations->elemnames[i][7]);
            varSet[idxN].varName         = strdup(tempName);
            varSet[idxN].annotation_flag = TRUE;
            varSet[idxN].lenval          = annotations->elemlengths[i];

            varSet[idxN].val             = (double*)malloc(varSet[idxN].lenval*sizeof(double));

            if ( fltPtr )
            {
               varSet[idxN].datatype = DB_FLOAT;
               for (j = 0; j < varSet[idxN].lenval; j++)
               {
                  varSet[idxZ].val[j] = (double) fltPtr[valIndex++];
               }
            }
            else
            {
               varSet[idxN].datatype = DB_DOUBLE;
               for (j = 0; j < varSet[idxN].lenval; j++)
               {
                  varSet[idxZ].val[j] = (double) dblPtr[valIndex++];
               }
            }

            idxN++;
         }

         if (strstr(annotations->elemnames[i], "/ZONAL/"))
         {
            varSet = ol_data->element_sets[0].varSet; /*set to element varSet*/
            strcpy(tempName, &annotations->elemnames[i][7]);
            varSet[idxZ].varName         = strdup(tempName);
            varSet[idxZ].annotation_flag = TRUE;
            varSet[idxZ].datatype        = DB_FLOAT;
            varSet[idxZ].lenval          = annotations->elemlengths[i];
            varSet[idxZ].val             = (double*)malloc(varSet[idxZ].lenval*sizeof(double));

            if ( fltPtr )
            {
               varSet[idxZ].datatype = DB_FLOAT;
               for (j = 0; j < varSet[idxZ].lenval; j++)
               {
                  varSet[idxZ].val[j] =  (double) fltPtr[valIndex++];
               }
            }
            else
            {
               varSet[idxZ].datatype = DB_DOUBLE;
               for (j = 0; j < varSet[idxZ].lenval; j++)
               {
                  varSet[idxZ].val[j] =  (double) dblPtr[valIndex++];
               }
            }

            idxZ++;
         }
      }

   valIndex = 0;
   annotations = DBGetCompoundarray(domfile, "ANNOTATION_DOUBLE");
   if (annotations)
      dblPtr = (double *) annotations->values;

   if (annotations)
      for (i=0; i<annotations->nelems; i++)
      {
         if (strstr(annotations->elemnames[i], "/NODAL/"))
         {
            varSet = ol_data->node_sets[0].varSet; /*set to node varSet*/
            strcpy(tempName, &annotations->elemnames[i][7]);
            varSet[idxN].varName         = strdup(tempName);
            varSet[idxN].annotation_flag = TRUE;
            varSet[idxN].datatype        = DB_DOUBLE;
            varSet[idxN].lenval          = annotations->elemlengths[i];
            varSet[idxN].val             = (double*)malloc(varSet[idxN].lenval*sizeof(double));
            for (j = 0; j < varSet[idxN].lenval; j++)
            {
               varSet[idxZ].val[j] =  (double) dblPtr[valIndex++];
            }

            idxN++;
         }

         if (strstr(annotations->elemnames[i], "/ZONAL/"))
         {
            varSet = ol_data->element_sets[0].varSet; /*set to element varSet*/
            strcpy(tempName, &annotations->elemnames[i][7]);
            varSet[idxZ].varName         = strdup(tempName);
            varSet[idxZ].annotation_flag = TRUE;
            varSet[idxZ].datatype        = DB_DOUBLE;
            varSet[idxZ].lenval          = annotations->elemlengths[i];
            varSet[idxZ].val             = (double*)malloc(varSet[idxZ].lenval*sizeof(double));
            for (j = 0; j < varSet[idxZ].lenval; j++)
            {
               varSet[idxZ].val[j] =  (double) dblPtr[valIndex++];
            }

            idxZ++;
         }
      }

   DBShowErrors(DB_TOP, NULL);

   /* Load nodal and zonal variables */
   for(i=0 ; i<toc->nucdvar ; i++)
   {
      const char *tocName = toc->ucdvar_names[i];
      DBucdvar *ucdvar = DBGetUcdvar(domfile, tocName);

      sane = 0;
      switch (ucdvar->centering)
      {
         case DB_NODECENT:
            varSet = ol_data->node_sets[0].varSet; /*set to node varSet*/

            if (ucdvar->nels != numNodes)   /*Error condition*/
            {
               printf("mismatch in number of values vs. number of nodes\n",
                      ucdvar->nels, numNodes);
               sane = 1;
            }

            else   /*valid node var*/
            {
               varSet[idxN].val  = (double*)malloc(numNodes*sizeof(double));
               varSet[idxN].varName = strdup(tocName);
               for (j = 0; j < numNodes; j++)
               {
                  varSet[idxN].val[j] = ((double**)ucdvar->vals)[0][j];
               }

               /* Mixed part */
               varSet[idxN].mixlen = ucdvar->mixlen;
               if (ucdvar->mixlen > 0)
               {
                  varSet[idxN].mixvals  = (double*)malloc(ucdvar->mixlen*sizeof(double));
                  for (j = 0; j < ucdvar->mixlen; j++)
                  {
                     varSet[idxN].mixvals[j] = ((double**)ucdvar->mixvals)[0][j];
                  }
               }

               idxN++;
            } /*end valid node var*/

            break;

         case DB_ZONECENT:
            varSet = ol_data->element_sets[0].varSet; /*set to element varSet*/

            if (ucdvar->nels != numElems)   /*Error condition*/
            {
               printf("mismatch in number of values vs. number of elements\n",
                      ucdvar->nels, numElems);
               sane = 1;
            }

            else   /*Valid element var*/
            {
               varSet[idxZ].val  = (double*)malloc(numElems*sizeof(double));
               varSet[idxZ].varName = strdup(tocName);

               for (j=0; j<numElems; j++)
               {
                  varSet[idxZ].val[j] = ((double**)ucdvar->vals)[0][j];
               }

               /* Mixed part */
               varSet[idxZ].mixlen = ucdvar->mixlen;
               if (ucdvar->mixlen > 0)
               {
                  varSet[idxZ].mixvals = (double*)malloc(ucdvar->mixlen*sizeof(double));
                  for (j = 0; j < ucdvar->mixlen; j++)
                  {
                     varSet[idxZ].mixvals[j] = ((double**)ucdvar->mixvals)[0][j];
                  }
               }

               idxZ++;
            } /*end valid element var*/

            break;

         default:
            printf("Unrecognized centering\n");
      } /* end switch on centering */

      if (sane==1)
      {
         printf("Skipping %s %s %s\n", tocName, CenteringStr(ucdvar->centering),
                DataTypeStr(ucdvar->datatype));
         continue;
      }

      DBFreeUcdvar(ucdvar);

   } /* end loop over all variables */

}





/**************************************************************************
 * ReadOverlinkRegions
 * @author Rob Neely
 * @purpose Reads a "material" object from the silo file
 * @param domfile The silo file pointer
 **************************************************************************/
static void ReadOverlinkRegions(DBfile *domfile,
                                overlink_data *ol_data)
{
   int i, j;
   DBmaterial *material;
   int numMat;
   int pure, mixed;
   int* regionId;
   int numElem = ol_data->element_sets[0].num_elements;
   int numMixEntries = 0;

   material = DBGetMaterial(domfile, "MATERIAL");
   if (material == NULL)
   {
      printf("ERROR: ReadOverlinRegions: Failed to read material\n");
   }

   /* Regions */
   numMat = material->nmat;
   ol_data->element_sets[0].numRegs = numMat;
   ol_data->element_sets[0].regionId = (int*)malloc(numMat*sizeof(int));
   for (i = 0; i < numMat; i++)
   {
      ol_data->element_sets[0].regionId[i] = material->matnos[i];
   }

   /* Region List */
   ol_data->element_sets[0].matList = (int*)malloc(numElem*sizeof(int));
   for (i = 0; i < numElem; i++)
   {
      ol_data->element_sets[0].matList[i] = material->matlist[i];
   }

   /* Mix Region Lists */
   numMixEntries = material->mixlen;
   ol_data->element_sets[0].numMixEntries = numMixEntries;
   if (numMixEntries > 0)
   {
      ol_data->element_sets[0].mixVF =
         (double*)malloc(ol_data->element_sets[0].numMixEntries*sizeof(double));
      ol_data->element_sets[0].mixNext =
         (int*)malloc(ol_data->element_sets[0].numMixEntries*sizeof(int));
      ol_data->element_sets[0].mixMat =
         (int*)malloc(ol_data->element_sets[0].numMixEntries*sizeof(int));
      ol_data->element_sets[0].mixZone =
         (int*)malloc(ol_data->element_sets[0].numMixEntries*sizeof(int));
      for (i = 0 ; i < numMixEntries; i++)
      {
         ol_data->element_sets[0].mixVF[i] = ((double*)material->mix_vf)[i];
         ol_data->element_sets[0].mixNext[i] = material->mix_next[i];
         ol_data->element_sets[0].mixMat[i] = material->mix_mat[i];
         ol_data->element_sets[0].mixZone[i] = material->mix_zone[i];
      }
   }
   else
   {
      ol_data->element_sets[0].mixVF = NULL;
      ol_data->element_sets[0].mixNext = NULL;
      ol_data->element_sets[0].mixMat = NULL;
      ol_data->element_sets[0].mixZone = NULL;
   }

#if 0
   ol_data->element_sets[0].numRegs = numMat;
   ol_data->element_sets[0].regSet = (ol_reg*)malloc(numMat*sizeof(ol_reg));

   for(i=0 ; i<numMat ; i++)   /*loop over regions */
   {
      char* matName ;
      int matNum; /* material number */

      ol_reg regSet = ol_data->element_sets[0].regSet[i];

      /* matName = strdup(material->matnames[i]); */
      /* ol_data->element_sets[0].regSet[i].regionName = strdup(matName); */

      matNum = material->matnos[i]; /* material number */
      regSet.regionId = matNum;

      /****************************
       * Count Pure/Mixed Elements
       ****************************/
      pure  = 0;  /* count of pure elements with this material */
      mixed = 0;  /* count of mixed elements with this material */
      for (j = 0; j < material->dims[0]; j++)   /*loop over elements*/
      {

         /* Pure */
         if (material->matlist[j] == matNum)
         {
            pure++;
         }

         /* Mixed */
         else if (material->matlist[j] < 0)
         {
            int ndx =  - material->matlist[j] - 1; /*index into mix arrays*/

            /* Is current region in this element */
            do
            {
               if (material->mix_mat[ndx] == matNum)   /*in element*/
               {
                  mixed++ ;
                  ndx = 0 ;  /* Sort of like a 'break' */
               }
               else
               {
                  ndx = material->mix_next[ndx] ;
                  if (ndx != 0)
                     ndx-- ; /* zero based offset arrays */
               }
            }
            while(ndx != 0) ;

         } /* end if zone pure or mixed */
      } /* end loop over elements (j) */

      /* Allocate storage for pure/mixed elements */
      ol_data->element_sets[0].regSet[i].numPureElem = pure;
      ol_data->element_sets[0].regSet[i].numMixElem = mixed;

      ol_data->element_sets[0].regSet[i].pureElems =
         (int*)malloc(pure*sizeof(int));
      ol_data->element_sets[0].regSet[i].mixElems =
         (int*)malloc(mixed*sizeof(int));
      ol_data->element_sets[0].regSet[i].mixElemVF =
         (double*)malloc(mixed*sizeof(double));

      /***************************
       * Fill Pure/Mixed Elements
       ***************************/
      pure  = 0;
      mixed = 0;
      for (j=0; j<material->dims[0]; j++)   /*loop over elements*/
      {

         /* Pure */
         if (material->matlist[j] == matNum)
         {
            ol_data->element_sets[0].regSet[i].pureElems[pure] = j;
            pure++ ;
         }

         /* Mixed */
         else if (material->matlist[j] < 0)
         {
            int ndx =  - material->matlist[j] - 1; /*index into mix arrays*/

            /* Is current region in this element */
            do
            {
               if(material->mix_mat[ndx] == matNum)   /* in element*/
               {
                  int zone = material->mix_zone[ndx];
                  /*ol_data->element_sets[0].regSet[i].mixElems[mixed] = j;*/
                  ol_data->element_sets[0].regSet[i].mixElems[mixed] = zone;
                  ol_data->element_sets[0].regSet[i].mixElemVF[mixed] =
                     ((double*)material->mix_vf)[ndx];
                  mixed++ ;
                  ndx = 0 ;  /* Sort of like a 'break' */
               }
               else
               {
                  ndx = material->mix_next[ndx] ;
                  if (ndx != 0)
                     ndx-- ;  /* zero based offset arrays */
               }
            }
            while(ndx != 0) ;

         } /* end if zone pure or mixed */
      } /* end loop over zones (j) */

   } /* end loop over materials (i) */
#endif
   DBFreeMaterial(material);
}

/* Print Help Routines */
char* CenteringStr(int centering)
{
   if (centering == DB_NODECENT)
   {
      return ("Node Centered");
   }
   else if (centering == DB_ZONECENT)
   {
      return("Element Centered");
   }
   else
   {
      return("Unknown Centering");
   }
}

char* DataTypeStr(int datatype)
{
   if (datatype == DB_FLOAT)
   {
      return ("Float");
   }
   else if (datatype == DB_DOUBLE)
   {
      return("Double");
   }
   else
   {
      return("Unknown data type");
   }
}
