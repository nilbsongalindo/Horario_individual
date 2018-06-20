"""
Script que realzia login no SIGAA e, posteriormente,obtém os componentes curriculares do aluno.
Já modificado para logar em contas que possuam mais de um vinculo com a UFPB.
"""
import requests
import pandas as pd
import json
from lxml import html

USERNAME = "usuario" #Nome de usuário e senha do aluno
PASSWORD = "senha"

LOGIN_URL = "https://sigaa.ufpb.br/sigaa/index.jsf" 
LOGIN_URL_MULT_VINCULOS = "https://sigaa.ufpb.br/sigaa/vinculos.jsf" 
URL = "https://sigaa.ufpb.br/sigaa/portal/discente/integralizacao/"
URL2 = "https://sigaa.ufpb.br/sigaa/portal/discente/integralizacao/dados/"

def main():
    session_requests = requests.session() #Necessário para salvar a sessão

    # Login
    result = session_requests.get(LOGIN_URL)

    # Criando o  payload para ser submetido no form de login
    payload = {
        "form": "form",
        "form:width": 1920,
        "form:height": 1080,
        "form:login": USERNAME,
        "form:senha": PASSWORD,
        "form:entrar": "Entrar",
        "javax.faces.ViewState": "j_id1",
    }

    headers = dict(
        referer = LOGIN_URL,
        enctype = "application/x-www-form-urlencoded",
    )

    #Faz login no sigaa
    result = session_requests.post(LOGIN_URL, data=payload, headers=headers)

    # Criando o  payload para ser submetido no form de escolha de vinculo
    payload = {
        "j_id_jsp_1522167007_1": "j_id_jsp_1522167007_1",
        "javax.faces.ViewState": "j_id1",
        "j_id_jsp_1522167007_2": "j_id_jsp_1522167007_2",
        "vinculo": "1",
    }

    headers = dict(
        referer = LOGIN_URL_MULT_VINCULOS,
        enctype = "application/x-www-form-urlencoded",
    )

    resultado = session_requests.post(LOGIN_URL_MULT_VINCULOS, data=payload, headers=headers)


    resultado = session_requests.get(URL)

    componentes = session_requests.get(URL2).json()

    componentes_log = session_requests.get(URL2)

    #Criando a tabela com as informações das disciplinas

#    df = pd.DataFrame.from_dict(componentes['disciplinas'])
#    df.to_csv('tabela.csv', sep='\t')

    #Criando json com as informações das disciplinas
    df = pd.DataFrame.from_dict(componentes['disciplinas'])
    df.to_json('instancia.json')    
    
    #listas com disciplinas pendentes e concluidas
    pendentes = list()
    concluidas = list()
    #Atualemente as listas não estão sendo utilizadas, mas pode servir para modificações futuras
    for i in range(0,len(componentes['disciplinas'])):
        matriculado = componentes['disciplinas'][i]['situacao']
        if matriculado == 'PENDENTE':
            pendentes.append(componentes['disciplinas'][i]['nome'])
        elif matriculado == 'CONCLUIDO':
            concluidas.append(componentes['disciplinas'][i]['nome'])

if __name__ == '__main__':
    main()
