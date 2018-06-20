#include <iostream>
#include <fstream>

using namespace std;
#define LIMITE_CREDITOS 3


int main(){
    /*
     *  Disciplinas referentes aos identificadores
     */
    int identificador[12] = {0};
    float creditos[12] = {0};
    int preRequisito[12] = {0};
    int situacao[12] = {0};
    
    string line;
    
    
    string horario[12];
    int choqueHorario[12][12]; //Armazena se tem choque de horario ou não
    ifstream arquivo("choque.txt", ios::in);
 
    for (int i = 0; i < 12; i++){
        arquivo >> horario[i];
    }

    for (int i = 0; i < 12; i++){
        for (int j = 0; j < 12; j++){
            if (horario[i] == horario[j])
                choqueHorario[i][j] = 1;
            else
                choqueHorario[i][j] = 0;
        }
    }

    for (int i = 0; i < 12; i++){
        for (int j = 0; j < 12; j++){
            cout << choqueHorario[i][j] << " ";
        }
        cout << endl;
    }
/*
    ifstream arquivo("instancia.txt", ios::in);
    
    //Armazenando identificador da disciplina
    for (int i = 0; i < 12; i++){
        arquivo >> identificador[i];
    }

    //Armazenando crédito da disciplina
    for (int i = 0; i < 12; i++){
        arquivo >> creditos[i];
    }

    int preReqI[12][12];
    //Armazenando Pré-requisito da disciplina
    for (int i = 0; i < 12; i++){
        arquivo >> preRequisito[i];
    }

    for (int i = 0; i < 12; i++){
        for (int j = 0; j < 12; j++){
            if (preRequisito[i] == identificador[j])
                preReqI[i][j] = preRequisito[i];
            else
                preReqI[i][j] = 0;
        }
    }

    for (int i = 0; i < 12; i++){
        for (int j = 0; j < 12; j++){
            cout << preReqI[i][j] << " ";
        }
        cout << endl;
    }

    //Armazenando se disciplina foi paga ou não
    for (int i = 0; i < 12; i++){
        arquivo >> creditos[i];
    }
*/
    return 0;
}