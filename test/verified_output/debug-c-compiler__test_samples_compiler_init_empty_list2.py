// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYLIST l;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
l = LIST_INIT(PYINT);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0