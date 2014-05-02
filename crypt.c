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
#include "pi.h"

int main(int argc, char *argv[]){
    char *filename = NULL;
    char *output = NULL;
    int opt, is_enc, keep_file = 0;

    while( (opt = getopt(argc, argv, "+e:d:o::")) != -1 ){
        switch(opt){
            case 'e':
                if( filename )
                    break;
                is_enc = 1;
                filename = optarg;
                break;
            case 'd':
                if( filename )
                    break;
                is_enc = 0;
                filename = optarg;
                break;
            case 'o':
                keep_file = 1;
                output = ( optarg ) ? optarg : argv[optind];
                break;
            default:
                fprintf(stderr,\
                    "Usage: %s [-d | -e file.txt] [-o [new_file.txt]] \n",\
                    argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if( ! output )
        output = dest_name(filename);
    enigma(filename, output, get_input("Enter the key: ", 100, 1), is_enc);
    if( ! keep_file ){
        remove(filename);
        rename(output, filename);
    }
    fprintf(stdout, "<%s> %s as <%s>\n", filename,\
        ( is_enc ) ? "encrypted" : "decrypted",\
        ( keep_file ) ? output : filename);
    exit(EXIT_SUCCESS);
}
