// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction assignment_from_void_function_result_void_fn;
void* assignment_from_void_function_result_x;

// FUNCTION DECLARATIONS COMPILER SECTION
void* NP_var0();

// FUNCTION DEFINITIONS COMPILER SECTION
void* NP_var0() {
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
assignment_from_void_function_result_void_fn.addr = &NP_var0;
assignment_from_void_function_result_x = ((void* (*)())assignment_from_void_function_result_void_fn.addr)();
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0