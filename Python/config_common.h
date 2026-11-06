#include "pycore_value_api.h"     // _PyValue_AsObjectAt()


static inline int
_config_dict_get(PyObject *dict, const char *name, PyObject **p_item)
{
    PyValue value;
    int rc = PyDict_GetValueString(dict, name, &value);
    if (rc < 0) {
        return -1;
    }
    if (rc == 0) {
        // We do not set an exception.
        return -1;
    }
    PyObject *item = _PyValue_AsObjectAt(value, BOX_SITE_DICT);
    PyValue_CLOSE(value);
    if (item == NULL) {
        return -1;
    }
    *p_item = item;
    return 0;
}


static inline PyValue
config_dict_get_value(PyObject *dict, const char *name)
{
    PyValue item;
    int rc = PyDict_GetValueString(dict, name, &item);
    if (rc <= 0) {
        if (rc == 0) {
            PyErr_Format(PyExc_ValueError, "missing config key: %s", name);
        }
        return PyValue_NULL;
    }
    return item;
}


static PyObject*
config_dict_get(PyObject *dict, const char *name)
{
    PyObject *item;
    if (_config_dict_get(dict, name, &item) < 0) {
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "missing config key: %s", name);
        }
        return NULL;
    }
    return item;
}


static void
config_dict_invalid_type(const char *name)
{
    PyErr_Format(PyExc_TypeError, "invalid config type: %s", name);
}
