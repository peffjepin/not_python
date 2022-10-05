// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var1;
NpList* NP_var0;
NpInt NP_var3;
NpList* NP_var2;
NpList* list_addition_l;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 1;
NP_var0 = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(NP_var0, &NP_var1);
NP_var1 = 2;
np_list_append(NP_var0, &NP_var1);
NP_var1 = 3;
np_list_append(NP_var0, &NP_var1);
NP_var3 = 4;
NP_var2 = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(NP_var2, &NP_var3);
NP_var3 = 5;
np_list_append(NP_var2, &NP_var3);
NP_var3 = 6;
np_list_append(NP_var2, &NP_var3);
list_addition_l = np_list_add(NP_var0, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0