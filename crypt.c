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
#include <errno.h>
#include "pi.h"

int main(int argc, char *argv[]){
    FILE *srcfile, *dstfile;
    char **filename = NULL;
    char *tmp, *output, *key;
    int opt, exit_code, mode_set, is_enc, keep_file, repeat_key;

    output = key = NULL;
    mode_set = keep_file = repeat_key = 0;
    opterr = 0;
    while( (opt = getopt(argc, argv, "edohk")) != -1 ){
        switch(opt){
            case 'e':
                if( mode_set ){
                    fprintf(stderr,\
                        "Both Decrypt and Encrypt modes set, aborting\n");
                    exit(EXIT_FAILURE);
                }
                mode_set = 1;
                is_enc = 1;
                break;
            case 'd':
                if( mode_set ){
                    fprintf(stderr,\
                        "Both Encrypt and Decrypt modes set, aborting\n");
                    exit(EXIT_FAILURE);
                }
                mode_set = 1;
                is_enc = 0;
                break;
            case 'o':
                keep_file = 1;
                break;
            case 'k':
                repeat_key = 1;
                break;
            case 'h':
                if( mode_set )
                    break;
                fprintf(stdout,\
                    "Usage: %s [OPERATION] [FILE]\n"\
                    "  -e\tencrypt file\n"\
                    "  -d\tdecrypt file\n"\
                    "  -k\trepeat the key on all files\n"
                    "  -o\tdon\'t overwrite the input file\n"\
                    "  -h\tshow this help message and exit\n\n",\
                    argv[0]);
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr,\
                    "Usage: %s [-d | -e] [-o] FILES \n",\
                    argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    exit_code = EXIT_SUCCESS;
    filename = &argv[optind];
    while( *filename != NULL ){
        output = dest_name(*filename, is_enc);
        if( ! (key && check_pass(key)) )
            key = get_input("Enter the key: ", 100, 1);
        if( (srcfile = fopen(*filename, "rb")) &&\
            (dstfile = fopen(output, "wb+")) ){
            pre_crypt(srcfile, dstfile, crack_the_code(key), is_enc);
            fclose(dstfile);
            fclose(srcfile);
            if( ! keep_file ){
                remove(*filename);
                rename(output, *filename);
            }
            fprintf(stdout, "<%s> %s as <%s>\n", *filename,\
                ( is_enc ) ? "encrypted" : "decrypted",\
                ( keep_file ) ? output : *filename);
        }
        else {
            tmp = *filename;
            if( srcfile ){
                fclose(srcfile);
                tmp = output;
            }
            fprintf(stderr, "%s - %s\n", tmp, strerror(errno));
            exit_code = EXIT_FAILURE;
        }
        if( ! repeat_key )
            erase_pass(&key);
        free(output);
        filename++;
    }
    exit(exit_code);
}
