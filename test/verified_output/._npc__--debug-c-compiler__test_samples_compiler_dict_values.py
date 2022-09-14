// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYDICT d;

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
PYDICT NP_var4 = d;
PYITER NP_var2 = dict_iter_vals(NP_var4);
void* NP_var5;
PYINT v;
while ((NP_var5 = NP_var2.next(NP_var2.iter),
v = (NP_var5) ? *(PYINT*)(NP_var5) : v,
NP_var5)){
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0