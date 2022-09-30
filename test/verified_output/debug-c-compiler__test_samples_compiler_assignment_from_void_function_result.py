// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
void* assignment_from_void_function_result_x;

// FUNCTION DECLARATIONS COMPILER SECTION
void* assignment_from_void_function_result_void_fn();

// FUNCTION DEFINITIONS COMPILER SECTION
void* assignment_from_void_function_result_void_fn() {
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
assignment_from_void_function_result_x = assignment_from_void_function_result_void_fn();
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0