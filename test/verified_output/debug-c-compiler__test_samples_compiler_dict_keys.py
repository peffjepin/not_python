
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_keys_d;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
NpInt _np_1;
_np_0 = 1;
_np_1 = 2;
NpDict* _np_2;
_np_2 = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(_np_2, &_np_0, &_np_1);
_np_0 = 3;
_np_1 = 4;
np_dict_set_item(_np_2, &_np_0, &_np_1);
_np_0 = 5;
_np_1 = 6;
np_dict_set_item(_np_2, &_np_0, &_np_1);
dict_keys_d = _np_2;
NpDict* _np_7;
_np_7 = dict_keys_d;
NpIter _np_5;
_np_5 = np_dict_iter_keys(_np_7);
NpInt _np_8;
void* _np_3;
_np_3 = _np_5.next(_np_5.iter);
while(_np_3) {
_np_8 = *((NpInt*)_np_3);
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