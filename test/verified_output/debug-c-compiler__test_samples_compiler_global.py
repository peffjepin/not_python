
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="incx", .length=4}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt global_x;
NpNone _np_0(NpContext __ctx__);
NpFunction global_incx;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
NpInt _np_1;
_np_1 = 1;
global_x = global_x + _np_1;
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
global_x = 1;
global_incx.__addr__ = _np_0;
global_incx.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0