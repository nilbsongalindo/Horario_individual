#ifndef PARSER_H
#define PARSER_H

#include "nlohmann/json.hpp"
#include <iostream> 
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include "UFFLP/UFFLP.h"

#define MAX_DISCIPLINAS 92
#define LIMITE_CREDITOS 32

using namespace std;
using json = nlohmann::json; //Para utiliar as funções da biblioteca do JSON

struct Data{
    /*
     *  Disciplinas referentes aos identificadores
     */
    int *identificador;
    string *nomeCadeira;
    int *creditos;
    int *preRequisito;
    int **preReqI;
    bool **choqueHorarioI;
    int *situacao;
    int disciplinas;
    int numPeriodos;
    int numDisciplinasOp;
    int numCreditosOp;
    int *identificadorOb;
    int *identificadorOp;
};

extern void atualizaDados (Data *data);
extern void solveCoin(Data *data, double numeroDePeriodos);
extern double solveCoin_Y(Data *data, double &numeroDePeriodos);

#endif