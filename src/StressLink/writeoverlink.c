#include "pasitP.h"
#include "silo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

/*******************
 * Local Prototypes
 *******************/
static void WriteOverlinkTopLevel(DBfile *dbfile,
                                  overlink_data* olinkd,
                                  const char *olinkdirname,
                                  int *errorCode);

static void WriteOverlinkDomain(DBfile *dbdomfile,
                                const char* olinkdirname,
                                overlink_data* olinkd);

static void WriteOverlinkRegions(DBfile *dbdomfile,
                                 overlink_data *olinkd);

static void WriteOverlinkDomainVars(DBfile *dbdomfile,
                                    overlink_data *olinkd);

/**************************************************************************
 * write_overlink
 * @author Gary Friedman
 * @purpose Writes the overlink files from the overlink data structure.
 *          Currently, we will write overlink as multiple files with a top
 *          level file (OvlTop.silo) and one file per domain (domainX.silo)
 * @param olinkdirname The name of the subdirectory for writing this file
 **************************************************************************/
void write_overlink(overlink_data* olinkd,
                    const char *olinkdirname,
                    int *errorCode)
{
   int silodriver = DB_HDF5;
   DBfile *dbfile;
   mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP ; /* mode = 750 */
   int numDomains = 1;
   int i, j;

   /* Currently, we will always write overlink as multiple files with a top
      level overlink file (OvlTop.silo) and domain files, one file per domain
      (domainX.silo) */

   printf("\nWriting overlink file '%s'\n\n", olinkdirname);

   /********************************
    * Write Overlink Top Level File
    ********************************/
   /* Create/Move top level overlink directory */
   if (mkdir(olinkdirname, mode) > 0)
   {
      printf("writeoverlink: mkdir(%s...) failed\n", olinkdirname);
   }
   if (chdir(olinkdirname) != 0)
   {
      perror("chdir in readoverlink");
   }

   /* Create OvlTop.silo file, which contains metadata */
   if ((dbfile = DBCreate("OvlTop.silo", DB_CLOBBER, DB_LOCAL,
                          NULL, silodriver)) == NULL)
   {
      printf("Failed to open %s/OvlTop.silo\n", olinkdirname);
   }

   WriteOverlinkTopLevel(dbfile, olinkd, olinkdirname, errorCode);

   DBClose(dbfile);

   /******************************
    * Write Domain Overlink Files
    ******************************/
   for (i = 0; i < numDomains; i++)   /* Domain loop */
   {
      char domname[40];
      DBfile *dbdomfile;
      sprintf(domname, "domain%d.silo", i);
      if ((dbdomfile = DBCreate(domname, DB_CLOBBER, DB_LOCAL, NULL, silodriver)) == NULL)
      {
         printf("Failed to create file %s\n", domname);
      }
      WriteOverlinkDomain(dbdomfile, olinkdirname, olinkd);

      DBClose(dbdomfile);
   } /* End Domain loop */

}

/**************************************************************************
 * WriteOverlinkTopLevel
 * @author Gary Friedman
 * @purpose Writes the top level overlink file
 * @param dbfile the silo file pointer
 * @param olinkd The internal overlink data structure
 * @param olinkdirname The name of the subdirectory for writing this file
 **************************************************************************/

static void WriteOverlinkTopLevel(DBfile *dbfile,
                                  overlink_data* olinkd,
                                  const char *olinkdirname,
                                  int *errorCode)
{
   int i, j, idx;
   int numDom = 1;
   mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP ; /* mode = 750 */
   char **names, **prepend, **varNames;
   int  *types ;
   DBoptlist *optlist ;
   int maxOpts = 10 ;

   /* for multivars */
   int numVars, numDomainVars;
   int* varAttr;
   int* varLen;
   int pos;

   /* Build an options list */
   optlist = DBMakeOptlist(maxOpts) ;
   /* TBD */

   /*
   WriteOverlinkBoundingBoxes(problem->boundingBoxes,
                              problem->decomp->numGlobalDomains,
                              file) ;
   */

   /**********************
    * VARIABLE ATTRIBUTES
    **********************/
   numVars = olinkd->numVars;
   numDomainVars = 0;
   pos = 0;
   /*node vars*/
   varNames = (char**)malloc(numVars * sizeof(char*)) ;
   varAttr  = (int*)  malloc(numVars * sizeof(int) * 5);
   varLen   = (int*)  malloc(numVars * sizeof(int));
   for (i = 0; i < olinkd->node_sets[0].numVars; i++)
   {
      if (olinkd->node_sets[0].varSet)
         varNames[numDomainVars] = (char*)
                                   strdup(olinkd->node_sets[0].varSet[i].varName);
      varLen[numDomainVars] = 5;
      varAttr[pos++] = DB_NODECENT;       /*centering*/
      varAttr[pos++] = 0;   /*scaling*/
      varAttr[pos++] = 0; /*linking*/
      varAttr[pos++] = 0;   /*unused*/
      varAttr[pos++] = DB_DOUBLE;      /*data type*/
      numDomainVars++;
   }
   /*zone (element) vars*/
   for (i = 0; i < olinkd->element_sets[0].numVars; i++)
   {
      if (olinkd->element_sets[0].varSet)
         varNames[numDomainVars] = (char*)
                                   strdup(olinkd->element_sets[0].varSet[i].varName);
      varLen[numDomainVars] = 5;
      varAttr[pos++] = DB_ZONECENT;       /*centering*/
      varAttr[pos++] = 0;   /*scaling*/
      varAttr[pos++] = 0; /*linking*/
      varAttr[pos++] = 0;   /*unused*/
      varAttr[pos++] = DB_DOUBLE;      /*data type*/
      numDomainVars++;
   }

   /*    DBPutCompoundarray(dbfile, "VAR_ATTRIBUTES", varNames, varLen,
    numDomainVars, varAttr, numDomainVars*5, DB_INT, NULL) ; */

   free(varLen) ;
   free(varAttr) ;

   /*********************
    * MULTIBLOCK OBJECTS
    *********************/
   /* Build the "prepend" string which will be prepended to all the
    * multiblock objects */
   prepend = (char**)malloc(numDom * sizeof(char*)) ;
   for(i=0 ; i<numDom ; i++)
   {
      prepend[i] = (char*)malloc(80) ;
      sprintf(prepend[i], "%s/domain%d.silo:", olinkdirname, i) ;
   }

   /* MULTI MESH */
   names = (char**)malloc(numDom * sizeof(char*)) ;
   types = (int*  )malloc(numDom * sizeof(int)) ;
   for (i=0; i<numDom; i++)
   {
      names[i] = (char*)malloc(100);
      types[i] = DB_UCDMESH;
      sprintf(names[i], "%s/MESH", prepend[i]);
   }
   DBPutMultimesh(dbfile, "MMESH", numDom, names, types, optlist);

   /* MULTI MATERIALS */
   for(i=0 ; i<numDom ; i++)
   {
      sprintf(names[i], "%s/MATERIAL", prepend[i]) ;
   }
   /* Build a multi material options list */
   DBPutMultimat(dbfile, "MMATERIAL", numDom, names, optlist) ;

   /* MULTI VARS */
   idx = 0;

   /* Nodal */
   for (i = 0; i < olinkd->node_sets[0].numVars; i++)
   {
      char* tempName = (char*)
                       strdup(olinkd->node_sets[0].varSet[i].varName);
      for(j=0 ; j<numDom ; j++)
      {
         types[j] = DB_UCDVAR ;
         sprintf(names[j], "%s/%s", prepend[j], tempName) ;
      }
      free (tempName);
      DBPutMultivar(dbfile, varNames[idx++], numDom, names, types, optlist) ;
   }

   /* Zonal */
   for (i = 0; i < olinkd->element_sets[0].numVars; i++)
   {
      char* tempName = (char*)
                       strdup(olinkd->element_sets[0].varSet[i].varName);
      for(j=0 ; j<numDom ; j++)
      {
         types[j] = DB_UCDVAR ;
         sprintf(names[j], "%s/%s", prepend[j], tempName) ;
      }
      free (tempName);
      DBPutMultivar(dbfile, varNames[idx++], numDom, names, types, optlist) ;
   }

   DBFreeOptlist(optlist) ;

   for(i=0 ; i<numDom ; i++)
   {
      free(names[i]);
      free(prepend[i]);
      free(types) ;
   }
   /*
      WriteOverlinkAnnotations(problem, fileType, file) ;
   */
}

/**************************************************************************
 * WriteOverlinkDomain
 * @author Gary Friedman
 * @purpose Writes one domain's worth of data to the overlink file
 * @param dbfile the silo file pointer
 * @param olinkdirname The name of the subdirectory for writing this file
 **************************************************************************/

static void WriteOverlinkDomain(DBfile *dbdomfile,
                                const char* olinkdirname,
                                overlink_data *olinkd)
{
   static const char *coordnames[3] = {"x", "y", "z"};
   static const char *zlname = "zonelist";
   int nzones;
   int ndim = 3;
   int origin = 0;
   int shapesize[1];
   int shapecnt[1];
   int nshapes = 1;
   int nodesPerZone = 8;
   int lnodelist;
   int i, j, idx;

   DBSetDeprecateWarnings(0);

   /******************
    * Write Zone List
    ******************/
   shapesize[0] = nodesPerZone;

   ol_node_set *nodeSet = olinkd->node_sets;
   ol_element_set *elemSet = olinkd->element_sets;

   nzones = elemSet->num_elements;
   shapecnt[0] = nzones;

   lnodelist = nzones * nodesPerZone;

   if ((DBPutZonelist(dbdomfile, zlname, nzones, ndim,
                      elemSet->conn,
                      lnodelist,
                      origin,
                      shapesize,
                      shapecnt,
                      nshapes)) == -1)
   {
      printf("WriteOverlinkDomain: DBPutZonelist failed.");
   }

   /******************
    * Write Face List
    ******************/
   /* IS A FACE LIST NEEDED FOR THIS APPLICATION ?? */
   /*   flname = WriteCommonFacelist(domain->elems,
        domain->elems->fieldInt("regionNumber"),
        domain->regionList->allMixed->numElem,
        false, file) ;
   */


   /********************************
    * Write Mesh Data (Coordinates)
    ********************************/
   DBPutUcdmesh(dbdomfile, "MESH", 3, (char**)coordnames,
                (float**)nodeSet[0].cur_coord,
                nodeSet[0].num_nodes,
                elemSet[0].num_elements,
                zlname,
                NULL,   /*flname*/
                DB_DOUBLE,
                NULL);  /*optlist*/

   /********************
    * Write Region List
    ********************/
   WriteOverlinkRegions(dbdomfile, olinkd);


   /********************
    * Write Domain Vars
    ********************/
   WriteOverlinkDomainVars(dbdomfile, olinkd);


} /* WriteOverlinkDomain */

/**************************************************************************
 * WriteOverlinkRegions
 * @author Gary Friedman
 * @purpose Writes one domain's worth of region data to the overlink file
 * @param dbfile the silo file pointer
 * @param olinkd The internal structure containing overlink data
 **************************************************************************/
static void WriteOverlinkRegions(DBfile *dbdomfile,
                                 overlink_data *olinkd)
{
   int i, j, idxM;
   ol_element_set *elemSet = olinkd->element_sets;

   int* matnos;
   int mixlen;
   int ndims, dims[1];
   int* matlist;
   int *mix_next, *mix_mat, *mix_zone;
   double *mix_vf;
   int numElems = elemSet[0].num_elements;
   int numRegs  = elemSet[0].numRegs;

   ndims = 1;
   dims[0] = numElems;
#if 0
   matnos = (int*)malloc(numRegs*sizeof(int));
   mixlen = 0;
   for (i = 0; i < numRegs; i++)
   {
      matnos[i] = elemSet[0].regSet[i].regionId;
      mixlen += elemSet[0].regSet[i].numMixElem;
   }

   matlist  = (int*)malloc(numElems*sizeof(int));
   mix_next = (int*)malloc(numElems*sizeof(int));
   mix_mat  = (int*)malloc(numElems*sizeof(int));
   mix_zone = (int*)malloc(numElems*sizeof(int));
   mix_vf   = (double*)malloc(numElems*sizeof(double));

   /* Initialize matlist to zeros */
   for (i = 0; i < numElems; i++)
   {
      matlist[i] = 0;
   }

   /* Set matlist */
   idxM = 0; /* index into mix_* arrays */
   for (i = 0; i < numRegs; i++)   /* loop over regions */
   {
      ol_reg regSet = elemSet[0].regSet[i];

      /* pure elements */
      for (j = 0; j < regSet.numPureElem; j++)
      {
         matlist[regSet.pureElems[j]] = regSet.regionId;
      } /* end loop over pure elements (j) */

      /* mixed elements */
      for (j = 0; j < regSet.numMixElem; j++)
      {
         int elemIdx = regSet.mixElems[j]-1;
         if (matlist[elemIdx] == 0)   /*first region in this element*/
         {
            matlist[elemIdx] = -(idxM+1); /*one base index into mix_* lists*/
            mix_next[idxM] = 0;
            mix_mat[idxM]  = regSet.regionId;
            mix_zone[idxM] = regSet.mixElems[j];
            mix_vf[idxM]   = regSet.mixElemVF[j];
            idxM++;
         }
         else   /* not first region of this mixed element */
         {
            int curIdx = -(matlist[elemIdx]) - 1;  /*ptr into mix_* of 1st reg*/
            while (mix_next[curIdx] != 0)
            {
               curIdx = mix_next[curIdx];
            }

            mix_next[curIdx] = idxM;
            mix_next[idxM] = 0;
            mix_mat[idxM]  = regSet.regionId;
            mix_zone[idxM] = regSet.mixElems[j];
            mix_vf[idxM]   = regSet.mixElemVF[j];
            idxM++;
         }
      } /* end loop over mixed elements (j) */

   } /* end loop over regions (i) */

#if 1
   /* sanity check */
   {
      /* for sanity check */
      int pure = 0;
      int mixed = 0;
      double sumVF = 0.0;
      int idx;
      for (i = 0; i < numElems; i++)   /* loop over elements */
      {
         if (matlist[i] > 0)
         {
            pure++;
         }
         else if (matlist[i] < 0)
         {
            mixed++;
            sumVF = 0;
            idx = -(matlist[i])-1;
            printf("Elem[i]: mixed ",i);
            do
            {
               sumVF += mix_vf[idx];
               printf(" %e ", mix_vf[idx]);
               idx = mix_next[idx];
            }
            while (idx != 0);
            printf(" sum=%e\n", sumVF);
            if(sumVF != 1)
            {
               printf("sum vf elem[%d] = %e\n", i, (sumVF-1.));
            }
         }
         else
         {
            printf("matlist[%d]=%d\n",i,matlist[i]);
         }
      }
      printf("number pure=%d, number mixed=%d\n",pure,mixed);
   }
#endif
   DBPutMaterial(dbdomfile,
                 "MATERIAL", "MESH",
                 elemSet[0].numRegs,
                 matnos,
                 matlist,
                 dims, ndims,
                 mix_next, mix_mat, mix_zone, (float *)mix_vf,
                 mixlen, DB_DOUBLE, NULL);
#endif
   DBPutMaterial(dbdomfile,
                 "MATERIAL", "MESH",
                 elemSet[0].numRegs,
                 elemSet[0].regionId,
                 elemSet[0].matList,
                 dims, ndims,
                 elemSet[0].mixNext, elemSet[0].mixMat, elemSet[0].mixZone, (float *)elemSet[0].mixVF,
                 elemSet[0].numMixEntries, DB_DOUBLE, NULL);

}

/**************************************************************************
 * WriteOverlinkDomainVars
 * @author Gary Friedman
 * @purpose Writes one domain's worth of variable data  to the overlink file
 * @param dbfile the silo file pointer
 * @param olinkd The internal structure containing overlink data
 **************************************************************************/
static void WriteOverlinkDomainVars(DBfile *dbdomfile,
                                    overlink_data *olinkd)
{

   int i, j;
   ol_node_set *nodeSet = olinkd->node_sets;
   ol_element_set *elemSet = olinkd->element_sets;
   ol_var* varSet;
   int num_nodes=0, num_elements=0;

   int    lengths[4] = {0, 0, 0, 0};
   char   *annot_toc_names[4]= {"ANNOTATION_CHAR", "ANNOTATION_INT", "ANNOTATION_FLOAT", "ANNOTATION_DOUBLE"};
   char   **annot_names_int, **annot_names_flt, **annot_names_dbl;
   char   temp_name[64];

   int    annot_count_int=0, annot_count_flt=0, annot_count_dbl=0, annot_count=0;

   int    annot_count_nodal=0, annot_count_zonal=0;
   int    *annot_lens_int, *annot_lens_flt, *annot_lens_dbl, annot_toc_lens[1]= {1};

   int    *annot_data_int=NULL;
   float  *annot_data_flt=NULL;
   double *annot_data_dbl=NULL;
   int    annot_data_len_int=0, annot_data_len_flt=0, annot_data_len_dbl=0;
   int    annot_index_int=0, annot_data_index_int=0,
          annot_index_flt=0, annot_data_index_flt=0,
          annot_index_dbl=0, annot_data_index_dbl=0;

   int     status=OK;

   /* Nodal Variables */
   varSet    = nodeSet[0].varSet;
   num_nodes = nodeSet[0].num_nodes;

   for (i = 0; i < nodeSet[0].numVars; i++)
   {
      if (!varSet[i].annotation_flag)
         DBPutUcdvar1(dbdomfile, varSet[i].varName, "MESH",
                      (float*)varSet[i].val, nodeSet[0].num_nodes,
                      (float*)varSet[i].mixvals, varSet[i].mixlen,
                      DB_DOUBLE, DB_NODECENT, NULL);
   }

   /* Zonal Variables */
   varSet       = elemSet[0].varSet;
   num_elements = elemSet[0].num_elements;

   for (i = 0; i < elemSet[0].numVars; i++)
   {
      if (!varSet[i].annotation_flag)
         DBPutUcdvar1(dbdomfile, varSet[i].varName, "MESH",
                      (float*)varSet[i].val, elemSet[0].num_elements,
                      (float*)varSet[i].mixvals, varSet[i].mixlen,
                      DB_DOUBLE, DB_ZONECENT, NULL);
   }

   /* Annotations */

   /* First count annotations */

   /* Search for nodal annotation vars */
   varSet = nodeSet[0].varSet;
   annot_data_len_int = 0;
   annot_data_len_flt = 0;

   for (i = 0; i < nodeSet[0].numVars; i++)
   {
      if (varSet[i].annotation_flag)
      {
         annot_count_nodal++;
         if (varSet[i].datatype==DB_INT)
         {
            annot_count_int++;
            annot_data_len_int+=varSet[i].lenval;
         }
         if (varSet[i].datatype==DB_FLOAT)
         {
            annot_count_flt++;
            annot_data_len_flt+=varSet[i].lenval;
         }
         if (varSet[i].datatype==DB_DOUBLE)
         {
            annot_count_dbl++;
            annot_data_len_dbl+=varSet[i].lenval;
         }
      }
   }

   /* Search for zonal annotation vars */
   varSet = elemSet[0].varSet;
   for (i = 0; i < elemSet[0].numVars; i++)
   {
      if (varSet[i].annotation_flag)
      {
         annot_count_zonal++;

         if (varSet[i].datatype==DB_INT)
         {
            annot_count_int++;
            annot_data_len_int+=varSet[i].lenval;
         }
         if (varSet[i].datatype==DB_FLOAT)
         {
            annot_count_flt++;
            annot_data_len_flt+=varSet[i].lenval;
         }
         if (varSet[i].datatype==DB_DOUBLE)
         {
            annot_count_dbl++;
            annot_data_len_dbl+=varSet[i].lenval;
         }
      }
   }

   annot_count = annot_count_int + annot_count_flt;

   /* Allocate space for the annotation vars */
   annot_lens_int  = (int *) malloc(annot_count_int*sizeof(int));
   annot_lens_flt  = (int *) malloc(annot_count_flt*sizeof(int));
   annot_lens_dbl  = (int *) malloc(annot_count_dbl*sizeof(int));

   annot_names_int = (char **) malloc(annot_count_int*sizeof(char *));
   annot_names_flt = (char **) malloc(annot_count_flt*sizeof(char *));
   annot_names_dbl = (char **) malloc(annot_count_dbl*sizeof(char *));

   annot_data_int      = (int *)    malloc(annot_data_len_int*sizeof(int));
   annot_data_flt      = (float *)  malloc(annot_data_len_flt*sizeof(float));
   annot_data_dbl      = (double *) malloc(annot_data_len_dbl*sizeof(double));

   /* Load the data for the compound arrays write */

   annot_index_int      = 0;
   annot_data_index_int = 0;
   annot_index_flt      = 0;
   annot_data_index_flt = 0;

   varSet = nodeSet[0].varSet;
   for (i = 0; i < nodeSet[0].numVars; i++)
   {
      if (varSet[i].annotation_flag)
      {
         strcpy(temp_name, "/NODAL/");
         strcat(temp_name, varSet[i].varName);

         if (varSet[i].datatype==DB_INT)
         {
            annot_names_int[annot_index_int] = strdup(temp_name);
            annot_lens_int[annot_index_int]  = varSet[i].lenval;
            for (j=0; j<varSet[i].lenval; j++)
            {
               annot_data_int[annot_data_index_int++] = (int) varSet[i].val[j];
            }
            annot_index_int++;
         }
         if (varSet[i].datatype==DB_FLOAT)
         {
            annot_names_flt[annot_index_flt] = strdup(temp_name);
            annot_lens_flt[annot_index_flt]  = varSet[i].lenval;
            for (j=0; j<varSet[i].lenval; j++)
            {
               annot_data_flt[annot_data_index_flt++] = (float) varSet[i].val[j];
            }
            annot_index_flt++;
         }
         if (varSet[i].datatype==DB_DOUBLE)
         {
            annot_names_dbl[annot_index_dbl] = strdup(temp_name);
            annot_lens_dbl[annot_index_dbl]  = varSet[i].lenval;
            for (j=0; j<varSet[i].lenval; j++)
            {
               annot_data_dbl[annot_data_index_dbl++] = (double) varSet[i].val[j];
            }
            annot_index_dbl++;
         }
      }
   }

   varSet = elemSet[0].varSet;
   for (i = 0; i < elemSet[0].numVars; i++)
   {
      if (varSet[i].annotation_flag)
      {
         strcpy(temp_name, "/ZONAL/");
         strcat(temp_name, varSet[i].varName);

         if (varSet[i].datatype==DB_INT)
         {
            annot_names_int[annot_index_int] = strdup(temp_name);
            annot_lens_int[annot_index_int]  = varSet[i].lenval;
            for (j=0; j<varSet[i].lenval; j++)
            {
               annot_data_int[annot_data_index_int++] = (int) varSet[i].val[j];
            }
            annot_index_int++;
         }
         if (varSet[i].datatype==DB_FLOAT)
         {
            annot_names_flt[annot_index_flt] = strdup(temp_name);
            annot_lens_flt[annot_index_flt]  = varSet[i].lenval;
            for (j=0; j<varSet[i].lenval; j++)
            {
               annot_data_flt[annot_data_index_flt++] = (float) varSet[i].val[j];
            }
            annot_index_flt++;
         }
         if (varSet[i].datatype==DB_DOUBLE)
         {
            annot_names_dbl[annot_index_dbl] = strdup(temp_name);
            annot_lens_dbl[annot_index_dbl]  = varSet[i].lenval;
            for (j=0; j<varSet[i].lenval; j++)
            {
               annot_data_dbl[annot_data_index_dbl++] = (double) varSet[i].val[j];
            }
            annot_index_dbl++;
         }
      }
   }

   /* Write out the annotations */
   if (annot_count>0)
   {
      lengths[0] = 0;               /* Char */
      lengths[1] = annot_count_int; /* Int */
      lengths[2] = annot_count_dbl; /* Double */
      lengths[3] = annot_count_dbl; /* Double */

      status = DBPutCompoundarray(dbdomfile, "ANNOTATION_LENGTHS",
                                  annot_toc_names, annot_toc_lens, 3, (void *) lengths,
                                  3, DB_INT, NULL);

      if (lengths[1])
         status = DBPutCompoundarray(dbdomfile, "ANNOTATION_INT",
                                     annot_names_int, annot_lens_int, annot_count_int, (void *) annot_data_int,
                                     annot_data_len_int, DB_INT, NULL);
      if (lengths[2])
         status = DBPutCompoundarray(dbdomfile, "ANNOTATION_FLOAT",
                                     annot_names_dbl, annot_lens_dbl, annot_count_dbl, (void *) annot_data_dbl,
                                     annot_data_len_dbl, DB_FLOAT, NULL);

      /* !!! NOTE - Currently all FP annotations are assumed to be double - so identify just as
      *            a FLOAT.
      */
      if (lengths[3])
         status = DBPutCompoundarray(dbdomfile, "ANNOTATION_FLOAT",
                                     annot_names_dbl, annot_lens_dbl, annot_count_dbl, (void *) annot_data_dbl,
                                     annot_data_len_dbl, DB_DOUBLE, NULL);
   }


   /* Release temporary storage */
   free(annot_lens_int);
   free(annot_lens_flt);
   free(annot_lens_dbl);

   free(annot_data_int);
   free(annot_data_flt);
   free(annot_data_dbl);

   for (i = 0; i < annot_count_int; i++)
   {
      if (annot_names_int)
         if (annot_names_int[i])
            free(annot_names_int[i]);
   }

   for (i = 0; i < annot_count_flt; i++)
   {
      if (annot_names_flt)
         if (annot_names_flt[i])
            free(annot_names_flt[i]);
   }

   for (i = 0; i < annot_count_dbl; i++)
   {
      if (annot_names_dbl)
         if (annot_names_dbl[i])
            free(annot_names_dbl[i]);
   }

   free(annot_names_int);
   free(annot_names_flt);
   free(annot_names_dbl);
}


void write_overlink_text(overlink_data   *ol,
                         const char      *olinkfilename,
                         int             *errorCode)
{
   printf("\nWriting overlink text file '%s'\n", olinkfilename);


   FILE        *fp;
   int         i, j, k;
   int         field_count=0;

   int num_node_sets, num_nodes;
   ol_node_set *node_set;

   int num_element_sets, num_elements;
   ol_element_set *element_set;

   int control[10];

   /* Open the output text file */
   fp = fopen(olinkfilename, "w+");

   /* Write node set data */

   fprintf(fp, "Node Set Data:");

   num_node_sets = ol->num_node_sets;


   for (i=0;
         i< num_node_sets;
         i++)
   {
      node_set  = &ol->node_sets[i];
      num_nodes = node_set->num_nodes;

      fprintf(fp, "\tNum Nodes=%d", num_nodes);
      fprintf(fp, "\tControl=%d/%d/%d", control[0], control[1], control[2]);

      /* Print 20 labels per line */
      fprintf(fp, "\tLabels= ");
      field_count=0;
      for (j=0; j<num_nodes; j++)
      {
         for (k=0; k<20; k++)
         {
            fprintf(fp, "\t\t%d ");
            field_count++;
            if (field_count==num_nodes) break;
         }
         fprintf(fp, "\n");
      }

      /* Print 12 coords per line */
      fprintf(fp, "\tLabels= ");
      field_count=0;
      for (j=0; j<num_nodes*3; j++)
      {
         for (k=0; k<12; k++)
         {
            fprintf(fp, "\t\t%d ");
            field_count++;
            if (field_count==num_nodes*3) break;
         }
         fprintf(fp, "\n");
      }
   }
   fclose(fp);
}
