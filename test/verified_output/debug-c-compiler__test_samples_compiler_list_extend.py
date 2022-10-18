
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_extend_l1;
NpList* list_extend_l2;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
_np_0 = 1;
NpList* _np_1;
_np_1 = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(_np_1, &_np_0);
_np_0 = 2;
np_list_append(_np_1, &_np_0);
_np_0 = 3;
np_list_append(_np_1, &_np_0);
list_extend_l1 = _np_1;
NpInt _np_2;
_np_2 = 4;
NpList* _np_3;
_np_3 = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(_np_3, &_np_2);
_np_2 = 5;
np_list_append(_np_3, &_np_2);
_np_2 = 6;
np_list_append(_np_3, &_np_2);
list_extend_l2 = _np_3;
NpList* _np_6;
_np_6 = list_extend_l1;
NpList* _np_7;
_np_7 = list_extend_l2;
void* _np_4;
_np_4 = np_list_extend(_np_6, _np_7);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0