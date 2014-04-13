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

char *dest_name(const char *filename){
    int len = strlen(filename);
    char append[9] = "_out.txt";
    char *ch, *name = malloc((len+9) * (sizeof *name));

    strcpy(name, filename);
    if( (ch = strrchr(name, '.')) )
        *ch = 0;
    strcat(name, append);
    return name;
}

void enigma(const char *source, char *dest, const char *key, const int *mode){
    FILE *dstfile, *srcfile = fopen(source, "r");
    int *order = crack_the_code(key);

    if( ! dest )
        dest = dest_name(source);
    dstfile = fopen(dest, "w+");
    if( ! (srcfile || dstfile) ){
        fprintf(stderr, "Erro ao abrir arquivo, saindo!\n");
        exit(EXIT_FAILURE);
    }
    handle_file(srcfile, dstfile, order, mode[1]);
    fclose(srcfile);
    fclose(dstfile);
    if( ! mode[0] ){
        remove(source);
        rename(dest, source);
    }
}

int handle_file(FILE *file, FILE *saveas, const int *order, const int mode){
    char block[1024], *cipher = malloc(1024 * (sizeof*cipher));
    int max_bytes = 1024 * (sizeof *block);
    int sig, c;

    while( ! (sig = feof(file)) ){
        memset(block, 0, max_bytes);
        c = fread(block, sizeof *block, max_bytes, file);
        memset(cipher, 0, max_bytes);
        ( mode ) ? encrypt(block, order, cipher) : decrypt(block, order, cipher);
        fwrite(cipher, sizeof *cipher, (c < max_bytes) ? strlen(cipher) : max_bytes, saveas);
    }
    free(cipher);
    return sig;
}

/*  crack_the_code
 *  A chave criptografica é copiada em um novo vetor apass. apass é usado para
 *  arrumar os itens em ordem alfabetica usando o algoritmo de ordenacao shellsort.
 *  Depois de ordenado, apass e a chave original pass são comparadas para
 *  encontrar a posicao de cada letra de apass em pass.
 *  Essa ordem é importante para criptografar e descriptografar o texto, e fica
 *  armazenada no vetor de inteiros code, que é o retorno da função.
 */
int *crack_the_code(const char *pass){
    int i, j, k, plen = strlen(pass);
    int *code = malloc((strlen(pass)+1) * (sizeof *code));
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

/*  encrypt() é a função que embaralha todo o texto.
 *  A frase original é percorrida em intervalos e guardada em um vetor novo
 *  scrabble.
 *  O intervalo é igual ao tamanho da palavra-chave. Uma palavra chave de 6
 *  letras faz o texto ser percorrido 0,6,12,18... depois 1,7,13,19... e assim
 *  por diante.
 *  A posição das letras na palavra-chave também interfere no modo que o texto é
 *  percorrido, por exemplo, CRIPTO faz com que os indices e seus múltiplos
 *  sejam percorridos na ordem 0,6... 2,8... 5,11... 3,9... 1,7... 4,10...
 *  Essa ordem é obtida em crack_the_code();
 */
void encrypt(const char *block, const int *order, char *cipher){
    int tlen = strlen(block);
    int i, j, n = 0;

    for( i = 0; i < *(order-1); i++ ){
        for( j = 0; (order[i]+j) < tlen; j += *(order-1) ){
            cipher[n++] = block[order[i]+j];
        }
    }
}

/*  decrypt() faz o inverso de encrypt().
 *  A função percorre o texto seguindo os caracteres um após o outro e os guarda
 *  no vetor in_order seguindo o intervalo e ordem definidos pela palavra-chave.
 *  Usar uma palavra chave diferente da usada para encriptar o texto vai gerar
 *  um texto ainda mais embaralhado aqui.
 */
void decrypt(const char *cipher, const int *order, char *block){
    int tlen = strlen(cipher);
    int i, j, n = 0;

    for( i = 0; i < *(order-1); i++ ){
        for( j = 0; (order[i]+j) < tlen; j += *(order-1) ){
            block[order[i]+j] = cipher[n++];
        }
    }
}

/*  input() lê a entrada de dados via teclado.
 *  Antes de iniciar a leitura, o retorno do terminal e desabilitado e
 *  reabilitado somente após toda leitura ter sido realizada.
 *  Isso previne que alguem espiando por cima do ombro do usuário consiga ler a
 *  palavra-chave na tela do computador.
 */
int input(char *txt){
    char tmp, *head = txt;

    set_term(1);
    while( (tmp = getchar()) != '\n' )
        *txt++ = tmp;
    *txt = 0;
    set_term(0);
    putchar('\n');
    return check_pass(head);
}

/*  check_pass() percorre a palavra-chave a fim de encontrar algum caractere
 *  repetido. Se houver repetição a função retorna 0, so nao houver retorna 1.
 */
int check_pass(const char *pass){
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
