
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_insert_l;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
NpInt _np_0;
_np_0 = 1;
list_insert_l = np_list_init(8, np_int_sort_fn, np_int_sort_fn_rev, np_void_int_eq);
if (global_exception) {
return 1;
}
np_list_append(list_insert_l, &_np_0);
if (global_exception) {
return 1;
}
_np_0 = 2;
np_list_append(list_insert_l, &_np_0);
if (global_exception) {
return 1;
}
_np_0 = 3;
np_list_append(list_insert_l, &_np_0);
if (global_exception) {
return 1;
}
NpList* _np_1;
_np_1 = list_insert_l;
NpInt _np_3;
_np_3 = 1;
NpInt _np_4;
_np_4 = 10;
NpNone _np_2;
_np_2 = np_list_insert(_np_1, _np_3, &_np_4);
if (global_exception) {
return 1;
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0