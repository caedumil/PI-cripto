/*  criptografia de transposicao */

#include <stdio.h>
#include <stdlib.h>
#include "crypt.h"

int main(int argc, char *argv[]){
    char pass[100];
    int opt;

    if( argc > 2 ){
        printf("Enter the key: ");
        input(pass);
    }
    while( (opt = getopt(argc, argv, "e:d:")) != -1 ){
        switch(opt){
            case 'e':
                encrypt(optarg, pass);
                break;
            case 'd':
                decrypt(optarg, pass);
                break;
            default:
                fprintf(stderr, "Usage: %s [-d | -e] [file ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}
