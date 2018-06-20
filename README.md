# **Coleta de dados para otimização do Horário Individual de alunos do CI - UFPB**
A maioria das universidades, dentre elas a UFPB (Universidade Federal da Praíba), adotam o sistema semestral de ensino. Dado o **PPC** (Projeto Pedacógico de Curso) de cada curso, as disciplinas que devem ser cursadas são organizadas de tal forma que o aluno ingressante pode concluir o curso no tempo previsto de acordo com a ordem sugerida de disciplinas. Devido ao tempo pessoal, reprovações, entre outros motivos, os alunos acabam recorrendo a disciplinas em horários e períodos diferentes do proposto no PPC, dando origem a necessidade de encontrar a melhor forma de organizar as disciplinas para que o curso seja concluído no menor tempo possível

Dito isso, alguns alunos do CI (Centro de Informática da UFPB) iniciaram o desenvolvimento do **SACI (Sistema de Apoio a Decisões do Centro de Informática)**, que visa solucionar alguns problemas do Centro, dentre eles a organização do horário individual para os alunos matriculados nos cursos. O transtorno causado pela organização da própria agenda se dá geralmente pela dificuldade de prever os melhores horários, baseado em fatores como: disciplinas com pré-requisitos, choque de horários com outras disciplinas ou atividades pessoais, disciplinas difíceis no mesmo período, entre outros.

Dado a quantidade de variáveis para se levar em consideração, o SACI propõe-se a coletar alguns dados do usuário de forma automática e exibir para o mesmo a melhor combinação de disciplinas, baseando-se nas necessidades do indivíduo. Para tanto um script foi desenvolvido a fim de obter tais informações, para garantir que cada estudante possa obter resultados otimizados, baseado nas disciplinas cursadas e pendentes do curso. 

    
## **O Script**

O script responsável pela coleta de dados acessa o **SIGAA (Sistema Integrado de Gestão de Atividades Acadêmicas)** para que seja possível obter as informações necessárias para o SACI. É escrito em python e para solicitar as requisições da página de acesso ao SIGAA e conseguir entrar no sistema, utiliza alguns módulos como:

 - [requests](http://docs.python-requests.org/en/master/)
- [pandas](https://pandas.pydata.org/pandas-docs/stable/)
- [json](https://docs.python.org/3/library/json.html)
- [lxml](http://lxml.de/)

#### Requests

O módulo requests permite que o programador envie requisições HTTP sem a necessidade de trabalho manual. Nike, Twitter, Spotify, Microsoft, Amazon, Lyft, BuzzFeed, Reddit, The NSA, Her Majesty's Government, Google, Twilio, Runscope, Mozilla, Heroku, PayPal, NPR, Obama for America, Transifex, Native Instruments, The Washington Post, SoundCloud, Kippt, Sony, e Instituições Federais dos Estados Unidos que não preferem ser nomeadas afirmam que utilizam o Requests internamente.

#### Pandas

O pandas é um pacote de python que provê rapidez, flexibilidade e ótimas estruturas de dados construídas para facilitar o trabalho com coleta de dados mais fácil e intuitivo.

#### Json

O JSON é uma ferramenta leve que auxilia na troca de dados entre linguagens, baseado em JavaScript.

#### Lxml

Lxml é uma poderisa API utilizada para realizar análise de arquivos XML e HTML. 