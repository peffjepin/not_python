// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var3;
NpInt NP_var1;
NpInt NP_var4;
NpInt NP_var2;
NpInt function_call4_z;

// FUNCTION DECLARATIONS COMPILER SECTION
NpInt function_call4_my_function(NpInt x, NpInt y);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt function_call4_my_function(NpInt x, NpInt y) {
NpInt NP_var0;
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