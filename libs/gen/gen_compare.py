headers = """int char_compare(void *a, void *b);
int double_compare(void *a, void *b);
int float_compare(void *a, void *b);
int int_compare(void *a, void *b);
int long_compare(void *a, void *b);
int short_compare(void *a, void *b);
int unsigned_char_compare(void *a, void *b);
int unsigned_int_compare(void *a, void *b);
int unsigned_long_compare(void *a, void *b);
int unsigned_short_compare(void *a, void *b);"""

sfmt = """{}
    return *(({}*) a) - *(({}*) b);
{}
"""

ufmt = """{}
    {} aa = *(({}*) a);
    {} bb = *(({}*) b);

    if ( aa == bb ) {}
        return 0;
    {}

    return aa < bb ? -1 : 1;
{}
"""

for header in headers.split('\n'):
    dtype = header[4:].split('compare')[0][:-1].replace('_', ' ')
    # print(dtype)

    signature = header.replace(';', ' {')
    if 'unsigned' not in dtype:
        print(sfmt.format(signature, dtype, dtype, '}'))
    else:
        print(ufmt.format(signature, dtype, dtype, dtype, dtype, '{', '}', '}'))