// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var3;
PYINT NP_var1;
PYINT NP_var4;
PYINT NP_var2;
PYINT function_call4_z;

// FUNCTION DECLARATIONS COMPILER SECTION
PYINT function_call4_my_function(PYINT x, PYINT y);

// FUNCTION DEFINITIONS COMPILER SECTION
PYINT function_call4_my_function(PYINT x, PYINT y) {
PYINT NP_var0;
NP_var0 = x+y;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var3 = 1+2;
NP_var1 = NP_var3+3;
NP_var4 = 4*5;
NP_var2 = NP_var4*6;
function_call4_z = function_call4_my_function(NP_var1, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0