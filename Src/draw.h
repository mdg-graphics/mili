/* $Id$ */
/* 
 * draw.h - Definitions for display routines.
 * 
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Oct 23 1991
 *
 * Copyright (c) 1991 Regents of the University of California
 */

#ifndef DRAW_H
#define DRAW_H

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include "misc.h"
#include "geometric.h"


#define CMAP_SIZE (256)


typedef struct 
{
    int win_x;
    int win_y;
    GLsizei vp_width;
    GLsizei vp_height;
    float aspect_correct;         /* Aspect correction for video output. */

    Bool_type orthographic;
    float cam_angle;
    float look_from[3];
    float look_at[3];
    float look_up[3];
    GLdouble near;
    GLdouble far;

    Transf_mat rot_mat;
    float trans[3];
    float scale[3];

    float bbox_trans[3];
    float bbox_scale;

    GLfloat matl_ambient[MAX_MATERIALS][4];
    GLfloat matl_diffuse[MAX_MATERIALS][4];
    GLfloat matl_specular[MAX_MATERIALS][4];
    GLfloat matl_shininess[MAX_MATERIALS];
    GLfloat matl_emission[MAX_MATERIALS][4];
    int current_material;

    GLfloat backgrnd_color[3];
    GLfloat foregrnd_color[3];
    GLfloat contour_color[3];
    GLfloat hilite_color[3];
    GLfloat select_color[3];
    GLfloat vector_color[3];
    GLfloat vector_hd_color[3];
    GLfloat cmap_min_color[3];   /* Last five tied to a colormap. */
    GLfloat cmap_max_color[3];
    GLfloat rmin_color[3];
    GLfloat rmax_color[3];
    GLfloat colormap[CMAP_SIZE][3];

    Bool_type lighting;
    Bool_type light_active[6];
    float light_pos[6][4];

    char vid_title[4][80];

} View_win_obj;

extern View_win_obj *v_win;


/*
 * Some useful RGB triples.
 */
extern GLfloat black[];
extern GLfloat white[];
extern GLfloat red[];
extern GLfloat green[];
extern GLfloat blue[];
extern GLfloat magenta[];
extern GLfloat cyan[];
extern GLfloat yellow[];


#endif /* DRAW_H */
