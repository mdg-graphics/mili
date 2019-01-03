/* Public header */

#ifndef __socitume_h__
#define __socitume_h__

#define PASIT_VERSION "1.07"
#define PASIT_DATE    "September 18, 2008"

#define OK     0
#define NOT_OK 1

#define FALSE   0
#define TRUE    1

int OverlinkToStresslink(const char* olinkfilename,
                         const char* slinkfilename,
                         int nummats, int *ignore_list);

int StresslinkToOverlink(const char* slinkfilename,
                         const char* olinkfilename,
                         int nummats, int *ignore_list);

int StresslinkToStresslink(const char* slinkfilename_in,
                           const char* slinkfilename_out,
                           int nummats, int *ignore_list);

int OverlinkToOverlink(const char* olinkfilename_in,
                       const char* olinkfilename_out,
                       int nummats, int *ignore_list);

#endif

