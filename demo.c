/*  criptografia de transposicao */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "crypt.h"

char *get_pass(char*);

int main(int argc, char *argv[]){
    char pass[100];
    int opt;

    while( (opt = getopt(argc, argv, "e:d:")) != -1 ){
        switch(opt){
            case 'e':
                encrypt(optarg, get_pass(pass));
                break;
            case 'd':
                decrypt(optarg, get_pass(pass));
                break;
            default:
                fprintf(stderr, "Usage: %s [-d | -e] [file ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}

char *get_pass(char *pass){
    printf("Enter the key: ");
    input(pass);
    return pass;
}
