// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var1;
PYINT NP_var2;
PYINT function_call5_z;

// FUNCTION DECLARATIONS COMPILER SECTION
PYINT function_call5_my_function(PYINT x, PYINT y);

// FUNCTION DEFINITIONS COMPILER SECTION
PYINT function_call5_my_function(PYINT x, PYINT y) {
PYINT NP_var0;
NP_var0 = x+y;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 2;
NP_var2 = 3;
function_call5_z = function_call5_my_function(NP_var1, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0