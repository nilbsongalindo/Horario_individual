/****************************************************************************
* UFFLP - An easy API for Mixed, Integer and Linear Programming
*
* Programmed by Artur Alves Pessoa,
*               DSc in Computer Science at PUC-Rio, Brazil
*               Assistant Professor of Production Engineering
*               at Fluminense Federal University (UFF), Brazil
*
*****************************************************************************/

#ifndef __UFF_LP_H__
#define __UFF_LP_H__

#ifdef __LINUX__

#define UFFLP_API
#define STDCALL

#else // def __LINUX__

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the UFFLP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// UFFLP_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef UFFLP_EXPORTS
#define UFFLP_API __declspec(dllexport)
#else
#define UFFLP_API __declspec(dllimport)
#endif
#define STDCALL __stdcall

#endif // ndef __LINUX__

enum UFFLP_VarType
{
   UFFLP_Continuous,
   UFFLP_Integer,
   UFFLP_Binary,
   UFFLP_SemiContinuous
};

// Note: The SemiContinuous variable type is not supported by the COIN/Cbc
// solver. To make an application portable, one has to check for the UFFLP
// error UnknownVarType whenever creating a SemiContinuous variable and
// to replace such variable by both a continuous and a binary variable
// in the case.

enum UFFLP_ConsType
{
   UFFLP_Less = -1,
   UFFLP_Equal,
   UFFLP_Greater
};

enum UFFLP_ObjSense
{
   UFFLP_Minimize,
   UFFLP_Maximize
};

enum UFFLP_ErrorType
{
   UFFLP_Ok,
   UFFLP_VarNameExists,    // variable name already exists
   UFFLP_ConsNameExists,   // constraint name already exists
   UFFLP_VarNameNotFound,  // variable name not found
   UFFLP_ConsNameNotFound, // constraint name not found
   UFFLP_UnableOpenFile,   // unable to open file for writing
   UFFLP_NotInCallback,    // operation is allowed only in a callback
   UFFLP_NotInHeuristic,   // operation is allowed only in a heuristic
   UFFLP_InvalidParameter, // the referred solver parameter does not exits
   UFFLP_InCallback,       // operation is not allowed inside a callback
   UFFLP_NoDualVariables,  // problems with integer variables have no dual
   UFFLP_CoeffExists,      // variable-constraint coefficient already exists
   UFFLP_InfeasibleSol,    // infeasible solution provided by heuristic
   UFFLP_InvalParamType,   // the parameter type does not exits
   UFFLP_InNonCutCallback, // operation in a callback is allowed only for cuts
   UFFLP_NotInIntCheck,    // operation is allowed only in an integer check
   UFFLP_NoSolExists,      // the current problem has no solution
   UFFLP_UnknownVarType    // trying to add a variable of unknown type
};

enum UFFLP_StatusType
{
   UFFLP_Optimal,          // Optimal solution found
   UFFLP_Infeasible,       // The problem is infeasible or unbounded
   UFFLP_Aborted,          // Stopped before proving optimality
   UFFLP_Feasible          // Feasible solution found but not proven optimal
};

enum UFFLP_ParameterType
{
   UFFLP_CutoffValue,      // Cutoff value for the objective function
   UFFLP_NodesLimit,       // Maximum number of B&B nodes to be explored
   UFFLP_TimeLimit         // Maximum number of seconds to run the B&B
};

enum UFFLP_ParamTypeType
{
   UFFLP_IntegerParam,     // integer parameter
   UFFLP_FloatParam        // floating point parameter
};

// Bound value assumed to be infinity
const double UFFLP_Infinity = 1E15;

class UFFProblem;

#ifndef WIN32
void STDCALL UFFLP_DummyCallBackFunction( UFFProblem* prob );
typedef __typeof__(&UFFLP_DummyCallBackFunction) UFFLP_CallBackFunction;
#else
typedef void (STDCALL* UFFLP_CallBackFunction)( UFFProblem* );
#endif

extern "C" {

// Create a problem
// @return a pointer to the created problem
UFFLP_API UFFProblem* STDCALL UFFLP_CreateProblem(void);

// Destroy a problem
// @param prob  pointer to the problem to be destroyed
UFFLP_API void STDCALL UFFLP_DestroyProblem(UFFProblem* prob);

// Insert a new variable in the problem's model
// @param prob  pointer to the problem
// @param name  variable's name
// @param lb    lower bound on the value of the new variable
// @param ub    upper bound on the value of the new variable
// @param obj   coefficent of the new variable in the objective function
// @param type  variable type (continuous, integer, binary, ...)
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_AddVariable(UFFProblem* prob,
      char* name, double lb, double ub, double obj, UFFLP_VarType type);

// Set a coefficient of a constraint before inserting it in the problem's
// model.
// @param prob  pointer to the problem
// @param cname constraint's name
// @param vname name of the variable whose coefficient is to be set
// @param value new value of the coefficient
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetCoefficient(UFFProblem* prob,
      char* cname, char* vname, double value);

// Insert a constraint in the problem's model.
// The coefficients for the added constraint must already be set through the
// function UFFLP_SetCoefficient. If the user code is inside the context of
// a cut callback, then the constraint is inserted as a cut.
// @param prob  pointer to the problem
// @param name  constraint's name
// @param rhs   right-hand side of the constraint
// @param type  variable type (less, equal, or greater)
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_AddConstraint(UFFProblem* prob,
      char* name, double rhs, UFFLP_ConsType type);

// Solve the problem
// @param prob  pointer to the problem
// @param sense objetive function sense (maximize or minimize)
// @return the solution status
UFFLP_API UFFLP_StatusType STDCALL UFFLP_Solve(UFFProblem* prob,
      UFFLP_ObjSense sense);

// Get the current value of the objective function. Inside a callback,
// the value for the current LP relaxation is returned.
// @param prob  pointer to the problem
// @param value pointer to where the value should be stored
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_GetObjValue(UFFProblem* prob,
      double* value);

// Get the value of a variable in the current solution. Inside a callback,
// the values for the current LP relaxation are returned.
// @param prob  pointer to the problem
// @param vname name of the variable
// @param value pointer to where the value should be stored
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_GetSolution(UFFProblem* prob,
      char* vname, double* value);

// Get the value of a dual variable in the current solution. Cannot be called
// inside a callback.
// @param prob  pointer to the problem
// @param cname name of the constraint
// @param value pointer to where the value should be stored
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_GetDualSolution(UFFProblem* prob,
      char* cname, double* value);

// Write the current model to a file in the CPLEX LP Format.
// @param prob  pointer to the problem
// @param fname name of the LP file
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_WriteLP(UFFProblem* prob,
      char* fname);

// Set the name of the log file and the level of information to be reported in
// this file.
// @param prob  pointer to the problem
// @param fname name of the log file. If "", then the information is written
//              to the standard output.
// @param level level of information to be reported in the log file
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetLogInfo(UFFProblem* prob,
      char* fname, int level);

// Set the address of the function that shall be called for generating user cuts
// @param cutFunc address of the cut generation function
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetCutCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction cutFunc);

// Print a message to the log file (only allowed in a callback).
// @param message string message to be printed
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_PrintToLog(UFFProblem* prob,
      char* message);

// Set the address of the function that shall be called for executing user
// relaxation-based primal heuristics
// @param heurFunc address of the primal heuristic function
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetHeurCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction heurFunc);

// Get the current value of the best integer solution found (only allowed in
// a heuristic callback).
// @param prob  pointer to the problem
// @param value pointer to where the value should be stored
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_GetBestSolutionValue(
      UFFProblem* prob, double* value);

// Set the value of a variable in the integer solution provided by the user
// (only allowed in a heuristic callback). Non-provided values are considered
// as zeroes.
// @param prob  pointer to the problem
// @param vname name of the variable
// @param value value of the variable in the provided solution
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetSolution(UFFProblem* prob,
      char* vname, double value);

// Set the value for a solver parameter. Both integer and floating point
// parameters are set through this function. For integer parameters, the value
// is truncated.
// @param prob  pointer to the problem
// @param param parameter to be set
// @param value new value for the parameter
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetParameter(UFFProblem* prob,
      UFFLP_ParameterType param, double value);

// Change the coefficient of a variable in the objective function. The problem
// can be solved again after that. Do not call it inside a callback.
// @param prob  pointer to the problem
// @param vname name of the variable
// @param value new value of the coefficient
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_ChangeObjCoeff(UFFProblem* prob,
      char* vname, double value);

// Set the branching priority of a variable (CPLEX only, ignored by COIN-OR).
// Variables with higher priorities are preferred. By default, variables receive
// priority zero.
// @param prob  pointer to the problem
// @param vname name of the variable
// @param prior value of the priority assigned to the variable
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetPriority(UFFProblem* prob,
      char* vname, int prior);

// Check the solution provided to the solver in the heuristic callback. If the
// solution is not feasible than print an error message in the log output
// informing the name of the first violated constraint. Must be called inside
// a heuristic callback.
// @param prob  pointer to the problem
// @param toler tolerance when checking the left-hand side against the right-
//              hand side.
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_CheckSolution(UFFProblem* prob,
      double toler);

// Set the value for a specific CPLEX parameter. Both integer and floating point
// parameters are set through this function. For integer parameters, the value
// is truncated.
// @param prob  pointer to the problem
// @param param parameter number (according to the CPLEX manual) to be set
// @param type  type of the parameter
// @param value new value for the parameter
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetCplexParameter(UFFProblem* prob,
      int param, UFFLP_ParamTypeType type, double value);

// Set the address of the function that shall be called for checking whether
// solutions found by the solver are valid. This call allows one to use a MIP model
// where integer solutions may be infeasible because not all constraints are
// represented. Although it is also required when using the COIN-CBC solver, the
// callback function is never called in this case.
// @param intChkFunc address of the integer solution check function
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetIntCheckCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction intChkFunc);

// Set that the integer solution currently being checked is infeasible. Must
// be called inside an integer check callback.
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetInfeasible(UFFProblem* prob);

// Change the bounds of a variable. The problem can be solved again after that.
// Do not call it inside a callback.
// @param prob  pointer to the problem
// @param vname name of the variable
// @param lb new value of the variable's lower bound
// @param ub new value of the variable's upper bound
// @return an error code or UFFLP_Ok if successful
UFFLP_API UFFLP_ErrorType STDCALL UFFLP_ChangeBounds(UFFProblem* prob,
      char* vname, double lb, double ub);

};

#endif
