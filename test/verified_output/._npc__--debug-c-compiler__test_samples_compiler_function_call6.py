// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT z;

// FUNCTION DECLARATIONS COMPILER SECTION
PYINT my_function(PYINT x, PYFLOAT y);

// FUNCTION DEFINITIONS COMPILER SECTION
PYINT my_function(PYINT x, PYFLOAT y) {
PYFLOAT NP_var0 = x/y;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var1 = 1;
PYFLOAT NP_var2 = 2.0;
z = my_function(NP_var1, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0