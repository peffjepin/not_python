// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt op_assignments_x;
NpInt NP_var0;
NpInt NP_var1;
NpInt NP_var2;
NpInt NP_var3;

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
op_assignments_x = (NpInt)((NpFloat)op_assignments_x/(NpFloat)NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0