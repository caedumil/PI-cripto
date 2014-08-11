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
 *  O retorno é restaurado usando a mesma operação, so que com o ECHO padrão.
 */
#ifdef __linux__
#include <termios.h>
int set_term(bool no_echo){
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ( no_echo == true ) ? ~ECHO : ECHO;
    if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1 )
        return 0;
    return 1;
}
#elif _WIN32
#include <windows.h>
int set_term(bool no_echo){
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD sett = 0;
    GetConsoleMode(hStdin, &sett);
    sett &= ( no_echo == true ) ? ~ENABLE_ECHO_INPUT : ENABLE_ECHO_INPUT;
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
    int len = strlen(filename);
    char *ch, *name = calloc(len+9, sizeof *name);

    strcpy(name, filename);
    if( (ch = strstr(name, "_enc.txt")) ||\
        (ch = strstr(name, "_dec.txt")) ||\
        (ch = strrchr(name, '.')) ){
        *ch = 0;
    }
    strcat(name, ( is_enc == true ) ? "_enc.txt" : "_dec.txt");
    return name;
}

/*  pre_crypt() lê o arquivo de entrada em blocos de BLOCK_SIZE bytes por vez,
 *  até o fim do arquivo. Cada bloco é processado e gravado no arquivo de saída
 *  antes de um novo bloco ser lido. O último byte dos blocos é '\0' para
 *  garantir a marcação o final da string.
 *  Ler o arquivo em blocos fixos permite controlar a quantidade de memória
 *  usada pelo programa.
 */
int pre_crypt(FILE *file, FILE *saveas, int *order, bool mode){
    char *before = calloc(BLOCK_SIZE, sizeof *before);
    char *after;
    int sig, size;

    while( (sig = feof(file)) == 0 ){
        memset(before, 0, BLOCK_SIZE);
        size = fread(before, sizeof *before, BLOCK_SIZE-1, file);
        after = crypt(before, size, order, mode);
        fwrite(after, sizeof *after, size, saveas);
        free(after);
    }
    free(before);
    memset(order-1, 0, *(order-1));
    free(order-1);
    return sig;
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
    int i, j, k, plen = strlen(pass);
    int *code = calloc(plen+1, sizeof *code);
    int gap[8] = {1, 4, 10, 23, 57, 132, 301, 701};
    char *ord_pass = calloc(plen+1, sizeof *ord_pass);
    char temp;

    strcpy(ord_pass, pass);
    for( i = 7; i >= 0; i--){
        for( j = gap[i]; j < plen; j++ ){
            temp = ord_pass[j];
            for( k = j; (k >= gap[i]) && (ord_pass[k-gap[i]] > temp); k -= gap[i] ){
                ord_pass[k] = ord_pass[k - gap[i]];
            }
            ord_pass[k] = temp;
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

/*  crypt() é a função que modifica todo o texto.
 *  O bloco 'before' é percorrido em intervalos e guardada em um novo bloco
 *  'after'. O intervalo é igual ao tamanho da palavra-chave. Uma palavra chave
 *  de 6 letras faz o texto ser percorrido 0,6,12,18... depois 1,7,13,19... e
 *  assim por diante.
 *  A posição das letras na palavra-chave também interfere no modo que o texto é
 *  percorrido, por exemplo, CRIPTO faz com que os indices e seus múltiplos
 *  sejam percorridos na ordem 0,6... 2,8... 5,11... 3,9... 1,7... 4,10...
 *  Essa ordem é obtida em crack_the_code().
 *  is_enc determina a maneira que os blocos são percorridos, um valor diferente
 *  de zero causa a criptografia do texto, e um valor igual a zero causa a
 *  descriptogrfia.
 */
char *crypt(const char *before, const int size, const int *order, bool is_enc){
    char *after = calloc(size+1, sizeof *after);
    int aa, bb, i, j, n = 0;

    for( i = 0; i < *(order-1); i++ ){
        for( j = 0; (order[i]+j) < size; j += *(order-1) ){
            aa = ( is_enc == true ) ? n++ : order[i]+j;
            bb = ( is_enc == true ) ? order[i]+j : n++;
            after[aa] = before[bb];
        }
    }
    return after;
}

/*  get_input() cria um vetor para guardar a entrada de dados via teclado,
 *  usando um tamanho especificado na chamada da função.
 */
char *get_input(const char *text, const int size, bool is_pass){
    char *in = calloc(size, sizeof *in);

    fprintf(stdout, "%s", text);
    if( input(in, is_pass) == false ){
        free(in);
        return get_input(text, size, is_pass);
    }
    return in;
}

/*  input() lê a entrada de dados via teclado.
 *  Se 'is_pass' for diferente de zero, antes de iniciar a leitura, o retorno
 *  do terminal e desabilitado e reabilitado somente após toda leitura ter sido
 *  realizada. Isso previne que alguem espiando por cima do ombro do usuário
 *  consiga ler a palavra-chave na tela do computador.
 *  'is_pass' também determina se o vetor lido será verificado por caracteres
 *  repetidos.
 */
bool input(char *txt, bool disable_echo){
    char tmp, *head = txt;

    set_term(disable_echo);
    while( (tmp = getchar()) != '\n' )
        *txt++ = tmp;
    *txt = 0;
    if( disable_echo == true ){
        set_term(false);
        putchar('\n');
        return check_pass(head);
    }
    return true;
}

/*  check_pass() percorre a palavra-chave a fim de encontrar algum caractere
 *  repetido. Se houver repetição a função retorna 0, so nao houver retorna 1.
 */
bool check_pass(const char *pass){
    int c;

    while( *pass != 0 ){
        c = 1;
        while( c < strlen(pass) ){
            if( *pass == pass[c++] )
                return false;
        }
        pass++;
    }
    return true;
}

/*  erase_pass() apaga da memoria a chave criptografica escrevendo '0's antes
 *  de liberar a memoria alocada.
 *  A funçao finaliza setando NULL no ponteiro que acessava a area de memoria
 *  liberada.
 */
void erase_pass(char **pass){
    memset(*pass, 0, strlen(*pass)+1);
    free(*pass);
    *pass = NULL;
}
