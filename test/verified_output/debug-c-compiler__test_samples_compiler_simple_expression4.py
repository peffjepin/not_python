// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var1;
NpInt NP_var0;
NpInt NP_var3;
NpFloat NP_var2;
NpFloat simple_expression4_a;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 2*3;
NP_var0 = 1+NP_var1;
NP_var3 = 3*2;
NP_var2 = (NpFloat)NP_var3/(NpFloat)4;
simple_expression4_a = NP_var0-NP_var2;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0