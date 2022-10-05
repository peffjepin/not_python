// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction function_call2_my_func;
NpInt NP_var2;
NpInt function_call2_y;

// FUNCTION DECLARATIONS COMPILER SECTION
NpInt NP_var0(NpInt x);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt NP_var0(NpInt x) {
NpInt NP_var1;
NP_var1 = x;
return NP_var1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call2_my_func.addr = &NP_var0;
NP_var2 = 3;
function_call2_y = ((NpInt (*)(NpInt))function_call2_my_func.addr)(NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0