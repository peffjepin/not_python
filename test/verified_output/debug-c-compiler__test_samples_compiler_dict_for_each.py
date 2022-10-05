// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpInt NP_var1;
NpDict* dict_for_each_d;
NpDict* NP_var2;
NpInt NP_var3;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_for_each_d = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(dict_for_each_d, &NP_var0, &NP_var1);
NP_var2 = dict_for_each_d;
DICT_ITER_KEYS(NP_var2, NpInt, NP_var3, NP_var4)
{
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0