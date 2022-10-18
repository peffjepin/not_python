
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFunction explicit_return_none_fn;

// FUNCTION DEFINITIONS COMPILER SECTION
void* _np_0(NpContext __ctx__) {
void* _np_1;
_np_1 = NULL;
return;
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
explicit_return_none_fn.addr = &_np_0;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0