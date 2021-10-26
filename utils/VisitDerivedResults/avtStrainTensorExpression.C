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
//                       avtStrainTensorExpression.C                         //
// ************************************************************************* //

#include <avtStrainTensorExpression.h>

#include <vtkDataArray.h>
#include <ExpressionException.h>
#include <avtExprNode.h>
#include <avtExpressionEvaluatorFilter.h>
#include <DebugStream.h>
#include <StringHelpers.h>

using namespace StringHelpers;

// ****************************************************************************
//  Method: avtStrainTensorExpression constructor
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov 14 12:59:38 PST 2006
//
// ****************************************************************************

avtStrainTensorExpression::avtStrainTensorExpression()
: quadraturePoint(INNER), referenceLoc(GLOBAL)
{
    ;
}

// ****************************************************************************
//  Method: avtStrainTensorExpression destructor
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov 14 12:59:38 PST 2006
//
// ****************************************************************************

avtStrainTensorExpression::~avtStrainTensorExpression()
{
    ;
}

// ****************************************************************************
//  Method: avtStrainTensorExpression::HexPartialDerivative
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
avtStrainTensorExpression::HexPartialDerivative
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
//  Method: avtStrainTensorExpression::ProcessArguments
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
avtStrainTensorExpression::ProcessArguments(ArgsExpr *args,
                                            ExprPipelineState *state)
{
    // get the argument list and # of arguments
    std::vector<ArgExpr*> *arguments = args->GetArgs();
    int nargs = arguments->size();

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

    // Check to see if the user passed in the 3nd or 4rd args (optional).

    bool quad_pt_ok = true;
    bool ref_loc_ok = true;


    // Options for Quad Point:
    //  INNER  (0)
    //  MIDDLE (1)
    //  OUTER  (2)
    if (nargs > 2 )
    {
        ArgExpr *quad_pt_arg= (*arguments)[2];
        ExprParseTreeNode *quad_pt_tree= quad_pt_arg->GetExpr();
        std::string quad_pt_type = quad_pt_tree->GetTypeName();

        // check for arg passed as integer
        if(quad_pt_type == "IntegerConst")
        {
            int val = dynamic_cast<IntegerConstExpr*>(quad_pt_tree)->GetValue();
            if (val == 0)
                SetQuadraturePoint(INNER);
            else if (val == 1)
                SetQuadraturePoint(MIDDLE);
            else if (val == 2)
                SetQuadraturePoint(OUTER);
            else
                quad_pt_ok = false;

        }
        // check for arg passed as string
        else if(quad_pt_type == "StringConst")
        {
            std::string sval =
                        dynamic_cast<StringConstExpr*>(quad_pt_tree)->GetValue();
            if(CaseInsenstiveEqual(sval,std::string("inner")))
                SetQuadraturePoint(INNER);
            else if(CaseInsenstiveEqual(sval,std::string("middle")))
                SetQuadraturePoint(MIDDLE);
            else if(CaseInsenstiveEqual(sval,std::string("outer")))
                SetQuadraturePoint(OUTER);
            else
                quad_pt_ok = false;
        }

        if(!quad_pt_ok)
        {
            EXCEPTION2(ExpressionException, outputVariableName,
                "avtStrainTensorExpression: Invalid <quadrature_point> argument.\n"
                " Valid options are: \"inner\",\"middle\", \"outer\" or 0,1,2 ");
        }

    }

    // Options for Ref Loc
    //  GLOBAL (0)
    //  LOCAL  (1)

    if (nargs > 3)
    {
        ArgExpr *ref_loc_arg= (*arguments)[3];
        ExprParseTreeNode *ref_loc_tree= ref_loc_arg->GetExpr();
        std::string ref_loc_type = ref_loc_tree->GetTypeName();

        // check for arg passed as integer
        if(ref_loc_type == "IntegerConst")
        {
            int val = dynamic_cast<IntegerConstExpr*>(ref_loc_tree)->GetValue();
            if (val == 0)
                SetReferenceLocation(GLOBAL);
            else if (val == 1)
                SetReferenceLocation(LOCAL);
            else
                ref_loc_ok = false;

        }
        // check for arg passed as string
        else if(ref_loc_type == "StringConst")
        {
            std::string sval =
                        dynamic_cast<StringConstExpr*>(ref_loc_tree)->GetValue();
            if(CaseInsenstiveEqual(sval,std::string("global")))
                SetReferenceLocation(GLOBAL);
            else if(CaseInsenstiveEqual(sval,std::string("local")))
                SetReferenceLocation(LOCAL);
            else
                ref_loc_ok = false;
        }

        if(! ref_loc_ok)
        {
            EXCEPTION2(ExpressionException, outputVariableName,
                "avtStrainTensorExpression: Invalid <reference_location> argument.\n"
                " Valid options are: \"global\",\"local\" or 0,1");
        }
   }
    debug5 << "avtStrainTensorExpression: QuadraturePoint = "
           << GetQuadraturePoint() <<endl;
    debug5 << "avtStrainTensorExpression: Reference Location = "
           << GetReferenceLocation() <<endl;

}


