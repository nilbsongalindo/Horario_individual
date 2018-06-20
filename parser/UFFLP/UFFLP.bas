Attribute VB_Name = "UFFLP"
'*******************************************************************
'* UFFLP - An easy API for Mixed, Integer and Linear Programming   *
'*                                                                 *
'* Programmed by Artur Alves Pessoa,                               *
'*               DSc in Computer Science at PUC-Rio, Brazil        *
'*               Assistant Professor of Production Engineering     *
'*               at Fluminense Federal University (UFF), Brazil    *
'*                                                                 *
'*******************************************************************

Option Explicit

' Variable types
Public Const UFFLP_Continuous = 0
Public Const UFFLP_Integer = 1
Public Const UFFLP_Binary = 2
Public Const UFFLP_SemiContinuous = 3

' Note: The SemiContinuous variable type is not supported by the COIN/Cbc
' solver. To make an application portable, one has to check for the UFFLP
' error "UnknownVarType" whenever creating a SemiContinuous variable and
' to replace such variable by both a continuous and a binary variable
' in the case.

' Constraint types
Public Const UFFLP_Less = -1
Public Const UFFLP_Equal = 0
Public Const UFFLP_Greater = 1

' Objective function senses
Public Const UFFLP_Minimize = 0
Public Const UFFLP_Maximize = 1

' Error codes
Public Const UFFLP_Ok = 0
Public Const UFFLP_VarNameExists = 1       ' variable name already exists
Public Const UFFLP_ConsNameExists = 2      ' constraint name already exists
Public Const UFFLP_VarNameNotFound = 3     ' variable name not found
Public Const UFFLP_ConsNameNotFound = 4    ' constraint name not found
Public Const UFFLP_UnableOpenFile = 5      ' unable to open file for writing
Public Const UFFLP_NotInCallback = 6       ' operation is allowed only inside a callback
Public Const UFFLP_NotInHeuristic = 7      ' operation is allowed only inside a heuristic
Public Const UFFLP_InvalidParameter = 8    ' the referred solver parameter does not exits
Public Const UFFLP_InCallback = 9          ' operation is not allowed inside a callback
Public Const UFFLP_NoDualVariables = 10    ' problems with integer variables have no dual
Public Const UFFLP_CoeffExists = 11        ' variable-constraint coefficient already exists
Public Const UFFLP_InfeasibleSol = 12      ' infeasible solution provided by heuristic
Public Const UFFLP_InvalParamType = 13     ' the parameter type does not exits
Public Const UFFLP_InNonCutCallback = 14   ' operation in a callback is allowed only for cuts
Public Const UFFLP_NotInIntCheck = 15      ' operation is allowed only in an integer check
Public Const UFFLP_NoSolExists = 16        ' the current problem has no solution
Public Const UFFLP_UnknownVarType = 17     ' trying to add a variable of unknown type

' Solution status
Public Const UFFLP_Optimal = 0             ' Optimal solution found
Public Const UFFLP_Infeasible = 1          ' The problem is infeasible or unbounded
Public Const UFFLP_Aborted = 2             ' Stopped before proving optimality
Public Const UFFLP_Feasible = 3            ' Feasible solution found but not proven optimal

' Solver parameters
Public Const UFFLP_CutoffValue = 0         ' Cutoff value for the objective function
Public Const UFFLP_NodesLimit = 1          ' Maximum number of B&B nodes to be explored
Public Const UFFLP_TimeLimit = 2           ' Maximum number of seconds to run the B&B

' CPLEX parameter types
Public Const UFFLP_IntegerParam = 0        ' integer parameter
Public Const UFFLP_FloatParam = 1          ' floating point parameter

' Bound value assumed to be infinity
Public Const UFFLP_Infinity = 1E+15

' Note: functions that are declared as private are redeclared as public later with
' an underline after the function name. For example "UFFLP_GetSolution" should be
' called as "UFFLP_GetSolution_" because the

' Create a problem
' @return a pointer to the created problem
Public Declare Function UFFLP_CreateProblem Lib "UFFLP.dll" () As Long

' Destroy a problem
' @param prob  pointer to the problem to be destroyed
Public Declare Sub UFFLP_DestroyProblem Lib "UFFLP.dll" (ByVal prob As Long)

' Insert a new variable in the problem's model
' @param prob  pointer to the problem
' @param name  variable's name
' @param lb    lower bound on the value of the new variable
' @param ub    upper bound on the value of the new variable
' @param obj   coefficent of the new variable in the objective function
' @param type  variable type (continuous, integer, binary, ...)
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_AddVariable Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef name As Byte, ByVal lb As Double, ByVal ub As Double, _
        ByVal obj As Double, ByVal type_ As Long) As Long

' Set a coefficient of a constraint before inserting it in the problem's
' model.
' @param prob  pointer to the problem
' @param cname constraint's name
' @param vname name of the variable whose coefficient is to be set
' @param value new value of the coefficient
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_SetCoefficient Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef cname As Byte, ByRef vname As Byte, ByVal value As Double) As Long

' Insert a constraint in the problem's model.
' The coefficients for the added constraint must already be set through the
' function UFFLP_SetCoefficient. If the user code is inside the context of
' a cut callback, then the constraint is inserted as a cut.
' @param prob  pointer to the problem
' @param name  constraint's name
' @param rhs   right-hand side of the constraint
' @param type  constraint type (less, equal, or greater)
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_AddConstraint Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef name As Byte, ByVal rhs As Double, ByVal type_ As Long) As Long

' Solve the problem
' @param prob  pointer to the problem
' @param sense objetive function sense (maximize or minimize)
' @return the solution status
Public Declare Function UFFLP_Solve Lib "UFFLP.dll" (ByVal prob As Long, _
       ByVal sense As Long) As Long

' Get the current value of the objective function. Inside a callback,
' the value for the current LP relaxation is returned.
' @param prob  pointer to the problem
' @param value pointer to where the value should be stored
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_GetObjValue Lib "UFFLP.dll" (ByVal prob As Long, _
       ByRef value As Double) As Long

' Get the value of a variable in the current solution. Inside a callback,
' the values for the current LP relaxation are returned.
' @param prob  pointer to the problem
' @param vname name of the variable
' @param value pointer to where the value should be stored
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_GetSolution Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef vname As Byte, ByRef value As Double) As Long

' Get the value of a dual variable in the current solution. Cannot be called
' inside a callback.
' @param prob  pointer to the problem
' @param cname name of the constraint
' @param value pointer to where the value should be stored
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_GetDualSolution Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef cname As Byte, ByRef value As Double) As Long

' Write the current model to a file in the CPLEX LP Format.
' @param prob  pointer to the problem
' @param fname name of the LP file
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_WriteLP Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef fname As Byte) As Long

' Set the name of the log file and the level of information to be reported in
' this file.
' @param prob  pointer to the problem
' @param fname name of the log file. If "", then the information is written
'              to the standard output.
' @param level level of information to be reported in the log file
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_SetLogInfo Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef fname As Byte, ByVal level As Long) As Long

' Set the address of the function that shall be called for generating user cuts.
' The function "UFFLP_ClearOverflow" should be called in the beginning of any
' callback function to avoid spurious overflow errors.
' @param cutFunc address of the cut generation function
' @return an error code or UFFLP_Ok if successful
Public Declare Function UFFLP_SetCutCallBack Lib "UFFLP.dll" (ByVal prob As Long, _
      ByVal cutFunc As Long) As Long

' Print a message to the log file (only allowed in a callback).
' @param message string message to be printed
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_PrintToLog Lib "UFFLP.dll" (ByVal prob As Long, _
      ByRef Message As Byte) As Long

' Set the address of the function that shall be called for executing user
' relaxation-based primal heuristics.
' The function "UFFLP_ClearOverflow" should be called in the beginning of any
' callback function to avoid spurious overflow errors.
' @param heurFunc address of the primal heuristic function
' @return an error code or UFFLP_Ok if successful
Public Declare Function UFFLP_SetHeurCallBack Lib "UFFLP.dll" (ByVal prob As Long, _
      ByVal heurFunc As Long) As Long

' Get the current value of the best integer solution found (only allowed in
' a heuristic callback).
' @param prob  pointer to the problem
' @param value pointer to where the value should be stored
' @return an error code or UFFLP_Ok if successful
Public Declare Function UFFLP_GetBestSolutionValue Lib "UFFLP.dll" (ByVal prob As Long, _
       ByRef value As Double) As Long

' Set the value of a variable in the integer solution provided by the user
' (only allowed in a heuristic callback). Non-provided values are considered
' as zeroes.
' @param prob  pointer to the problem
' @param vname name of the variable
' @param value value of the variable in the provided solution
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_SetSolution Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef vname As Byte, ByVal value As Double) As Long

' Set the value for a solver parameter. Both integer and floating point
' parameters are set through this function. For integer parameters, the value
' is truncated.
' @param prob  pointer to the problem
' @param param parameter to be set
' @param value new value for the parameter
' @return an error code or UFFLP_Ok if successful
Public Declare Function UFFLP_SetParameter Lib "UFFLP.dll" (ByVal prob As Long, _
        ByVal param As Long, ByVal value As Double) As Long


' Change the coefficient of a variable in the objective function. The problem
' can be solved again after that. Do not call it inside a callback.
' @param prob  pointer to the problem
' @param vname name of the variable
' @param value new value of the coefficient
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_ChangeObjCoeff Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef vname As Byte, ByVal value As Double) As Long

' Set the branching priority of a variable (CPLEX only, ignored by COIN-OR).
' Variables with higher priorities are preferred. By default, variables receive
' priority zero.
' @param prob  pointer to the problem
' @param vname name of the variable
' @param prior value of the priority assigned to the variable
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_SetPriority Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef vname As Byte, ByVal prior As Long) As Long

' Check the solution provided to the solver in the heuristic callback. If the
' solution is not feasible than print an error message in the log output
' informing the name of the first violated constraint. Must be called inside
' a heuristic callback.
' The function "UFFLP_ClearOverflow" should be called in the beginning of any
' callback function to avoid spurious overflow errors.
' @param prob  pointer to the problem
' @param toler tolerance when checking the left-hand side against the right-
'              hand side.
' @return an error code or UFFLP_Ok if successful
Public Declare Function UFFLP_CheckSolution Lib "UFFLP.dll" (ByVal prob As Long, _
        ByVal toler As Double) As Long

' Set the value for a specific CPLEX parameter. Both integer and floating point
' parameters are set through this function. For integer parameters, the value
' is truncated.
' @param prob  pointer to the problem
' @param param parameter number (according to the CPLEX manual) to be set
' @param type  type of the parameter
' @param value new value for the parameter
' @return an error code or UFFLP_Ok if successful
Public Declare Function UFFLP_SetCplexParameter Lib "UFFLP.dll" (ByVal prob As Long, _
      ByVal param As Long, ByVal type_ As Long, ByVal value As Double) As Long

' Set the address of the function that shall be called for checking whether
' solutions found by the solver are valid. This call allows one to use a MIP model
' where integer solutions may be infeasible because not all constraints are
' represented. Although it is also required when using the COIN-CBC solver, the
' callback function is never called in this case.
' @param intChkFunc address of the integer solution check function
' @return an error code or UFFLP_Ok if successful
Public Declare Function UFFLP_SetIntCheckCallBack Lib "UFFLP.dll" (ByVal prob As Long, _
      ByVal intChkFunc As Long) As Long

' Set that the integer solution currently being checked is infeasible. Must
' be called inside an integer check callback.
' @return an error code or UFFLP_Ok if successful
Public Declare Function UFFLP_SetInfeasible Lib "UFFLP.dll" (ByVal prob As Long) As Long

' Change the bounds of a variable. The problem can be solved again after that.
' Do not call it inside a callback.
' @param prob  pointer to the problem
' @param vname name of the variable
' @param lb new value of the variable's lower bound
' @param ub new value of the variable's upper bound
' @return an error code or UFFLP_Ok if successful
Private Declare Function UFFLP_ChangeBounds Lib "UFFLP.dll" (ByVal prob As Long, _
        ByRef vname As Byte, ByVal lb As Double, ByVal ub As Double) As Long

' This function should be called in the beginning of any callback function to
' avoid spurious overflow errors.
Public Sub UFFLP_ClearOverflow()
    Dim aux As Double
    
    ' This piece of code seems to clear the overflow flag avoiding spurious errors
    On Error Resume Next
    aux = 1
    On Error GoTo 0     ' Disable the "error bypassing" mode
End Sub

'=============================================================================
'=========================== Internal Functions ==============================
'=============================================================================
'
' Function for conversion of VB strings to C strings
Private Sub StrToC(ByRef str As String, ByRef conv() As Byte)
    Dim i As Integer
    ReDim conv(Len(str)) As Byte
    
    ' copy the ACSII code of each character
    For i = 1 To Len(str)
        conv(i - 1) = Asc(VBA.Mid$(str, i, 1))
    Next
    
    ' Add the trailing zero
    conv(Len(str)) = 0
End Sub

'==============================================================================
'============== Redeclaration of API functions using VB strings ===============
'==============================================================================

Public Function UFFLP_AddVariable_(ByVal prob As Long, ByRef name As String, ByVal lb As Double, ByVal ub As Double, ByVal obj As Double, ByVal type_ As Long) As Long
    Dim auxName() As Byte
    StrToC name, auxName
    UFFLP_AddVariable_ = UFFLP_AddVariable(prob, auxName(0), lb, ub, obj, type_)
End Function

Public Function UFFLP_SetCoefficient_(ByVal prob As Long, ByRef cname As String, ByRef vname As String, ByVal value As Double) As Long
    Dim auxCons() As Byte
    Dim auxVar() As Byte
    StrToC cname, auxCons
    StrToC vname, auxVar
    UFFLP_SetCoefficient_ = UFFLP_SetCoefficient(prob, auxCons(0), auxVar(0), value)
End Function

Public Function UFFLP_AddConstraint_(ByVal prob As Long, ByRef name As String, ByVal rhs As Double, ByVal type_ As Long) As Long
    Dim auxName() As Byte
    StrToC name, auxName
    UFFLP_AddConstraint_ = UFFLP_AddConstraint(prob, auxName(0), rhs, type_)
End Function

Public Function UFFLP_GetSolution_(ByVal prob As Long, ByRef vname As String, ByRef value As Double) As Long
    Dim auxName() As Byte
    Dim aux As Double
    StrToC vname, auxName
    UFFLP_GetSolution_ = UFFLP_GetSolution(prob, auxName(0), aux)
        
    ' Avoid spurious overflow errors
    UFFLP_ClearOverflow
    
    value = aux
End Function

Public Function UFFLP_GetDualSolution_(ByVal prob As Long, ByRef cname As String, ByRef value As Double) As Long
    Dim auxName() As Byte
    Dim aux As Double
    StrToC cname, auxName
    UFFLP_GetDualSolution_ = UFFLP_GetDualSolution(prob, auxName(0), aux)
    
    ' Avoid spurious overflow errors
    UFFLP_ClearOverflow
    
    value = aux
End Function

Public Function UFFLP_GetObjValue_(ByVal prob As Long, ByRef value As Double) As Long
    Dim aux As Double
    UFFLP_GetObjValue_ = UFFLP_GetObjValue(prob, aux)
    
    ' Avoid spurious overflow errors
    UFFLP_ClearOverflow
    
    value = aux
End Function

Public Function UFFLP_WriteLP_(ByVal prob As Long, ByRef fname As String) As Long
    Dim auxName() As Byte
    StrToC fname, auxName
    UFFLP_WriteLP_ = UFFLP_WriteLP(prob, auxName(0))
End Function

Public Function UFFLP_SetLogInfo_(ByVal prob As Long, ByRef fname As String, ByVal level As Long) As Long
    Dim auxName() As Byte
    StrToC fname, auxName
    UFFLP_SetLogInfo_ = UFFLP_SetLogInfo(prob, auxName(0), level)
End Function

Public Function UFFLP_PrintToLog_(ByVal prob As Long, ByRef Message As String) As Long
    Dim auxName() As Byte
    StrToC Message, auxName
    UFFLP_PrintToLog_ = UFFLP_PrintToLog(prob, auxName(0))
End Function

Public Function UFFLP_SetSolution_(ByVal prob As Long, ByRef vname As String, ByVal value As Double) As Long
    Dim auxName() As Byte
    StrToC vname, auxName
    UFFLP_SetSolution_ = UFFLP_SetSolution(prob, auxName(0), value)
End Function

Public Function UFFLP_ChangeObjCoeff_(ByVal prob As Long, ByRef vname As String, ByVal value As Double) As Long
    Dim auxName() As Byte
    StrToC vname, auxName
    UFFLP_ChangeObjCoeff_ = UFFLP_ChangeObjCoeff(prob, auxName(0), value)
End Function

Public Function UFFLP_SetPriority_(ByVal prob As Long, ByRef vname As String, ByVal prior As Long) As Long
    Dim auxName() As Byte
    StrToC vname, auxName
    UFFLP_SetPriority_ = UFFLP_SetPriority(prob, auxName(0), prior)
End Function

Public Function UFFLP_ChangeBounds_(ByVal prob As Long, ByRef vname As String, ByVal lb As Double, ByVal ub As Double) As Long
    Dim auxName() As Byte
    StrToC vname, auxName
    UFFLP_ChangeBounds_ = UFFLP_ChangeBounds(prob, auxName(0), lb, ub)
End Function

