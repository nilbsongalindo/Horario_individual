#ifndef FUNCOES_H
#define FUNCOES_H

#include <iostream>
#include <fstream>
#include <sstream>
#include "UFFLP/UFFLP.h"

#define LIMITE_CREDITOS 5
#define MIN_PERIODOS 5
#define CRED_ESTAGIO 5

using namespace std;

struct Data{
    /*
     *  Disciplinas referentes aos identificadores
     */
    int *identificador;
    float *creditos;
    int *preRequisito;
    int **preReqI;
    bool **choqueHorarioI;
    int *situacao;
    int disciplinas;
    int numPeriodosFaltantes;
    int numDisciplinasOp;
};

void atualizaDados(Data *data);
void solveCoin(Data *data);
void solveCoin_Y(Data *data);

#endif