#ifndef MATRIX_OPS_H
#define MATRIX_OPS_H

#include "draw.h"
#include "geometric.h"

typedef struct Matrix_stack
{
  struct Matrix_stack * prev;
  struct Matrix_stack * next;
  Transf_mat tmat;
} Matrix_stack;


GLenum gl_mat_mode = GL_MODELVIEW;
Matrix_stack * gl_mats[4] = {NULL};

inline int mode_to_idx( GLenum mode )
{
  int idx = mode == GL_MODELVIEW ? 0 :
            mode == GL_PROJECTION ? 1 :
            mode == GL_TEXTURE ? 2 :
            mode == GL_COLOR ? 3 : -1;
  //assert( idx >= 0 && idx <= 3 );
  return idx;
}

inline int idx_to_mat( int idx )
{
  //assert( idx >= 0 && idx <= 3 );
  GLenum enums[] = { GL_MODELVIEW_MATRIX, GL_PROJECTION_MATRIX, GL_TEXTURE_MATRIX, GL_COLOR_MATRIX };
  return enums[idx];
}

void mat_init( )
{
  int midx;
  for(midx = 0; midx < 4; ++midx)
  {
    gl_mats[midx] = (Matrix_stack*) malloc(sizeof(Matrix_stack));
    gl_mats[midx]->prev = NULL;
    gl_mats[midx]->next = NULL;
    mat_copy( &(gl_mats[midx]->tmat), &ident_matrix);
  }
}

inline const Transf_mat * get_mode_matrix( GLenum mode )
{
  return &(gl_mats[mode_to_idx(mode)]->tmat);
}

void matrix_mode( GLenum mode )
{
  glMatrixMode( mode );
  mode_to_idx( mode );// just to check the mode is valid
  gl_mat_mode = mode;
}

void push_matrix( )
{
  glPushMatrix();
  Matrix_stack * current_mat = gl_mats[mode_to_idx( gl_mat_mode )];
  Matrix_stack * new_mat = (Matrix_stack*) malloc(sizeof(Matrix_stack));
  current_mat->prev = new_mat;
  new_mat->next = current_mat;
  mat_copy(&new_mat->tmat,&current_mat->tmat);
  gl_mats[mode_to_idx( gl_mat_mode )] = new_mat;
}

void pop_matrix( )
{
  glPopMatrix();
  Matrix_stack * current_mat = gl_mats[mode_to_idx( gl_mat_mode )];
  Matrix_stack * next_mat = current_mat->next;
  gl_mats[mode_to_idx( gl_mat_mode )] = next_mat;
  next_mat->prev = NULL;
  free(current_mat);
}

void load_identity( )
{
  glLoadIdentity();
  mat_copy(&( gl_mats[mode_to_idx( gl_mat_mode )]->tmat),&ident_matrix);
}

void translate( float x, float y, float z )
{
  glTranslatef( x, y, z );
  Matrix_stack * glmv = gl_mats[mode_to_idx( gl_mat_mode )];
  Transf_mat t;
  mat_copy(&t, &ident_matrix);
  t.mat[3][0] = x;
  t.mat[3][1] = y;
  t.mat[3][2] = z;
  mat_mul( &(glmv->tmat), &(glmv->tmat), &t );
}

inline void translatev( float xyz[3] )
{
  translate( xyz[0], xyz[1], xyz[2] );
}

void scale( float x, float y, float z )
{
  glScalef( x, y, z );
  float wmv[4][4];
  Matrix_stack * glmv = gl_mats[ mode_to_idx( gl_mat_mode ) ];
  float (* cm)[4][4] = &(glmv->tmat.mat);
  memcpy(&wmv[0][0],&cm[0][0],sizeof(float)*16);
  float scl[4] = { x, y, z, 1.0 };
  int ii, jj;
  for( ii = 0; ii < 4; ++ii )
  {
    for( jj = 0; jj < 4; ++jj )
    {
      (*cm)[jj][ii] = wmv[jj][ii] * scl[ii];
    }
  }
}

inline void scalev( float xyz[3] )
{
  scale( xyz[0], xyz[1], xyz[2] );
}

void rotate( float a, float x, float y, float z )
{
  glRotatef( a, x, y, z );
  // normalize the xyz vector
  float n = 1.0 / sqrt( x*x + y*y + z*z );
  x *= n;
  y *= n;
  z *= n;
  // calc trig data
  float c = cos(a);
  float s = sin(a);
  float omc = 1 - c;
  //
  Transf_mat r;
  r.mat[0][0] = x*x * omc + c;
  r.mat[0][1] = x*y * omc - z*s;
  r.mat[0][2] = x*z * omc + y*s;
  r.mat[0][3] = 0;

  r.mat[1][0] = y*x * omc + z*s;
  r.mat[1][1] = y*y * omc + c;
  r.mat[1][2] = y*z * omc - x*s;
  r.mat[2][2] = 0;

  r.mat[2][0] = z*x * omc - y*s;
  r.mat[2][1] = z*y * omc + x*s;
  r.mat[2][2] = z*z * omc + c;
  r.mat[2][3] = 0;

  r.mat[3][0] = r.mat[3][1] = r.mat[3][2] = 0;
  r.mat[3][3] = 1;

  Matrix_stack * glmv = gl_mats[ mode_to_idx( gl_mat_mode ) ];
  mat_mul(&glmv->tmat, &r, &glmv->tmat);
}

inline void rotatev( float a, float xyz[3] )
{
  rotate( a, xyz[0], xyz[1], xyz[2] );
}

void mode_matrix_multiply( float mat[16] )
{
  glMultMatrixf( mat );
  Matrix_stack * glmv = gl_mats[ mode_to_idx( gl_mat_mode ) ];
  Transf_mat mmat;
  array_to_mat( mat, &mmat );
  mat_mul(&glmv->tmat, &mmat, &glmv->tmat);
}

#endif
