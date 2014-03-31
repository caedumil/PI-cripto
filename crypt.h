/*
 *
 */

#ifndef CRYPT_H_INCLUDED
#define CRYPT_H_INCLUDED

int set_term(int);

char *set_string(FILE *file);

char *read_file(char *filename);

void write_file(char *filename, char *text);

int *crack_the_code(const char *pass);

void encrypt(char *filename, char *pass);

void decrypt(char *filename, char*pass);

void input(char *txt);

#endif
