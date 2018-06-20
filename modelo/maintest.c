#include <iostream>
#include <fstream>
#include <sstream>
#include "UFFLP/UFFLP.h"

using namespace std;
#define LIMITE_CREDITOS 5
#define MIN_PERIODOS 4
#define CRED_ESTAGIO 5

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
    int disciplinas = 12;
    int numPeriodosFaltantes = 5;
    int numDisciplinasOp = 0;
};

int solveCoin(Data *data){int identificador[12] = {0};
//***********************************************************************************************
    float creditos[12] = {0};
    int preRequisito[12] = {0};
    int situacao[12] = {0};
    
    /*
     * INICIA CRIAÇÃO DA MATRIZ DE CHOQUE DE HORÁRIOS 
     */

    string horario[12];
    bool choqueHorario[12][12]; //Armazena se tem choque de horario ou não
    ifstream arquivoChoque("choque.txt", ios::in);
    
    
    for (int i = 0; i < 12; i++){
        arquivoChoque >> horario[i];
    }

    for (int i = 0; i < 12; i++){
        arquivoChoque >> horario[i];
    }

    for (int i = 0; i < 12; i++){
        for (int j = 0; j < 12; j++){
            if (horario[i] == horario[j])
                choqueHorario[i][j] = true;
            else
                choqueHorario[i][j] = false;
        }
    }

    /*
    for (int i = 0; i < 12; i++){
        for (int j = 0; j < 12; j++){
            cout << choqueHorario[i][j] << " ";
        }
        cout << endl;
    }*/

    //----------------------------------------------------------------------

    string line;
    ifstream arquivo("instancia.txt", ios::in);
    
    //Armazenando identificador da disciplina
    for (int i = 0; i < 12; i++){
        arquivo >> identificador[i];
    }

    //Armazenando crédito da disciplina
    for (int i = 0; i < 12; i++){
        arquivo >> creditos[i];
    }

    //Armazenando Pré-requisito da disciplina
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


    //Armazenando se disciplina foi paga ou não
    for (int i = 0; i < 12; i++){
        arquivo >> situacao[i];
    }

    //Choque de horário

    data->identificador = identificador;
    data->creditos = creditos;
    data->preRequisito = preRequisito;
    data->situacao = situacao;
//***********************************************************************************************

    //Preenche matriz de pre-requisitos direto em i
    data->preReqI = new int*[data->disciplinas];
    for (int i = 0; i < data->disciplinas; i++){
        data->preReqI[i] = new int[data->disciplinas];
        for (int j = 0; j < data->disciplinas; j++){
            data->preReqI[i][j] = preReqI[i][j];
        }
    }

    //Preenche matriz de choque de horário direto em i
    data->choqueHorarioI = new bool*[data->disciplinas];
    for (int i = 0; i < data->disciplinas; i++){
        data->choqueHorarioI[i] = new bool[data->disciplinas];
        for (int j = 0; j < data->disciplinas; j++){
            data->choqueHorarioI[i][j] = choqueHorario[i][j];
        }
    }

    // Cria problema
    UFFProblem* prob = UFFLP_CreateProblem();

    // Cria variaveis X
	string varName, consName;
  	stringstream s;

    for (int i = 0; i < data->disciplinas; i++) {
		for (int j = 0; j < data->numPeriodosFaltantes; j++) {
                s.clear();
				s << "X(" << i << "," << j << ")";
				s >> varName;
		        UFFLP_AddVariable(prob, (char*)varName.c_str(), 0.0, 1.0, j, UFFLP_Binary);
		}
	}

    //PRIMEIRA RESTRIÇÃO DO MODELO - LIMITE DE CRÉDITOS POR PERÍODO
    for (int j = 0; j < data->numPeriodosFaltantes; j++){
        s.clear();
        s << "LimCreditos_" << j;
        s >> consName;

        for (int i = 0; i < data->disciplinas; i++){
            s.clear();
            s << "X(" << i << "," << j << ")";
   	        s >> varName;
   	        UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), data->creditos[i]);
        }
        UFFLP_AddConstraint( prob, (char*)consName.c_str(), LIMITE_CREDITOS, UFFLP_Less);
    }

    //SEGUNDA RESTRIÇÃO
    for (int i = 0; i < data->disciplinas; i++){ // Usando número de disciplinas faltantes pois é a mesma quantidade de disciplinas obrigatórias
		if (!situacao[i]){
			s.clear();
			s << "Obrigatorias_" << i;
			s >> consName;
				for(int j = 0; j < data->numPeriodosFaltantes; j++){
				    s.clear();
				    s << "X(" << i << "," << j << ")";
		   	        s >> varName;
		   	        UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), 1);
				}
			UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1, UFFLP_Equal);
        }
    }

    //TERCEIRA RESTRIÇÃO DO MODELO - obriga mínimo de créditos de optativas
    s.clear();
    s << "MinOptativas_";
    s >> consName;
    for (int i = 0; i < data->numDisciplinasOp; i++){
        for (int j = 0; j < data->numPeriodosFaltantes; j++){
            s.clear();
            s << "X(" << i << "," << j << ")";
   	        s >> varName;
   	        UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), data->creditos[i]);
        }
    }
    UFFLP_AddConstraint( prob, (char*)consName.c_str(), data->numDisciplinasOp, UFFLP_Greater);

    //QUARTA RESTRIÇÃO- PAGA OP APENAS UMA VEZ POR PERÍODO
    for (int i = 0; i < data->numDisciplinasOp; i++){ // Usando número de disciplinas faltantes pois é a mesma quantidade de disciplinas obrigatórias
		if (!situacao[i]){
			s.clear();
			s << "OpUmaVez_" << i;
			s >> consName;
				for(int j = 0; j < data->numPeriodosFaltantes; j++){
				    s.clear();
				    s << "X(" << i << "," << j << ")";
		   	        s >> varName;
		   	        UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), 1);
				}
			UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1, UFFLP_Less);
        }
    }

    //QUINTA RESTRIÇÃO - PRE-REQUISITOS
    for (int i = 0; i < data->disciplinas; i++){
        for (int k = 0; k < data->disciplinas; k++){
            if (data->preReqI[i][k]){
                s.clear();
                s << "PreReq_" << i << "_" << k;
                s >> consName;

                for(int j = 0; j < data->numPeriodosFaltantes; j++){
                    s.clear();
                    s << "X(" << i << "," << j << ")";
                    s >> varName;
                    UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), j);
        
                    s.clear();
                    s << "X(" << k << "," << j << ")";
                    s >> varName;
                    UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), -j);
                }

                UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1, UFFLP_Greater);
            }
        }
    }

    
    //SEXTA RESTRIÇÃO
    for(int j = 0; j < data->numPeriodosFaltantes; j++){
        for(int i = 0; i < data->disciplinas; i++){
            for(int k = 0; k < data->disciplinas; k++){
                
                if ((data->choqueHorarioI[i][k] == true) && (i != k)){
                    s.clear();
                    s << "Choq_Horario_" << j << "_" << i << "_" << k;
                    s >> consName;

                    s.clear();
                    s << "X(" << k << "," << j << ")";
                    s >> varName;
                    UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), 1);

                    s.clear();
                    s << "X(" << i << "," << j << ")";
                    s >> varName;
                    UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), 1);
                    UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1, UFFLP_Less);
                }
                
            }
        }
    }
    
    /*
    //SÉTIMA RESTRIÇÃO - PERÍODOS NECESSÁRIOS PARA TÉRMINO DO CURSO-N CONSIDERA CONJUNTO ME
    for(int i; i < data->disciplinas; i++){ // Entender contra barra e adicionar conjunto do estagio e monografia
        s.clear();
        s << "Periodos_Neces_";
        s >> consName;
        for(int j = 0; j <= data->numPeriodosFaltantes; j++){
             s.clear();
             s << "X(" << i << "," << j << ")";
   	         s >> varName;
             UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), data->numPeriodosFaltantes);
    
        }
        UFFLP_AddConstraint( prob, (char*)consName.c_str(), MIN_PERIODOS, UFFLP_Greater);
    }
    */
    //OITAVA RESTRIÇÃO - FORÇA ESTAGIO E MONOGRAFIA PARA O ULTIMO PERÍODO
    /*for (int i = 0; i < conj_estagio_monografia; i++){
        s.clear();
        s << "Estagio_Monografia_";
        s >> consName;
        for (int j = 0; j < data -> numPeriodosFaltantes; j++){
            s.clear();
            s << "X(" << i << "," << j << ")";
   	        s >> varName;

            UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), data->numPeriodosFaltantes);
        }
        UFFLP_AddConstraint( prob, (char*)consName.c_str(), MIN_PERIODOS, UFFLP_Greater);
    }*/

    //NONA RESTRIÇÃO
    /*for (int i = 0; i < data->disciplinas; i++){
        s.clear();
        s << "Paga_Disciplina_";
        s >> consName;
        for (int j = 0; j < data->numPeriodosFaltantes; j++){
            s.clear();
            s << "X(" << i << "," << j << ")";
   	        s >> varName;

            UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), 1);
        }
        UFFLP_AddConstraint( prob, (char*)consName.c_str(), 1, UFFLP_Binary);
    }*/

/*--------------------------------------------------------------------------------------------------------------------------------------------
    //DÉCIMA RESTRIÇÃO
    s.clear();
    s << "Y";
   	s >> varName;
    UFFLP_SetCoefficient( prob, (char*)consName.c_str(),(char*)varName.c_str(), 1);
    UFFLP_AddConstraint( prob, (char*)consName.c_str(), MIN_PERIODOS, UFFLP_Greater); // Min_Periodos é o proprio y?
--------------------------------------------------------------------------------------------------------------------------------------------*/
    // Escreve modelo no arquivo .lp
    UFFLP_WriteLP( prob, "PAAA.lp" );

    UFFLP_StatusType status = UFFLP_Solve( prob, UFFLP_Minimize );

    if (status == UFFLP_Optimal){

        double value;
        cout << "Solucao otima encontrada!" << endl << endl;
        cout << "Solucao:" << endl;

        UFFLP_GetObjValue( prob, &value );
        cout << "Valor da funcao objetivo = " << value << endl;


        // Imprime valor das variaveis nao-nulas

            for (int i = 0; i < data->disciplinas; i++) {
                for (int j = 0; j < data->numPeriodosFaltantes; j++) {

                s.clear();
                s << "X(" << i << "," << j << ")";
                s >> varName;
                UFFLP_GetSolution( prob, (char*)varName.c_str(), &value );

                if (value > 0.1) {
                cout << varName << " = " << value << endl;
                    }
            }
        }
        cout << endl;/*
            for (int i = 0; i < data->numPrepostos; i++) {
                for (int j = 0; j < data->numAudiencias; j++) {

                s.clear();
                s << "Y(" << i << "," << j << ")";
                s >> varName;
                UFFLP_GetSolution( prob, (char*)varName.c_str(), &value );

                if (value > 0.1) {
                cout << varName << " = " << value << endl;
                    }
            }*/
    }
    else
        cout << "Não foi encontrada uma solução ótima..." << endl;


    // Destroy the problem instance
    UFFLP_DestroyProblem( prob );

	return 0;
}


int main(){    
    /*
     *  Disciplinas referentes aos identificadores
     */
    Data* data = new Data();
    solveCoin(data);
}