/*  Copyright (c) 2014, Carlos Millett
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation and/or
 *  other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pi.h"

/*  set_term() desabilita o retorno de caracteres no terminal. Basicamente a
 *  função pega os atributos atuais do terminal rodando o programa e salva em
 *  uma struct.
 *  Para desabilitar o retorno é feita uma operação com bits para negar os
 *  atributos relacionados somente com o ECHO, negando-o logicamente com o
 *  operador '~'.
 */
#ifdef __linux__
#include <termios.h>

int set_term(void){
    struct termios term;

    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~ECHO;

    if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1 )
        return 0;
    return 1;
}
#elif _WIN32
#include <windows.h>

int set_term(void){
    HANDLE hStdin;
    DWORD sett;

    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    sett = 0;

    GetConsoleMode(hStdin, &sett);
    sett &= ~ENABLE_ECHO_INPUT;

    if( SetConsoleMode(hStdin, sett) == 0 )
        return 0;
    return 1;
}
#endif

/*  dest_name() usa o nome do arquivo a ser processado como base para o nome do
 *  arquivo de saida. Dependendo das opções usadas na invocação do programa,
 *  o arquivo nomeado aqui pode ser ou não temporário.
 *  Essa função só é executada caso nenhum nome tenha sido especificado ao
 *  executar o programa.
 */
char *dest_name(const char *filename, bool is_enc){
    char enc[5] = ".enc";
    char dec[5] = ".dec";
    char *ch, *name;

    name = calloc(strlen(filename)+5, sizeof *name);

    strcpy(name, filename);

    if( (ch = strstr(name, enc)) ||\
        (ch = strstr(name, dec)) ){
        *ch = 0;
    }

    strcat(name, ( is_enc ) ? enc : dec);

    return name;
}

/*  pre_crypt() lê o arquivo de entrada em blocos de BLOCK_SIZE bytes por vez,
 *  até o fim do arquivo. Cada bloco é processado e gravado no arquivo de saída
 *  antes de um novo bloco ser lido. O último byte dos blocos é '\0' para
 *  garantir a marcação o final da string.
 *  Ler o arquivo em blocos fixos permite controlar a quantidade de memória
 *  usada pelo programa.
 *  Para determinar qual das duas funçóes, encrypt() e decrypt(), será chamada
 *  é usado um ponterio para função que recebe o endereço da função
 *  correspondente ao modo de operação escolhido.
 */
void pre_crypt(FILE *file, FILE *saveas, int *order, bool is_enc){
    char *before, *after;
    char *(*crypt)(const char*, const int, const int*);
    int size;

    before = calloc(BLOCK_SIZE, sizeof *before);
    crypt = ( is_enc ) ? encrypt : decrypt;

    while( ! feof(file) ){
        memset(before, 0, BLOCK_SIZE);
        size = fread(before, sizeof *before, BLOCK_SIZE-1, file);
        after = crypt(before, size, order);
        fwrite(after, sizeof *after, size, saveas);
        free(after);
    }

    memset(order-1, 0, *(order-1));
    free(order-1);
    free(before);
}

/*  crack_the_code()
 *  A chave criptografica é copiada em um novo vetor 'ord_pass'. 'ord_pass' é
 *  usado para arrumar os itens em ordem alfabetica usando o algoritmo de
 *  ordenacao shellsort.
 *  Depois de ordenado, 'ord_pass' e a chave original 'pass' são comparadas
 *  para encontrar a posicao de cada letra de 'ord_pass' em 'pass'.
 *  Essa ordem é importante para criptografar e descriptografar o texto, e
 *  fica armazenada no vetor de inteiros 'code', que é o retorno da função.
 *  A primeira posição de 'code' guarda seu tamanho, e o ponteiro retornado
 *  aponta para a segunda posição, escondendo a informação do tamanho do
 *  vetor.
 */
int *crack_the_code(const char *pass){
    int i, j, k, plen;
    int gap[8] = {1, 4, 10, 23, 57, 132, 301, 701};
    int *code;
    char tmp;
    char *ord_pass;

    plen = strlen(pass);
    code = calloc(plen+1, sizeof *code);
    ord_pass = calloc(plen+1, sizeof *ord_pass);

    strcpy(ord_pass, pass);

    for( i = 7; i >= 0; i--){
        for( j = gap[i]; j < plen; j++ ){
            tmp = ord_pass[j];
            for( k = j; (k >= gap[i]) && (ord_pass[k-gap[i]] > tmp); k -= gap[i] ){
                ord_pass[k] = ord_pass[k - gap[i]];
            }
            ord_pass[k] = tmp;
        }
    }

    code[0] = plen;

    for( i = 0; i <= plen; i++ ){
        for( j = 0; j < plen; j++ ){
            if( ord_pass[i] == pass[j] )
                code[i+1] = j;
        }
    }

    free(ord_pass);

    return code+1;
}

/*  __crypt() são as funções que modificam o arquivo.
 *  O bloco 'before' é percorrido em intervalos e guardada em um novo bloco
 *  'after'. O intervalo é igual ao tamanho da palavra-chave. Uma palavra chave
 *  de 6 letras faz o texto ser percorrido 0,6,12,18... depois 1,7,13,19... e
 *  assim por diante.
 *  A posição das letras na palavra-chave também interfere no modo que o texto é
 *  percorrido, por exemplo, CRIPTO faz com que os indices e seus múltiplos
 *  sejam percorridos na ordem 0,6... 2,8... 5,11... 3,9... 1,7... 4,10...
 *  Essa ordem é obtida em crack_the_code().
 *  A diferença entre encrypt() e decrypt() está na maneira que o vetor é
 *  percorrido e reorganizado.
 */
char *encrypt(const char *before, const int size, const int *order){
    char *after;
    int i, j, n;

    after = calloc(size+1, sizeof *after);
    n = 0;

    for( i = 0; i < *(order-1); i++ ){
        for( j = 0; (order[i]+j) < size; j += *(order-1) ){
            after[n++] = before[order[i]+j];
        }
    }

    return after;
}

char *decrypt(const char *before, const int size, const int *order){
    char *after;
    int i, j, n;

    after = calloc(size+1, sizeof *after);
    n = 0;

    for( i = 0; i < *(order-1); i++ ){
        for( j = 0; (order[i]+j) < size; j += *(order-1) ){
            after[order[i]+j] = before[n++];
        }
    }

    return after;
}

/*  get_passwd() aloca uma região na memória, com tamanho definido pela
 *  chamada da função, para receber a chave criptográfica recebida por
 *  teclado.
 */
char *get_passwd(const char *text, const int size){
    char *in, *pass;

    in = calloc(size, sizeof *in);

    fprintf(stdout, "%s", text);

    set_term();
    fgets(in, size, stdin);
    putchar('\n');

    pass = strtok(in, "\n");

    if( ! valid_passwd(pass) ){
        free(in);
        return get_passwd(text, size);
    }
    return pass;
}

/*  valid_passwd() verifica se a chave contém ou não caracteres repetidos.
 *  A função procura pela ocorrência do primeiro caractere do vetor, enquanto
 *  a única ocorrência for a da primeira posição, a função continua até
 *  verificar o vetor por completo. Qualquer caractere que se repita no vetor
 *  faz a função retornar 'false'.
 */
bool valid_passwd(const char *pass){
    while( strrchr(pass, *pass) == pass ){
        if( *(pass++) == 0 )
            return true;
    }
    return false;
}

/*  erase_passwd() apaga da memória a chave criptografica escrevendo '0's antes
 *  de liberar a região alocada.
 *  A função finaliza setando NULL no ponteiro que acessava a área de memória
 *  liberada.
 */
void erase_passwd(char **pass){
    memset(*pass, 0, strlen(*pass)+1);
    free(*pass);
    *pass = NULL;
}
