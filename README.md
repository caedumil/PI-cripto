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

## Como compilar?

    make

## Como usar?

`crypt -e [arquivo.txt]` para encriptar o conteudo do arquivo.

`crypt -d [arquivo.txt]` para decriptar o conteudo do arquivo.

`crypt -h` apresenta mensagem de ajuda e encerra o programa.
