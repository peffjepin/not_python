// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var3;
PyInt NP_var4;
PyInt NP_var7;
PyInt NP_var6;
PyInt NP_var5;
PyInt function_call7_a;

// FUNCTION DECLARATIONS COMPILER SECTION
PyInt function_call7_my_function(PyInt x, PyInt y, PyInt z, PyInt w);

// FUNCTION DEFINITIONS COMPILER SECTION
PyInt function_call7_my_function(PyInt x, PyInt y, PyInt z, PyInt w) {
PyInt NP_var1;
NP_var1 = x+y;
PyInt NP_var2;
NP_var2 = NP_var1+z;
PyInt NP_var0;
NP_var0 = NP_var2+w;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var3 = 1;
NP_var4 = 2;
NP_var7 = 5*4;
NP_var6 = NP_var7+1;
NP_var5 = 3;
function_call7_a = function_call7_my_function(NP_var3, NP_var4, NP_var5, NP_var6);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0