
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_pop_l;
NpInt list_pop_v1;
NpInt list_pop_v2;
NpInt list_pop_v1;
NpInt list_pop_v2;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
NpInt _np_0;
_np_0 = 1;
list_pop_l = np_list_init(8, np_int_sort_fn, np_int_sort_fn_rev, np_void_int_eq);
if (global_exception) {
return 1;
}
np_list_append(list_pop_l, &_np_0);
if (global_exception) {
return 1;
}
_np_0 = 2;
np_list_append(list_pop_l, &_np_0);
if (global_exception) {
return 1;
}
_np_0 = 3;
np_list_append(list_pop_l, &_np_0);
if (global_exception) {
return 1;
}
NpList* _np_1;
_np_1 = list_pop_l;
np_list_pop(_np_1, -1, &list_pop_v1);
if (global_exception) {
return 1;
}
NpList* _np_2;
_np_2 = list_pop_l;
NpInt _np_3;
_np_3 = 0;
np_list_pop(_np_2, _np_3, &list_pop_v2);
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