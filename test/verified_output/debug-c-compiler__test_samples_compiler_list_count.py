
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_count_l;
NpInt list_count_n;

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
_np_0 = 2;
np_list_append(_np_1, &_np_0);
_np_0 = 3;
np_list_append(_np_1, &_np_0);
list_count_l = _np_1;
NpList* _np_3;
_np_3 = list_count_l;
NpInt _np_4;
_np_4 = 2;
list_count_n = np_list_count(_np_3, &_np_4);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0