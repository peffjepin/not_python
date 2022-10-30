
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_copy_l1;
NpList* list_copy_l2;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
NpInt _np_0;
_np_0 = 1;
if (global_exception) {
return 1;
}
list_copy_l1 = np_list_init(8, np_int_sort_fn, np_int_sort_fn_rev, np_void_int_eq);
if (global_exception) {
return 1;
}
np_list_append(list_copy_l1, &_np_0);
_np_0 = 2;
if (global_exception) {
return 1;
}
np_list_append(list_copy_l1, &_np_0);
_np_0 = 3;
if (global_exception) {
return 1;
}
np_list_append(list_copy_l1, &_np_0);
NpList* _np_1;
_np_1 = list_copy_l1;
if (global_exception) {
return 1;
}
list_copy_l2 = np_list_copy(_np_1);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0