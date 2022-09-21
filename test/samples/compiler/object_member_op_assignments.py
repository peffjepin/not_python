class A:
    value: int = 1

    def __init__(self):
        self.value += 1


a = A()
a.value += 1
