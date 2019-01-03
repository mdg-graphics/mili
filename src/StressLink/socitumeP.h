/* Private internal header */

#ifndef __socitumep_h__
#define __socitumep_h__

/* Used for defining Axis/Directions */
#define X 0
#define Y 1
#define Z 2

#define OK     0
#define NOT_OK 1

#define FALSE   0
#define TRUE    1

#define OFF 0
#define ON  1

#define MAX_WARNINGS 10

typedef enum element_type_e
{
   SOLID,  /* Hex */
   BEAM,   /* Beam */
   SHELL,
   THICK_SHELL,
   DISCRETE,
   DELAM,
   PARTICLE
} element_type;


/******************
 * Stresslink Data
 ******************/
typedef struct _stresslink_data stresslink_data;

typedef struct _stresslink_node_set sl_node_set;

struct _stresslink_node_set
{
   int    num_nodes;
   int    len_control;
   int   *control;
   int    *labels;            /* Allocated to size of num_nodes */
   float *init_coord;         /* X, Y, Z  - allocated to size of num_nodes*3  - also known as x0-y0/z0 */
   float *cur_coord;          /* X, Y, Z  - allocated to size of num_nodes*3  - also known as xd-yd/zd */

   /* Nodal Variables */
   float *t_disp;            /* Translational displacement  - size is num_nodes*3 */
   float *r_disp;            /* Rotational displacement     - size is num_nodes*3 */
   float *t_vel;             /* Translational velocity      - size is num_nodes*3 */
   float *r_vel;             /* Rotational velocity         - size is num_nodes */

   /* Acceleration not currently used - to be added later ? */
   float *t_acc;             /* Translational acceleration  - size is num_nodes*3 */
   float *r_acc;             /* Rotational acceleration     - size is num_nodes*3 */
};

typedef struct _stresslink_element_set sl_element_set;

struct _stresslink_element_set
{
   element_type type;
   int    len_control;
   int    nnodes, nips, nhist;
   int    *control;
   int    num_elements;
   int    mat_model;

   int   *labels;
   int   *conn;               /* 1-num_nodes, 1-num_elements */

   float *vol_ref;            /* 1-nips, 1-num_elements */
   float *hist_var;           /* 1-num_hist_vars, 1-nips, 1-num_elements */

   /* History variables - broken out*/
   float *pressure;           /* 1-nips, 1-num_elements */
   float *sxx, *syy, *szz;    /* 1-nips, 1-num_elements */
   float *sxy, *syz, *szx;    /* 1-nips, 1-num_elements */
   float *v;                  /* 1-nips, 1-num_elements */
   /* Mat Model 11 History Variables */
   float        *mat11_eps, *mat11_E, *mat11_Q, *mat11_vol, *mat11_Pev, *mat11_eos;
   /* Hist Var=     7         8         9          10           11          12     */

   /* Variables translated from Ale3D */
   float *rhon;
};

typedef struct _stresslink_slide_surface_set sl_slide_set;
struct _stresslink_slide_surface
{
   int    len_control, len_variables;
   int    *control;
   int    *variables;
};


/******************************************************
/* This is the structure that contains all of the data
 * required to write out a stresslink file
 ******************************************************
 */

struct _stresslink_data
{
   int   *control;
   float *state_data;

   /* Nodal Data */
   int num_node_sets;
   sl_node_set *node_sets;

   /* Element Data */
   int            num_element_sets;
   sl_element_set *element_sets;

   /* Slide Surface Data: TBD */
   int num_slide_sets;
   sl_slide_set *slide_sets;
};


/****************
 * Overlink Data
 ****************/
typedef struct _overlink_data        overlink_data;
typedef struct _overlink_node_set    ol_node_set;
typedef struct _overlink_element_set ol_element_set;
typedef struct _overlink_var         ol_var;
typedef struct _overlink_region      ol_reg;

/* Field Variable */
struct _overlink_var
{
   char* varName;
   int annotation_flag;
   int datatype;
   int len_val;
   double* val;
   int mixlen;
   double* mixvals;
};

#if 0
/* Material */
struct _overlink_region
{
   int     regionId;
   char*   regionName;
   int     numPureElem;
   int*    pureElems;   /* numPureElem ints */
   int     numMixElem;
   int*    mixElems;    /* numMixElems ints */
   double* mixElemVF;   /* numMixElems floats */
};
#endif


/* node set */
struct _overlink_node_set
{
   int     num_nodes;
   int     numVars; /* number nodal variables */
   int    *labels;            /* Allocated to size of num_nodes */
   double *cur_coord[3];      /* X, Y, Z  - allocated to size of num_nodes*3 */

   ol_var* varSet;            /* set of numVars */
};

/* element set */
struct _overlink_element_set
{
   element_type type;
   int    nnodes, nips, nhist;
   int     num_elements;

   int     numRegs;      /* number of regions (materials) */
   int*    regionId;     /* numRegs array with region ids */
   int*    matList;
   int     numMixEntries;
   double* mixVF;
   int*    mixNext;
   int*    mixMat;
   int*    mixZone;

   int    *conn;         /* 1-num_nodes, 1-num_elements */
   float  *hist_var;     /* 1-num_hist_vars, 1-nips, 1-num_elements */

   int     numVars;      /*number of zonal variables*/
   ol_var* varSet ;      /* set of numVars */
};

/* This is the structures that contains all of the
 * data required to write out an overlink file */
struct _overlink_data
{
   int numVars;
   char** varNames;

   /* Nodal Data */
   int            num_node_sets;
   ol_node_set    *node_sets;

   /* Element Data */
   int            num_element_sets;
   ol_element_set *element_sets;
};


/* Standard error codes */
typedef enum soc_errorCode_e
{
   SOC_ERR_OPEN_OVL,   /* Couldn't open overlink file */
   SOC_ERR_OPEN_STR,   /* Couldn't open stresslink file */
   SOC_ERR_WRITE_OVL,  /* Couldn't write overlink file */
   SOC_ERR_WRITE_STR,  /* Couldn't write stresslink file */
} soc_err;


/* Internal prototypes */
overlink_data* read_overlink(const char *olinkfilename,
                             int *errorCode);
stresslink_data* read_stresslink(const char *slinkfilename,
                                 int *errorCode);

void write_stresslink(stresslink_data* slinkd,
                      const char *slinkfilename,
                      int nummats, int *ignore_list,
                      int *errorCode);

void write_overlink(overlink_data* olinkd,
                    const char *olinkfilename,
                    int nummats, int *ignore_list,
                    int *errorCode);

stresslink_data* stresslink_data_new( int num_node_sets, int num_element_sets );
void             stresslink_data_destory( stresslink_data* );

overlink_data*   overlink_data_new();
void             overlink_data_destroy( overlink_data * );

void soc_errormsg(const soc_err errnum,
                  char errmsg[]);


#endif
