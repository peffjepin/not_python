// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var1;
PYINT function_call2_y;

// FUNCTION DECLARATIONS COMPILER SECTION
PYINT function_call2_my_func(PYINT x);

// FUNCTION DEFINITIONS COMPILER SECTION
PYINT function_call2_my_func(PYINT x) {
PYINT NP_var0;
NP_var0 = x;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 3;
function_call2_y = function_call2_my_func(NP_var1);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0