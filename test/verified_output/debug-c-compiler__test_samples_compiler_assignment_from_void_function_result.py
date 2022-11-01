
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="void_fn", .length=7}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpNone assignment_from_void_function_result_x;
NpNone _np_0(NpContext __ctx__);
NpFunction assignment_from_void_function_result_void_fn;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
assignment_from_void_function_result_void_fn.__addr__ = _np_0;
assignment_from_void_function_result_void_fn.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
if (global_exception) {
return 1;
}
assignment_from_void_function_result_x = ((NpNone (*)(NpContext))assignment_from_void_function_result_void_fn.__addr__)(assignment_from_void_function_result_void_fn.__ctx__);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0