#include "parser.h"

int main (){
    Data data;
    double numeroDePeriodos = 0;
    atualizaDados(&data);
    solveCoin_Y(&data, numeroDePeriodos);
    numeroDePeriodos++;
    cout << numeroDePeriodos << "  Eaeee galera";
    solveCoin(&data, numeroDePeriodos);
    return 0;
}