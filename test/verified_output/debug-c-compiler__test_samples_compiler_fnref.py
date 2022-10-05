// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="hello", .length=5}};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
void* (*fnref_fnref)(NpString);
NpString NP_var1;

// FUNCTION DECLARATIONS COMPILER SECTION
void* fnref_my_function(NpString value);

// FUNCTION DEFINITIONS COMPILER SECTION
void* fnref_my_function(NpString value) {
NpString NP_var0;
NP_var0 = value;
builtin_print(1, NP_var0);
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
fnref_fnref = fnref_my_function;
NP_var1 = NOT_PYTHON_STRING_CONSTANTS[0];
fnref_fnref(NP_var1);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0