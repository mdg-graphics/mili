/*
 Copyright (c) 2016, Lawrence Livermore National Security, LLC.
 Produced at the Lawrence Livermore National Laboratory. Written
 by Kevin Durrenberger: durrenberger1@llnl.gov. CODE-OCEC-16-056.
 All rights reserved.

 This file is part of Mili. For details, see <URL describing code
 and how to download source>.

 Please also read this link-- Our Notice and GNU Lesser General
 Public License.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License (as published by
 the Free Software Foundation) version 2.1 dated February 1999.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms
 and conditions of the GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

 * Internal use header file for mili database.
 */

#ifndef MILI_INTERNAL_H
#define MILI_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <dirent.h>
#else
#include <direct.h>
#include <sys/stat.h>
#endif

#include "list.h"
#include "misc.h"
#include "mili.h"
#include "gahl.h"
#include "mili_enum.h"
#include "sarray.h"
#include "parson.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*
 * Keep these current with mili.h!
 */
#define QTY_PD_ENTRY_TYPES (7)
#define WRITE_LOCK (3) /* must be unique wrt STATE_DATA, NON_STATE_DATA */
/* conn_words[] in file mesh_u.c */

#define INVALID_FAM_ID(fid) (fid < 0 || fid >= fam_array_length || fam_list[fid] == NULL)
#define CHECK_WRITE_ACCESS(fam)          \
    {                                    \
        if ( (fam)->access_mode == 'r' ) \
            return BAD_ACCESS_TYPE;      \
    }
#define ST_FILE_SUFFIX(f, i) ((i) + (f)->st_file_index_offset)
#define LOCK_FILE_SIZE (128)
#define MAX_LOCK_TRIES (100)
#define EXT_SIZE(f, t) (f->external_size[t])
#define DEFAULT_SUFFIX_WIDTH (2)
#define DONT_CARE (0)
#define DEFAULT_PARTITION_SCHEME STATE_COUNT
#define DEFAULT_STATES_PER_FILE (10000000)
#define DEFAULT_BYTES_PER_FILE (LONGLONG)(10000000)
#define ABSOLUTE_MAX_FILE_SIZE (LONGLONG)(2000000000)
#define FAM(fam_id) ((Mili_family *)fam_id)
#define ID_FAIL (-1)
#define ROUND_UP_INT(n, r) (((n) % (r)) ? (n) + ((r) - (n) % (r)) : (n))
#define ADVANCE_STRING(s, l)           \
    {                                  \
        for ( ; *(s)++ && (s) < (l); ) \
            ;                          \
    }
#define DEFAULT_HASH_TABLE_SIZE (5009)
#define SMALL_HASH_TABLE_SIZE (551)

#define MAX_LABEL_CLASSES 2024

#if defined(_WIN32) || defined(WIN32)
#define SDTLIBCC __cdecl
#else
#define SDTLIBCC
#endif

typedef struct _dump_control
{
    Bool_type raw;
    Bool_type formatted;
    Dump_verbosity brevity;
    Bool_type directory;
    Bool_type include_external;
    Bool_type include_header;
} Dump_control;

typedef struct _state_descriptor
{
    int file;
    LONGLONG offset;
    float time;
    int srec_format;
} State_descriptor;

typedef struct _state_file_descriptor
{
    int state_qty;
} State_file_descriptor;

typedef struct _int_range
{
    struct _int_range *next;
    struct _int_range *prev;
    int start;
    int stop;
} Int_range;

typedef struct _block_list
{
    int object_qty;
    int block_qty;
    Int_range *blocks;
} Block_list;

typedef struct _io_mem_buffer
{
    void *data;
    LONGLONG used;
    LONGLONG output;
    LONGLONG size;
    Block_list *invalid;
} IO_mem_buffer;

typedef struct _io_mem_store
{
    IO_mem_buffer **data_buffers;
    int type;
    LONGLONG current_index;
    LONGLONG current_output_index;
    LONGLONG traverse_index;
    LONGLONG traverse_remain;
    char *traverse_next;
} IO_mem_store;

typedef LONGLONG Dir_entry[QTY_ENTRY_FIELDS];
typedef int TempDir_entry[QTY_ENTRY_FIELDS];

typedef struct _file_dir
{
    int commit_count;
    int qty_entries;
    Dir_entry *dir_entries;
    int qty_names;
    char **names;
    IO_mem_store *name_data;
} File_dir;

typedef struct _param_ref
{
    /* char *name;     Get from hash entry key field. */
    /* int type;       Get from Dir_entry. */
    int file_index;
    int entry_index;
    int rank;
    int *dims;
    void *data;
} Param_ref;

typedef struct _svar
{
    Aggregate_type *agg_type;
    int *data_type;
    int *order;
    int *dims;
    int *list_size;
    char *name;
    char *title;
    char *list_names;
    struct _svar **svars;
    /*    int commit_num; */
} Svar;

typedef struct _translated_ref
{
    char name[64];
    int reqd_qty;
    int atom_qty;
    int offset; /* Offset from svar first atom */
    int index;  /* Which subrec svar */
    Bool_type done;
} Translated_ref;

typedef struct _buffer_queue
{
    LONGLONG buffer_size;
    int buffer_count;
    int new_count;
    void **data_buffers;
    int *state_numbers;
    int recent;
} Buffer_queue;

typedef struct _sub_srec
{
    struct _sub_srec *next;
    struct _sub_srec *prev;
    char *name;
    char *mclass;
    int organization;
    /* State variable data */
    int qty_svars;
    Svar **svars;
    /* Object data */
    int qty_id_blks;
    int *mo_id_blks;
    int *surface_variable_flag;
    int mo_qty;

    int *mo_id_blks_remapped;
    int qty_id_blks_remapped;
    int mo_qty_remapped;

    /* Subrecord data */
    LONGLONG offset;
    int *lump_atoms;      /* qty of 4- or 8-byte atoms in lump */
    LONGLONG *lump_sizes; /* external byte qty of lump */
    LONGLONG *lump_offsets;
    /* Input buffering */
    Buffer_queue *ibuffer;
    int *qty_per_proc;
} Sub_srec;

typedef struct _srec
{
    int qty_subrecs;
    Sub_srec **subrecs;
    LONGLONG size;
    Db_object_status status;
} Srec;

typedef struct _mesh_object_class_data
{
    char *long_name;
    char *short_name;
    int superclass;
    int *surface_sizes;
    Block_list *blocks;
} Mesh_object_class_data;

typedef struct _mesh_descriptor
{
    char *name;
    union /* Alternatives by mesh type */
    {
        Hash_table *umesh_data;
    } mesh_data;
} Mesh_descriptor;

/*******************************************************
 * The following structures were added January 11, 2008
 * to support new Mili reader functions.
 ******************************************************
 */

typedef float Mili_GVec2D[2];
typedef float Mili_GVec3D[3];
typedef int Mili_Int_2tuple[2];

typedef struct mili__mo_class_labels
{
    int local_id;
    int label_num;
} Mili_mo_class_labels;

typedef struct _mili_elem_data
{
    int *nodes;
    int *mat;
    int *part;
    Bool_type has_degen;
} Mili_elem_data;

typedef struct _mili_material_data
{
    int elem_qty;
    int elem_block_qty;
    Mili_Int_2tuple *elem_blocks;
    struct _mili_mo_class_data *elem_class;
    int node_block_qty;
    Mili_Int_2tuple *node_blocks;
} Mili_material_data;

typedef union Mili_object_data
{
    float *nodes;
    Mili_GVec2D *nodes2d;
    Mili_GVec3D *nodes3d;
    Mili_GVec2D *particles2d;
    Mili_GVec3D *particles3d;
    Mili_elem_data *elems;
    Mili_material_data *materials;
} Mili_object_data;

typedef struct _mili_elem_block_obj
{
    int num_blocks;
    int *block_lo;
    int *block_hi;
    float *block_bbox[2][3];
} Mili_elem_block_obj;

typedef struct _mili_mo_class_data
{
    int mesh_id;
    char *short_name;
    char *long_name;
    int superclass;
    int elem_class_index;
    int qty;
    int simple_start;
    int simple_stop;
    Mili_object_data objects;
    Mili_elem_block_obj *p_elem_block;
    Bool_type labels_found;
    Mili_mo_class_labels *labels;
    int *labels_index;
    int labels_min;
    int labels_max;
} Mili_mo_class_data;

typedef struct mili__list_head
{
    int qty;
    void *list;
} Mili_list_head;

typedef struct _Mili_mesh_data
{
    Hash_table *class_table;       /* Hash table to store classes */
    Mili_mo_class_data *node_geom; /* Direct access to "Nodal" class */
    Mili_list_head classes_by_sclass[M_QTY_SUPERCLASS];
} Mili_mesh_data;
/* Direct access by superclass */

typedef struct _label_class_list
{
    char *mclass;
    int last_matid;
} label_class_list_type;

typedef struct _Mili_analysis
{
    int db_ident;
    char *root_name;
    Mili_mesh_data *mesh_table;
    int mesh_qty;
    float *result;
    float *state_times;
} Mili_analysis;

/*
 * *                  * *
 * *  The Big Kahuna  * *
 * *                  * *
 */

typedef struct _mili_family
{
    long pid;
    int my_id;
    char *root;
    int root_len;
    char *path;
    char *file_root;
    char *aFile;
    char access_mode;
    int num_procs;
    int post_modified;
    int visit_file_on;
    JSON_Value *root_value;
    JSON_Object *root_object;
    Database_type db_type;
    int lock_file_descriptor;
    int st_suffix_width;
    Bool_type non_state_ready;
    Bool_type hide_states;
    /* Family header data */
    char *char_header;
    /* Data descriptors */
    Bool_type swap_bytes;
    Precision_limit_type precision_limit;
    File_partition_scheme partition_scheme;
    int states_per_file;
    LONGLONG bytes_per_file_limit;
    Bool_type active_family;
    /* Total states written (initialized!) this open */
    int written_st_qty;
    /* Quantity of commits performed to family */
    int commit_count;
    /* Current file(s) data */
    FILE *cur_file;
    LONGLONG cur_file_size;
    int cur_index;
    int file_count;
    LONGLONG next_free_byte;
    FILE *cur_st_file;
    char cur_st_file_mode;
    LONGLONG cur_st_file_size;
    int cur_st_index;
    int st_file_count; /* Count includes partial file in active db. */
    int st_file_index_offset;
    LONGLONG cur_st_offset;
    int file_st_qty;
    int cur_srec_id;
    int state_qty;
    short state_closed;
    short state_dirty;
    /* NEW state map file parameters */
    FILE *time_state_file;
    Bool_type write_tfile;
    char state_end_marker;
    char *time_file_name;
    State_file_descriptor *file_map;
    State_descriptor *state_map;
    /* Directory data */
    File_dir *directory;
    /* Parameter data */
    Hash_table *param_table;
    /* Mesh data */
    Mesh_type mesh_type;
    int dimensions;
    Mesh_descriptor **meshes;
    int qty_meshes;
    /* State record descriptor data */
    Srec **srecs;
    Mesh_descriptor **srec_meshes;
    int qty_srecs;
    int commit_max;
    /* State variable table and I/O stores */
    Hash_table *svar_table;
    IO_mem_store *svar_c_ios;
    IO_mem_store *svar_i_ios;
    int *svar_hdr;
    /* Subrecord table */
    Hash_table *subrec_table;
    Bool_type subrec_start_check;
    /* I/O routines for this family */
    /* For access by datatype. */
    LONGLONG (*read_funcs[QTY_PD_ENTRY_TYPES + 1])(FILE *file, void *data, LONGLONG qty);
    LONGLONG (*state_read_funcs[QTY_PD_ENTRY_TYPES + 1])(FILE *file, void *data, LONGLONG qty);
    LONGLONG (*write_funcs[QTY_PD_ENTRY_TYPES + 1])(FILE *file, void *data, LONGLONG qty);
    LONGLONG (*state_write_funcs[QTY_PD_ENTRY_TYPES + 1])(FILE *file, void *data, LONGLONG qty);
    int external_size[QTY_PD_ENTRY_TYPES + 1];
    /**/
    int external_type[QTY_PD_ENTRY_TYPES + 1];

    /***********************************
     * TI_DATA file parameters
     * Added May 20 2006: I.R. Corey
     ***********************************
     */

    /* Parameter data */
    Hash_table *ti_param_table;

    /* Various TI state flags */
    Bool_type ti_enable;
    Bool_type ti_only; /* If true,then only TI files are read and written */
    Bool_type ti_data_found;

    /* Directory data */
    File_dir *ti_directory;

    FILE *ti_cur_file;

    int ti_commit_count;
    int ti_var_flag;
    int ti_cur_index;
    LONGLONG ti_cur_file_size;
    LONGLONG ti_next_free_byte;
    int ti_cur_st_index;
    int ti_file_count; /* Count includes partial file in active db. */
    int ti_file_index_offset;
    LONGLONG ti_cur_offset;

    /* Class definition variables */
    int ti_meshid;
    int ti_matid;
    int ti_state;
    char ti_superclass_name[32];
    char ti_short_name[M_MAX_NAME_LEN];
    char ti_long_name[M_MAX_NAME_LEN];
    Bool_type ti_meshvar;
    Bool_type ti_nodal;
    label_class_list_type label_class_list[MAX_LABEL_CLASSES];
    int num_label_classes;
    label_class_list_type global_id_class_list[MAX_LABEL_CLASSES];
    int num_global_id_classes;
    Bool_type ti_search_by_metadata;

    /* Saved - Class definition variables */
    int ti_saved_meshid;
    int ti_saved_matid;
    int ti_saved_state;
    char ti_saved_superclass_name[32];
    char ti_saved_short_name[M_MAX_NAME_LEN];
    char ti_saved_long_name[M_MAX_NAME_LEN];
    Bool_type ti_saved_meshvar;
    Bool_type ti_saved_nodal;
} Mili_family;

/*
 * Library-private file family management routines and data.
 */

extern int internal_sizes[QTY_PD_ENTRY_TYPES + 1];
extern int mili_verbose;
Return_value validate_fam_id(Famid fam_id);
Return_value parse_control_string(char *ctl_str, Mili_family *fam, Bool_type *p_create);
Return_value non_state_file_open(Mili_family *fam, int index, char mode);
Return_value non_state_file_seek(Mili_family *fam, LONGLONG offset);
Return_value non_state_file_close(Mili_family *fam);
Return_value state_file_open(Mili_family *fam, int index, char mode);
Return_value state_file_close(Mili_family *fam);
void set_file_access(char, Bool_type, char *);
Return_value seek_state_file(FILE *cur_st_file, LONGLONG offset);
Return_value open_buffered(char *fname, char *mode, FILE **p_file_descr, LONGLONG *p_size);
Return_value prep_for_new_data(Mili_family *fam, int ftype);
void verbosity(int turn_on); /* TRUE to turn on. */
void test_write_lock(Mili_family *fam);
Return_value get_name_lock(Mili_family *fam, int ftype);
Return_value free_name_lock(Mili_family *fam, int ftype);
Return_value update_active_family(Mili_family *fam);
Return_value read_header(Mili_family *fam);
void cleanse(Mili_family *);
int select_fam(struct dirent *pdirent);
void free_dirent_list(int qty, struct dirent **list);
Return_value gen_next_state_loc(Mili_family *fam, int in_file, int in_state, int *p_out_file, int *p_out_state);
Bool_type match_old_control_string_format(char *control_string, char **ctl_str);
Return_value open_family(Famid fam_id);
int mc_get_next_fid(void);
Return_value className_to_classEnum(char *className, int *classEnum);
Return_value determine_map_mode(Mili_family *fam);
/*
 * File locking enable/disable functions.
 */

void mc_filelocking_disable(void);
void mc_filelocking_enable(void);

/*****************************************************************
 * Time Independent file functions:
 * Added May 2006 by I.R. Corey
 *****************************************************************
 */
Return_value ti_make_label_description(int meshid, int mat_id, char *superclass, char *short_name, char *new_name);

Return_value ti_file_open(Famid fam_id, int index, char mode);
Return_value ti_file_close(Famid fam_id);
Return_value ti_commit(Famid fam_id);
Return_value commit_ti_dir(Mili_family *fam);
void delete_ti_dir(Mili_family *fam);
Return_value add_ti_dir_entry(Mili_family *fam, Dir_entry_type etype, int modifier1, int modifier2, int string_qty,
                              char **strings, LONGLONG offset, LONGLONG length);
Return_value load_ti_directories(Mili_family *fam);

/* direc.c - directory management routines. */
Return_value add_dir_entry(Mili_family *fam, Dir_entry_type etype, int modifier1, int modifier2, int string_qty,
                           char **strings, LONGLONG offset, LONGLONG length);
Return_value commit_dir(Mili_family *fam);
void delete_dir(Mili_family *fam);
Return_value load_directories(Mili_family *fam);

/* param.c - parameter management routines. */
Return_value read_scalar(Mili_family *fam, Param_ref *p_pr, void *p_value);
Return_value mili_read_string(Mili_family *fam, Param_ref *p_pr, char *p_value);
Return_value write_string(Mili_family *fam, char *name, char *value, Dir_entry_type etype);
Return_value write_array(Mili_family *fam, int type, char *name, int order, int *dimensions, void *data,
                         Dir_entry_type etype);
Return_value read_param_array(Mili_family *fam, Param_ref *p_pr, void **p_data);
Return_value write_scalar(Mili_family *fam, int type, char *name, void *data, Dir_entry_type etype);
Return_value dump_param(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                        int head_indent, int body_indent);

/* util.c - utility routines. */
int find_proc_count(Famid fam_id);
void tab(int qty);
Return_value parse_int_list(char *list_string, int *count, int **iarray);
int str_dup(char **dest, char *src);
int str_dup_f2c(char **dest, char *src, int ftn_len);
void make_fnam(int ftype, Mili_family *fam, int fnum, char dest[]);
Return_value get_file_index(int file_type, char *fam_root, char *file_name, int *index);
char *my_calloc(int cnt, long size, char *descr);
void *my_realloc(void *ptr, long size, long add, char *descr);
void *mili_recalloc(void *ptr, long size, long add, char *descr);
void *get_write_func(Mili_family *fam, int type);
int is_numeric(char *ptest);
int is_all_upper(char *ptest);
Buffer_queue *create_buffer_queue(int buf_qty, long length);
Return_value init_buffer_queue(Buffer_queue *p_bq, int buf_qty, long length);
void delete_buffer_queue(Buffer_queue *p_bq);
Return_value mili_scandir(char *path, char *root, StringArray *p_sarr);
void swap_bytes(int qty, long field_size, void *p_source, void *p_destination);
void get_mili_version(char *mili_version_ptr);

/* dep.c - routines for handling architecture dependencies. */
Return_value set_default_io_routines(Mili_family *fam);
Return_value set_state_data_io_routines(Mili_family *fam);
extern void (*write_funcs[QTY_PD_ENTRY_TYPES + 1])();

/* svar.c - routines for managing state variables. */
int recurse_vec_arr_list_size(Svar *svar);
Bool_type valid_svar_data(Aggregate_type atype, char *name, int num_type, int vec_length, char **components, int rank,
                          int *dims);
void delete_svars(Mili_family *fam);
void delete_svar(void *data);
Return_value commit_svars(Mili_family *fam);
Return_value load_svars(Mili_family *fam);
Return_value dump_state_var_dict(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                                 int head_indent, int body_indent);
Return_value delete_svar_with_ios(Svar *psv, IO_mem_store *pcioms, IO_mem_store *piioms);
void Mset_hash_dump_(int *compress);

/* srec.c - routines for managing state record descriptors. */
Return_value update_static_map(Famid fam_id, State_descriptor *p_sd);
Return_value commit_srecs(Mili_family *fam);
void delete_srecs(Mili_family *fam);
void delete_subrec(void *ptr_subrec);
Return_value list_to_blocks(int size, int *list, int **dest, int *blk_cnt);
Return_value load_srec_formats(Mili_family *fam);
Return_value build_state_map(Mili_family *fam, Bool_type initial_build);
Return_value dump_state_rec_data(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                                 int head_indent, int body_indent);

/* mesh_u.c - routines for managing unstructured mesh geometry. */
Return_value create_class_data(int superclass, char *short_name, char *long_name, Mesh_object_class_data **pp_mocd);
Return_value check_object_ids(Block_list *obj_group, int qty_id_blks, int *obj_id_blks);
void delete_meshes(Mili_family *fam);
void delete_mo_data(void *ptr_mo_data);
Return_value insert_range(Block_list *p_bl, int start, int stop);
Return_value get_class_qty(Mili_family *fam, int *modifiers, int *class_qty);
void get_elem_conn_classes_names(Mili_family *fam, int mesh_id, int *count, char ***out_names);
int count_elem_conn_defs(Mili_family *fam, int mesh_id, char *class_name);
Return_value get_elem_qty_in_def(Mili_family *fam, int *int_args, char *class_name, int *elem_qty);
int count_node_entries(Mili_family *fam, int mesh_id, char *class_name);
Return_value get_node_block_size(Mili_family *fam, int *modifiers, char *class_name, int *blk_size);
Return_value init_meshes(Mili_family *fam);
Return_value dump_nodes(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                        int head_indent, int body_indent);
Return_value dump_elem_conns(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                             int head_indent, int body_indent);
Return_value dump_surface_conns(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                                int head_indent, int body_indent);
Return_value dump_class_idents(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                               int head_indent, int body_indent);
Return_value reset_class_data(Mili_family *fam, int mesh_id, Bool_type force_init);

/* io_mem.c - I/O memory store management routines. */
IO_mem_store *ios_create(int datatype);
IO_mem_store *ios_create_empty();
Return_value ios_input(Mili_family *fam, int datatype, LONGLONG qty_units, IO_mem_store *pioms, void **new_data);
Return_value ti_ios_input(Mili_family *fam, int datatype, LONGLONG qty_units, IO_mem_store *pioms, void **new_data);
void ios_destroy(IO_mem_store *pioms);
void *ios_alloc(LONGLONG qty, IO_mem_store *pioms);
Return_value ios_unalloc(void *pmem, int qty_units, IO_mem_store *pioms);
int ios_str_dup(char **ppcopy, char *pstring, IO_mem_store *pioms);
Return_value ios_output(Mili_family *fam, FILE *ofile, IO_mem_store *pioms, int *num_items_written);
LONGLONG ios_get_fresh(IO_mem_store *pioms);
Return_value ios_int_traverse(IO_mem_store *pioms, LONGLONG size, int **pp_data);
Return_value ios_string_traverse(IO_mem_store *pioms, char **pp_data);
Return_value ios_traverse_init(IO_mem_store *pioms, int last);
Return_value mc_get_class_info_by_index(Mili_family *in, int *mesh_id, int *index, int *superclass, char *short_name,
                                        char *long_name);
/* mili_statemap.c - routines */
Return_value load_static_maps(Mili_family *, Bool_type, Bool_type);
Return_value rebuild_state_tfile(Mili_family *);
/* read_db.c - routines for managing mesh object structs */
void mili_delete_mo_class_data(void *p_data);

/* write_db.c */
Return_value write_state_data(int state_num, Mili_analysis *out_db);
/*read_db.c */
Return_value read_state_data(int state_num, Mili_analysis *in_db);

char **get_count_elem_conn_classes_names(Mili_family *fam, int mesh_id, int *ret_count);
#endif
