
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="fn", .length=2}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__);
NpFunction explicit_return_none_fn;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
NpNone _np_1;
_np_1 = 0;
return _np_1;
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
explicit_return_none_fn.__addr__ = _np_0;
explicit_return_none_fn.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0