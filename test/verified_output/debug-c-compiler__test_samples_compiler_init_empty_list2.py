
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* init_empty_list2_l;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
init_empty_list2_l = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0