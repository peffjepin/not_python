// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var1;
PYFLOAT NP_var2;
PYFLOAT function_call6_z;

// FUNCTION DECLARATIONS COMPILER SECTION
PYFLOAT function_call6_my_function(PYINT x, PYFLOAT y);

// FUNCTION DEFINITIONS COMPILER SECTION
PYFLOAT function_call6_my_function(PYINT x, PYFLOAT y) {
PYFLOAT NP_var0;
NP_var0 = (PYFLOAT)x/y;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 1;
NP_var2 = 2.0;
function_call6_z = function_call6_my_function(NP_var1, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0