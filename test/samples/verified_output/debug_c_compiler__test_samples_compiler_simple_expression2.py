// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYFLOAT x;
PYFLOAT y;
PYFLOAT z;

// FUNCTION DECLARATIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
x = 1.0;
y = 2.0;
{
PYFLOAT NP_var0 = x+y;
PYFLOAT NP_var1 = NP_var0+3.0;
z = NP_var1;
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
}
exitcode=0