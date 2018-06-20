/****************************************************************************
* UFFLP - An easy API for Mixed, Integer and Linear Programming
*
* Programmed by Artur Alves Pessoa,
*               DSc in Computer Science at PUC-Rio, Brazil
*               Assistant Professor of Production Engineering
*               at Fluminense Federal University (UFF), Brazil
*
*****************************************************************************/

#include <OsiCbcSolverInterface.hpp>
#include <OsiClpSolverInterface.hpp>
#include <CglPreProcess.hpp>
#include <OsiAuxInfo.hpp>
#include <CbcStrategy.hpp>
#include <CbcFeasibilityBase.hpp>
#include <CbcCutGenerator.hpp>

#include "UFFProblem.h"

#include <sstream>

#define UFFLP_VERSION   "2.0 over Coin-Cbc 2.4"

//====================== CUT GENERATION ==========================

void UFFCutGenerator::generateCuts( const OsiSolverInterface & si,
      OsiCuts & cs, const CglTreeInfo info ) const
{
   // check if a user callback is defined
   if (userCutFunc != NULL)
   {
      // set the cut generation context
      problem->generatingCuts = true;

      // set the cut collection pointer
      problem->cutCollection = &cs;

      // clear the cut coefficients
      problem->cutCoeffMap.clear();
      problem->coeffCutMap.clear();

      // call the user's cut generation routine
      (*userCutFunc)( problem );

      // reset the context
      problem->generatingCuts = false;
   }

   // flush the log file if any
   if (logFile != NULL) fflush( logFile );
}

CglCutGenerator* UFFCutGenerator::clone() const
{
   return new UFFCutGenerator(*this);
}

//====================== HEURISTIC ==========================

UFFPrimalHeuristic::UFFPrimalHeuristic( CbcModel & model )
   : CbcHeuristic( model )
{
   userHeurFunc = NULL;
}

CbcHeuristic* UFFPrimalHeuristic::clone() const
{
   return new UFFPrimalHeuristic(*this);
}

void UFFPrimalHeuristic::resetModel(CbcModel * model)
{
   model_ = model;
}

int UFFPrimalHeuristic::solution(double & solutionValue,
			 double * betterSolution)
{
   bool hasBetterSol = false;
   int n = problem->solver->getNumCols();

   // if must check integer feasibility, disable all the other heuristics
   if (problem->feasibilityCheck)
   {
      for (int i = 1; i < problem->model->numberHeuristics(); i++)
         problem->model->heuristic(i)->setWhen(0);
   }

   // check if a user callback is defined and the current node is solved
   if ((userHeurFunc != NULL) && (problem->model->phase() != 1))
   {
      // set the primal heuristic context
      problem->inHeuristic = true;

      // reset the solution flag
      problem->newSolutionSet = false;

      // set the value of the best integer solution
      problem->bestIntSolValue = solutionValue;

      // set the pointer to the buffer where the new solution is stored
      problem->primalSolution = new double[n];
      memset( problem->primalSolution, 0, n * sizeof(double) );

      // call the user's primal heuristic routine
      (*userHeurFunc)( problem );

      // reset the context
      problem->inHeuristic = false;

      // if a solution has been provided...
      if (problem->newSolutionSet)
      {
         // calculate the objective value
         double value = 0.0;
         int i;
         double* coeffs = (double*)problem->solver->getObjCoefficients();
         for (i = 0; i < n; i++)
            value += coeffs[i] * problem->primalSolution[i];

         // check if the solution is better
         double sense = problem->solver->getObjSense();
         if ( (sense * value) < solutionValue )
         {
            // copy the new solution to COIN
            memcpy( betterSolution, problem->primalSolution,
                  n * sizeof(double) );
            solutionValue = (sense * value);
            hasBetterSol = true;
         }
      }

      // release the solution buffer
      delete [] problem->primalSolution;
   }

   // flush the log file if any
   if (logFile != NULL) fflush( logFile );

   // return 0 if no better solution found, and 1 if found a better solution
   return (hasBetterSol ? 1 : 0);
}

//================== FEASIBILITY CHECK ======================

// Class to disallow strong branching solutions
class CbcFeasibilityNoStrong : public CbcFeasibilityBase
{
public:
   // Default Constructor 
   CbcFeasibilityNoStrong () {};

   virtual ~CbcFeasibilityNoStrong() {};
   // Copy constructor 
   CbcFeasibilityNoStrong ( const CbcFeasibilityNoStrong &rhs) {};

   // Assignment operator 
   CbcFeasibilityNoStrong & operator=( const CbcFeasibilityNoStrong& rhs)
   { return * this; };

   /// Clone
   virtual CbcFeasibilityBase * clone() const
   { return new CbcFeasibilityNoStrong(); };

   /**
     On input mode:
     0 - called after a solve but before any cuts
     -1 - called after strong branching
     Returns :
     0 - no opinion
     -1 pretend infeasible
     1 pretend integer solution
   */
   virtual int feasible(CbcModel * model, int mode) 
   { return mode; };
};

//================== CONSTRUCTORS/DESTRUCTORS ======================

UFFProblem::UFFProblem()
      : solverLog(), modelLog()
{
   // Create the solver interface
   OsiCbcSolverInterface* cbcSolver = new OsiCbcSolverInterface;
   solver = cbcSolver;

   // Get the branch-and-cut model
   model = cbcSolver->getModelPtr();

   // Set default log message handlers
   model->solver()->passInMessageHandler( &solverLog );
   model->passInMessageHandler( &modelLog );

   // Set log level zero (no log message) by default
   // for (0 - B&B, 1 - solver, 3 - cut gen)
   model->messageHandler()->setLogLevel( 0, 0 );
   model->messageHandler()->setLogLevel( 1, 0 );
   model->messageHandler()->setLogLevel( 3, 0 );
   logFileName = "";
   logLevel = 0;

   // reset the callback pointers and contexts
   userCutGen.userCutFunc = NULL;
   generatingCuts = false;
   userHeur = new UFFPrimalHeuristic( *model );
   inHeuristic = false;

   // initialize other stuff
   cutoffValue = solver->getInfinity();
   hasIntegerVar = false;
   hasBeenSolved = false;
   probCache.sync = false;
   feasibilityCheck = false;
}

UFFProblem::~UFFProblem()
{
   delete solver;
   delete userHeur;
}

//====================== API FUNCTIONS ==========================

UFFLP_ErrorType UFFProblem::addVariable( char* name, double lb, double ub,
      double obj, UFFLP_VarType type )
{
   // check the variable type
   if ( (type != UFFLP_Continuous) && (type != UFFLP_Integer)
         && (type != UFFLP_Binary) )
      return UFFLP_UnknownVarType;

   // change the bounds depending on the type
   double lb2 = lb;
   double ub2 = ub;
   if (type == UFFLP_Binary)
   {
      if (lb2 < 0.0) lb2 = 0.0;
      if (ub2 > 1.0) ub2 = 1.0;
   }
   if (lb2 == UFFLP_Infinity)
      lb2 = solver->getInfinity();
   if (lb2 == -UFFLP_Infinity)
      lb2 = -solver->getInfinity();
   if (ub2 == UFFLP_Infinity)
      ub2 = solver->getInfinity();
   if (ub2 == -UFFLP_Infinity)
      ub2 = -solver->getInfinity();

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

   // if not using the problem cache
   int idx;
   if (probCache.sync)
   {
      // insert the column in the solver
      idx = solver->getNumCols();
      if (pcol == NULL)
         solver->addCol(0, NULL, NULL, lb2, ub2, obj);
      else
      {
         solver->addCol(pcol->indices.size(), &pcol->indices[0],
               &pcol->coeffs[0], lb2, ub2, obj);
      }
      if (type == UFFLP_Continuous)
         solver->setContinuous( idx );
      else
      {
         solver->setInteger( idx );
         hasIntegerVar = true;
      }
   }
   else
   {
      // insert the column in the problem cache
      idx = probCache.collb.size();
      if (pcol == NULL)
      {
         probCache.indexMatrix.push_back( std::vector<int>() );
         probCache.valueMatrix.push_back( std::vector<double>() );
      }
      else
      {
         probCache.indexMatrix.push_back( std::vector<int>(
               &pcol->indices[0],
               &pcol->indices[pcol->indices.size()] ) );
         probCache.valueMatrix.push_back( std::vector<double>(
               &pcol->coeffs[0],
               &pcol->coeffs[pcol->indices.size()] ) );
      }
      probCache.collb.push_back( lb2 );
      probCache.colub.push_back( ub2 );
      probCache.obj.push_back( obj );
      if (type == UFFLP_Continuous)
         probCache.isintcol.push_back( false );
      else
      {
         probCache.isintcol.push_back( true );
         hasIntegerVar = true;
      }
   }

   // save the variable index associated to its name
#ifdef _DEBUG
   FILE* f = fopen( "debug.txt", "at" );
   fprintf( f, "Adding variable %s with index %d\n", name, idx );
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
   if (inHeuristic) return UFFLP_InNonCutCallback;

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
      // check whether the constraint exist of the variable not
      if (varIdx == -1) return UFFLP_VarNameNotFound;
      if (ctrIdx >= 0) return UFFLP_ConsNameExists;

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
   if (inHeuristic) return UFFLP_InNonCutCallback;

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
   double lb = -solver->getInfinity();
   double ub = solver->getInfinity();
   if ( (type == UFFLP_Equal) || (type == UFFLP_Greater) )
      lb = rhs;
   if ( (type == UFFLP_Equal) || (type == UFFLP_Less) )
      ub = rhs;

   // if generating cuts...
   if (generatingCuts)
   {
      OsiRowCut cut;
      cut.setRow( pctr->indices.size(), &pctr->indices[0], &pctr->coeffs[0] );
      cut.setLb(lb);
      cut.setUb(ub);
      cutCollection->insert(cut);
   }

   // if not generating cuts...
   else
   {
#ifdef _DEBUG
      FILE* f = fopen( "debug.txt", "at" );
      fprintf( f, "Inserting row with %d coefficients\nIndices", pctr->indices.size() );
      int i;
      for (i = 0; i < (int)pctr->indices.size(); i++)
         fprintf( f, " %d", pctr->indices[i] );
      fprintf( f, "\nCoeffs" );
      for (i = 0; i < (int)pctr->coeffs.size(); i++)
         fprintf( f, " %g", pctr->coeffs[i] );
      fprintf( f, "\n" );
      fclose( f );
#endif

      // if not using the problem cache
      int idx = 0;
      if (probCache.sync)
      {
         // add the constraint to the model
         idx = solver->getNumRows();
         solver->addRow( pctr->indices.size(), &pctr->indices[0], &pctr->coeffs[0],
               lb, ub );
      }
      else
      {
         // insert the column in the problem cache
         idx = probCache.rowlb.size();
         for (int i = 0; i < (int)pctr->indices.size(); i++)
         {
            probCache.indexMatrix[pctr->indices[i]].push_back( idx );
            probCache.valueMatrix[pctr->indices[i]].push_back(
                     pctr->coeffs[i] );
         }
         probCache.rowlb.push_back( lb );
         probCache.rowub.push_back( ub );
      }

      // save the constraint index associated to its name
      std::pair<const std::string,int> ctr(name, idx);
      ctrMap.insert( ctr );
   }

   return UFFLP_Ok;
}

UFFLP_StatusType UFFProblem::solve(UFFLP_ObjSense sense)
{
   // Synchronize the cache of variables and constraints with the COIN-OR
   synchronizeProblem();

   // Set the objective function sense
   if (sense == UFFLP_Maximize)
      solver->setObjSense( -1 );

   // Check if a log file name has been set
   FILE* f = NULL;
   if ( logFileName != "" )
   {
      // Open the log file
      f = fopen( logFileName.c_str(), "wt" );
      fprintf( f, "UFFLP version " UFFLP_VERSION " by Artur Alves Pessoa\n" );
      fflush( f );
   }

   // Configure the log file if necessary
   CoinMessageHandler newSolverLog( f );
   CoinMessageHandler newModelLog( f );
   if (f != NULL)
   {
      model->solver()->passInMessageHandler( &newSolverLog );
      model->passInMessageHandler( &newModelLog );
   }

   // Set the cutoff value
   if (cutoffValue != solver->getInfinity())
      model->setCutoff( cutoffValue * solver->getObjSense() );

   // Set log level for (0 - B&B, 1 - solver, 3 - cut gen)
   model->messageHandler()->setLogLevel( 0, logLevel );
   model->messageHandler()->setLogLevel( 1, logLevel );
   model->messageHandler()->setLogLevel( 3, logLevel );

   bool solvedByInitial = false;
   if (hasIntegerVar)
   {
      if (feasibilityCheck)
      {
         // Don't allow dual stuff
         OsiClpSolverInterface* osiclp =
               dynamic_cast< OsiClpSolverInterface*> (model->solver());
         if (osiclp) osiclp->setSpecialOptions(osiclp->specialOptions()|262144);
      } 

      // Solve the relaxation
      model->initialSolve();

      // check is the initial solution is already feasible and optimal
      bool isInteger = true;
      if (model->solver()->isProvenOptimal())
      {
         for (int i = 0; i < model->solver()->getNumCols(); i++)
         {
            if (model->isInteger(i))
            {
               double x = model->solver()->getColSolution()[i];
               if (fabs(x - floor(x + 0.5)) > model->getIntegerTolerance())
                  isInteger = false;
            }
         }
      }
      else
         isInteger = false;
      if (isInteger && !feasibilityCheck)
      {
         model->setBestSolution( model->solver()->getColSolution(),
               model->solver()->getNumCols(),
               model->solver()->getObjValue() );
         solvedByInitial = true;
      }
      else
      {
         // Set the cut generator (set the "normal" flag true, both the "atSolution"
         // and the "infeasible" flags false, the "howOftenInSub" parameter -100,
         // the "whatDepth" parameter 1, and the "whatDepthInSub" parameters -1
         userCutGen.problem = this;
         userCutGen.logFile = f;
         model->addCutGenerator( &userCutGen, 1, "UFFLP_User", true, false, false,
               -100, 1, -1 );

         // Set the primal heuristic
         userHeur->problem = this;
         userHeur->logFile = f;
         model->addHeuristic( userHeur, "UFFLP_User", 0 );

         if (feasibilityCheck)
         {
            // Make sure cut generator called correctly (a)
            int iGenerator = model->numberCutGenerators() - 1;
            model->cutGenerator(iGenerator)->setMustCallAgain(true);
            // Say cuts needed at continuous (b)
            OsiBabSolver oddCuts;
            oddCuts.setSolverType(4);
            model->passInSolverCharacteristics(&oddCuts);
            // Say no to all solutions by strong branching (c)
            CbcFeasibilityNoStrong noStrong;
            model->setProblemFeasibility(noStrong);
            // Say don't recompute solution (d)
            model->setSpecialOptions(4);
         }

         // Use the tunning from CbcMain
         model->setMinimumDrop(CoinMin(1.0,
               fabs(model->getMinimizationObjValue())*1.0e-3+1.0e-4));
         if (model->getNumCols()<500)
            // always do 100 if possible
            model->setMaximumCutPassesAtRoot(-100);
         else if (model->getNumCols()<5000)
            model->setMaximumCutPassesAtRoot(100); // use minimum drop
         else
            model->setMaximumCutPassesAtRoot(20);
         model->setMaximumCutPasses(2);
         model->setAllowablePercentageGap(0.01); // the same as the CPLEX default

         // Solve the MIP
         int ns = 5;
         if (model->getNumCols()<5000) ns = 20;
         CbcStrategyDefault strategy(0,ns,5,logLevel);
         if ((userHeur->userHeurFunc != NULL) || (userCutGen.userCutFunc != NULL)
               || feasibilityCheck)
            strategy.setupPreProcessing(0,0);
         else
            strategy.setupPreProcessing();
         model->setStrategy(strategy);
         model->messageHandler()->setLogLevel(1, 0);
         if ((model->getNumCols() > 2000) || (model->getNumRows() > 1500))
            model->setPrintFrequency(100);
         model->branchAndBound();
      }
      hasBeenSolved = true;
   }
   else
      // Solve the relaxation
      solver->initialSolve();

   if (f != NULL)
   {
      // Restore the default log message handlers
      model->solver()->passInMessageHandler( &solverLog );
      model->passInMessageHandler( &modelLog );

      // Close the log file
      fclose( f );
   }

   // Set log level zero (no log message) out of "UFFLP_Solve"
   // for (0 - B&B, 1 - solver, 3 - cut gen)
   model->messageHandler()->setLogLevel( 0, 0 );
   model->messageHandler()->setLogLevel( 1, 0 );
   model->messageHandler()->setLogLevel( 3, 0 );

   // check the optimization result
   if (hasIntegerVar)
   {
      // Check if the optimal solution has been found
      if ( solvedByInitial || model->isProvenOptimal() )
      {
         return UFFLP_Optimal;
      }

      // Check if at least one feasible solution exists
      if ( model->bestSolution() != NULL)
      {
         return UFFLP_Feasible;
      }

      // Check is the problem is infeasible
      if ( model->isProvenInfeasible() ||
            model->isInitialSolveProvenDualInfeasible() ||
            model->isInitialSolveProvenPrimalInfeasible() )
      {
         return UFFLP_Infeasible;
      }
   }
   else
   {
      // Check if the optimal solution has been found
      if ( solver->isProvenOptimal() )
      {
         return UFFLP_Optimal;
      }

      // Check is the problem is infeasible
      if ( solver->isProvenDualInfeasible() ||
            solver->isProvenPrimalInfeasible() )
      {
         return UFFLP_Infeasible;
      }
   }

   // Assume unfinished
   return UFFLP_Aborted;
}

UFFLP_ErrorType UFFProblem::getObjValue(double* value)
{
   *value = solver->getObjValue();
   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::getSolution(char* vname, double* value)
{
   // find the variable index
   std::map<std::string,int>::iterator it2 = varMap.find( vname );
   if (it2 == varMap.end()) return UFFLP_VarNameNotFound;
   int varIdx = it2->second;

   // get the variable value and store it at the user's area
   const double *solution;
   solution = solver->getColSolution();
   if (solution == NULL) return UFFLP_NoSolExists;
   *value = solution[varIdx];

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::getDualSolution(char* cname, double* value)
{
   // find the constraint index
   std::map<std::string,int>::iterator it = ctrMap.find( cname );
   if (it == ctrMap.end()) return UFFLP_ConsNameNotFound;
   int ctrIdx = it->second;

   // if inside a callback...
   if (generatingCuts || inHeuristic)
   {
      return UFFLP_InCallback;
   }

   // if the problem has integer variables
   if (hasIntegerVar)
   {
      return UFFLP_NoDualVariables;
   }

   // get the dual value and store it at the user's area
   const double *solution;
   solution = solver->getRowPrice();
   *value = solution[ctrIdx];

   return UFFLP_Ok;
}


UFFLP_ErrorType UFFProblem::writeLP(char* fname)
{
   std::map<std::string,int>::iterator it;
   static char objName[] = "obj";

   // Synchronize the cache of variables and constraints with the COIN-OR
   synchronizeProblem();

   // build a vector of variable names
   std::vector<const char*> varNames;
   varNames.resize( varMap.size(), NULL );
   for (it = varMap.begin(); it != varMap.end(); it++)
   {
      varNames[it->second] = (it->first).c_str();
   }

   // build a vector of constraint names
   std::vector<const char*> ctrNames;
   ctrNames.resize( ctrMap.size()+1, NULL );
   for (it = ctrMap.begin(); it != ctrMap.end(); it++)
   {
      ctrNames[it->second] = (it->first).c_str();
   }
   ctrNames[ctrMap.size()] = objName;

   // Check if the LP file can be open for writing
   FILE* f = fopen( fname, "wt" );
   if (f == NULL) return UFFLP_UnableOpenFile;
   fclose( f );

   // Write the problem data
   solver->writeLpNative( fname, &ctrNames[0], &varNames[0] );

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
   userCutGen.userCutFunc = cutFunc;

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::printToLog(char* message)
{
   // check if inside a callback
   if ( !generatingCuts && !inHeuristic ) return UFFLP_NotInCallback;

   // print the message to the log file
   if (generatingCuts)
   {
      if (userCutGen.logFile == NULL)
         printf( "UFFLP: %s\n", message );
      else
         fprintf( userCutGen.logFile, "UFFLP: %s\n", message );
   }
   else
   {
      if (userHeur->logFile == NULL)
         printf( "UFFLP: %s\n", message );
      else
         fprintf( userHeur->logFile, "UFFLP: %s\n", message );
   }

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setHeurCallBack(UFFLP_CallBackFunction heurFunc)
{
   // set the callback function
   userHeur->userHeurFunc = heurFunc;

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
      model->setMaximumNodes( int(value) );
      break;

   case UFFLP_TimeLimit:
      model->setMaximumSeconds( value );
      break;

   default:
      return UFFLP_InvalidParameter;
   }

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::changeObjCoeff(char* vname, double value)
{
   // check if we are in a callback context
   if ( inHeuristic || generatingCuts ) return UFFLP_InCallback;

   // find the variable index
   std::map<std::string,int>::iterator it = varMap.find( vname );
   if (it == varMap.end()) return UFFLP_VarNameNotFound;
   int varIdx = it->second;

   // set the coefficient in the objective function
   solver->setObjCoeff( varIdx, value );

   return UFFLP_Ok;
}

UFFLP_ErrorType UFFProblem::setPriority(char* vname, int prior)
{
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
      rhs = solver->getRightHandSide()[ctrIdx];
      sense = solver->getRowSense()[ctrIdx];

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

void UFFProblem::synchronizeProblem()
{
   if (!probCache.sync)
   {
      double objVal = 0.0;
      double *solution = NULL;

      // rebuild the solver if the problem has already been solved before
      if (hasBeenSolved)
      {
         // save the best feasible solution if any
         if (solver->getColSolution() != NULL)
         {
            solution = new double[solver->getNumCols()];
            memcpy( solution, solver->getColSolution(),
                  solver->getNumCols()*sizeof(double) );
            objVal = solver->getObjValue();
         }

         // destroy the solver
         delete solver;

         // recreate the solver interface
         OsiCbcSolverInterface* cbcSolver = new OsiCbcSolverInterface;
         solver = cbcSolver;

         // Get the branch-and-cut model
         model = cbcSolver->getModelPtr();

         // Set default log message handlers
         model->solver()->passInMessageHandler( &solverLog );
         model->passInMessageHandler( &modelLog );

         // Set log level zero (no log message) by default
         // for (0 - B&B, 1 - solver, 3 - cut gen)
         model->messageHandler()->setLogLevel( 0, 0 );
         model->messageHandler()->setLogLevel( 1, 0 );
         model->messageHandler()->setLogLevel( 3, 0 );
      }

      // convert the problem matrix to the COIN-OR format
      int idx = 0;
      probCache.start.push_back( idx );
      for (int c = 0; c < (int)probCache.indexMatrix.size(); c++)
      {
         for (int r = 0; r < (int)probCache.indexMatrix[c].size(); r++)
         {
            probCache.index.push_back( probCache.indexMatrix[c][r] );
            probCache.value.push_back( probCache.valueMatrix[c][r] );
         }
         idx += probCache.indexMatrix[c].size();
         probCache.start.push_back( idx );
      }

#ifdef _DEBUG
      FILE* f = fopen( "debug.txt", "at" );
      fprintf( f, "Synchronizing problem with %d columns ans %d rows\nStart =",
            probCache.collb.size(),
            probCache.rowlb.size() );
      int i;
      for (i = 0; i < (int)probCache.start.size(); i++)
         fprintf( f, " %d", probCache.start[i] );
      fprintf( f, "\nIndex =" );
      for (i = 0; i < (int)probCache.index.size(); i++)
         fprintf( f, " %d", probCache.index[i] );
      fprintf( f, "\nValue =" );
      for (i = 0; i < (int)probCache.value.size(); i++)
         fprintf( f, " %g", probCache.value[i] );
      fprintf( f, "\n" );
      fclose( f );
#endif

      // load the problem to the COIN-OR
      solver->loadProblem(
            probCache.collb.size(),
            probCache.rowlb.size(),
            &probCache.start[0],
            &probCache.index[0],
            &probCache.value[0],
            &probCache.collb[0],
            &probCache.colub[0],
            &probCache.obj[0],
            &probCache.rowlb[0],
            &probCache.rowub[0] );

      // set the variable types
      for (int c = 0; c < (int)probCache.indexMatrix.size(); c++)
      {
         if (probCache.isintcol[c])
            solver->setInteger( c );
         else
            solver->setContinuous( c );
      }

      // set the solution from the previous problem
      if (solution != NULL)
      {
         model->saveBestSolution(solution, objVal);
         delete [] solution;
      }

      // clear the cache
      probCache.sync = true;
      hasBeenSolved = false;
   }
}

UFFLP_ErrorType UFFProblem::changeBounds(char* vname, double lb, double ub)
{
   // check if we are in a callback context
   if ( inHeuristic || generatingCuts ) return UFFLP_InCallback;

   // find the variable index
   std::map<std::string,int>::iterator it = varMap.find( vname );
   if (it == varMap.end()) return UFFLP_VarNameNotFound;
   int varIdx = it->second;

   // set the bounds
   probCache.collb[varIdx] = lb;
   probCache.colub[varIdx] = ub;
   probCache.sync = false;

   return UFFLP_Ok;
}

// This is only for avoiding a link error at the COIN-OR lib
int mainTest (int argc, const char *argv[],int algorithm,
	      ClpSimplex empty, ClpSolve solveOptionsIn,
	      int switchOffValue,bool doVector)
{
    return 0;
}
