class A:
    value: int

    def fn(self) -> int:
        return self.value


a = A(1)
x = a.fn()
