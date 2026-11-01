from test.support import is_apple_mobile
from test.test_importlib import abc, util

machinery = util.import_importlib('importlib.machinery')

import unittest
import sys
import sysconfig


class FinderTests(abc.FinderTests):

    """Test the finder for extension modules."""

    def setUp(self):
        if not self.machinery.EXTENSION_SUFFIXES or not util.EXTENSIONS:
            raise unittest.SkipTest("Requires dynamic loading support.")
        if util.EXTENSIONS.name in sys.builtin_module_names:
            raise unittest.SkipTest(
                f"{util.EXTENSIONS.name} is a builtin module"
            )

    def find_spec(self, fullname):
        if is_apple_mobile:
            # Apple mobile platforms require a specialist loader that uses
            # .fwork files as placeholders for the true `.so` files.
            loaders = [
                (
                    self.machinery.AppleFrameworkLoader,
                    [
                        ext.replace(".so", ".fwork")
                        for ext in self.machinery.EXTENSION_SUFFIXES
                    ]
                )
            ]
        else:
            loaders = [
                (
                    self.machinery.ExtensionFileLoader,
                    self.machinery.EXTENSION_SUFFIXES
                )
            ]

        importer = self.machinery.FileFinder(util.EXTENSIONS.path, *loaders)

        return importer.find_spec(fullname)

    def test_module(self):
        self.assertTrue(self.find_spec(util.EXTENSIONS.name))

    # No extension module as an __init__ available for testing.
    test_package = test_package_in_package = None

    # No extension module in a package available for testing.
    test_module_in_package = None

    # Extension modules cannot be an __init__ for a package.
    test_package_over_module = None

    def test_failure(self):
        self.assertIsNone(self.find_spec('asdfjkl;'))

    def test_runtime_extension_suffixes(self):
        suffixes = self.machinery.EXTENSION_SUFFIXES
        if 'win32' in sys.platform:
            # Either "_d.pyd" or ".pyd" must be in suffixes
            self.assertTrue({"_d.pyd", ".pyd"}.intersection(suffixes))
        else:
            # This fork's compiler/object ABI requires rebuilt extensions.
            # Generic upstream stable-ABI and untagged names are excluded.
            self.assertTrue(suffixes)
            self.assertTrue(all(suffix.startswith('.boilrc1-')
                                for suffix in suffixes), suffixes)


(Frozen_FinderTests,
 Source_FinderTests
 ) = util.test_both(FinderTests, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
