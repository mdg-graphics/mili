/*****************************************************************************
*
* Copyright (c) 2000 - 2011, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-442911
* All rights reserved.
*
* This file is  part of VisIt. For  details, see https://visit.llnl.gov/.  The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or other materials provided with the distribution.
*  - Neither the name of  the LLNS/LLNL nor the names of  its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED. IN  NO EVENT  SHALL LAWRENCE  LIVERMORE NATIONAL  SECURITY,
* LLC, THE  U.S.  DEPARTMENT OF  ENERGY  OR  CONTRIBUTORS BE  LIABLE  FOR  ANY
* DIRECT,  INDIRECT,   INCIDENTAL,   SPECIAL,   EXEMPLARY,  OR   CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

// ************************************************************************* //
//                              avtMiliFileFormat.C                          //
// ************************************************************************* //

/*
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - June 21, 2011: Modifications to support new features in
 *  .mili file such as specification of file path and SPH particles.
 *
 ************************************************************************/

// Compile Line: /usr/bin/c++ -I/usr/apps/mdg/include -L/usr/apps/mdg/lib -g -c avtMiliFileFormat.C
// Link Line:    /usr/bin/c++ MakeMili.o -L/usr/apps/mdg/lib -lmili -o  avtMiliFileFormat.exe

// #define DEBUG 1

#define MILI_VERSION "13.1"
#define DATE_VERSION "June 1, 2013"

#include <avtMiliFileFormat.h>

#include <vector>
#include <algorithm>
#include <string>
using std::getline;
#include <snprintf.h>
#include <visitstream.h>
#include <set>

extern "C" {
#include <mili_enum.h>
}

#include <vtkCellData.h>
#include <vtkCellTypes.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>

#include <Expression.h>

#include <avtCallback.h>
#include <avtDatabase.h>
#include <avtDatabaseMetaData.h>
#include <avtGhostData.h>
#include <avtMaterial.h>
#include <avtVariableCache.h>
#include <avtUnstructuredPointBoundaries.h>

#include <DebugStream.h>
#include <ImproperUseException.h>
#include <InvalidFilesException.h>
#include <InvalidVariableException.h>
#include <UnexpectedValueException.h>

#ifdef PARALLEL
#include <mpi.h>
#include <avtParallel.h>
#endif

using std::vector;
using std::pair;
using std::string;
using std::ifstream;

static const char *free_nodes_str = "Free_nodes";
static const char *no_free_nodes_str = "No_free_nodes-SAND";
static const int free_nodes_strlen = strlen(free_nodes_str);
static const int no_free_nodes_strlen = strlen(no_free_nodes_str);

static const char *part_nodes_str = "SPH_nodes";
static const int part_nodes_strlen = strlen(part_nodes_str);

static const char *dbc_nodes_str = "DBC_nodes";
static const int dbc_nodes_strlen = strlen(dbc_nodes_str);

// Mili geometry data
static const int n_elem_types = 9;
static const int elem_sclasses[n_elem_types] =
{
    M_TRUSS, M_BEAM, M_TRI, M_QUAD, M_TET, M_PYRAMID, M_WEDGE, M_HEX, M_PARTICLE
};
static int conn_count[n_elem_types] =
{
    2, 3, 3, 4, 4, 5, 6, 8, 1
};

#define Warn(msg) IssueWarning(msg, __LINE__)

bool miliLoadMessageDisplayed = false;
static void ErrorHandler( const char *, int);
static void MyInvalidVariableException( );


// ****************************************************************************
//  Method:  avtMiliFileFormat::IssueWarning
//
//  Purpose: Convenience method to issue warning messages. Manages number of
//      times a given warning message will be output
//
//  Programmer:  Mark C. Miller
//  Creation:    January 4, 2005
//
// ****************************************************************************
void
avtMiliFileFormat::IssueWarning(const char *msg, int key)
{
    if (warn_map.find(key) == warn_map.end())
    {
        warn_map[key] = 1;
    }
    else
    {
        warn_map[key]++;
    }

    if (warn_map[key] <= 5)
    {
        if (!avtCallback::IssueWarning(msg))
        {
            cerr << msg << endl;
        }
    }

    if (warn_map[key] == 5)
    {
        const char *smsg = "\n\nFurther warnings will be suppresed";
        if (!avtCallback::IssueWarning(smsg))
        {
            cerr << smsg << endl;
        }
    }
}

// ****************************************************************************
//  Constructor:  avtMiliFileFormat::avtMiliFileFormat
//
//  Arguments:
//    fname      the file name of one of the Mili files.
//
//  Programmer:  Hank Childs
//  Creation:    April 11, 2003
//
//  Modifications
//    Akira Haddox, Fri May 23 08:13:09 PDT 2003
//    Added in support for multiple meshes. Changed to MTMD.
//    Changed to read in .mili format files.
//
//    Akira Haddox, Tue Jul 22 15:34:40 PDT 2003
//    Initialized setTimesteps.
//
//    Akira Haddox, Fri Jul 25 11:09:13 PDT 2003
//    Added reading in of variable dimensions.
//
//    Akira Haddox, Mon Aug 18 14:33:15 PDT 2003
//    Added partition file support.
//
//    Hank Childs, Tue Jul 20 15:53:30 PDT 2004
//    Add support for more data types (float, double, char, int, etc).
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added initialization of structures having to do with free nodes mesh
//
//    Kathleen Bonnell, Wed Aug 5 17:44:22 PDT 2009
//    Test for windows-style slashes when scanning filename.
//
// ****************************************************************************

avtMiliFileFormat::avtMiliFileFormat(const char *fname)
    : avtMTMDFileFormat(fname)
{
    bool printMessage=TRUE;

#ifdef PARALLEL
    if ( PAR_Rank()!= 0 )
    {
        printMessage=FALSE;
    }
#endif

#ifdef MDSERVER
    if ( printMessage && !miliLoadMessageDisplayed )
    {
        printf("\nUsing MDG Visit Mili Plugin. Visit Version %s / Mili Version %s (%s)\n\n\n", VISIT_VERSION, MILI_VERSION, DATE_VERSION );
    }
#endif

    loadMiliInfo(fname);

    //
    // Code from GRIZ.
    //
    const char *p_c, *p_src;
    const char *p_root_start, *p_root_end;
    char *p_dest;
    char *path;
    char root[128];
    char path_text[256];

    /* Scan forward to end of name string. */
    for ( p_c = fname ; *p_c != '\0'; p_c++ )
    {
        ;
    }

    /* Scan backward to last non-slash character. */
    for ( p_c--; (*p_c == '/' || *p_c == '\\') && p_c != fname; p_c-- )
    {
        ;
    }
    p_root_end = p_c;

    /* Scan backward to last slash character preceding last non-slash char. */
    for ( ; (*p_c != '/' && *p_c != '\\') && p_c != fname; p_c-- )
    {
        ;
    }

    p_root_start = ( *p_c == '/' || *p_c == '\\' ) ? p_c + 1 : p_c;

    /* Generate the path argument to mc_open(). */
    if ( p_root_start == fname )
        /* No path preceding root name. */
    {
        path = NULL;
    }
    else
    {
        /* Copy path (less last slash). */

        path = path_text;

        for ( p_src = fname, p_dest = path_text;
                p_src < p_root_start - 1;
                *p_dest++ = *p_src++ )
        {
            ;
        }

        if ( p_src == fname )
            /* Path must be just "/".  If that's what the app wants... */
        {
            *p_dest++ = *fname;
        }

        *p_dest = '\0';
    }

    /* Generate root name argument to mc_open(). */
    for ( p_src = p_root_start, p_dest = root;
            p_src <= p_root_end;
            *p_dest++ = *p_src++ )
    {
        ;
    }
    *p_dest = '\0';

    //
    // If it ends in .m or .mili, strip it off.
    //
    int len = strlen(root);

    if (len > 4 && strcmp(&(root[len - 5]), ".mili") == 0)
    {
        root[len - 5] = '\0';
    }
    else if (len > 1 && strcmp(&(root[len - 2]), ".m") == 0)
    {
        root[len - 2] = '\0';
    }
    else
    {
        EXCEPTION1(InvalidFilesException, fname);
    }

    fampath=NULL;
    famroot = new char[strlen(root) + 1];
    strcpy(famroot, root);
    if (path)
    {
        fampath = new char[strlen(path) + 1];
        strcpy(fampath, path);
    }

    if ( strlen(filepath)>0 )
    {
        if (fampath)
        {
            delete [] fampath;
        }
        fampath = new char[strlen(filepath) + 1];
        strcpy( fampath, filepath );
    }

    free_nodes = NULL;
    free_nodes_ts = -1;
    num_free_nodes = 0;
    free_nodes_found = false;

    part_nodes = NULL;
    part_nodes_ts = -1;
    part_nodes_found = false;

    dbc_nodes = NULL;
    dbc_nodes_ts = -1;
    dbc_nodes_found = false;

    numSPH_classes = 0;
    numDBC_classes = 0;
}


// ****************************************************************************
//  Destructor:  avtMiliFileFormat::~avtMiliFileFormat
//
//  Programmer:  Hank Childs
//  Creation:    April 11, 2003
//
//  Modifications:
//    Akira Haddox, Fri May 23 08:51:11 PDT 2003
//    Added in support for multiple meshes. Changed to MTMD.
//
//    Akira Haddox, Wed Jul 23 12:57:14 PDT 2003
//    Moved allocation of cached information to FreeUpResources.
//
//    Hank Childs, Tue Jul 27 10:40:44 PDT 2004
//    Sucked in code from FreeUpResources.
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Free structures having to do with free nodes mesh
//
//    Mark C. Miller, Wed Mar  8 08:40:55 PST 2006
//    Added code to cleanse Mili subrecords
// ****************************************************************************

avtMiliFileFormat::~avtMiliFileFormat()
{
    //
    // Close mili databases, and delete non-essential allocated memory.
    // Keep the original sizes of vectors though.
    //
    int i, j;
    for (i = 0; i < ndomains; ++i)
    {
        CloseDB(i);
    }
    for (i = 0; i < connectivity.size(); ++i)
        for (j = 0; j < connectivity[i].size(); ++j)
            if (connectivity[i][j] != NULL)
            {
                connectivity[i][j]->Delete();
                connectivity[i][j] = NULL;
            }
    for (i = 0; i < materials.size(); ++i)
        for (j = 0; j < materials[i].size(); ++j)
            if (materials[i][j])
            {
                delete materials[i][j];
                materials[i][j] = NULL;
            }
    connectivity.clear();
    materials.clear();

    //    for (i = 0; i < sub_records.size(); ++i)
    //      for (j = 0; j < sub_records[i].size(); ++j)
    //         mc_cleanse_subrec(&sub_records[i][j]);

    //
    // Reset flags to indicate the meshes needs to be read in again.
    //
    for (i = 0; i < ndomains; ++i)
    {
        readMesh[i] = false;
    }

    delete [] famroot;
    if (fampath)
    {
        delete [] fampath;
    }

    if (free_nodes)
    {
        delete [] free_nodes;
        free_nodes     = NULL;
        num_free_nodes = 0;
        free_nodes_ts   = -1;
    }

    if (dbc_nodes)
    {
        delete [] dbc_nodes;
        dbc_nodes     = NULL;
        dbc_nodes_ts = -1;
    }
}


// ****************************************************************************
//  Function: read_results
//
//  Purpose:
//      A wrapper around mc_read_results that handles multiple types (floats,
//      doubles, etc.).
//
//  Programmer: Hank Childs
//  Creation:   July 20, 2004
//
//  Modifications:
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added logic to read "param arrays" via a different Mili API call. Note
//    that param arrays are always alloc'd by Mili
// ****************************************************************************

static void
read_results(Famid &dbid, int ts, int sr, int rank,
             char **name, int vtype, int amount, float *buff)
{
    int  i;

    bool isParamArray = strncmp(*name, "params/", 7) == 0;

    void *buff_to_read_into = NULL;
    if (!isParamArray)
    {
        switch (vtype)
        {
            case M_STRING:
                buff_to_read_into = new char[amount];
                break;
            case M_FLOAT:
            case M_FLOAT4:
                buff_to_read_into = buff;
                break;
            case M_FLOAT8:
                buff_to_read_into = new double[amount];
                break;
            case M_INT:
            case M_INT4:
                buff_to_read_into = new int[amount];
                break;
            case M_INT8:
                buff_to_read_into = new long[amount];
                break;
        }
    }

    int rval;
    if (isParamArray)
    {
        char tmpName[256];
        strcpy(tmpName, &(*name)[7]);
        rval = mc_read_param_array(dbid, tmpName, &buff_to_read_into);
        if (rval == OK && (vtype == M_FLOAT || vtype == M_FLOAT4))
        {
            float *pflt = (float *) buff_to_read_into;
            for (i = 0 ; i < amount ; i++)
            {
                buff[i] = (float)(pflt[i]);
            }
        }
    }
    else
    {
        rval = mc_read_results(dbid, ts, sr, rank, name, buff_to_read_into);
    }

    if (rval != OK)
    {
        ErrorHandler(__FUNCTION__, __LINE__);
        EXCEPTION1(InvalidVariableException, name[0]);
    }

    char   *c_tmp = NULL;
    double *d_tmp = NULL;
    int    *i_tmp = NULL;
    long   *l_tmp = NULL;
    switch (vtype)
    {
        case M_STRING:
            c_tmp = (char *) buff_to_read_into;
            for (i = 0 ; i < amount ; i++)
            {
                buff[i] = (float)(c_tmp[i]);
            }
            delete [] c_tmp;
            break;
        case M_FLOAT:
        case M_FLOAT4:
            break;
        case M_FLOAT8:
            d_tmp = (double *) buff_to_read_into;
            for (i = 0 ; i < amount ; i++)
            {
                buff[i] = (float)(d_tmp[i]);
            }
            delete [] d_tmp;
            break;
        case M_INT:
        case M_INT4:
            i_tmp = (int *) buff_to_read_into;
            for (i = 0 ; i < amount ; i++)
            {
                buff[i] = (float)(i_tmp[i]);
            }
            delete [] i_tmp;
            break;
        case M_INT8:
            l_tmp = (long *) buff_to_read_into;
            for (i = 0 ; i < amount ; i++)
            {
                buff[i] = (float)(l_tmp[i]);
            }
            delete [] l_tmp;
            break;
    }

    if (isParamArray)
    {
        free(buff_to_read_into);
    }
}


// ****************************************************************************
//  Method:  avtMiliFileFormat::GetMesh
//
//  Purpose:
//      Gets the mesh for timestep 'ts' and domain 'dom'.
//
//  Arguments:
//    ts         the time step
//    dom        the domain
//    mesh       the name of the mesh to read
//
//  Programmer:  Hank Childs
//  Creation:    April 11, 2003
//
//  Modifications:
//    Akira Haddox, Fri May 23 08:51:11 PDT 2003
//    Added in support for multiple meshes. Changed to MTMD.
//
//    Akira Haddox, Tue Jul 22 08:09:28 PDT 2003
//    Fixed the try block. Properly dealt with cell variable blocks.
//
//    Akira Haddox, Mon Aug 18 14:33:15 PDT 2003
//    Commented out previous sand-based ghosts.
//
//    Hank Childs, Sat Jun 26 11:24:47 PDT 2004
//    Check for bad files where number of timesteps is incorrectly reported.
//
//    Hank Childs, Fri Aug 27 17:12:50 PDT 2004
//    Rename ghost data array.
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added logic to read the "free nodes" mesh, too. Removed huge block of
//    unused #ifdef'd code having to do with ghost zones.
//
//    Mark C. Miller, Tue Jan  3 17:55:22 PST 2006
//    Added code to deal with case where nodal positions are time invariant.
//    They are not stored as "results" but instead part of the mesh read
//    in the ReadMesh() call.
//
//    Mark C. Miller, Wed Nov 15 01:46:16 PST 2006
//    Added a "no_free_nodes" mesh by ghost labeling sanded nodes. Added
//    the logic to label sanded nodes here.
//
//    Mark C. Miller, Tue Nov 21 10:16:42 PST 2006
//    Fixed leak of sand_arr. Made it request sand_arr only if the
//    no_free_nodes mesh was requested
// ****************************************************************************

vtkDataSet *
avtMiliFileFormat::GetMesh(int ts, int dom, const char *mesh)
{
    int i;
    bool sph_mesh=false, free_mesh=false, dbc_mesh=false;

    debug5 << "Reading in " << mesh << " for domain/ts : " << dom << ',' << ts
           << endl;

    if (!readMesh[dom])
    {
        ReadMesh(dom);
    }

    if (!validateVars[dom])
    {
        ValidateVariables(dom);
    }

    int mesh_id;
    int num_nodes=0;
    temp_node_ids = NULL;
    temp_zone_ids = NULL;

    //
    // The valid meshnames are meshX, where X is an int > 0.
    // We need to verify the name, and get the meshId.
    //
    if (strstr(mesh, "mesh") != mesh)
    {
        ErrorHandler(__FUNCTION__, __LINE__);
        EXCEPTION1(InvalidVariableException, mesh);
    }

    //
    // Do a checked conversion to integer.
    //
    char *check = 0;
    mesh_id = (int) strtol(mesh + 4, &check, 10);
    if (mesh_id == 0 || check == mesh + 4)
    {
        ErrorHandler(__FUNCTION__, __LINE__);
        EXCEPTION1(InvalidVariableException, mesh)
    }
    --mesh_id;

    //
    // The connectivity does not change over time, so use the one we have
    // already calculated.
    //
    vtkUnstructuredGrid *rv = vtkUnstructuredGrid::New();
    rv->ShallowCopy(connectivity[dom][mesh_id]);

    //
    // The node positions are stored in 'nodpos'.
    //
    char *nodpos_str = (char *) "nodpos";
    int nodpos = -2;

    // Since this whole plugin assumes GetVariableIndex
    // handles throwing of invalid variable exception
    // we have to wrap this with TRY/CATCH to deal with
    // case where nodal positions are stored in params
    TRY
    {
        nodpos = GetVariableIndex(nodpos_str, mesh_id);
    }
    CATCH(InvalidVariableException)
    {
        nodpos = -1;
    }
    ENDTRY

    int subrec = -1;
    int vsize = M_FLOAT;

    subrec = vars[nodpos_str].sub_record_ids[dom][0]; /* nodpos only in one sub-record */
    vsize  = vars[nodpos_str].vars_size[dom];

    int amt = 2*dims*nnodes[dom][mesh_id];
    float *fpts = NULL;
    if (nodpos != -1)
    {
        if (subrec == -1)
        {
            if (rv->GetPoints() == 0)
            {
                char msg[1024];
                SNPRINTF(msg, sizeof(msg),
                         "Unable to find coords for domain %d. Skipping it", dom);
                Warn(msg);

                // null out the returned grid
                rv->Delete();
                rv = vtkUnstructuredGrid::New();
                return rv;
            }
        }
        else
        {
            fpts = new float[amt];
            read_results(dbid[dom], ts+1, subrec, 1, &nodpos_str, vsize, amt, fpts);

            vtkPoints *pts = vtkPoints::New();
            pts->SetNumberOfPoints(nnodes[dom][mesh_id]);
            float *vpts = (float *) pts->GetVoidPointer(0);
            float *tmp = fpts;
            for (int pt = 0 ; pt < nnodes[dom][mesh_id] ; pt++)
            {
                *(vpts++) = *(tmp++);
                *(vpts++) = *(tmp++);
                if (dims >= 3)
                {
                    *(vpts++) = *(tmp++);
                }
                else
                {
                    *(vpts++) = 0.;
                }
            }
            rv->SetPoints(pts);
            pts->Delete();

            //
            // Ghost out nodes that belong to zones that are "sanded"
            // Start by assuming all nodes are N/A and then remove the
            // N/A ghost node type for all those nodes that belong to
            // zones that are NOT sanded.
            //
            if (strstr(mesh, no_free_nodes_str))
            {
                vtkFloatArray *sand_arr = (vtkFloatArray *) GetVar(ts, dom, "sand");
                if (sand_arr)
                {
                    float *sand_vals = (float*) sand_arr->GetVoidPointer(0);

                    vtkUnsignedCharArray *ghost_nodes = vtkUnsignedCharArray::New();
                    ghost_nodes->SetName("avtGhostNodes");
                    ghost_nodes->SetNumberOfTuples(nnodes[dom][mesh_id]);
                    unsigned char *gnp = ghost_nodes->GetPointer(0);
                    for (i = 0 ; i < nnodes[dom][mesh_id]; i++)
                    {
                        gnp[i] = 0;
                        avtGhostData::AddGhostNodeType(gnp[i],
                                                       NODE_NOT_APPLICABLE_TO_PROBLEM);
                    }
                    for (int cell = 0;
                            cell < ncells[dom][mesh_id];
                            cell++)
                    {
                        if (sand_vals[cell] > 0.5) // element status is "good"
                        {
                            vtkIdType npts = 0, *pts = 0;
                            rv->GetCellPoints(cell, npts, pts);
                            if (npts && pts)
                            {
                                for (int node = 0; node < npts; node++)
                                    avtGhostData::RemoveGhostNodeType(gnp[pts[node]],
                                                                      NODE_NOT_APPLICABLE_TO_PROBLEM);
                            }
                        }
                    }
                    sand_arr->Delete();
                    rv->GetPointData()->AddArray(ghost_nodes);
                    ghost_nodes->Delete();
                }
            }
        }
    }
    else
    {
        //
        // We can arrive here if there are no nodal positions results
        // but we have initial mesh positions from reading the mesh
        // header information (mc_load_nodes).
        //
        if (rv->GetPoints() == 0)
        {
            char msg[1024];
            SNPRINTF(msg, sizeof(msg),
                     "Unable to find coords for domain %d. Skipping it", dom);
            Warn(msg);

            // null out the returned grid
            rv->Delete();
            rv = vtkUnstructuredGrid::New();
            return rv;
        }
    }

    //
    // Add Node and Elements Labels to Mesh
    //
    if ( temp_node_ids!=NULL )
    {
        free (temp_node_ids);
    }
    num_nodes = mili_node_class["node"].node_data[dom].qty;

    temp_node_ids = (int *) malloc( num_nodes*sizeof(int) );
    vtkIntArray *nodeLabels = vtkIntArray::New();
    for (i=0;
            i<num_nodes;
            i++)
    {
        temp_node_ids[i] = mili_node_class["node"].node_data[dom].labels[i];
        temp_node_ids[i] = (1000*(dom+1)) + i;
    }

    nodeLabels->SetArray((int*) temp_node_ids, num_nodes, 0 );
    void_ref_ptr nr = void_ref_ptr(nodeLabels, avtVariableCache::DestructVTKObject);
    //    cache->CacheVoidRef(mesh, AUXILIARY_DATA_GLOBAL_NODE_IDS, ts,
    //		dom, nr);

    vtkIntArray *zoneLabels = vtkIntArray::New();
    for ( mili_elem_class_iter =  mili_elem_class.begin();
            mili_elem_class_iter != mili_elem_class.end();
            mili_elem_class_iter++ )
    {
        std::string  class_name;
        class_name = (*mili_elem_class_iter).first;
    }

    temp_zone_ids = (int *) malloc( ncells[dom][mesh_id]*sizeof(int) );
    for ( i=0;
            i<ncells[dom][mesh_id];
            i++ )
    {
        temp_zone_ids[i] = (1000*(dom+1)) + i;
    }

    zoneLabels->SetArray((int*) temp_zone_ids,
                         ncells[dom][mesh_id], 0);
    void_ref_ptr zr = void_ref_ptr(zoneLabels, avtVariableCache::DestructVTKObject);
    cache->CacheVoidRef(mesh, AUXILIARY_DATA_GLOBAL_ZONE_IDS, ts,
                        dom, zr);

    //
    // If VisIt really asked for the free nodes mesh, compute that now,
    // otherwise, just return the mesh
    //
    if (strstr(mesh, no_free_nodes_str) ||
            !strstr(mesh, free_nodes_str))
    {
        free_mesh = false;
    }
    else
    {
        free_mesh = true;
    }

    if ( strstr(mesh, part_nodes_str))
    {
        sph_mesh = true;
    }

    if ( strstr(mesh, dbc_nodes_str))
    {
        dbc_mesh = true;
    }

    if ( free_mesh==false && sph_mesh==false && dbc_mesh==false  )
    {
        if (fpts)
        {
            delete [] fpts;
        }
        fpts = NULL;
        return rv;
    }

    //
    // Element status' are stored in the "sand" variable
    //
    int cell, node;
    int num_free = nnodes[dom][mesh_id];

    unsigned char *ns = new unsigned char[nnodes[dom][mesh_id]];
    const unsigned char MESH = 'm';
    const unsigned char FREE = 'f';

    if ( free_mesh )
    {
        vtkFloatArray *sand_arr = (vtkFloatArray *) GetVar(ts, dom, "sand");
        if (sand_arr->GetNumberOfTuples() != ncells[dom][mesh_id])
        {
            EXCEPTION2(UnexpectedValueException, ncells[dom][mesh_id],
                       sand_arr->GetNumberOfTuples());
        }

        float *sand_vals = (float*) sand_arr->GetVoidPointer(0);
        memset(ns, FREE, nnodes[dom][mesh_id]);

        bool *valid_nodes;
        valid_nodes = new bool[nnodes[dom][mesh_id]];
        memset(valid_nodes, false, nnodes[dom][mesh_id]*sizeof(bool));

        //
        // Populate nodal status array based on element status'
        //
        for (cell = 0;
                cell < ncells[dom][mesh_id];
                cell++)
        {
            if (sand_vals[cell] > 0.5) // element status is "good"
            {
                vtkIdType npts = 0, *pts = 0;
                rv->GetCellPoints(cell, npts, pts);
                if (npts && pts)
                {
                    for (node = 0;
                            node < npts;
                            node++)
                    {
                        int nid = pts[node];
                        valid_nodes[nid]=true;
                        if (ns[nid] != MESH)
                        {
                            ns[nid] = MESH;
                            num_free--;
                        }
                    }
                }
            }
        }
        sand_arr->Delete();
        for (node = 0;
                node < nnodes[dom][mesh_id];
                node++)
        {
            if (valid_nodes[node]==false && num_free>0)
            {
                num_free--;
                ns[node] = MESH;
            }

        }
        delete [] valid_nodes;
    }

    vtkPoints *freepts = vtkPoints::New();
    vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::New();
    if ( free_mesh )
    {
        vtkPoints *freepts = vtkPoints::New();
        vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::New();
        if ( num_free>0 )
        {
            //
            // Cache the list of nodes that are the free nodes
            //

            num_free_nodes = num_free;
            if (free_nodes)
            {
                delete [] free_nodes;
            }
            free_nodes = new int[num_free];
            memset(free_nodes, -1, num_free*sizeof(int));
            free_nodes_ts = ts;

            freepts->SetNumberOfPoints(num_free);
            float *fptr_dst = (float *) freepts->GetVoidPointer(0);
            float *fptr_src = fpts;
            int fnode = 0;
            for (node = 0;
                    node < num_free;
                    node++)
            {
                if (ns[node] == FREE )
                {
                    free_nodes[fnode/3] = node;
                    fptr_dst[fnode++] = fptr_src[3*node+0];
                    fptr_dst[fnode++] = fptr_src[3*node+1];
                    fptr_dst[fnode++] = fptr_src[3*node+2];
                }
            }

            ugrid->SetPoints(freepts);
            ugrid->Allocate(num_free);
            vtkIdType onevertex[1];
            for (node = 0;
                    node < num_free;
                    node++)
            {
                onevertex[0] = node;
                ugrid->InsertNextCell(VTK_VERTEX, 1, onevertex);
            }
            freepts->Delete();
            delete [] fpts;
            delete [] ns;
            rv->Delete();
            return ugrid;
        }
        else
        {
            if (free_nodes)
            {
                delete [] free_nodes;
            }
            free_nodes = NULL;
            free_nodes_ts = ts;
            freepts->SetNumberOfPoints(0);
            ugrid->SetPoints(freepts);
            freepts->Delete();
            delete [] fpts;
            ugrid->Delete();
            return rv;
        }
    }

    vtkPoints *partpts = vtkPoints::New();
    if ( sph_mesh )
    {
        if ( numSPH_classes>0 )
        {
            //
            // Cache the list of nodes that are the SPH particle nodes
            //
            int i=0;
            int fnode = 0;
            int pncount = num_part_elems[dom];

            partpts->SetNumberOfPoints(pncount);
            float *fptr_dst = (float *) partpts->GetVoidPointer(0);
            float *fptr_src = fpts;

            part_nodes_ts = ts;

            for ( i = 0;
                    i < pncount;
                    i++)
            {
                node = part_nodes[dom][i];
                fptr_dst[fnode++] = fptr_src[3*node+0];
                fptr_dst[fnode++] = fptr_src[3*node+1];
                fptr_dst[fnode++] = fptr_src[3*node+2];
            }

            ugrid->SetPoints(partpts);
            ugrid->Allocate(pncount);
            vtkIdType onevertex[1];
            for (node = 0;
                    node < pncount;
                    node++)
            {
                onevertex[0] = node;
                ugrid->InsertNextCell(VTK_VERTEX, 1, onevertex);
            }
            partpts->Delete();
            delete [] fpts;
            delete [] ns;
            rv->Delete();
            return ugrid;
        }
        else
        {
            part_nodes_ts = ts;
            partpts->SetNumberOfPoints(0);
            partpts->Delete();
            delete [] fpts;
            ugrid->Delete();
            return rv;
        }
    }

    if ( dbc_mesh )
    {
        if ( numDBC_classes>0 )
        {
            //
            // Cache the list of nodes that are the DBC particle nodes
            //
            int i=0;
            int fnode = 0;
            int pncount = num_dbc_elems[dom];

            partpts->SetNumberOfPoints(pncount);
            float *fptr_dst = (float *) partpts->GetVoidPointer(0);
            float *fptr_src = fpts;

            dbc_nodes_ts = ts;

            for ( i = 0;
                    i < pncount;
                    i++)
            {
                node = dbc_nodes[dom][i];
                fptr_dst[fnode++] = fptr_src[3*node+0];
                fptr_dst[fnode++] = fptr_src[3*node+1];
                fptr_dst[fnode++] = fptr_src[3*node+2];
            }

            ugrid->SetPoints(partpts);
            ugrid->Allocate(pncount);
            vtkIdType onevertex[1];
            for (node = 0;
                    node < pncount;
                    node++)
            {
                onevertex[0] = node;
                ugrid->InsertNextCell(VTK_VERTEX, 1, onevertex);
            }
            partpts->Delete();
            delete [] fpts;
            delete [] ns;
            rv->Delete();
            return ugrid;
        }
        else
        {
            dbc_nodes_ts = ts;
            partpts->SetNumberOfPoints(0);
            partpts->Delete();
            delete [] fpts;
            ugrid->Delete();
            return rv;
        }
    }
}


// ****************************************************************************
//  Method: avtMiliFileFormat::GetVariableIndex
//
//  Purpose:
//      Gets the index of a variable.
//
//  Programmer: Hank Childs
//  Creation:   April 16, 2003
//
//  Modifications:
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added logic to deal with free nodes mesh variables
//
//    Mark C. Miller, Wed Nov 15 01:46:16 PST 2006
//    Changed names of free_node variables from 'xxx_free_nodes' to
//    'free_nodes/xxx' to put them in a submenu in GUI.
//
//    Kathleen Bonnell, Thur Mar 26 08:14:54 MST 2009
//    Made 'p' const for compiling on windows.
//
// ****************************************************************************

int
avtMiliFileFormat::GetVariableIndex(const char *varname)
{
    string tmpname = varname;
    const char *p    = strstr(varname, free_nodes_str);
    const char *pn   = strstr(varname, part_nodes_str);
    const char *dbcn = strstr(varname, dbc_nodes_str);

    int var_index=0;
    size_t found;

    if (p)
        tmpname = string(varname, free_nodes_strlen+1,
                         strlen(varname) - (free_nodes_strlen+1));
    if (pn)
        tmpname = string(varname, part_nodes_strlen+1,
                         strlen(varname) - (part_nodes_strlen+1));
    if (dbcn)
        tmpname = string(varname, dbc_nodes_strlen+1,
                         strlen(varname) - (dbc_nodes_strlen+1));

    for ( vars_iter =  vars.begin();
            vars_iter != vars.end();
            vars_iter++ )
    {
        std::string  var_name;
        var_name = (*vars_iter).first;
        if (var_name == tmpname)
        {
            return var_index;
        }
        else
        {
            var_index++;
        }
    }

    ErrorHandler(__FUNCTION__, __LINE__);
    EXCEPTION1(InvalidVariableException, varname);
}

// ****************************************************************************
//  Method: avtMiliFileFormat::GetVariableIndex
//
//  Purpose:
//      Gets the index of a variable that is associated with a particular
//      mesh.
//
//  Programmer: Akira Haddox
//  Creation:   May 23, 2003
//
//  Modifications:
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added logic to deal with free nodes mesh variables
//
//    Mark C. Miller, Wed Nov 15 01:46:16 PST 2006
//    Changed names of free_node variables from 'xxx_free_nodes' to
//    'free_nodes/xxx' to put them in a submenu in GUI.
//
//    Kathleen Bonnell, Thur Mar 26 08:14:54 MST 2009
//    Made 'p' const for compiling on windows.
//
// ****************************************************************************

int
avtMiliFileFormat::GetVariableIndex(const char *var_name, int mesh_id)
{
    int var_index=0;
    State_variable sv;
    string tmpname = var_name;

    const char *p    = strstr(var_name, free_nodes_str);
    const char *pn   = strstr(var_name, part_nodes_str);
    const char *dbcn = strstr(var_name, dbc_nodes_str);

    size_t found;

    if (p)
        tmpname = string(var_name, free_nodes_strlen+1,
                         strlen(var_name) - (free_nodes_strlen+1));

    if (pn)
        tmpname = string(var_name, part_nodes_strlen+1,
                         strlen(var_name) - (part_nodes_strlen+1));

    if (dbcn)
        tmpname = string(var_name, dbc_nodes_strlen+1,
                         strlen(var_name) - (dbc_nodes_strlen+1));

    /*    found = tmpname.find_last_of("/");
    if ( found!=string::npos )
    tmpname = tmpname.substr(0, found); */

    for ( vars_iter =  vars.begin();
            vars_iter != vars.end();
            vars_iter++ )
    {
        std::string  var_name;
        var_name = (*vars_iter).first;
        if ( var_name == tmpname && vars[var_name].var_mesh_associations == mesh_id)
        {
            return var_index;
        }
        else
        {
            var_index++;
        }
    }

    ErrorHandler(__FUNCTION__, __LINE__);
    EXCEPTION1(InvalidVariableException, var_name);
}

// ****************************************************************************
//  Method: avtMiliFileFormat::DecodeMultiMeshVarname
//
//  Purpose:
//      Takes in a variable name used to populate, and returns the
//      original variable name, and associated mesh id.
//
//  Programmer: Akira Haddox
//  Creation:   June 26, 2003
//
// ****************************************************************************

void
avtMiliFileFormat::DecodeMultiMeshVarname(const string &varname,
        string &decoded, int &meshid)
{
    decoded = varname;
    meshid = 0;

    char *ptr = &(decoded[0]);
    while(*ptr != '\0')
    {
        if(*ptr == '(')
        {
            break;
        }
        ++ptr;
    }

    if (*ptr == '\0')
    {
        return;
    }

    char *check;
    meshid = (int) strtol(ptr + strlen("(mesh"), &check, 10);
    --meshid;

    *ptr = '\0';
}

// ****************************************************************************
//  Method: avtMiliFileFormat::DecodeMultiLevelVarname
//
//  Purpose:
//      Takes in a variable name used to populate, and returns the
//      original variable name less the directory path.
//
//  Programmer: Ivan R. Corey
//  Creation:   June 22, 2011
//
// ****************************************************************************

void
avtMiliFileFormat::DecodeMultiLevelVarname(const string &inname, string &decoded)
{
    size_t found;
    string vname;
    bool moreLevels=true;

    // Seperate var dir from name
    decoded = inname;
    if (strncmp(inname.c_str(), "params/", 7) == 0)
    {
        return;
    }

    while (moreLevels)
    {
        found=0;
        found = decoded.find_first_of("/");
        if ( found!=string::npos )
        {
            decoded = decoded.substr(found+1);
        }
        else
        {
            moreLevels=false;
        }

        if (strncmp(decoded.c_str(), "params/", 7) == 0)
        {
            moreLevels=false;
        }
    }
}

// ****************************************************************************
//  Method: avtMiliFileFormat::isVecVar
//
//  Purpose:
//      Takes in a variable name and determines if it
//      is a vector var. If so the variable name - less
//      the component name is returned,
//
//  Programmer: Ivan R. Corey
//  Creation:   Nov 09, 2011
//
// ****************************************************************************

bool
avtMiliFileFormat::isVecVar(const string &inname, string &varName)
{
    size_t found=0, lastFound=0;
    string compName;
    varName = "";

    // Seperate var dir from name
    found = inname.find_first_of("/");
    if ( found!=string::npos )
    {
        compName = inname.substr(found+1);
        varName = inname.substr(0, found);
        svars_iter = svars.find(varName);
        if ( svars_iter!=svars.end() )
        {
            return true;
        }

    }
    return false;
}

// ****************************************************************************
//  Method: avtMiliFileFormat::OpenDB
//
//  Purpose:
//      Open up a family database for a given domain.
//
//  Programmer: Akira Haddox
//  Creation:   June 26, 2003
//
//  Modifications:
//
//    Akira Haddox, Tue Jul 22 15:34:40 PDT 2003
//    Added in setting of times.
//
//    Hank Childs, Mon Oct 20 10:03:58 PDT 2003
//    Made a new data member for storing times.  Populated that here.
//
//    Hank Childs, Wed Aug 18 16:17:52 PDT 2004
//    Add some special handling for single domain families.
//
// ****************************************************************************

void
avtMiliFileFormat::OpenDB(int dom, bool quickOpen)
{
    if (dbid[dom] == -1)
    {
        int rval;
        if (ndomains == 1)
        {
            rval = mc_open( famroot, fampath, (char *)"r", &(dbid[dom]) );

            if ( rval != OK )
            {
                // Try putting in the domain number and see what happens...
                // We need this because makemili accepts it and there are
                // legacy .mili files that look like fam rather than fam000.
                char rootname[255];
                sprintf(rootname, "%s%.3d", famroot, dom);
                if ( quickOpen )
                {
                    rval = mc_quick_open(rootname, fampath, (char *)"r", &(dbid[dom]) );
                }
                else
                {
                    rval = mc_open(rootname, fampath, (char *)"r", &(dbid[dom]) );
                }
            }
            if ( rval != OK )
            {
                EXCEPTION1(InvalidFilesException, famroot);
            }
        }
        else
        {
            char famname[128];
            sprintf(famname, "%s%.3d", famroot, dom);
            if ( quickOpen )
            {
                rval = mc_quick_open( famname, fampath, (char *)"r", &(dbid[dom]) );
            }
            else
            {
                rval = mc_open( famname, fampath, (char *)"r", &(dbid[dom]) );
            }
            if ( rval != OK )
            {
                EXCEPTION1(InvalidFilesException, famname);
            }
        }

        if ( quickOpen )
        {
            return;
        }

        //
        // The first domain that we open, we use to find the times.
        //
        if (!setTimesteps)
        {
            setTimesteps = true;

            //
            // First entry is how many timesteps we want (all of them),
            // the following entries are which timesteps we want.
            // The index is 1 based.
            //
            vector<int> timeVars(ntimesteps + 1);
            timeVars[0] = ntimesteps;
            int i;
            for (i = 1; i <= ntimesteps; ++i)
            {
                timeVars[i] = i;
            }

            vector<float> ttimes(ntimesteps);
            rval = mc_query_family(dbid[dom], MULTIPLE_TIMES, &(timeVars[0]),
                                   0, &(ttimes[0]));

            //
            // Some Mili files are written out incorrectly -- they have an
            // extra timestep at the end with no data.  Detect and ignore.
            //
            if (ntimesteps >= 2)
                if (ttimes[ntimesteps-1] == ttimes[ntimesteps-2])
                {
                    ntimesteps--;
                }

            times.clear();
            if (rval == OK)
            {
                for (i = 0; i < ntimesteps; ++i)
                {
                    times.push_back(ttimes[i]);
                }
            }
        }
    }
}

// ****************************************************************************
//  Method: avtMiliFileFormat::CloseDB
//
//  Purpose:
//      Close a family database for a given domain.
//
//  Programmer: Bob Corey
//  Creation:   January 10th, 2013
//
//  Modifications:
//
// ****************************************************************************

void
avtMiliFileFormat::CloseDB(int dom)
{
    if (dbid[dom] != -1)
    {
        mc_close(dbid[dom]);
        dbid[dom] = -1;
    }
}

// ****************************************************************************
//  Method: avtMiliFileFormat::GetSizeInfoForGroup
//
//  Purpose:
//      Returns the number of cells and where they start in the connectivity
//      index.
//
//  Programmer: Hank Childs
//  Creation:   April 16, 2003
//
//  Modifications
//    Akira Haddox, Fri May 23 08:13:09 PDT 2003
//    Added in support for multiple meshes. Changed for MTMD.
//
// ****************************************************************************

void
avtMiliFileFormat::GetSizeInfoForGroup(const char *group_name, int &offset,
                                       int &g_size, int dom)
{
    if (!readMesh[dom])
    {
        ReadMesh(dom);
    }

    int g_index = -1;

    if ( !strcmp(group_name, "mat" ) )
    {
        int mesh_id = 0;
        g_size = nmaterials[mesh_id];
        return;
    }
    if ( !strcmp(group_name, "glob" ) )
    {
        g_size = 1;
        return;
    }

    for (int i = 0;
            i < element_group_name[dom].size();
            i++)
    {
        if (element_group_name[dom][i] == group_name)
        {
            g_index = i;
            break;
        }
    }
    if (g_index == -1)
    {
        EXCEPTION0(ImproperUseException);
    }
    int mesh_id = group_mesh_associations[dom][g_index];

    offset = connectivity_offset[dom][g_index];
    if (g_index == (connectivity_offset[dom].size()-1))
    {
        g_size = ncells[dom][mesh_id] - connectivity_offset[dom][g_index];
    }
    else
    {
        g_size = connectivity_offset[dom][g_index+1]
                 - connectivity_offset[dom][g_index];
    }
}


// ****************************************************************************
//  Method: avtMiliFileFormat::ReadMesh
//
//  Purpose:
//      Read the connectivity for the meshes in a certain domain.
//
//  Programmer: Hank Childs (adapted by Akira Haddox)
//  Creation:   June 25, 2003
//
//  Modifications:
//    Akira Haddox, Tue Jul 22 09:21:39 PDT 2003
//    Changed ConstructMaterials call to match new signature.
//
//    Akira Haddox, Thu Aug  7 10:07:40 PDT 2003
//    Fixed beam support.
//
//    Mark C. Miller, Tue Jan  3 17:55:22 PST 2006
//    Added code to get initial nodal positions with mc_load_nodes()
//
//    Eric Brugger, Thu Mar 29 11:43:07 PDT 2007
//    Added code to detect tetrahedra stored as degenerate hexahedra and
//    convert them to tetrahedra.
//
//    Brad Whitlock, Thu May 10 16:30:33 PST 2007
//    I corrected a bug that caused node 1 to be messed up.
//
// ****************************************************************************

void
avtMiliFileFormat::ReadMesh(int dom)
{
    if (dbid[dom] == -1)
    {
        OpenDB(dom, false);
    }

    char short_name[1024];
    char long_name[1024];

    //
    // Read in the meshes.
    //

    int mesh_id;
    for (mesh_id = 0; mesh_id < nmeshes; ++mesh_id)
    {
        //
        // Determine the number of nodes.
        //
        int node_id = 0;
        mc_get_class_info(dbid[dom], mesh_id, M_NODE, node_id, short_name,
                          long_name, &nnodes[dom][mesh_id]);

        //
        // Read initial nodal position information, if available
        //
        vtkPoints *pts = vtkPoints::New();
        pts->SetNumberOfPoints(nnodes[dom][mesh_id]);
        float *vpts = (float *) pts->GetVoidPointer(0);
        if (mc_load_nodes(dbid[dom], mesh_id, short_name, vpts) == 0)
        {
            //
            // We need to insert zeros if we're in 2D
            //
            if (dims == 2)
            {
                for (int p = nnodes[dom][mesh_id]-1; p >= 0; p--)
                {
                    int q = p*3, r = p*2;
                    // Store the coordinates in reverse so we don't mess up at node 1.
                    vpts[q+2] = 0.0;
                    vpts[q+1] = vpts[r+1];
                    vpts[q+0] = vpts[r+0];
                }
            }
        }
        else
        {
            pts->Delete();
            pts = NULL;
        }

        //
        // Determine the connectivity.  This will also calculate the number of
        // cells.
        //
        connectivity[dom][mesh_id] = vtkUnstructuredGrid::New();

        //
        // Make one pass through the data and read all of the connectivity
        // information.
        //
        vector < vector<int *> > conn_list;
        vector < vector<int *> > mat_list;
        vector < vector<int> > list_size;
        conn_list.resize(n_elem_types);
        list_size.resize(n_elem_types);
        mat_list.resize(n_elem_types);
        int i, j, k;
        ncells[dom][mesh_id] = 0;
        int ncoords = 0;
        for (i = 0 ; i < n_elem_types ; i++)
        {
            int args[2];
            args[0] = mesh_id;
            args[1] = elem_sclasses[i];
            int ngroups = 0;
            mc_query_family(dbid[dom], QTY_CLASS_IN_SCLASS, (void*) args, NULL,
                            (void*) &ngroups);
            for (j = 0 ; j < ngroups ; j++)
            {
                int nelems;
                mc_get_class_info(dbid[dom], mesh_id, elem_sclasses[i], j,
                                  short_name, long_name, &nelems);

                int *conn = new int[nelems * conn_count[i]];
                int *mat = new int[nelems];
                int *part = new int[nelems];

                mc_load_conns(dbid[dom], mesh_id, short_name, conn, mat, part);
                conn_list[i].push_back(conn);
                mat_list[i].push_back(mat);
                delete [] part;

                list_size[i].push_back(nelems);
                connectivity_offset[dom].push_back(ncells[dom][mesh_id]);
                element_group_name[dom].push_back(short_name);
                group_mesh_associations[dom].push_back(mesh_id);
                ncells[dom][mesh_id] += list_size[i][j];

                ncoords += list_size[i][j] * (conn_count[i] + 1);
            }
        }

        //
        // Allocate an appropriately sized dataset using that connectivity
        // information.
        //
        connectivity[dom][mesh_id]->Allocate(ncells[dom][mesh_id], ncoords);

        //
        // The materials are in a format that is not AVT friendly.  Convert it
        // now.
        //
        materials[dom][mesh_id] = ConstructMaterials(mat_list, list_size,
                                  mesh_id);

        //
        // Now that we have our avtMaterial, we can deallocate the
        // old pure material data.
        //
        for (i = 0; i < mat_list.size(); ++i)
            for (j = 0; j < mat_list[i].size(); ++j)
            {
                delete[] (mat_list[i][j]);
            }

        //
        // Now construct the connectivity in a VTK dataset.
        //
        int ii=0;
        for (i = 0 ; i < n_elem_types ; i++)
        {
            for (j = 0 ; j < conn_list[i].size() ; j++)
            {
                int *conn = conn_list[i][j];
                int nelems = list_size[i][j];
                for (k = 0 ; k < nelems ; k++)
                {
                    vtkIdType verts[100];
                    for(int cc = 0; cc < conn_count[i]; ++cc)
                    {
                        verts[cc] = (vtkIdType)conn[cc];
                    }

                    switch (elem_sclasses[i])
                    {
                        case M_TRUSS:
                            connectivity[dom][mesh_id]->InsertNextCell(VTK_LINE,
                                    conn_count[i], verts);
                            break;
                        case M_BEAM:
                            // Beams are lines that have a third point to define
                            // the normal. Since we don't need to visualize it,
                            // we just drop the normal point.
                            connectivity[dom][mesh_id]->InsertNextCell(VTK_LINE,
                                    2, verts);
                            break;
                        case M_TRI:
                            connectivity[dom][mesh_id]->InsertNextCell(
                                VTK_TRIANGLE,
                                conn_count[i], verts);
                            break;
                        case M_QUAD:
                            connectivity[dom][mesh_id]->InsertNextCell(VTK_QUAD,
                                    conn_count[i], verts);
                            break;
                        case M_TET:
                            connectivity[dom][mesh_id]->InsertNextCell(VTK_TETRA,
                                    conn_count[i], verts);
                            break;
                        case M_PYRAMID:
                            connectivity[dom][mesh_id]->InsertNextCell(VTK_PYRAMID,
                                    conn_count[i], verts);
                            break;
                        case M_WEDGE:
                            connectivity[dom][mesh_id]->InsertNextCell(VTK_WEDGE,
                                    conn_count[i], verts);
                            break;
                        case M_HEX:
                            if (conn[0] == conn[1] && conn[1] == conn[2] &&
                                    conn[2] == conn[3] && conn[4] == conn[5] &&
                                    conn[5] == conn[6] && conn[6] == conn[7])
                            {
                                connectivity[dom][mesh_id]->InsertNextCell(
                                    VTK_HEXAHEDRON,
                                    conn_count[i], verts);
                                /*                            connectivity[dom][mesh_id]->InsertNextCell(
                                                                        VTK_VERTEX,
                                                                        1, verts);*/
                            }
                            else
                            {
                                connectivity[dom][mesh_id]->InsertNextCell(
                                    VTK_HEXAHEDRON,
                                    conn_count[i], verts);
                            }
                            break;
                        default:
                            debug1 << "Unable to add cell" << endl;
                            break;
                    }

                    conn += conn_count[i];
                }
                conn = conn_list[i][j];
                delete [] conn;
            }
        }

        //
        // Hook up points to mesh if we have 'em
        //
        if (pts)
        {
            connectivity[dom][mesh_id]->SetPoints(pts);
            pts->Delete();
        }

    }// end mesh reading loop

    readMesh[dom] = true;
}

// ****************************************************************************
//  Method: avtMiliFileFormat::ValidateVariables
//
//  Purpose:
//      Read in the information to determine which vars are valid for
//      which subrecords. Also read in subrecord info.
//
//  Programmer: Hank Childs (adapted by Akira Haddox)
//  Creation:   June 25, 2003
//
//  Modifications:
//    Akira Haddox, Wed Jul 23 09:47:30 PDT 2003
//    Adapted code to assume it knows all the variables (which are now
//    obtained from the .mili file). Set validate vars flag after run.
//    Changed sub_records to hold the mili Subrecord structure.
//
//    Kathleen Bonnell, Wed Jul  6 14:27:42 PDT 2005
//    Initialize sv with memset to remove free of invalid pointer when
//    mc_cleanse_st_variable is called.
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added code to deal with param-array variables
//    Added memset call to zero-out Subrecord struct
//
//    Mark C. Miller, Mon Mar  6 14:25:49 PST 2006
//    Added call to cleanse subrec at end of loop to fix a memory leak
//
//    Mark C. Miller, Wed Mar  8 08:40:55 PST 2006
//    Moved code to cleanse subrec to destructor
//
// ****************************************************************************

void
avtMiliFileFormat::ValidateVariables(int dom)
{
    int rval;
    if (dbid[dom] == -1)
    {
        OpenDB(dom, false);
    }

    int srec_qty = 0;
    rval = mc_query_family(dbid[dom], QTY_SREC_FMTS, NULL, NULL,
                           (void*) &srec_qty);

    int i;
    for (i = 0;
            i < srec_qty;
            i++)
    {
        int substates = 0;
        rval = mc_query_family(dbid[dom], QTY_SUBRECS, (void *) &i, NULL,
                               (void *) &substates);

        for (int j = 0;
                j < substates;
                j++)
        {
            Subrecord sr;
            memset(&sr, 0, sizeof(sr));
            rval = mc_get_subrec_def(dbid[dom], i, j, &sr);

            //
            // To date, we believe none of the variables are valid for this
            // subrecord.  This will change as we look through its variable
            // list.
            //
            //
            for ( vars_iter =  vars.begin();
                    vars_iter != vars.end();
                    vars_iter++ )
            {
                std::string  var_name;
                var_name = (*vars_iter).first;

                bool pushVal = strncmp(var_name.c_str(), "params/", 7) == 0;
                // vars[var_name].vars_size[dom] = M_FLOAT;
            }

            std::string  var_name;

            for (int k = 0;
                    k < sr.qty_svars;
                    k++)
            {
                State_variable sv;
                memset(&sv, 0, sizeof(sv));
                mc_get_svar_def(dbid[dom], sr.svar_names[k], &sv);

                bool sameAsVar=false;
                for ( vars_iter =  vars.begin();
                        vars_iter != vars.end();
                        vars_iter++ )
                {
                    var_name = (*vars_iter).first;
                    if (var_name == sv.short_name)
                    {
                        sameAsVar = true;
                        break;
                    }
                }

                //
                // If we find the variable then record its sub-record id
                //
                if (sameAsVar)
                {
                    vars[var_name].sub_record_ids[dom].push_back(j);
                    vars[var_name].state_record_ids[dom].push_back(i);
                    vars[var_name].vars_size[dom] = sv.num_type;
                }

                mc_cleanse_st_variable(&sv);
            }
        }
    } // srec_qty

    validateVars[dom] = true;
}


// ****************************************************************************
//  Method: avtMiliFileFormat::ConstructMaterials
//
//  Purpose:
//      Constructs a material list from the partial material lists.
//
//  Programmer: Akira Haddox
//  Creation:   May 22, 2003
//
//  Modifications:
//    Akira Haddox, Tue Jul 22 09:21:39 PDT 2003
//    Find number of materials from global mesh information.
//
//    Hank Childs, Fri Aug 20 15:31:30 PDT 2004
//    Increment the material number here to match what the meta-data says.
//
// ****************************************************************************

avtMaterial *
avtMiliFileFormat::ConstructMaterials(vector< vector<int *> > &mat_list,
                                      vector< vector<int> > &list_size,
                                      int meshId)
{
    int size = 0;
    int i, j;
    for (i = 0; i < list_size.size(); ++i)
        for (j = 0; j < list_size[i].size(); ++j)
        {
            size += list_size[i][j];
        }

    int *mlist = new int[size];

    //
    // Fill in the material type sequentially for each zone. We go in
    // order of increasing element type.
    //
    int elem, gr;
    int count = 0;
    for (elem = 0; elem < mat_list.size(); ++elem)
    {
        for (gr = 0; gr < mat_list[elem].size(); ++gr)
        {
            int *ml = mat_list[elem][gr];
            for (i = 0; i < list_size[elem][gr]; ++i)
            {
                int mat = ml[i];
                mlist[count++] = mat;
            }
        }
    }

    vector<string> mat_names(nmaterials[meshId]);
    char str[32];
    for (i = 0; i < mat_names.size(); ++i)
    {
        // sprintf(str, "mat%d", i+1);  Bug is Visit - mat names must be an
        //                              int for now.

        sprintf(str, "%d", i+1);
        mat_names[i] = str;
    }

    avtMaterial *mat = new avtMaterial(nmaterials[meshId], mat_names, size,
                                       mlist, 0, NULL, NULL, NULL, NULL);

    delete [] mlist;
    return mat;
}


// ****************************************************************************
//  Method:  avtMiliFileFormat::RestrictVarToFreeNodes
//
//  Purpose: Restrict a given variable to the free nodes mesh
//
//  Programmer:  Mark C. Miller
//  Creation:    July 18, 2005
//
// ****************************************************************************
vtkFloatArray *
avtMiliFileFormat::RestrictVarToFreeNodes(vtkFloatArray *src, int ts) const
{
    if (free_nodes_ts != ts)
    {
        EXCEPTION2(UnexpectedValueException, free_nodes_ts, ts);
    }

    int ncomps = src->GetNumberOfComponents();
    vtkFloatArray *dst = vtkFloatArray::New();
    dst->SetNumberOfComponents(ncomps);
    dst->SetNumberOfTuples(num_free_nodes);
    float *dstp = (float *) dst->GetVoidPointer(0);
    float *srcp = (float *) src->GetVoidPointer(0);
    int fn;
    float f1, f2;

    for (int i = 0;
            i < num_free_nodes;
            i++)
        for (int j = 0;
                j < ncomps;
                j++)
        {
            if ( free_nodes[i]>0 )
            {
                dstp[i*ncomps+j] = srcp[free_nodes[i]*ncomps+j];
            }
        }

    return dst;
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::RestrictVarToPartNodes
//
//  Purpose: Restrict a given variable to the particle nodes mesh
//
//  Programmer:  Bob Corey
//  Creation:    May 22, 2012
//
// ****************************************************************************
vtkFloatArray *
avtMiliFileFormat::RestrictVarToPartNodes(vtkFloatArray *src, bool *cells, int dom, int ts) const
{
    int valid_cell_index=0;
    if (part_nodes_ts != ts)
    {
        EXCEPTION2(UnexpectedValueException, part_nodes_ts, ts);
    }

    int ncomps = src->GetNumberOfComponents();
    int pncount = num_part_elems[dom];
    vtkFloatArray *dst = vtkFloatArray::New();
    dst->SetNumberOfComponents(ncomps);
    dst->SetNumberOfTuples(pncount);
    float *dstp = (float *) dst->GetVoidPointer(0);
    float *srcp = (float *) src->GetVoidPointer(0);

    for (int i = 0;
            i < num_part_elems[dom];
            i++)
        for (int j = 0;
                j < ncomps;
                j++)
            if ( cells[i] )
            {
                dstp[valid_cell_index*ncomps+j] = srcp[i*ncomps+j];
                valid_cell_index++;
            }

    return dst;
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::RestrictVarToDBCNodes
//
//  Purpose: Restrict a given variable to the DBC nodes mesh
//
//  Programmer:  Bob Corey
//  Creation:    May 22, 2012
//
// ****************************************************************************
vtkFloatArray *
avtMiliFileFormat::RestrictVarToDBCNodes(vtkFloatArray *src, bool *cells, int dom, int ts) const
{
    int valid_cell_index=0;
    if (dbc_nodes_ts != ts)
    {
        EXCEPTION2(UnexpectedValueException, dbc_nodes_ts, ts);
    }

    int ncomps = src->GetNumberOfComponents();
    int dbccount = num_dbc_elems[dom];
    vtkFloatArray *dst = vtkFloatArray::New();
    dst->SetNumberOfComponents(ncomps);
    dst->SetNumberOfTuples(dbccount);
    float *dstp = (float *) dst->GetVoidPointer(0);
    float *srcp = (float *) src->GetVoidPointer(0);

    for (int i = 0;
            i < num_dbc_elems[dom];
            i++)
        for (int j = 0;
                j < ncomps;
                j++)
            if ( cells[i] )
            {
                dstp[valid_cell_index*ncomps+j] = srcp[i*ncomps+j];
                valid_cell_index++;
            }

    return dst;
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::GetVar
//
//  Purpose:
//      Gets variable 'var' for timestep 'ts'.
//
//  Arguments:
//    ts         the time step
//    var        the name of the variable to read
//
//  Programmer:  Hank Childs
//  Creation:    April 11, 2003
//
//  Modifications
//    Akira Haddox, Fri May 23 08:13:09 PDT 2003
//    Added in support for multiple meshes. Changed to MTMD.
//
//    Akira Haddox, Thu Jul 24 13:36:38 PDT 2003
//    Properly dealt with cell variable blocks.
//
//    Hank Childs, Tue Jul 20 15:53:30 PDT 2004
//    Add support for more data types (float, double, char, int, etc).
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added code to deal with param array variables
//    Added code to deal with variables defined on the free nodes mesh
//
//    Mark C. Miller, Wed Nov 15 01:46:16 PST 2006
//    Added "no_free_nodes" variants of variables. Changed names of
//    free_node variables from 'xxx_free_nodes' to 'free_nodes/xxx'
// ****************************************************************************

vtkDataArray *
avtMiliFileFormat::GetVar(int ts, int dom, const char *name)
{
    bool isParamArray=false;
    bool vecVar=false;
    string compName, vecVarName;
    int meshid = 0;
    string usename = name;
    bool isFreeNodesVar = false;
    bool isPartNodesVar = false;
    bool isDBCNodesVar = false;
    vtkFloatArray *rv = 0;
    int pncount   = num_part_elems[dom];
    int dbccount  = num_dbc_elems[dom];

    if (!readMesh[dom])
    {
        ReadMesh(dom);
    }

    if (strstr(name, no_free_nodes_str))
    {
        usename = string(name, no_free_nodes_strlen+1,
                         strlen(name) - (no_free_nodes_strlen+1));
        isFreeNodesVar = false;
    }
    else if (strstr(name, free_nodes_str))
    {
        usename = string(name, free_nodes_strlen+1,
                         strlen(name) - (free_nodes_strlen+1));
        isFreeNodesVar = true;
    }
    else if (strstr(name, part_nodes_str))
    {
        usename = string(name, part_nodes_strlen+1,
                         strlen(name) - (part_nodes_strlen+1));
        isPartNodesVar = true;
    }
    else if (strstr(name, dbc_nodes_str))
    {
        usename = string(name, dbc_nodes_strlen+1,
                         strlen(name) - (dbc_nodes_strlen+1));
        isDBCNodesVar = true;
    }


    //
    // Populate Enum Vars
    //
    if ( strstr(name, "/MiliClasses") )
    {
        vtkIntArray *rv = vtkIntArray::New();
        int i=0, npoints=0, pt_index=0;
        if ( isPartNodesVar )
        {
            npoints = pncount;
        }
        else if ( isDBCNodesVar )
        {
            npoints = dbccount;
        }
        else
        {
            npoints = ncells[dom][meshid];
        }

        rv->SetNumberOfTuples(npoints);
        int *p = (int *) rv->GetVoidPointer(0);
        for ( i=0;
                i<qty_cells;
                i++ )
        {
            p[pt_index++] = global_class_ids[dom][i];
        }
        return rv;
    }

    if ( strstr(name, "/MiliSand") )
    {
        vtkIntArray *rv = vtkIntArray::New();
        char sandVar[64];
        int i=0, npoints=0;
        if ( isPartNodesVar )
        {
            npoints = pncount;
        }
        else if ( isDBCNodesVar )
        {
            npoints = dbccount;
        }
        else
        {
            npoints = ncells[dom][meshid];
        }

        strcpy( sandVar, "sand" );
        if ( strstr(name, part_nodes_str ) )
        {
            npoints = num_part_elems[dom];
            strcpy( sandVar, part_nodes_str );
            strcat( sandVar, "/sand" );
        }

        rv->SetNumberOfTuples(npoints);
        int *p = (int *) rv->GetVoidPointer(0);
        vtkFloatArray *sand_arr = (vtkFloatArray *) GetVar(ts, dom, sandVar);
        float *sand_vals = (float*) sand_arr->GetVoidPointer(0);
        for ( i=0;
                i<npoints;
                i++ )
        {
            p[i] = (int) sand_vals[i];
        }
        return rv;
    }
    if ( strstr(name, "/MiliSphType") )
    {
        vtkIntArray *rv = vtkIntArray::New();
        int i=0, npoints=0;
        char sphVar[64];

        if ( isPartNodesVar )
        {
            npoints = pncount;
        }
        else
        {
            npoints = ncells[dom][meshid];
        }

        rv->SetNumberOfTuples(npoints);
        int *p = (int *) rv->GetVoidPointer(0);

        if ( strstr(name, part_nodes_str ) )
        {
            npoints = num_part_elems[dom];
            strcpy( sphVar, part_nodes_str );
            strcat( sphVar, "/sph_itype" );
        }
        else
        {
            strcpy( sphVar, "sph_itype" );
        }

        vtkFloatArray *sph_arr = (vtkFloatArray *) GetVar(ts, dom, sphVar);
        float *sph_vals = (float*) sph_arr->GetVoidPointer(0);

        for ( i=0;
                i<npoints;
                i++ )
        {
            p[i] = (int) sph_vals[i];
        }
        return rv;
    }
    if ( !strcmp(name, "Classes_Zone") )
    {
        vtkIntArray *rv = vtkIntArray::New();
        int i=0, npoints=0, pt_index=0;
        if ( isPartNodesVar )
        {
            npoints = pncount;
        }
        else
        {
            npoints = ncells[dom][meshid];
        }

        rv->SetNumberOfTuples(npoints);
        int *p = (int *) rv->GetVoidPointer(0);
        for ( i=0;
                i<qty_cells;
                i++ )
        {
            p[pt_index++] = global_class_ids[dom][i];
        }
        return rv;
    }

    //
    // Read data for Global and Material results.
    //
    if (strncmp(name, "Global/", 7) == 0)
    {
        vtkFloatArray *rv = vtkFloatArray::New();
        Subrecord sr;
        int start = 0;
        int csize = 0;
        int i=0, npoints=0;
        int sr_index, j;
        int rval;

        string vname;

        DecodeMultiLevelVarname(usename, vname);
        char *tmp = (char *) vname.c_str();

        sr_index =  vars[vname].sub_record_ids[dom][0];
        j        =  vars[vname].state_record_ids[dom][0];
        rval     = mc_get_subrec_def(dbid[dom], j, sr_index, &sr);

        GetSizeInfoForGroup(sr.class_name, start,
                            csize, dom);

        float *arr = new float[csize*2];

        rv->SetNumberOfTuples(ncells[dom][meshid]);
        float *p = (float *) rv->GetVoidPointer(0);
        int vsize = vars[vname].vars_size[dom];

        npoints = ncells[dom][meshid];
        for (i = 0;
                i < ncells[dom][meshid];
                i++)
        {
            p[i] = 0.;
        }

        read_results(dbid[dom], ts+1, vars[vname].sub_record_ids[dom][0],
                     1, &tmp, vsize, csize, arr);
        for (i = 0;
                i < ncells[dom][meshid];
                i++)
        {
            p[i] = arr[0];
        }
        return rv;
    }

    if (strncmp(name, "Material/", 9) == 0)
    {
        vtkFloatArray *rv = vtkFloatArray::New();
        Subrecord sr;
        int start = 0;
        int csize = 0;
        int i=0, npoints=0;
        int sr_index, j;
        int rval;
        string vname;

        DecodeMultiLevelVarname(usename, vname);
        char *tmp = (char *) vname.c_str();

        sr_index =  vars[vname].sub_record_ids[dom][0];
        j        =  vars[vname].state_record_ids[dom][0];
        rval     = mc_get_subrec_def(dbid[dom], j, sr_index, &sr);

        GetSizeInfoForGroup(sr.class_name, start,
                            csize, dom);

        float *arr = new float[csize*2];

        rv->SetNumberOfTuples(ncells[dom][meshid]);
        float *p = (float *) rv->GetVoidPointer(0);
        int vsize = vars[vname].vars_size[dom];

        npoints = ncells[dom][meshid];
        for (i = 0;
                i < ncells[dom][meshid];
                i++)
        {
            p[i] = 0.;
        }

        read_results(dbid[dom], ts+1, vars[vname].sub_record_ids[dom][0],
                     1, &tmp, vsize, csize, arr);
        int p_index=0;
        for (i = 0;
                i < qty_cells;
                i++)
        {
            p[p_index++] = arr[global_matlist[dom][i]];
        }

        return rv;
    }

    if (!readMesh[dom])
    {
        ReadMesh(dom);
    }

    //
    // Process variables from Mili TOC File
    //

    if (!validateVars[dom])
    {
        ValidateVariables(dom);
    }

    size_t found;
    string vname, var_name;

    if (nmeshes != 1)
    {
        DecodeMultiMeshVarname(usename, vname, meshid);
        usename = vname;
    }

    // Seperate var dir from name
    DecodeMultiLevelVarname(usename, compName);
    vecVar = isVecVar(usename, vecVarName);

    DecodeMultiLevelVarname(usename, vname);
    if (strncmp(vname.c_str(), "params/", 7) == 0)
    {
        isParamArray=true;
    }

    if ( vecVar )
    {
        vname = vecVarName;
    }

    var_name = vname;

    int v_index = GetVariableIndex(vname.c_str(), meshid);
    int mesh_id = vars[var_name].var_mesh_associations;

    bool *part_elem_cells=NULL;
    if (isPartNodesVar)
    {
        part_elem_cells = new bool[ncells[dom][mesh_id]];
        memset( part_elem_cells, false, sizeof(bool)*ncells[dom][mesh_id] );
    }

    if (vars[var_name].centering == AVT_NODECENT)
    {
        int i;
        int nvars = 0;
        int sr_valid = -1;
        for (i = 0;
                i < vars[var_name].sub_record_ids[dom].size();
                i++)
        {
            sr_valid = i;
            nvars++;
        }
        if (!isParamArray && nvars != 1)
        {
            ErrorHandler(__FUNCTION__, __LINE__);
            EXCEPTION1(InvalidVariableException, name);
        }
        int vsize = vars[var_name].vars_size[dom];

        // Since data in param arrays is constant over all time,
        // we just cache it here in the plugin. Lets look in the
        // cache *before* we try to read it (again).
        if (isParamArray)
        {
            rv = (vtkFloatArray*) cache->GetVTKObject(usename.c_str(),
                    avtVariableCache::SCALARS_NAME, -1, dom, "none");
        }

        if (rv == 0)
        {
            int amt = nnodes[dom][mesh_id];
            rv = vtkFloatArray::New();
            rv->SetNumberOfTuples(amt);
            float *p = (float *) rv->GetVoidPointer(0);

            string vname;
            DecodeMultiLevelVarname(usename, vname);
            if ( vecVar )
            {
                vname = vecVarName+"["+compName+"]";
            }

            char *tmp = (char *) vname.c_str();  // Bypass const
            read_results(dbid[dom], ts+1, vars[var_name].sub_record_ids[dom][sr_valid], 1,
                         &tmp, vsize, amt, p);

            //
            // We explicitly cache param arrays at ts=-1
            //
            if (isParamArray)
            {
                cache->CacheVTKObject(usename.c_str(), avtVariableCache::SCALARS_NAME,
                                      -1, dom, "none", rv);
            }
        }
        else
        {
            // The reference count will be decremented by the generic database,
            // because it will assume it owns it.
            rv->Register(NULL);
        }

        //
        // Restrict variables on free nodes to the free nodes mesh
        //
        if (isFreeNodesVar)
        {
            vtkFloatArray *newrv = RestrictVarToFreeNodes(rv, ts);
            rv->Delete();
            rv = newrv;
        }

        //
        // Restrict variables on particles to the particles mesh
        //
        if (isPartNodesVar)
        {
            vtkFloatArray *newrv = RestrictVarToPartNodes(rv, part_elem_cells, dom, ts);
            rv->Delete();
            rv = newrv;
            delete [] part_elem_cells;
        }
        \
        //
        // Restrict variables on particles to the DBC mesh
        //
        if (isDBCNodesVar)
        {
            vtkFloatArray *newrv = RestrictVarToDBCNodes(rv, part_elem_cells, dom, ts);
            rv->Delete();
            rv = newrv;
            delete [] part_elem_cells;
        }
    }
    else
    {
        rv = vtkFloatArray::New();
        rv->SetNumberOfTuples(ncells[dom][mesh_id]);
        float *p = (float *) rv->GetVoidPointer(0);

        int i;
        for (i = 0;
                i < ncells[dom][mesh_id];
                i++)
        {
            p[i] = 0.;
        }

        for (i = 0;
                i < vars[var_name].sub_record_ids[dom].size();
                i++)
        {
            int vsize = vars[var_name].vars_size[dom];
            int start = 0;
            int csize = 0;
            Subrecord sr;
            int sr_index, j;
            int rval;

            sr_index =  vars[var_name].sub_record_ids[dom][i];
            j        =  vars[var_name].state_record_ids[dom][i];
            memset(&sr, 0, sizeof(sr));
            rval = mc_get_subrec_def(dbid[dom], j, sr_index, &sr);

            GetSizeInfoForGroup(sr.class_name, start,
                                csize, dom);

            string vname;
            DecodeMultiLevelVarname(usename, vname);
            if ( vecVar )
            {
                vname = vecVarName+"["+compName+"]";
            }

            char *tmp = (char *) vname.c_str();  // Bypass const

            // Simple read in: one block
            if (sr.qty_blocks == 1)
            {
                int k=0;
                int amount=0;
                int cell_index=0;

                // Adjust start
                amount = (sr.mo_blocks[1] - sr.mo_blocks[0]) +1;
                start += sr.mo_blocks[0] - 1;

                if ( isPartNodesVar && part_elem_cells )
                    for ( cell_index=start;
                            cell_index<start+amount;
                            cell_index++ )
                    {
                        part_elem_cells[cell_index] = true;
                    }

                read_results(dbid[dom], ts+1, vars[var_name].sub_record_ids[dom][i],
                             1, &tmp, vsize, amount, p + start);
            }
            else
            {
                int nBlocks = sr.qty_blocks;
                int *blocks = sr.mo_blocks;

                int pSize = 0;
                int b;
                for (b = 0; b < nBlocks; ++b)
                {
                    pSize += blocks[b * 2 + 1] - blocks[b * 2] + 1;
                }

                float *arr = new float[pSize];

                read_results(dbid[dom], ts + 1, vars[var_name].sub_record_ids[dom][i],
                             1, &tmp, vsize, pSize, arr);

                float *ptr = arr;
                // Fill up the blocks into the array.
                start=0;
                for (b = 0; b < nBlocks; ++b)
                {
                    int c;
                    for (c = blocks[b * 2] - 1; c <= blocks[b * 2 + 1] - 1;
                            ++c)
                    {
                        p[c +start] = *(ptr++);
                        if ( isPartNodesVar )
                        {
                            part_elem_cells[c +start] = true;
                        }
                    }
                }
                delete [] arr;
            }
            mc_cleanse_subrec(&sr);
        }

        //
        // Restrict variables on particles to the particles mesh
        //
        if (isPartNodesVar)
        {
            vtkFloatArray *newrv = RestrictVarToPartNodes(rv, part_elem_cells, dom, ts);
            rv->Delete();
            rv = newrv;
            delete [] part_elem_cells;
        }
    }
    return rv;
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::GetVectorVar
//
//  Purpose:
//      Gets variable 'var' for timestep 'ts'.
//
//  Arguments:
//    ts         the time step
//    var        the name of the variable to read
//
//  Programmer:  Hank Childs
//  Creation:    April 11, 2003
//
//  Modifications
//    Akira Haddox, Fri May 23 08:13:09 PDT 2003
//    Added in support for multiple meshes. Changed to MTMD.
//
//    Akira Haddox, Thu Jul 24 13:36:38 PDT 2003
//    Properly dealt with cell variable blocks.
//
//    Hank Childs, Mon Sep 22 07:36:48 PDT 2003
//    Add support for reading in tensors.
//
//    Hank Childs, Tue Jul 20 15:53:30 PDT 2004
//    Add support for more data types (float, double, char, int, etc).
//
//    Hank Childs, Tue Jul 27 12:42:12 PDT 2004
//    Fix problem with reading in double nodal vectors.
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added code to deal with variables defined on the free nodes mesh
//
//    Mark C. Miller, Wed Nov 15 01:46:16 PST 2006
//    Added "no_free_nodes" variants of variables. Changed names of
//    free_node variables from 'xxx_free_nodes' to 'free_nodes/xxx'
// ****************************************************************************

vtkDataArray *
avtMiliFileFormat::GetVectorVar(int ts, int dom, const char *name)
{
    if (!readMesh[dom])
    {
        ReadMesh(dom);
    }
    if (!validateVars[dom])
    {
        ValidateVariables(dom);
    }

    string usename = name;
    string var_name, tmpname;
    int meshid = 0;

    bool isFreeNodesVar = false;
    bool isPartNodesVar = false;

    if (strstr(name, no_free_nodes_str))
    {
        usename = string(name, no_free_nodes_strlen+1,
                         strlen(name) - (no_free_nodes_strlen+1));
        isFreeNodesVar = false;
    }
    else if (strstr(name, free_nodes_str))
    {
        usename = string(name, free_nodes_strlen+1,
                         strlen(name) - (free_nodes_strlen+1));
        isFreeNodesVar = true;
    }
    else if (strstr(name, part_nodes_str))
    {
        usename = string(name, part_nodes_strlen+1,
                         strlen(name) - (part_nodes_strlen+1));
        isPartNodesVar = true;
    }

    if (nmeshes == 1)
    {
        var_name = usename;
        DecodeMultiLevelVarname(var_name, tmpname);
        var_name   = tmpname;
        usename = tmpname;
    }
    else
    {
        DecodeMultiMeshVarname(usename, var_name, meshid);
    }

    int v_index = GetVariableIndex(var_name.c_str(), meshid);
    int mesh_id = vars[var_name].var_mesh_associations;

    //
    // We stuff tensors into the vector field, so explicitly look up the
    // dimension of vector (3 for vector, 6 for symm tensor, 9 for tensor).
    //
    int vdim = vars[var_name].var_dimension;

    vtkFloatArray *rv = vtkFloatArray::New();
    rv->SetNumberOfComponents(vdim);

    bool *part_elem_cells=NULL;
    if (isPartNodesVar)
    {
        part_elem_cells = new bool[ncells[dom][mesh_id]];
        memset( part_elem_cells, false, sizeof(bool)*ncells[dom][mesh_id] );
    }

    if ( vars[var_name].centering == AVT_NODECENT)
    {
        int nvars = 0;
        int sr_valid = -1;
        for (int i = 0;
                i < vars[var_name].sub_record_ids[dom].size();
                i++)
        {
            sr_valid = i;
            nvars++;
        }
        if (nvars != 1)
        {
            ErrorHandler(__FUNCTION__, __LINE__);
            EXCEPTION1(InvalidVariableException, name);
        }

        int vsize = vars[var_name].vars_size[dom];
        int amt = nnodes[dom][mesh_id];
        rv->SetNumberOfTuples(amt);
        float *ptr = (float *) rv->GetVoidPointer(0);

        DecodeMultiLevelVarname(usename, var_name);
        char *tmp = (char *) var_name.c_str();  // Bypass const

        read_results(dbid[dom], ts+1, vars[var_name].sub_record_ids[dom][sr_valid], 1,
                     &tmp, vsize, amt*vdim, ptr);

        //
        // Restrict variables on free nodes to the free nodes mesh
        //
        if (isFreeNodesVar)
        {
            vtkFloatArray *newrv = RestrictVarToFreeNodes(rv, ts);
            rv->Delete();
            rv = newrv;
        }

        //
        // Restrict variables on particles to the particles mesh
        //
        if (isPartNodesVar)
        {
            vtkFloatArray *newrv = RestrictVarToPartNodes(rv, part_elem_cells, dom, ts);
            rv->Delete();
            rv = newrv;
            delete [] part_elem_cells;
        }
    }
    else
    {
        rv->SetNumberOfTuples(ncells[dom][mesh_id]);
        float *p = (float *) rv->GetVoidPointer(0);
        int i;
        int nvals = ncells[dom][mesh_id] * vdim;
        for (i = 0 ; i < nvals ; i++)
        {
            p[i] = 0.;
        }

        for (int i = 0;
                i < vars[var_name].sub_record_ids[dom].size();
                i++)
        {
            int vsize = vars[var_name].vars_size[dom];
            int start = 0;
            int csize = 0;

            Subrecord sr;
            int sr_index, j;
            int rval;

            sr_index =  vars[var_name].sub_record_ids[dom][i];
            j        =  vars[var_name].state_record_ids[dom][i];
            memset(&sr, 0, sizeof(sr));
            rval = mc_get_subrec_def(dbid[dom], j, sr_index, &sr);

            GetSizeInfoForGroup(sr.class_name, start,
                                csize, dom);

            DecodeMultiLevelVarname(usename, var_name);
            char *tmp = (char *) var_name.c_str();  // Bypass const

            // Simple read in: one block
            if (sr.qty_blocks == 1)
            {
                int amount=0;
                int cell_index=0;

                // Adjust start
                start += (sr.mo_blocks[0] - 1);
                amount = (sr.mo_blocks[1] - sr.mo_blocks[0]) + 1;

                if ( isPartNodesVar )
                    for ( cell_index=start;
                            cell_index<start+amount;
                            cell_index++ )
                    {
                        part_elem_cells[cell_index] = true;
                    }

                read_results(dbid[dom], ts+1, vars[var_name].sub_record_ids[dom][i],
                             1, &tmp, vsize, csize*vdim, p + vdim * start);
            }
            else
            {
                int nBlocks = sr.qty_blocks;
                int *blocks = sr.mo_blocks;

                int pSize = 0;
                int b;
                for (b = 0; b < nBlocks; ++b)
                {
                    pSize += blocks[b * 2 + 1] - blocks[b * 2] + 1;
                }

                float *arr = new float[pSize * vdim];

                read_results(dbid[dom], ts + 1, vars[var_name].sub_record_ids[dom][i],
                             1, &tmp, vsize, pSize*vdim, arr);

                float *ptr = arr;

                // Fill up the blocks into the array.
                for (b = 0; b < nBlocks; ++b)
                {
                    int c, k;
                    for (c = blocks[b * 2] - 1; c <= blocks[b * 2 + 1] - 1;
                            ++c)
                        for (k = 0; k < vdim; ++k)
                        {
                            p[vdim * (c + start) + k] = *(ptr++);
                        }
                }
                delete [] arr;
            }
            {
                int vsize = vars[var_name].vars_size[dom];
                int start = 0;
                int csize = 0;

                Subrecord sr;
                int sr_index, j;
                int rval;

                sr_index =  vars[var_name].sub_record_ids[dom][i];
                j        =  vars[var_name].state_record_ids[dom][i];
                memset(&sr, 0, sizeof(sr));
                rval = mc_get_subrec_def(dbid[dom], j, sr_index, &sr);

                GetSizeInfoForGroup(sr.class_name, start,
                                    csize, dom);

                DecodeMultiLevelVarname(usename, var_name);
                char *tmp = (char *) var_name.c_str();  // Bypass const

                // Simple read in: one block
                if (sr.qty_blocks == 1)
                {
                    // Adjust start
                    start += (sr.mo_blocks[0] - 1);

                    read_results(dbid[dom], ts+1, vars[var_name].sub_record_ids[dom][i],
                                 1, &tmp, vsize, csize*vdim, p + vdim * start);
                }
                else
                {
                    int nBlocks = sr.qty_blocks;
                    int *blocks = sr.mo_blocks;

                    int pSize = 0;
                    int b;
                    for (b = 0; b < nBlocks; ++b)
                    {
                        pSize += blocks[b * 2 + 1] - blocks[b * 2] + 1;
                    }

                    float *arr = new float[pSize * vdim];

                    read_results(dbid[dom], ts + 1, vars[var_name].sub_record_ids[dom][i],
                                 1, &tmp, vsize, pSize*vdim, arr);

                    float *ptr = arr;

                    // Fill up the blocks into the array.
                    for (b = 0; b < nBlocks; ++b)
                    {
                        int c, k;
                        for (c = blocks[b * 2] - 1; c <= blocks[b * 2 + 1] - 1;
                                ++c)
                            for (k = 0; k < vdim; ++k)
                            {
                                p[vdim * (c + start) + k] = *(ptr++);
                                if ( part_elem_cells )
                                {
                                    part_elem_cells[c+start] = true;
                                }
                            }
                    }
                    delete [] arr;
                }
                mc_cleanse_subrec(&sr);
            }
        }
        //
        // Restrict variables on particles to the particles mesh
        //
        if (isPartNodesVar)
        {
            vtkFloatArray *newrv = RestrictVarToPartNodes(rv, part_elem_cells, dom, ts);
            rv->Delete();
            rv = newrv;
            delete [] part_elem_cells;
        }
    }

    //
    // If we have a symmetric tensor, put that in the form of a normal
    // tensor.
    //
    if (vdim == 6)
    {
        vtkFloatArray *new_rv = vtkFloatArray::New();
        int ntups = rv->GetNumberOfTuples();
        new_rv->SetNumberOfComponents(9);
        new_rv->SetNumberOfTuples(ntups);
        for (int i = 0 ; i < ntups ; i++)
        {
            double orig_vals[6];
            float new_vals[9];
            rv->GetTuple(i, orig_vals);
            new_vals[0] = orig_vals[0];  // XX
            new_vals[1] = orig_vals[3];  // XY
            new_vals[2] = orig_vals[5];  // XZ
            new_vals[3] = orig_vals[3];  // YX
            new_vals[4] = orig_vals[1];  // YY
            new_vals[5] = orig_vals[4];  // YZ
            new_vals[6] = orig_vals[5];  // ZX
            new_vals[7] = orig_vals[4];  // ZY
            new_vals[8] = orig_vals[2];  // ZZ
            new_rv->SetTuple(i, new_vals);
        }
        rv->Delete();
        rv = new_rv;
    }

    return rv;
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::GetCycles
//
//  Purpose:
//      Returns the actual cycle numbers for each time step.
//
//  Arguments:
//   cycles      the output vector of cycle numbers
//
//  Programmer:  Hank Childs
//  Creation:    April 11, 2003
//
// ****************************************************************************

void
avtMiliFileFormat::GetCycles(vector<int> &cycles)
{
    int nTimesteps = GetNTimesteps();

    cycles.resize(nTimesteps);
    for (int i = 0 ; i < nTimesteps ; i++)
    {
        cycles[i] = i;
    }
}


// ****************************************************************************
//  Method:  avtMiliFileFormat::GetTimes
//
//  Purpose:
//      Returns the actual times for each time step.
//
//  Arguments:
//   out_times   the output vector of times
//
//  Programmer:  Hank Childs
//  Creation:    October 20, 2003
//
// ****************************************************************************

void
avtMiliFileFormat::GetTimes(vector<double> &out_times)
{
    out_times = times;
}


// ****************************************************************************
//  Method:  avtMiliFileFormat::GetNTimesteps
//
//  Purpose:
//      Returns the number of timesteps
//
//  Programmer:  Hank Childs
//  Creation:    April 11, 2003
//
// ****************************************************************************
int
avtMiliFileFormat::GetNTimesteps()
{
    return ntimesteps;
}


// ****************************************************************************
//  Method:  avtMiliFileFormat::PopulateDatabaseMetaData
//
//  Purpose:
//    Returns meta-data about the database.
//
//  Arguments:
//    md         The meta-data structure to populate
//    timeState  The time index to use (if metadata varies with time)
//
//  Programmer:  Hank Childs
//  Creation:    April 11, 2003
//
//  Modifications
//    Akira Haddox, Fri May 23 08:13:09 PDT 2003
//    Added in support for multiple meshes. Changed for MTMD.
//
//    Hank Childs, Sat Sep 20 08:15:54 PDT 2003
//    Added support for tensors and add some expressions based on tensors.
//
//    Hank Childs, Sat Oct 18 09:51:03 PDT 2003
//    Fix typo for strain/stress expressions.
//
//    Hank Childs, Sat Oct 18 10:53:51 PDT 2003
//    Do not read in the partition info if we are on the mdserver.
//
//    Hank Childs, Mon Oct 20 10:07:00 PDT 2003
//    Call OpenDB for domain 0 to populate the times.
//
//    Hank Childs, Sat Jun 26 10:28:45 PDT 2004
//    Make the materials start at "1" and go up.  Also make the domain
//    decomposition say processor instead of block.
//
//    Hank Childs, Wed Aug 18 16:25:15 PDT 2004
//    Added new expressions for displacement and position.
//
//    Mark C. Miller, Tue May 17 18:48:38 PDT 2005
//    Added timeState arg to satisfy new interface
//
//    Mark C. Miller, Mon Jul 18 13:41:13 PDT 2005
//    Added code to add free nodes mesh and variables
//
//    Mark C. Miller, Wed Nov 15 01:46:16 PST 2006
//    Added "no_free_nodes" variants of meshes, material and variables.
//    Changed names of free_node variables from 'xxx_free_nodes' to
//    'free_nodes/xxx'. Populated any node-centered expressions for both
//    original and free_node variants and zone-centered expressions for
//    both original and no_free_node variants. Changed cellOrigin to 1
//    to address off-by-one errors during pick. Bob Corey says that so far,
//    all clients that write mili data are Fortran clients. They expect to
//    get node/zone numbers from pick starting from '1'.
//
//    Mark C. Miller, Wed Nov 29 12:08:49 PST 2006
//    Suppress creation of "no_free_nodes" flavors of expressions when
//    not needed
//
//    Thomas R. Treadway, Tue Dec  5 15:14:11 PST 2006
//    Added a derived strain and displacement algorithms
//
//    Mark C. Miller, Tue Mar 27 08:39:55 PDT 2007
//    Added support for node origin
// ****************************************************************************
void
avtMiliFileFormat::PopulateDatabaseMetaData(avtDatabaseMetaData *md,
        int timeState)
{
    int i, j, k;

    bool has_any_fn_mesh = false, has_any_pn_mesh = false, has_any_dbc_mesh = false;

    vector<bool> has_fn_mesh;
    std::string MiliDerivedName = "Mili-Derived/";
    char vname[256];
    int status=0;

    qty_cells = 0;
    qty_nodes = 0;

    part_nodes       = new int *[ndomains];
    part_elems       = new int *[ndomains];
    num_part_elems   = new int  [ndomains];

    dbc_nodes       = new int *[ndomains];
    dbc_elems       = new int *[ndomains];
    num_dbc_elems   = new int  [ndomains];

    global_class_ids = new int *[ndomains];
    global_matlist   = new int *[ndomains];

    //
    // Collect all of the Svar definitions.
    //
    for ( vars_iter =  vars.begin();
            vars_iter != vars.end();
            vars_iter++ )
    {
        std::string  var_name;
        var_name = (*vars_iter).first;
        if (var_name == "sand" )
        {
            free_nodes_found = true;
        }
    }

    for ( int dom=0;
            dom<ndomains;
            dom++ )
    {

        bool quickOpen=true;
        OpenDB(dom, quickOpen);

        part_nodes[dom]     = 0;
        part_elems[dom]     = 0;
        num_part_elems[dom] = 0;
        dbc_nodes[dom]      = 0;
        dbc_elems[dom]      = 0;
        num_dbc_elems[dom]  = 0;

        for ( vars_iter =  vars.begin();
                vars_iter != vars.end();
                vars_iter++ )
        {
            std::string  var_name;
            var_name = (*vars_iter).first;
            strcpy( vname, var_name.c_str() );

            svars_iter = svars.find(var_name);
            if ( svars_iter!=svars.end() )
            {
                continue;
            }

            State_variable sv;
            int type;

            memset(&sv, 0, sizeof(sv));
            mc_get_svar_def(dbid[dom], vname, &sv);
            svars[var_name] = sv;
        }
        for (int mesh_id = 0;
                mesh_id < nmeshes;
                mesh_id++)
        {
            PopulateMiliClassData( dom, mesh_id );
            PopulateMiliVarData(   dom, mesh_id );
        }
        PopulateClassMetaData( dom, md );
        CloseDB(dom);
    }
    PopulatePrimalMetaData( md );

    for (i = 0;
            i < nmeshes;
            ++i)
    {
        char meshname[32];
        char matname[32];
        sprintf(meshname, "mesh%d", i + 1);
        sprintf(matname, "materials%d", i + 1);

        const string fnmeshname  = string(meshname) + "_" + string(free_nodes_str);
        const string pnmeshname  = string(meshname) + "_" + string(part_nodes_str);
        const string dbcmeshname = string(meshname) + "_" + string(dbc_nodes_str);

        // Free Nodes Mesh
        const string nofnmeshname = string(meshname) + "_" + string(no_free_nodes_str);
        const string nofnmatname  = string(meshname) + "_" + string(no_free_nodes_str);

        avtMeshMetaData *mesh = new avtMeshMetaData;
        mesh->name = meshname;
        mesh->meshType = AVT_UNSTRUCTURED_MESH;
        mesh->numBlocks = ndomains;
        mesh->blockOrigin = 0;
        mesh->cellOrigin = 1; // Bob Corey says all mili writers so far are Fortran
        mesh->nodeOrigin = 1; // Bob Corey says all mili writers so far are Fortran
        mesh->spatialDimension = dims;
        mesh->topologicalDimension = dims;
        mesh->blockTitle = "processors1";
        mesh->blockPieceName = "processor1";
        mesh->hasSpatialExtents = false;
        md->Add(mesh);

        vector<string> mnames(nmaterials[i]);
        int j;
        char str[32];
        for (j = 0; j < nmaterials[i]; ++j)
        {
            // sprintf(str, "mat%d", i+1);  Bug is Visit - mat names must be an
            //                              int for now.

            sprintf(str, "%d", j+1);
            mnames[j] = str;
        }
        AddMaterialToMetaData(md, matname, meshname, nmaterials[i], mnames);
        AddMaterialToMetaData(md, nofnmatname, nofnmeshname, nmaterials[i], mnames);

        //
        // Add the free-nodes and no-free-nodes meshes
        // if variable "sand" is defined on this mesh
        //
        has_fn_mesh.push_back(false);
        for ( vars_iter =  vars.begin();
                vars_iter != vars.end();
                vars_iter++ )
        {
            std::string  var_name;
            var_name = (*vars_iter).first;
            if (var_name == "sand" &&  vars[var_name].var_mesh_associations == i)
            {
                avtMeshMetaData *fnmesh = new avtMeshMetaData;
                fnmesh->name = fnmeshname;
                fnmesh->meshType = AVT_POINT_MESH;
                fnmesh->numBlocks = ndomains;
                fnmesh->blockOrigin = 0;
                fnmesh->cellOrigin = 1; // All mili writers so far are Fortran
                fnmesh->nodeOrigin = 1; // All mili writers so far are Fortran
                fnmesh->spatialDimension = dims;
                fnmesh->topologicalDimension = 0;
                fnmesh->blockTitle = "processors";
                fnmesh->blockPieceName = "processor";
                fnmesh->hasSpatialExtents = false;
                md->Add(fnmesh);

                avtMeshMetaData *nofnmesh = new avtMeshMetaData;
                nofnmesh->name = nofnmeshname;
                nofnmesh->meshType = AVT_UNSTRUCTURED_MESH;
                nofnmesh->numBlocks = ndomains;
                nofnmesh->blockOrigin = 0;
                nofnmesh->cellOrigin = 1; // All mili writers so far are Fortran
                nofnmesh->nodeOrigin = 1; // All mili writers so far are Fortran
                nofnmesh->spatialDimension = dims;
                nofnmesh->topologicalDimension = dims;
                nofnmesh->blockTitle = "processors";
                nofnmesh->blockPieceName = "processor";
                nofnmesh->hasSpatialExtents = false;
                md->Add(nofnmesh);

                has_fn_mesh[i] = true;
                has_any_fn_mesh = true;
            }
        }

        // Add the SPH Particle Meshes
        if ( numSPH_classes>0 )
        {
            avtMeshMetaData *pnmesh = new avtMeshMetaData;
            const string pnmatname = string(meshname) + "_" + string(part_nodes_str);
            AddMaterialToMetaData(md, pnmatname, pnmeshname, nmaterials[i], mnames);

            pnmesh->name = pnmeshname;
            pnmesh->meshType = AVT_POINT_MESH;
            pnmesh->numBlocks = ndomains;
            pnmesh->blockOrigin = 0;
            pnmesh->cellOrigin = 1; // All mili writers so far are Fortran
            pnmesh->nodeOrigin = 1; // All mili writers so far are Fortran
            pnmesh->spatialDimension = dims;
            pnmesh->topologicalDimension = 0;
            pnmesh->blockTitle = "processors";
            pnmesh->blockPieceName = "processor";
            pnmesh->hasSpatialExtents = false;
            md->Add(pnmesh);
            has_any_pn_mesh = true;
        }

        // Add the DBC Particle Meshes
        if ( numDBC_classes>0 )
        {
            avtMeshMetaData *dbcmesh = new avtMeshMetaData;
            const string dbcmatname = string(meshname) + "_" + string(dbc_nodes_str);
            AddMaterialToMetaData(md, dbcmatname, dbcmeshname, nmaterials[i], mnames);

            dbcmesh->name = dbcmeshname;
            dbcmesh->meshType = AVT_POINT_MESH;
            dbcmesh->numBlocks = ndomains;
            dbcmesh->blockOrigin = 0;
            dbcmesh->cellOrigin = 1; // All mili writers so far are Fortran
            dbcmesh->nodeOrigin = 1; // All mili writers so far are Fortran
            dbcmesh->spatialDimension = dims;
            dbcmesh->topologicalDimension = 0;
            dbcmesh->blockTitle = "processors";
            dbcmesh->blockPieceName = "processor";
            dbcmesh->hasSpatialExtents = false;
            md->Add(dbcmesh);
            has_any_dbc_mesh = true;
        }
    }

    // Add Free-node and Particle-node paths
    vector<string> nofndirs;
    nofndirs.push_back("");
    if (has_any_fn_mesh)
    {
        nofndirs.push_back(string(no_free_nodes_str) + "/");
    }

    vector<string> fndirs;
    fndirs.push_back("");
    if (has_any_fn_mesh)
    {
        fndirs.push_back(string(free_nodes_str) + "/");
    }

    vector<string> fnname;
    fnname.push_back("");
    if (has_any_fn_mesh)
    {
        fnname.push_back("_" + string(free_nodes_str));
    }

    // Add Particle-node Paths
    vector<string> pndirs;
    if (has_any_pn_mesh)
    {
        pndirs.push_back(string((part_nodes_str)) + "/");
    }

    vector<string> pnname;
    if (has_any_pn_mesh)
    {
        pnname.push_back("_" + string(part_nodes_str));
    }

    // Add DBC-node Paths
    vector<string> dbcdirs;
    if (has_any_dbc_mesh)
    {
        dbcdirs.push_back(string((dbc_nodes_str)) + "/");
    }

    vector<string> dbcname;
    if (has_any_pn_mesh)
    {
        dbcname.push_back("_" + string(dbc_nodes_str));
    }

    // Add the SVARS
    for ( vars_iter =  vars.begin();
            vars_iter != vars.end();
            vars_iter++ )
    {
        std::string  var_name;
        var_name = (*vars_iter).first;
        strcpy( vname, var_name.c_str() );

        //
        // Don't list the node position variable.
        //
        if (var_name == "nodpos")
        {
            continue;
        }

        char meshname[32];
        sprintf(meshname, "mesh%d",  vars[var_name].var_mesh_associations + 1);

        string fnmeshname;
        string nofnmeshname;
        string pnmeshname;

        fnmeshname   = string(meshname) + "_" + string(free_nodes_str);
        nofnmeshname = string(meshname) + "_" + string(no_free_nodes_str);
        pnmeshname   = string(meshname) + "_" + string(part_nodes_str);

        //
        // Determine if this is a tensor or a symmetric tensor.
        //

        if (vars[var_name].var_type == AVT_VECTOR_VAR && vars[var_name].var_dimension != dims)
        {
            if (dims == 3)
            {
                if (vars[var_name].var_dimension == 6)
                {
                    vars[var_name].var_type = AVT_SYMMETRIC_TENSOR_VAR;
                }
                else if (vars[var_name].var_dimension == 9)
                {
                    vars[var_name].var_type = AVT_TENSOR_VAR;
                }
                else
                {
                    continue;
                }
            }
            else if (dims == 2)
            {
                if (vars[var_name].var_dimension == 3)
                {
                    vars[var_name].var_type = AVT_SYMMETRIC_TENSOR_VAR;
                }
                else if (vars[var_name].var_dimension == 4)
                {
                    vars[var_name].var_type = AVT_TENSOR_VAR;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }

        string vname;
        string mvnStr;
        char multiVname[64];

        if (nmeshes == 1)
        {
            vname = vars[var_name].dir + var_name;
        }
        else
        {
            sprintf(multiVname, "%s%s-mesh%d", vars[var_name].dir.c_str(), var_name.c_str(),
                    vars[var_name].var_mesh_associations + 1);
            sprintf(multiVname, "%s-mesh%d", var_name.c_str(),
                    vars[var_name].var_mesh_associations + 1);
            mvnStr = multiVname;
            vname  = mvnStr;
        }

        bool do_fn_mesh = has_fn_mesh[vars[var_name].var_mesh_associations] &&
                          (vars[var_name].centering == AVT_NODECENT);
        bool do_nofn_mesh = has_fn_mesh[vars[var_name].var_mesh_associations];

        string fnvname = string(free_nodes_str) + "/" + vname;
        string nofnvname = string(no_free_nodes_str) + "/" + vname;

        bool do_pn_mesh = numSPH_classes>0 && mili_primal_fields[var_name].SPH_var;
        string pnvname = string(part_nodes_str) + "/" + vname;

        bool do_dbc_mesh = dbc_nodes_found;
        string dbcvname = string(dbc_nodes_str) + "/" + vname;

        string globalvname = string("Global") + "/" + vname;
        string matvname    = string("Material") + "/" + vname;

        switch ( vars[var_name].var_type )
        {
            case AVT_SCALAR_VAR:
                if ( vars[var_name].var_dimension == 1 )
                {
                    AddScalarVarToMetaData(md, vname, meshname,  vars[var_name].centering);

                    if (vars[var_name].global_var)
                    {
                        AddScalarVarToMetaData(md, globalvname, meshname,  vars[var_name].centering);
                    }
                    if (vars[var_name].mat_var)
                    {
                        AddScalarVarToMetaData(md, matvname, meshname,  vars[var_name].centering);
                    }

                    // Add to Free-node meshes vars
                    if (do_fn_mesh )
                    {
                        AddScalarVarToMetaData(md, fnvname, fnmeshname.c_str(), vars[var_name].centering);
                    }
                    if (do_nofn_mesh )
                    {
                        AddScalarVarToMetaData(md, nofnvname, nofnmeshname.c_str(), vars[var_name].centering);
                    }

                    // Add to Particle-node meshes vars
                    if (do_pn_mesh)
                    {
                        AddScalarVarToMetaData(md, pnvname, pnmeshname.c_str(), vars[var_name].centering);
                    }
                }

                break;

            case AVT_VECTOR_VAR:
                AddVectorVarToMetaData(md, vname, meshname, vars[var_name].centering, dims);

                if (vars[var_name].global_var)
                {
                    AddVectorVarToMetaData(md, globalvname, meshname,  vars[var_name].centering);
                }
                if (vars[var_name].mat_var)
                {
                    AddVectorVarToMetaData(md, matvname, meshname,  vars[var_name].centering);
                }

                // Add to Free-node meshes vars
                if (do_fn_mesh && !do_pn_mesh )
                {
                    AddVectorVarToMetaData(md, fnvname, fnmeshname.c_str(), vars[var_name].centering, dims);
                }
                if (do_nofn_mesh && !do_pn_mesh )
                {
                    AddVectorVarToMetaData(md, nofnvname, nofnmeshname.c_str(), vars[var_name].centering, dims);
                }

                // Add to Particle-node meshes vars
                if (do_pn_mesh)
                {
                    AddVectorVarToMetaData(md, pnvname, pnmeshname.c_str(), vars[var_name].centering, dims);
                }

                break;

            case AVT_SYMMETRIC_TENSOR_VAR:
                AddSymmetricTensorVarToMetaData(md, vname, meshname, vars[var_name].centering, dims);

                if (vars[var_name].global_var)
                {
                    AddSymmetricTensorVarToMetaData(md, globalvname, meshname,  vars[var_name].centering);
                }
                if (vars[var_name].mat_var)
                {
                    AddSymmetricTensorVarToMetaData(md, matvname, meshname,  vars[var_name].centering);
                }

                // Add to Free-node meshes vars
                if (do_fn_mesh && !do_pn_mesh )
                    AddSymmetricTensorVarToMetaData(md, fnvname, fnmeshname.c_str(),
                                                    vars[var_name].centering, dims);
                if (do_nofn_mesh && !do_pn_mesh )
                    AddSymmetricTensorVarToMetaData(md, nofnvname, nofnmeshname.c_str(),
                                                    vars[var_name].centering, dims);

                // Add to Particle-node meshes vars
                if (do_pn_mesh)
                    AddSymmetricTensorVarToMetaData(md, pnvname, pnmeshname.c_str(),
                                                    vars[var_name].centering, dims);
                break;

            case AVT_TENSOR_VAR:
                AddTensorVarToMetaData(md, vname, meshname, vars[var_name].centering, dims);

                if (vars[var_name].global_var)
                {
                    AddTensorVarToMetaData(md, globalvname, meshname,  vars[var_name].centering);
                }
                if (vars[var_name].mat_var)
                {
                    AddTensorVarToMetaData(md, matvname, meshname,  vars[var_name].centering);
                }

                // Add to Free-node meshes vars
                if (do_fn_mesh)
                {
                    AddTensorVarToMetaData(md, fnvname, fnmeshname.c_str(), vars[var_name].centering, dims);
                }
                if (do_nofn_mesh)
                {
                    AddTensorVarToMetaData(md, nofnvname, nofnmeshname.c_str(), vars[var_name].centering, dims);
                }

                // Add to Particle-node meshes vars
                if (do_pn_mesh)
                {
                    AddTensorVarToMetaData(md, pnvname, pnmeshname.c_str(), vars[var_name].centering, dims);
                }

                break;

            default:
                break;
        }

        //
        // Add vector variables with named components as scalars also
        //
        Expression vec_expr[100], ve1;
        int exp_index=0;
        State_variable sv;
        string compName, compIndex, vname_comp;
        char compIndex_char[64];
        std::vector<std::string> var_names;
        sv = svars[vname];

        if ( sv.vec_size>1 )
        {
            for ( j=0;
                    j<sv.vec_size;
                    j++ )
            {
                var_names.push_back(sv.components[j]);
            }
            AddArrayVarToMetaData(md, vname, var_names, meshname, vars[var_name].centering);
            if (do_fn_mesh)
            {
                vname_comp = fnvname + "/" + compName;
                AddArrayVarToMetaData(md, vname, var_names, fnmeshname.c_str(), vars[var_name].centering);
            }
            if (do_nofn_mesh)
            {
                vname_comp = nofnvname + "/" + compName;
                AddArrayVarToMetaData(md, vname, var_names, nofnmeshname.c_str(), vars[var_name].centering);
            }

            for ( j=0;
                    j<sv.vec_size;
                    j++ )
            {
                Expression primal_expr;

                // First add the Variable to the MetaData
                compName = string( sv.components[j] );
                sprintf( compIndex_char,"[%d]", j );
                compIndex = string( compIndex_char );
                vname_comp = vname + "/" + compName;

                primal_expr.SetName("Mili-Primal/"+vname_comp);
                primal_expr.SetDefinition("<"+vname+">"+compIndex);
                primal_expr.SetType(Expression::ScalarMeshVar);
                primal_expr.SetHidden(false);

                std::stringstream definition;
                definition << "array_decompose(" << vname << "," << i << ")";
                primal_expr.SetDefinition(definition.str());
                primal_expr.SetType(Expression::ScalarMeshVar);

                md->AddExpression(&primal_expr);
            }
        }
    }

    //
    // By calling OpenDB for domain 0, it will populate the times.
    //
    OpenDB(0, false);

    bool do_fn_mesh = has_fn_mesh[vars[string("stress")].var_mesh_associations];

    if (do_fn_mesh || nofndirs[0]=="" )
        TRY
    {
        // This call throw an exception if stress does not exist.
        GetVariableIndex("stress");

        for (i = 0; i < nofndirs.size(); i++)
        {
            Expression pressure_expr;
            pressure_expr.SetName(MiliDerivedName+nofndirs[i]+"pressure");
            pressure_expr.SetDefinition("-trace(<"+nofndirs[i]+"stress>)/3");
            pressure_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&pressure_expr);

            Expression stressx_expr;
            stressx_expr.SetName(MiliDerivedName+nofndirs[i]+"stress/x");
            stressx_expr.SetDefinition("<"+nofndirs[i]+"stress>[0][0]");
            stressx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&stressx_expr);

            Expression stressy_expr;
            stressy_expr.SetName(MiliDerivedName+nofndirs[i]+"stress/y");
            stressy_expr.SetDefinition("<"+nofndirs[i]+"stress>[1][1]");
            stressy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&stressy_expr);

            Expression stressz_expr;
            stressz_expr.SetName(MiliDerivedName+nofndirs[i]+"stress/z");
            stressz_expr.SetDefinition("<"+nofndirs[i]+"stress>[2][2]");
            stressz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&stressz_expr);

            Expression stressxy_expr;
            stressxy_expr.SetName(MiliDerivedName+nofndirs[i]+"stress/xy");
            stressxy_expr.SetDefinition("<"+nofndirs[i]+"stress>[0][1]");
            stressxy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&stressxy_expr);

            Expression stressxz_expr;
            stressxz_expr.SetName(MiliDerivedName+nofndirs[i]+"stress/xz");
            stressxz_expr.SetDefinition("<"+nofndirs[i]+"stress>[0][2]");
            stressxz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&stressxz_expr);

            Expression stressyz_expr;
            stressyz_expr.SetName(MiliDerivedName+nofndirs[i]+"stress/yz");
            stressyz_expr.SetDefinition("<"+nofndirs[i]+"stress>[1][2]");
            stressyz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&stressyz_expr);

            Expression seff_expr;
            seff_expr.SetName(MiliDerivedName+nofndirs[i]+"eff_stress");
            seff_expr.SetDefinition("effective_tensor(<"+nofndirs[i]+"stress>)");
            seff_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&seff_expr);

            Expression p_dev_stress1_expr;
            p_dev_stress1_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_dev_stress/1");
            p_dev_stress1_expr.SetDefinition
            ("principal_deviatoric_tensor(<"+nofndirs[i]+"stress>)[0]");
            p_dev_stress1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_stress1_expr);

            Expression p_dev_stress2_expr;
            p_dev_stress2_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_dev_stress/2");
            p_dev_stress2_expr.SetDefinition
            ("principal_deviatoric_tensor(<"+nofndirs[i]+"stress>)[1]");
            p_dev_stress2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_stress2_expr);

            Expression p_dev_stress3_expr;
            p_dev_stress3_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_dev_stress/3");
            p_dev_stress3_expr.SetDefinition
            ("principal_deviatoric_tensor(<"+nofndirs[i]+"stress>)[2]");
            p_dev_stress3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_stress3_expr);

            Expression maxshr_expr;
            maxshr_expr.SetName(MiliDerivedName+nofndirs[i]+"max_shear_stress");
            maxshr_expr.SetDefinition
            ("tensor_maximum_shear(<"+nofndirs[i]+"stress>)");
            maxshr_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&maxshr_expr);

            Expression prin_stress1_expr;
            prin_stress1_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_stress/1");
            prin_stress1_expr.SetDefinition
            ("principal_tensor(<"+nofndirs[i]+"stress>)[0]");
            prin_stress1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_stress1_expr);

            Expression prin_stress2_expr;
            prin_stress2_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_stress/2");
            prin_stress2_expr.SetDefinition
            ("principal_tensor(<"+nofndirs[i]+"stress>)[1]");
            prin_stress2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_stress2_expr);

            Expression prin_stress3_expr;
            prin_stress3_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_stress/3");
            prin_stress3_expr.SetDefinition
            ("principal_tensor(<"+nofndirs[i]+"stress>)[2]");
            prin_stress3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_stress3_expr);
        }
    }
    CATCH(InvalidVariableException)
    {
        ErrorHandler(__FUNCTION__, __LINE__);
    }
    ENDTRY

    if (do_fn_mesh || nofndirs[0]=="")
        TRY
    {
        // This call throw an exception if strain does not exist.
        GetVariableIndex("strain");

        for (i = 0; i < nofndirs.size(); i++)
        {
            Expression strainx_expr;
            strainx_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/x");
            strainx_expr.SetDefinition("<"+nofndirs[i]+"strain>[0][0]");
            strainx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainx_expr);

            Expression strainy_expr;
            strainy_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/y");
            strainy_expr.SetDefinition("<"+nofndirs[i]+"strain>[1][1]");
            strainy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainy_expr);

            Expression strainz_expr;
            strainz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/z");
            strainz_expr.SetDefinition("<"+nofndirs[i]+"strain>[2][2]");
            strainz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainz_expr);

            Expression strainxy_expr;
            strainxy_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/xy");
            strainxy_expr.SetDefinition("<"+nofndirs[i]+"strain>[0][1]");
            strainxy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainxy_expr);

            Expression strainxz_expr;
            strainxz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/xz");
            strainxz_expr.SetDefinition("<"+nofndirs[i]+"strain>[0][2]");
            strainxz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainxz_expr);

            Expression strainyz_expr;
            strainyz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/yz");
            strainyz_expr.SetDefinition("<"+nofndirs[i]+"strain>[1][2]");
            strainyz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainyz_expr);

            Expression seff_expr;
            seff_expr.SetName(MiliDerivedName+nofndirs[i]+"eff_strain");
            seff_expr.SetDefinition("effective_tensor(<"+nofndirs[i]+"strain>)");
            seff_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&seff_expr);

            Expression p_dev_strain1_expr;
            p_dev_strain1_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_dev_strain/1");
            p_dev_strain1_expr.SetDefinition
            ("principal_deviatoric_tensor(<"+nofndirs[i]+"strain>)[0]");
            p_dev_strain1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_strain1_expr);

            Expression p_dev_strain2_expr;
            p_dev_strain2_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_dev_strain/2");
            p_dev_strain2_expr.SetDefinition
            ("principal_deviatoric_tensor(<"+nofndirs[i]+"strain>)[1]");
            p_dev_strain2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_strain2_expr);

            Expression p_dev_strain3_expr;
            p_dev_strain3_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_dev_strain/3");
            p_dev_strain3_expr.SetDefinition
            ("principal_deviatoric_tensor(<"+nofndirs[i]+"strain>)[2]");
            p_dev_strain3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_strain3_expr);

            Expression maxshr_expr;
            maxshr_expr.SetName(MiliDerivedName+nofndirs[i]+"max_shear_strain");
            maxshr_expr.SetDefinition
            ("tensor_maximum_shear(<"+nofndirs[i]+"strain>)");
            maxshr_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&maxshr_expr);

            Expression prin_strain1_expr;
            prin_strain1_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_strain/1");
            prin_strain1_expr.SetDefinition
            ("principal_tensor(<"+nofndirs[i]+"strain>)[0]");
            prin_strain1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_strain1_expr);

            Expression prin_strain2_expr;
            prin_strain2_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_strain/2");
            prin_strain2_expr.SetDefinition
            ("principal_tensor(<"+nofndirs[i]+"strain>)[1]");
            prin_strain2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_strain2_expr);

            Expression prin_strain3_expr;
            prin_strain3_expr.SetName(MiliDerivedName+nofndirs[i]+"prin_strain/3");
            prin_strain3_expr.SetDefinition
            ("principal_tensor(<"+nofndirs[i]+"strain>)[2]");
            prin_strain3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_strain3_expr);
        }
    }
    CATCH(InvalidVariableException)
    {
        // If strain not given compute the strain at nodes given
        // the current geometry and the initial configuration.

        for (i = 0; i < nofndirs.size(); i++)
        {
            string tmpmeshname = "mesh1";
            if (nofndirs[i] != "")
            {
                tmpmeshname += "_"+string(no_free_nodes_str);
            }
            string tmpvarname = MiliDerivedName+nofndirs[i]+"strain/initial_strain_coords";
            string tmpvelname = nofndirs[i]+"nodvel";
            Expression initial_coords_expr;
            initial_coords_expr.SetName(tmpvarname);
            initial_coords_expr.SetDefinition
            ("conn_cmfe(coord(<[0]i:"+tmpmeshname+">),"+tmpmeshname+")");
            initial_coords_expr.SetType(Expression::VectorMeshVar);
            initial_coords_expr.SetHidden(true);
            md->AddExpression(&initial_coords_expr);

            Expression strain_green_expr;
            strain_green_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/green_lagrange");
            strain_green_expr.SetDefinition(
                "strain_green_lagrange("+tmpmeshname+",<"+tmpvarname+">)");
            strain_green_expr.SetType(Expression::TensorMeshVar);
            md->AddExpression(&strain_green_expr);

            Expression strain_infinitesimal_expr;
            strain_infinitesimal_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal");
            strain_infinitesimal_expr.SetDefinition(
                "strain_infinitesimal("+tmpmeshname+",<"+tmpvarname+">)");
            strain_infinitesimal_expr.SetType(Expression::TensorMeshVar);
            md->AddExpression(&strain_infinitesimal_expr);

            Expression strain_almansi_expr;
            strain_almansi_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/almansi");
            strain_almansi_expr.SetDefinition(
                "strain_almansi("+tmpmeshname+",<"+tmpvarname+">)");
            strain_almansi_expr.SetType(Expression::TensorMeshVar);
            md->AddExpression(&strain_almansi_expr);

            Expression strain_rate_expr;
            strain_rate_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/rate");
            strain_rate_expr.SetDefinition(
                "strain_rate("+tmpmeshname+",<"+tmpvelname+">)");
            strain_rate_expr.SetType(Expression::TensorMeshVar);
            md->AddExpression(&strain_rate_expr);

// green_lagrange strain
            Expression straingx_expr;
            straingx_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/x");
            straingx_expr.SetDefinition
            ( "<"+MiliDerivedName+nofndirs[i]+"strain/green_lagrange>[0][0]");
            straingx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&straingx_expr);

            Expression straingy_expr;
            straingy_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/y");
            straingy_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/green_lagrange>[1][1]");
            straingy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&straingy_expr);

            Expression straingz_expr;
            straingz_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/z");
            straingz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/green_lagrange>[2][2]");
            straingz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&straingz_expr);

            Expression straingxy_expr;
            straingxy_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/xy");
            straingxy_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/green_lagrange>[0][1]");
            straingxy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&straingxy_expr);

            Expression straingxz_expr;
            straingxz_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/xz");
            straingxz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/green_lagrange>[0][2]");
            straingxz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&straingxz_expr);

            Expression straingyz_expr;
            straingyz_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/yz");
            straingyz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/green_lagrange>[1][2]");
            straingyz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&straingyz_expr);

            Expression sgeff_expr;
            sgeff_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/eff_strain");
            sgeff_expr.SetDefinition
            ("effective_tensor(<MiliDerived/"+nofndirs[i]+"strain/green_lagrange>)");
            sgeff_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&sgeff_expr);

            Expression p_dev_straing1_expr;
            p_dev_straing1_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/prin_dev_strain/1");
            p_dev_straing1_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/green_lagrange>)[0]");
            p_dev_straing1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straing1_expr);

            Expression p_dev_straing2_expr;
            p_dev_straing2_expr.SetName
            (MiliDerivedName+nofndirs[i]+
             "strain/green_lagrange_strain/prin_dev_strain/2");
            p_dev_straing2_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/green_lagrange>)[1]");
            p_dev_straing2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straing2_expr);

            Expression p_dev_straing3_expr;
            p_dev_straing3_expr.SetName
            (MiliDerivedName+nofndirs[i]+
             "strain/green_lagrange_strain/prin_dev_strain/3");
            p_dev_straing3_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/green_lagrange>)[2]");
            p_dev_straing3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straing3_expr);

            Expression maxshrg_expr;
            maxshrg_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/max_shear_strain");
            maxshrg_expr.SetDefinition
            ("tensor_maximum_shear(<MiliDerived/"+nofndirs[i]+"strain/green_lagrange>)");
            maxshrg_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&maxshrg_expr);

            Expression prin_straing1_expr;
            prin_straing1_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/prin_strain/1");
            prin_straing1_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/green_lagrange>)[0]");
            prin_straing1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straing1_expr);

            Expression prin_straing2_expr;
            prin_straing2_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/prin_strain/2");
            prin_straing2_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/green_lagrange>)[1]");
            prin_straing2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straing2_expr);

            Expression prin_straing3_expr;
            prin_straing3_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/green_lagrange_strain/prin_strain/3");
            prin_straing3_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/green_lagrange>)[2]");
            prin_straing3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straing3_expr);

// infinitesimal strain
            Expression strainix_expr;
            strainix_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/x");
            strainix_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/infinitesimal>[0][0]");
            strainix_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainix_expr);

            Expression strainiy_expr;
            strainiy_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/y");
            strainiy_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/infinitesimal>[1][1]");
            strainiy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainiy_expr);

            Expression strainiz_expr;
            strainiz_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/z");
            strainiz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/infinitesimal>[2][2]");
            strainiz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainiz_expr);

            Expression strainixy_expr;
            strainixy_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/xy");
            strainixy_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/infinitesimal>[0][1]");
            strainixy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainixy_expr);

            Expression strainixz_expr;
            strainixz_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/xz");
            strainixz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/infinitesimal>[0][2]");
            strainixz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainixz_expr);

            Expression strainiyz_expr;
            strainiyz_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/yz");
            strainiyz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/infinitesimal>[1][2]");
            strainiyz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainiyz_expr);

            Expression sieff_expr;
            sieff_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/eff_strain");
            sieff_expr.SetDefinition
            ("effective_tensor(<MiliDerived/"+nofndirs[i]+"strain/infinitesimal>)");
            sieff_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&sieff_expr);

            Expression p_dev_straini1_expr;
            p_dev_straini1_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/prin_dev_strain/1");
            p_dev_straini1_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/infinitesimal>)[0]");
            p_dev_straini1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straini1_expr);

            Expression p_dev_straini2_expr;
            p_dev_straini2_expr.SetName
            (MiliDerivedName+nofndirs[i]+
             "strain/infinitesimal_strain/prin_dev_strain/2");
            p_dev_straini2_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/infinitesimal>)[1]");
            p_dev_straini2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straini2_expr);

            Expression p_dev_straini3_expr;
            p_dev_straini3_expr.SetName
            (MiliDerivedName+nofndirs[i]+
             "strain/infinitesimal_strain/prin_dev_strain/3");
            p_dev_straini3_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/infinitesimal>)[2]");
            p_dev_straini3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straini3_expr);

            Expression maxshri_expr;
            maxshri_expr.SetName
            (MiliDerivedName+nofndirs[i]+
             "strain/infinitesimal_strain/max_shear_strain");
            maxshri_expr.SetDefinition
            ("tensor_maximum_shear(<MiliDerived/"+nofndirs[i]+
             "strain/infinitesimal>)");
            maxshri_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&maxshri_expr);

            Expression prin_straini1_expr;
            prin_straini1_expr.SetName
            (MiliDerivedName+nofndirs[i]+
             "strain/infinitesimal_strain/prin_strain/1");
            prin_straini1_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/infinitesimal>)[0]");
            prin_straini1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straini1_expr);

            Expression prin_straini2_expr;
            prin_straini2_expr.SetName
            (MiliDerivedName+nofndirs[i]+
             "strain/infinitesimal_strain/prin_strain/2");
            prin_straini2_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/infinitesimal>)[1]");
            prin_straini2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straini2_expr);

            Expression prin_straini3_expr;
            prin_straini3_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/infinitesimal_strain/prin_strain/3");
            prin_straini3_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/infinitesimal>)[2]");
            prin_straini3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straini3_expr);

// almansi strain
            Expression strainax_expr;
            strainax_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/almansi_strain/x");
            strainax_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/almansi>[0][0]");
            strainax_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainax_expr);

            Expression strainay_expr;
            strainay_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/y");
            strainay_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/almansi>[1][1]");
            strainay_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainay_expr);

            Expression strainaz_expr;
            strainaz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/almansi_strain/z");
            strainaz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/almansi>[2][2]");
            strainaz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainaz_expr);

            Expression strainaxy_expr;
            strainaxy_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/almansi_strain/xy");
            strainaxy_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/almansi>[0][1]");
            strainaxy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainaxy_expr);

            Expression strainaxz_expr;
            strainaxz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/almansi_strain/xz");
            strainaxz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/almansi>[0][2]");
            strainaxz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainaxz_expr);

            Expression strainayz_expr;
            strainayz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/almansi_strain/yz");
            strainayz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"strain/almansi>[1][2]");
            strainayz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainayz_expr);

            Expression saeff_expr;
            saeff_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/eff_strain");
            saeff_expr.SetDefinition
            ("effective_tensor(<MiliDerived/"+nofndirs[i]+"strain/almansi>)");
            saeff_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&saeff_expr);

            Expression p_dev_straina1_expr;
            p_dev_straina1_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/prin_dev_strain/1");
            p_dev_straina1_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/almansi>)[0]");
            p_dev_straina1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straina1_expr);

            Expression p_dev_straina2_expr;
            p_dev_straina2_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/prin_dev_strain/2");
            p_dev_straina2_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/almansi>)[1]");
            p_dev_straina2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straina2_expr);

            Expression p_dev_straina3_expr;
            p_dev_straina3_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/prin_dev_strain/3");
            p_dev_straina3_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+
             "strain/almansi>)[2]");
            p_dev_straina3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_straina3_expr);

            Expression maxshra_expr;
            maxshra_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/max_shear_strain");
            maxshra_expr.SetDefinition
            ("tensor_maximum_shear(<MiliDerived/"+nofndirs[i]+"strain/almansi>)");
            maxshra_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&maxshra_expr);

            Expression prin_straina1_expr;
            prin_straina1_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/prin_strain/1");
            prin_straina1_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/almansi>)[0]");
            prin_straina1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straina1_expr);

            Expression prin_straina2_expr;
            prin_straina2_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/prin_strain/2");
            prin_straina2_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/almansi>)[1]");
            prin_straina2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straina2_expr);

            Expression prin_straina3_expr;
            prin_straina3_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/almansi_strain/prin_strain/3");
            prin_straina3_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/almansi>)[2]");
            prin_straina3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_straina3_expr);

// Rate strain
            Expression strainrx_expr;
            strainrx_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/rate_strain/x");
            strainrx_expr.SetDefinition("<"+MiliDerivedName+nofndirs[i]+"strain/rate>[0][0]");
            strainrx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainrx_expr);

            Expression strainry_expr;
            strainry_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/rate_strain/y");
            strainry_expr.SetDefinition("<"+MiliDerivedName+nofndirs[i]+"strain/rate>[1][1]");
            strainry_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainry_expr);

            Expression strainrz_expr;
            strainrz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/rate_strain/z");
            strainrz_expr.SetDefinition("<"+MiliDerivedName+nofndirs[i]+"strain/rate>[2][2]");
            strainrz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainrz_expr);

            Expression strainrxy_expr;
            strainrxy_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/rate_strain/xy");
            strainrxy_expr.SetDefinition("<"+MiliDerivedName+nofndirs[i]+"strain/rate>[0][1]");
            strainrxy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainrxy_expr);

            Expression strainrxz_expr;
            strainrxz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/rate_strain/xz");
            strainrxz_expr.SetDefinition("<"+MiliDerivedName+nofndirs[i]+"strain/rate>[0][2]");
            strainrxz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainrxz_expr);

            Expression strainryz_expr;
            strainryz_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/rate_strain/yz");
            strainryz_expr.SetDefinition("<"+MiliDerivedName+nofndirs[i]+"strain/rate>[1][2]");
            strainryz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&strainryz_expr);

            Expression sreff_expr;
            sreff_expr.SetName(MiliDerivedName+nofndirs[i]+"strain/rate_strain/eff_strain");
            sreff_expr.SetDefinition
            ("effective_tensor(<MiliDerived/"+nofndirs[i]+"strain/rate>)");
            sreff_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&sreff_expr);

            Expression p_dev_strainr1_expr;
            p_dev_strainr1_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/rate_strain/prin_dev_strain/1");
            p_dev_strainr1_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+"strain/rate>)[0]");
            p_dev_strainr1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_strainr1_expr);

            Expression p_dev_strainr2_expr;
            p_dev_strainr2_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/rate_strain/prin_dev_strain/2");
            p_dev_strainr2_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+"strain/rate>)[1]");
            p_dev_strainr2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_strainr2_expr);

            Expression p_dev_strainr3_expr;
            p_dev_strainr3_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/rate_strain/prin_dev_strain/3");
            p_dev_strainr3_expr.SetDefinition
            ("principal_deviatoric_tensor(<MiliDerived/"+nofndirs[i]+"strain/rate>)[2]");
            p_dev_strainr3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&p_dev_strainr3_expr);

            Expression maxshrr_expr;
            maxshrr_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/rate_strain/max_shear_strain");
            maxshrr_expr.SetDefinition
            ("tensor_maximum_shear(<MiliDerived/"+nofndirs[i]+"strain/rate>)");
            maxshrr_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&maxshrr_expr);

            Expression prin_strainr1_expr;
            prin_strainr1_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/rate_strain/prin_strain/1");
            prin_strainr1_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/rate>)[0]");
            prin_strainr1_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_strainr1_expr);

            Expression prin_strainr2_expr;
            prin_strainr2_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/rate_strain/prin_strain/2");
            prin_strainr2_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/rate>)[1]");
            prin_strainr2_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_strainr2_expr);

            Expression prin_strainr3_expr;
            prin_strainr3_expr.SetName
            (MiliDerivedName+nofndirs[i]+"strain/rate_strain/prin_strain/3");
            prin_strainr3_expr.SetDefinition
            ("principal_tensor(<MiliDerived/"+nofndirs[i]+"strain/rate>)[2]");
            prin_strainr3_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&prin_strainr3_expr);
        }
    }
    ENDTRY

    TRY
    {
        // This call throw an exception if nodvel does not exist.
        GetVariableIndex("nodvel");

        for (i = 0; i < fndirs.size(); i++)
        {
            Expression velx_expr;
            velx_expr.SetName(MiliDerivedName+fndirs[i]+"velocity/x");
            velx_expr.SetDefinition("<"+fndirs[i]+"nodvel>[0]");
            velx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&velx_expr);

            Expression vely_expr;
            vely_expr.SetName(MiliDerivedName+fndirs[i]+"velocity/y");
            vely_expr.SetDefinition("<"+fndirs[i]+"nodvel>[1]");
            vely_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&vely_expr);

            Expression velz_expr;
            velz_expr.SetName(MiliDerivedName+fndirs[i]+"velocity/z");
            velz_expr.SetDefinition("<"+fndirs[i]+"nodvel>[2]");
            velz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&velz_expr);

            Expression velmag_expr;
            velmag_expr.SetName(MiliDerivedName+fndirs[i]+"velocity/mag");
            velmag_expr.SetDefinition("magnitude(<"+fndirs[i]+"nodvel>)");
            velmag_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&velmag_expr);
        }
    }
    CATCH(InvalidVariableException)
    {
        ErrorHandler(__FUNCTION__, __LINE__);
    }
    ENDTRY

    TRY
    {
        // This call throw an exception if nodacc does not exist.
        GetVariableIndex("nodacc");

        for (i = 0; i < fndirs.size(); i++)
        {
            Expression accx_expr;
            accx_expr.SetName(MiliDerivedName+fndirs[i]+"acceleration/x");
            accx_expr.SetDefinition("<"+fndirs[i]+"nodacc>[0]");
            accx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&accx_expr);

            Expression accy_expr;
            accy_expr.SetName(MiliDerivedName+fndirs[i]+"acceleration/y");
            accy_expr.SetDefinition("<"+fndirs[i]+"nodacc>[1]");
            accy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&accy_expr);

            Expression accz_expr;
            accz_expr.SetName(MiliDerivedName+fndirs[i]+"acceleration/z");
            accz_expr.SetDefinition("<"+fndirs[i]+"nodacc>[2]");
            accz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&accz_expr);

            Expression accmag_expr;
            accmag_expr.SetName(MiliDerivedName+fndirs[i]+"acceleration/mag");
            accmag_expr.SetDefinition("magnitude(<"+fndirs[i]+"nodacc>)");
            accmag_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&accmag_expr);
        }
    }
    CATCH(InvalidVariableException)
    {
        ErrorHandler(__FUNCTION__, __LINE__);
    }
    ENDTRY

    TRY
    {
        // This call throw an exception if noddisp does not exist.
        GetVariableIndex("noddisp");

        for (i = 0; i < fndirs.size(); i++)
        {
            Expression dispx_expr;
            dispx_expr.SetName(MiliDerivedName+fndirs[i]+"displacement/x");
            dispx_expr.SetDefinition("<"+fndirs[i]+"noddisp>[0]");
            dispx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&dispx_expr);

            Expression dispy_expr;
            dispy_expr.SetName(MiliDerivedName+fndirs[i]+"displacement/y");
            dispy_expr.SetDefinition("<"+fndirs[i]+"noddisp>[1]");
            dispy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&dispy_expr);

            Expression dispz_expr;
            dispz_expr.SetName(MiliDerivedName+fndirs[i]+"displacement/z");
            dispz_expr.SetDefinition("<"+fndirs[i]+"noddisp>[2]");
            dispz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&dispz_expr);

            Expression dispmag_expr;
            dispmag_expr.SetName(MiliDerivedName+fndirs[i]+"displacement/mag");
            dispmag_expr.SetDefinition("magnitude(<"+fndirs[i]+"noddisp>)");
            dispmag_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&dispmag_expr);
        }
    }
    CATCH(InvalidVariableException)
    {
        for (i = 0; i < fndirs.size(); i++)
        {
            string tmpmeshname = "mesh1";
            if (nofndirs[i] != "")
            {
                tmpmeshname += "_"+string(no_free_nodes_str);
            }
            string tmpvarname =
                MiliDerivedName+nofndirs[i]+"displacement/initial_disp_coords";

            Expression initial_disp_coords;
            initial_disp_coords.SetName(tmpvarname);
            initial_disp_coords.SetDefinition
            ("conn_cmfe(coord(<[0]i:"+tmpmeshname+">),"+tmpmeshname+")");
            initial_disp_coords.SetType(Expression::VectorMeshVar);
            initial_disp_coords.SetHidden(true);
            md->AddExpression(&initial_disp_coords);

            Expression noddisp;
            noddisp.SetName(MiliDerivedName+nofndirs[i]+"displacement/vec");
            noddisp.SetDefinition(
                "displacement("+tmpmeshname+",<"+tmpvarname+">)");
            noddisp.SetType(Expression::VectorMeshVar);
            noddisp.SetHidden(true);
            md->AddExpression(&noddisp);

            Expression dispx_expr;
            dispx_expr.SetName(MiliDerivedName+nofndirs[i]+"displacement/x");
            dispx_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"displacement/vec>[0]");
            dispx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&dispx_expr);

            Expression dispy_expr;
            dispy_expr.SetName(MiliDerivedName+nofndirs[i]+"displacement/y");
            dispy_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"displacement/vec>[1]");
            dispy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&dispy_expr);

            Expression dispz_expr;
            dispz_expr.SetName(MiliDerivedName+nofndirs[i]+"displacement/z");
            dispz_expr.SetDefinition
            ("<"+MiliDerivedName+nofndirs[i]+"displacement/vec>[2]");
            dispz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&dispz_expr);

            Expression dispmag_expr;
            dispmag_expr.SetName(MiliDerivedName+nofndirs[i]+"displacement/mag");
            dispmag_expr.SetDefinition
            ("magnitude(<MiliDerived/"+nofndirs[i]+"displacement/vec>)");
            dispmag_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&dispmag_expr);
        }
    }
    ENDTRY

    if (nmeshes == 1)
    {
        for (i = 0; i < fndirs.size(); i++)
        {
            Expression posx_expr;
            posx_expr.SetName(MiliDerivedName+fndirs[i]+"nodpos/x");
            posx_expr.SetDefinition("coord(mesh1"+fnname[i]+")[0]");
            posx_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&posx_expr);

            Expression posy_expr;
            posy_expr.SetName(MiliDerivedName+fndirs[i]+"nodpos/y");
            posy_expr.SetDefinition("coord(mesh1"+fnname[i]+")[1]");
            posy_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&posy_expr);

            Expression posz_expr;
            posz_expr.SetName(MiliDerivedName+fndirs[i]+"nodpos/z");
            posz_expr.SetDefinition("coord(mesh1"+fnname[i]+")[2]");
            posz_expr.SetType(Expression::ScalarMeshVar);
            md->AddExpression(&posz_expr);
        }

        if ( dbc_nodes_found )
            for (i = 0; i < pndirs.size(); i++)
            {
                Expression posx_expr;
                posx_expr.SetName(MiliDerivedName+dbcdirs[i]+"nodpos/x");
                posx_expr.SetDefinition("coord(mesh1"+dbcname[i]+")[0]");
                posx_expr.SetType(Expression::ScalarMeshVar);
                md->AddExpression(&posx_expr);

                Expression posy_expr;
                posy_expr.SetName(MiliDerivedName+dbcdirs[i]+"nodpos/y");
                posy_expr.SetDefinition("coord(mesh1"+dbcname[i]+")[1]");
                posy_expr.SetType(Expression::ScalarMeshVar);
                md->AddExpression(&posy_expr);

                Expression posz_expr;
                posz_expr.SetName(MiliDerivedName+dbcdirs[i]+"nodpos/z");
                posz_expr.SetDefinition("coord(mesh1"+dbcname[i]+")[2]");
                posz_expr.SetType(Expression::ScalarMeshVar);
                md->AddExpression(&posz_expr);
            }
    }
    else
    {
        for (i = 0; i < nmeshes; ++i)
        {
            for (int j = 0; j < fndirs.size(); j++)
            {
                char meshname[32];
                char expr_name[128];
                char defn_name[128];
                sprintf(meshname, "mesh%d", i + 1);

                Expression posx_expr;
                sprintf(expr_name, "MiliDerived/%snodpos/%s/x", fndirs[i].c_str(), meshname);
                sprintf(defn_name, "coord(%s%s)[0]", meshname, fnname[j].c_str());
                posx_expr.SetName(expr_name);
                posx_expr.SetDefinition(defn_name);
                posx_expr.SetType(Expression::ScalarMeshVar);
                md->AddExpression(&posx_expr);

                sprintf(expr_name, "MiliDerived/%snodpos/%s/y", fndirs[i].c_str(), meshname);
                sprintf(defn_name, "coord(%s%s)[1]", meshname, fnname[j].c_str());
                Expression posy_expr;
                posy_expr.SetName(expr_name);
                posy_expr.SetDefinition(defn_name);
                posy_expr.SetType(Expression::ScalarMeshVar);
                md->AddExpression(&posy_expr);

                sprintf(expr_name, "MiliDerived/%snodpos/%s/z", fndirs[i].c_str(), meshname);
                sprintf(defn_name, "coord(%s%s)[2]", meshname, fnname[j].c_str());
                Expression posz_expr;
                posz_expr.SetName(expr_name);
                posz_expr.SetDefinition(defn_name);
                posz_expr.SetType(Expression::ScalarMeshVar);
                md->AddExpression(&posz_expr);
            }
        }
    }

    if (!readPartInfo && !avtDatabase::OnlyServeUpMetaData())
    {
        ParseDynaPart();
    }
}

// ****************************************************************************
//  Method: avtMiliFileFormat::GetAuxiliaryData
//
//  Purpose:
//      Gets the auxiliary data specified.
//
//  Arguments:
//      var        The variable of interest.
//      ts         The timestep of interest.
//      dom        The domain of interest.
//      type       The type of auxiliary data.
//      <unnamed>  The arguments for that type -- not used.
//      df         Destructor function.
//
//  Returns:    The auxiliary data.
//
//  Programmer: Akira Haddox
//  Creation:   May 23, 2003
//
//  Modifications
//    Akira Haddox, Fri May 23 08:13:09 PDT 2003
//    Added in support for multiple meshes. Changed for MTMD.
//
// ****************************************************************************

void *
avtMiliFileFormat::GetAuxiliaryData(const char *var, int ts, int dom,
                                    const char * type, void *,
                                    DestructorFunction &df)
{
    bool good_type=false;
    int i=0;

    if ( !strcmp(type, AUXILIARY_DATA_MATERIAL) ||
            !strcmp(type, AUXILIARY_DATA_GLOBAL_NODE_IDS) ||
            !strcmp(type, AUXILIARY_DATA_GLOBAL_ZONE_IDS) )
    {
        good_type = true;
    }

    if ( !good_type )
    {
        return NULL;
    }

    if (!readMesh[dom] && !strcmp(type, AUXILIARY_DATA_MATERIAL))
    {
        ReadMesh(dom);
    }

    //
    // The valid variables are meshX, where X is an int > 0.
    // We need to verify the name, and get the meshId.
    //
    if (strstr(var, "materials") == var)
    {

        //
        // Do a checked conversion to integer.
        //
        char *check;
        int mesh_id = (int) strtol(var + strlen("materials"), &check, 10);
        if (check != NULL && check[0] != '\0' && !strstr(check, no_free_nodes_str))
        {
            ErrorHandler(__FUNCTION__, __LINE__);
            EXCEPTION1(InvalidVariableException, var)
        }
        --mesh_id;

        avtMaterial *myCopy = materials[dom][mesh_id];
        avtMaterial *mat = new avtMaterial(myCopy->GetNMaterials(),
                                           myCopy->GetMaterials(),
                                           myCopy->GetNZones(),
                                           myCopy->GetMatlist(),
                                           myCopy->GetMixlen(),
                                           myCopy->GetMixMat(),
                                           myCopy->GetMixNext(),
                                           myCopy->GetMixZone(),
                                           myCopy->GetMixVF());
        df = avtMaterial::Destruct;
        return (void*) mat;
    }

    return NULL;

    vtkDataSet *rv = NULL;
    if (strcmp(type, AUXILIARY_DATA_GLOBAL_NODE_IDS) == 0)
    {
        vtkIntArray *nodeLabels = vtkIntArray::New();
        nodeLabels->SetArray((int*) mili_node_class["node"].node_data[dom].labels,
                             mili_node_class["node"].node_data[dom].qty, 0);
        df = avtVariableCache::DestructVTKObject;
        return nodeLabels;
    }
    else if (strcmp(type, AUXILIARY_DATA_GLOBAL_ZONE_IDS) == 0)
    {
        vtkIntArray *zoneLabels = vtkIntArray::New();
        if ( temp_zone_ids!=NULL )
        {
            free (temp_zone_ids);
        }
        temp_zone_ids = (int *) malloc( ncells[dom][0]*sizeof(int) );
        for ( i=0;
                i<ncells[dom][0];
                i++ )
        {
            temp_zone_ids[i] = (1000*(dom+1)) + i;
        }

        zoneLabels->SetArray((int*) temp_zone_ids,
                             ncells[dom][0], 0);
        return zoneLabels;
    }
    ErrorHandler(__FUNCTION__, __LINE__);
    EXCEPTION1(InvalidVariableException, var);
}


// ****************************************************************************
//  Method: avtMiliFileFormat::FreeUpResources
//
//  Purpose:
//      Close databases and free up non-essential memory.
//
//  Programmer: Akira Haddox
//  Creation:   July 23, 2003
//
//  Modifications:
//
//    Hank Childs, Tue Jul 27 10:18:26 PDT 2004
//    Moved the code to free up resources to the destructor.
// ****************************************************************************

void
avtMiliFileFormat::FreeUpResources()
{
}


// ****************************************************************************
//  Method: avtMiliFileFormat::ParseDynaPart
//
//  Purpose:
//    Read though a DynaPart output file to gather information about
//    shared nodes for generating ghostzones.
//
//  Programmer: Akira Haddox
//  Creation:   August 6, 2003
//
// ****************************************************************************

#define READ_THROUGH_NEXT_COMMENT(_in) while(_in.get() != '#') ;\
                                       _in.getline(buf, 1024)
#define READ_VECTOR(_in, _dest) \
    for (_macro_i = 0; _macro_i < _dest.size(); ++_macro_i) \
        _in >> _dest[_macro_i]

void
avtMiliFileFormat::ParseDynaPart()
{
    readPartInfo = true;
    int i;
    int _macro_i;

    string fname = fampath;
    fname += "/" + dynaPartFilename;

    ifstream in;
    in.open(fname.c_str());

    if (in.fail())
    {
        EXCEPTION1(InvalidFilesException, fname.c_str());
    }

    char buf[1024];

    // Read the header line
    in.getline(buf, 1024);

    // Skip through the version and initial comments
    do
    {
        in.getline(buf, 1024);
    }
    while (buf[0] == '#');

    // Get the number of discrete elements
    READ_THROUGH_NEXT_COMMENT(in);
    in.getline(buf, 1024);

    // Get the number of each type
    READ_THROUGH_NEXT_COMMENT(in);
    int nNodal;
    int nHexs;
    int nBeams;
    int nShells;
    int nThickShells;
    int nProc;
    in >> nNodal >> nHexs >> nBeams >> nShells >> nThickShells >> nProc;

    // Get nodes per processor
    vector<int> nNodalPerProc(nProc, 0);
    READ_THROUGH_NEXT_COMMENT(in);
    READ_VECTOR(in, nNodalPerProc);

    // Get hexs per processor
    vector<int> nHexsPerProc(nProc, 0);
    if (nHexs)
    {
        READ_THROUGH_NEXT_COMMENT(in);
        READ_VECTOR(in, nHexsPerProc);
    }

    // Get Beams per processor
    vector<int> nBeamsPerProc(nProc, 0);
    if (nBeams)
    {
        READ_THROUGH_NEXT_COMMENT(in);
        READ_VECTOR(in, nBeamsPerProc);
    }

    // Get shells per processor
    vector<int> nShellsPerProc(nProc, 0);
    if (nShells)
    {
        READ_THROUGH_NEXT_COMMENT(in);
        READ_VECTOR(in, nShellsPerProc);
    }

    // Get thick shells per processor
    vector<int> nThickShellsPerProc(nProc, 0);
    if (nThickShells)
    {
        READ_THROUGH_NEXT_COMMENT(in);
        READ_VECTOR(in, nThickShellsPerProc);
    }


    // Get number of shared nodes per processor
    vector<int> nSharedNodes(nProc, 0);
    READ_THROUGH_NEXT_COMMENT(in);
    READ_VECTOR(in, nSharedNodes);

    // Get the number of processors a processor shares with
    vector<int> nProcComm(nProc, 0);
    READ_THROUGH_NEXT_COMMENT(in);
    READ_VECTOR(in, nProcComm);

    // Get the node divisions per processor
    vector<vector<int> > nodesProcMap(nProc);
    for (i = 0; i < nProc; ++i)
    {
        nodesProcMap[i].resize(nNodalPerProc[i]);
        READ_THROUGH_NEXT_COMMENT(in);
        READ_VECTOR(in, nodesProcMap[i]);
    }

    //
    // Now we need to skip through all the sections that define what
    // the partitioning is.
    //
    int skippedSections = 0;
    for (i = 0; i < nProc; ++i)
    {
//        if (nNodalPerProc[i]) ++skippedSections;
        if (nHexsPerProc[i])
        {
            ++skippedSections;
        }
        if (nBeamsPerProc[i])
        {
            ++skippedSections;
        }
        if (nShellsPerProc[i])
        {
            ++skippedSections;
        }
        if (nThickShellsPerProc[i])
        {
            ++skippedSections;
        }
    }

    for (i = 0; i < skippedSections; ++i)
    {
        READ_THROUGH_NEXT_COMMENT(in);
    }

    // The next READ_THROUGH_NEXT_COMMENT queues us up to the right point

    vector<int> nAdjacentProc(nProc, 0);
    vector<vector<int> > adjacentProc(nProc);
    vector<vector<int> > nSharedNodesPerProc(nProc);

    // Indexed: [domain] shares with [domain]
    vector<vector<vector< int > > >     sharedNodes;

    sharedNodes.resize(nProc);
    for (i = 0; i < nProc; ++i)
    {
        sharedNodes[i].resize(nProc);

        // Get the adjacent processors
        READ_THROUGH_NEXT_COMMENT(in);
        int adp;
        for (;;)
        {
            in >> adp;
            if (in.fail())
            {
                break;
            }
            adjacentProc[i].push_back(adp);
        }
        in.clear();
        nAdjacentProc[i] = adjacentProc[i].size();

        // We may have stripped the '#' out of the comment
        // use a getline this time.
        in.getline(buf, 1024);

        // Read in how many shared nodes there are for each shared processor
        nSharedNodesPerProc[i].resize(nAdjacentProc[i]);
        READ_VECTOR(in, nSharedNodesPerProc[i]);

        int j;
        READ_THROUGH_NEXT_COMMENT(in);
        for (j = 0; j < nAdjacentProc[i]; ++j)
        {
            int index = adjacentProc[i][j];
            sharedNodes[i][index].resize(nSharedNodesPerProc[i][j]);
            READ_VECTOR(in, sharedNodes[i][index]);
        }
    }

    // Remapp the shared node ids to the ones we store
    vector<vector<vector<pair<int,int> > > > mappings(nProc);
    for (i = 0; i < nProc; ++i)
    {
        mappings[i].resize(nProc);
        int j;
        for (j = 0; j < nProc; ++j)
        {
            int iPtr = 0;
            int k;
            for (k = 0; k < sharedNodes[i][j].size(); ++k)
            {
                int relative = sharedNodes[i][j][k];
                // Proc i is sharing with proc j
                while (nodesProcMap[i][iPtr] != relative)
                    // Incriment iPtr. This check shouldn't be
                    // necessary really.
                    if (++iPtr > nodesProcMap[j].size())
                    {
                        break;
                    }

                int jPtr;
                for (jPtr = 0; nodesProcMap[j][jPtr] != relative; ++jPtr)
                {
                    ;
                }

                sharedNodes[i][j][k] = iPtr;
                mappings[i][j].push_back(pair<int, int>(iPtr, jPtr));
            }
        }
    }

    in.close();

    avtUnstructuredPointBoundaries *upb = new avtUnstructuredPointBoundaries;

    for (i = 0; i < ndomains; ++i)
    {
        int j;
        for (j = 0; j < ndomains; ++j)
        {
            if (i == j)
            {
                continue;
            }

            if (sharedNodes[i][j].size() == 0)
            {
                continue;
            }

            if (i < j)
            {
                vector<int> d1pts;
                vector<int> d2pts;

                for (int k = 0; k < mappings[i][j].size(); ++k)
                {
                    d1pts.push_back(mappings[i][j][k].first);
                    d2pts.push_back(mappings[i][j][k].second);
                }

                upb->SetSharedPoints(i, j, d1pts, d2pts);
            }
        }
    }

    upb->SetTotalNumberOfDomains(ndomains);

    void_ref_ptr vr = void_ref_ptr(upb,
                                   avtUnstructuredPointBoundaries::Destruct);

    cache->CacheVoidRef("any_mesh", AUXILIARY_DATA_DOMAIN_BOUNDARY_INFORMATION,
                        -1, -1, vr);
}

bool
avtMiliFileFormat::CanCacheVariable(const char *varname)
{
    if (strncmp(varname, "params/", 7) == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}


// ****************************************************************************
//  Method: avtMiliFileFormat::loadMiliInfo
//
//  Purpose:
//    Read though a DynaPart output file to gather information about
//    shared nodes for generating ghostzones.
//
//  Programmer: I. R. Corey
//  Creation:   June 21, 2011
//
// ****************************************************************************
void
avtMiliFileFormat::loadMiliInfo(const char *fname)
{
    ifstream in;
    char dummy[256], path[256];
    bool newFormat=false;
    int i=0, j=0;
    int meshid=0;
    char *oneLine;
    bool lineReturned=false, eof=false;

    bool printMessage=TRUE;

#ifdef PARALLEL
    if ( PAR_Rank()!= 0 )
    {
        printMessage=FALSE;
    }
#endif
    sandFound = false;
    sphTypeFound  = false;

    in.open(fname);
    strcpy( filepath, "" );
    oneLine = readMiliFileLine(in, "*", "Path:", 0, &lineReturned, &eof);
    newFormat=false;

    if (lineReturned)
    {
        newFormat=true;
        sscanf(oneLine,"%*s %s", path);

#ifdef MDSERVER
        if ( printMessage && !miliLoadMessageDisplayed )
        {
            printf("\nOpening Mili file with path=%s\n", path);
            miliLoadMessageDisplayed = true;
        }
#endif
        strcpy( filepath, path );
        delete [] oneLine;
    }
    else
    {
        in.clear();
        in.seekg(0);
    }

    ndomains = nmeshes = 1;
    setTimesteps = false;

    if (newFormat )
    {
        oneLine = readMiliFileLine(in, "*", "Domains:", 0, &lineReturned, &eof);
        if ( lineReturned )
        {
            sscanf(oneLine,"%*s %d", &ndomains);   // Num Domains
            delete [] oneLine;
        }

        oneLine = readMiliFileLine(in, "*", "Timesteps:", 0, &lineReturned, &eof);
        if ( lineReturned )
        {
            sscanf(oneLine,"%*s %d", &ntimesteps); // Num Timesteps
            delete [] oneLine;
        }

        oneLine = readMiliFileLine(in, "*", "Dimensions:", 0, &lineReturned, &eof);
        if ( lineReturned )
        {
            sscanf(oneLine,"%*s %d", &dims);       // Num Dimensions
            delete [] oneLine;
        }

        oneLine = readMiliFileLine(in, "*", "Number_of_Meshes:", 0, &lineReturned, &eof);
        if ( lineReturned )
        {
            sscanf(oneLine,"%*s %d", &nmeshes);    // Num Meshes
            delete [] oneLine;
        }
    }
    else
    {
        in >> ndomains >> ntimesteps >> dims >> nmeshes;
    }
    dbid.resize(ndomains, -1);
    readMesh.resize(ndomains, false);
    validateVars.resize(ndomains, false);
    nnodes.resize(ndomains);
    ncells.resize(ndomains);
    connectivity.resize(ndomains);
    element_group_name.resize(ndomains);
    connectivity_offset.resize(ndomains);
    group_mesh_associations.resize(ndomains);
    materials.resize(ndomains);

    nmaterials.resize(nmeshes);

    int dom;
    for (dom = 0; dom < ndomains; ++dom)
    {
        nnodes[dom].resize(nmeshes, 0);
        ncells[dom].resize(nmeshes, 0);
        connectivity[dom].resize(nmeshes, NULL);
        materials[dom].resize(nmeshes, NULL);
    }

    int nvars;
    for (int mesh_id = 0;
            mesh_id < nmeshes;
            ++mesh_id)
    {
        if ( newFormat )
        {
            oneLine = readMiliFileLine(in, "*", "Mesh:", 0, &lineReturned, &eof);
            if ( lineReturned )
            {
                delete [] oneLine;
            }

            int nmats=0;
            oneLine = readMiliFileLine(in, "*", "Number_of_Materials:", 0, &lineReturned, &eof);
            if ( lineReturned )
            {
                sscanf(oneLine,"%*s %d", &nmats); // Num Mats
                nmaterials[mesh_id] = nmats;
                delete [] oneLine;
            }

            oneLine = readMiliFileLine(in, "*", "Number_of_Variables:", 0, &lineReturned, &eof);
            if ( lineReturned )
            {
                sscanf(oneLine,"%*s %d", &nvars); // Num Vars
                delete [] oneLine;
            }
        }
        else
        {
            in >> nmaterials[mesh_id];
            in >> nvars;
        }

        for ( int varid=0;
                varid<nvars;
                varid++ )
        {
            char nameC[256], descrC[256];
            int dim, type, center;
            string dir, name, tempName;
            string varDescr, varDir, var_name;
            size_t found, found_glob, found_mat;

            // Replace spaces in var names with special character so we can parse as a
            if ( newFormat )
            {
                int i=0, savei=0;
                oneLine = readMiliFileLine(in, "*", "", 0, &lineReturned, &eof);
                if (lineReturned)
                {
                    // Replace spaces in var names with special character so we can parse as a
                    // single name.
                    if ( strlen(oneLine)>10)
                        for ( i=10;
                                i<(strlen(oneLine)-1);
                                i++ )
                        {
                            if ( oneLine[i]==' ' && oneLine[i-1]!=' ' && oneLine[i+1]!=' ' )
                            {
                                oneLine[i]='~';
                            }
                            savei=i;
                        }

                    sscanf(oneLine,"%d %d %d %s %s", &type, &center, &dim, nameC, descrC);
                    delete [] oneLine;

                    for ( int i=0;
                            i<strlen(nameC);
                            i++ ) if ( nameC[i]=='~' )
                        {
                            nameC[i]=' ';
                        }
                    name = string(nameC);

                    // Remove trailing spaces
                    int len=name.length();

                    found = name.find_last_not_of(" ");
                    if ( found!=string::npos && found!=(name.length()-1) )
                    {
                        name = name.substr(0, found+1);
                    }

                    for ( int i=0;
                            i<strlen(descrC);
                            i++ ) if ( descrC[i]=='~' )
                        {
                            descrC[i]=' ';
                        }

                    // Determine of Material or Global and remove notation
                    // from the name.

                    var_name = name;
                    if ( var_name=="" )
                    {
                        continue;
                    }

                    found_glob = var_name.find("[glob]");
                    if ( found_glob!=string::npos )
                    {
                        var_name.replace( found_glob, 6, "" );
                        vars[var_name].global_var = true;
                    }

                    found_mat = var_name.find("[mat]");
                    if ( found_mat!=string::npos )
                    {
                        var_name.replace( found_mat, 5, "" );
                        vars[var_name].mat_var = true;
                    }
                    if ( found_glob!=string::npos && found_mat!=string::npos )
                    {
                        vars[var_name].mat_var    = false;
                        vars[var_name].global_var = false;
                    }
                }
            }
            else
            {
                in >> type >> center >> dim;

                // Strip out leading white space.
                while(isspace(in.peek()))
                {
                    in.get();
                }

                getline(in, name);
                var_name = name;

                vars[var_name].global_var = false;
                vars[var_name].mat_var    = false;
            }

            // Seperate var dir from name
            found=0;

            found = var_name.find_last_of("/");
            if ( found!=string::npos )
            {
                varDir = var_name.substr(0, found+1);
                if (varDir!="params/")
                {
                    var_name = var_name.substr(found+1);
                }
            }
            if (varDir=="params/")
            {
                varDir="";
            }

            if ( var_name=="sand" )
            {
                sandFound = true;
            }

            if ( var_name=="sph_itype" )
            {
                sphTypeFound = true;
            }

            varDescr                             = string(descrC);
            vars[var_name].descr                 = varDescr;
            vars[var_name].dir                   = varDir;
            vars[var_name].centering             = avtCentering(center);
            vars[var_name].var_type              = avtVarType(type);
            vars[var_name].var_dimension         = dim;
            vars[var_name].var_mesh_associations = mesh_id;
            vars[var_name].sub_record_ids.resize(ndomains);
            vars[var_name].state_record_ids.resize(ndomains);
            vars[var_name].vars_size = new int[ndomains];
        }
    }

    // Read int the part file, if it exists.
    readPartInfo = true;
    dynaPartFilename = "";

    in >> dynaPartFilename;
    if (dynaPartFilename != "")
    {
        readPartInfo = false;
    }
    else
    {
        readPartInfo = true;
    }

    in.close();
}


// ***************************************************************************
//  Function: readMiliFileLine
//
//  Purpose:
//
//  Arguments: in - input stream
//             commentSymbol - optional comment string
//             keyword - if set, then return line beginning with this string
//             lineN - return line number N
//             lineReturned - set to true if a line of data is returned
//             eof - return eof state (true | false)
//
//  Notes:
//
//  Modifications:
//
// ****************************************************************************
char *
avtMiliFileFormat::readMiliFileLine(ifstream &in, const char *commentSymbol, const char *kw,
                                    int lineN, bool *lineReturned, bool *eof)
{
    char *oneLine, dummy[512];
    int i=0;
    int maxLen=0;
    int lineNumber=0;

    bool lineFound=false, commentFound=false;

    if (lineN>0)
    {
        in.seekg(0);   // Rewind file
    }

    *eof=false;

    while (!lineFound)
    {
        in.getline(dummy, 500 );

        if (in.eof())
        {
            *eof=true;
            return NULL;
        }

        if (strlen(dummy)==0)
        {
            continue;
        }

        string field1;
        std::stringstream ss(dummy);
        field1="";
        ss >> field1;

        // Check for a comment line
        if (commentSymbol)
            if (strlen(commentSymbol)>0)
            {
                if (strlen(commentSymbol) <= field1.length())
                {
                    maxLen = strlen(commentSymbol);
                }
                else
                {
                    maxLen = field1.length();
                }

                commentFound=true;
                int charMatch=0;
                for (int i=0;
                        i<maxLen;
                        i++)
                {
                    if (field1[i]!=commentSymbol[i])
                    {
                        commentFound=false;
                        break;
                    }
                    else
                    {
                        charMatch++;
                    }
                    if (  charMatch!=strlen(commentSymbol) )
                    {
                        commentFound=false;
                    }
                }
            }

        if (commentFound)
        {
            continue;
        }

        char kwUpper[256];
        if (kw)
            if (strlen(kw)>0)
            {
                /* Convert fields to upper case */
                strcpy(kwUpper, kw);
                for (int i=0;
                        i<strlen(kw);
                        i++)
                {
                    kwUpper[i] = toupper(kw[i]);
                }
            }

        for (int i = 0;
                i < field1.length();
                i++)
        {
            field1[i]=toupper(field1[i]);
        }

        if ( !strcmp(field1.c_str(), kwUpper ))
        {
            lineFound=true;
            break;
        }

        if ( lineN>0 )
        {
            if (lineNumber==lineN)
            {
                if (in.eof())
                {
                    *eof=true;
                }
                lineFound=true;
            }
        }

        if (lineN<=0 && strlen(kw)==0)
        {
            lineFound=true;
        }
    }
    if (lineFound)
    {
        oneLine = new char[strlen(dummy)+2];
        strcpy(oneLine, dummy);
        *lineReturned=true;
        return oneLine;
    }
    else
    {
        *lineReturned=false;
        return NULL;
    }
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::PopulateMiliClassData
//
//  Purpose:
//      Gets Mili Class Data.
//
//  Arguments:
//    dom        the domain
//
//  Programmer:  Ivan R. Corey
//  Creation:    April 11, 2011
//
//  Modifications:
//
// ****************************************************************************

void
avtMiliFileFormat::PopulateMiliClassData( int dom, int mesh_id )
{
    int i,j,k;
    int *node_labels;
    int *conns, *mats, *parts;

    int inital_vars_count=vars.size();
    int label_index=0;

    char short_name[1024];
    char long_name[1024];
    int status;
    int node_id=0;

    status = mc_get_class_info(dbid[dom], mesh_id, M_NODE, node_id, short_name,
                               long_name, &qty_nodes);

    node_labels = new int[qty_nodes];
    int block_qty=0, *block_list=NULL;
    labelsFound = false;

    // Determine if we have labels

    if ( dom==0 )
    {
        global_node_id = 0;
        global_zone_id = 0;
    }

    status = mc_load_node_labels(dbid[dom], mesh_id, (char *)"node",
                                 &block_qty, &block_list,
                                 node_labels);
    if ( block_qty>0 )
    {
        labelsFound = true;
        if ( block_list!= NULL )
        {
            free(block_list);
        }
    }

    int class_id = 1;
    for (i = 0 ;
            i < n_elem_types ;
            i++)
    {
        int args[2];
        args[0] = mesh_id;
        args[1] = elem_sclasses[i];
        int ngroups = 0;
        bool DBC_class=false;

        mc_query_family(dbid[dom], QTY_CLASS_IN_SCLASS, (void*) args, NULL,
                        (void*) &ngroups);

        for (j = 0;
                j < ngroups;
                j++)
        {
            int nelems, sclass=0;
            mc_get_class_info(dbid[dom], mesh_id, elem_sclasses[i], j,
                              short_name, long_name, &nelems);

            // Update Particle Class counts

            // Add the classname
            if (mili_elem_class.find( short_name ) == mili_elem_class.end())
            {
                mili_elem_class[short_name].short_name = string(short_name);
                mili_elem_class[short_name].long_name  = string(long_name);
                mili_elem_class[short_name].qty             = 0;
                mili_elem_class[short_name].class_id        = class_id++;
                mili_elem_class[short_name].SPHClass        = false;
                mili_elem_class[short_name].addedToMetadata = false;
                mili_elem_class[short_name].zone_data       = new mili_zones_type[ndomains];
                for ( k=0;
                        k<ndomains;
                        k++ )
                {
                    mili_elem_class[short_name].zone_data[k].qty = 0;
                }

                mili_classes[string(short_name)]            = string(short_name);
            }

            qty_cells+=nelems;
            mili_elem_class[short_name].sclass = elem_sclasses[i];
            if ( is_particle_class( mili_elem_class[short_name].sclass, short_name, &DBC_class ) )
            {
                if ( !DBC_class )
                {
                    mili_elem_class[short_name].SPHClass = true;
                    mili_SPH_classes[short_name] = short_name;
                    numSPH_classes++;
                    num_part_elems[dom] += nelems;
                    part_nodes_found = true;
                }
                else
                {
                    mili_elem_class[short_name].DBCClass = true;
                    mili_DBC_classes[short_name] = short_name;
                    numDBC_classes++;
                    num_dbc_elems[dom] += nelems;
                    dbc_nodes_found = true;
                }
            } // is_particle_class()

            mili_elem_class[short_name].qty += nelems;

            conns = new int[nelems * conn_count[i]];
            mats  = new int[nelems];
            parts = new int[nelems];

            mc_load_conns(dbid[dom], mesh_id, short_name, conns, mats, parts);

            mili_elem_class[short_name].zone_data[dom].qty       = nelems;

            mili_elem_class[short_name].zone_data[dom].labels    = NULL;
            mili_elem_class[short_name].zone_data[dom].visit_ids = NULL;
            mili_elem_class[short_name].zone_data[dom].sand      = NULL;
            mili_elem_class[short_name].addedToMetadata          = false;

            if ( nelems>0 )
            {
                mili_elem_class[short_name].zone_data[dom].mats      = new int[nelems];
                mili_elem_class[short_name].zone_data[dom].parts     = new int[nelems];
                mili_elem_class[short_name].zone_data[dom].conns     = new int[nelems*conn_count[i]];
                mili_elem_class[short_name].zone_data[dom].conn_count = conn_count[i];
                mili_elem_class[short_name].zone_data[dom].ids       = new int[nelems];
            }
            else
            {
                mili_elem_class[short_name].zone_data[dom].mats      = NULL;
                mili_elem_class[short_name].zone_data[dom].parts     = NULL;
                mili_elem_class[short_name].zone_data[dom].conns     = NULL;
                mili_elem_class[short_name].zone_data[dom].conn_count = NULL;
                mili_elem_class[short_name].zone_data[dom].ids       = NULL;
            }

            for (k=0;
                    k<nelems;
                    k++)
            {
                mili_elem_class[short_name].zone_data[dom].parts[k]   = parts[k];
                mili_elem_class[short_name].zone_data[dom].mats[k]    = mats[k];
            }

            for (k=0;
                    k<nelems*conn_count[i];
                    k++)
            {
                mili_elem_class[short_name].zone_data[dom].conns[k] = conns[k];
            }

            delete [] conns;
            delete [] mats;
            delete [] parts;

            if ( mili_elem_class[short_name].zone_data[dom].labels == NULL )
            {
                mili_elem_class[short_name].zone_data[dom].labels    = new int[nelems];
                mili_elem_class[short_name].zone_data[dom].ids       = new int[nelems];
                mili_elem_class[short_name].zone_data[dom].visit_ids = new int[nelems];
            }

            // Load the Connection Labels
            if ( labelsFound )
            {
                int *temp_ids    = new int[nelems];
                int *temp_labels = new int[nelems];

                mc_load_conn_labels( dbid[dom], mesh_id, short_name,
                                     nelems,
                                     &block_qty, &block_list,
                                     temp_ids, temp_labels );

                for (k=0;
                        k<nelems;
                        k++)
                {
                    mili_elem_class[short_name].zone_data[dom].labels[k]    = temp_labels[k];
                    mili_elem_class[short_name].zone_data[dom].ids[k]       = temp_ids[k];
                    mili_elem_class[short_name].zone_data[dom].visit_ids[k] = global_zone_id++;
                    if ( global_zone_id>qty_cells )
                    {
                        labelsFound = true;
                    }
                }

                mili_elem_class[short_name].zone_data[dom].block_qty   = block_qty;
                mili_elem_class[short_name].zone_data[dom].block_list = new int[block_qty*2];
                for (k=0;
                        k<block_qty*2;
                        k++)
                {
                    mili_elem_class[short_name].zone_data[dom].block_list[k] = block_list[k];
                }

                delete [] temp_ids;
                delete [] temp_labels;
                if ( block_list!=NULL )
                {
                    free(block_list);
                }
            }
            else
            {
                for (k=0;
                        k<nelems;
                        k++)
                {
                    mili_elem_class[short_name].zone_data[dom].labels[k]    = global_zone_id;
                    mili_elem_class[short_name].zone_data[dom].ids[k]       = global_zone_id;
                    mili_elem_class[short_name].zone_data[dom].visit_ids[k] = global_zone_id;
                    global_zone_id++;
                }

                mili_elem_class[short_name].zone_data[dom].block_qty   = 1;
                mili_elem_class[short_name].zone_data[dom].block_list = new int[block_qty*2];
                mili_elem_class[short_name].zone_data[dom].block_list[0] = 1;
                mili_elem_class[short_name].zone_data[dom].block_list[1] = nelems;
            }
        }
    }

    mc_get_class_info(dbid[dom], mesh_id, M_NODE, node_id,
                      short_name, long_name, &qty_nodes);

    if (mili_node_class.find( short_name ) == mili_node_class.end())
    {
        mili_node_class[short_name].short_name = string(short_name);
        mili_node_class[short_name].long_name  = string(long_name);
        mili_node_class[short_name].qty        = qty_nodes;
        mili_node_class[short_name].node_data  = new  mili_nodes_type[ndomains];
        for ( i=0;
                i<ndomains;
                i++ )
        {
            mili_node_class[short_name].node_data[dom].qty=0;
            mili_node_class[short_name].node_data[dom].labels=NULL;
        }
    }
    else
    {
        mili_node_class[short_name].qty += qty_nodes;
    }

    // Load the Node labels

    block_qty=0;
    block_list=NULL;
    strcpy( short_name, "node" );
    mc_load_node_labels(dbid[dom], mesh_id, (char *)"node",
                        &block_qty, &block_list,
                        node_labels);

    mili_node_class[short_name].node_data[dom].qty=qty_nodes;
    mili_node_class[short_name].node_data[dom].labels = new int[qty_nodes];

    if ( node_labels!=NULL && block_qty>0 )
    {
        labelsFound = true;

        for ( label_index=0;
                label_index<qty_nodes;
                label_index++ )
        {
            mili_node_class[short_name].node_data[dom].labels[label_index] = node_labels[label_index];
        }

        if ( block_list!= NULL )
        {
            free(block_list);
        }
        if ( node_labels!=NULL )
        {
            free(node_labels);
        }
    }
    else
    {
        for ( label_index=0;
                label_index<qty_nodes;
                label_index++ )
        {
            mili_node_class[short_name].node_data[dom].labels[label_index] = global_node_id++;
        }
    }
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::PopulateClassMetaData
//
//  Purpose:
//      Add Mili Class data to the MD structure.
//
//  Arguments:
//    dom        the domain
//   *md         avtDatabaseMetaData  - Metadata structure
//
//  Programmer:  Ivan R. Corey
//  Creation:    April 12, 2011
//
//  Modifications:
//
// ****************************************************************************
void
avtMiliFileFormat::PopulateClassMetaData( int dom, avtDatabaseMetaData *md )
{
    int i=0;
    int num_elems=0;
    std::string pnmeshname_str, dbcmeshname_str, fnmeshname_str;
    char meshname[64];

    global_class_ids[dom] = new int [qty_cells];
    global_matlist[dom]   = new int [qty_cells];

    for ( mili_elem_class_iter =  mili_elem_class.begin();
            mili_elem_class_iter != mili_elem_class.end();
            mili_elem_class_iter++ )
    {
        int i=0;
        mili_elem_class_type    class_data;
        std::string        class_name, tempStr;
        int                class_id=0;
        char class_id_str[64];
        int                sclass=0;
        char               sclass_name[256];

        class_name = (*mili_elem_class_iter).first;

        sclass = mili_elem_class[class_name].sclass;
        sprintf(sclass_name, "%d", sclass);

        if ( mili_elem_class[class_name].zone_data )
        {
            num_elems = mili_elem_class[class_name].zone_data[dom].qty;
        }
        else
        {
            num_elems = 0;
        }

        for ( i=0;
                i<num_elems;
                i++ )
        {
            int elemid=0, classid=0;
            classid = mili_elem_class[class_name].class_id;
            elemid  = mili_elem_class[class_name].zone_data[dom].visit_ids[i];
            global_class_ids[dom][elemid] = classid;
            global_matlist[dom][elemid]   = mili_elem_class[class_name].zone_data[dom].mats[i];
        }
    }

    if ( dom==0 )
        for ( i = 0;
                i < nmeshes;
                i++ )
        {
            sprintf(meshname, "mesh%d", i + 1);
            PopulateClassMetaDataForMesh( dom, meshname, md );

            if ( free_nodes_found )
            {
                sprintf(meshname, "mesh%d", i + 1);
                fnmeshname_str = string(meshname) + "_" + string(free_nodes_str);
                strcpy( meshname, fnmeshname_str.c_str() );
                PopulateClassMetaDataForMesh( dom, meshname, md );
            }

            if ( part_nodes_found )
            {
                pnmeshname_str = string(meshname) + "_" + string(part_nodes_str);
                strcpy( meshname, pnmeshname_str.c_str() );
                PopulateClassMetaDataForMesh( dom, meshname, md );
            }

            if ( dbc_nodes_found )
            {
                sprintf(meshname, "mesh%d", i + 1);
                dbcmeshname_str = string(meshname) + "_" + string(dbc_nodes_str);
                strcpy( meshname, dbcmeshname_str.c_str() );
                PopulateClassMetaDataForMesh( dom, meshname, md );
            }
        }
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::PopulateClassMetaDataForMesh
//
//  Purpose:
//      Add Mili Class data to the MD structure.
//
//  Arguments:
//    dom        the domain
//   *md         avtDatabaseMetaData  - Metadata structure
//
//  Programmer:  Ivan R. Corey
//  Creation:    April 12, 2011
//
//  Modifications:
//
// ****************************************************************************

void
avtMiliFileFormat::PopulateClassMetaDataForMesh( int dom, char *meshname, avtDatabaseMetaData *md )
{
    int i=0;
    int elemid=0;
    char class_id_str[64], subset_name[64];
    bool create_subset=true, create_class=true;;

    strcpy( subset_name, meshname );
    strcat( subset_name, "/" );
    strcat( subset_name, "MiliClasses" );


    // Add subsets for non-particle meshes and for DBC meshes only if
    // the DBC mesh have>1 class.

    if ( numDBC_classes<=1 && strstr(meshname, part_nodes_str) )
    {
        create_subset = false;
    }
    if ( numDBC_classes<=1 && strstr(meshname, dbc_nodes_str) )
    {
        create_subset = false;
    }

    if ( create_subset )
    {
        avtScalarMetaData *subset_classes = new avtScalarMetaData(subset_name, meshname, AVT_ZONECENT);
        subset_classes->hideFromGUI = true;
        subset_classes->SetEnumerationType(avtScalarMetaData::ByValue);

        for ( mili_elem_class_iter =  mili_elem_class.begin();
                mili_elem_class_iter != mili_elem_class.end();
                mili_elem_class_iter++ )
        {

            int i=0;
            mili_elem_class_type    class_data;
            std::string        class_name, tempStr;
            int                class_id=0;
            char class_id_str[64], class_name_str[64];;
            int                sclass=0;
            char               sclass_name[256];
            bool DBC_class=false, part_class=false;;

            class_name = (*mili_elem_class_iter).first;
            strcpy( class_name_str, class_name.c_str() );
            part_class = is_particle_class( 0, class_name_str, &DBC_class );

            sclass = mili_elem_class[class_name].sclass;
            sprintf(sclass_name, "%d", sclass);

            // Mili Class names and ids
            avtScalarMetaData *classes = new avtScalarMetaData();
            classes->meshName = meshname;
            sprintf( class_id_str, "%d", class_id );
            //
            // Note - Never put special characters used in expressions such as [ or ( in
            // a string name! They will be parsed by Visit as an expression.
            //
            classes->name  = "MiliClass/" + class_name + "/Id/" + string(class_id_str) +"/Sclass/"+ string(sclass_name);
            if ( mili_elem_class[class_name].sclass == M_NODE )
            {
                classes->centering   = AVT_NODECENT;
            }
            else
            {
                classes->centering   = AVT_ZONECENT;
            }
            classes->hideFromGUI = true; // Hide from GUI
            md->Add(classes);

            // For a DBC class only subset on DBC classes and not other classes like brick. Only subset
            // if more than on DBC class.
            create_class = true;
            if ( strstr(meshname, dbc_nodes_str) && !DBC_class )
            {
                create_class = false;
            }
            if ( create_class )
                if (  mili_elem_class[class_name].sclass>=M_TRUSS && mili_elem_class[class_name].sclass <= M_PARTICLE )
                {
                    subset_classes->AddEnumNameValue(class_name, mili_elem_class[class_name].class_id);
                }
            mili_elem_class[class_name].addedToMetadata = true;
        }
        md->Add(subset_classes);
    }

    // LABELS For Nodes
    avtScalarMetaData *avtLabelsNodes = new avtScalarMetaData();
    avtLabelsNodes->meshName    = meshname;
    avtLabelsNodes->name        = "Labels_Node";
    avtLabelsNodes->centering   = AVT_NODECENT;
    avtLabelsNodes->hideFromGUI = true;
    md->Add(avtLabelsNodes);

    // LABELS For Zones/Elements
    avtScalarMetaData *avtLabelsZones = new avtScalarMetaData();
    avtLabelsZones->meshName    = meshname;
    avtLabelsZones->name        = "Labels_Zone";
    avtLabelsZones->centering   = AVT_ZONECENT;
    avtLabelsZones->hideFromGUI = true; // Hide from GUI
    md->Add(avtLabelsZones);

    // IDS For Zones/Elements
    avtScalarMetaData *avtIdsZones = new avtScalarMetaData();
    avtIdsZones->meshName    = meshname;
    avtIdsZones->name        = "Ids_Zone";
    avtIdsZones->centering   = AVT_ZONECENT;
    avtIdsZones->hideFromGUI = true;
    md->Add(avtIdsZones);

    avtScalarMetaData *avtMatsZone = new avtScalarMetaData();
    avtMatsZone->meshName    = meshname;
    avtMatsZone->name        = "Classes_Zone";
    avtMatsZone->centering   = AVT_ZONECENT;
    avtMatsZone->hideFromGUI = false;
    md->Add(avtMatsZone);

    avtScalarMetaData *avtTypesZone = new avtScalarMetaData();
    avtTypesZone->meshName    = meshname;
    avtTypesZone->name        = "Types_Zone";
    avtTypesZone->centering   = AVT_ZONECENT;
    avtTypesZone->hideFromGUI = true; // Hide from GUI
    md->Add(avtTypesZone);

    // Add Subset Variables

    avtScalarMetaData *avtClassSubset = new avtScalarMetaData();
    avtClassSubset->meshName   = meshname;
    avtClassSubset->name        = "Class_Subset";
    avtClassSubset->centering   = AVT_ZONECENT;
    avtClassSubset->hideFromGUI = true;
    md->Add(avtClassSubset);

    if ( sandFound )
    {
        strcpy( subset_name, meshname );
        strcat( subset_name, "/" );
        strcat( subset_name, "MiliSand" );
        avtScalarMetaData *subset_sand = new avtScalarMetaData(subset_name, meshname, AVT_ZONECENT);
        subset_sand->hideFromGUI = true;
        subset_sand->SetEnumerationType(avtScalarMetaData::ByValue);
        subset_sand->AddEnumNameValue("Inactive", 0);
        subset_sand->AddEnumNameValue("Active", 1);
        md->Add(subset_sand);

        avtScalarMetaData *avtSandSubset = new avtScalarMetaData();
        avtSandSubset->meshName   = meshname;
        avtSandSubset->name        = "Sand_Subset";
        avtSandSubset->centering   = AVT_ZONECENT;
        avtSandSubset->hideFromGUI = true;
        md->Add(avtSandSubset);
    }

    if ( sphTypeFound && strstr(meshname, part_nodes_str) )
    {
        strcpy( subset_name, meshname );
        strcat( subset_name, "/" );
        strcat( subset_name, "MiliSphType" );
        avtScalarMetaData *subset_SphType = new avtScalarMetaData(subset_name, meshname, AVT_ZONECENT);
        subset_SphType->hideFromGUI = true;
        subset_SphType->SetEnumerationType(avtScalarMetaData::ByValue);
        subset_SphType->AddEnumNameValue("Not SPH", -1);
        subset_SphType->AddEnumNameValue("Free to move", 0);
        subset_SphType->AddEnumNameValue("SPH Ghost", 1);
        md->Add(subset_SphType);

        avtScalarMetaData *avtSphTypeSubset = new avtScalarMetaData();
        avtSphTypeSubset->meshName   = meshname;
        avtSphTypeSubset->name        = "SphType_Subset";
        avtSphTypeSubset->centering   = AVT_ZONECENT;
        avtSphTypeSubset->hideFromGUI = true;
        md->Add(avtSphTypeSubset);
    }
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::PopulatelMiliVarData
//
//  Purpose:
//      Gets Mili Variable Data for classes and svars.
//
//  Arguments:
//    dom        the domain
//
//  Programmer:  Ivan R. Corey
//  Creation:    May 21, 2010
//
//  Modifications:
//
// ****************************************************************************

void
avtMiliFileFormat::PopulateMiliVarData( int dom, int mesh_id )
{
    int i,j;
    int nelems=0;
    int srec_qty = 0;

    char short_name[1024];
    char long_name[1024];
    string primal_class_name, primal_name;

    bool read_ti=true;

    int inital_vars_count=vars.size();
    int args[2];
    int rval;

    // Load the Mili Metadata
    if ( dom==0 )
    {
        char mili_version[256], host[256], arch[512], timestamp[512], xmilics_version[512];
        mc_get_metadata( dbid[dom],  mili_version, host, arch, timestamp, xmilics_version );
    }

    // Load TI variables
    int  num_entries = 0;
    char **temp_list;
    int element_set_num=1;

    num_entries = mc_ti_htable_search_wildcard(dbid[dom], 0, FALSE,
                  (char *)"*", (char *)"NULL", (char *)"NULL",
                  NULL );
    if ( num_entries> 0 )
    {
        temp_list = (char **) malloc(num_entries*sizeof(char *));

        num_entries = mc_ti_htable_search_wildcard(dbid[dom], 0, FALSE,
                      (char *)"*", (char *)"NULL", (char *)"NULL",
                      temp_list);
        for ( int i=0; i<num_entries; i++ )
        {
            int status;
            char class_name[128];

            // Do not add Element or Node label entries to TOC. These fields are
            // loaded in from mc_load labels functions.
            if ( strncmp( temp_list[i], "Element Labels", 14)==0 ||
                    strncmp( temp_list[i], "Node Labels", 11)   ==0 )
            {
                free( temp_list[i] );
                continue;
            }

            int dataType, dataLen;
            bool DBC_class=false, dummy=false;
            rval = mc_ti_get_data_len( dbid[dom], temp_list[i], &dataType, &dataLen );

            if ( is_particle_class( 0, temp_list[i], &DBC_class ) )
            {
                status = mc_ti_read_string( dbid[dom], temp_list[i], class_name );
                dummy = is_particle_class( 0, class_name, &DBC_class );

                // SPH Class
                if ( !DBC_class )
                {
                    mili_elem_class[class_name].SPHClass = true;
                    mili_SPH_classes[class_name] = class_name;
                    if  (mili_elem_class[class_name].SPHClass == false )
                    {
                        mili_elem_class[class_name].SPHClass = true;
                        numSPH_classes++;
                    }
                    part_nodes_found = true;
                }
                // DBC Class
                else
                {
                    mili_elem_class[class_name].DBCClass = true;
                    mili_DBC_classes[class_name] = class_name;
                    if  (mili_elem_class[class_name].DBCClass == false )
                    {
                        mili_elem_class[class_name].DBCClass = true;
                        numDBC_classes++;
                    }
                    dbc_nodes_found = true;
                }
            }

            mili_ti_fields[temp_list[i]].fieldName = strdup(temp_list[i]);
            mili_ti_fields[temp_list[i]].dataType = dataType;
            mili_ti_fields[temp_list[i]].dataLen  = dataLen;

            // Load Element Set Integration Point Data
            if ( strncmp( temp_list[i], "IntLabel", 8)==0 )
            {

                // *********************************************************
                // Read int the integration point labels. The last element
                // of the array is the total number of integration points.
                //
                // For example:
                //  5 10 25 32 51
                //             ^- This element set has a max of 51
                //                integration points.
                //
                //             Inner  =  5
                //             Middle = 25
                //             Outer  = 32
                // *********************************************************
                bool status=false;
                int datatype=0, datalength=0, num_items_read=0;
                char es_name[16], *es_name_ptr=NULL;

                status = mc_ti_get_data_len( dbid[dom], temp_list[i],
                                             &datatype, &datalength );
                if ( status !=OK )
                {
                    break;
                }

                es_name_ptr = strstr( temp_list[i], "es_" );
                if ( !es_name_ptr )
                {
                    break;
                }
                strcpy ( es_name, es_name_ptr );

                es_intpoints[es_name].es_id           = element_set_num;
                es_intpoints[es_name].num_labels      = datalength-1;

                status = mc_ti_read_array(dbid[dom], temp_list[i],
                                          (void **) & es_intpoints[es_name].labels, &num_items_read );

                es_intpoints[es_name].intpoints_total = es_intpoints[es_name].labels[num_items_read-1];
                status = set_default_intpoints( es_name );
            }
            free( temp_list[i] );
        }
        free( temp_list );
    }
    rval = mc_query_family(dbid[dom], QTY_SREC_FMTS, NULL, NULL,
                           (void*) &srec_qty);

    for (i = 0 ; i < srec_qty ; i++)
    {
        int subrecs = 0;
        rval = mc_query_family(dbid[dom], QTY_SUBRECS, (void *) &i, NULL,
                               (void *) &subrecs);

        for (int j = 0;
                j < subrecs;
                j++)
        {
            Subrecord sr;
            memset(&sr, 0, sizeof(sr));
            rval = mc_get_subrec_def(dbid[dom], i, j, &sr);

            // Add Svar components to Primals list
            for (int k = 0;
                    k < sr.qty_svars;
                    k++)
            {
                State_variable sv;
                bool SPH_var=false;

                memset(&sv, 0, sizeof(sv));
                mc_get_svar_def(dbid[dom], sr.svar_names[k], &sv);

                primal_name = string(sr.svar_names[k]);

                // Skip it if is was not requested in the .mili file
                vars_iter = vars.find( primal_name );
                if ( vars_iter==vars.end() )
                {
                    continue;
                }

                mili_primal_fields[primal_name].class_names.push_back(sr.class_name);
                mili_primal_fields[primal_name].dataType    = sv.num_type;
                mili_primal_fields[primal_name].vecLen      = sv.vec_size;
                mili_primal_fields[primal_name].SPH_var     = false;
                mili_primal_fields[primal_name].DBC_var     = false;
                mili_primal_fields[primal_name].es_var      = false;
                mili_primal_fields[primal_name].reg_var     = false;

                mili_class_iter = mili_SPH_classes.find( sr.class_name );
                if ( mili_class_iter!=mili_SPH_classes.end() )
                {
                    SPH_var = true;
                    mili_primal_fields[primal_name].SPH_var = true;
                }
                else
                {
                    mili_primal_fields[primal_name].reg_var = true;
                }
                if ( !SPH_var )
                    if ( mili_class_iter!=mili_DBC_classes.end() )
                    {
                        mili_primal_fields[primal_name].DBC_var = true;
                    }
                    else
                    {
                        mili_primal_fields[primal_name].reg_var = true;
                    }

                if ( mili_elem_class[sr.class_name].sclass == M_NODE )
                {
                    mili_primal_fields[primal_name].node_centered = true;
                }
                else
                {
                    mili_primal_fields[primal_name].node_centered = false;
                }

                if ( strncmp( primal_name.c_str(), "es_", 3 ) )
                {
                    mili_primal_fields[primal_name].es_var = true;
                }

                if ( sv.vec_size> 1 )
                {
                    for ( int  v = 0;
                            v < sv.vec_size;
                            v++)
                    {
                        mili_primal_components[primal_name].push_back(sv.components[v]);
                    }
                }
                else
                {
                    mili_primal_components[primal_name].push_back("NULL");
                }

                mili_primal_classes[primal_name].push_back(sr.class_name);
            }
        }
    }

    // Remove duplicates from the primal components list.
    for ( mili_primal_components_iter =  mili_primal_components.begin();
            mili_primal_components_iter != mili_primal_components.end();
            mili_primal_components_iter++ )
    {
        (*mili_primal_components_iter).second.sort();
        (*mili_primal_components_iter).second.unique();
    }

    // Remove duplicates from the primal classes list.
    for ( mili_primal_classes_iter =  mili_primal_classes.begin();
            mili_primal_classes_iter != mili_primal_classes.end();
            mili_primal_classes_iter++ )
    {
        (*mili_primal_classes_iter).second.sort();
        (*mili_primal_classes_iter).second.unique();
    }

    int num_parts=0, num_dbc=0;
    int conn_index=0, elem_index=0;
    std::string  class_name;

    if ( numSPH_classes>0 )
    {
        part_nodes[dom] = new int[num_part_elems[dom]];
        part_elems[dom] = new int[num_part_elems[dom]];
        elem_index = 0;
        for ( mili_class_iter  = mili_SPH_classes.begin();
                mili_class_iter != mili_SPH_classes.end();
                mili_class_iter++ )
        {
            class_name = (*mili_class_iter).first;

            // Collect the node numbers for the SPH Particles
            num_parts = mili_elem_class[class_name].zone_data[dom].qty;

            if ( num_parts>0 )
            {
                conn_index = 0;
                for ( i=0;
                        i<num_parts;
                        i++ )
                {
                    part_elems[dom][elem_index] = mili_elem_class[class_name].zone_data[dom].labels[i];
                    part_nodes[dom][elem_index] = mili_elem_class[class_name].zone_data[dom].conns[conn_index];
                    elem_index++;
                    conn_index+= mili_elem_class[class_name].zone_data[dom].conn_count;
                }
            }
        }
    } /* numSPH_classes */

    if ( numDBC_classes>0 )
    {
        dbc_nodes[dom] = new int[num_dbc_elems[dom]];
        dbc_elems[dom] = new int[num_dbc_elems[dom]];
        elem_index = 0;

        for ( mili_class_iter  = mili_DBC_classes.begin();
                mili_class_iter != mili_DBC_classes.end();
                mili_class_iter++ )
        {
            class_name = (*mili_class_iter).first;

            // Collect the node numbers for the DBC Particles
            num_dbc = mili_elem_class[class_name].zone_data[dom].qty;

            if ( num_dbc>0 )
            {
                conn_index = 0;

                for ( i=0;
                        i<num_dbc;
                        i++ )
                {
                    dbc_elems[dom][elem_index] = mili_elem_class[class_name].zone_data[dom].labels[i];
                    dbc_nodes[dom][elem_index] = mili_elem_class[class_name].zone_data[dom].conns[conn_index];
                    elem_index++;
                    conn_index                 += mili_elem_class[class_name].zone_data[dom].conn_count;
                }
            }
        }
    } /* numDBC_classes */
}

// ****************************************************************************
//  Method:  avtMiliFileFormat::PopulatePrimalMetaData
//
//  Purpose:
//      Add Mili Primal data to the MD structure.
//
//  Arguments:
//    dom        the domain
//   *md         avtDatabaseMetaData  - Metadata structure
//
//  Programmer:  Ivan R. Corey
//  Creation:    April 12, 2011
//
//  Modifications:
//
// ****************************************************************************

void
avtMiliFileFormat::PopulatePrimalMetaData( avtDatabaseMetaData *md )
{
    int args[2];
    char short_name[1024];
    char long_name[1024];
    bool part_class=false;

    for ( mili_primal_components_iter =  mili_primal_components.begin();
            mili_primal_components_iter != mili_primal_components.end();
            mili_primal_components_iter++ )
    {
        string class_name, comp_name;
        string primal_name, noexpr_primal_name;

        std::list<std::string>  classes;
        std::list<std::string>  components;

        std::list<std::string>::iterator iter_comp, iter_class;

        primal_name = (*mili_primal_components_iter).first;
        components  = (*mili_primal_components_iter).second;

        int  comp_index=0, comp_index_x=0, comp_index_y=0;
        int num_classes=0;
        char comp_index_string[256], comp_string[256];

        classes     = mili_primal_classes[primal_name];
        num_classes = classes.size();

        for( iter_class =  classes.begin();
                iter_class != classes.end();
                iter_class++ )
        {
            class_name = (*iter_class);
            char es_name[64], *slash_ptr;;
            int comp_len=components.size();
            int num_labels=0, *labels;
            int status=OK;

            comp_index=0;
            comp_index_x=0,
            comp_index_y=0;

            for( iter_comp =  components.begin();
                    iter_comp != components.end();
                    iter_comp++ )
            {
                int comp_len=components.size();
                int num_classes=0;
                comp_name = (*iter_comp);

                string svar_name, expr_name;
                size_t pos;

                svar_name = primal_name;
                expr_name = primal_name;

                if ( class_name=="mat" )
                {
                    expr_name="Material/" + svar_name;
                }
                if ( class_name=="glob" )
                {
                    expr_name="Global/" + svar_name;
                }

                /* Add the Primals */
                Expression primal_expr;

                status = get_es_info( primal_name.c_str(), &num_labels, &labels );

                // Add Multi-Integration Point Primals

                if ( status && num_labels>0 )
                {
                    if ( comp_name!="NULL" )
                    {
                        char comp_index_str[16], intPoint_str[16];
                        int i=0;

                        /* Add the Primals */
                        Expression es_primal_expr;
                        for ( i=0;
                                i<num_labels;
                                i++ )
                        {
                            Expression es_primal_expr;
                            sprintf( intPoint_str, "%d", labels[i] );

                            sprintf(comp_index_str, "[%d][%d]", i, comp_index );

                            es_primal_expr.SetName("Mili-Primal/"+class_name+"/"+svar_name+"/"+
                                                   "IntPoint "+string(intPoint_str)+"/"+comp_name);
                            es_primal_expr.SetDefinition("<"+expr_name+">"+string(comp_index_str));

                            es_primal_expr.SetType(Expression::ScalarMeshVar);
                            es_primal_expr.SetHidden(false);
                            md->AddExpression(&es_primal_expr);
                        }
                    }
                    free (labels);
                }
                else
                {
                    if ( comp_name!="NULL" )
                    {
                        char comp_index_str[16];
                        if ( comp_len==3 )
                        {
                            sprintf(comp_index_str, "[%d]", comp_index_x);
                        }
                        if ( comp_len==6 )
                        {
                            sprintf(comp_index_str, "[%d][%d]", comp_index_x, comp_index_y);
                        }

                        primal_expr.SetName("Mili-Primal/"+class_name+"/"+svar_name+"/"+comp_name);
                        primal_expr.SetDefinition("<"+expr_name+">"+comp_index_str);
                        if ( num_classes>1 )
                        {
                            class_name = "Shared";
                            primal_expr.SetName("Mili-Primal/"+class_name+"/"+svar_name+"/"+comp_name);
                            primal_expr.SetDefinition("<"+expr_name+">"+string(comp_index_str));
                        }
                    }
                    else
                    {
                        primal_expr.SetName("Mili-Primal/"+class_name+"/"+svar_name);
                        primal_expr.SetDefinition("<"+expr_name+">");
                        if ( num_classes>1 )
                        {
                            class_name = "Shared";
                            primal_expr.SetName("Mili-Primal/"+class_name+"/"+svar_name+"/"+comp_name);
                            primal_expr.SetDefinition("<"+expr_name+">");
                        }
                    }
                    primal_expr.SetType(Expression::ScalarMeshVar);
                    primal_expr.SetHidden(false);
                    md->AddExpression(&primal_expr);
                }

                if ( comp_index<3 )
                {
                    comp_index_x++;
                    comp_index_y++;
                }
                else
                {
                    if ( comp_index==3 )
                    {
                        comp_index_x = 0;
                        comp_index_y = 1;
                    }
                    if ( comp_index==4 )
                    {
                        comp_index_x = 0;
                        comp_index_y = 2;
                    }
                    if ( comp_index==5 )
                    {
                        comp_index_x = 1;
                        comp_index_y = 0;
                    }
                }
                comp_index++;
            }
        }
    }
}
// ****************************************************************************
//  Function: is_particle_var
//
//  Purpose:
//      Returns TRUE if this var is an particle class primal field.
//
//  Programmer: Bob Corey
//  Creation:   October 01, 2012
//
//  Modifications:
//
// ****************************************************************************

bool
avtMiliFileFormat::is_particle_var( const char *var_name )
{
    mili_primal_fields_iter = mili_primal_fields.find(var_name);
    if ( mili_primal_fields_iter == mili_primal_fields.end() )
    {
        return( false );
    }
    else

    {
        return( mili_primal_fields[var_name].SPH_var );
    }
}

// ****************************************************************************
//  Function: is_particle_class
//
//  Purpose:
//      Returns TRUE if this class is a particle class.
//
//  Programmer: Bob Corey
//  Creation:   May 08, 2012
//
//  Modifications:
//
// ****************************************************************************

bool
avtMiliFileFormat::is_particle_class( int superclass, char *class_name, bool *DBC_class )
{
    int i=0;
    char class_upper[256];

    if ( superclass==M_PARTICLE )
    {
        return( true );
    }

    if ( !class_name )
    {
        return( false );
    }

    strcpy( class_upper, class_name );
    for ( i=0;
            i<strlen(class_name);
            i++ )
    {
        class_upper[i] = toupper( class_name[i] );
    }

    if ( !strncmp( class_upper, "PARTICLE", 8 )       ||
            !strncmp( class_upper, "PARTICLE_ELEM", 16 ) ||
            !strncmp( class_upper, "NTET", 4 )           ||
            !strncmp( class_upper, "DBC", 3 ))
    {
        *DBC_class = false;
        if ( !strncmp( class_upper, "DBC", 3 ))
        {
            *DBC_class=true;
        }
        return( true );
    }
    return( false );
}

// Element Set Functions

// ****************************************************************************
//  Function: get_intpoints
//
//  Purpose:
//      Returns the current set of valid integration points for inner, middle,
//      and outer surfaces.
//
//  Programmer: Bob Corey
//  Creation:   April 3, 2013
//
//  Modifications:
//
// ****************************************************************************

bool
avtMiliFileFormat::get_intpoints( const char *es_name, int intpoints[3] )
{
    std::map<std::string, intpoints_type> ::iterator es_iter;
    es_iter = es_intpoints.find(es_name);
    if ( es_iter==es_intpoints.end() )
    {
        return ( false );   // Name not found
    }

    intpoints[0] = es_intpoints[es_name].in_mid_out_current[0];
    intpoints[1] = es_intpoints[es_name].in_mid_out_current[1];
    intpoints[2] = es_intpoints[es_name].in_mid_out_current[2];

    return ( true );
}

// ****************************************************************************
//  Function: get_es_info
//
//  Purpose:
//      Returns true if this is a valid element set var, and if true returns
//      the number of labels.
//
//  Programmer: Bob Corey
//  Creation:   April 8, 2013
//
//  Modifications:
//
// ****************************************************************************
bool
avtMiliFileFormat::get_es_info( const char *es_name, int *num_labels,
                                int **labels )
{
    std::map<std::string, intpoints_type> ::iterator es_iter;
    es_iter = es_intpoints.find(es_name);
    int i=0;
    *num_labels = 0;
    int *labels_ptr=NULL;

    if ( es_iter==es_intpoints.end() )
    {
        return ( false );   // Name not found
    }

    *num_labels = es_intpoints[es_name].num_labels;
    labels_ptr = (int *) malloc( es_intpoints[es_name].num_labels*sizeof(int) );

    // Return a copy of the labels

    for ( i=0;
            i<es_intpoints[es_name].num_labels;
            i++ )
    {
        labels_ptr[i] = es_intpoints[es_name].labels[i];
    }
    *labels = labels_ptr;

    return ( true );
}


// ****************************************************************************
//  Function: set_default_intpoints
//
//  Purpose:
//      Sets current and default int points using labels. Set a surface to -1
//      if valid integration point does not exist.
//
//
//  Programmer: Bob Corey
//  Creation:   April 8, 2013
//
//  Modifications:
//
// ****************************************************************************

bool
avtMiliFileFormat::set_default_intpoints( const char *es_name )
{
    int i=0;
    int num_labels=0;
    int calc_middle=0, middle_point=-1;
    std::map<std::string, intpoints_type> ::iterator es_iter;

    es_iter = es_intpoints.find(es_name);
    if ( es_iter==es_intpoints.end() )
    {
        return ( false );   // Name not found
    }

    es_intpoints[es_name].in_mid_out_default[0] = -1; // -1 = undefined integration point
    es_intpoints[es_name].in_mid_out_default[1] = -1;
    es_intpoints[es_name].in_mid_out_default[2] = -1;

    num_labels = es_intpoints[es_name].num_labels;

    if ( es_intpoints[es_name].labels[0] == 1 )
    {
        es_intpoints[es_name].in_mid_out_default[0] = 1;   /* Inner */
    }

    /* Calculate the MIDDLE integration point */
    calc_middle = es_intpoints[es_name].intpoints_total/2.0;
    if ( calc_middle*2 != es_intpoints[es_name].intpoints_total )   /* Odd number of points */
    {
        calc_middle++;

        /* Make sure the middle point is in the list of labels */
        for ( i=0;
                i<num_labels-1;
                i++ )
        {
            if ( es_intpoints[es_name].labels[i] == calc_middle )
            {
                middle_point = calc_middle;
            }
        }

    }
    es_intpoints[es_name].in_mid_out_default[1] = middle_point;

    if ( es_intpoints[es_name].labels[num_labels-1] == es_intpoints[es_name].intpoints_total )
    {
        es_intpoints[es_name].in_mid_out_default[2] =  es_intpoints[es_name].intpoints_total;   /* Outer */
    }

    es_intpoints[es_name].in_mid_out_current[0] = es_intpoints[es_name].in_mid_out_default[0];
    es_intpoints[es_name].in_mid_out_current[1] = es_intpoints[es_name].in_mid_out_default[1];
    es_intpoints[es_name].in_mid_out_current[2] = es_intpoints[es_name].in_mid_out_default[2];

    return ( true );
}

static void
ErrorHandler( const char *function, int line )
{
#ifdef DEBUG
    printf("\nException: Function=%s@Line=%d\n", function, line);
#endif
}
static void
MyInvalidVariableException( )
{
#ifdef DEBUG
    printf("\nException: MyInvalidVariableException\n");
#endif
}

void
toupper( string &str )
{
    //  std::transform(str.begin(), str.end(),str.begin(), ::toupper);
}

#ifdef TEST
int main(int argc, char* argv[])
{
    char * argument = NULL;
    char filename[512];

    strcpy( filename, argv[1] );
}
#endif
