// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var3;
NpInt NP_var4;
NpInt NP_var7;
NpInt NP_var6;
NpInt NP_var5;
NpInt function_call7_a;

// FUNCTION DECLARATIONS COMPILER SECTION
NpInt function_call7_my_function(NpInt x, NpInt y, NpInt z, NpInt w);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt function_call7_my_function(NpInt x, NpInt y, NpInt z, NpInt w) {
NpInt NP_var1;
NP_var1 = x+y;
NpInt NP_var2;
NP_var2 = NP_var1+z;
NpInt NP_var0;
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