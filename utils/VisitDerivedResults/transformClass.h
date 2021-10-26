//Chadway Cooper 
//Sept 7, 2012


#ifndef TRANSFORM_CLASS_H
#define TRANSFORM_CLASS_H

class transformClass
{
 public:
  void    hex_g2l_mtx(double x[8], double y[8], double z[8], int n1, int n2, int n3, double localMat[3][3] );
  void    G2LMatrixShell(double x[4], double y[4], double z[4], double localMat[3][3]);
  bool    TransformStressStrain(double input[][6], double trans_matrix[3][3], double *res_array);
  double* TransformTensors(int qty, double tensors[][6], double mat[][3]);
  bool    TransposeTensors(int qty, char* name, double tensor[][6], double *res_array);

  double**     GetTransMat(){return transMat;};
  double       GetTransMat(int x, int y){return *(*(transMat+x)+y);};
  void         SetTransMat(double** input){transMat = input;};
  void         SetTransMat(int x, int y, double val){*(*(transMat+x)+y) = val;};
  void         SetTransMat(int node1, int node2, int node3);

 private:
  double**  transMat;

};

#endif

  
