// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction function_call1_my_func;
NpInt function_call1_x;

// FUNCTION DECLARATIONS COMPILER SECTION
NpInt NP_var0(NpContext ctx);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt NP_var0(NpContext ctx) {
NpInt NP_var1;
NP_var1 = 1;
return NP_var1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call1_my_func.addr = &NP_var0;
function_call1_x = ((NpInt (*)(NpContext ctx))function_call1_my_func.addr)(function_call1_my_func.ctx);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0