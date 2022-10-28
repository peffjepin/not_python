
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_sort_l;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
_np_0 = 1;
list_sort_l = np_list_init(8, np_int_sort_fn, np_int_sort_fn_rev, np_void_int_eq);
np_list_append(list_sort_l, &_np_0);
_np_0 = 2;
np_list_append(list_sort_l, &_np_0);
_np_0 = 3;
np_list_append(list_sort_l, &_np_0);
NpList* _np_1;
_np_1 = list_sort_l;
NpNone _np_2;
_np_2 = np_list_sort(_np_1, 0);
NpList* _np_3;
_np_3 = list_sort_l;
NpBool _np_5;
_np_5 = 1;
NpNone _np_4;
_np_4 = np_list_sort(_np_3, _np_5);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0