/*
 *
 */

#include <unistd.h>
#include <string.h>

#ifdef __linux__
#include <termios.h>
int set_term(void){
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~ECHO;
    if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1 )
        return -1;
    return 1;
}
int reset_term(void){
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag ^= ECHO;
    if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1 )
        return -1;
    return 1;
}

#elif _WIN32
 /* GATINHO!! GATINHO E O NOME DISSO!! */
int set_term(void){
    return 1;
}
int reset_term(void){
    return 1;
}

 /* ps.: tem coisa errada acontecendo no windows, arquivo salvo tem mais coisa
  * que deveria...
  */

#endif

char *set_string(FILE *file){
    char *big_ass_string;

    fseek(file, 0L, SEEK_END);
    big_ass_string = malloc((ftell(file)) * (sizeof *big_ass_string));
    rewind(file);
    return big_ass_string;
}

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

/*  crack_the_code, aka the shellsort algorithm */
int *crack_the_code(const char *pass){
    int i, j, k, plen = strlen(pass);
    int *code = malloc((strlen(pass)) * (sizeof *code));
    int gap[8] = {1, 4, 10, 23, 57, 132, 301, 701};
    char *apass = malloc((strlen(pass)) * (sizeof *apass));
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

char *tail(char *text, int size, int add){
    char *new_text = malloc((size+add) * (sizeof *new_text));
    char ch = 'a';
    int i;

    strcpy(new_text, text);
    free(text);
    for( i = 0; i < add; i++ ){
        new_text[size+i] = ch++;
    }
    new_text[size+i] = 0;
    return new_text;
}

void encrypt(char *filename, char *pass){
    char *scrabble, *text = read_file(filename);
    int plen = strlen(pass);
    int tlen = strlen(text);
    int i, j, n, *order = crack_the_code(pass);

/*  em arquivos grandes isso aqui gera complicação
 *
 *    if( (tlen % plen) != 0 ){
 *        text = tail(text, tlen, plen-(tlen%plen)+1);
 *        tlen = strlen(text);
 *    }
 */
    scrabble = malloc(tlen * (sizeof *scrabble));
    n = 0;
    for( i = 0; i < plen; i++ ){
        for( j = 0; (order[i]+j) < tlen; j += plen ){
            scrabble[n++] = text[order[i]+j];
        }
    }
    write_file(filename, scrabble);
}

void decrypt(char *filename, char*pass){
    char *in_order, *text = read_file(filename);
    int plen = strlen(pass);
    int tlen = strlen(text);
    int i, j, n, *order = crack_the_code(pass);

    in_order = malloc(tlen * sizeof *in_order);
    n = 0;
    for( i = 0; i < plen; i++ ){
        for( j = 0; (order[i]+j) < tlen; j += plen ){
            in_order[order[i]+j] = text[n++];
        }
    }
/*  melhorar esse pedaço aqui, nao funciona 100% das vezes
 *  fica de fora por enquanto
 *
 *    for( n = n-1; n > 0; n-- ){
 *        if( in_order[n] == 'c' && in_order[n-1] == 'b' && in_order[n-2] == 'a' )
 *            in_order[n-2] = 0;
 *    }
 */
    write_file(filename, in_order);
}

void input(char *txt){
    char tmp;

    set_term();
    while( (tmp = getchar()) != '\n' )
        *txt++ = tmp;
    *txt = 0;
    reset_term();
}
