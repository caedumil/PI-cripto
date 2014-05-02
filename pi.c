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
int set_term(int mode){
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ( mode ) ? ~ECHO : ECHO;
    if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1 )
        return 0;
    return 1;
}
#elif _WIN32
#include <windows.h>
int set_term(int mode){
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD sett = 0;
    GetConsoleMode(hStdin, &sett);
    sett &= ( mode ) ? ~ENABLE_ECHO_INPUT : ENABLE_ECHO_INPUT;
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
char *dest_name(const char *filename){
    int len = strlen(filename);
    char append[9] = "_out.txt";
    char *ch, *name = calloc(len+9, sizeof *name);

    strcpy(name, filename);
    if( (ch = strstr(filename, append)) || (ch = strrchr(name, '.')) )
        *ch = 0;
    strcat(name, append);
    return name;
}

/*  enigma() abre os arquivos de entrada e saída, chama pre_crypt() que vai
 *  cuidar do processamento do texto, e substitui, ou não, o arquivo de entrada
 *  pelo de saída.
 */
void enigma(const char *source, char *dest, const char *key, const int mode){
    FILE *srcfile = fopen(source, "r");
    FILE *dstfile = fopen(dest, "w+");
    int *order = crack_the_code(key);

    if( ! (srcfile && dstfile) ){
        fprintf(stderr, "Erro ao abrir arquivo, saindo!\n");
        exit(EXIT_FAILURE);
    }
    pre_crypt(srcfile, dstfile, order, mode);
    fclose(srcfile);
    fclose(dstfile);
}

/*  pre_crypt() lê o arquivo de entrada em blocos de BLOCK_SIZE bytes por vez,
 *  até o fim do arquivo. Cada bloco é processado e gravado no arquivo de saída
 *  antes de um novo bloco ser lido. O último byte dos blocos é '\0' para
 *  garantir a marcação o final da string.
 *  Ler o arquivo em blocos fixos permite controlar a quantidade de memória
 *  usada pelo programa.
 */
int pre_crypt(FILE *file, FILE *saveas, const int *order, const int mode){
    char *before = calloc(BLOCK_SIZE, sizeof *before);
    char *after = calloc(BLOCK_SIZE, sizeof *after);
    int sig;

    while( ! (sig = feof(file)) ){
        memset(before, 0, BLOCK_SIZE);
        fread(before, sizeof *before, BLOCK_SIZE-1, file);
        memset(after, 0, BLOCK_SIZE);
        crypt(before, after, order, mode);
        fwrite(after, sizeof *after, strlen(after), saveas);
    }
    free(before);
    free(after);
    return sig;
}

/*  crack_the_code()
 *  A chave criptografica é copiada em um novo vetor 'apass'. 'apass' é usado
 *  para arrumar os itens em ordem alfabetica usando o algoritmo de ordenacao
 *  shellsort.
 *  Depois de ordenado, 'apass' e a chave original 'pass' são comparadas para
 *  encontrar a posicao de cada letra de 'apass' em 'pass'.
 *  Essa ordem é importante para criptografar e descriptografar o texto, e fica
 *  armazenada no vetor de inteiros 'code', que é o retorno da função. A
 *  primeira posição de 'code' guarda seu tamanho, e o ponteiro retornado
 *  aponta para a segunda posição, escondendo a informação do tamanho do vetor.
 */
int *crack_the_code(const char *pass){
    int i, j, k, plen = strlen(pass);
    int *code = calloc(strlen(pass)+1, sizeof *code);
    int gap[8] = {1, 4, 10, 23, 57, 132, 301, 701};
    char *apass = calloc(strlen(pass)+1, sizeof *apass);
    char temp;

    strcpy(apass, pass);
    for( i = 7; i >= 0; i--){
        for( j = gap[i]; j < plen; j++ ){
            temp = apass[j];
            for( k = j; (k >= gap[i]) && (apass[k - gap[i]] > temp); k -= gap[i] ){
                apass[k] = apass[k - gap[i]];
            }
            apass[k] = temp;
        }
    }
    code[0] = plen;
    for( i = 0; i <= plen; i++ ){
        for( j = 0; j < plen; j++ ){
            if( apass[i] == pass[j] )
                code[i+1] = j;
        }
    }
    free(apass);
    return ++code;
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
void crypt(const char *before, char *after, const int *order, const int is_enc){
    int tlen = strlen(before);
    int line, leap, i, j, n = 0;

    for( i = 0; i < *(order-1); i++ ){
        for( j = 0; (order[i]+j) < tlen; j += *(order-1) ){
            line = n++;
            leap = order[i]+j;
            after[( is_enc ) ? line : leap] = before[( is_enc ) ? leap : line];
        }
    }
}

/*  get_input() cria um vetor para guardar a entrada de dados via teclado,
 *  usando um tamanho especificado na chamada da função.
 */
char *get_input(const char *text, const int size, const int is_pass){
    char *in = calloc(size, sizeof *in);

    fprintf(stdout, "%s", text);
    if( ! input(in, is_pass) ){
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
int input(char *txt, const int is_pass){
    char tmp, *head = txt;

    set_term(is_pass);
    while( (tmp = getchar()) != '\n' )
        *txt++ = tmp;
    *txt = 0;
    if( is_pass ){
        set_term(0);
        putchar('\n');
        return check_pass(head);
    }
    return 1;
}

/*  check_pass() percorre a palavra-chave a fim de encontrar algum caractere
 *  repetido. Se houver repetição a função retorna 0, so nao houver retorna 1.
 */
int check_pass(const char *pass){
    int c;

    while( *pass != 0 ){
        c = 1;
        while( c < strlen(pass) ){
            if( *pass == pass[c++] )
                return 0;
        }
        pass++;
    }
    return 1;
}
