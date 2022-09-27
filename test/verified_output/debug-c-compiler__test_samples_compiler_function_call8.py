// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYFLOAT NP_var2;
PYINT NP_var1;
PYFLOAT function_call8_z;

// FUNCTION DECLARATIONS COMPILER SECTION
PYFLOAT function_call8_my_function(PYINT x, PYFLOAT y);

// FUNCTION DEFINITIONS COMPILER SECTION
PYFLOAT function_call8_my_function(PYINT x, PYFLOAT y) {
PYFLOAT NP_var0;
NP_var0 = (PYFLOAT)x/y;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var2 = 2.0;
NP_var1 = 1;
function_call8_z = function_call8_my_function(NP_var1, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0