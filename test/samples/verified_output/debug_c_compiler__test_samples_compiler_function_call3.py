// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYFLOAT z;

// FUNCTION DECLARATIONS COMPILER SECTION
PYFLOAT my_func(PYFLOAT x, PYFLOAT y);

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
{
PYFLOAT NP_var5 = 1;
PYFLOAT NP_var6 = 3;
PYFLOAT NP_var0 = my_func(NP_var5, NP_var6);
PYFLOAT NP_var7 = 2;
PYFLOAT NP_var8 = 3;
PYFLOAT NP_var1 = my_func(NP_var7, NP_var8);
PYFLOAT NP_var2 = 2*NP_var0;
PYFLOAT NP_var3 = 3/NP_var1;
PYFLOAT NP_var4 = NP_var0+NP_var3;
z = NP_var4;
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
}
exitcode=0