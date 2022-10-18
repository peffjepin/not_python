
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
void* assignment_from_void_function_result_x;
NpFunction assignment_from_void_function_result_void_fn;

// FUNCTION DEFINITIONS COMPILER SECTION
void* _np_0(NpContext __ctx__) {
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
assignment_from_void_function_result_void_fn.addr = &_np_0;
assignment_from_void_function_result_x = ((void* (*)(NpContext ctx))assignment_from_void_function_result_void_fn.addr)(assignment_from_void_function_result_void_fn.ctx);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0