// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction explicit_return_none_fn;

// FUNCTION DECLARATIONS COMPILER SECTION
void* NP_var0(NpContext ctx);

// FUNCTION DEFINITIONS COMPILER SECTION
void* NP_var0(NpContext ctx) {
void* NP_var1;
NP_var1 = NULL;
return;
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
explicit_return_none_fn.addr = &NP_var0;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0