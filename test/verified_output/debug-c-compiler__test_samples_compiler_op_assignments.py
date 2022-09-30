// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt op_assignments_x;
PyInt NP_var0;
PyInt NP_var1;
PyInt NP_var2;
PyInt NP_var3;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
op_assignments_x = 10;
NP_var0 = 1;
op_assignments_x = op_assignments_x-NP_var0;
NP_var1 = 1;
op_assignments_x = op_assignments_x+NP_var1;
NP_var2 = 2;
op_assignments_x = op_assignments_x*NP_var2;
NP_var3 = 2;
op_assignments_x = (PyInt)((PyFloat)op_assignments_x/(PyFloat)NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0