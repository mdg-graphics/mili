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
//                       stressStrainBase.C                         //
// ************************************************************************* //

#include <stressStrainBase.h>

#include <vtkDataArray.h>
#include <ExpressionException.h>
#include <avtExprNode.h>
#include <avtExpressionEvaluatorFilter.h>
#include <DebugStream.h>
#include <StringHelpers.h>
#include <ctype.h>
#include <fileClass.h>

using namespace StringHelpers;

// ****************************************************************************
//  Method:  stressStrainBase constructor
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov 14 12:59:38 PST 2006
//
// ****************************************************************************

stressStrainBase::stressStrainBase()
: quadraturePoint(INNER), referenceLoc(GLOBAL)
{
    ;
}

// ****************************************************************************
//  Method:  stressStrainBase destructor
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov 14 12:59:38 PST 2006
//
// ****************************************************************************

stressStrainBase::~stressStrainBase()
{
    ;
}

// ****************************************************************************
//  Method:  stressStrainBase::HexPartialDerivative
//
//  Purpose:
//      Computes the the partial derivative of the 8 brick shape
//      functions with respect to each coordinate axis.  The
//      coordinates of the brick are passed in "coorX,Y,Z", and
//      the partial derivatives are returned in "dNx,y,z".
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov 14 12:59:38 PST 2006
//
// ****************************************************************************

void
stressStrainBase::HexPartialDerivative
    (double dNx[8], double dNy[8], double dNz[8],
    double coorX[8], double coorY[8], double coorZ[8])
{   // copied from Griz
//  Local shape function derivatives evaluated at node points.
    static double dN1[8] = { -.125, -.125, .125, .125,
                            -.125, -.125, .125, .125 };
    static double dN2[8] = { -.125, -.125, -.125, -.125,
                            .125, .125, .125, .125 };
    static double dN3[8] = { -.125, .125, .125, -.125,
                            -.125, .125, .125, -.125};
    double jacob[9], invJacob[9], detJacob;

    for (int k = 0; k < 9; k++)
       jacob[k] = 0.;
    for (int k = 0; k < 8; k++ )
    {   
        jacob[0] += dN1[k]*coorX[k];
        jacob[1] += dN1[k]*coorY[k];
        jacob[2] += dN1[k]*coorZ[k];
        jacob[3] += dN2[k]*coorX[k];
        jacob[4] += dN2[k]*coorY[k];
        jacob[5] += dN2[k]*coorZ[k];
        jacob[6] += dN3[k]*coorX[k];
        jacob[7] += dN3[k]*coorY[k];
        jacob[8] += dN3[k]*coorZ[k];
    }  
    detJacob =   jacob[0]*jacob[4]*jacob[8] + jacob[1]*jacob[5]*jacob[6]
               + jacob[2]*jacob[3]*jacob[7] - jacob[2]*jacob[4]*jacob[6] 
               - jacob[1]*jacob[3]*jacob[8] - jacob[0]*jacob[5]*jacob[7];
    
    if ( fabs( detJacob ) < 1.0e-20 )
    {   
        EXCEPTION2(ExpressionException, outputVariableName,
                "HexPartialDerivative, Element is degenerate! Result is invalid!");
    }
    
    /* Develop inverse of mapping. */                         
    detJacob = 1.0 / detJacob;                                
                                                              
    /* Cofactors of the jacobian matrix. */                   
    invJacob[0] = detJacob * (  jacob[4]*jacob[8] - jacob[5]*jacob[7] );    
    invJacob[1] = detJacob * ( -jacob[1]*jacob[8] + jacob[2]*jacob[7] );    
    invJacob[2] = detJacob * (  jacob[1]*jacob[5] - jacob[2]*jacob[4] );    
    invJacob[3] = detJacob * ( -jacob[3]*jacob[8] + jacob[5]*jacob[6] );    
    invJacob[4] = detJacob * (  jacob[0]*jacob[8] - jacob[2]*jacob[6] );    
    invJacob[5] = detJacob * ( -jacob[0]*jacob[5] + jacob[2]*jacob[3] );    
    invJacob[6] = detJacob * (  jacob[3]*jacob[7] - jacob[4]*jacob[6] );    
    invJacob[7] = detJacob * ( -jacob[0]*jacob[7] + jacob[1]*jacob[6] );    
    invJacob[8] = detJacob * (  jacob[0]*jacob[4] - jacob[1]*jacob[3] );    
                                                              
    /* Partials dN(k)/dx, dN(k)/dy, dN(k)/dz. */              
    for (int k = 0; k < 8; k++ )                                 
    {                                                         
        dNx[k] = invJacob[0]*dN1[k] + invJacob[1]*dN2[k] + invJacob[2]*dN3[k];
        dNy[k] = invJacob[3]*dN1[k] + invJacob[4]*dN2[k] + invJacob[5]*dN3[k];
        dNz[k] = invJacob[6]*dN1[k] + invJacob[7]*dN2[k] + invJacob[8]*dN3[k];
    }
}


// ****************************************************************************
//  Method: stressStrainBase::ProcessArguments
//
//  Purpose:
//      Parses optional arguments.
//
//  Arguments:
//      args      Expression arguments
//      state     Expression pipeline state
//
//  Programmer:   Cyrus Harrison
//  Creation:     June 16, 2012
//
// ****************************************************************************
void
stressStrainBase::ProcessArguments(ArgsExpr *args, ExprPipelineState *state)
{
    // get the argument list and # of arguments
    std::vector<ArgExpr*> *arguments = args->GetArgs();
    int nargs = arguments->size();

    //initialize Arguments
    SetQuadraturePoint(0);
    SetReferenceLocation(0);
    SetStrainVariety(0);
    SetStrainType(0);
    SetPrinterOn(0);

    // check for call with no args
    if (nargs == 0)
    {
        EXCEPTION2(ExpressionException, outputVariableName,
                   "Strain Tensor Expression: Incorrect syntax.\n"
                   " usage: <tensor_expr>(<mesh>,<vector_field>,"
                   " <quadrature_point=\"inner\">,"
                   " <reference_location=\"global\">)\n\n"
                   " quadrature_point options: "
                   " \"inner\",\"middle\", \"outer\" or 0,1,2\n"
                   " reference_location options: "
                   " \"global\",\"local\", or 0,1\n");
    }

    // first argument is the mesh name, let it do its own magic
    //
    // NOTE: in the future we do not need this, the mesh is implicitly
    //  known from the field passed.
    //
    ArgExpr *mesh_arg = (*arguments)[0];
    avtExprNode *mesh_tree = dynamic_cast<avtExprNode*>(mesh_arg->GetExpr());
    mesh_tree->CreateFilters(state);


    // second argument is the field name, let it do its own magic
    ArgExpr *field_arg = (*arguments)[1];
    avtExprNode *field_tree = dynamic_cast<avtExprNode*>(field_arg->GetExpr());
    field_tree->CreateFilters(state);
   
    //new argument Parser portion
    if( nargs > 2){
      fileClass printer = fileClass("processResults.txt");
      printer.open();
      for(int i = 2; i < nargs; i++){
	ArgExpr *temp_arg= (*arguments)[i];
        //ExprParseTreeNode *temp_arg_tree= temp_arg->GetExpr();
        //avtExprNode *temp_tree = dynamic_cast<avtExprNode*>(temp_arg->GetExpr());
        //temp_tree->CreateFilters(state);
        std::string sval = temp_arg->GetText();
	printer.write(sval);
	
	int found = sval.find(":");
	if (found == 0){
	  //error
	  char msg[1024];
	  sprintf(msg, " Your argument input was incorrectly formatted, make sure to include (the argument you are trying to change):(new value for the argument)");
	  EXCEPTION2(ExpressionException, outputVariableName, msg);
	}
        printer.write(found);

	//get first half
	std::string holder = sval.substr(1,found-1);
	printer.write(holder);
	//get second half
	std::string remainder = sval.substr(found+1,sval.size()-(found+2));
	printer.write(remainder);
	if(holder.compare("QUAD") == 0){
	  //input value is an integer
	  if(isdigit( remainder.at(0) )){      
	       switch(atoi(remainder.c_str())){
	         case 0: 
		   SetQuadraturePoint(0); break;
	         case 1: 
		   SetQuadraturePoint(1); break;
	         case 2: 
		   SetQuadraturePoint(2); break;
		 default: //error
		   char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: QuadraturePoint");
		   EXCEPTION2(ExpressionException, outputVariableName, msg);
		   break; 
	       }	  
	  }
	  //input value is a string
	  else{
	    //compare to find enum
	    if(     CaseInsenstiveEqual(sval,std::string("inner"))) 
	      {SetQuadraturePoint(0);}
	    else if(CaseInsenstiveEqual(sval,std::string("middle")))
	      {SetQuadraturePoint(1);}
	    else if(CaseInsenstiveEqual(sval,std::string("outer"))) 
	      {SetQuadraturePoint(2);}
	    else{ //error
	      char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: QuadraturePoint");
	      EXCEPTION2(ExpressionException, outputVariableName, msg);
	    }
	  }
	  printer.write(GetQuadraturePoint());
	}
	else if(holder.compare("LOCO") == 0){
	  //input value is an integer
	  if(isdigit( remainder.at(0) )){      
	       switch(atoi(remainder.c_str())){
	         case 0: 
		   SetReferenceLocation(0); break;
	         case 1: 
		   SetReferenceLocation(1); break;
		 default: //error
		   char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: ReferenceLocation");
		   EXCEPTION2(ExpressionException, outputVariableName, msg);
		   break; 
	       }	  
	  }
	  //input value is a string
	  else{
	    //compare to find enum
	    if(     CaseInsenstiveEqual(sval,std::string("global")))
	      {SetReferenceLocation(0);}
	    else if(CaseInsenstiveEqual(sval,std::string("local"))) 
	      {SetReferenceLocation(1);}
	    else{ //error
	      char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: ReferenceLocation");
	      EXCEPTION2(ExpressionException, outputVariableName, msg);
	    }
	  }
	  printer.write(GetReferenceLocation());
	}
	else if(holder.compare("STRN") == 0){
	  //input value is an integer
	  if(isdigit( remainder.at(0) )){      
	       switch(atoi(remainder.c_str())){
	         case 0: 
		   SetStrainVariety(0); break;
	         case 1: 
		   SetStrainVariety(1); break;
	         case 2: 
		   SetStrainVariety(2); break;
	         case 3: 
		   SetStrainVariety(3); break;
		 default: //error
		   char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: StrainVariety");
		   EXCEPTION2(ExpressionException, outputVariableName, msg);
		   break; 
	       }	  
	  }
	  //input value is a string
	  else{
	    //compare to find enum
	    if(     CaseInsenstiveEqual(sval,std::string("almansi")))       
	      {SetStrainVariety(0);}
	    else if(CaseInsenstiveEqual(sval,std::string("green_legrange")))
	      {SetStrainVariety(1);}
	    else if(CaseInsenstiveEqual(sval,std::string("infinitesimal"))) 
	      {SetStrainVariety(2);}
	    else if(CaseInsenstiveEqual(sval,std::string("rate")))          
	      {SetStrainVariety(3);}
	    else{ //error
	      char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: StrainVariety");
	      EXCEPTION2(ExpressionException, outputVariableName, msg);
	    }
	  }
	  printer.write(GetStrainVariety());
	}
	else if(holder.compare("TYPE") == 0){	  
	  //input value is an integer
	  if(isdigit( remainder.at(0) )){      
	       switch(atoi(remainder.c_str())){
	         case 0: 
		   SetStrainType(0); break;
	         case 1: 
		   SetStrainType(1); break;
	         case 2: 
		   SetStrainType(2); break;
	         case 3: 
		   SetStrainType(3); break;
	         case 4: 
		   SetStrainType(4); break;
	         case 5: 
		   SetStrainType(5); break;
	         case 6: 
		   SetStrainType(6); break;
	         case 7: 
		   SetStrainType(7); break;
	         case 8: 
		   SetStrainType(8); break;
		 default: //error
		   char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: StrainType");
		   EXCEPTION2(ExpressionException, outputVariableName, msg);
		   break; 
	       }	  
	  }
	  //input value is a string
	  else{
	    //compare to find enum
	    if(     CaseInsenstiveEqual(sval,std::string("x")))     
	      {SetStrainType(0);}
	    else if(CaseInsenstiveEqual(sval,std::string("y")))     
	      {SetStrainType(1);}
	    else if(CaseInsenstiveEqual(sval,std::string("z")))     
	      {SetStrainType(2);}
	    else if(CaseInsenstiveEqual(sval,std::string("xy")))    
	      {SetStrainType(3);}
	    else if(CaseInsenstiveEqual(sval,std::string("yz")))    
	      {SetStrainType(4);}
	    else if(CaseInsenstiveEqual(sval,std::string("zx")))    
	      {SetStrainType(5);}
	    else if(CaseInsenstiveEqual(sval,std::string("gam_xy")))
	      {SetStrainType(6);}
	    else if(CaseInsenstiveEqual(sval,std::string("gam_yZ")))
	      {SetStrainType(7);}
	    else if(CaseInsenstiveEqual(sval,std::string("gam_zx")))
	      {SetStrainType(8);}
	    else{ //error
	      char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: StrainType");
	      EXCEPTION2(ExpressionException, outputVariableName, msg);
	    }
	  }
	  printer.write(GetStrainType());
	}
	else if(holder.compare("PRNT") == 0){
	  //input value is an integer
	  if(isdigit( remainder.at(0) )){      
	       switch(atoi(remainder.c_str())){
	         case 0: 
		   SetPrinterOn(0); break;
	         case 1: 
		   SetPrinterOn(1); break;
		 default: //error
		   char msg[1024];
		   sprintf(msg, " This is not a valid input for the argument: PrinterOn");
		   EXCEPTION2(ExpressionException, outputVariableName, msg);
		   break; 
	       }	  
	  }
	  //input value is a string
	  else{
	    //compare to find enum
	    if(     CaseInsenstiveEqual(sval,std::string("off"))) 
	      {SetPrinterOn(0);}
	    else if(CaseInsenstiveEqual(sval,std::string("on")))
	      {SetPrinterOn(1);}
	    else{ //error
	      char msg[1024];
	      sprintf(msg, " This is not a valid input for the argument: PrinterOn");
	      EXCEPTION2(ExpressionException, outputVariableName, msg);
	    }
	  }
	  printer.write(GetPrinterOn());
	}
	else{
	  //error
	  char msg[1024];
	  sprintf(msg, " your input did not match any known argument");
	  EXCEPTION2(ExpressionException, outputVariableName, msg);
	}
      }
      printer.close();
    }//end of new code

    debug5 << "stressStrainBase: QuadraturePoint = " << GetQuadraturePoint() <<endl;
    debug5 << "stressStrainBase: Reference Location = " << GetReferenceLocation() <<endl;
}


