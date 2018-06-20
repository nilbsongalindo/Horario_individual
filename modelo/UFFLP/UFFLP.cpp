/****************************************************************************
* UFFLP - An easy API for Mixed, Integer and Linear Programming
*
* Programmed by Artur Alves Pessoa,
*               DSc in Computer Science at PUC-Rio, Brazil
*               Assistant Professor of Production Engineering
*               at Fluminense Federal University (UFF), Brazil
*
*****************************************************************************/

#ifndef __LINUX__

// UFFLP.cpp : Defines the entry point for the DLL application.
//

#include <windows.h>

#endif // ndef __LINUX__

#include "UFFLP.h"
#ifdef USE_CPLEX
#include "CPLEX/UFFProblem.h"
#else
#include "UFFProblem.h"
#endif

#ifndef __LINUX__

//===================================================================
// DLL MAIN (NOTHING SPECIAL)
//===================================================================

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}

#endif // ndef __LINUX__

//===================================================================
// C++ FUNCTIONS THAT CALL THE prob'S METHODS
//===================================================================

UFFProblem* CPP_UFFLP_CreateProblem(void)
{
   return new UFFProblem();
}

void CPP_UFFLP_DestroyProblem(UFFProblem* prob)
{
   delete prob;
}

UFFLP_ErrorType CPP_UFFLP_AddVariable(UFFProblem* prob, char* name,
      double lb, double ub, double obj, UFFLP_VarType type)
{
   return prob->addVariable( name, lb, ub, obj, type );
}

UFFLP_ErrorType CPP_UFFLP_SetCoefficient(UFFProblem* prob,
      char* cname, char* vname, double value)
{
   return prob->setCoefficient( cname, vname, value );
}

UFFLP_ErrorType CPP_UFFLP_AddConstraint(UFFProblem* prob, char* name,
      double rhs, UFFLP_ConsType type)
{
   return prob->addConstraint( name, rhs, type );
}

UFFLP_StatusType CPP_UFFLP_Solve(UFFProblem* prob, UFFLP_ObjSense sense)
{
   return prob->solve( sense );
}

UFFLP_ErrorType CPP_UFFLP_GetObjValue(UFFProblem* prob, double* value)
{
   return prob->getObjValue(value);
}

UFFLP_ErrorType CPP_UFFLP_GetSolution(UFFProblem* prob, char* vname,
      double* value)
{
   return prob->getSolution( vname, value );
}

UFFLP_ErrorType CPP_UFFLP_GetDualSolution(UFFProblem* prob, char* cname,
      double* value)
{
   return prob->getDualSolution( cname, value );
}

UFFLP_ErrorType CPP_UFFLP_WriteLP(UFFProblem* prob, char* fname)
{
   return prob->writeLP( fname );
}

UFFLP_ErrorType CPP_UFFLP_SetLogInfo(UFFProblem* prob, char* fname, int level)
{
   return prob->setLogInfo( fname, level );
}

UFFLP_ErrorType CPP_UFFLP_SetCutCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction cutFunc)
{
   return prob->setCutCallBack( cutFunc );
}

UFFLP_ErrorType CPP_UFFLP_PrintToLog(UFFProblem* prob, char* message)
{
   return prob->printToLog( message );
}

UFFLP_ErrorType CPP_UFFLP_SetHeurCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction heurFunc)
{
   return prob->setHeurCallBack( heurFunc );
}

UFFLP_ErrorType CPP_UFFLP_GetBestSolutionValue(UFFProblem* prob,
      double* value)
{
   return prob->getBestSolutionValue( value );
}

UFFLP_ErrorType CPP_UFFLP_SetSolution(UFFProblem* prob, char* vname,
      double value)
{
   return prob->setSolution( vname, value );
}

UFFLP_ErrorType CPP_UFFLP_SetParameter(UFFProblem* prob,
      UFFLP_ParameterType param, double value)
{
   return prob->setParameter( param, value );
}

UFFLP_ErrorType CPP_UFFLP_ChangeObjCoeff(UFFProblem* prob, char* vname,
      double value)
{
   return prob->changeObjCoeff( vname, value );
}

UFFLP_ErrorType CPP_UFFLP_SetPriority(UFFProblem* prob, char* vname,
      int prior)
{
   return prob->setPriority( vname, prior );
}

UFFLP_ErrorType CPP_UFFLP_CheckSolution(UFFProblem* prob, double toler)
{
   return prob->checkSolution( toler );
}

UFFLP_ErrorType CPP_UFFLP_SetCplexParameter(UFFProblem* prob,
      int param, UFFLP_ParamTypeType type, double value)
{
#ifdef USE_CPLEX
   return prob->setCplexParameter( param, type, value );
#else
   return UFFLP_Ok;
#endif
}

UFFLP_ErrorType CPP_UFFLP_SetIntCheckCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction intChkFunc)
{
#ifdef USE_CPLEX
   return prob->setIntCheckCallBack( intChkFunc );
#else
   prob->setFeasibilityCheck(intChkFunc != NULL);
   return UFFLP_Ok;
#endif
}

UFFLP_ErrorType CPP_UFFLP_SetInfeasible(UFFProblem* prob)
{
#ifdef USE_CPLEX
   return prob->setInfeasible();
#else
   return UFFLP_NotInIntCheck;
#endif
}

UFFLP_ErrorType CPP_UFFLP_ChangeBounds(UFFProblem* prob,
      char* vname, double lb, double ub)
{
   return prob->changeBounds( vname, lb, ub );
}

//===================================================================
// EXPORTED ANSI C FUNCTIONS
//===================================================================

extern "C" {

UFFLP_API UFFProblem* STDCALL UFFLP_CreateProblem(void)
{
   return CPP_UFFLP_CreateProblem();
}

UFFLP_API void STDCALL UFFLP_DestroyProblem(UFFProblem* prob)
{
   CPP_UFFLP_DestroyProblem( prob );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_AddVariable(UFFProblem* prob,
      char* name, double lb, double ub, double obj, UFFLP_VarType type)
{
   return CPP_UFFLP_AddVariable( prob, name, lb, ub, obj, type );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetCoefficient(UFFProblem* prob,
      char* cname, char* vname, double value)
{
   return CPP_UFFLP_SetCoefficient( prob, cname, vname, value );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_AddConstraint(UFFProblem* prob,
      char* name, double rhs, UFFLP_ConsType type)
{
   return CPP_UFFLP_AddConstraint( prob, name, rhs, type );
}

UFFLP_API UFFLP_StatusType STDCALL UFFLP_Solve(UFFProblem* prob,
      UFFLP_ObjSense sense)
{
   return CPP_UFFLP_Solve( prob, sense );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_GetObjValue(UFFProblem* prob,
      double* value)
{
   return CPP_UFFLP_GetObjValue( prob, value );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_GetSolution(UFFProblem* prob,
      char* vname, double* value)
{
   return CPP_UFFLP_GetSolution( prob, vname, value );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_GetDualSolution(UFFProblem* prob,
      char* cname, double* value)
{
   return CPP_UFFLP_GetDualSolution( prob, cname, value );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_WriteLP(UFFProblem* prob,
      char* fname)
{
   return CPP_UFFLP_WriteLP( prob, fname );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetLogInfo(UFFProblem* prob,
      char* fname, int level)
{
   return CPP_UFFLP_SetLogInfo( prob, fname, level );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetCutCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction cutFunc)
{
   return CPP_UFFLP_SetCutCallBack( prob, cutFunc );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_PrintToLog(UFFProblem* prob,
      char* message)
{
   return CPP_UFFLP_PrintToLog( prob, message );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetHeurCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction heurFunc)
{
   return CPP_UFFLP_SetHeurCallBack( prob, heurFunc );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_GetBestSolutionValue(
      UFFProblem* prob, double* value)
{
   return CPP_UFFLP_GetBestSolutionValue( prob, value);
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetSolution(UFFProblem* prob,
      char* vname, double value)
{
   return CPP_UFFLP_SetSolution( prob, vname, value );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetParameter(UFFProblem* prob,
      UFFLP_ParameterType param, double value)
{
   return CPP_UFFLP_SetParameter( prob, param, value );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_ChangeObjCoeff(UFFProblem* prob,
      char* vname, double value)
{
   return CPP_UFFLP_ChangeObjCoeff( prob, vname, value );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetPriority(UFFProblem* prob,
      char* vname, int prior)
{
   return CPP_UFFLP_SetPriority( prob, vname, prior );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_CheckSolution(UFFProblem* prob,
      double toler)
{
   return CPP_UFFLP_CheckSolution( prob, toler );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetCplexParameter(UFFProblem* prob,
      int param, UFFLP_ParamTypeType type, double value)
{
   return CPP_UFFLP_SetCplexParameter( prob, param, type, value );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetIntCheckCallBack(UFFProblem* prob,
      UFFLP_CallBackFunction intChkFunc)
{
   return CPP_UFFLP_SetIntCheckCallBack( prob, intChkFunc );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_SetInfeasible(UFFProblem* prob)
{
   return CPP_UFFLP_SetInfeasible( prob );
}

UFFLP_API UFFLP_ErrorType STDCALL UFFLP_ChangeBounds(UFFProblem* prob,
      char* vname, double lb, double ub)
{
   return CPP_UFFLP_ChangeBounds( prob, vname, lb, ub );
}

};
