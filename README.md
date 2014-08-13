# Crypt

Algoritmo criptográfico de transposição.  
Implementação para estudo e demonstração de seu funcionamento.

Parte do Projeto Integrado V (2014-01).

## Info

* pi.c e pi.h tem a implementação do algoritmo criptográfico,
juntamente com código para leitura e gravação em arquivos, e
processamento de dados recebidos via teclado.
* crypt.c aplica as funções em um programa capaz de demonstrar
o funcionamento do algoritmo.

## Para compilar

    make

## Como usar?

    crypt [-h] [-e | -d] [-k] [-o] FILES

Argumentos necessários (mutuamente exclusivos):

* -e    criptografa os arquivos.
* -d    descriptografa os arquivos.

Argumentos opcionais:

* -k    repete a chave criptográfica em todos os arquivos.
* -o    não remove o arquivo original.
* -h    apresenta uma mensagem de ajuda e finaliza o programa.
