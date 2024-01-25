/* $Id: md.c,v 1.26 2011/05/26 19:41:55 durrenberger1 Exp $ */

/*
 * md - Mili dump utility.  Md unpacks and writes as text the
 *      descriptive information and data found in the non-state
 *      data file(s).
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
 *                with Visit.
 *                See SCR #587.
 ************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include "mili_internal.h"
#include "eprtf.h"

#define DIR_ENTRY_COLUMN_SPACING 8

static void clean_up_and_go(Return_value rval, Mili_family *fam);
static void usage();
static Return_value dump_family(Mili_family *fam, Dump_control *p_dc);
static Return_value dump_header(Mili_family *p_fam, Dump_control *p_dc);
static Return_value dump_directory(File_dir *p_fd);

static Return_value (*dump_funcs[QTY_DIR_ENTRY_TYPES])() = {
    dump_nodes, dump_elem_conns,   dump_class_idents, dump_state_var_dict, dump_state_rec_data, dump_param, dump_param,
    NULL,       dump_surface_conns};

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

/*****************************************************************
 * TAG( Level ) LOCAL
 *
 * Indentation level enumeration.
 */
typedef enum
{
    LEVEL0,
    LEVEL1,
    LEVEL2,
    LEVEL3,
    LEVEL4,
    LEVEL5,
    LEVEL6,
    LEVEL7,
    LEVEL8,
    LEVEL9,
    LEVEL_QTY
} Level;

/*****************************************************************
 * TAG( indents ) LOCAL
 *
 * Array of indentation character counts, indexed by Level enum.
 */
static int indents[LEVEL_QTY] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36};

/*****************************************************************
 * TAG( main ) PUBLIC
 *
 * Open a Mili file family for read access and dump its contents
 * to standard output.
 */
int main(int argc, char *argv[])
{
    Mili_family *fam;
    int fam_id;
    int root_len, path_len;
    Return_value rval;
    char root_name[M_MAX_NAME_LEN];
    char path[128];
    char *p_path, *p_name, *p_probe, *p_trail;
    Dump_control dctrl;
    int i;
    Bool_type no_name_parsed, more;
    Bool_type create_db;
    char *p_c;

    memset(path, (int)'\0', 128);
    memset(root_name, (int)'\0', M_MAX_NAME_LEN);

    /* Initialize output modes. */
    dctrl.raw = FALSE;
    dctrl.formatted = TRUE;
    dctrl.brevity = DV_LONG;
    dctrl.directory = FALSE;
    dctrl.include_external = FALSE;
    dctrl.include_header = FALSE;

    no_name_parsed = TRUE;

    if ( argc < 2 )
    {
        usage();
    }

    for ( i = 1; i < argc; i++ )
    {
        if ( *argv[i] == '-' )
        {
            /* Parse flags. */
            p_c = argv[i] + 1;
            more = TRUE;
            while ( more )
            {
                switch ( *p_c )
                {
                    case 'h':
                        dctrl.brevity = DV_HEADERS;
                        break;
                    case 's':
                        dctrl.brevity = DV_SHORT;
                        break;
                    case 'l':
                        dctrl.brevity = DV_LONG;
                        break;
                    case 'd':
                        dctrl.directory = TRUE;
                        break;
                    case 'e':
                        dctrl.include_external = TRUE;
                        break;
                    case 'f':
                        dctrl.include_header = TRUE;
                        break;
                    case 'a':
                        dctrl.brevity = DV_LONG;
                        dctrl.directory = TRUE;
                        dctrl.include_external = TRUE;
                        dctrl.include_header = TRUE;
                        break;
                    case '\0':
                        more = FALSE;
                        break;
                    default:
                        usage();
                        break;
                }

                p_c++;
            }
        }
        else if ( no_name_parsed )
        {
            /* Sort the path_name into path and name. */

            p_path = path;
            p_name = root_name;
            p_probe = p_trail = argv[i];

            /* Until end of path_name is encountered... */
            while ( *p_probe )
            {
                /* Seek forward to next forward slash or NULL-terminator. */
                while ( *p_probe )
                {
                    if ( *p_probe == '/' )
                    {
                        break;
                    }
                    else
                    {
                        p_probe++;
                    }
                }

                if ( *p_probe == '/' )
                {
                    /* If at a slash, copy up to slash into path buffer. */
                    for ( ; p_trail <= p_probe; *p_path++ = *p_trail++ )
                        ;
                    p_probe++;
                }
                else
                {
                    /*
                     * If at NULL-terminator, copy into name buffer and
                     * terminate path buffer.
                     */
                    for ( ; p_trail <= p_probe; *p_name++ = *p_trail++ )
                        ;
                    if ( p_path > path )
                    {
                        *(p_path - 1) = '\0';
                    }
                }
            }

            if ( p_path == path )
            {
                path[0] = '.';
            }

            no_name_parsed = FALSE;
        }
        else
        {
            usage();
        }
    }

    root_len = (int)strlen(root_name);
    if ( root_len == 0 )
    {
        usage();
    }

    path_len = (int)strlen(path);

    fam = NEW(Mili_family, "Mili family");

    /* Build the family root string. */
    fam->root_len = path_len + root_len + 1;
    fam->root = (char *)calloc(fam->root_len + 1, sizeof(char));
    sprintf(fam->root, "%s/%s", path, root_name);

    /* Save the separate components. */
    str_dup(&fam->path, path);
    str_dup(&fam->file_root, root_name);

    /* Parse the control string and initialize the header. */
    rval = parse_control_string("Ar", fam, &create_db);
    if ( rval != OK )
    {
        clean_up_and_go(rval, fam);
    }

    /* Initialize file indices. */
    fam->cur_index = -1;
    fam->cur_st_index = -1;

    /* State data file pointer. */
    fam->cur_st_file = NULL;

    /* Set appropriate I/O routines for family. */
    rval = set_default_io_routines(fam);
    if ( rval != OK )
    {
        clean_up_and_go(rval, fam);
    }

    /* Check family for current write activity by any process. */
    test_write_lock(fam);

    /* Need this stuff for open_family(). */
    fam_list = RENEW_N(Mili_family *, fam_list, fam_qty, 1, "Big Kahuna pointers");
    fam_list[fam_qty] = fam;
    fam_id = fam_qty;
    fam_qty++;
    fam->my_id = fam_id;

    rval = open_family(fam_id);
    if ( rval != OK )
    {
        clean_up_and_go(rval, fam);
        free(fam_list[fam_qty]);
        fam_qty--;
    }

    /* Go do it. */
    rval = dump_family(fam, &dctrl);
    if ( rval != OK )
    {
        clean_up_and_go(rval, fam);
    }

    cleanse(fam);

    return 0;
}

/*****************************************************************
 * TAG( usage ) LOCAL
 *
 * Print usage line and exit.
 */
static void usage()
{
    printf("Usage: md [<path>/]<root name> [-<flag>...]\n");
    printf("Flags: h     # Output section header lines only      #\n");
    printf("       s     # Short output                          #\n");
    printf("       l     # Long  output (default)                #\n");
    printf("       f     # Include family header record dump     #\n");
    printf("       d     # Include directory dump                #\n");
    printf("       e     # Include external (host and date) info #\n");
    printf("       a     # Output all (equiv. to -lfde)          #\n");
    exit(1);
}

/*****************************************************************
 * TAG( clean_up_and_go ) LOCAL
 *
 * .
 */
static void clean_up_and_go(Return_value rval, Mili_family *fam)
{
    mc_print_error(NULL, rval);
    cleanse(fam);
    exit(0);
}

/*****************************************************************
 * TAG( dump_family ) LOCAL
 *
 * Mili dump main driver.
 */
static Return_value dump_family(Mili_family *fam, Dump_control *p_dc)
{
    FILE *p_f;
    char fname[M_MAX_NAME_LEN];
    File_dir *p_fd;
    Return_value rval;
    int i, j, nam_idx, num_written;
    Dir_entry_type etype;

    rval = load_directories(fam);
    if ( rval != OK )
    {
        return rval;
    }

    /*
     * Loop through all the available non-state files and traverse
     * the directories to dump entries.
     */

    make_fnam(NON_STATE_DATA, fam, 0, fname);

    for ( j = 0; j < fam->file_count; j++ )
    {
        p_f = fopen(fname, "rb");
        if ( p_f == NULL )
        {
            return OPEN_FAILED;
        }

        rval = eprintf(&num_written, "%*tFILE%8tindex: %3d%23tpathname: %s\n", indents[LEVEL0] + 1, j, fname);
        if ( rval != OK )
        {
            return rval;
        }

        /* If first file, dump the header. */
        if ( j == 0 && p_dc->include_header )
        {
            rval = dump_header(fam, p_dc);
            if ( rval != OK )
            {
                return rval;
            }
        }

        p_fd = fam->directory + j;
        nam_idx = 0;
        for ( i = 0; i < p_fd->qty_entries; i++ )
        {
            etype = (Dir_entry_type)p_fd->dir_entries[i][TYPE_IDX];

            if ( dump_funcs[etype] != NULL )
            {
                rval = dump_funcs[etype](fam, p_f, p_fd->dir_entries[i], p_fd->names + nam_idx, p_dc, indents[LEVEL1],
                                         indents[LEVEL2]);
                if ( rval != OK )
                {
                    return rval;
                }
            }

            nam_idx += p_fd->dir_entries[i][STRING_QTY_IDX];
        }

        if ( p_dc->directory )
        {
            rval = dump_directory(p_fd);
            if ( rval != OK )
            {
                return rval;
            }
        }

        /* Prepare to access next file in family. */
        fclose(p_f);
        make_fnam(NON_STATE_DATA, fam, j + 1, fname);
    }

    return OK;
}

/*****************************************************************
 * TAG( dump_header ) LOCAL
 *
 * Dump the database family header from the "A" file.
 */
static Return_value dump_header(Mili_family *fam, Dump_control *p_dc)
{
    Return_value rval;
    int i, j;
    int header_bytes, num_written;
    char char_str[] = {'\0', '\0'};
    char *prec_limit_labels[] = {"EMPTY", "single", "double", "quad", "none"};

    rval = eprintf(&num_written, "%*tHEADER%*t@0\n", indents[LEVEL1] + 1, indents[LEVEL1] + 45);
    if ( rval != OK )
    {
        return rval;
    }

    if ( p_dc->brevity == DV_HEADERS )
    {
        return OK;
    }

    rval = eprintf(&num_written, "%*tByte%*tValue%*tInterpretation\n", indents[LEVEL2] + 1, indents[LEVEL2] + 8,
                   indents[LEVEL2] + 19);
    if ( rval != OK )
    {
        return rval;
    }

    header_bytes = CHAR_HEADER_SIZE + fam->char_header[ADDL_FIELDS_IDX] * 4;

    for ( i = 0; i < header_bytes; i++ )
    {
        switch ( i )
        {
            case 0:
            case 1:
            case 2:
            case 3:
                if ( p_dc->brevity != DV_LONG )
                {
                    break;
                }

                char_str[0] = fam->char_header[i];
                rval = eprintf(&num_written,
                               "%*t%d%*t%d (\"%s\")%*tMili family identifier byte %d"
                               "\n",
                               indents[LEVEL2] + 1, i, indents[LEVEL2] + 8, (int)fam->char_header[i], char_str,
                               indents[LEVEL2] + 19, i + 1);
                if ( rval != OK )
                {
                    return rval;
                }
                break;

            case HDR_VERSION_IDX:
                if ( p_dc->brevity != DV_LONG )
                {
                    break;
                }

                rval = eprintf(&num_written, "%*t%d%*t%d%*tHeader format version\n", indents[LEVEL2] + 1, i,
                               indents[LEVEL2] + 8, (int)fam->char_header[i], indents[LEVEL2] + 19);
                if ( rval != OK )
                {
                    return rval;
                }
                break;

            case DIR_VERSION_IDX:
                if ( p_dc->brevity != DV_LONG )
                {
                    break;
                }

                rval = eprintf(&num_written, "%*t%d%*t%d%*tDirectory format version\n", indents[LEVEL2] + 1, i,
                               indents[LEVEL2] + 8, (int)fam->char_header[i], indents[LEVEL2] + 19);
                if ( rval != OK )
                {
                    return rval;
                }
                break;

            case ENDIAN_IDX:
                rval = eprintf(&num_written, "%*t%d%*t%d%*tByte order = %s\n", indents[LEVEL2] + 1, i,
                               indents[LEVEL2] + 8, (int)fam->char_header[i], indents[LEVEL2] + 19,
                               (fam->char_header[i] == MILI_BIG_ENDIAN) ? "big endian" : "little endian");
                if ( rval != OK )
                {
                    return rval;
                }
                break;

            case PRECISION_LIMIT_IDX:
                rval = eprintf(&num_written, "%*t%d%*t%d%*tPrecision limit = %s\n", indents[LEVEL2] + 1, i,
                               indents[LEVEL2] + 8, (int)fam->char_header[i], indents[LEVEL2] + 19,
                               prec_limit_labels[fam->char_header[i]]);
                if ( rval != OK )
                {
                    return rval;
                }
                break;

            case ST_FILE_SUFFIX_WIDTH_IDX:
                if ( p_dc->brevity != DV_LONG )
                {
                    break;
                }

                rval = eprintf(&num_written, "%*t%d%*t%d%*tState file name numeric suffix width\n", indents[LEVEL2] + 1,
                               i, indents[LEVEL2] + 8, (int)fam->char_header[i], indents[LEVEL2] + 19);
                if ( rval != OK )
                {
                    return rval;
                }
                break;

            case PARTITION_SCHEME_IDX:
                if ( p_dc->brevity != DV_LONG )
                {
                    break;
                }

                rval = eprintf(&num_written, "%*t%d%*t%d%*tState file partition scheme = %s\n", indents[LEVEL2] + 1, i,
                               indents[LEVEL2] + 8, (int)fam->char_header[i], indents[LEVEL2] + 19,
                               (fam->char_header[i] == STATE_COUNT) ? "state count" : "UNKNOWN");
                if ( rval != OK )
                {
                    return rval;
                }
                break;

            default:
                /* Verify that rest of header is zeros. */
                for ( j = i; j < header_bytes; j++ )
                {
                    if ( fam->char_header[j] != 0 )
                    {
                        break;
                    }
                }

                if ( j < header_bytes )
                {
                    rval = eprintf(&num_written, "%*t%d-%d%*tFOUND NON-ZERO BYTES; Contact MDG\n", indents[LEVEL2] + 1,
                                   i, header_bytes - 1, indents[LEVEL2] + 8);
                    if ( rval != OK )
                    {
                        return rval;
                    }
                }
                else if ( p_dc->brevity == DV_LONG )
                {
                    rval = eprintf(&num_written, "%*t%d-%d%*t0...%*tUNUSED\n", indents[LEVEL2] + 1, i, header_bytes - 1,
                                   indents[LEVEL2] + 8, indents[LEVEL2] + 19);
                    if ( rval != OK )
                    {
                        return rval;
                    }
                }

                i = header_bytes; /* Force termination */
                break;
        }
    }
    return OK;
}

/*****************************************************************
 * TAG( dump_directory ) LOCAL
 *
 * Dump the non-state file directory.
 *
 */
static Return_value dump_directory(File_dir *p_fd)
{
    Dir_entry *p_de;
    int col_space;
    int i, j;
    int qty_ent, nnames, num_written;
    Return_value rval;

    col_space = DIR_ENTRY_COLUMN_SPACING;
    p_de = p_fd->dir_entries;
    qty_ent = p_fd->qty_entries;
    nnames = 0;

    rval = eprintf(&num_written, "%*tDIRECTORY:\n", indents[LEVEL1] + 1);
    if ( rval != OK )
    {
        return rval;
    }

    rval = eprintf(&num_written, "%*t%-*s%-*s%-*s%-*s%-*s%-s\n", indents[LEVEL2] + 1, col_space, "Etype", col_space,
                   "Mod1", col_space, "Mod2", col_space, "Offset", col_space, "Length", "String\n");
    if ( rval != OK )
    {
        return rval;
    }
    for ( i = 0; i < qty_ent; i++ )
    {
        rval = eprintf(&num_written, "%*t%-*d%-*d%-*d%-*d%-*d%-s\n", indents[LEVEL2] + 1, col_space, p_de[i][TYPE_IDX],
                       col_space, p_de[i][MODIFIER1_IDX], col_space, p_de[i][MODIFIER2_IDX], col_space,
                       p_de[i][OFFSET_IDX], col_space, p_de[i][LENGTH_IDX],
                       ((p_de[i][STRING_QTY_IDX] > 0) ? p_fd->names[nnames] : " "));
        if ( rval != OK )
        {
            return rval;
        }

        for ( j = 1; j < p_de[i][STRING_QTY_IDX]; j++ )
        {
            rval = eprintf(&num_written, "%*t%-s\n", indents[LEVEL2] + 1 + 5 * col_space, p_fd->names[nnames + j]);
            if ( rval != OK )
            {
                return rval;
            }
        }

        nnames += p_de[i][STRING_QTY_IDX];
    }
    return OK;
}
