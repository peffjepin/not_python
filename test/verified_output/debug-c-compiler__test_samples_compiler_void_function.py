// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction void_function_my_function;

// FUNCTION DECLARATIONS COMPILER SECTION
void* NP_var0();

// FUNCTION DEFINITIONS COMPILER SECTION
void* NP_var0() {
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
void_function_my_function.addr = &NP_var0;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0