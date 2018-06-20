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

#include "..\UFFLP.h"

#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

typedef std::vector<bool> VectorBool;
typedef std::vector<VectorBool> MatrixBool;

//============== GLOBAL VARIABLES (use with caution) ============
int n;   // number of vertices
int m;   // number of edges
int heuristicCalls = 0;

// Adjacency matrix: edges[i][j] is true if and only if the
//                   vertex i is incompatible with vertex j
MatrixBool edges;

// Initial solution read from a text file
std::vector<int> initialSolution;

// ReadInputFile: Reads an input text file containing an instance of the
//                Maximum Independent Set Problem.
// @param fileName  Name of the input file
bool ReadInputFile( char* fileName )
{
   // try to open the input file
   std::ifstream f;
   f.open( fileName, std::ifstream::in );
   if ( f.fail() )
   {
      std::cout << "Fail to open file " << fileName << " for reading";
      std::cout << std::endl;
      return false;
   }

   // read the number of vertices and edges
   f >> n;
   f >> m;

   // allocate the adjacency matrix
   edges.clear();
   edges.resize( n );
   int i;
   for (i = 0; i < n; i++)
   {
      edges[i].resize( n, false );
   }

   // read the edges
   int j, e;
   for (e = 0; e < m; e++)
   {
      f >> i;
      f >> j;
      edges[i-1][j-1] = true;
      edges[j-1][i-1] = true;
   }

   // close the input file
   f.close();

   // try to open the initial solution file
   std::stringstream s;
   std::string solutionFile;
   s << fileName << ".sol";
   s >> solutionFile;
   f.open( solutionFile.c_str(), std::ifstream::in );
   if ( f.fail() )
   {
      std::cout << "Fail to open file " << solutionFile << " for reading";
      std::cout << std::endl;
      return false;
   }

   // read the number of vertices
   int k;
   f >> k;

   // read the solution vertices
   initialSolution.clear();
   initialSolution.resize( k );
   for (i = 0; i < k; i++)
   {
      f >> initialSolution[i];
      initialSolution[i]--;
   }

   // close the initial solution file
   f.close();

   return true;
}

// Heuristic: Use the initial solution read from a text file
// @param prob  Pointer to the problem
void __stdcall Heuristic( UFFProblem* prob )
{
   std::stringstream s;
   std::string varName;

   // only run the heuristic on the first call
   if (heuristicCalls > 0)
   {
      heuristicCalls++;
      return;
   }
   heuristicCalls++;

   // print information to the log file
   {
      std::stringstream message;
      message << "Loading initial solution with " << initialSolution.size()
            << " vertices";
      UFFLP_PrintToLog( prob, (char*)message.str().c_str() );
   }

   // send the integer solution to the solver
   int i;
   for (i = 0; i < (int)initialSolution.size(); i++)
   {
      // set the value of the corresponding variable "x_i" to one
      s.clear();
      s << "x_" << initialSolution[i]+1;
      s >> varName;
      UFFLP_SetSolution( prob, (char*)varName.c_str(), 1.0 );
   }
}

int main( int argc, char* argv[] )
{
   // check the programs argument
   if (argc != 2)
   {
      std::cout << "Use: MaxInSet.exe <filename>" << std::endl;
      return 1;
   }

   // read the input file
   if ( !ReadInputFile(argv[1]) )
      return 2;

   // create an empty problem instance
   UFFProblem* prob = UFFLP_CreateProblem();

   std::cout << "Creating variables..." << std::endl;

   // create one binary variable for each vertex
   int i;
   std::stringstream s;
   std::string varName;
   for (i = 0; i < n; i++)
   {
      // the variable "x_i" is 1 if the vertex "i" is used
      s.clear();
      s << "x_" << i+1;
      s >> varName;
      UFFLP_AddVariable(prob, (char*)varName.c_str(), 0.0, 1.0,
            1.0, UFFLP_Binary);
   }

   std::cout << std::endl << "Creating constraints..." << std::endl;

   // create one constraint for each edge
   std::string consName;
   int j;
   for (i = 0; i < n-1; i++)
   {
      for (j = i+1; j < n; j++)
      {
         // skip the non-existing edges
         if (!edges[i][j]) continue;

         // assembly the constraint name "edge_i_j"
         s.clear();
         s << "edge_" << i+1 << "_" << j+1;
         s >> consName;

         // set the coefficients for the constraint "x_i + x_j <= 1"
         // add the variable "x_i" to the constraint
         s.clear();
         s << "x_" << i+1;
         s >> varName;
         UFFLP_SetCoefficient( prob, (char*)consName.c_str(),
               (char*)varName.c_str(), 1.0 );

         // add the variable "x_j" to the constraint
         s.clear();
         s << "x_" << j+1;
         s >> varName;
         UFFLP_SetCoefficient( prob, (char*)consName.c_str(),
               (char*)varName.c_str(), 1.0 );

         // create the constraint
         UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1.0,
               UFFLP_Less );
      }
   }

   std::cout << std::endl << "Solving the problem..." << std::endl;

   // Write the problem in LP format for debug
   UFFLP_WriteLP( prob, "MaxInSet.lp" );

   // Configure the log file and the log level = 2
   UFFLP_SetLogInfo( prob, "MaxInSet.log", 2 );

   // Set the primal heuristic callback
   UFFLP_SetHeurCallBack( prob, Heuristic );

   // solve the problem
   UFFLP_StatusType status = UFFLP_Solve( prob, UFFLP_Maximize );

   // check if an optimal solution has been found
   if ((status == UFFLP_Optimal) || (status == UFFLP_Feasible))
   {
      double value;
      if (status == UFFLP_Optimal)
         std::cout << "Optimal solution found!" << std::endl << std::endl;
      else
         std::cout << "Feasible solution found!" << std::endl << std::endl;
      std::cout << "Solution:" << std::endl;

      // get the value of the objective function
      UFFLP_GetObjValue( prob, &value );
      std::cout << "Objective function value = " << value << std::endl;

      // print the value of all variables set to 1 in the solution
      for (i = 0; i < n; i++)
      {
         // get the value of the variable "x_i"
         s.clear();
         s << "x_" << i+1;
         s >> varName;
         UFFLP_GetSolution( prob, (char*)varName.c_str(), &value );

         // if it is non-zero, then print
         if (value > 0.1)
            std::cout << varName << " = " << value << std::endl;
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
