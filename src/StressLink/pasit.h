/* Public header */

#ifndef __pasit_h__
#define __pasit_h__

#define PASIT_VERSION "1.00.15"
#define PASIT_DATE    "January 05, 2009"

#define OK     0
#define NOT_OK 1

#define FALSE   0
#define TRUE    1

int passit_mat_input_ignore_list[1000];
int passit_mat_output_ignore_list[1000];

int OverlinkToStresslink(const char* olinkfilename,
                         const char* slinkfilename);

int StresslinkToOverlink(const char* slinkfilename,
                         const char* olinkfilename);

int StresslinkToStresslink(const char* slinkfilename_in,
                           const char* slinkfilename_out);

int OverlinkToOverlink(const char* olinkfilename_in,
                       const char* olinkfilename_out);

#endif

