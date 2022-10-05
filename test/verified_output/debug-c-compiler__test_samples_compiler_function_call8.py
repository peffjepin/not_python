// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFloat NP_var2;
NpInt NP_var1;
NpFloat function_call8_z;

// FUNCTION DECLARATIONS COMPILER SECTION
NpFloat function_call8_my_function(NpInt x, NpFloat y);

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat function_call8_my_function(NpInt x, NpFloat y) {
NpFloat NP_var0;
NP_var0 = (NpFloat)x/y;
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