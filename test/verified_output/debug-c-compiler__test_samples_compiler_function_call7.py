// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var3;
PYINT NP_var4;
PYINT NP_var7;
PYINT NP_var6;
PYINT NP_var5;
PYINT function_call7_a;

// FUNCTION DECLARATIONS COMPILER SECTION
PYINT function_call7_my_function(PYINT x, PYINT y, PYINT z, PYINT w);

// FUNCTION DEFINITIONS COMPILER SECTION
PYINT function_call7_my_function(PYINT x, PYINT y, PYINT z, PYINT w) {
PYINT NP_var1;
NP_var1 = x+y;
PYINT NP_var2;
NP_var2 = NP_var1+z;
PYINT NP_var0;
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