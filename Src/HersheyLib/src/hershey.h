
/*
 * Function declarations for Hershey Font Library.
 */

extern void hmove( float, float, float );
extern void hmove2( float, float );
extern void hrmv( float, float, float );
extern void hrdr( float, float, float );
extern void hfont( char * );
extern int hnumchars( void );
extern void hsetpath( char * );
extern void hgetcharsize( char, float *, float * );
extern void hdrawchar( int );
extern void htextsize( float, float );
extern float hgetfontwidth( void );
extern float hgetfontheight( void );
extern void hgetfontsize( float *, float * );
extern float hgetdescender( void );
extern float hgetascender( void );
extern void hcharstr( char * );
extern float hstrlength( char * );
extern void hboxtext( float, float, float, float, char * );
extern void hboxfit( float, float, int );
extern void hcentertext( int );
extern void hrightjustify( int );
extern void hleftjustify( int );
extern void hfixedwidth( int );
extern void htextang( float );

