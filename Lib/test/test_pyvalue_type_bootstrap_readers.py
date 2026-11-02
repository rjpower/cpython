import unittest
import warnings

try:
    import _testcapi
except ImportError:
    _testcapi = None


class BootstrapReaderReferences(unittest.TestCase):
    def test_scalar_type_fast_path(self):
        class Text(str):
            pass
        class Object:
            pass
        for value, expected in [(0,int),(10**60,int),(4.5,float),(float('nan'),float),
                                (True,bool),(None,type(None)),('x',str),
                                (Text('x'),Text),(Object(),Object)]:
            self.assertIs(type(value), expected)
            self.assertIs(type.__call__(type, value), expected)

    def test_carlo_verre_guards(self):
        with self.assertRaisesRegex(TypeError, "can't apply this __setattr__ to type object"):
            object.__setattr__(str, 'bootstrap_marker', 1)
        with self.assertRaisesRegex(TypeError, "can't apply this __delattr__ to type object"):
            object.__delattr__(str, 'lower')

    def test_slot_dispatch_smoke(self):
        class Value:
            def __add__(self, other): return ('add', other)
            def __radd__(self, other): return ('radd', other)
            def __len__(self): return 2
            def __getitem__(self, index): return index + 10
        value=Value()
        self.assertEqual(value+3, ('add',3))
        self.assertEqual(3+value, ('radd',3))
        self.assertEqual(len(value),2)
        self.assertEqual(value[1],11)

    def test_set_name_uses_original_copy_and_skips_scalars(self):
        events=[]
        class Descriptor:
            def __init__(self, first=False): self.first=first
            def __set_name__(self, owner, name):
                events.append(name)
                if self.first: owner.later='replacement'
        class Owner:
            first=Descriptor(True)
            number=123
            later=Descriptor()
        self.assertEqual(events,['first','later'])
        self.assertEqual(Owner.later,'replacement')
        self.assertEqual(Owner.number,123)

    def test_set_name_note_keeps_selected_names(self):
        sentinel=RuntimeError('set-name sentinel')
        selected={}
        events=[]
        class Key:
            def __repr__(self):
                events.append('key-repr')
                selected['owner'].__name__='RenamedOwner'
                Descriptor.__name__='RenamedDescriptor'
                return 'KEY'
        class Descriptor:
            def __set_name__(self, owner, name):
                events.append('set-name')
                selected['owner']=owner
                raise sentinel
        key=Key()
        with warnings.catch_warnings(record=True):
            warnings.simplefilter('always',RuntimeWarning)
            with self.assertRaises(RuntimeError) as caught:
                type('OriginalOwner',(),{key:Descriptor()})
        self.assertIs(caught.exception,sentinel)
        self.assertEqual(events,['set-name','key-repr'])
        self.assertEqual(sentinel.__notes__,[
            "Error calling __set_name__ on 'Descriptor' instance KEY in 'OriginalOwner'"])

    @unittest.skipUnless(_testcapi is not None and
        {'create_type_with_token','get_tp_token','pytype_getbasebytoken'} <= vars(_testcapi).keys(),
        'host lacks existing type-token fixture')
    def test_existing_type_token_fixture(self):
        Base=_testcapi.create_type_with_token('bootstrap.TokenBase',0)
        class Child(Base): pass
        token=_testcapi.get_tp_token(Base)
        for use_mro in [True,False]:
            status,result=_testcapi.pytype_getbasebytoken(Child,token,use_mro,True)
            self.assertEqual(status,1)
            self.assertIs(result,Base)


if __name__ == '__main__':
    unittest.main(verbosity=2)
