// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFloat NP_var5;
NpFloat NP_var6;
NpFloat NP_var1;
NpFloat NP_var7;
NpFloat NP_var8;
NpFloat NP_var2;
NpFloat NP_var3;
NpFloat NP_var4;
NpFloat function_call3_z;

// FUNCTION DECLARATIONS COMPILER SECTION
NpFloat function_call3_my_func(NpFloat x, NpFloat y);

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat function_call3_my_func(NpFloat x, NpFloat y) {
NpFloat NP_var0;
NP_var0 = x+y;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var5 = 1.0;
NP_var6 = 3.0;
NP_var1 = function_call3_my_func(NP_var5, NP_var6);
NP_var7 = 2.0;
NP_var8 = 3.0;
NP_var2 = function_call3_my_func(NP_var7, NP_var8);
NP_var3 = 2*NP_var1;
NP_var4 = (NpFloat)3/NP_var2;
function_call3_z = NP_var1+NP_var4;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0