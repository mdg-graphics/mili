//begin
#ifndef STRESS_STRAIN_CLASS_H
#define STRESS_STRAIN_CLASS_H

#include <avtStrainTensorExpression.h>
//#include <string>

class EXPRESSION_API stressStrainClass : public avtStrainTensorExpression
{
  //self contained info
 private:

  /*required information for calculations
  //cellType;
  //value1
  //value2
  //. . . .
  */
  /*parameters
  int global;
  int referenceSurface;
  */
 public:

  //constructors
  stressStrainClass();
  virtual ~stressStrainClass();
  stressStrainClass(int x);

  virtual const char *GetType(void){return "stressStrainClass";};
  virtual const char *GetDescription(void){return "Calculating the Almansi Stress";};
  virtual int NumVariableArguments(){return 2;};

  //generic calculate
  void performCalculation();
  //focused calculation
  double performCalculation(std::string in);
  //convert value for
  int calculateID(std::string in);
  int computationID(std::string in);
  //String setTransMatrix();
  //void getTransMatrix();
  //virtual void DoOperation(vtkDataArray* in, vtkDataArray* out, int ncomps, int ntuples);
 protected:
  virtual vtkDataArray* DeriveVariable(vtkDataSet* in_ds);
  virtual avtVarType GetVariableType(void){return AVT_TENSOR_VAR;};
  virtual int GetNumberOfComponentsInOutput(int ncompsIn1, int ncompsIn2){return (ncompsIn1 > ncompsIn2 ? ncompsIn1 : ncompsIn2);}
  virtual int GetVariableDimension(void);
};

#endif
//end
