
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpNone assignment_from_void_function_result_x;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpFunction assignment_from_void_function_result_void_fn;
assignment_from_void_function_result_void_fn.addr = _np_0;
assignment_from_void_function_result_x = ((NpNone (*)(NpContext))assignment_from_void_function_result_void_fn.addr)(assignment_from_void_function_result_void_fn.ctx);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0