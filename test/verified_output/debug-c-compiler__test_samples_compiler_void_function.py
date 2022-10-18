
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFunction void_function_my_function;

// FUNCTION DEFINITIONS COMPILER SECTION
void* _np_0(NpContext __ctx__) {
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
void_function_my_function.addr = &_np_0;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0