// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="one", .length=3},
{.data="two", .length=3},
{.data="three", .length=5}};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpString NP_var1;
NpDict* dict_items_d;
NpDict* NP_var4;
NpIter NP_var2;
NpInt NP_var6;
NpString NP_var7;
NpInt NP_var9;
NpString NP_var10;
NpString NP_var11;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = NOT_PYTHON_STRING_CONSTANTS[0];
dict_items_d = DICT_INIT(NpInt, NpString, np_void_int_eq);
np_dict_set_item(dict_items_d, &NP_var0, &NP_var1);
NP_var0 = 2;
NP_var1 = NOT_PYTHON_STRING_CONSTANTS[1];
np_dict_set_item(dict_items_d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = NOT_PYTHON_STRING_CONSTANTS[2];
np_dict_set_item(dict_items_d, &NP_var0, &NP_var1);
NP_var4 = dict_items_d;
NP_var2 = np_dict_iter_items(NP_var4);
DictItem NP_var5;
void* NP_var8 = NULL;
while ((NP_var8 = NP_var2.next(NP_var2.iter),
NP_var5 = (NP_var8) ? *(DictItem*)NP_var8 : NP_var5,
NP_var6 = (NP_var8) ? *(NpInt*)(NP_var5.key) : NP_var6,
NP_var7 = (NP_var8) ? *(NpString*)(NP_var5.val) : NP_var7,
NP_var8)){
NP_var9 = NP_var6;
NP_var10 = NP_var7;
NP_var11 = np_int_to_str(NP_var9);
builtin_print(2, NP_var11, NP_var10);
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0