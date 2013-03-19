#!/usr/bin/env python
#
############################################################################
# grizIt_Mili.py - Griz Mili Database class
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      September 27, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - Sept 17, 2010: Created.
#
############################################################################
#
import pdb

import os, sys, re, bisect, math, copy
import grizIt_VisitInfo as VI
sys.path.append(VI.visitPath)
import visit

import grizIt_GuiMain
import grizIt_GrizMiscCommands as MISC
import grizIt_GuiDialog

import grizIt_ParseArgs      as PA
from grizIt_Env              import *
import grizIt_Init           as INIT
from grizIt_Utils            import *
import grizIt_TestSuite      as TS 

from grizIt_ParseGrizCommand import *
import grizIt_Session        as S

session = S.session
env     = S.env

class Mili:
    def __init__(self):
        self.db_ident = 0
        self.root_name = ""
    #end def

    #####################
    # Open Mili Database
    #####################
    def openDB(self, database):

        if database[-5:] != ".mili":
           database += ".mili"

        try:
            os.stat(database)
        except:
            w = S.WarningMsg("Database '%s' was not found." % (database))
            return

        if visit.OpenDatabase(database) == 0:
           w = S.WarningMsg("VisIt cannot open database '%s'." % (database))
        else:
           if session.CurDatabase != "":
              visit.DeleteAllPlots()
                        
              visit.CloseDatabase(session.CurDatabase)
              p = S.InfoMsg("Closing Visit Database: "+session.CurDatabase)
              if visit.OpenDatabase(database) == 0:
                 w = S.WarningMsg("VisIt cannot open database '%s'." % (database))
           #end if
                
           visit.OpenDatabase(database)
           
           a = visit.SILRestriction()
           session.CurDatabase = database
           
           DBMetaData = self.load_metadata(database)
           
           self.load_mili_toc(database)
           self.load_mili_meshdata(database)

           numStates = session.state_count

           if session.mesh_count == 0:
               w = S.WarningMsg("WARNING: No meshes found in database '%s'." % (session.CurDatabase))
           else:
               for meshNum in range(session.mesh_count):
                   session.DBMeshNames += [DBMetaData.GetMeshes(meshNum).name]
                   #end for
                   CurMeshName = session.DBMeshNames[0]
             #end else
             
           if session.DBMatSetCount == 0:
              w = S.WarningMsg("No material sets found in database"+database)
           else:
              CurMatSetName = DBMetaData.GetMaterials(0).name
           #end else
                
           if session.DBPrimalVarCount == 0:
              w = S.WarningMsg("No primal results found in database"+database)

           if (session.mesh_count > 0) and (session.DBMatSetCount > 0) and (session.DBPrimalVarCount > 0):
               visit.AddPlot("Mesh",           CurMeshName)
               visit.AddPlot("FilledBoundary", CurMatSetName)
               
               visit.SetActivePlots((0,1,2))
               
               MaterialsSILRes = visit.SILRestriction()
               
               MatSetName  =  session.DBMatSetNames[0]
               MatMeshName =  session.DBMatMeshNames[0]
               MaterialSet =  session.DBMaterialSets[0]
               
               for currentMaterial in session.DBMaterials:
                   MaterialsSILRes.TurnOnSet(currentMaterial)
               #end for
                 
               visit.SetPlotSILRestriction(MaterialsSILRes)
                 
               if not session.OnOff_edges:
                   visit.SetActivePlots(0)
                   visit.HideActivePlots()
               #end if
                    
               visit.DrawPlots()
                    
               MatModeOn = True
           #end if

           w = S.InfoMsg("Database '%s' is now active; %d states found." % (session.CurDatabase, session.state_count))
    #end def


###########################
# Load Database Metadata
###########################

    def load_metadata(self, database):
        DBMetaData = visit.GetMetaData(session.CurDatabase)

        session.maxst         = visit.GetDatabaseNStates()
        session.state_count   = visit.GetDatabaseNStates()
        session.mesh_count    = DBMetaData.GetNumMeshes()
        session.DBMatSetCount = DBMetaData.GetNumMaterials()
        session.state_times   = DBMetaData.GetTimes()

        visit.SuppressQueryOutputOn()
        visit.SuppressQueryOutputOff()
        
        scalarCount     = DBMetaData.GetNumScalars()
        expressionCount = DBMetaData.GetExprList().GetNumExpressions()
        
        for scalarNum in range(scalarCount):
            scalarName = DBMetaData.GetScalars(scalarNum).name
        #end for

        primal_found  = False
        derived_found = False
        
        for expressionNum in range(expressionCount):
            expressionName = DBMetaData.GetExprList().GetExpressions(expressionNum).name
            print "Exp=", expressionName
            
            if expressionName[:12] == "Mili-Primal/":
               expressionName = expressionName[12:]
               session.PrimalList += [expressionName.replace("/", " ")]
               
            if expressionName[:13] == "Mili-Derived/" or expressionName[:8] == "derived/":
               expressionName = expressionName[13:]
               session.DerivedList += [expressionName.replace("/", " ")]
        #end for
        
        print "\n\n\nDerived List=", session.DerivedList

        session.DBPrimalVarCount  = self.make_primal_list(session.PrimalList)
        session.DBDerivedVarCount = self.make_derived_list(session.DerivedList)

        for matSetNum in range(session.DBMatSetCount):
            dbMaterials = DBMetaData.GetMaterials(matSetNum)
            session.DBMatSetNames  += [dbMaterials.name]
            session.DBMatMeshNames += [dbMaterials.meshName]
            session.DBMaterialSets += [dbMaterials.materialNames]
        #end for

        #
        # Get Mili Class Data
        #
        numScalars = DBMetaData.GetNumScalars()
        for i in range(numScalars):
            list = DBMetaData.GetScalars(i).name.split("/")

            if ( list[0] != "MiliClass" ):
                 continue
            name = list[1]
            id   = list[3]
            
            session.MiliClassNames += [name]
            session.MiliClassIds   += [id]
        print "\nClasss=",session.MiliClassNames 
        return(DBMetaData)
    #end def

# Get the list of Primals from the Mili TOC File
    def load_mili_toc(self, database):
        for line in fileinput.input(database):
            words = line.split()
            n = len(words)
            if n==4:
               session.DBPrimalVarCount += 1
               session.DBPrimalVarsMili[words[3]] = words[3] 
            #endif
    #endif

    def make_primal_list(self, expression_list):
        session.DBPrimalVars = buildHierDict(expression_list)
        count = len(session.DBPrimalVars)
        print "\nHier dict=", session.DBPrimalVars
        printHierDict( session.DBPrimalVars, "" )
        return( count )
    
    def make_derived_list(self, expression_list):
        session.DBDerivedVars = buildHierDict(expression_list)
        count = len(session.DBDerivedVars)
        printHierDict( session.DBDerivedVars, "" )
        return( count )
    #end def

# Get the list of Primals from the Mili TOC File
    def load_mili_meshdata(self, database):
        visit.AddPlot("FilledBoundary", "materials1", 1, 1)
        visit.AddPlot("Pseudocolor",    "eeff_in", 1, 1)
        visit.DrawPlots()

        visit.Query("NumNodes")
        session.DBNodeCount = visit.GetQueryOutputValue()
        visit.Query("NumZones")
        session.DBZoneCount = visit.GetQueryOutputValue()
    
#    visit.PythonQuery(file="grizIt_Query.vpq", vars=["default", "Labels_Node"])
#    vals = visit.GetQueryOutputValue()
#    print "\nNode Labels=", vals
        
#    visit.PythonQuery(file="grizIt_Query.vpq", vars=["default", "Result/Brick/sx"])
  #endif

    
    def populate_env_variables(self):
        """This function is to go through and populates the proper env variables, like the materials list etc. This function is meant to be used after loading the database."""
        if session.CurDatabase:
            materials_list = visit.GetMaterials() # a tuple like ('mat1', 'mat2')
            session.materials = materials_list

            for mat in materials_list:
                session.materials_visible[int(mat[3:])] = True #Starts with all mats visible
                session.enabled_mats[int(mat[3:])] = True
                session.materialNums.append(int(mat[3:]))
        else: w = S.WarningMsg("No cur database - instead get:"+session.CurDatabase)
     #end def
     

###################
# Testing Driver
###################
if __name__ == "__main__":
     database = raw_input("Please input the Mili Database name>")

     visit.AddArgument("-nowin")
     visit.AddArgument("-noconfig")

     visit.AddArgument("-v "+VI.visitVersion)
     visit.AddArgument("-nosplash")
            
     visit.Launch()
    
     visit.OpenGUI()

     miliDB = Mili()
     miliDB.openDB(database)
      

