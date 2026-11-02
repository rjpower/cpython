import unittest


class MroAdjacentReadersTests(unittest.TestCase):
    def test_successful_bases_reassignment_updates_descendants(self):
        class A:
            pass

        class B:
            pass

        class C(A):
            pass

        class D(C):
            pass

        C.__bases__ = (B,)
        self.assertEqual(C.__mro__, (C, B, object))
        self.assertEqual(D.__mro__, (D, C, B, object))
        self.assertEqual(B.__subclasses__(), [C])
        self.assertEqual(A.__subclasses__(), [])

    def test_invalid_scalar_bases_keep_exact_diagnostic_and_original_bases(self):
        class C:
            pass

        original = C.__bases__
        for value, type_name in ((123456789012345678901234567890, 'int'),
                                 (4.5, 'float'),
                                 (None, 'NoneType')):
            with self.subTest(value=value):
                message = ("C.__bases__ must be tuple of classes, not "
                           f"'{type_name}'")
                with self.assertRaisesRegex(TypeError, '^' + message + '$'):
                    C.__bases__ = (value,)
                self.assertIs(C.__bases__, original)

    def test_cycle_and_layout_rejections_preserve_bases(self):
        class A:
            pass

        class C(A):
            pass

        original = C.__bases__
        with self.assertRaisesRegex(TypeError, 'inheritance cycle'):
            C.__bases__ = (C,)
        self.assertIs(C.__bases__, original)

        class Left:
            __slots__ = ('left',)

        class Right:
            __slots__ = ('right',)

        class Slotted(Left):
            pass

        old_bases = Slotted.__bases__
        with self.assertRaises(TypeError):
            Slotted.__bases__ = (Right,)
        self.assertIs(Slotted.__bases__, old_bases)

    def test_custom_mro_failure_restores_earlier_descendant_mros(self):
        sentinel = RuntimeError('bozo')

        class WorkOnce(type):
            def __new__(mcls, name, bases, namespace):
                mcls.flag = 0
                return super().__new__(mcls, name, bases, namespace)

            def mro(cls):
                if cls.flag > 0:
                    raise sentinel
                cls.flag += 1
                return type.mro(cls)

        class WorkAlways(type):
            def mro(cls):
                return type.mro(cls)

        class C:
            pass

        class C2:
            pass

        class D(C):
            pass

        class E(D):
            pass

        class F(D, metaclass=WorkOnce):
            pass

        class G(D, metaclass=WorkAlways):
            pass

        e_mro_before = E.__mro__
        d_mro_before = D.__mro__
        with self.assertRaises(RuntimeError) as caught:
            D.__bases__ = (C2,)
        self.assertIs(caught.exception, sentinel)
        self.assertIs(E.__mro__, e_mro_before)
        self.assertIs(D.__mro__, d_mro_before)

    def test_reentrant_bases_assignment_keeps_inner_selected_result(self):
        self.ready = False

        class M(type):
            def mro(cls):
                if self.ready and cls.__name__ == 'C':
                    self.ready = False
                    C.__bases__ = (B2,)
                return type.mro(cls)

        class A(metaclass=M):
            pass

        class B1(A):
            pass

        class B2(A):
            pass

        class C(A):
            pass

        self.ready = True
        C.__bases__ = (B1,)
        B1.__bases__ = (C,)

        self.assertEqual(C.__bases__, (B2,))
        self.assertEqual(B2.__subclasses__(), [C])
        self.assertEqual(B1.__subclasses__(), [])
        self.assertEqual(B1.__bases__, (C,))
        self.assertEqual(C.__subclasses__(), [B1])


if __name__ == '__main__':
    unittest.main(verbosity=2)
