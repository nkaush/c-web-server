#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    char* str = strdup(argv[1]);
    
    char* delim = "&";

    char* token = NULL;
    while ((token = strsep(&str, delim))) {        
        char* key = strsep(&token, "=");
        printf("k=[%s], v=[%s]\n", key, token);
    }
}