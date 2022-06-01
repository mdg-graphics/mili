/* $Id: mode_test.c,v 1.1 2021/07/23 16:03:03 rhathaw Exp $ */

/*
 * mode_test.c:
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
 *
 *
 * This program is meant to exercise a variety of Mili family open
 * control string options.  Confirmation of test case outcomes requires
 * examining data internal to Mili.  However, if both the Mili library
 * and this application are compiled with "MODE_TEST" defined, Mili
 * will make the necessary internal state information available in an
 * exported buffer ("mode_test_data") to be read by this application.
 *
 * Many of the tests require an input Mili db with an appropriate
 * endianness or precision limit.  If required db's do not exist, 
 * mode_test will create appropriately configured db's, but in the 
 * case of endianness tests this presumes a correctness which the test 
 * is actually intended to evaluate.
 *
 * Notation:
 *
 *   Sources - entities for which test conditions can be specified
 *      H[]  - The compute host running the test
 *      F[]  - A Mili family
 *      R[]  - A control string specified to an mc_open() call
 *
 *   Conditions - properties which may be specified for a source
 *      Ex   - Endianness specification, where x can be "b" (big endian),
 *             "l" (little endian), or "n" (native endianness of host);
 *      Px   - Precision limit specification, where x can be "d" (double), 
 *             or "s" (single);
 *      Ax   - Access mode specification, where x can be "r" (read),
 *             "w" (write), or "a" (append),
 *      0    - Source is empty or non-existent;
 *      !0   - Source exists;
 *
 *   Compositions of Sources and Conditions
 *      H[C] - Host has condition C, where C is "Ex" (above);
 *      F[C] - Extant Mili family has condition C, where C is "Ex", "Px", 
 *             "0", or "!0";
 *      R[C] - Mili database open control string request specifies
 *             condition C, where C is "Ex", "Px", "Ax", or "0";
 *
 *   Miscellaneous notation
 *      =>   - "results in" or "leads to"
 *
 * Test Category A - Write access, Endianness
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * A1       | H[Eb],R[Eb]=>no swap            | fam->swap_bytes = FALSE
 * A2       | H[Eb],R[El]=>swap               | fam->swap_bytes = TRUE
 * A3       | H[Eb],R[En]=>no swap            | fam->swap_bytes = FALSE
 * A4       | H[El],R[Eb]=>swap               | fam->swap_bytes = TRUE
 * A5       | H[El],R[El]=>no swap            | fam->swap_bytes = FALSE
 * A6       | H[El],R[En]=>no swap            | fam->swap_bytes = FALSE
 *
 *
 * Test Category B - Read access, Endianness
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * B1       | H[Eb],F[Eb]=>no swap            | fam->swap_bytes = FALSE
 * B2       | H[Eb],F[El]=>swap               | fam->swap_bytes = TRUE
 * B3       | H[El],F[Eb]=>swap               | fam->swap_bytes = TRUE
 * B4       | H[El],F[El]=>no swap            | fam->swap_bytes = FALSE
 *
 *
 * Test Category C - Append access, extant db, Precision
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * C1       | R[Ps],F[Pd]=>prec limit double  | fam->precision_limit = PREC_LIMIT_DOUBLE
 * C2       | R[Ps],F[Ps]=>prec limit single  | fam->precision_limit = PREC_LIMIT_SINGLE
 * C3       | R[Pd],F[Pd]=>prec limit double  | fam->precision_limit = PREC_LIMIT_DOUBLE
 * C4       | R[Pd],F[Ps]=>prec limit single  | fam->precision_limit = PREC_LIMIT_SINGLE
 *
 *
 * Test Category D - Append access, extant db, Endianness
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * D1       | H[Eb],F[Eb],R[Eb]=>no swap      | fam->swap_bytes = FALSE
 * D2       | H[Eb],F[Eb],R[El]=>no swap      | fam->swap_bytes = FALSE
 * D3       | H[Eb],F[Eb],R[En]=>no swap      | fam->swap_bytes = FALSE
 * D4       | H[Eb],F[El],R[Eb]=>swap         | fam->swap_bytes = TRUE
 * D5       | H[Eb],F[El],R[El]=>swap         | fam->swap_bytes = TRUE
 * D6       | H[Eb],F[El],R[En]=>swap         | fam->swap_bytes = TRUE
 * D7       | H[El],F[Eb],R[Eb]=>swap         | fam->swap_bytes = TRUE
 * D8       | H[El],F[Eb],R[El]=>swap         | fam->swap_bytes = TRUE
 * D9       | H[El],F[Eb],R[En]=>swap         | fam->swap_bytes = TRUE
 * D10      | H[El],F[El],R[Eb]=>no swap      | fam->swap_bytes = FALSE
 * D11      | H[El],F[El],R[El]=>no swap      | fam->swap_bytes = FALSE
 * D12      | H[El],F[El],R[En]=>no swap      | fam->swap_bytes = FALSE
 *
 *
 * Test Category E - Append access, no extant db, Endianness
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * E1       | F[0],H[Eb],R[Aa],R[Eb]=>no swap | fam->swap_bytes = FALSE
 * E2       | F[0],H[Eb],R[Aa],R[El]=>swap    | fam->swap_bytes = TRUE
 * E3       | F[0],H[Eb],R[Aa],R[En]=>no swap | fam->swap_bytes = FALSE
 * E4       | F[0],H[El],R[Aa],R[Eb]=>swap    | fam->swap_bytes = TRUE
 * E5       | F[0],H[El],R[Aa],R[El]=>no swap | fam->swap_bytes = FALSE
 * E6       | F[0],H[El],R[Aa],R[En]=>no swap | fam->swap_bytes = FALSE
 *
 *
 * Test Category F - Read access, extant db, Precision (identical to category C)
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * F1       | R[Ps],F[Pd]=>prec limit double  | fam->precision_limit = PREC_LIMIT_DOUBLE
 * F2       | R[Ps],F[Ps]=>prec limit single  | fam->precision_limit = PREC_LIMIT_SINGLE
 * F3       | R[Pd],F[Pd]=>prec limit double  | fam->precision_limit = PREC_LIMIT_DOUBLE
 * F4       | R[Pd],F[Ps]=>prec limit single  | fam->precision_limit = PREC_LIMIT_SINGLE
 *
 *
 * Test Category G - Read access, no extant db
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * G1       | F[0],R[Ar]=>open fail           | Return status = OPEN_FAIL
 *
 *
 * Test Category H - Empty extant database operations
 * Note - Output database from H1 forms input for H2
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * H1       | R[Aw],close=>no errors, valid db| close return status = OK
 * H2       | R[Aa],write object,close        | "A" file has objects written
 *
 *
 * Test Category I - Defaults (empty request)
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * I1       | R[0],H[Eb],F[0]=>no swap,       | fam->swap_bytes = FALSE,
 *          | prec limit double,              | fam->precision_limit = PREC_LIMIT_DOUBLE,
 *          | write access                    | fam->access_mode = 'w'
 * I2       | R[0],H[El],F[0]=>no swap,       | fam->swap_bytes = FALSE,
 *          | prec limit double,              | fam->precision_limit = PREC_LIMIT_DOUBLE,
 *          | write access                    | fam->access_mode = 'w'
 * I3       | R[0],F[Pd]=>prec limit double,  | fam->precision_limit = PREC_LIMIT_DOUBLE,
 *          | read access                     | fam->access_mode = 'r'
 * I4       | R[0],F[Ps]=>prec limit single,  | fam->precision_limit = PREC_LIMIT_SINGLE,
 *          | read access                     | fam->access_mode = 'r'
 * I5       | R[0],H[Eb],F[Eb]=>no swap,      | fam->swap_bytes = FALSE,
 *          | read access                     | fam->access_mode = 'r'
 * I6       | R[0],H[El],F[Eb]=>swap,         | fam->swap_bytes = TRUE,
 *          | read access                     | fam->access_mode = 'r'
 * I7       | R[0],H[Eb],F[El]=>swap,         | fam->swap_bytes = TRUE,
 *          | read access                     | fam->access_mode = 'r'
 * I8       | R[0],H[El],F[El]=>no swap,      | fam->swap_bytes = FALSE,
 *          | read access                     | fam->access_mode = 'r'
 *
 *
 * Test Category J - Defaults for write access, extant db
 * 
 * Test id. | Requirement                     | Result confirmation
 * ---------|---------------------------------|-----------------------------------------
 * J1       | H[Eb],F[El],F[Ps],R[Aw]=>       | fam->swap_bytes = FALSE,
 *          | no swap, prec limit double      | fam->precision_limit = PREC_LIMIT_DOUBLE
 * J2       | H[El],F[Eb],F[Ps],R[Aw]=>       | fam->swap_bytes = FALSE,
 *          | no swap, prec limit double      | fam->precision_limit = PREC_LIMIT_DOUBLE
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mili.h"
#include "mili_enum.h"
#include "mili_endian.h"


main( int argc, char *argv[] )
{
    Famid fid;
    int rval;
    char *a1 = "A01test";
    char *a2 = "A02test";
    char *a3 = "A03test";
    char *a4 = "A04test";
    char *a5 = "A05test";
    char *a6 = "A06test";
    char *c1 = "C01test";
    char *c2 = "C02test";
    char *c3 = "C03test";
    char *c4 = "C04test";
    char *e1 = "E01test";
    char *e2 = "E02test";
    char *e3 = "E03test";
    char *e4 = "E04test";
    char *e5 = "E05test";
    char *e6 = "E06test";
    char *f1 = "F01test";
    char *f2 = "F02test";
    char *f3 = "F03test";
    char *f4 = "F04test";
    char *g1 = "G01test";
    char *h1 = "H01test";
    char *i1 = "I01test";
    char *i2 = "I02test";
    char *j1 = "J01test";
    char *j2 = "J02test";
    char *big_e_db, *little_e_db;
    struct stat statbuf;
    int db_save;
#ifdef MODE_TEST
    /* 
     * After an mc_open() call but before any other Mili call,
     *   mode_test_data[0] = fam->swap_bytes
     *   mode_test_data[1] = fam->precision_limit
     *   mode_test_data[2] = fam->access_mode
     */
    extern int mode_test_data[];
#else
    printf( "MODE_TEST not defined to pre-processor; test results can only\n"
            "be confirmed manually in debugger.\n" );
#endif

    /* Parse command param if present to save test db. */
    if ( argc > 1 
         && ( ( *argv[1] == 's' ) 
              || ( *argv[1] == '-' && *(argv[1] + 1) == 's' ) )
       )
        db_save = 1;
    else
        db_save = 0;

    big_e_db = little_e_db = NULL;


    /*
     * Test case A1
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( a1, "data", "AwEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test A1 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test A1 - post-test close failed", rval );
            else
                big_e_db = a1;
        }
        else
            printf( "Test A1 abort - unable to open db.\n" );
    }
    else
        printf( "Test A1 bypassed on little endian host.\n" );


    /*
     * Test case A2
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( a2, "data", "AwEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test A2 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test A2 - post-test close failed", rval );
            else
                little_e_db = a2;
        }
        else
            printf( "Test A2 abort - unable to open db.\n" );
    }
    else
        printf( "Test A2 bypassed on little endian host.\n" );


    /*
     * Test case A3
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( a3, "data", "AwEn", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test A3 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval == OK )
            {
                rval = mc_delete_family( a3, "data" );
                if ( rval != OK )
                    mc_print_error( "Test A3 - post-test clean-up failed", rval );
            }
            else
                mc_print_error( "Test A3 - post-test close failed", rval );
        }
        else
            printf( "Test A3 abort - unable to open db.\n" );
    }
    else
        printf( "Test A3 bypassed on little endian host.\n" );


    /*
     * Test case A4
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( a4, "data", "AwEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test A4 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test A4 - post-test close failed", rval );
            else
                big_e_db = a4;
        }
        else
            printf( "Test A4 abort - unable to open db.\n" );
    }
    else
        printf( "Test A4 bypassed on big endian host.\n" );


    /*
     * Test case A5
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( a5, "data", "AwEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test A5 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test A5 - post-test close failed", rval );
            else
                little_e_db = a5;
        }
        else
            printf( "Test A5 abort - unable to open db.\n" );
    }
    else
        printf( "Test A5 bypassed on big endian host.\n" );


    /*
     * Test case A6
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( a6, "data", "AwEn", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test A6 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval == OK )
            {
                rval = mc_delete_family( a6, "data" );
                if ( rval != OK )
                    mc_print_error( "Test A6 - post-test clean-up failed", rval );
            }
            else
                mc_print_error( "Test A6 - post-test close failed", rval );
        }
        else
            printf( "Test A6 abort - unable to open db.\n" );
    }
    else
        printf( "Test A6 bypassed on big endian host.\n" );


    /*
     * Test Category B prep
     */

    if ( BIG_ENDIAN_HOST )
    {
        /* Prefer db's created on little endian host if they exist. */
        rval = stat( "data/A04testA", &statbuf );
        if ( rval == 0 )
        {
            mc_delete_family( a1, "data" );
            big_e_db = a4;
        }

        rval = stat( "data/A05testA", &statbuf );
        if ( rval == 0 )
        {
            mc_delete_family( a2, "data" );
            little_e_db = a5;
        }
    }

    if ( LITTLE_ENDIAN_HOST )
    {
        /* Prefer db's created on big endian host if they exist. */
        rval = stat( "data/A01testA", &statbuf );
        if ( rval == 0 )
        {
            mc_delete_family( a4, "data" );
            big_e_db = a1;
        }

        rval = stat( "data/A02testA", &statbuf );
        if ( rval == 0 )
        {
            mc_delete_family( a5, "data" );
            little_e_db = a2;
        }
    }


    /*
     * Test case B1
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "ArEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test B1 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test B1 - post-test close failed", rval );
        }
        else
            mc_print_error( "Test B1 abort - unable to open db", rval );
    }
    else
        printf( "Test B1 bypassed on little endian host.\n" );


    /*
     * Test case B2
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "ArEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test B2 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test B2 - post-test close failed", rval );
        }
        else
            mc_print_error( "Test B2 abort - unable to open db", rval );
    }
    else
        printf( "Test B2 bypassed on little endian host.\n" );


    /*
     * Test case B3
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "ArEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test B3 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test B3 - post-test close failed", rval );
        }
        else
            mc_print_error( "Test B3 abort - unable to open db", rval );
    }
    else
        printf( "Test B3 bypassed on big endian host.\n" );


    /*
     * Test case B4
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "ArEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test B4 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test B4 - post-test close failed", rval );
        }
        else
            mc_print_error( "Test B4 abort - unable to open db", rval );
    }
    else
        printf( "Test B4 bypassed on big endian host.\n" );


    /*
     * Test case C1
     */
    rval = mc_open( c1, "data", "AwPd", &fid );
    if ( rval != OK )
        printf( "Test case C1 abort - unable to create target db.\n" );
    else
    {
        int val = 10;
        rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
        if ( rval != OK )
            mc_print_error( "Test C1 - integer var write failed", rval );
        rval = mc_close( fid );
        if ( rval != OK )
            mc_print_error( "Test C1 - close target db failed", rval );

        /* The test... */
        rval = mc_open( c1, "data", "AaPs", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[1] != PREC_LIMIT_DOUBLE )
                printf( "Test C1 failed.\n" );
#endif
            rval = mc_delete_family( c1, "data" );
            if ( rval != OK )
                mc_print_error( "Test C1 - post-test clean-up failed", rval );
        }
        else
            mc_print_error( "Test C1 abort - unable to open db", rval );
    }


    /*
     * Test case C2
     */
    rval = mc_open( c2, "data", "AwPs", &fid );
    if ( rval != OK )
        printf( "Test case C2 abort - unable to create target db.\n" );
    else
    {
        int val = 10;
        rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
        if ( rval != OK )
            mc_print_error( "Test C2 - integer var write failed", rval );
        rval = mc_close( fid );
        if ( rval != OK )
            mc_print_error( "Test C2 - close target db failed", rval );

        /* The test... */
        rval = mc_open( c2, "data", "AaPs", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[1] != PREC_LIMIT_SINGLE )
                printf( "Test C2 failed.\n" );
#endif
            rval = mc_delete_family( c2, "data" );
            if ( rval != OK )
                mc_print_error( "Test C2 - post-test clean-up failed", rval );
        }
        else
            mc_print_error( "Test C2 abort - unable to open db", rval );
    }


    /*
     * Test case C3
     */
    rval = mc_open( c3, "data", "AwPd", &fid );
    if ( rval != OK )
        printf( "Test case C3 abort - unable to create target db.\n" );
    else
    {
        int val = 10;
        rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
        if ( rval != OK )
            mc_print_error( "Test C3 - integer var write failed", rval );
        rval = mc_close( fid );
        if ( rval != OK )
            mc_print_error( "Test C3 - close target db failed", rval );

        /* The test... */
        rval = mc_open( c3, "data", "AaPd", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[1] != PREC_LIMIT_DOUBLE )
                printf( "Test C3 failed.\n" );
#endif
            rval = mc_delete_family( c3, "data" );
            if ( rval != OK )
                mc_print_error( "Test C3 - post-test clean-up failed", rval );
        }
        else
            mc_print_error( "Test C3 abort - unable to open db", rval );
    }


    /*
     * Test case C4
     */
    rval = mc_open( c4, "data", "AwPs", &fid );
    if ( rval != OK )
        printf( "Test case C4 abort - unable to create target db.\n" );
    else
    {
        int val = 10;
        rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
        if ( rval != OK )
            mc_print_error( "Test C4 - integer var write failed", rval );
        rval = mc_close( fid );
        if ( rval != OK )
            mc_print_error( "Test C4 - close target db failed", rval );

        /* The test... */
        rval = mc_open( c4, "data", "AaPd", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[1] != PREC_LIMIT_SINGLE )
                printf( "Test C4 failed.\n" );
#endif
            rval = mc_delete_family( c4, "data" );
            if ( rval != OK )
                mc_print_error( "Test C4 - post-test clean-up failed", rval );
        }
        else
            mc_print_error( "Test C4 abort - unable to open db", rval );
    }


    /*
     * Test case D1
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "AaEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test D1 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D1 - post-test close failed", rval );
        }
        else
            printf( "Test D1 abort - unable to open db.\n" );
    }
    else
        printf( "Test D1 bypassed on little endian host.\n" );


    /*
     * Test case D2
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "AaEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test D2 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D2 - post-test close failed", rval );
        }
        else
            printf( "Test D2 abort - unable to open db.\n" );
    }
    else
        printf( "Test D2 bypassed on little endian host.\n" );


    /*
     * Test case D3
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "AaEn", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test D3 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D3 - post-test close failed", rval );
        }
        else
            printf( "Test D3 abort - unable to open db.\n" );
    }
    else
        printf( "Test D3 bypassed on little endian host.\n" );


    /*
     * Test case D4
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "AaEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test D4 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D4 - post-test close failed", rval );
        }
        else
            printf( "Test D4 abort - unable to open db.\n" );
    }
    else
        printf( "Test D4 bypassed on little endian host.\n" );


    /*
     * Test case D5
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "AaEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test D5 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D5 - post-test close failed", rval );
        }
        else
            printf( "Test D5 abort - unable to open db.\n" );
    }
    else
        printf( "Test D5 bypassed on little endian host.\n" );


    /*
     * Test case D6
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "AaEn", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test D6 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D6 - post-test close failed", rval );
        }
        else
            printf( "Test D6 abort - unable to open db.\n" );
    }
    else
        printf( "Test D6 bypassed on little endian host.\n" );


    /*
     * Test case D7
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "AaEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test D7 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D7 - post-test close failed", rval );
        }
        else
            printf( "Test D7 abort - unable to open db.\n" );
    }
    else
        printf( "Test D7 bypassed on big endian host.\n" );


    /*
     * Test case D8
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "AaEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test D8 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D8 - post-test close failed", rval );
        }
        else
            printf( "Test D8 abort - unable to open db.\n" );
    }
    else
        printf( "Test D8 bypassed on big endian host.\n" );


    /*
     * Test case D9
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "AaEn", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test D9 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D9 - post-test close failed", rval );
        }
        else
            printf( "Test D9 abort - unable to open db.\n" );
    }
    else
        printf( "Test D9 bypassed on big endian host.\n" );


    /*
     * Test case D10
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "AaEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test D10 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D10 - post-test close failed", rval );
        }
        else
            printf( "Test D10 abort - unable to open db.\n" );
    }
    else
        printf( "Test D10 bypassed on big endian host.\n" );


    /*
     * Test case D11
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "AaEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test D11 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D11 - post-test close failed", rval );
        }
        else
            printf( "Test D11 abort - unable to open db.\n" );
    }
    else
        printf( "Test D11 bypassed on big endian host.\n" );


    /*
     * Test case D12
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "AaEn", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test D12 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test D12 - post-test close failed", rval );
        }
        else
            printf( "Test D12 abort - unable to open db.\n" );
    }
    else
        printf( "Test D12 bypassed on big endian host.\n" );


    /*
     * Test case E1
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( e1, "data", "AaEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test E1 failed.\n" );
#endif
            rval = mc_delete_family( e1, "data" );
            if ( rval != OK )
                mc_print_error( "Test E1 - post-test clean-up failed", rval );
        }
        else
            printf( "Test E1 abort - unable to open db.\n" );
    }
    else
        printf( "Test E1 bypassed on little endian host.\n" );


    /*
     * Test case E2
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( e2, "data", "AaEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test E2 failed.\n" );
#endif
            rval = mc_delete_family( e2, "data" );
            if ( rval != OK )
                mc_print_error( "Test E2 - post-test clean-up failed", rval );
        }
        else
            printf( "Test E2 abort - unable to open db.\n" );
    }
    else
        printf( "Test E2 bypassed on little endian host.\n" );


    /*
     * Test case E3
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( e3, "data", "AaEn", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test E3 failed.\n" );
#endif
            rval = mc_delete_family( e3, "data" );
            if ( rval != OK )
                mc_print_error( "Test E3 - post-test clean-up failed", rval );
        }
        else
            printf( "Test E3 abort - unable to open db.\n" );
    }
    else
        printf( "Test E3 bypassed on little endian host.\n" );


    /*
     * Test case E4
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( e4, "data", "AaEb", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0] ) /* want swap_bytes TRUE */
                printf( "Test E4 failed.\n" );
#endif
            rval = mc_delete_family( e4, "data" );
            if ( rval != OK )
                mc_print_error( "Test E4 - post-test clean-up failed", rval );
        }
        else
            printf( "Test E4 abort - unable to open db.\n" );
    }
    else
        printf( "Test E4 bypassed on big endian host.\n" );


    /*
     * Test case E5
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( e5, "data", "AaEl", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test E5 failed.\n" );
#endif
            rval = mc_delete_family( e5, "data" );
            if ( rval != OK )
                mc_print_error( "Test E5 - post-test clean-up failed", rval );
        }
        else
            printf( "Test E5 abort - unable to open db.\n" );
    }
    else
        printf( "Test E5 bypassed on big endian host.\n" );


    /*
     * Test case E6
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( e6, "data", "AaEn", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] ) /* want swap_bytes FALSE */
                printf( "Test E6 failed.\n" );
#endif
            rval = mc_delete_family( e6, "data" );
            if ( rval != OK )
                mc_print_error( "Test E6 - post-test clean-up failed", rval );
        }
        else
            printf( "Test E6 abort - unable to open db.\n" );
    }
    else
        printf( "Test E6 bypassed on big endian host.\n" );


    /*
     * Test case F1
     */
    rval = mc_open( f1, "data", "AwPd", &fid );
    if ( rval != OK )
        printf( "Test case F1 abort - unable to create target db.\n" );
    else
    {
        int val = 10;
        rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
        if ( rval != OK )
            mc_print_error( "Test F1 - integer var write failed", rval );
        rval = mc_close( fid );
        if ( rval != OK )
            mc_print_error( "Test F1 - close target db failed", rval );

        /* The test... */
        rval = mc_open( f1, "data", "ArPs", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[1] != PREC_LIMIT_DOUBLE )
                printf( "Test F1 failed.\n" );
#endif
            rval = mc_delete_family( f1, "data" );
            if ( rval != OK )
                mc_print_error( "Test F1 - post-test clean-up failed", rval );
        }
        else
            mc_print_error( "Test F1 abort - unable to open db", rval );
    }


    /*
     * Test case F2
     */
    rval = mc_open( f2, "data", "AwPs", &fid );
    if ( rval != OK )
        printf( "Test case F2 abort - unable to create target db.\n" );
    else
    {
        int val = 10;
        rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
        if ( rval != OK )
            mc_print_error( "Test F2 - integer var write failed", rval );
        rval = mc_close( fid );
        if ( rval != OK )
            mc_print_error( "Test F2 - close target db failed", rval );

        /* The test... */
        rval = mc_open( f2, "data", "ArPs", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[1] != PREC_LIMIT_SINGLE )
                printf( "Test F2 failed.\n" );
#endif
            rval = mc_delete_family( f2, "data" );
            if ( rval != OK )
                mc_print_error( "Test F2 - post-test clean-up failed", rval );
        }
        else
            mc_print_error( "Test F2 abort - unable to open db", rval );
    }


    /*
     * Test case F3
     */
    rval = mc_open( f3, "data", "AwPd", &fid );
    if ( rval != OK )
        printf( "Test case F3 abort - unable to create target db.\n" );
    else
    {
        int val = 10;
        rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
        if ( rval != OK )
            mc_print_error( "Test F3 - integer var write failed", rval );
        rval = mc_close( fid );
        if ( rval != OK )
            mc_print_error( "Test F3 - close target db failed", rval );

        /* The test... */
        rval = mc_open( f3, "data", "ArPd", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[1] != PREC_LIMIT_DOUBLE )
                printf( "Test F3 failed.\n" );
#endif
            /* Leave db around for use in Category I. */
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test F3 - post-test db close failed", rval );
        }
        else
            mc_print_error( "Test F3 abort - unable to open db", rval );
    }


    /*
     * Test case F4
     */
    rval = mc_open( f4, "data", "AwPs", &fid );
    if ( rval != OK )
        printf( "Test case F4 abort - unable to create target db.\n" );
    else
    {
        int val = 10;
        rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
        if ( rval != OK )
            mc_print_error( "Test F4 - integer var write failed", rval );
        rval = mc_close( fid );
        if ( rval != OK )
            mc_print_error( "Test F4 - close target db failed", rval );

        /* The test... */
        rval = mc_open( f4, "data", "ArPd", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[1] != PREC_LIMIT_SINGLE )
                printf( "Test F4 failed.\n" );
#endif
            /* Leave db around for use in Category I. */
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test F4 - post-test db close failed", rval );
        }
        else
            mc_print_error( "Test F4 abort - unable to open db", rval );
    }


    /*
     * Test case G1 - Read access on non-existing db.
     */
    rval = mc_open( g1, "data", "Ar", &fid );

    if ( rval == OK )
        printf( "Test G1 failed.\n" );


    /*
     * Test case H1 - Non-existing db, empty db operations.
     */
    rval = mc_open( h1, "data", "Aw", &fid );

    if ( rval == OK )
    {
        rval = mc_close( fid );
        if ( rval != OK )
        {
            mc_print_error( "Test H1 - post-test close failed", rval );
        }
        else
        {
            /*
             * Test case H2 - Append to "empty" extant db.
             */
            rval = mc_open( h1, "data", "Aa", &fid );
            if ( rval != OK )
            {
                mc_print_error( "Test H2 abort - unable to open target db", rval );
            }
            else
            {
                int val = 10;

                rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
                if ( rval != OK )
                    mc_print_error( "Test H2 - integer var write failed", rval );
                else
                {
                    rval = mc_close( fid );
                    if ( rval != OK )
                        mc_print_error( "Test H2 - post-test close failed", rval );

                    rval = mc_delete_family( h1, "data" );
                    if ( rval != OK )
                        mc_print_error( "Test H2 - post-test clean-up failed", rval );
                }
            }
        }
    }
    else
        mc_print_error( "Test H1 abort - unable to open db", rval );


    /*
     * Test case I1
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( i1, "data", "", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] /* want swap_bytes FALSE */
                 || mode_test_data[1] != PREC_LIMIT_DOUBLE
                 || mode_test_data[2] != 'w' )
                printf( "Test I1 failed.\n" );
#endif
            rval = mc_delete_family( i1, "data" );
            if ( rval != OK )
                mc_print_error( "Test I1 - post-test clean-up failed", rval );
        }
        else
            printf( "Test I1 abort - unable to open db.\n" );
    }
    else
        printf( "Test I1 bypassed on little endian host.\n" );


    /*
     * Test case I2
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( i2, "data", "", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0] /* want swap_bytes FALSE */
                 || mode_test_data[1] != PREC_LIMIT_DOUBLE
                 || mode_test_data[2] != 'w' )
                printf( "Test I2 failed.\n" );
#endif
            rval = mc_delete_family( i2, "data" );
            if ( rval != OK )
                mc_print_error( "Test I2 - post-test clean-up failed", rval );
        }
        else
            printf( "Test I2 abort - unable to open db.\n" );
    }
    else
        printf( "Test I2 bypassed on big endian host.\n" );


    /*
     * Test case I3
     */
    rval = mc_open( f3, "data", NULL, &fid ); /* Try with NULL control string instead of zero length. */

    if ( rval == OK )
    {
#ifdef MODE_TEST
        if ( mode_test_data[1] != PREC_LIMIT_DOUBLE
             || mode_test_data[2] != 'r' )
            printf( "Test I3 failed.\n" );
#endif
        rval = mc_delete_family( f3, "data" );
        if ( rval != OK )
            mc_print_error( "Test I3 - post-test clean-up failed", rval );
    }
    else
        printf( "Test I3 abort - unable to open db \"data/%s\".\n", f3 );


    /*
     * Test case I4
     */
    rval = mc_open( f4, "data", NULL, &fid ); /* Try with NULL control string instead of zero length. */

    if ( rval == OK )
    {
#ifdef MODE_TEST
        if ( mode_test_data[1] != PREC_LIMIT_SINGLE
             || mode_test_data[2] != 'r' )
            printf( "Test I4 failed.\n" );
#endif
        rval = mc_delete_family( f4, "data" );
        if ( rval != OK )
            mc_print_error( "Test I4 - post-test clean-up failed", rval );
    }
    else
        printf( "Test I4 abort - unable to open db \"data/%s\".\n", f4 );


    /*
     * Test case I5
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0]  /* want swap_bytes FALSE */
                 || mode_test_data[2] != 'r' )
                printf( "Test I5 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test I5 - post-test db close failed", rval );
        }
        else
            printf( "Test I5 abort - unable to open db \"data/%s\".\n", big_e_db );
    }
    else
        printf( "Test I5 bypassed on little endian host.\n" );


    /*
     * Test case I6
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( big_e_db, "data", "", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0]  /* want swap_bytes TRUE */
                 || mode_test_data[2] != 'r' )
                printf( "Test I6 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test I6 - post-test db close failed", rval );
        }
        else
            printf( "Test I6 abort - unable to open db \"data/%s\".\n", big_e_db );
    }
    else
        printf( "Test I6 bypassed on big endian host.\n" );


    /*
     * Test case I7
     */
    if ( BIG_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0]  /* want swap_bytes TRUE */
                 || mode_test_data[2] != 'r' )
                printf( "Test I7 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test I7 - post-test db close failed", rval );
        }
        else
            printf( "Test I7 abort - unable to open db \"data/%s\".\n", little_e_db );
    }
    else
        printf( "Test I7 bypassed on little endian host.\n" );


    /*
     * Test case I8
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        rval = mc_open( little_e_db, "data", "", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( mode_test_data[0]  /* want swap_bytes FALSE */
                 || mode_test_data[2] != 'r' )
                printf( "Test I8 failed.\n" );
#endif
            rval = mc_close( fid );
            if ( rval != OK )
                mc_print_error( "Test I8 - post-test db close failed", rval );
        }
        else
            printf( "Test I8 abort - unable to open db \"data/%s\".\n", little_e_db );
    }
    else
        printf( "Test I8 bypassed on big endian host.\n" );


    /*
     * Test case J1
     */
    if ( BIG_ENDIAN_HOST )
    {
        /* Create little endian, single precision db as input to actual test. */
        rval = mc_open( j1, "data", "AwElPs", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0]  /* want swap_bytes TRUE */
                 || mode_test_data[1] != PREC_LIMIT_SINGLE )
                printf( "Test J1 abort - initial db opened incorrectly.\n" );
            else
            {
#endif
                rval = mc_close( fid );

                if ( rval == OK )
                {
                    rval = mc_open( j1, "data", "Aw", &fid );

                    if ( rval == OK )
                    {
#ifdef MODE_TEST
                        if ( mode_test_data[0]  /* want swap_bytes FALSE */
                             || mode_test_data[1] != PREC_LIMIT_DOUBLE )
                            printf( "Test J1 failed.\n" );
#endif
                        rval = mc_delete_family( j1, "data" );
                        if ( rval != OK )
                            mc_print_error( "Test J1 - post-test db clean-up failed", rval );
                    }
                    else
                        mc_print_error( "Test J1 abort - unable to open input db", rval );
                }
                else
                    mc_print_error( "Test J1 abort - failure closing input db before test", rval );
#ifdef MODE_TEST
            }
#endif
        }
        else
            printf( "Test J1 abort - unable to open db \"data/%s\".\n", little_e_db );
    }
    else
        printf( "Test J1 bypassed on little endian host.\n" );


    /*
     * Test case J2
     */
    if ( LITTLE_ENDIAN_HOST )
    {
        /* Create big endian, single precision db as input to actual test. */
        rval = mc_open( j2, "data", "AwEbPs", &fid );

        if ( rval == OK )
        {
#ifdef MODE_TEST
            if ( !mode_test_data[0]  /* want swap_bytes TRUE */
                 || mode_test_data[1] != PREC_LIMIT_SINGLE )
                printf( "Test J2 abort - initial db opened incorrectly.\n" );
            else
            {
#endif
                rval = mc_close( fid );

                if ( rval == OK )
                {
                    rval = mc_open( j2, "data", "Aw", &fid );

                    if ( rval == OK )
                    {
#ifdef MODE_TEST
                        if ( mode_test_data[0]  /* want swap_bytes FALSE */
                             || mode_test_data[1] != PREC_LIMIT_DOUBLE )
                            printf( "Test J2 failed.\n" );
#endif
                        rval = mc_delete_family( j2, "data" );
                        if ( rval != OK )
                            mc_print_error( "Test J2 - post-test db clean-up failed", rval );
                    }
                    else
                        mc_print_error( "Test J2 abort - unable to open input db", rval );
                }
                else
                    mc_print_error( "Test J2 abort - failure closing input db before test", rval );
#ifdef MODE_TEST
            }
#endif
        }
        else
            printf( "Test J2 abort - unable to open db \"data/%s\".\n", little_e_db );
    }
    else
        printf( "Test J2 bypassed on big endian host.\n" );


    /*
     * Clean up big_e_db and little_e_db if they were generated on this host.
     */
    if ( BIG_ENDIAN_HOST )
    {
        if ( !db_save )
        {
            if ( strcmp( big_e_db, a1 ) == 0 )
            {
                rval = mc_delete_family( big_e_db, "data" );
                if ( rval != OK )
                    mc_print_error( "Database clean-up failed for local big-endian db", rval );
            }

            if ( strcmp( little_e_db, a2 ) == 0 )
            {
                rval = mc_delete_family( little_e_db, "data" );
                if ( rval != OK )
                    mc_print_error( "Database clean-up failed for local little-endian db", rval );
            }
        }
    }
    else
    {
        if ( !db_save )
        {
            if ( strcmp( big_e_db, a4 ) == 0 )
            {
                rval = mc_delete_family( big_e_db, "data" );
                if ( rval != OK )
                    mc_print_error( "Database clean-up failed for local big-endian db", rval );
            }

            if ( strcmp( little_e_db, a5 ) == 0 )
            {
                rval = mc_delete_family( little_e_db, "data" );
                if ( rval != OK )
                    mc_print_error( "Database clean-up failed for local little-endian db", rval );
            }
        }
    }


#ifdef OLD_CRAP
    if ( rval == OK )
    {
        pid_t rpid;
        char cbuf[128], c2buf[128];
        char *p_bin_name;

        rpid = fork();
        if ( rpid == 0 )
        {
            int status;

            /* Child process */

            /* Gen path to "md" binary assuming this program is in the "test" subdirectory. */
            strcpy( cbuf, argv[0] );
            p_bin_name = strstr( cbuf, "mode_test" );
            if ( p_bin_name == NULL )
            {
                fprintf( stderr, "Test H1 child aborting after fork().\n" );
                exit( 1 );
            }
            else
            {
                strcpy( p_bin_name, "../src/md" );
                strcpy( c2buf, "data/" );
                strcat( c2buf, h1 );

                /* Run "md" on the created db. */
                execl( cbuf, "md", "-a", c2buf, (char *) 0 );
                
                fprintf( stderr, "Child returned from exec().\n" );
                perror( NULL );
            }
        }
        else
        {
            /* Wait for "md" to run. */
            waitpid( rpid, NULL, 0 );

            rval = mc_close( fid );
            if ( rval != OK )
            {
                mc_print_error( "Test H1 - post-test close failed", rval );
            }
            else
            {
                /*
                 * Test case H2 - Append to "empty" extant db.
                 */
                rval = mc_open( h1, "data", "Aa", &fid );
                if ( rval != OK )
                {
                    mc_print_error( "Test H2 abort - unable to open target db", rval );
                }
                else
                {
                    int val = 10;

                    rval = mc_wrt_scalar( fid, M_INT, "an_int_var", &val );
                    if ( rval != OK )
                        mc_print_error( "Test H2 - integer var write failed", rval );
                    else
                    {
                        rval = mc_close( fid );
                        if ( rval != OK )
                            mc_print_error( "Test H2 - post-test close failed", rval );
                        else
                        {
                            rpid = fork();
                            if ( rpid == 0 )
                            {
                                /* Child process */

                                /* Run "md" on the created db. */
                                execl( cbuf, "-a", c2buf, (char *) 0 );
                            }
                            else
                            {
                                /* Wait for "md" to run. */
                                waitpid( rpid, NULL, 0 );

                                rval = mc_delete_family( h1, "data" );
                                if ( rval != OK )
                                {
                                    mc_print_error( "Test H2 - post-test clean-up failed", rval );
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
        mc_print_error( "Test H1 abort - unable to open db", rval );
#endif

    exit( 0 );
}
