class A:
    value: int

    def __iadd__(self, other: int) -> A:
        self.value += other
        return self


a = A(1)
a += 1
