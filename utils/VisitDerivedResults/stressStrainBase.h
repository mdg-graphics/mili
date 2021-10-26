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
//                       stressStrainBase.h                         //
// ************************************************************************* //

#ifndef STRESS_STRAIN_BASE_FILTER_H
#define STRESS_STRAIN_BASE_FILTER_H

#include <avtMultipleInputExpressionFilter.h>


// ****************************************************************************
//  Class: stressStrainBase
//
//  Purpose:
//      Calculates the strain tensor.
//
//  Programmer: Thomas R. Treadway
//  Creation:   Tue Nov  7 15:59:56 PST 2006
//
//  Modifications:
//    Cyrus Harrison, Tue Jun 26 13:28:32 PDT 2012
//    Added custom arg parsing and enums for quad pt, and ref loc options.
//
//    Chadway Cooper, Fri Sept 7, 2012
//    Based on base class from Cyrus HArrison, added more arguments to the global
//    list and added dynamic argument parsing of the additional arguments in the 
//    Mili file funcation call strings.
//
// ****************************************************************************

class EXPRESSION_API stressStrainBase
    : public avtMultipleInputExpressionFilter
{
  public:
                           stressStrainBase();
    virtual                ~stressStrainBase();

    virtual const char     *GetType(void)
                               { return "stressStrainBase"; };
    virtual const char     *GetDescription(void)
                               {return "Calculating strain tensor";};
    virtual int             NumVariableArguments() { return 2; }

    // we need custom argument parsing.
    virtual void            ProcessArguments(ArgsExpr*, ExprPipelineState *);

    // Options for Quadrature Point
    enum QuadraturePoint
        {
         INNER  = 0,
         MIDDLE = 1,
         OUTER  = 2
        };

    // Options for Reference Location
    enum ReferenceLocation
        {
          GLOBAL = 0,
          LOCAL  = 1
        };

    // Options for Reference Location
    enum StrainVariety
        {
          ALMANSI = 0,
          GREEN_LEGRANGE  = 1,
          INFINITESIMAL = 2,
          RATE = 3,
	  CURVATURE = 4,
	  DISPLACEMENT = 5,
	  EFFECTIVE = 6,
	  COMPACTNESS = 7,
	  PRINCIPALDEV = 8,
	  PRINCIPAL = 9,
	  SHEAR = 10
        };

    // Options for Reference Location
    enum StrainType
        {
          X = 0,
          Y = 1,
          Z = 2,
          XY = 3,
          YZ = 4,
          ZX = 5,
          GAM_XY = 6,
          GAM_YZ = 7,
          GAM_ZX = 8
        };

    // get/set for Quadrature Point
    int                    GetQuadraturePoint() const {return quadraturePoint;}
    int                    SetQuadraturePoint(int quad_pt) {quadraturePoint = quad_pt;}

    // get/set for Reference Location
    int                    GetReferenceLocation() const {return referenceLoc;}
    int                    SetReferenceLocation(int ref_loc) {referenceLoc = ref_loc;}

    // get/set for Strain Variety
    int                    GetStrainVariety() const {return strainVariety;}
    int                    SetStrainVariety(int str_var) {strainVariety = str_var;}

    // get/set for Strain Type
    int                    GetStrainType() const {return strainType;}
    int                    SetStrainType(int str_typ) {strainType = str_typ;}

    // get/set for printer On
    int                    GetPrinterOn() const {return printerOn;}
    int                    SetPrinterOn(int on_off) {printerOn = on_off;}

  protected:
    void                   HexPartialDerivative
                               (double dNx[8], double dNy[8], double dNz[8],
                                double coorX[8], double coorY[8], 
                                double coorZ[8]);
    virtual avtVarType     GetVariableType(void) { return AVT_TENSOR_VAR; };

 private:
    int     quadraturePoint;
    int     referenceLoc;
    int     strainVariety;
    int     strainType;
    int     printerOn;
};

#endif


