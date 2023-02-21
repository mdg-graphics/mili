/*
 * Defines a simple cube.
 */

#include <stdio.h>
#include <string.h>
#include "mili_internal.h"
#include "mili.h"

#define MAX_STRING_LENGTH 1000
#define MAX_FILE_SIZE 10000000 // 10 MB
#define MAX_STATES 1000
#define TOTAL_STATES 10
#define TRUE 1
#define FALSE 0
#define MAX_RNAME_LEN (64)

const int NUM_NODES = 71;
const int NUM_NODAL_SVARS = 9;

const int NUM_BRICKS = 8;
const int NUM_BRICK_SVARS = 6;

const int NUM_PYRAMIDS = 6;
const int NUM_PYRAMID_SVARS = 2;

const int NUM_TRUSS = 6;
const int NUM_TRUSS_SVARS = 3;

const int NUM_TETS = 2;
const int NUM_TET_SVARS = 2;
const int NUM_TET_INT_POINTS = 2;

const int NUM_BEAMS = 12;
const int NUM_BEAM_SVARS = 1;

const int NUM_TRIS = 4;
const int NUM_TRI_SVARS = 1;

const int NUM_QUADS = 4;
const int NUM_QUAD_SVARS = 1;

const int NUM_WEDGES = 4;
const int NUM_WEDGE_SVARS = 1;

const int NUM_MATS = 13;

// Nodal State Variable Names
char *node_svar_names[] = {
    "nodpos", "ux", "uy", "uz",
    "nodvel", "vx", "vy", "vz",
    "nodacc", "ax", "ay", "az",
};

// Nodal State Variable Titles
char *node_svar_titles[] = {
    "Node position", "X Position", "Y Position", "Z Position",
    "Node Velocity", "X Velocity", "Y Velocity", "Z Velocity",
    "Node Acceleration", "X Accel.", "Y Accel.", "Z Accel."
};

// Hex State Variable Names
char * hex_svar_names[] = {
    "stress", "sx", "sy", "sz", "sxy", "syz", "szx"
};

// Hex State Variable Titles
char *hex_svar_titles[] = {
    "Stress", "Sigma-xx", "Sigma-yy", "Sigma-zz",
    "Sigma-xy", "Sigma-yz", "Sigma-zx"
};

// Tet Vec array state variable info
char *tet_vec_array[] = {"temp", "tetVar", "Temperature", "Tet State Variable"};

// Pyramid State Variable Info
char pyramid_svar_names[NUM_PYRAMID_SVARS][MAX_RNAME_LEN] = { "pvar1", "pvar2" };
char pyramid_svar_titles[NUM_PYRAMID_SVARS][MAX_RNAME_LEN] = { "Pyramid Variable 1", "Pyramid Variable 2"};
int pyramid_svar_types[] = { M_FLOAT, M_FLOAT };

// Truss State Variables Info
char truss_svar_names[NUM_TRUSS_SVARS][MAX_RNAME_LEN] = { "trussvar1", "trussvar2", "trussvar3"};
char truss_svar_titles[NUM_TRUSS_SVARS][MAX_RNAME_LEN] = { "Truss Variable 1", "Truss Variable 2", "Truss Variable 3"};
int truss_svar_types[] = { M_FLOAT, M_FLOAT, M_FLOAT };

// Beam State Variable Info
char beam_svar_names[NUM_BEAM_SVARS][MAX_RNAME_LEN] = { "axf" };
char beam_svar_titles[NUM_BEAM_SVARS][MAX_RNAME_LEN] = { "Axial force" };
int beam_svar_types[] = { M_FLOAT };

// Tri State Variable Info
char tri_svar_names[NUM_TRI_SVARS][MAX_RNAME_LEN] = { "triVar" };
char tri_svar_titles[NUM_TRI_SVARS][MAX_RNAME_LEN] = { "Tri State Variable" };
int tri_svar_types[] = { M_FLOAT };

// Quad State Variable Info
char quad_svar_names[NUM_QUAD_SVARS][MAX_RNAME_LEN] = { "quadVar" };
char quad_svar_titles[NUM_QUAD_SVARS][MAX_RNAME_LEN] = { "Quad State Variable" };
int quad_svar_types[] = { M_FLOAT };

// Wedge State Variable Info
char wedge_svar_names[NUM_WEDGE_SVARS][MAX_RNAME_LEN] = { "wedgeVar" };
char wedge_svar_titles[NUM_WEDGE_SVARS][MAX_RNAME_LEN] = { "Wedge State Variable" };
int wedge_svar_types[] = { M_FLOAT };

// Nodal Positions
float node_positions[] = {
    0.0, 0.0, 0.02,
    0.0, 0.0, 0.01,
    0.0, 0.01, 0.01,
    0.0, 0.01, 0.02,
    0.01, 0.0, 0.02,
    0.01, 0.0, 0.01,
    0.01, 0.01, 0.01,
    0.01, 0.01, 0.02,
    0.0, 0.0, 0.0,
    0.0, 0.01, 0.0,
    0.01, 0.0, 0.0,
    0.01, 0.01, 0.0,
    0.0, 0.02, 0.01,
    0.0, 0.02, 0.02,
    0.01, 0.02, 00.01,
    0.01, 0.02, 0.02, 
    0.0, 0.02, 0.0,
    0.01, 0.02, 0.0,
    0.02, 0.0, 0.02,
    0.02, 0.0, 0.01,
    0.02, 0.01, 0.01,
    0.02, 0.01, 0.02,
    0.02, 0.0, 0.0,
    0.02, 0.01, 0.0,
    0.02, 0.02, 0.01,
    0.02, 0.02, 0.02,
    0.02, 0.02, 0.0,
    0.01, 0.03, 0.01,
    0.01, 0.01, 0.03,
    -0.01, 0.01, 0.01,
    0.01, -0.01, 0.01,
    0.03, 0.01, 0.01,
    0.01, 0.01, -0.01,
    0.01, 0.05, 0.01,
    0.01, 0.01, 0.05,
    -0.03, 0.01, 0.01,
    0.01, -0.03, 0.01,
    0.05, 0.01, 0.01,
    0.01, 0.01, -0.03,
    0.0, -0.04, 0.02,
    0.0, -0.04, 0.0,
    0.02, -0.04, 0.02,
    0.02, -0.04, 0.00, 
    0.02, 0.01, 0.06,
    0.00, 0.01, 0.06, 
    -0.04, 0.01, 0.0,
    -0.04, 0.01, 0.02,
    0.06, 0.01, 0.02,
    0.06, 0.01, 0.0,
    0.02, 0.01, -0.04,
    0.0, 0.01, -0.04,
    0.02, 0.01, 0.08,
    0.0, 0.01, 0.08,
    -0.06, 0.01, 0.0,
    -0.06, 0.01, 0.02,
    0.08, 0.01, 0.02,
    0.08, 0.01, 0.0,
    0.0, 0.01, -0.06,
    0.02, 0.01, -0.06,
    -0.04, -0.04, 0.02,
    -0.04, -0.04, 0.0,
    -0.03, -0.04, 0.01,
    0.02, -0.04, 0.06,
    0.0, -0.04, 0.06,
    0.01, -0.04, 0.05,
    0.06, -0.04, 0.0,
    0.06, -0.04, 0.02,
    0.05, -0.04, 0.01,
    0.0, -0.04, -0.04,
    0.02, -0.04, -0.04,
    0.01, -0.04, -0.03
};

int node_labels[] = {
    1,2,3,4,5,6,7,8,9,10,
    11,12,13,14,15,16,17,18,19,20,
    21,22,23,24,25,26,27,28,29,30,
    31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,
    51,52,53,54,55,56,57,58,59,60,
    61,62,63,64,65,66,67,68,69,70,
    71
};


int bricks[][10] = {
    {1, 2, 3, 4, 5, 6, 7, 8, 1, 1},
    {2, 9, 10, 3, 6, 11, 12, 7, 2, 1},
    {4, 3, 13, 14, 8, 7, 15, 16, 3, 1},
    {3, 10, 17, 13, 7, 12, 18, 15, 4, 1},
    {5, 6, 7, 8, 19, 20, 21, 22, 1, 1},
    {6, 11, 12, 7, 20, 23, 24, 21, 2, 1},
    {8, 7, 15, 16, 22, 21, 25, 26, 3, 1},
    {7, 12, 18, 15, 21, 24, 27, 25, 4, 1}
};

int brick_labels[] = {
    1,2,3,4,5,6,7,8
};

int pyramids[][7] = {
    17, 14, 26, 27, 28, 7, 1,
    1, 19, 26, 14, 29, 6, 1,
    9, 1, 14, 17, 30, 5, 1,
    9, 23, 19, 1, 31, 7, 1,
    23, 27, 26, 19, 32, 5, 1,
    9, 17, 27, 23, 33, 6, 1
};

int pyramid_labels[] = {
    1,2,3,4,5,6
};

int trusses[][4] = {
    28, 34, 8, 1,
    29, 35, 8, 1,
    30, 36, 8, 1,
    31, 37, 8, 1,
    32, 38, 8, 1,
    33, 39, 8, 1
};

int truss_labels[] = {
    10, 20, 30, 40, 50, 60
};

int tets[][6] = { 
    40, 41, 43, 37, 9, 1,
    40, 42, 43, 37, 9, 1
};
int tet_labels[] = {
    88, 98
};

int beams[][5] = {
    40, 1, 37, 10, 1,
    42, 19, 37, 10, 1,
    43, 23, 37, 10, 1,
    41, 9, 37, 10, 1,
    36, 35, 7, 10, 1,
    35, 38, 7, 10, 1,
    38, 39, 7, 10, 1,
    39, 36, 7, 10, 1,
    35, 34, 7, 10, 1,
    36, 34, 7, 10, 1,
    38, 34, 7, 10, 1,
    39, 34, 7, 10, 1
};

int beam_labels[] = {
    2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24
};

int tris[][5] = {
    44, 45, 35, 11, 1,
    46, 47, 36, 11, 1,
    48, 49, 38, 11, 1,
    50, 51, 39, 11, 1
};

int tri_labels[] = {
    42, 72, 102, 132
};

int quads[][6] = {
    44, 45, 53, 52, 12, 1,
    46, 47, 55, 54, 12, 1,
    48, 49, 57, 56, 12, 1,
    50, 51, 58, 59, 12, 1
};

int quad_labels[] = {
    5, 10, 15, 20
};

int wedges[][8] = {
    47, 60, 61, 46, 36, 62, 13, 1,
    44, 63, 64, 45, 35, 65, 13, 1,
    49, 66, 67, 48, 38, 68, 13, 1,
    51, 69, 70, 50, 39, 71, 13, 1
};

int wedge_labels[] = {
    11,12,13,14
};


float state_data[] = {
//Node Positions
    0.0, 0.0, 0.02,
    0.0, 0.0, 0.01,
    0.0, 0.01, 0.01,
    0.0, 0.01, 0.02,
    0.01, 0.0, 0.02,
    0.01, 0.0, 0.01,
    0.01, 0.01, 0.01,
    0.01, 0.01, 0.02,
    0.0, 0.0, 0.0,
    0.0, 0.01, 0.0,
    0.01, 0.0, 0.0,
    0.01, 0.01, 0.0,
    0.0, 0.02, 0.01,
    0.0, 0.02, 0.02,
    0.01, 0.02, 00.01,
    0.01, 0.02, 0.02, 
    0.0, 0.02, 0.0,
    0.01, 0.02, 0.0,
    0.02, 0.0, 0.02,
    0.02, 0.0, 0.01,
    0.02, 0.01, 0.01,
    0.02, 0.01, 0.02,
    0.02, 0.0, 0.0,
    0.02, 0.01, 0.0,
    0.02, 0.02, 0.01,
    0.02, 0.02, 0.02,
    0.02, 0.02, 0.0,
    0.01, 0.03, 0.01,
    0.01, 0.01, 0.03,
    -0.01, 0.01, 0.01,
    0.01, -0.01, 0.01,
    0.03, 0.01, 0.01,
    0.01, 0.01, -0.01,
    0.01, 0.05, 0.01,
    0.01, 0.01, 0.05,
    -0.03, 0.01, 0.01,
    0.01, -0.03, 0.01,
    0.05, 0.01, 0.01,
    0.01, 0.01, -0.03,
    0.0, -0.04, 0.02,
    0.0, -0.04, 0.0,
    0.02, -0.04, 0.02,
    0.02, -0.04, 0.00, 
    0.02, 0.01, 0.06,
    0.00, 0.01, 0.06, 
    -0.04, 0.01, 0.0,
    -0.04, 0.01, 0.02,
    0.06, 0.01, 0.02,
    0.06, 0.01, 0.0,
    0.02, 0.01, -0.04,
    0.0, 0.01, -0.04,
    0.02, 0.01, 0.08,
    0.0, 0.01, 0.08,
    -0.06, 0.01, 0.0,
    -0.06, 0.01, 0.02,
    0.08, 0.01, 0.02,
    0.08, 0.01, 0.0,
    0.0, 0.01, -0.06,
    0.02, 0.01, -0.06,
    -0.04, -0.04, 0.02,
    -0.04, -0.04, 0.0,
    -0.03, -0.04, 0.01,
    0.02, -0.04, 0.06,
    0.0, -0.04, 0.06,
    0.01, -0.04, 0.05,
    0.06, -0.04, 0.0,
    0.06, -0.04, 0.02,
    0.05, -0.04, 0.01,
    0.0, -0.04, -0.04,
    0.02, -0.04, -0.04,
    0.01, -0.04, -0.03,
//Nodal velocities
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
//Nodal Accelerations
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
//Hex Stress values
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
// Pyramid State Variables
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
// Truss State variables
    2.0, 2.0, 2.0, 3.0, 4.0, 5.0,
    2.0, 2.0, 2.0, 3.0, 4.0, 5.0,
    2.0, 2.0, 2.0, 3.0, 4.0, 5.0,
// Tet vec array values
    10.0, 100.0, 20.0, 200.0, 30.0, 300.0, 40.0, 400.0,
// Beam axial stress values
   3.4, 3.4, 3.4, 3.4, 3.4, 3.4, 3.4, 3.4, 3.4, 3.4, 3.4, 3.4, 
// Tri variables
    10.0, 10.0, 10.0, 10.0,
// Quad variables
    20.0, 20.0, 20.0, 20.0,
// Wedge variables
    30.0, 30.0, 30.0, 30.0
};

void define_all_state_variables(Famid);
void define_all_mesh_object_classes(Famid, int);
void write_states(Famid, int, int, float, float);
void define_subrecords(Famid, int);

void standard_error_check(Famid fam_id, int stat, char *msg) {
    if(stat != 0) {
        mc_close(fam_id);
        mc_print_error(msg, stat);
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    int stat;
    int mesh_id, srec_id;
    int mo_ids[2];
    Famid fam_id;
    char database_name[MAX_STRING_LENGTH];
    strcpy(database_name, "mixdb_wrt_stream.plt");

    // Create the Mili database.
    stat = mc_open(database_name, ".", "AwPdEn", &fam_id);
    if(stat != 0) {
        mc_print_error("mc_open", stat);
        exit(1);
    }

    mc_set_state_map_file_on(fam_id, 1);
    mc_limit_filesize(fam_id, MAX_FILE_SIZE);
    mc_limit_states(fam_id, MAX_STATES);
    mc_activate_visit_file(fam_id, TRUE);

    // Write out database title
    stat = mc_wrt_string(fam_id, "title", "Mixdb Test Using mc_wrt_stream");
    standard_error_check(fam_id, stat, "mc_wrt_scalar (title)");

    // Define State Variables
    define_all_state_variables(fam_id);

    // Create a mesh.
    stat = mc_make_umesh(fam_id, "mixdb_wrt_stream", 3, &mesh_id);
    standard_error_check(fam_id, stat, "mc_make_umesh");

    // Define Mesh object classes 
    define_all_mesh_object_classes(fam_id, mesh_id);

    // Create state record format descriptor
    stat = mc_open_srec(fam_id, mesh_id, &srec_id);
    standard_error_check(fam_id, stat, "mc_open_srec");

    define_subrecords(fam_id, srec_id);

    stat = mc_close_srec(fam_id, srec_id);
    standard_error_check(fam_id, stat, "mc_close_srec");

    stat = mc_flush(fam_id, NON_STATE_DATA);
    standard_error_check(fam_id, stat, "mc_flush");

    float start_time = 0.0;
    float time_increment = 0.1;
    int state_count = 10;
    write_states(fam_id, srec_id, state_count, start_time, time_increment);
    
    stat = mc_close(fam_id); 
    standard_error_check(fam_id, stat, "mc_close");

    return 0;
}


void define_all_state_variables(Famid fam_id) {
    int stat;
    // Define nodal position vector consisting of "ux", "uy", "uz"
    stat = mc_def_vec_svar(fam_id, M_FLOAT, 3, node_svar_names[0], node_svar_titles[0],
                            node_svar_names+1, node_svar_titles+1);
    standard_error_check(fam_id, stat, "mc_def_vec_svar (nodpos)");

    // Define nodal velocity vector consisting of "vx", "vy", "vz"
    stat = mc_def_vec_svar(fam_id, M_FLOAT, 3, node_svar_names[4], node_svar_titles[4],
                           node_svar_names+5, node_svar_titles+5);
    standard_error_check(fam_id, stat, "mc_def_vec_svar (nodvel)");

    // Define nodal acceleration vector consisting of "ax", "ay", "az"
    stat = mc_def_vec_svar(fam_id, M_FLOAT, 3, node_svar_names[8], node_svar_titles[8],
                           node_svar_names+9, node_svar_titles+9);
    standard_error_check(fam_id, stat, "mc_def_vec_svar (nodacc)");

    //Define Hex stress vector
    stat = mc_def_vec_svar(fam_id, M_FLOAT, 6, hex_svar_names[0], hex_svar_titles[0],
                           hex_svar_names+1, hex_svar_titles+1);
    standard_error_check(fam_id, stat, "mc_def_vec_svar (stress)");

    //Define Tet vector array
    int dims = 2;
    stat = mc_def_vec_arr_svar(fam_id, 1, &dims, "es_1a", "Element Set 1a", 2, tet_vec_array, tet_vec_array+2, M_FLOAT);
    standard_error_check(fam_id, stat, "mc_def_vec_arr_svar (es_1a)");

    // Define Pyramid state variables
    int qty = sizeof(pyramid_svar_names) / sizeof(pyramid_svar_names[0]);
    stat = mc_def_svars(fam_id, qty, (char*) pyramid_svar_names, MAX_RNAME_LEN, (char*) pyramid_svar_titles, MAX_RNAME_LEN, pyramid_svar_types);
    standard_error_check(fam_id, stat, "mc_def_svars (pyramid)");

    // Define Truss state variables
    qty = sizeof(truss_svar_names) / sizeof(truss_svar_names[0]);
    stat = mc_def_svars(fam_id, qty, (char*) truss_svar_names, MAX_RNAME_LEN, (char*) truss_svar_titles, MAX_RNAME_LEN, truss_svar_types);
    standard_error_check(fam_id, stat, "mc_def_svars (truss)");

    // Define Beam state variables
    qty = sizeof(beam_svar_names) / sizeof(beam_svar_names[0]);
    stat = mc_def_svars(fam_id, qty, (char*) beam_svar_names, MAX_RNAME_LEN, (char*) beam_svar_titles, MAX_RNAME_LEN, beam_svar_types);
    standard_error_check(fam_id, stat, "mc_def_svars (beam)");

    // Define Tri state variables
    qty = sizeof(tri_svar_names) / sizeof(tri_svar_names[0]);
    stat = mc_def_svars(fam_id, qty, (char*) tri_svar_names, MAX_RNAME_LEN, (char*) tri_svar_titles, MAX_RNAME_LEN, tri_svar_types);
    standard_error_check(fam_id, stat, "mc_def_svars (tri)");

    // Define Quad state variables
    qty = sizeof(quad_svar_names) / sizeof(quad_svar_names[0]);
    stat = mc_def_svars(fam_id, qty, (char*) quad_svar_names, MAX_RNAME_LEN, (char*) quad_svar_titles, MAX_RNAME_LEN, quad_svar_types);
    standard_error_check(fam_id, stat, "mc_def_svars (quad)");

    // Define Wedge state variables
    qty = sizeof(wedge_svar_names) / sizeof(wedge_svar_names[0]);
    stat = mc_def_svars(fam_id, qty, (char*) wedge_svar_names, MAX_RNAME_LEN, (char*) wedge_svar_titles, MAX_RNAME_LEN, wedge_svar_types);
    standard_error_check(fam_id, stat, "mc_def_svars (wedge)");
}


void define_all_mesh_object_classes(Famid fam_id, int mesh_id) {
    int stat;
    // Define mesh class
    stat = mc_def_class(fam_id, mesh_id, M_MESH, "mesh", "Mesh");
    standard_error_check(fam_id, stat, "mc_def_class (M_MESH)");

    // Populate mesh class
    stat = mc_def_class_idents(fam_id, mesh_id, "mesh", 1, 1);
    standard_error_check(fam_id, stat, "mc_def_class_idents (mesh)");

    // Create and populate a material class
    stat = mc_def_class(fam_id, mesh_id, M_MAT, "mat", "Material");
    standard_error_check(fam_id, stat, "mc_def_class (mat)");

    stat = mc_def_class_idents(fam_id, mesh_id, "mat", 1, NUM_MATS);
    standard_error_check(fam_id, stat, "mc_def_class_idents (mat)");

    // Define and populate Node Class
    stat = mc_def_class(fam_id, mesh_id, M_NODE, "node", "Nodal");
    standard_error_check(fam_id, stat, "mc_def_class (node)");

    stat = mc_def_nodes(fam_id, mesh_id, "node", 1, NUM_NODES, (float*) node_positions);
    standard_error_check(fam_id, stat, "mc_def_nodes");

    stat = mc_def_node_labels(fam_id, mesh_id, "node", NUM_NODES, (int*) node_labels);
    standard_error_check(fam_id, stat, "mc_def_node_labels");

    // Define and populate a hex class.
    stat = mc_def_class(fam_id, mesh_id, M_HEX, "brick", "Brick");
    standard_error_check(fam_id, stat, "mc_def_class (M_HEX)");
    
    stat = mc_def_conn_seq_labels(fam_id, mesh_id, "brick", 1, NUM_BRICKS, brick_labels, (int *) bricks);
    standard_error_check(fam_id, stat, "mc_def_conn_seq_labels (brick)");

    // Define and populate a pyramid class
    stat = mc_def_class(fam_id, mesh_id, M_PYRAMID, "pyramid", "Pyramid");
    standard_error_check(fam_id, stat, "mc_def_class (M_PYRAMID)");

    stat = mc_def_conn_seq_labels(fam_id, mesh_id, "pyramid", 1, NUM_PYRAMIDS, pyramid_labels, (int *) pyramids);
    standard_error_check(fam_id, stat, "mc_def_conn_seq_labels (pyramid)");

    //Define and populate a truss class
    stat = mc_def_class(fam_id, mesh_id, M_TRUSS, "truss", "Truss");
    standard_error_check(fam_id, stat, "mc_def_class (M_TRUSS)");

    stat = mc_def_conn_seq_labels(fam_id, mesh_id, "truss", 1, NUM_TRUSS, truss_labels, (int *) trusses);
    standard_error_check(fam_id, stat, "mc_def_conn_seq_labels (truss)");

    // Define and populate a tet class
    stat = mc_def_class(fam_id, mesh_id, M_TET, "tet", "Tet");
    standard_error_check(fam_id, stat, "mc_def_class (M_TET)");

    stat = mc_def_conn_seq_labels(fam_id, mesh_id, "tet", 1, NUM_TETS, tet_labels, (int *) tets);
    standard_error_check(fam_id, stat, "mc_def_conn_seq_labels (tet)");

    // Define and populate a beam class
    stat = mc_def_class(fam_id, mesh_id, M_BEAM, "beam", "Beam");
    standard_error_check(fam_id, stat, "mc_def_class (M_BEAM)");

    stat = mc_def_conn_seq_labels(fam_id, mesh_id, "beam", 1, NUM_BEAMS, beam_labels, (int *) beams);
    standard_error_check(fam_id, stat, "mc_def_conn_seq_labels (beam)");

    // Define and populate a tri class
    stat = mc_def_class(fam_id, mesh_id, M_TRI, "tri", "Tri");
    standard_error_check(fam_id, stat, "mc_def_class (M_TRI)");

    stat = mc_def_conn_seq_labels(fam_id, mesh_id, "tri", 1, NUM_TRIS, tri_labels, (int *) tris);
    standard_error_check(fam_id, stat, "mc_def_conn_seq_labels (tri)");

    // Define and populate a quad class
    stat = mc_def_class(fam_id, mesh_id, M_QUAD, "quad", "Quad");
    standard_error_check(fam_id, stat, "mc_def_class (M_QUAD)");

    stat = mc_def_conn_seq_labels(fam_id, mesh_id, "quad", 1, NUM_QUADS, quad_labels, (int *) quads);
    standard_error_check(fam_id, stat, "mc_def_conn_seq_labels (quad)");

    // Define and populate a wedge class
    stat = mc_def_class(fam_id, mesh_id, M_WEDGE, "wedge", "Wedge");
    standard_error_check(fam_id, stat, "mc_def_class (M_WEDGE)");

    stat = mc_def_conn_seq_labels(fam_id, mesh_id, "wedge", 1, NUM_WEDGES, wedge_labels, (int *) wedges);
    standard_error_check(fam_id, stat, "mc_def_conn_seq_labels (wedge)");
}


void write_states(Famid fam_id, int srec_id, int state_count, float start_time, float time_increment) {
    int stat;
    double time = 0.0;
    int file_suffix, state_index;
    int i,j;
    int offset;

    int state_data_size = (NUM_NODES * NUM_NODAL_SVARS)
                          + (NUM_BRICKS * NUM_BRICK_SVARS)
                          + (NUM_PYRAMIDS * NUM_PYRAMID_SVARS)
                          + (NUM_TRUSS * NUM_TRUSS_SVARS)
                          + (NUM_TETS * NUM_TET_SVARS * NUM_TET_INT_POINTS)
                          + (NUM_BEAMS * NUM_BEAM_SVARS)
                          + (NUM_TRIS * NUM_TRI_SVARS)
                          + (NUM_QUADS * NUM_QUAD_SVARS)
                          + (NUM_WEDGES * NUM_WEDGE_SVARS);
    // Start indexes for each element in state_data array
    int start_hex_elems = (NUM_NODES * NUM_NODAL_SVARS);
    int start_pyramid_elems = start_hex_elems + (NUM_BRICKS * NUM_BRICK_SVARS);
    int start_truss_elems = start_pyramid_elems + (NUM_PYRAMIDS * NUM_PYRAMID_SVARS);
    int start_tet_elems = start_truss_elems + (NUM_TRUSS * NUM_TRUSS_SVARS);
    int start_beam_elems = start_tet_elems + (NUM_TETS * NUM_TET_SVARS * NUM_TET_INT_POINTS);
    int start_tri_elems = start_beam_elems + (NUM_BEAMS * NUM_BEAM_SVARS);
    int start_quad_elems = start_tri_elems + (NUM_TRIS * NUM_TRI_SVARS);
    int start_wedge_elems = start_quad_elems + (NUM_QUADS * NUM_QUAD_SVARS);

    for(i = 0; i < state_count; i++) {
        time = start_time + (i * time_increment);
        //Write State data
        stat = mc_new_state(fam_id, srec_id, time, &file_suffix, &state_index);
        standard_error_check(fam_id, stat, "mc_new_state");

        stat = mc_wrt_stream(fam_id, M_FLOAT, state_data_size, state_data);

        stat = mc_end_state(fam_id, srec_id);
        standard_error_check(fam_id, stat, "mc_end_state");

        //Increment state variable data so that it does not remain constant
        float data_increment = 0.01;
        for(j = start_hex_elems; j < state_data_size; j++) {
            state_data[j] += data_increment;
        }
    }    
}


void define_subrecords(Famid fam_id, int srec_id) {
    int stat;
    int mo_ids[2];

    mo_ids[0] = 1;
    mo_ids[1] = NUM_NODES;

    // Define Subrecord for Node Positions
    stat = mc_def_subrec(fam_id, srec_id, "NodePosSubrec", OBJECT_ORDERED, 1, node_svar_names[0], MAX_RNAME_LEN,
                         "node", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (NodePosSubrec)");

    // Define Subrecord for Node Velocities
    stat = mc_def_subrec(fam_id, srec_id, "NodeVelSubrec", OBJECT_ORDERED, 1, node_svar_names[4], MAX_RNAME_LEN,
                         "node", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (NodeVelSubrec)");

    // Define Subrecord for Node Accelerations
    stat = mc_def_subrec(fam_id, srec_id, "NodeAccSubrec", OBJECT_ORDERED, 1, node_svar_names[8], MAX_RNAME_LEN,
                         "node", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (NodeAccSubrec)");

    mo_ids[0] = 1;
    mo_ids[1] = NUM_BRICKS;
    // Define Subrecord for Hex Stresses
    stat = mc_def_subrec(fam_id, srec_id, "HexStressSubrec", OBJECT_ORDERED, 1, hex_svar_names[0], MAX_RNAME_LEN,
                         "brick", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (HexStressSubrec)");

    mo_ids[0] = 1;
    mo_ids[1] = NUM_PYRAMIDS;
    //Define Subrecord for Pyramid variable 1 and 2
    stat = mc_def_subrec(fam_id, srec_id, "PyramidVar1Subrec", OBJECT_ORDERED, 1, pyramid_svar_names[0], MAX_RNAME_LEN,
                         "pyramid", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (PyramidVar1Subrec)");

    stat = mc_def_subrec(fam_id, srec_id, "PyramidVar2Subrec", OBJECT_ORDERED, 1, pyramid_svar_names[1], MAX_RNAME_LEN,
                         "pyramid", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (PyramidVar2Subrec)");

    mo_ids[0] = 1;
    mo_ids[1] = NUM_TRUSS;
    //Define Subrecord for Truss variables 1-3 
    stat = mc_def_subrec(fam_id, srec_id, "TrussVar1Subrec", OBJECT_ORDERED, 1, truss_svar_names[0], MAX_RNAME_LEN,
                         "truss", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (TrussVar1Subrec)");

    stat = mc_def_subrec(fam_id, srec_id, "TrussVar2Subrec", OBJECT_ORDERED, 1, truss_svar_names[1], MAX_RNAME_LEN,
                         "truss", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (TrussVar2Subrec)");

    stat = mc_def_subrec(fam_id, srec_id, "TrussVar3Subrec", OBJECT_ORDERED, 1, truss_svar_names[2], MAX_RNAME_LEN,
                         "truss", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (TrussVar3Subrec)");

    mo_ids[0] = 1;
    mo_ids[1] = NUM_TETS;
    // Define Subrecord for TET vec array
    stat = mc_def_subrec(fam_id, srec_id, "TetVecArrSubrec", OBJECT_ORDERED, 1, "es_1a", MAX_RNAME_LEN,
                         "tet", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (TetVecArraySubrec)");

    char * elem_set_name = "IntLabel_es_1";
    int int_points[] = {1, 2, 2};
    int int_point_dims = 3;
    stat = mc_ti_wrt_array(fam_id, M_INT, elem_set_name, 1, &int_point_dims, int_points);
    standard_error_check(fam_id, stat, "mc_ti_wrt_array (IntLabel_es_1)");


    mo_ids[0] = 1;
    mo_ids[1] = NUM_BEAMS;
    //Define Subrecord for beam axf 
    stat = mc_def_subrec(fam_id, srec_id, "BeamAxfSubrec", OBJECT_ORDERED, 1, beam_svar_names[0], MAX_RNAME_LEN,
                         "beam", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (BeamAxfSubrec)");

    mo_ids[0] = 1;
    mo_ids[1] = NUM_TRIS;
    //Define Subrecord for Tri state variable 
    stat = mc_def_subrec(fam_id, srec_id, "TriVarSubrec", OBJECT_ORDERED, 1, tri_svar_names[0], MAX_RNAME_LEN,
                         "tri", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (TriVarSubrec)");

    mo_ids[0] = 1;
    mo_ids[1] = NUM_QUADS;
    //Define Subrecord for Quad state variable 
    stat = mc_def_subrec(fam_id, srec_id, "QuadVarSubrec", OBJECT_ORDERED, 1, quad_svar_names[0], MAX_RNAME_LEN,
                         "quad", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (QuadVarSubrec)");

    mo_ids[0] = 1;
    mo_ids[1] = NUM_WEDGES;
    //Define Subrecord for Wedge state variable 
    stat = mc_def_subrec(fam_id, srec_id, "WedgeVarSubrec", OBJECT_ORDERED, 1, wedge_svar_names[0], MAX_RNAME_LEN,
                         "wedge", M_BLOCK_OBJ_FMT, 1, mo_ids, 0);
    standard_error_check(fam_id, stat, "mc_def_subrec (WedgeVarSubrec)");
}
