// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="hello", .length=5}};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction fnref_my_function;
NpFunction fnref_fnref;
NpString NP_var2;

// FUNCTION DECLARATIONS COMPILER SECTION
void* NP_var0(NpContext ctx, NpString value);

// FUNCTION DEFINITIONS COMPILER SECTION
void* NP_var0(NpContext ctx, NpString value) {
NpString NP_var1;
NP_var1 = value;
builtin_print(1, NP_var1);
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
fnref_my_function.addr = &NP_var0;
fnref_fnref = fnref_my_function;
NP_var2 = NOT_PYTHON_STRING_CONSTANTS[0];
((void* (*)(NpContext ctx, NpString))fnref_fnref.addr)(fnref_fnref.ctx, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0