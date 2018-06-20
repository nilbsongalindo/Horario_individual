/****************************************************************************
* An Example Program for...
* UFFLP - An easy API for Mixed, Integer and Linear Programming
*
* Programmed by Artur Alves Pessoa,
*               DSc in Computer Science at PUC-Rio, Brazil
*               Assistant Professor of Production Engineering
*               at Fluminense Federal University (UFF), Brazil
*
*****************************************************************************/

#include "../UFFLP.h"

#include <string>
#include <sstream>
#include <iostream>

#include <stdlib.h>

const int n = 5;

double costs[n][n] = {
   {5, 3, 4, 4, 7},
   {3, 8, 5, 4, 9},
   {8, 6, 5, 7, 9},
   {6, 5, 7, 8, 7},
   {8, 4, 8, 9, 5}
};

int main(void)
{
   // create an empty problem instance
   UFFProblem* prob = UFFLP_CreateProblem();

   // create one binary variable for each machine and each task
   int m, t;
   std::string varName;
   std::stringstream s;
   for (m = 0; m < n; m++)
   {
      for (t = 0; t < n; t++)
      {
         // the variable "x_m_t" is 1 if the machine "m" executes the task "t"
         s.clear();
         s << "x_" << m << "_" << t;
         s >> varName;
         UFFLP_AddVariable(prob, (char*)varName.c_str(), 0.0, 1.0, costs[m][t],
               UFFLP_Continuous);
      }
   }

   // create onde constraint for each machine
   std::string consName;
   for (m = 0; m < n; m++)
   {
      // assembly the constraint name "mach_m"
      s.clear();
      s << "mach_" << m;
      s >> consName;

      // set the coefficients for the constraint "SUM(t=1..n) x_m_t = 1"
      for (t = 0; t < n; t++)
      {
         // add the variable "x_m_t" to the constraint
         s.clear();
         s << "x_" << m << "_" << t;
         s >> varName;
         UFFLP_SetCoefficient( prob, (char*)consName.c_str(),
               (char*)varName.c_str(), 1.0 );
      }

      // create the constraint
      UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1.0, UFFLP_Equal );
   }

   // create onde constraint for each task
   for (t = 0; t < n; t++)
   {
      // assembly the constraint name "task_t"
      s.clear();
      s << "task_" << t;
      s >> consName;

      // set the coefficients for the constraint "SUM(m=1..n) x_m_t = 1"
      for (m = 0; m < n; m++)
      {
         // add the variable "x_m_t" to the constraint
         s.clear();
         s << "x_" << m << "_" << t;
         s >> varName;
         UFFLP_SetCoefficient( prob, (char*)consName.c_str(),
               (char*)varName.c_str(), 1.0 );
      }

      // create the constraint
      UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1.0, UFFLP_Equal );
   }

   // Write the problem in LP format for debug
   UFFLP_WriteLP( prob, "assign.lp" );

   // Configure the log file and the log level = 2
   UFFLP_SetLogInfo( prob, "assign.log", 2 );

   // solve the problem
   UFFLP_StatusType status = UFFLP_Solve( prob, UFFLP_Minimize );

   // check if an optimal solution has been found
   if (status == UFFLP_Optimal)
   {
      double value;
      std::cout << "Optimal solution found!" << std::endl << std::endl;
      std::cout << "Solution:" << std::endl;

      // get the value of the objective function
      UFFLP_GetObjValue( prob, &value );
      std::cout << "Objective function value = " << value << std::endl;

      // print the value of all variables set to 1 in the solution
      for (m = 0; m < n; m++)
      {
         for (t = 0; t < n; t++)
         {
            // get the value of the variable "x_m_t"
            s.clear();
            s << "x_" << m << "_" << t;
            s >> varName;
            UFFLP_GetSolution( prob, (char*)varName.c_str(), &value );

            // if it is non-zero, then print
            if (value > 0.1)
               std::cout << varName << " = " << value << std::endl;
         }
      }
      std::cout << std::endl;

      // print the values of the machine dual variables
      value = 0.0;
      for (m = 0; m < n; m++)
      {
         // get the dual value of the constraint "mach_m"
         s.clear();
         s << "mach_" << m;
         s >> consName;
         UFFLP_GetDualSolution( prob, (char*)consName.c_str(), &value );

         // print it
         std::cout << consName << " = " << value << std::endl;
      }
      std::cout << std::endl;

      // print the values of the task dual variables
      for (t = 0; t < n; t++)
      {
         // get the dual value of the constraint "task_t"
         s.clear();
         s << "task_" << t;
         s >> consName;
         UFFLP_GetDualSolution( prob, (char*)consName.c_str(), &value );

         // print it
         std::cout << consName << " = " << value << std::endl;
      }
   }

   // check if the problem is infeasible
   else if (status == UFFLP_Infeasible)
   {
      std::cout << "The problem is infeasible!" << std::endl;
   }

   // check the other case
   else
   {
      std::cout << "It seems that the solver did not finish its job...";
      std::cout << std::endl;
   }

   // destroy the problem instance
   UFFLP_DestroyProblem( prob );

   return 0;
}
