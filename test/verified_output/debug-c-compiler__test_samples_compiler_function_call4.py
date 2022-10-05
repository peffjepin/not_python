// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction function_call4_my_function;
NpInt NP_var4;
NpInt NP_var2;
NpInt NP_var5;
NpInt NP_var3;
NpInt function_call4_z;

// FUNCTION DECLARATIONS COMPILER SECTION
NpInt NP_var0(NpInt x, NpInt y);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt NP_var0(NpInt x, NpInt y) {
NpInt NP_var1;
NP_var1 = x+y;
return NP_var1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call4_my_function.addr = &NP_var0;
NP_var4 = 1+2;
NP_var2 = NP_var4+3;
NP_var5 = 4*5;
NP_var3 = NP_var5*6;
function_call4_z = ((NpInt (*)(NpInt, NpInt))function_call4_my_function.addr)(NP_var2, NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0