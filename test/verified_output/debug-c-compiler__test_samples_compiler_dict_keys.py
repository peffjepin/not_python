// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYDICT d;
PYINT NP_var5;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
PYINT NP_var1 = 2;
d = DICT_INIT(PYINT, PYINT, void_int_eq);
dict_set_item(d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = 4;
dict_set_item(d, &NP_var0, &NP_var1);
NP_var0 = 5;
NP_var1 = 6;
dict_set_item(d, &NP_var0, &NP_var1);
PYDICT NP_var4 = d;
PYITER NP_var2 = dict_iter_keys(NP_var4);
void* NP_var6;
while ((NP_var6 = NP_var2.next(NP_var2.iter),
NP_var5 = (NP_var6) ? *(PYINT*)(NP_var6) : NP_var5,
NP_var6)){
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0