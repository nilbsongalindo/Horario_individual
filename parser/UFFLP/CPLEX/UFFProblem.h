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

#include <cplex.h>

#include <string>
#include <map>
#include <vector>

#include "../UFFLP.h"

struct Constraint
{
   std::vector<int> indices;     // constraint indices
   std::vector<double> coeffs;   // constraint coefficients
   bool added;       // true when the constraint has already been added
   double rhs;       // right-hand side
   char sense;       // less when 'L', equal when 'E' or greater when 'G'
};

struct CplexParamValue
{
   UFFLP_ParamTypeType type;
   double value;
};

class UFFProblem
{
public:

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

   // Set the value for a specific CPLEX parameter. Both integer and floating point
   // parameters are set through this function. For integer parameters, the value
   // is truncated.
   UFFLP_ErrorType setCplexParameter(int param, UFFLP_ParamTypeType type,
         double value);

   // Set the address of the function that shall be called for checking whether
   // solutions found by the solver are valid.
   UFFLP_ErrorType setIntCheckCallBack(UFFLP_CallBackFunction intChkFunc);

   // Set that the integer solution currently being checked is infeasible. Must
   // be called inside an integer check callback.
   UFFLP_ErrorType setInfeasible();

   // Change the bounds of a variable. The problem can be solved again after
   // that. Do not call it inside a callback.
   UFFLP_ErrorType changeBounds(char* vname, double lb, double ub);

   // Generate user cuts: this method is called from the cut callback internal
   // function and calls the cut callback user function
   void generateCuts( void* cbdata, int wherefrom );

   // CheckIntegerSol: this method is called from the incumbent callback
   // internal function and calls the solution check callback user function.
   // It avoids using infeasible integer solutions when the initial
   // formulation is not complete.
   void checkIntegerSol(double objval, double *x, int *isfeas_p);

   // User heuristic: this method is called from the heuristic callback
   // internal function and calls the heuristic callback user function
   bool doHeuristic(void *cbdata, int wherefrom, double* objValue,
         double* solution);

private:
   // map of variable-constraint coefficients to indices in the ctrCoeff map
   std::map<std::string, int> coeffCtrMap;

   // map of variable-constraint coefficients to indices in the cutCoeff map
   std::map<std::string, int> coeffCutMap;

   // Map of variable (column) names to column coefficients
   std::map<std::string,Constraint> varCoeffMap;

   // Pointer to the solver environment
   static CPXENVptr env;

   // number of instances of this class using the environment
   static int envCount;

   // Pointer to a problem for the solver
   CPXLPptr lp;

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

   // Map of values for CPLEX parameters
   std::map<int,CplexParamValue> paramValues;

   // Flag that indicates that the current context is the cut generation
   bool generatingCuts;

   // Flag that indicates that the current context is the cut generation for
   // an integer solution
   bool checkingSolution;

   // Flag that indicates that the current solution being check is feasible
   bool solutionIsFeasible;

   // Pointer to the current callback data
   void* callbackData;

   // Flag that indicates that the current context is the primal heuristic
   bool inHeuristic;

   // Flag that indicates that a solution has been provided by the primal
   // heuristic
   bool newSolutionSet;

   // The current value of the best integer solution
   double bestIntSolValue;

   // Pointer to the current integer solution's buffer
   double* primalSolution;

   // Pointer to the current node LP solution's buffer
   double* nodeLpSolution;

   // The current node LP solution's objective value
   double nodeLpValue;

   // Cutoff value for the objective function
   double cutoffValue;

   // User's cut generation function
   UFFLP_CallBackFunction userCutFunc;

   // User's primal heuristic function
   UFFLP_CallBackFunction userHeurFunc;

   // User's function to check whether an integer solutin is valid
   UFFLP_CallBackFunction userIntChkFunc;

   // Current log file
   CPXFILEptr logFile;

   // Flag that indicates that the problem has at least one integer variable
   bool hasIntegerVar;
};

#endif
