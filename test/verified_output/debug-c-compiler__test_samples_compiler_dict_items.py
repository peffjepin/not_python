
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="one", .length=3},
{.data="two", .length=3},
{.data="three", .length=5}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_items_d;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
NpInt _np_0;
_np_0 = 1;
NpString _np_1;
_np_1 = NOT_PYTHON_STRING_CONSTANTS[1];
dict_items_d = np_dict_init(8, 24, np_void_int_eq);
if (global_exception) {
return 1;
}
np_dict_set_item(dict_items_d, &_np_0, &_np_1);
if (global_exception) {
return 1;
}
_np_0 = 2;
_np_1 = NOT_PYTHON_STRING_CONSTANTS[2];
np_dict_set_item(dict_items_d, &_np_0, &_np_1);
if (global_exception) {
return 1;
}
_np_0 = 3;
_np_1 = NOT_PYTHON_STRING_CONSTANTS[3];
np_dict_set_item(dict_items_d, &_np_0, &_np_1);
if (global_exception) {
return 1;
}
NpDict* _np_3;
_np_3 = dict_items_d;
NpIter _np_4;
_np_4 = np_dict_iter_items(_np_3);
if (global_exception) {
return 1;
}
NpInt _np_6;
NpString _np_7;
DictItem _np_8;
while(1) {
_np_4.next_data = _np_4.next(_np_4.iter);
if (_np_4.next_data) _np_8 = *((DictItem*)_np_4.next_data);
NpPointer _np_9;
_np_9 = _np_4.next_data;
if (!_np_9) {
break;
}
NpPointer _np_10;
_np_10 = _np_8.key;
_np_6 = *((NpInt*)_np_10);
_np_10 = _np_8.val;
_np_7 = *((NpString*)_np_10);
NpInt _np_12;
_np_12 = _np_6;
NpString _np_13;
_np_13 = np_int_to_str(_np_12);
if (global_exception) {
return 1;
}
NpString _np_14;
_np_14 = _np_7;
NpNone _np_11;
_np_11 = builtin_print(2, _np_13, _np_14);
if (global_exception) {
return 1;
}
_np_2:
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0