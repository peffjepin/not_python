// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction function_call6_my_function;
NpInt NP_var2;
NpFloat NP_var3;
NpFloat function_call6_z;

// FUNCTION DECLARATIONS COMPILER SECTION
NpFloat NP_var0(NpInt x, NpFloat y);

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat NP_var0(NpInt x, NpFloat y) {
NpFloat NP_var1;
NP_var1 = (NpFloat)x/y;
return NP_var1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call6_my_function.addr = &NP_var0;
NP_var2 = 1;
NP_var3 = 2.0;
function_call6_z = ((NpFloat (*)(NpInt, NpFloat))function_call6_my_function.addr)(NP_var2, NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0