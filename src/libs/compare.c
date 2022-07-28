#include "types/compare.h"

int shallow_compare(void *a, void *b) {
    if ( a == b ) {
        return 0;
    }

    return a < b ? -1 : 1; 
}

bool compare_equiv(compare comp, void *a, void *b) {
    return comp(a, b);
}

int string_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } else {
        return strcmp((char*) a, (char*) b);
    }
}

int char_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 

    return *((char*) a) - *((char*) b);
}

int double_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 

    return *((double*) a) - *((double*) b);
}

int float_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 

    return *((float*) a) - *((float*) b);
}

int int_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 
    
    return *((int*) a) - *((int*) b);
}

int long_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 

    return *((long*) a) - *((long*) b);
}

int short_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 

    return *((short*) a) - *((short*) b);
}

int unsigned_char_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 

    unsigned char aa = *((unsigned char*) a);
    unsigned char bb = *((unsigned char*) b);

    if ( aa == bb ) {
        return 0;
    }

    return aa < bb ? -1 : 1;
}

int unsigned_int_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 

    unsigned int aa = *((unsigned int*) a);
    unsigned int bb = *((unsigned int*) b);

    if ( aa == bb ) {
        return 0;
    }

    return aa < bb ? -1 : 1;
}

int unsigned_long_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 

    unsigned long aa = *((unsigned long*) a);
    unsigned long bb = *((unsigned long*) b);

    if ( aa == bb ) {
        return 0;
    }

    return aa < bb ? -1 : 1;
}

int unsigned_short_compare(void *a, void *b) {
    if ( !a && !b ) {
        return 0;
    } else if ( !a || !b ) {
        return (!a) ? -1 : 1;
    } 
    
    unsigned short aa = *((unsigned short*) a);
    unsigned short bb = *((unsigned short*) b);

    if ( aa == bb ) {
        return 0;
    }

    return aa < bb ? -1 : 1;
}
