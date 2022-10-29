
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt global_x;
NpFunction global_incx;
NpNone _np_0(NpContext __ctx__);

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
NpInt _np_1;
_np_1 = 1;
global_x = global_x + _np_1;
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
global_x = 1;
global_incx.addr = _np_0;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0