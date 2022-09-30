// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="one", .length=3},
{.data="two", .length=3},
{.data="three", .length=5}};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyString NP_var1;
PyDict* dict_items_d;
PyDict* NP_var4;
PyIter NP_var2;
PyInt NP_var6;
PyString NP_var7;
PyInt NP_var9;
PyString NP_var10;
PyString NP_var11;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = NOT_PYTHON_STRING_CONSTANTS[0];
dict_items_d = DICT_INIT(PyInt, PyString, void_int_eq);
dict_set_item(dict_items_d, &NP_var0, &NP_var1);
NP_var0 = 2;
NP_var1 = NOT_PYTHON_STRING_CONSTANTS[1];
dict_set_item(dict_items_d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = NOT_PYTHON_STRING_CONSTANTS[2];
dict_set_item(dict_items_d, &NP_var0, &NP_var1);
NP_var4 = dict_items_d;
NP_var2 = dict_iter_items(NP_var4);
DictItem NP_var5;
void* NP_var8 = NULL;
while ((NP_var8 = NP_var2.next(NP_var2.iter),
NP_var5 = (NP_var8) ? *(DictItem*)NP_var8 : NP_var5,
NP_var6 = (NP_var8) ? *(PyInt*)(NP_var5.key) : NP_var6,
NP_var7 = (NP_var8) ? *(PyString*)(NP_var5.val) : NP_var7,
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