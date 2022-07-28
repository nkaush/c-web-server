#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libs/callbacks.h"
#include "internals/format.h"

#define NUM_VERBS 8

int main(int argc, char** argv) {
    char* arr[NUM_VERBS] = {
        "GET",
        "HEAD",
        "POST",
        "PUT",
        "DELETE",
        "CONNECT",
        "OPTIONS",
        "TRACE"
    };

    for (size_t i = 0; i < NUM_VERBS; ++i) {
        printf("%s\t--> %zu\n", arr[i], string_hash_function(arr[i]));
    }

    char str[80] = "This is - www.tutorialspoint.com - website";
    const char s[2] = "-";
    char *token;

    /* get the first token */
    printf("%p\n", str);
    token = strtok(str, s);

    /* walk through other tokens */
    while( token != NULL ) {
        printf( "[%s] --> %p\n", token, token);

        token = strtok(NULL, s);
    }

    fprintf(stderr, BOLDRED"test error\n");
    printf("hello from stdout\n");

    return(0);
}