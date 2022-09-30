// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyFloat NP_var5;
PyFloat NP_var6;
PyFloat NP_var1;
PyFloat NP_var7;
PyFloat NP_var8;
PyFloat NP_var2;
PyFloat NP_var3;
PyFloat NP_var4;
PyFloat function_call3_z;

// FUNCTION DECLARATIONS COMPILER SECTION
PyFloat function_call3_my_func(PyFloat x, PyFloat y);

// FUNCTION DEFINITIONS COMPILER SECTION
PyFloat function_call3_my_func(PyFloat x, PyFloat y) {
PyFloat NP_var0;
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
NP_var4 = (PyFloat)3/NP_var2;
function_call3_z = NP_var1+NP_var4;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0