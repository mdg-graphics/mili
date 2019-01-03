/*****************************************************************************
*
* Copyright (c) 2000 - 2012, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-442911
* All rights reserved.
*
* This file is  part of VisIt. For  details, see https://visit.llnl.gov/.  The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or other materials provided with the distribution.
*  - Neither the name of  the LLNS/LLNL nor the names of  its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED. IN  NO EVENT  SHALL LAWRENCE  LIVERMORE NATIONAL  SECURITY,
* LLC, THE  U.S.  DEPARTMENT OF  ENERGY  OR  CONTRIBUTORS BE  LIABLE  FOR  ANY
* DIRECT,  INDIRECT,   INCIDENTAL,   SPECIAL,   EXEMPLARY,  OR   CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

// ************************************************************************* //
//                      stressStrain.C                        //
// ************************************************************************* //

#include <stressStrainClass.h>
#include <math.h>

#include <vtkCellType.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkIdList.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>

#include <ExpressionException.h>
#include <avtExprNode.h>
#include <avtExpressionEvaluatorFilter.h>
#include <DebugStream.h>
#include <StringHelpers.h>

#include <vector>
#include <string>

#include <fileClass.h>
#include <transformClass.h>

//for if statements
#include <avtCurvatureExpression.h>
#include <avtDisplacementExpression.h>
#include <avtEffectiveTensorExpression.h>
#include <avtLocalizedCompactnessExpression.h>
#include <avtPrincipalDeviatoricTensorExpression.h>
#include <avtPrincipalTensorExpression.h>
#include <avtTensorMaximumShearExpression.h>

using namespace std;
//using namespace stringHelpers;

// ****************************************************************************
//  Method: avtStrainAlmansiExpression constructor
//
//  Programmer: Thomas R. Treadway
//  Creation:   Wed Nov 15 12:57:36 PST 2006
//
// ****************************************************************************

stressStrainClass::stressStrainClass()
{
    ;
}

// ****************************************************************************
//  Method: avtStrainAlmansiExpression destructor
//
//  Programmer: Thomas R. Treadway
//  Creation:   Wed Nov 15 12:57:36 PST 2006
//
// ****************************************************************************

stressStrainClass::~stressStrainClass()
{
    ;
}


// ****************************************************************************
//  Method: stressStrainClass::DoOperation
//
//  Purpose:
//      Determines the principals of a tensor.
//
//  Programmer: Hank Childs
//  Creation:   September 23, 2003
//
//  Modifications:
//
//    Hank Childs, Thu Jun 17 08:16:40 PDT 2004
//    Fix typo that caused bad derivation.
//
//    Hank Childs, Fri Jun  9 14:22:43 PDT 2006
//    Comment out currently unused variable.
//
//    Eric Brugger, Mon Aug  8 09:29:25 PDT 2011
//    I reduced the tolerance on invariant1 to be less restrictive.
//
// ****************************************************************************

void
stressStrainClass::DoOperation(vtkDataArray *in, vtkDataArray *out, int ncomps, int ntuples)
{
  int strn = GetStrainVariety();
  if (strn == 6){
      avtEffectiveTensorExpression temp1; 
      return temp1.DoOperation(in,out,ncomps,ntuples);
  }
  else if (strn == 8){
      avtPrincipalDeviatoricTensorExpression temp2;
      return temp2.DoOperation(in,out,ncomps,ntuples);
  }
  else if (strn == 9){
      avtPrincipalTensorExpression temp3;
      return temp3.DoOperation(in,out,ncomps,ntuples);
  }
  else{
    // error
    char msg[1024];
    sprintf(msg, " There is an error in how DoOperation is being called");
    EXCEPTION2(ExpressionException, outputVariableName, msg);
  }

}


// ****************************************************************************
//  Method: avtStrainAlmansiExpression::DeriveVariable
//
//  Purpose:
//      Determines the strain using Almansi
//
//  Programmer: Thomas R. Treadway
//  Creation:   Wed Nov 15 12:57:36 PST 2006
//
//  Modifications:
//    Kathleen Biagas, Wed Apr 12 12:00:10 PDT 2012
//    Set output array's data type to same as input's.
//
// ****************************************************************************

vtkDataArray *
stressStrainClass::DeriveVariable(vtkDataSet *in_ds)
{
  int strn = GetStrainVariety();
  if( strn == 0 || strn == 1 || strn == 2 || strn == 3){
    
        char msg[1024];
        double vals[3];
        double out2[9];
        // same as Griz variables
        double x[8], y[8], z[8];         // Initial element geom.
        double xx[8], yy[8], zz[8];      // Current element geom.
        double px[8], py[8], pz[8];      // Global derivates dN/dx,dN/dy,dN/dz.
        double F[9];                     // Element deformation gradient.
        double detF;                     // Determinant of element
                                       // deformation gradient.
        double strain[6];                // Calculated strain.
	double* strain2;
        int i, j, k;
        std::vector<int> cellsToIgnore;
        double avgTensor[9];             // ghost zone value
        int nTensors = 0;                // number of tensors in average

        // Let's get the points from the input dataset.
        if (in_ds->GetDataObjectType() != VTK_UNSTRUCTURED_GRID)
        {
          EXCEPTION2(ExpressionException, outputVariableName, "The strain expression only operates on unstructured grids.");
        }
        vtkUnstructuredGrid *in_usg = vtkUnstructuredGrid::SafeDownCast(in_ds);
        int nCells = in_usg->GetNumberOfCells();
	SetCoordData(in_ds->GetPointData()->GetArray(varnames[1]));
        //vtkDataArray *coord_data = GetCoordData();
        //vtkDataArray *primal_data = in_ds->GetCellData()->GetArray("strain");
        if (GetCoordData() == NULL)
        {
          sprintf(msg, "The strain expression "
              "could not extract the data array for: %s", varnames[1]);
          EXCEPTION2(ExpressionException, outputVariableName, msg);
        }
        vtkDataArray *ghost_data = in_ds->GetPointData()->GetArray("avtGhostNodes");
        unsigned char *ghost =
          (unsigned char *) (ghost_data ? ghost_data->GetVoidPointer(0) : 0);
        vtkIdList *pointIds = vtkIdList::New();
        vtkDataArray *out = (GetCoordData())->NewInstance();
        out->SetNumberOfComponents(9);
        out->SetNumberOfTuples(nCells);
        for (j = 0; j < 9; j++) 
          avgTensor[j] = 0.0;

	fileClass printer = fileClass("stressStrainOut.txt");
	if(GetPrinterOn() == 1){
	  printer.open();
	  printer.write(nCells);
	}
        for (i = 0; i < nCells; i++)
        {   // Check Voxel format
            int cellType = in_usg->GetCellType(i);
            // ignore everything but hexes
            if (cellType == VTK_HEXAHEDRON)
            {   // Grab a pointer to the cell's points' underlying data array
              in_usg->GetCellPoints(i, pointIds);
              bool anyGhost = false;
              if (ghost)
              {
                for (j = 0; j < 8; j++)
                {
                  if (ghost[pointIds->GetId(j)] != 0)
                  {   // any ghost nodes in this hex
                      anyGhost = true;
                      break;
                  }
                } 
              }
              if (anyGhost)            
              {
                cellsToIgnore.push_back(i);
                continue;            
              }
              for (j = 0; j < 8; j++)
              {   // Package initial element geometry points into vtkDataArray
                (GetCoordData())->GetTuple(pointIds->GetId(j), vals);
                x[j] = vals[0];
                y[j] = vals[1];
                z[j] = vals[2];
              }
              for (j = 0; j < 8; j++)
              {   // Package current element geometry points into vtkDataArray
                in_usg->GetPoint(pointIds->GetId(j), vals);
                xx[j] = vals[0];
                yy[j] = vals[1];
                zz[j] = vals[2];
              }
              //
              // This is where the strain algorithms start to differ
	      //int strn = GetStrainVariety();
	      if(strn == 0 || strn == 3){//if strain almansi or rate
	        stressStrainBase::HexPartialDerivative(px, py, pz, xx, yy, zz);
	      }
	      else{	      
	        stressStrainBase::HexPartialDerivative(px, py, pz, x, y, z);
	      }
              for (j = 0; j < 9; j++) 
                F[j] = 0.0;
              // Copied from Griz	 
	      if(strn == 0 || strn == 3){//if strain almansi or rate
	        for ( k = 0; k < 8; k++ ){
		  F[0] = F[0] + px[k]*x[k];
                  F[1] = F[1] + py[k]*x[k];
                  F[2] = F[2] + pz[k]*x[k];
                  F[3] = F[3] + px[k]*y[k];
                  F[4] = F[4] + py[k]*y[k];
                  F[5] = F[5] + pz[k]*y[k];
                  F[6] = F[6] + px[k]*z[k];
                  F[7] = F[7] + py[k]*z[k];
                  F[8] = F[8] + pz[k]*z[k];
	        }
	      }
	      else{
	        for ( k = 0; k < 8; k++ ){
       		  F[0] = F[0] + px[k]*xx[k];
                  F[1] = F[1] + py[k]*xx[k];
                  F[2] = F[2] + pz[k]*xx[k];
                  F[3] = F[3] + px[k]*yy[k];
                  F[4] = F[4] + py[k]*yy[k];
                  F[5] = F[5] + pz[k]*yy[k];
                  F[6] = F[6] + px[k]*zz[k];
                  F[7] = F[7] + py[k]*zz[k];
                  F[8] = F[8] + pz[k]*zz[k];
	        }

	      }
	      //same for all cases
              detF = F[0]*F[4]*F[8] + F[1]*F[5]*F[6]
                   + F[2]*F[3]*F[7]
                   - F[2]*F[4]*F[6] - F[1]*F[3]*F[8]
                   - F[0]*F[5]*F[7];
              detF = fabs( detF );
	    
	      if(strn == 0){//if strain almansi
	        strain[0] = -0.5*(F[0]*F[0] + F[3]*F[3] + F[6]*F[6] - 1.0);     
	        strain[1] = -0.5*(F[1]*F[1] + F[4]*F[4] + F[7]*F[7] - 1.0);     
	        strain[2] = -0.5*(F[2]*F[2] + F[5]*F[5] + F[8]*F[8] - 1.0);     
	        strain[3] = -0.5*(F[0]*F[1] + F[3]*F[4] + F[6]*F[7] );
	        strain[4] = -0.5*(F[1]*F[2] + F[4]*F[5] + F[7]*F[8] );
	        strain[5] = -0.5*(F[0]*F[2] + F[3]*F[5] + F[6]*F[8] );
	      }
	      else if(strn == 1){//if strain Green
   	        strain[0] = 0.5*(F[0]*F[0] + F[3]*F[3] + F[6]*F[6] - 1.0);     
	        strain[1] = 0.5*(F[1]*F[1] + F[4]*F[4] + F[7]*F[7] - 1.0);     
	        strain[2] = 0.5*(F[2]*F[2] + F[5]*F[5] + F[8]*F[8] - 1.0);     
	        strain[3] = 0.5*(F[0]*F[1] + F[3]*F[4] + F[6]*F[7] );
	        strain[4] = 0.5*(F[1]*F[2] + F[4]*F[5] + F[7]*F[8] );
	        strain[5] = 0.5*(F[0]*F[2] + F[3]*F[5] + F[6]*F[8] );
	      }
	      else if(strn == 2){//if strain Infi
	        strain[0] = F[0] - 1.0;     
	        strain[1] = F[4] - 1.0;     
	        strain[2] = F[8] - 1.0;     
	        strain[3] = 0.5 * ( F[1] + F[3] );
	        strain[4] = 0.5 * ( F[5] + F[7] );
	        strain[5] = 0.5 * ( F[2] + F[6] );
	      }
	      else if(strn == 3){//if strain rate
	        strain[0] = F[0];     
	        strain[1] = F[4];     
	        strain[2] = F[8];     
	        strain[3] = 0.5 * ( F[1] + F[3] );
	        strain[4] = 0.5 * ( F[5] + F[7] );
	        strain[5] = 0.5 * ( F[2] + F[6] );
	      }
	      else{
		// error
		char msg[1024];
		sprintf(msg, " You are using a value that is not currently implemented in the stressStrain code ");
		EXCEPTION2(ExpressionException, outputVariableName, msg);
	      }
              // End of differences
              //
              // reorder Tensor
              out2[0] = strain[0];  // XX
              out2[1] = strain[3];  // XY
              out2[2] = strain[5];  // XZ
              out2[3] = strain[3];  // YX
              out2[4] = strain[1];  // YY
              out2[5] = strain[4];  // YZ
              out2[6] = strain[5];  // ZX
              out2[7] = strain[4];  // ZY
              out2[8] = strain[2];  // ZZ
              nTensors++;
              for (j = 0; j < 9; j++)
                avgTensor[j] += out2[j];
            } 
	    else if(cellType == VTK_QUAD)
	    { //this is the new section to be coded

	      // Grab a pointer to the cell's points' underlying data array
              in_usg->GetCellPoints(i, pointIds);
	      // Package initial element geometry points into vtkDataArray
              (GetCoordData())->GetTuple(pointIds->GetId(3), vals);
	      strain2 = GetData((double*)vals);
	    //new code	      
	    bool global = true;
	    bool tensorTrans = true;
	    int surface = 0;//MIDDLE
	    int obj_qty = 1;//temporary fix
	    bool interpolate = true;
	    double resultElem[6] = {0.,0.,0.,0.,0.,0.};
	    int object_ids[6] = {0,1,2,3,4,5};
	    double res1[6] = {1.,2.,3.,4.,5.,6.};
	    double res2[6] = {1.,2.,3.,4.,5.,6.};
	    double eps[6] = {1.,2.,3.,4.,5.,6.};
	    int comp_indx = 0; 
	    double quadResult = 0.0;
	    int temp = 9;          
	    double** tempMat;

	    //initialize the transformation matrix
	    for(j = 0; j < 9; j++){
	      //SetTransMat((j/3),(j%3),1);
	    }

	    if(GetReferenceLocation()){//GLOBAL
	      if(GetPrinterOn()){
		  switch(GetQuadraturePoint()){
		        case 0://MIDDLE
			  strain2 = GetData(0,vals);
			  res1[0] = *(strain2+0);  
			  res1[1] = *(strain2+1);  
			  res1[2] = *(strain2+2);  
			  res1[3] = *(strain2+3);  
			  res1[4] = *(strain2+4);  
			  res1[5] = *(strain2+5);  

		       

			  //need to finish equivalent function	
			  //res1 = transform_stress_strain(1,res1,GetTransMat());

			  strain2 = GetData(1,vals);
			  res2[0] = *(strain2+0);  
			  res2[1] = *(strain2+1);  
			  res2[2] = *(strain2+2);  
			  res2[3] = *(strain2+3);  
			  res2[4] = *(strain2+4);  
			  res2[5] = *(strain2+5);  

			  //need to finish equivalent function
			  //res2 = transform_stress_strain(1,res2,GetTransMat());

			  for( j = 0; j < temp; j++){
			    out2[j] = 0.5 * (res1[(j%6)] + res2[(j%6)]);
			  }		   
			  break;
		        case 1://INNER
			  //res2 = GetData(vals);
			  strain2 = GetData(1,vals);
			  res2[0] = *(strain2+0);  
			  res2[1] = *(strain2+1);  
			  res2[2] = *(strain2+2);  
			  res2[3] = *(strain2+3);  
			  res2[4] = *(strain2+4);  
			  res2[5] = *(strain2+5);  
			  //res2 = transform_stress_strain(1,res2,GetTransMat());
			  break;
		        case 2://OUTER
			  //res2 = GetData(vals);
			  strain2 = GetData(1,vals);
			  res2[0] = *(strain2+0);  
			  res2[1] = *(strain2+1);  
			  res2[2] = *(strain2+2);  
			  res2[3] = *(strain2+3);  
			  res2[4] = *(strain2+4);  
			  res2[5] = *(strain2+5);  
			  //res2 = transform_stress_strain(1,res2,GetTransMat());
			  break;
		    }
		}
		else{
		    switch(GetQuadraturePoint()){
		        case 0://MIDDLE
			  //res1 = GetData(0,vals);
			  strain2 = GetData(0,vals);
			  res1[0] = *(strain2+0);  
			  res1[1] = *(strain2+1);  
			  res1[2] = *(strain2+2);  
			  res1[3] = *(strain2+3);  
			  res1[4] = *(strain2+4);  
			  res1[5] = *(strain2+5);  

			  //res2 = GetData(1,vals);
			  strain2 = GetData(1,vals);
			  res2[0] = *(strain2+0);  
			  res2[1] = *(strain2+1);  
			  res2[2] = *(strain2+2);  
			  res2[3] = *(strain2+3);  
			  res2[4] = *(strain2+4);  
			  res2[5] = *(strain2+5);  
			        for( j = 0; j < temp; j++){
			            out2[j] = 0.5 * (res1[j%6] + res2[j%6]);
				}
			    break;
		        case 1://INNER			    
			  //res2 = GetData(0,vals);
			  strain2 = GetData(0,vals);
			  res2[0] = *(strain2+0);  
			  res2[1] = *(strain2+1);  
			  res2[2] = *(strain2+2);  
			  res2[3] = *(strain2+3);  
			  res2[4] = *(strain2+4);  
			  res2[5] = *(strain2+5); 
			  break;
		        case 2://OUTER
			  //res2 = GetData(1,vals);
			  strain2 = GetData(1,vals);
			  res2[0] = *(strain2+0);  
			  res2[1] = *(strain2+1);  
			  res2[2] = *(strain2+2);  
			  res2[3] = *(strain2+3);  
			  res2[4] = *(strain2+4);  
			  res2[5] = *(strain2+5);  
			  break;
		    }
		}		
		if(surface != 0 && object_ids != NULL){
		    for(j = 0; j < temp; j++){
		        out2[j] = res2[j%6];
		    }
		}

	    }
	    else{//LOCAL
	        switch(GetQuadraturePoint()){
    	            case 0://MIDDLE
		      //res1 = GetData(0,vals);
		      strain2 = GetData(0,vals);
		      res1[0] = *(strain2+0);  
		      res1[1] = *(strain2+1);  
		      res1[2] = *(strain2+2);  
		      res1[3] = *(strain2+3);  
		      res1[4] = *(strain2+4);  
		      res1[5] = *(strain2+5);  

		      //res2 = GetData(1,vals);
		      strain2 = GetData(1,vals);
		      res2[0] = *(strain2+0);  
		      res2[1] = *(strain2+1);  
		      res2[2] = *(strain2+2);  
		      res2[3] = *(strain2+3);  
		      res2[4] = *(strain2+4);  
		      res2[5] = *(strain2+5);  

		      for(j = 0; j < 6; j++){
		        res1[j] = 0.5 * ((double) res1[j] + res2[j]);
		      }
		      //needs to be finished
		      //global_to_local_mtx( );
		      //res1 = transform_tensors(1,res1,GetTransMat());	
		    
		      for(j = 0; j < temp; j++){
		        out2[j] = res1[j%6];
		      }
		      break;
		    case 1://INNER

		      //res1 = GetData(0,vals);
		      strain2 = GetData(0,vals);
		      res1[0] = *(strain2+0);  
		      res1[1] = *(strain2+1);  
		      res1[2] = *(strain2+2);  
		      res1[3] = *(strain2+3);  
		      res1[4] = *(strain2+4);  
		      res1[5] = *(strain2+5);  

		      //res2 = GetData(1,vals);
		      strain2 = GetData(1,vals);
		      res2[0] = *(strain2+0);  
		      res2[1] = *(strain2+1);  
		      res2[2] = *(strain2+2);  
		      res2[3] = *(strain2+3);  
		      res2[4] = *(strain2+4);  
		      res2[5] = *(strain2+5);  

		      //needs to be finished
		        // the current version that we copied over from frame does not use the values that we have implemented.
		        // it is trying to use the node values to perform the transformation.
		        // once we have that information we simply need to pass it into the function and proceed normally.
		       
		        //G2LMatrixShell( );
		        //res1 = transform_tensors(1,res1,GetTransMat());
			    
		      for(j = 0; j < temp; j++){
		        out2[j] = res1[i%6];
		      }
		      break;
		    case 2://OUTER

		      //res1 = GetData(0,vals);
		      strain2 = GetData(0,vals);
		      res1[0] = *(strain2+0);  
		      res1[1] = *(strain2+1);  
		      res1[2] = *(strain2+2);  
		      res1[3] = *(strain2+3);  
		      res1[4] = *(strain2+4);  
		      res1[5] = *(strain2+5);  

		      //res2 = GetData(1,vals);
		      strain2 = GetData(1,vals);
		      res2[0] = *(strain2+0);  
		      res2[1] = *(strain2+1);  
		      res2[2] = *(strain2+2);  
		      res2[3] = *(strain2+3);  
		      res2[4] = *(strain2+4);  
		      res2[5] = *(strain2+5);  

		      //needs to be finished
		      //global_to_local_mtx( );
		      //res1 = transform_tensors(1,res1,GetTransMat());	
		    
		      for(j = 0; j < temp; j++){
		        out2[j] = res1[i%6];
		      }
	       	      break;		    
		}
	    }
	    
	    PrintArguments();
	    PrintGlobals();
	  }
          else 
          { // cell is not a hexhedron also ignore              
            cellsToIgnore.push_back(i);
          }
          out->SetTuple(i, out2);
	  if(GetPrinterOn() == 1){
	    printer.write(i,cellType,out2[5]);
	  }
        }
	if(GetPrinterOn() == 1){
	  printer.close();
	}
        if (nTensors > 0)
        {   
          for (j = 0; j < 9; j++)
            avgTensor[j] = avgTensor[j]/nTensors;
        }
        for (i = 0; i < cellsToIgnore.size(); i++)
        {   
          out->SetTuple(cellsToIgnore[i], avgTensor);
        }
        return out;
    }
  else{
        char msg[1024];
      sprintf(msg, "The strain expression "
          "could not extract the data array for: %s", varnames[1]);
      EXCEPTION2(ExpressionException, outputVariableName, msg);
      return NULL;   
  }
}

// ****************************************************************************
//  Method: avtStrainAlmansiExpression::GetVariableDimension
//
//  Purpose:
//      Determines what the variable dimension of the output is.
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov 28 09:56:22 PST 2006
//
// ****************************************************************************

int
stressStrainClass::GetNumberOfComponentsInOutput(int ncompsIn1)
{
  int variety = GetStrainVariety();
  switch(variety){
    case 4:
      return 1;
      break;
    case 6:
      return 1;
      break;
    case 7:
      return 1;
      break;
    case 8:
      return 3;
      break;
    case 9:
      return 3;
      break;
    case 10:
      return 1;
      break;
  }
}

// ****************************************************************************
//  Method: avtStrainAlmansiExpression::GetVariableDimension
//
//  Purpose:
//      Determines what the variable dimension of the output is.
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov 28 09:56:22 PST 2006
//
// ****************************************************************************

int
stressStrainClass::GetNumberOfComponentsInOutput(int ncompsIn1, int ncompsIn2)
{
   return (ncompsIn1 > ncompsIn2 ? ncompsIn1 : ncompsIn2);
}

// ****************************************************************************
//  Method: avtStrainAlmansiExpression::GetVariableDimension
//
//  Purpose:
//      Determines what the variable dimension of the output is.
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov 28 09:56:22 PST 2006
//
// ****************************************************************************

int
stressStrainClass::GetVariableDimension(void)
{
    if (*(GetInput()) == NULL)
        return avtMultipleInputExpressionFilter::GetVariableDimension();
    if (varnames.size() != 2)
        return avtMultipleInputExpressionFilter::GetVariableDimension();

    avtDataAttributes &atts = GetInput()->GetInfo().GetAttributes();
    if (! atts.ValidVariable(varnames[0]))
        return avtMultipleInputExpressionFilter::GetVariableDimension();
    int ncomp1 = atts.GetVariableDimension(varnames[0]);
    
    if (! atts.ValidVariable(varnames[1]))
        return avtMultipleInputExpressionFilter::GetVariableDimension();
    int ncomp2 = atts.GetVariableDimension(varnames[1]);
    
    return GetNumberOfComponentsInOutput(ncomp1, ncomp2);
}

void
stressStrainClass::PrintArguments()
{
  fileClass printer = fileClass("realArgsOut.txt");
  printer.open();
  for(int i = 0; i < varnames.size(); i++)
    printer.write(varnames[i]);
  printer.close();
}

void
stressStrainClass::PrintGlobals()
{
  fileClass printer = fileClass("realGlobalsOut.txt");
  printer.open();
  printer.write(GetQuadraturePoint());
  printer.write(GetReferenceLocation());
  printer.write(GetStrainVariety());
  printer.write(GetStrainType());
  printer.write(GetPrinterOn());
  printer.close();
}

double*
stressStrainClass::GetData(double* vals)
{
  double result[6];
  //this should be using the primal data and subrect data 
  //to generate results. for now we are just going to copy 
  //the first element of the coord data into the function
  double lVals[3]; 
  //lVals = (double[3])*vals;
  lVals[0] = *(vals+0);
  lVals[1] = *(vals+1);
  lVals[2] = *(vals+2);
  //lVals = *vals;
  for(int i = 0; i < 6; i++){
    result[i] = lVals[1];
  }

  return result;
}

double*
  stressStrainClass::GetData(int choice,double* vals)
{
  double result[6];
  //this should be using the primal data and subrect data 
  //to generate results. for now we are just going to copy 
  //the first element of the coord data into the function
  double lVals[3]; 
  //lVals = (double[3])*vals;
  lVals[0] = *(vals+0);
  lVals[1] = *(vals+1);
  lVals[2] = *(vals+2);
  //lVals = *vals;
  for(int i = 0; i < 6; i++){
    result[i] = lVals[(choice%3)];
  }

  return result;
}
