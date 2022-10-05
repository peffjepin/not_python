// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpList* list_index_l;
NpList* NP_var2;
NpInt NP_var3;
NpInt list_index_i;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_index_l = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(list_index_l, &NP_var0);
NP_var2 = list_index_l;
NP_var3 = 1;
list_index_i = np_list_index(NP_var2, &NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0