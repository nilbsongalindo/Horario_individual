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

typedef std::vector<double> VectorDouble;
typedef std::vector<VectorDouble> MatrixDouble;

// GLOBAL VARIABLES (use with caution)
int n;   // number of facilities
int m;   // number of cities

// ReadInputFile: Reads an input file containing an instance of the
//                Uncapacitated Facility Location Problem in the ORLIB format.
// @param fileName  Name of the input file
// @param openCost  Vector of costs for openning each potential facility
// @param allocCost Matrix of allocation costs: allocCost[j][i] is the cost of
//                  all demand to city i from facility j
bool ReadInputFile( char* fileName, VectorDouble& openCost,
      MatrixDouble& allocCost )
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

   // read the number of facilities and cities
   f >> n;
   f >> m;

   // read the open costs (and skip the capacities)
   int j;
   double aux;
   openCost.clear();
   for (j = 0; j < n; j++)
   {
      f >> aux;
      f >> aux;
      openCost.push_back(aux);
   }

   // read the allocation costs (and skip the demands)
   allocCost.clear();
   allocCost.resize( n );
   int i;
   for (i = 0; i < m; i++)
   {
      f >> aux;
      for (j = 0; j < n; j++)
      {
         f >> aux;
         allocCost[j].push_back(aux);
      }
   }

   // close the input file and return ok
   f.close();
   return true;
}

// CutGenerator: Generate cuts of the form "x_i_j <= y_j"
// @param fileName  Pointer to the problem
void __stdcall CutGenerator( UFFProblem* prob )
{
   double x_i_j, y_j;
   std::stringstream s, message;
   std::string xName, yName, cutName;
   int nCuts = 0;

   // find a "x_i_j" greater than "y_j"
   int i, j;
   for (j = 0; j < n; j++)
   {
      // get the value of the variable "y_j"
      s.clear();
      s << "y_" << j;
      s >> yName;
      UFFLP_GetSolution( prob, (char*)yName.c_str(), &y_j );

      for (i = 0; i < m; i++)
      {
         // get the value of the variable "x_i_j"
         s.clear();
         s << "x_" << i << "_" << j;
         s >> xName;
         UFFLP_GetSolution( prob, (char*)xName.c_str(), &x_i_j );

         // check if the cut is violated
         // -> NEVER insert a cut when the violation is too small
         if (x_i_j > y_j + 0.01)
         {
            // set the cut name
            s.clear();
            s << "CUT_" << i << "_" << j;
            s >> cutName;

            //insert the cut
            UFFLP_SetCoefficient( prob, (char*)cutName.c_str(),
                  (char*)xName.c_str(), 1.0 );
            UFFLP_SetCoefficient( prob, (char*)cutName.c_str(),
                  (char*)yName.c_str(), -1.0 );
            UFFLP_AddConstraint(prob, (char*)cutName.c_str(), 0.0, UFFLP_Less);
            nCuts++;
         }
      }
   }

   // print information to the log file
   message.clear();
   message << nCuts << " cuts inserted";
   UFFLP_PrintToLog( prob, (char*)message.str().c_str() );
}

int main( int argc, char* argv[] )
{
   VectorDouble openCost;
   MatrixDouble allocCost;

   // check the programs argument
   if (argc != 2)
   {
      std::cout << "Use: FacLoc.exe <filename>" << std::endl;
      return 1;
   }

   // read the input file
   if ( !ReadInputFile(argv[1], openCost, allocCost) )
      return 2;

   // create an empty problem instance
   UFFProblem* prob = UFFLP_CreateProblem();

   std::cout << "Creating variables..." << std::endl;

   // create one binary variable for each facility
   int j;
   std::stringstream s;
   std::string varName;
   for (j = 0; j < n; j++)
   {
      // the variable "y_j" is 1 if the facility "j" is open
      s.clear();
      s << "y_" << j;
      s >> varName;
      UFFLP_AddVariable(prob, (char*)varName.c_str(), 0.0, 1.0,
            openCost[j], UFFLP_Binary);
   }
   std::cout << ".";

   // create one binary variable for each city and each facility
   int i;
   for (i = 0; i < m; i++)
   {
      for (j = 0; j < n; j++)
      {
         // the variable "x_i_j" is 1 if the demand to city "i" is allocated
         // from facility "j"
         s.clear();
         s << "x_" << i << "_" << j;
         s >> varName;
         UFFLP_AddVariable(prob, (char*)varName.c_str(), 0.0, 1.0,
               allocCost[j][i], UFFLP_Binary);
      }
      std::cout << ".";
   }

   std::cout << std::endl << "Creating constraints..." << std::endl;

   // create one constraint for each city
   std::string consName;
   for (i = 0; i < m; i++)
   {
      // assembly the constraint name "city_i"
      s.clear();
      s << "city_" << i;
      s >> consName;

      // set the coefficients for the constraint "SUM(j=1..n) x_i_j = 1"
      for (j = 0; j < n; j++)
      {
         // add the variable "x_i_j" to the constraint
         s.clear();
         s << "x_" << i << "_" << j;
         s >> varName;
         UFFLP_SetCoefficient( prob, (char*)consName.c_str(),
               (char*)varName.c_str(), 1.0 );
      }

      // create the constraint
      UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1.0,
            UFFLP_Equal );
      std::cout << ".";
   }

   // create onde constraint for each facility
   for (j = 0; j < n; j++)
   {
      // assembly the constraint name "open_j"
      s.clear();
      s << "open_" << j;
      s >> consName;

      // set the coefficients for the constraint "SUM(i=1..m) x_i_j <= m y_j"
      for (i = 0; i < m; i++)
      {
         // add the variable "x_i_j" to the constraint
         s.clear();
         s << "x_" << i << "_" << j;
         s >> varName;
         UFFLP_SetCoefficient( prob, (char*)consName.c_str(),
               (char*)varName.c_str(), 1.0 );
      }

      // add the term "- m y_j" to the constraint
      s.clear();
      s << "y_" << j;
      s >> varName;
      UFFLP_SetCoefficient( prob, (char*)consName.c_str(),
            (char*)varName.c_str(), -m );

      // create the constraint
      UFFLP_AddConstraint( prob, (char*)consName.c_str(), 0.0,
            UFFLP_Less );
      std::cout << ".";
   }

   std::cout << std::endl << "Solving the problem..." << std::endl;

   // Write the problem in LP format for debug
   UFFLP_WriteLP( prob, "FacLoc.lp" );

   // Configure the log file and the log level = 2
   UFFLP_SetLogInfo( prob, "FacLoc.log", 2 );

   // Set the cut generation callback
   UFFLP_SetCutCallBack( prob, CutGenerator );

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
      for (j = 0; j < n; j++)
      {
         // get the value of the variable "y_j"
         s.clear();
         s << "y_" << j;
         s >> varName;
         UFFLP_GetSolution( prob, (char*)varName.c_str(), &value );

         // if it is non-zero, then print
         if (value > 0.1)
         {
            std::cout << varName << " = " << value << std::endl;

            // only print the city variables if the facility is open
            for (i = 0; i < m; i++)
            {
               // get the value of the variable "x_i_j"
               s.clear();
               s << "x_" << i << "_" << j;
               s >> varName;
               UFFLP_GetSolution( prob, (char*)varName.c_str(), &value );

               // if it is non-zero, then print
               if (value > 0.1)
                  std::cout << varName << " = " << value << std::endl;
            }
         }
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
