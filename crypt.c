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
    FILE *srcfile;
    FILE *dstfile;
    int opt;
    char *tmp;
    char *output;
    char *key;
    char **filename;
    bool mode_set;
    bool is_enc;
    bool keep_file;
    bool repeat_key;

    key = NULL;
    output = NULL;
    mode_set = false;
    keep_file = false;
    repeat_key = false;

    opterr = 0;

    while( (opt = getopt(argc, argv, "edohk")) != -1 ){
        switch(opt){
            case 'e':
                if( mode_set ){
                    fprintf(stderr,\
                        "Both Decrypt and Encrypt modes set, aborting\n");
                    exit(EXIT_FAILURE);
                }

                mode_set = true;
                is_enc = true;
                break;
            case 'd':
                if( mode_set ){
                    fprintf(stderr,\
                        "Both Encrypt and Decrypt modes set, aborting\n");
                    exit(EXIT_FAILURE);
                }

                mode_set = true;
                is_enc = false;
                break;
            case 'o':
                keep_file = true;
                break;
            case 'k':
                repeat_key = true;
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
            default:
                exit(EXIT_SUCCESS);
        }
    }

    if( ! mode_set )
        exit(EXIT_FAILURE);

    filename = &argv[optind];

    while( *filename ){
        output = dest_name(*filename, is_enc);

        if( ! key )
            key = get_passwd("Enter the key: ", 100);

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
        } else {
            tmp = *filename;

            if( srcfile ){
                fclose(srcfile);
                tmp = output;
            }

            fprintf(stderr, "%s - %s\n", tmp, strerror(errno));
        }

        if( ! repeat_key )
            erase_passwd(&key);

        free(output);
        filename++;
    }
    exit(EXIT_SUCCESS);
}
