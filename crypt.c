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
#include "crypt.h"

/*  set_term() desabilita o retorno de caracteres no terminal. Basicamente a
 *  funcao cria pega os atributos atuais do terminal rodando o programa q salva
 *  numa struct.
 *  Para desabilitar o retorno é feita uma operacao com bits para negar os
 *  atributos relacionados somente com o ECHO, negando-o logicamente com o
 *  operador '~'.
 *  O retorno é restaurado usando a mesma operacao, so que com o ECHO padrao.
 */
#ifdef __linux__
#include <termios.h>
int set_term(int mode){
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    if( mode )
        term.c_lflag &= ~ECHO;
    else
        term.c_lflag &= ECHO;
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
    if( mode )
        sett &= ~ENABLE_ECHO_INPUT;
    else
        sett &= ENABLE_ECHO_INPUT;
    if( SetConsoleMode(hStdin, sett) == 0 )
        return 0;
    return 1;
}
#endif

/*  set_string() aloca dinamicamente um vetor de tamanho suficiente para
 *  acomodar todo o texto a ser criptografado.
 *  O tamanho do texto é determinado colocando o ponteiro do tipo FILE no final
 *  do arquivo aberto (usando fseek) e contando a diferenca entre o inicio e o
 *  fim do arquivo (usando ftell).
 *  o vetor alocado é o retorno da funcao.
 */
char *set_string(FILE *file){
    char *big_ass_string;

    fseek(file, 0L, SEEK_END);
    big_ass_string = malloc((ftell(file)) * (sizeof *big_ass_string));
    rewind(file);
    return big_ass_string;
}

/*  read_file usa o argumnto passado para o programa e tenta abrir o arquivo
 *  especificado. Caso consiga, usa set_string e em seguida carrega o vetor com
 *  o conteudo do arquivo, cararcter por caracter, ate que a funcao fgetc
 *  informe que chegou ao fim do arquivo ( retornando EOF - End Of File).
 *  O vetor com tudo presente no arquivo e o retorno da funcao.
 */
char *read_file(char *filename){
    FILE *file = fopen(filename, "r");
    char *big_ass_string;
    int ch, n = 0;

    if( ! file ){
        fprintf(stderr, "Erro ao abrir arquivo para leitura, saindo\n");
        exit(EXIT_FAILURE);
    }
    big_ass_string = set_string(file);
    while( (ch = fgetc(file)) != EOF ){
        big_ass_string[n++] = ch;
    }
    if( big_ass_string[n-1] == '\n' )
        n--;
    big_ass_string[n] = 0;
    fclose(file);
    return big_ass_string;
}

/*  write_file() escreve a string gerada pelo programa em um arquivo.
 *  O vetor é percorrido posicao a posicao e o caracter é escrito no arquivo
 *  (usando fputc).
 *  Quando o fim da string e encontrado ( '\0' ) a funcao encerra a escrita e
 *  fecha o arquivo.
 */
void write_file(char *filename, char *text){
    FILE *file = fopen(filename, "w");
    int ch, n = 0;

    if( ! file ){
        fprintf(stderr, "Erro ao abrir arquivo para escrita, saindo\n");
        exit(EXIT_FAILURE);
    }
    while( (ch = text[n++]) != 0 ){
        fputc(ch, file);
    }
    fputc('\n', file);
    fclose(file);
}

/*  crack_the_code
 *  A chave criptografica e copiada em um novo vetor apass. apass e usado para
 *  ordenar os itens em ordem alfabetica usando o algoritmo de ordenacao shellsort.
 *  Depois de ordenado, apass e a chave original pass sao comparadas para
 *  encontrar a posicao de cada letra de apass em pass.
 *  Essa ordem e importante para criptografar e descriptografar o texto e fica
 *  armazenada no vetor de inteiros code, que e o retorno da funcao.
 */
int *crack_the_code(const char *pass){
    int i, j, k, plen = strlen(pass);
    int *code = malloc((strlen(pass)) * (sizeof *code));
    int gap[8] = {1, 4, 10, 23, 57, 132, 301, 701};
    char *apass = malloc((strlen(pass)+1) * (sizeof *apass));
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
    for( i = 0; i < plen; i++ ){
        for( j = 0; j < plen; j++ ){
            if( apass[i] == pass[j] )
                code[i] = j;
        }
    }
    free(apass);
    return code;
}

/*  encrypt() e a funcao que embaralha todo o texto.
 *  A frase original é percorrida em intervalos e guardada em um vetor novo
 *  scrabble.
 *  O intervalo é igual ao tamanho da palavra-chave. Uma palavra chave de 6
 *  letras faz o texto ser percorrido 0,6,12,18... depois 1,7,13,19... e assim
 *  por diante.
 *  A posicao das letras na palavra-chave tambem interfere no modo que o texto e
 *  percorrido, por exemplo, CRIPTO faz com que os indices e seus multiplos
 *  sejam percorridos na ordem 0,6... 2,8... 5,11... 3,9... 1,7... 4,10...
 *  Essa ordem e obtida em crack_the_code();
 */
void encrypt(char *filename, char *pass){
    char *scrabble, *text = read_file(filename);
    int plen = strlen(pass);
    int tlen = strlen(text);
    int i, j, n, *order = crack_the_code(pass);

    scrabble = malloc((tlen+1) * (sizeof *scrabble));
    n = 0;
    for( i = 0; i < plen; i++ ){
        for( j = 0; (order[i]+j) < tlen; j += plen ){
            scrabble[n++] = text[order[i]+j];
        }
    }
    scrabble[n] = 0;
    write_file(filename, scrabble);
}

/*  decrypt() faz o inverso de encrypt().
 *  A funcao percorre o texto seguindo os caracteres um apos o outro e os guarda
 *  no vetor in_order seguindo o intervalo e ordem definidos pela palavra-chave.
 *  Usar uma palavra chave diferente da usada para encriptar o texto vai gerar
 *  um texto ainda mais embaralhado aqui.
 */
void decrypt(char *filename, char*pass){
    char *in_order, *text = read_file(filename);
    int plen = strlen(pass);
    int tlen = strlen(text);
    int i, j, n, *order = crack_the_code(pass);

    in_order = malloc((tlen+1) * (sizeof *in_order));
    n = 0;
    for( i = 0; i < plen; i++ ){
        for( j = 0; (order[i]+j) < tlen; j += plen ){
            in_order[order[i]+j] = text[n++];
        }
    }
    in_order[n] = 0;
    write_file(filename, in_order);
}

/*  input() le a entrada de dados via teclado.
 *  Antes de iniciar a leitura o retorno do terminal e desabilitado e
 *  reabilitado somente apos toda leitura ter sido realizada.
 *  Isso previne que alguem espiando por cima do ombro do usuario consiga ler a
 *  palavra-chave na tela do computador.
 */
int input(char *txt){
    char tmp, *head = txt;

    set_term(1);
    while( (tmp = getchar()) != '\n' )
        *txt++ = tmp;
    *txt = 0;
    set_term(0);
    return check_pass(head);
}

/*  check_pass() percorre a palavra-chave a fim de encontrar algum caracter
 *  repetido. Se houver repeticao a funcao retorna 0, so nao houver retorna 1.
 */
int check_pass(char *pass){
    int c = 1;

    while( *pass != 0 ){
        for( ; c < strlen(pass); c++ ){
            if( *pass == pass[c] )
                return 0;
        }
        pass++;
        c++;
    }
    return 1;
}
