/****************************************************************************
* UFFLP - An easy API for Mixed, Integer and Linear Programming
*
* Programmed by Artur Alves Pessoa,
*               DSc in Computer Science at PUC-Rio, Brazil
*               Assistant Professor of Production Engineering
*               at Fluminense Federal University (UFF), Brazil
*
*****************************************************************************/

#include "UFFProblem.h"

#include <math.h>
#ifdef __LINUX__
#include <memory.h>
#endif

#include <sstream>

#define UFFLP_VERSION   "2.0 over Cplex %d.%d"

//====================== CUT GENERATION ==========================

int CPXPUBLIC GenerateCuts(CPXCENVptr env, void *cbdata, int wherefrom,
      void *cbhandle, int *useraction_p)
{
   // call the cut generation function of the corresponding problem instance
   UFFProblem* prob = (UFFProblem*)cbhandle;
   prob->generateCuts( cbdata, wherefrom );

   return 0;
}

void UFFProblem::generateCuts( void* cbdata, int wherefrom )
{
   // check if a user callback is defined
   if (userCutFunc != NULL)
   {
      // set the cut generation context
      generatingCuts = true;

      // set the cut collection pointer
      callbackData = cbdata;

      // Get the node LP objective value
      CPXLPptr nodeLP;
      CPXgetcallbacknodelp( env, callbackData, CPX_CALLBACK_MIP_CUT, &nodeLP );
      CPXsolution( env, nodeLP, NULL, &nodeLpValue, NULL, NULL, NULL, NULL );

      // get the variable values and objective for the current node LP solution
      CPXgetcallbacknodex( env, cbdata, wherefrom, nodeLpSolution, 0,
            varMap.size()-1 );

      // clear the cut coefficients
      cutCoeffMap.clear();
      coeffCutMap.clear();

      // call the user's cut generation routine
      (*userCutFunc)( this );

      // reset the context
      generatingCuts = false;
   }
}

//======= CHECK WHETHER INTEGER SOLUTIONS ARE VALID ==========

int CPXPUBLIC CheckIntegerSol(CPXCENVptr env, void *cbdata, int wherefrom,
      void *cbhandle, double objval, double *x, int *isfeas_p,
      int *useraction_p)
{
   // call the cut generation function of the corresponding problem instance
   UFFProblem* prob = (UFFProblem*)cbhandle;
   prob->checkIntegerSol( objval, x, isfeas_p );

   return 0;
}

void UFFProblem::checkIntegerSol(double objval, double *x, int *isfeas_p)
{
   // check if a user callback is defined
   if (userIntChkFunc != NULL)
   {
      // set the cut generation for integer solution context
      checkingSolution = true;
      solutionIsFeasible = true;

      // Get the node LP objective value
      nodeLpValue = objval;

      // get the variable values and objective for the current node LP solution
      int n = varMap.size();
      memcpy( nodeLpSolution, x, n * sizeof(double) );

      // call the user's solution check routine
      (*userIntChkFunc)( this );

      // discard the solution if any cut has been generated
      if (solutionIsFeasible)
         *isfeas_p = 1;
      else
         *isfeas_p = 0;

      // reset the context
      checkingSolution = false;
   }
}

//====================== HEURISTIC ==========================

int CPXPUBLIC DoHeuristic( CPXCENVptr env, void *cbdata, int wherefrom,
      void *cbhandle, double *objval_p, double *x, int *checkfeas_p,
      int *useraction_p)
{
   // convert the problem pointer
   UFFProblem* prob = (UFFProblem*)cbhandle;

   // call the heuristic function of the corresponding problem instance
   if ( prob->doHeuristic( cbdata, wherefrom, objval_p, x ) )
   {
      *checkfeas_p = 1;
      *useraction_p = CPX_CALLBACK_SET;
   }

   return 0;
}

bool UFFProblem::doHeuristic(void *cbdata, int wherefrom, double* objValue,
      double* solution)
{
   bool hasBetterSol = false;
   int n = varMap.size();

   // check if a user callback is defined
   if (userHeurFunc != NULL)
   {
      // set the primal heuristic context
      inHeuristic = true;

      // reset the solution flag
      newSolutionSet = false;

      // set the value of the best integer solution
      double sense = CPXgetobjsen(env, lp);
      bestIntSolValue = sense * CPX_INFBOUND;
      CPXgetcallbackinfo(env, cbdata, wherefrom,
            CPX_CALLBACK_INFO_BEST_INTEGER, &bestIntSolValue);

      // get the variable values and objective for the current node LP solution
      nodeLpValue = *objValue;
      memcpy( nodeLpSolution, solution, n * sizeof(double) );

      // set the pointer to the buffer where the new solution is stored
      primalSolution = new double[n];
      memset( primalSolution, 0, n * sizeof(double) );

      // call the user's primal heuristic routine
      (*userHeurFunc)( this );

      // reset the context
      inHeuristic = false;

      // if a solution has been provided...
      if (newSolutionSet)
      {
         // calculate the objective value
         double value = 0.0;
         int i;
         double coeff;
         for (i = 0; i < n; i++)
         {
            CPXgetobj( env, lp, &coeff, i, i );
            value += coeff * primalSolution[i];
         }

         // check if the solution is better
         double sense = CPXgetobjsen(env, lp);
         if ( (sense * value) < (sense * bestIntSolValue) )
         {
            // copy the new solution to COIN
            memcpy( solution, primalSolution, n * sizeof(double) );
            *objValue = value;
            hasBetterSol = true;
         }
      }

      // release the solution buffer
      delete [] primalSolution;
   }

   // return whether found a better solution
   return hasBetterSol;
}

//================== CONSTRUCTORS/DESTRUCTORS ======================

CPXENVptr UFFProblem::env = NULL;

int UFFProblem::envCount = 0;

UFFProblem::UFFProblem()
{
   int status = -1;
   int* crash = NULL;

   // create the environment if necessary
   if (env == NULL)
   {
      env = CPXopenCPLEX( &status );

      // force segmentation fault when CPLEX cannot be openned
      if (status != 0) *crash = 0;
   }
   envCount++;

   // create the problem
   lp = CPXcreateprob(env, &status, "");
   if (status != 0)
   {
      // destroy the environment if necessary
      envCount--;
      if (envCount == 0)
      {
         CPXcloseCPLEX( &env );
         env = NULL;
      }

      *crash = 0;
   }
   CPXchgprobtype(env, lp, CPXPROB_MILP);

   // Set log level zero (no log message) by default
   CPXsetintparam( env, CPX_PARAM_MIPDISPLAY, 0 );
   logFileName = "";
   logLevel = 0;

   // reset the callback pointers and contexts
   userCutFunc = NULL;
   generatingCuts = false;
   userHeurFunc = NULL;
   inHeuristic = false;
   userIntChkFunc = NULL;
   checkingSolution = false;

   // other initializations
   cutoffValue = CPX_INFBOUND;
   hasIntegerVar = false;
   solutionIsFeasible = true;
}

UFFProblem::~UFFProblem()
{
   // destroy the problem
   CPXfreeprob(env, &lp);

   // destroy the environment if necessary
   envCount--;
   if (envCount == 0)
   {
      CPXcloseCPLEX( &env );
      env = NULL;
   }
}

UFFLP_ErrorType UFFProblem::addVariable( char* name, double lb, double ub,
      double obj, UFFLP_VarType type )
{
   // change the bounds depending on the type
   double lb2 = lb;
   double ub2 = ub;
   int idx = varMap.size();
   if (type == UFFLP_Binary)
   {
      if (lb2 < 0.0) lb2 = 0.0;
      if (ub2 > 1.0) ub2 = 1.0;
   }
   if (lb2 == UFFLP_Infinity)
      lb2 = CPX_INFBOUND;
   if (lb2 == -UFFLP_Infinity)
      lb2 = -CPX_INFBOUND;
   if (ub2 == UFFLP_Infinity)
      ub2 = CPX_INFBOUND;
   if (ub2 == -UFFLP_Infinity)
      ub2 = -CPX_INFBOUND;

   // check if the variable name exists
   std::map<std::string,int>::iterator it = varMap.find( name );
   if (it != varMap.end()) return UFFLP_VarNameExists;

   // check if at least one column coefficient is set
   Constraint* pcol = NULL;
   std::map<std::string,Constraint>::iterator it2 =
         varCoeffMap.find( name );
   if (it2 != varCoeffMap.end())
   {
      // get the column coefficients
      pcol = &varCoeffMap[name];
   }

   // insert the column in the solver
   char cpxType;
   switch (type)
   {
   case UFFLP_Continuous:
      cpxType = CPX_CONTINUOUS;
      break;
   case UFFLP_Integer:
      cpxType = CPX_INTEGER;
      hasIntegerVar = true;
      break;
   case UFFLP_Binary:
      cpxType = CPX_BINARY;
      hasIntegerVar = true;
      break;
   case UFFLP_SemiContinuous:
      cpxType = CPX_SEMICONT;
      hasIntegerVar = true;
      break;
   default:
      return UFFLP_UnknownVarType;
   }

   // insert the column in the solver
   if (pcol == NULL)
      CPXnewcols(env, lp, 1, &obj, &lb2, &ub2, &cpxType, &name);
   else
   {
      int beg = 0;
      CPXaddcols(env, lp, 1, pcol->indices.size(), &obj, &beg,
            &pcol->indices[0], &pcol->coeffs[0], &lb2, &ub2, &name);
      CPXchgctype(env, lp, 1, &idx, &cpxType);
   }

   // save the variable index associated to its name
#ifdef __DEBUG
   FILE* f = fopen( "debug.txt", "at" );
   fprintf( f, "Adding variable %s of type %d with index %d\n",
         name, type, idx );
   fclose( f );
#endif
   std::pair<const std::string,int> var(name, idx);
   varMap.insert( var );

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setCoefficient( char* cname, char* vname,
      double value )
{
   // check whether inside a callback other than cut
   if (inHeuristic || checkingSolution) return UFFLP_InNonCutCallback;

   int varIdx, ctrIdx;
   varIdx = ctrIdx = -1;

   // check if the constraint is not already added
   std::map<std::string,int>::iterator it = ctrMap.find( cname );
   if (it != ctrMap.end()) ctrIdx = it->second;

   // find the variable index
   std::map<std::string,int>::iterator it2 = varMap.find( vname );
   if (it2 != varMap.end()) varIdx = it2->second;

   // if generating cuts...
   if (generatingCuts)
   {
      // check whether the variable exists
      if (varIdx == -1) return UFFLP_VarNameNotFound;

      // check whether the coefficient exist
      std::stringstream s;
      std::string coefname;
      s << vname << "@" << cname;
      s >> coefname;
      std::map<std::string,int>::iterator it4 = coeffCutMap.find( coefname );
      if (it4 != coeffCutMap.end())
         return UFFLP_CoeffExists;

      // add a new cut if it does not exist
      std::map<std::string,Constraint>::iterator it3 =
            cutCoeffMap.find( cname );
      if (it3 == cutCoeffMap.end())
      {
         Constraint ctr;
         ctr.added = false;
         std::pair<const std::string,Constraint> cutCoeff(cname, ctr);
         cutCoeffMap.insert( cutCoeff );
      }

      // set the variable coefficient in the cut
      Constraint* pctr = &cutCoeffMap[cname];
      std::pair<const std::string,int> coeff(coefname, pctr->indices.size());
      coeffCutMap.insert( coeff );
      pctr->indices.push_back( varIdx );
      pctr->coeffs.push_back( value );
   }

   // if not generating cuts...
   else
   {
      std::map<std::string,Constraint>::iterator it3;

      // check whether the coefficient exist
      std::stringstream s;
      std::string coefname;
      s << vname << "@" << cname;
      s >> coefname;
      std::map<std::string,int>::iterator it4 = coeffCtrMap.find( coefname );
      if (it4 != coeffCtrMap.end())
         return UFFLP_CoeffExists;

      if ((varIdx >= 0) && (ctrIdx == -1))
      {
         // add a new constraint if it does not exist
         it3 = ctrCoeffMap.find( cname );
         if (it3 == ctrCoeffMap.end())
         {
            Constraint ctr;
            ctr.added = false;
            std::pair<const std::string,Constraint> ctrCoeff(cname, ctr);
            ctrCoeffMap.insert( ctrCoeff );
         }

         // set the variable coefficient in the constraint
         Constraint* pctr = &ctrCoeffMap[cname];
         std::pair<const std::string,int> coeff(coefname, pctr->indices.size());
         coeffCtrMap.insert( coeff );
         pctr->indices.push_back( varIdx );
         pctr->coeffs.push_back( value );
      }

      else if ((varIdx == -1) && (ctrIdx >= 0))
      {
         // add a new variable if it does not exist
         it3 = varCoeffMap.find( vname );
         if (it3 == varCoeffMap.end())
         {
            Constraint col;
            std::pair<const std::string,Constraint> varCoeff(vname, col);
            varCoeffMap.insert( varCoeff );
         }

         // set the constraint coefficient in the variable
         Constraint* pcol = &varCoeffMap[vname];
         std::pair<const std::string,int> coeff(coefname, ctrIdx);
         coeffCtrMap.insert( coeff );
         pcol->indices.push_back( ctrIdx );
         pcol->coeffs.push_back( value );
      }

      else return (ctrIdx >= 0)? UFFLP_ConsNameExists: UFFLP_VarNameNotFound;
   }

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::addConstraint( char* name, double rhs,
      UFFLP_ConsType type )
{
   // check whether inside a callback other than cut
   if (inHeuristic || checkingSolution) return UFFLP_InNonCutCallback;

   Constraint* pctr;

   // if generating cuts...
   if (generatingCuts)
   {
      // check if at least one cut coefficient is set
      std::map<std::string,Constraint>::iterator it =
            cutCoeffMap.find( name );
      if (it == cutCoeffMap.end()) return UFFLP_ConsNameNotFound;

      // get the cut coefficients
      pctr = &cutCoeffMap[name];
   }

   // if not generating cuts...
   else
   {
      // check if at least one constraint coefficient is set
      std::map<std::string,Constraint>::iterator it =
            ctrCoeffMap.find( name );
      if (it == ctrCoeffMap.end()) return UFFLP_ConsNameNotFound;

      // get the constraint coefficients
      pctr = &ctrCoeffMap[name];
   }

   // check if the constraint has already been added
   if (pctr->added) return UFFLP_ConsNameExists;
   pctr->added = true;

   // set the constraint's lower and upper bounds
   char cpxType;
   switch (type)
   {
   case UFFLP_Less:
      cpxType = 'L';
      break;
   case UFFLP_Equal:
      cpxType = 'E';
      break;
   case UFFLP_Greater:
      cpxType = 'G';
      break;
   }

   // save the constraint info (necessary only in a branch callback)
   pctr->rhs = rhs;
   pctr->sense = cpxType;

   // if generating cuts...
   if (generatingCuts)
   {
      // add the cut to the node
#if CPX_VERSION >= 1120
      CPXcutcallbackadd( env, callbackData, CPX_CALLBACK_MIP_CUT,
            (int)pctr->indices.size(), rhs, cpxType,
            &pctr->indices[0], &pctr->coeffs[0], 0 );
#else
      CPXcutcallbackadd( env, callbackData, CPX_CALLBACK_MIP_CUT,
            (int)pctr->indices.size(), rhs, cpxType,
            &pctr->indices[0], &pctr->coeffs[0] );
#endif

#ifdef __DEBUG
      FILE* f = fopen( "debug.txt", "at" );
      fprintf( f, "Inserting row of type %d with RHS %g and with %d coefficients"
            "\nIndices", type, rhs, pctr->indices.size() );
      int i;
      for (i = 0; i < (int)pctr->indices.size(); i++)
         fprintf( f, " %d", pctr->indices[i] );
      fprintf( f, "\nCoeffs" );
      for (i = 0; i < (int)pctr->coeffs.size(); i++)
         fprintf( f, " %g", pctr->coeffs[i] );
      fprintf( f, "\n" );
      fclose( f );
#endif
   }

   // if not generating cuts...
   else
   {
      // add the constraint to the model
      int idx = ctrMap.size();
      int i;
#ifdef __DEBUG
      FILE* f = fopen( "debug.txt", "at" );
      fprintf( f, "Inserting row of type %d with RHS %g and with %d coefficients"
            "\nIndices", type, rhs, pctr->indices.size() );
      for (i = 0; i < (int)pctr->indices.size(); i++)
         fprintf( f, " %d", pctr->indices[i] );
      fprintf( f, "\nCoeffs" );
      for (i = 0; i < (int)pctr->coeffs.size(); i++)
         fprintf( f, " %g", pctr->coeffs[i] );
      fprintf( f, "\n" );
      fclose( f );
#endif
      CPXnewrows(env, lp, 1, &rhs, &cpxType, NULL, &name);
      for (i = 0; i < (int)pctr->indices.size(); i++)
      {
         CPXchgcoef(env, lp, idx, pctr->indices[i], pctr->coeffs[i] );
      }

      // save the constraint index associated to its name
      std::pair<const std::string,int> ctr(name, idx);
      ctrMap.insert( ctr );
   }

   return UFFLP_Ok;
}

UFFLP_StatusType UFFProblem::solve(UFFLP_ObjSense sense)
{
   // set the objective sense
   if (sense == UFFLP_Maximize)
      CPXchgobjsen( env, lp, CPX_MAX);
   else
      CPXchgobjsen( env, lp, CPX_MIN);

   // set the cutoff value
   if ((cutoffValue > -CPX_INFBOUND) && (cutoffValue < CPX_INFBOUND))
   {
      if (sense == UFFLP_Maximize)
         CPXsetdblparam( env, CPX_PARAM_CUTLO, cutoffValue );
      else
         CPXsetdblparam( env, CPX_PARAM_CUTUP, cutoffValue );
   }

   // write the UFFLP header to the log file
   if ( logFileName == "" )
   {
      // Write the UFFLP header to the screen
      printf( "UFFLP version " UFFLP_VERSION " by Artur Alves Pessoa\n",
               CPX_VERSION/100, (CPX_VERSION/10)%10 );
   }
   else
   {
      // Write the UFFLP header to a file
      FILE* f = fopen( logFileName.c_str(), "wt" );
      if (f != NULL)
      {
         fprintf( f, "UFFLP version " UFFLP_VERSION " by Artur Alves Pessoa\n",
                  CPX_VERSION/100, (CPX_VERSION/10)%10 );
         fclose(f);
      }
   }

   // Check if a log file name has been set
   int prevScrInd;
   logFile = NULL;
   if ( logFileName != "" )
   {
      // Reopen the log file for appending
      logFile = CPXfopen( logFileName.c_str(), "a" );
   }
   else
   {
      // Enable log on screen
      CPXgetintparam( env, CPX_PARAM_SCRIND, &prevScrInd );
      CPXsetintparam( env, CPX_PARAM_SCRIND, CPX_ON );
   }

   // Configure the log file if necessary
   CPXsetlogfile( env, logFile );

   // Set log level
   CPXsetintparam( env, CPX_PARAM_MIPDISPLAY, logLevel );

   // declare variables callback pointers
   int(CPXPUBLIC *saveCutCallback)(CALLBACK_CUT_ARGS);
   void* saveCutPointer;
   int(CPXPUBLIC *saveIncCallback)(CALLBACK_INCUMBENT_ARGS);
   void* saveIncPointer;
   int(CPXPUBLIC *saveHeurCallback)(CALLBACK_HEURISTIC_ARGS);
   void* saveHeurPointer;

   // if the problem is a MIP, prepare to solve
   if (hasIntegerVar)
   {
      // allocate the buffer for the node LP solutions
      nodeLpSolution = new double[varMap.size()];

      // save the previous callback functions
      CPXgetcutcallbackfunc( env, &saveCutCallback, &saveCutPointer );
      CPXgetincumbentcallbackfunc( env, &saveIncCallback, &saveIncPointer );
      CPXgetheuristiccallbackfunc( env, &saveHeurCallback, &saveHeurPointer );

      // set some default parameters
      CPXsetintparam( env, CPX_PARAM_MIPINTERVAL, 1 );
      if ( (userCutFunc != NULL) || (userHeurFunc != NULL) )
      {
         // set the heuristic and cut callbacks
         CPXsetcutcallbackfunc( env, GenerateCuts, this );
         CPXsetheuristiccallbackfunc( env, DoHeuristic, this );

         // set the incumbent and branch callbacks
         if ( (userCutFunc != NULL) && (userIntChkFunc != NULL) )
         {
            CPXsetincumbentcallbackfunc( env, CheckIntegerSol, this );
            CPXsetintparam( env, CPX_PARAM_REDUCE, CPX_PREREDUCE_PRIMALONLY );
         }
         else
         {
            CPXsetincumbentcallbackfunc( env, NULL, NULL );
            CPXsetintparam( env, CPX_PARAM_REDUCE, CPX_PREREDUCE_PRIMALANDDUAL );
         }

         // make sure to work with the original model inside the callbacks
         CPXsetintparam( env, CPX_PARAM_MIPCBREDLP, CPX_OFF );
         CPXsetintparam( env, CPX_PARAM_PRELINEAR, CPX_OFF );
      }
      else
      {
         // set no callback
         CPXsetcutcallbackfunc( env, NULL, NULL );
         CPXsetincumbentcallbackfunc( env, NULL, NULL );
         CPXsetheuristiccallbackfunc( env, NULL, NULL );

         // use the CPLEX defaults if no callback is set
         CPXsetintparam( env, CPX_PARAM_MIPCBREDLP, CPX_ON );
         CPXsetintparam( env, CPX_PARAM_PRELINEAR, CPX_ON );
         CPXsetintparam( env, CPX_PARAM_REDUCE, CPX_PREREDUCE_PRIMALANDDUAL );
      }
   }

   // set the user-defined parameters
   std::map<int,CplexParamValue>::iterator it;
   for (it = paramValues.begin(); it != paramValues.end(); it++)
   {
      switch (it->second.type)
      {
      case UFFLP_IntegerParam:
         CPXsetintparam(env, it->first, int(it->second.value));
         break;
      case UFFLP_FloatParam:
         CPXsetdblparam(env, it->first, it->second.value);
         break;
      }
   }

   // solve as an LP problem if no integer variable is defined
   if (!hasIntegerVar)
   {
      CPXchgprobtype(env, lp, CPXPROB_LP);
      CPXlpopt (env, lp);
   }
   else
   {
      // Solve the MIP
      CPXmipopt( env, lp );
   }

   // get the optimization status and close the log file
   int status = CPXgetstat( env, lp );
   if (logFile != NULL)
   {
      CPXsetlogfile( env, NULL );
      CPXfclose( logFile );
   }
   else
      CPXsetintparam( env, CPX_PARAM_SCRIND, prevScrInd );

#ifdef __DEBUG
   FILE* f = fopen( "debug.txt", "at" );
   fprintf( f, "The solution status is %d\n", status );
   fclose( f );
#endif

   if (!hasIntegerVar)
   {
      // Check if the optimal solution has been found
      if ((status == CPX_STAT_OPTIMAL) || (status == CPX_STAT_OPTIMAL_INFEAS))
      {
         return UFFLP_Optimal;
      }

      // Check is the problem is infeasible
      if ( (status == CPX_STAT_INFEASIBLE) || (status == CPX_STAT_INForUNBD)
            || (status == CPX_STAT_UNBOUNDED) )
      {
         return UFFLP_Infeasible;
      }
   }
   else
   {
      // restore the callbacks
      CPXsetcutcallbackfunc( env, saveCutCallback, saveCutPointer );
      CPXsetincumbentcallbackfunc( env, saveIncCallback, saveIncPointer );
      CPXsetheuristiccallbackfunc( env, saveHeurCallback, saveHeurPointer );

      // release the buffer for the node LP solutions
      delete [] nodeLpSolution;

      // Check if the optimal solution has been found
      if ( (status == CPXMIP_OPTIMAL) || (status == CPXMIP_OPTIMAL_TOL)
            || (status == CPX_STAT_OPTIMAL_INFEAS) )
      {
         return UFFLP_Optimal;
      }

      // check if a feasible solution has been found
      if ( (status == CPXMIP_NODE_LIM_FEAS) || (status == CPXMIP_TIME_LIM_FEAS) )
      {
         return UFFLP_Feasible;
      }

      // Check is the problem is infeasible
      if ( (status == CPXMIP_INFEASIBLE) || (status == CPXMIP_INForUNBD)
            || (status == CPXMIP_UNBOUNDED) )
      {
         return UFFLP_Infeasible;
      }
   }

   // Assume unfinished
   return UFFLP_Aborted;
}

UFFLP_ErrorType UFFProblem::getObjValue(double* value)
{
   // if inside a callback...
   if (generatingCuts || inHeuristic || checkingSolution)
   {
      *value = nodeLpValue;
   }

   // if not inside a callback...
   else
   {
      CPXgetmipobjval(env, lp, value);
   }
   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::getSolution(char* vname, double* value)
{
   // find the variable index
   std::map<std::string,int>::iterator it2 = varMap.find( vname );
   if (it2 == varMap.end()) return UFFLP_VarNameNotFound;
   int varIdx = it2->second;

   // if inside a callback...
   if (generatingCuts || inHeuristic || checkingSolution)
   {
      *value = nodeLpSolution[varIdx];
   }

   // if not inside a callback...
   else
   {
      // get the variable value and store it at the user's area
      CPXgetmipx (env, lp, value, varIdx, varIdx);
   }

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::getDualSolution(char* cname, double* value)
{
   // find the constraint index
   std::map<std::string,int>::iterator it = ctrMap.find( cname );
   if (it == ctrMap.end()) return UFFLP_ConsNameNotFound;
   int ctrIdx = it->second;

   // if inside a callback...
   if (generatingCuts || inHeuristic || checkingSolution)
   {
      return UFFLP_InCallback;
   }

   // if the problem has integer variables
   if (hasIntegerVar)
   {
      return UFFLP_NoDualVariables;
   }

   // get the dual value and store it at the user's area
   CPXgetpi (env, lp, value, ctrIdx, ctrIdx);

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::writeLP(char* fname)
{
   // Write the problem data
   CPXwriteprob(env, lp, fname, "LP");

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setLogInfo(char* fname, int level)
{
   // Configure the log parameters
   logFileName = fname;
   logLevel = level;

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setCutCallBack(UFFLP_CallBackFunction cutFunc)
{
   // set the callback function
   userCutFunc = cutFunc;

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::printToLog(char* message)
{
   // check if inside a callback
   if ( !generatingCuts && !inHeuristic && !checkingSolution )
      return UFFLP_NotInCallback;

   // print the message to the log file
   if (logFile == NULL)
      printf( "UFFLP: %s\n", message );
   else
   {
      // Close the current cplex log file
      CPXsetlogfile( env, NULL );
      CPXfclose( logFile );

      // Append the user message to the file
      FILE* f = fopen( logFileName.c_str(), "at" );
      if (f != NULL)
      {
         fprintf( f, "UFFLP: %s\n", message );
         fclose(f);
      }

      // Reopen the log file and reassign to CPLEX
      logFile = CPXfopen( logFileName.c_str(), "a" );
      CPXsetlogfile( env, logFile );
   }

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setHeurCallBack(UFFLP_CallBackFunction heurFunc)
{
   // set the callback function
   userHeurFunc = heurFunc;

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::getBestSolutionValue(double* value)
{
   // check if we are in the heuristic context
   if ( !inHeuristic ) return UFFLP_NotInHeuristic;

   // set the value of the best integer solution
   *value = bestIntSolValue;

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setSolution(char* vname, double value)
{
   // check if we are in the heuristic context
   if ( !inHeuristic ) return UFFLP_NotInHeuristic;

   // find the variable index
   std::map<std::string,int>::iterator it = varMap.find( vname );
   if (it == varMap.end()) return UFFLP_VarNameNotFound;
   int varIdx = it->second;

   // set the variable value
   primalSolution[varIdx] = value;

   // set the solution flag
   newSolutionSet = true;

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setParameter(UFFLP_ParameterType param,
      double value)
{
   // check if the parameter exists
   switch (param)
   {
   case UFFLP_CutoffValue:
      cutoffValue = value;
      break;

   case UFFLP_NodesLimit:
      CPXsetintparam( env, CPX_PARAM_NODELIM, int(value) );
      break;

   case UFFLP_TimeLimit:
      CPXsetdblparam( env, CPX_PARAM_TILIM, value );
      break;

   default:
      return UFFLP_InvalidParameter;
   }

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::changeObjCoeff(char* vname, double value)
{
   // check if we are in a callback context
   if ( inHeuristic || generatingCuts || checkingSolution )
      return UFFLP_InCallback;

   // find the variable index
   std::map<std::string,int>::iterator it = varMap.find( vname );
   if (it == varMap.end()) return UFFLP_VarNameNotFound;
   int varIdx = it->second;

   // set the coefficient in the objective function
   CPXchgobj( env, lp, 1, &varIdx, &value );

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setPriority(char* vname, int prior)
{
   // check if we are in a callback context
   if ( inHeuristic || generatingCuts || checkingSolution )
      return UFFLP_InCallback;

   // find the variable index
   std::map<std::string,int>::iterator it = varMap.find( vname );
   if (it == varMap.end()) return UFFLP_VarNameNotFound;
   int varIdx = it->second;

   // set the variable priority for branching
   CPXcopyorder( env, lp, 1, &varIdx, &prior, NULL );

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::checkSolution(double toler)
{
   // check if we are in a heuristic callback context
   if ( !inHeuristic ) return UFFLP_NotInHeuristic;

   // check all constraints
   std::map<std::string,Constraint>::iterator it;
   for (it = ctrCoeffMap.begin(); it != ctrCoeffMap.end(); it++)
   {
      // calculate the left-hand side for the current constraint
      double lhs = 0.0;
      Constraint* pctr = &(it->second);
      if (!pctr->added) continue;   // skip the constraints not added
      for (int i = 0; i < (int)pctr->indices.size(); i++)
         lhs += pctr->coeffs[i] * primalSolution[pctr->indices[i]];

      // get the constraint right-hand side and the sense
      double rhs;
      char sense;
      int ctrIdx = ctrMap[it->first];
      CPXgetrhs(env, lp, &rhs, ctrIdx, ctrIdx);
      CPXgetsense(env, lp, &sense, ctrIdx, ctrIdx);

      // check the constraint
      switch( sense )
      {
      case 'L':
         if (lhs > rhs + toler)
         {
            std::stringstream s;
            s << "Heuristic solution violates constraint " << it->first
                  << " with " << lhs << " greater than " << rhs;
            printToLog((char*)s.str().c_str());
            return UFFLP_InfeasibleSol;
         }
         break;
      case 'E':
         if (fabs(lhs - rhs) > toler)
         {
            std::stringstream s;
            s << "Heuristic solution violates constraint " << it->first
                  << " with " << lhs << " not equal to " << rhs;
            printToLog((char*)s.str().c_str());
            return UFFLP_InfeasibleSol;
         }
         break;
      case 'G':
         if (lhs < rhs - toler)
         {
            std::stringstream s;
            s << "Heuristic solution violates constraint " << it->first
                  << " with " << lhs << " less than " << rhs;
            printToLog((char*)s.str().c_str());
            return UFFLP_InfeasibleSol;
         }
         break;
      }
   }

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setCplexParameter(int param, UFFLP_ParamTypeType type,
      double value)
{
   // set the parameter only to check if it is valid
   int error;
   switch (type)
   {
   case UFFLP_IntegerParam:
      error = CPXsetintparam(env, param, int(value));
      break;
   case UFFLP_FloatParam:
      error = CPXsetdblparam(env, param, value);
      break;
   default:
      return UFFLP_InvalParamType;
   };
   if (error != 0) return UFFLP_InvalidParameter;

   // store the parameter value for setting when solving the problem
   CplexParamValue pv;
   pv.type = type;
   pv.value = value;
   paramValues[param] = pv;

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setIntCheckCallBack(UFFLP_CallBackFunction intChkFunc)
{
   // set the callback function
   userIntChkFunc = intChkFunc;

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setInfeasible()
{
   // check whether in an integer solution check callback
   if (!checkingSolution) return UFFLP_NotInIntCheck;

   // set that the solution is infeasible and return
   solutionIsFeasible = false;
   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::changeBounds(char* vname, double lb, double ub)
{
   // check if we are in a callback context
   if ( inHeuristic || generatingCuts || checkingSolution )
      return UFFLP_InCallback;

   // find the variable index
   std::map<std::string,int>::iterator it = varMap.find( vname );
   if (it == varMap.end()) return UFFLP_VarNameNotFound;
   int varIdx[2]; varIdx[0] = it->second; varIdx[1] = it->second;

   // set the coefficient in the objective function
   char lu[2];   lu[0] = 'L'; lu[1] = 'U';
   double bd[2]; bd[0] = lb;  bd[1] = ub;
   CPXchgbds( env, lp, 2, &varIdx[0], &lu[0], &bd[0] );

   return UFFLP_Ok;
}
