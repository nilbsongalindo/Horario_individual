/****************************************************************************
* UFFLP - An easy API for Mixed, Integer and Linear Programming
*
* Programmed by Artur Alves Pessoa,
*               DSc in Computer Science at PUC-Rio, Brazil
*               Assistant Professor of Production Engineering
*               at Fluminense Federal University (UFF), Brazil
*
*****************************************************************************/

#ifndef __UFF_PROBLEM_H__
#define __UFF_PROBLEM_H__

#include <string>
#include <map>
#include <vector>
#include <stdio.h>

#include "UFFLP.h"

#include "/home/mateus/Cbc-2.4.0/include/coin/CoinMessageHandler.hpp"
#include "/home/mateus/Cbc-2.4.0/include/coin/CglCutGenerator.hpp"
#include "/home/mateus/Cbc-2.4.0/include/coin/CbcHeuristic.hpp"

/*
#include <CoinMessageHandler.hpp>
#include <CglCutGenerator.hpp>
#include <CbcHeuristic.hpp>
*/
class OsiSolverInterface;
class CbcModel;
class UFFProblem;

struct Constraint
{
   std::vector<int> indices;     // constraint indices
   std::vector<double> coeffs;   // constraint coefficients
   bool added;      // true when the constraint has already been added
};

class UFFCutGenerator : public CglCutGenerator
{
public:
   friend class UFFProblem;

   virtual void generateCuts( const OsiSolverInterface & si, OsiCuts & cs,
         const CglTreeInfo info = CglTreeInfo()) const;

   virtual CglCutGenerator * clone() const;

private:
   // Problem pointer
   UFFProblem* problem;

   // User's cut generation function
   UFFLP_CallBackFunction userCutFunc;

   // Current log file
   FILE* logFile;
};

class UFFPrimalHeuristic : public CbcHeuristic
{
public:
   friend class UFFProblem;

   // Constructor
   UFFPrimalHeuristic( CbcModel & model );

   // Clone
   virtual CbcHeuristic * clone() const;

   // Resets stuff if model changes
   virtual void resetModel(CbcModel * model);

   // returns 0 if no solution, 1 if valid solution
   // with better objective value than one passed in
   // Sets solution values if good, sets objective value
   // This is called after cuts have been added - so can not add cuts
   virtual int solution(double & objectiveValue,
		       double * newSolution);

private:
   // Problem pointer
   UFFProblem* problem;

   // User's primal heuristic function
   UFFLP_CallBackFunction userHeurFunc;

   // Current log file
   FILE* logFile;
};

class UFFProblem
{
   // stores variables, coefficients and constraints
   struct ProblemCache
   {
      bool sync;  // when true, the problem has already been synchronized

      // matrix of column coefficient indices
      std::vector< std::vector<int> > indexMatrix;

      // matrix of column coefficient values
      std::vector< std::vector<double> > valueMatrix;

      // parameters needed to load the problem
      std::vector<CoinBigIndex> start;
      std::vector<int> index;
      std::vector<double> value;
      std::vector<double> collb;
      std::vector<double> colub;
      std::vector<double> obj;
      std::vector<double> rowlb;
      std::vector<double> rowub;

      // determines whether each variable is integer
      std::vector<bool> isintcol;
   };

public:
   friend class UFFCutGenerator;
   friend class UFFPrimalHeuristic;

   // Constructor/Destructor
   UFFProblem();
   ~UFFProblem();

   // Insert a new variable into the problem
   UFFLP_ErrorType addVariable( char* name, double lb, double ub, double obj,
      UFFLP_VarType type );

   // Set a coefficient of a constraint before inserting it in the problem's
   // model.
   UFFLP_ErrorType setCoefficient(char* cname, char* vname, double value);

   // Insert a constraint in the problem's model.
   // The coefficients for the added constraint must already be set through the
   // method setCoefficient. If the user code is inside the context of a cut
   // callback, then the constraint is inserted as a cut.
   UFFLP_ErrorType addConstraint(char* name, double rhs, UFFLP_ConsType type);

   // Solve the problem. Return the solution status.
   UFFLP_StatusType solve(UFFLP_ObjSense sense);

   // Get the current value of the objective function. Inside a callback,
   // the value for the current LP relaxation is returned.
   UFFLP_ErrorType getObjValue(double* value);

   // Get the value of a variable in the current solution. Inside a callback,
   // the values for the current LP relaxation are returned.
   UFFLP_ErrorType getSolution(char* vname, double* value);

   // Get the value of a dual variable in the current solution. Cannot be
   // called inside a callback.
   UFFLP_ErrorType getDualSolution(char* cname, double* value);

   // Write the current model to a file in the CPLEX LP Format.
   UFFLP_ErrorType writeLP(char* fname);

   // Set the name of the log file and the level of information to be reported
   // in this file.
   UFFLP_ErrorType setLogInfo(char* fname, int level);

   // Set the address of the function that shall be call for generating user
   // cuts
   UFFLP_ErrorType setCutCallBack(UFFLP_CallBackFunction cutFunc);

   // Print a message to the log file (only allowed in a callback).
   UFFLP_ErrorType printToLog(char* message);

   // Set the address of the function that shall be called for executing user
   // relaxation-based primal heuristics
   UFFLP_ErrorType setHeurCallBack(UFFLP_CallBackFunction heurFunc);

   // Get the current value of the best integer solution found (only allowed in
   // a heuristic callback).
   UFFLP_ErrorType getBestSolutionValue(double* value);

   // Set the value of a variable in the integer solution provided by the user
   // (only allowed in a heuristic callback). Non-provided values are considered
   // as zeroes.
   UFFLP_ErrorType setSolution(char* vname, double value);

   // Set the value for a solver parameter. Both integer and floating point
   // parameters are set through this function. For integer parameters, the
   // value is truncated.
   UFFLP_ErrorType setParameter(UFFLP_ParameterType param, double value);

   // Change the coefficient of a variable in the objective function. The
   // problem can be solved again after that. Do not call it inside a callback.
   UFFLP_ErrorType changeObjCoeff(char* vname, double value);

   // Set the branching priority of a variable (CPLEX only, ignored by COIN-OR).
   // Variables with higher priorities are preferred. By default, variables receive
   // priority zero.
   UFFLP_ErrorType setPriority(char* vname, int prior);

   // Check the solution provided to the solver in the heuristic callback. If the
   // solution is not feasible than print an error message in the log output
   // informing the name of the first violated constraint. Must be called inside
   // a heuristic callback.
   UFFLP_ErrorType checkSolution(double toler);

   // Change the bounds of a variable. The problem can be solved again after that.
   // Do not call it inside a callback.
   UFFLP_ErrorType changeBounds(char* vname, double lb, double ub);

   // Indicate that the integer feasible solution may be infeasible for the
   // complete problem
   inline void setFeasibilityCheck(bool value)
   { feasibilityCheck = value; };

private:

   // set the problem data from the cache to the CBC
   void synchronizeProblem();

   // map of variable-constraint coefficients to indices in the ctrCoeff map
   std::map<std::string, int> coeffCtrMap;

   // map of variable-constraint coefficients to indices in the cutCoeff map
   std::map<std::string, int> coeffCutMap;

   // Pointer to the solver interface
   OsiSolverInterface* solver;

   // Pointer to the branch-and-cut model
   CbcModel* model;

   // Default log message handler for the solver interface
   CoinMessageHandler solverLog;

   // Default log message handler for the branch-and-cut model
   CoinMessageHandler modelLog;

   // Log file name ("" if writing in the standard output)
   std::string logFileName;

   // Current log level
   int logLevel;

   // Map of variable names to variable indices in the solver
   std::map<std::string,int> varMap;

   // Map of constraint names to constraint indices in the solver
   std::map<std::string,int> ctrMap;

   // Map of constraint names to constraint coefficients
   std::map<std::string,Constraint> ctrCoeffMap;

   // Map of cut names to cut coefficients
   std::map<std::string,Constraint> cutCoeffMap;

   // Map of variable (column) names to column coefficients
   std::map<std::string,Constraint> varCoeffMap;

   // UFFLP cut generation object
   UFFCutGenerator userCutGen;

   // Flag that indicates that the current context is the cut generation
   bool generatingCuts;

   // Pointer to the current cut collection
   OsiCuts* cutCollection;

   // UFFLP primal heuristic object
   UFFPrimalHeuristic* userHeur;

   // Flag that indicates that the current context is the primal heuristic
   bool inHeuristic;

   // Flag that indicates that a solution has been provided by the primal
   // heuristic
   bool newSolutionSet;

   // The current value of the best integer solution
   double bestIntSolValue;

   // Pointer to the current integer solution's buffer
   double* primalSolution;

   // The cutoff value parameter that must be set inside the method "solve"
   // where the objective sense is already known
   double cutoffValue;

   // Flag that indicates that the problem has at least one integer variable
   bool hasIntegerVar;

   // Flag the indicates that the MIP problem has already been solved at least
   // once
   bool hasBeenSolved;

   // Flag the indicates that the integer feasible solution may be infeasible
   // for the complete problem
   bool feasibilityCheck;

   // stores the created variables, coefficients and constraints to send the the
   // COIN-CBC all toghether
   ProblemCache probCache;
};

#endif
