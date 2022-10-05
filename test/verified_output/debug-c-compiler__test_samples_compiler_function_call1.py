// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction function_call1_my_func;
NpInt function_call1_x;

// FUNCTION DECLARATIONS COMPILER SECTION
NpInt NP_var0();

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt NP_var0() {
NpInt NP_var1;
NP_var1 = 1;
return NP_var1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call1_my_func.addr = &NP_var0;
function_call1_x = ((NpInt (*)())function_call1_my_func.addr)();
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0