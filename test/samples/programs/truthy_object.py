class A:
    value: bool

    def __bool__(self) -> bool:
        return self.value


a_true = A(True)
a_false = A(False)

if (a_true):
    print(a_true)
if (a_false):
    print(a_false)
