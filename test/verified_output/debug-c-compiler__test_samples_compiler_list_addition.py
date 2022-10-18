
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_addition_l;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_1;
_np_1 = 1;
NpList* _np_2;
_np_2 = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(_np_2, &_np_1);
_np_1 = 2;
np_list_append(_np_2, &_np_1);
_np_1 = 3;
np_list_append(_np_2, &_np_1);
NpList* _np_0;
_np_0 = _np_2;
NpInt _np_4;
_np_4 = 4;
NpList* _np_5;
_np_5 = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(_np_5, &_np_4);
_np_4 = 5;
np_list_append(_np_5, &_np_4);
_np_4 = 6;
np_list_append(_np_5, &_np_4);
NpList* _np_3;
_np_3 = _np_5;
list_addition_l = np_list_add(_np_0, _np_3);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0