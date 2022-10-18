
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt op_assignments_x;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
op_assignments_x = 10;
NpInt _np_0;
_np_0 = 1;
op_assignments_x = op_assignments_x-_np_0;
NpInt _np_1;
_np_1 = 1;
op_assignments_x = op_assignments_x+_np_1;
NpInt _np_2;
_np_2 = 2;
op_assignments_x = op_assignments_x*_np_2;
NpInt _np_3;
_np_3 = 2;
op_assignments_x = (NpInt)((NpFloat)op_assignments_x/(NpFloat)_np_3);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0