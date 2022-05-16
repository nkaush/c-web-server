fmt = """dictionary* {key_type}_to_{value_type}_dictionary_create(void) {{
    return dictionary_create(
        {key_type}_hash_function, {key_type}_compare,
        {key_type}_copy_constructor, {key_type}_destructor,
        {value_type}_copy_constructor, {value_type}_destructor
    );
}}
"""

types = ["shallow", "string", "char", "double", "float", "int", "long", "short", "unsigned_char", "unsigned_int", "unsigned_long", "unsigned_short"]

def generate(key_type, value_type):
    print(fmt.format(key_type=key_type, value_type=value_type))
    
for k in types:
    for v in types:
        generate(k, v)