
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="hello", .length=5}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFunction fnref_fnref;
NpFunction fnref_my_function;

// FUNCTION DEFINITIONS COMPILER SECTION
void* _np_0(NpContext __ctx__, NpString value) {
NpString _np_2;
_np_2 = value;
void* _np_1;
_np_1 = builtin_print(1, _np_2);
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
fnref_my_function.addr = &_np_0;
fnref_fnref = fnref_my_function;
NpString _np_4;
_np_4 = NOT_PYTHON_STRING_CONSTANTS[0];
void* _np_3;
_np_3 = ((void* (*)(NpContext ctx, NpString))fnref_fnref.addr)(fnref_fnref.ctx, _np_4);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0