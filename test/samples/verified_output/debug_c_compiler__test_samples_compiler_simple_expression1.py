// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT x;
PYINT y;
PYINT z;

// FUNCTION DECLARATIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
x = 1;
y = 2;
{
PYINT var0 = x+y;
PYINT var1 = var0+3;
z = var1;
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
}
exitcode=0