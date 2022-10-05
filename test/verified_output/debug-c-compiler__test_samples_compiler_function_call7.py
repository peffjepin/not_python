// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction function_call7_my_function;
NpInt NP_var4;
NpInt NP_var5;
NpInt NP_var8;
NpInt NP_var7;
NpInt NP_var6;
NpInt function_call7_a;

// FUNCTION DECLARATIONS COMPILER SECTION
NpInt NP_var0(NpInt x, NpInt y, NpInt z, NpInt w);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt NP_var0(NpInt x, NpInt y, NpInt z, NpInt w) {
NpInt NP_var2;
NP_var2 = x+y;
NpInt NP_var3;
NP_var3 = NP_var2+z;
NpInt NP_var1;
NP_var1 = NP_var3+w;
return NP_var1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call7_my_function.addr = &NP_var0;
NP_var4 = 1;
NP_var5 = 2;
NP_var8 = 5*4;
NP_var7 = NP_var8+1;
NP_var6 = 3;
function_call7_a = ((NpInt (*)(NpInt, NpInt, NpInt, NpInt))function_call7_my_function.addr)(NP_var4, NP_var5, NP_var6, NP_var7);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0