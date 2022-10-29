
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFunction explicit_return_none_fn;
NpNone _np_0(NpContext __ctx__);

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
NpNone _np_1;
_np_1 = 0;
return _np_1;
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
explicit_return_none_fn.addr = _np_0;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0