// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt global_x;
NpFunction global_incx;

// FUNCTION DECLARATIONS COMPILER SECTION
void* NP_var0(NpContext ctx);

// FUNCTION DEFINITIONS COMPILER SECTION
void* NP_var0(NpContext ctx) {
NpInt NP_var1;
NP_var1 = 1;
global_x = global_x+NP_var1;
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
global_x = 1;
global_incx.addr = &NP_var0;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0