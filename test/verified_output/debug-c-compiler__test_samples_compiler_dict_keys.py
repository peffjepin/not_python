
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_keys_d;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
_np_0 = 1;
NpInt _np_1;
_np_1 = 2;
dict_keys_d = np_dict_init(8, 8, np_void_int_eq);
np_dict_set_item(dict_keys_d, &_np_0, &_np_1);
_np_0 = 3;
_np_1 = 4;
np_dict_set_item(dict_keys_d, &_np_0, &_np_1);
_np_0 = 5;
_np_1 = 6;
np_dict_set_item(dict_keys_d, &_np_0, &_np_1);
NpDict* _np_3;
_np_3 = dict_keys_d;
NpIter _np_4;
_np_4 = np_dict_iter_keys(_np_3);
NpInt _np_6;
while(1) {
_np_4.next_data = _np_4.next(_np_4.iter);
if (_np_4.next_data) _np_6 = *((NpInt*)_np_4.next_data);
NpPointer _np_7;
_np_7 = _np_4.next_data;
if (!_np_7) {
break;
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