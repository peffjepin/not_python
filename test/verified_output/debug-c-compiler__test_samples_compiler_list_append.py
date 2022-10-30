
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_append_l;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
if (global_exception) {
return 1;
}
list_append_l = np_list_init(8, np_int_sort_fn, np_int_sort_fn_rev, np_void_int_eq);
NpList* _np_0;
_np_0 = list_append_l;
NpInt _np_2;
_np_2 = 1;
NpNone _np_1;
_np_1 = np_list_append(_np_0, &_np_2);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0