.. highlight:: c

.. _typehintobjects:

Objects for Type Hinting
------------------------

Various built-in types for type hinting are provided.  Currently,
two types exist -- :ref:`GenericAlias <types-genericalias>` and
:ref:`Union <types-union>`.  Only ``GenericAlias`` is exposed to C.

.. c:function:: PyObject* Py_GenericAlias(PyObject *origin, PyObject *args)

   Create a :ref:`GenericAlias <types-genericalias>` object.
   Equivalent to calling the Python class
   :class:`types.GenericAlias`.  The *origin* and *args* arguments set the
   ``GenericAlias``\ 's ``__origin__`` and ``__args__`` attributes respectively.
   *origin* should be a :c:expr:`PyTypeObject*`, and *args* can be a
   :c:expr:`PyTupleObject*` or any ``PyObject*``.  If *args* passed is
   not a tuple, a 1-tuple is automatically constructed and ``__args__`` is set
   to ``(args,)``.
   Minimal checking is done for the arguments, so the function will succeed even
   if *origin* is not a type.
   The ``GenericAlias``\ 's ``__parameters__`` attribute is constructed lazily
   from ``__args__``.  On failure, an exception is raised and ``NULL`` is
   returned.

   .. seealso:: The data model method :meth:`~object.__class_getitem__`.

   .. versionadded:: 3.9

.. c:function:: PyValue Py_GenericAliasValue(PyObject *origin, PyValue args)

   :c:func:`Py_GenericAlias` in the :c:macro:`METH_O` calling convention, for
   a ``__class_getitem__`` method.  Here's an example of how to make an
   extension type generic::

      ...
      static PyMethodDef my_obj_methods[] = {
          // Other methods.
          ...
          {"__class_getitem__", _PyCFunction_CAST(Py_GenericAliasValue), METH_O|METH_CLASS, "my_obj is generic over its contained type"}
          ...
      }

   .. versionadded:: 3.16

.. c:var:: PyTypeObject Py_GenericAliasType

   The C type of the object returned by :c:func:`Py_GenericAlias`. Equivalent to
   :class:`types.GenericAlias` in Python.

   .. versionadded:: 3.9
