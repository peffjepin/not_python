
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="one", .length=3},
{.data="two", .length=3},
{.data="three", .length=5}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_items_d;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
NpString _np_1;
_np_0 = 1;
_np_1 = NOT_PYTHON_STRING_CONSTANTS[0];
NpDict* _np_2;
_np_2 = DICT_INIT(NpInt, NpString, np_void_int_eq);
np_dict_set_item(_np_2, &_np_0, &_np_1);
_np_0 = 2;
_np_1 = NOT_PYTHON_STRING_CONSTANTS[1];
np_dict_set_item(_np_2, &_np_0, &_np_1);
_np_0 = 3;
_np_1 = NOT_PYTHON_STRING_CONSTANTS[2];
np_dict_set_item(_np_2, &_np_0, &_np_1);
dict_items_d = _np_2;
NpDict* _np_7;
_np_7 = dict_items_d;
NpIter _np_5;
_np_5 = np_dict_iter_items(_np_7);
NpInt _np_8;
NpString _np_9;
void* _np_3;
_np_3 = _np_5.next(_np_5.iter);
while(_np_3) {
_np_8 = *((NpInt*)(((DictItem*)_np_3)->key));
_np_9 = *((NpString*)(((DictItem*)_np_3)->val));
NpInt _np_11;
_np_11 = _np_8;
NpString _np_12;
_np_12 = np_int_to_str(_np_11);
NpString _np_13;
_np_13 = _np_9;
void* _np_10;
_np_10 = builtin_print(2, _np_12, _np_13);
_np_4:
_np_3 = _np_5.next(_np_5.iter);
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0