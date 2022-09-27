// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYFLOAT NP_var5;
PYFLOAT NP_var6;
PYFLOAT NP_var1;
PYFLOAT NP_var7;
PYFLOAT NP_var8;
PYFLOAT NP_var2;
PYFLOAT NP_var3;
PYFLOAT NP_var4;
PYFLOAT function_call3_z;

// FUNCTION DECLARATIONS COMPILER SECTION
PYFLOAT function_call3_my_func(PYFLOAT x, PYFLOAT y);

// FUNCTION DEFINITIONS COMPILER SECTION
PYFLOAT function_call3_my_func(PYFLOAT x, PYFLOAT y) {
PYFLOAT NP_var0;
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
NP_var4 = (PYFLOAT)3/NP_var2;
function_call3_z = NP_var1+NP_var4;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0