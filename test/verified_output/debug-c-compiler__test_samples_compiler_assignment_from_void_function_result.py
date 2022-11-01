
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

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
assignment_from_void_function_result_void_fn.addr = _np_0;
if (global_exception) {
return 1;
}
assignment_from_void_function_result_x = ((NpNone (*)(NpContext))assignment_from_void_function_result_void_fn.addr)(assignment_from_void_function_result_void_fn.ctx);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0