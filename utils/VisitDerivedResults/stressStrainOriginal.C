//begin
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

#include <vtkCellType.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkIdList.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkMath.h>
#include <vtkCurvatures.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>

#include <ExpressionException.h>
#include <avtExprNode.h>
#include <avtExpressionEvaluatorFilter.h>
#include <DebugStream.h>
#include <StringHelpers.h>

//voids
#include <avtPrincipalDeviatoricTensorExpression.h>
#include <avtPrincipalTensorExpression.h>
#include <avtEffectiveTensorExpression.h>

//dataArrays
#include <avtStrainAlmansiExpression.h>
#include <avtStrainGreenLagrangeExpression.h>
#include <avtStrainInfinitesimalExpression.h>
#include <avtStrainRateExpression.h>
#include <avtTensorMaximumShearExpression.h>
#include <avtCurvatureExpression.h>
#include <avtDisplacementExpression.h>
#include <avtLocalizedCompactnessExpression.h>

//other
#include <avtStrainTensorExpression.h>

using namespace std;

#include "stressStrainClass.h"


  //  INPUT:
  // OUTPUT:
stressStrainClass::stressStrainClass(){
  ;
} 
  //  INPUT:
  // OUTPUT:
stressStrainClass::~stressStrainClass(){
  ;
}

  //  INPUT:
  // OUTPUT:
stressStrainClass::stressStrainClass(int x){
  ;
}

  //  INPUT:
  // OUTPUT:
void stressStrainClass::performCalculation(){
  ;
}

  //  INPUT:
  // OUTPUT:
double stressStrainClass::performCalculation(string in){
  double temp = 0.0;

  

  return temp;
}
  // function used to decode a string representing the 
  // possible different calculations.
  // Ex: "strain_alamansi"
  // Note this could be done elsewhere and just recieve a number for the calculation
  //  INPUT:
  // OUTPUT:
int stressStrainClass::calculateID(string in){

  if(true){

  }
  else{

  }

  return 0;
}

  // function used to ficure out the numerical value of a specified computation
  //
  // Ex: "strain_alamansi"
  // Note this could be done elsewhere and just recieve a number for the computation
  //  INPUT:
  // OUTPUT:
int stressStrainClass::computationID(string in){

  if(true){

  }
  else{

  }

  return 0;
}
//to accomodate the functions that are expected to return through the out variable
//void stressStrainClass::DoOperation(vtkDataArray* in, vtkDataArray* out, int ncomps, int ntuples)
//{
  //we want this operation to eventually be able to recieve a number that is 
  //indicative of the specific operation that is desired
  //for now we will just pass along the information to an example function 
  //and act primarily as a middle man
  
  //this will replace Effective Tensor expression for starters
  //avtEffectiveTensorExpression temp;
  //temp.DoOperation(in,out,ncomps,ntuples);


//}
//to accomodate the functions that are supposed to return a dataArray
vtkDataArray* stressStrainClass::DeriveVariable(vtkDataSet* in_ds)
{
  //we want this operation to eventually be able to recieve a number that is 
  //indicative of the specific operation that is desired
  //for now we will just pass along the information to an example function 
  //and act primarily as a middle man
  
  //this will replace Almansi Strain expression for starters
  avtStrainAlmansiExpression temp;
  return temp.DeriveVariable(in_ds);
  //return avtStrainAlmansiExpression::DeriveVariable(in_ds);


} 

//
int
stressStrainClass::GetVariableDimension(void)
{
  avtStrainAlmansiExpression temp;
  return temp.GetVariableDimension();
}


//end
