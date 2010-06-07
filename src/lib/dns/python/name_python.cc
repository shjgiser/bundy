// Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

// $Id$

//
// Declaration of the custom exceptions
// Initialization and addition of these go in the module init at the
// end
//
static PyObject* po_EmptyLabel;
static PyObject* po_TooLongName;
static PyObject* po_TooLongLabel;
static PyObject* po_BadLabelType;
static PyObject* po_BadEscape;
static PyObject* po_IncompleteName;
static PyObject* po_InvalidBufferPosition;
static PyObject* po_DNSMessageFORMERR;
// This one's for our 'general' exception
// TODO: does this need an according general class?
// should we replace it with some more direct base exception?
static PyObject* po_IscException;

//
// Declaration of enums
// Initialization and addition of these go in the module init at the
// end
//
static PyObject* po_NameRelation;

//
// Definition of the classes
//

// For each class, we need a struct, a helper functions (init, destroy,
// and static wrappers around the methods we export), a list of methods,
// and a type description
using namespace isc::dns;

// NameComparisonResult
typedef struct {
    PyObject_HEAD
    isc::dns::NameComparisonResult* ncr;
} s_NameComparisonResult;

static int NameComparisonResult_init(s_NameComparisonResult* self UNUSED_PARAM, PyObject* args UNUSED_PARAM);
static void NameComparisonResult_destroy(s_NameComparisonResult* self);
static PyObject* NameComparisonResult_getOrder(s_NameComparisonResult* self);
static PyObject* NameComparisonResult_getCommonLabels(s_NameComparisonResult* self);
static PyObject* NameComparisonResult_getRelation(s_NameComparisonResult* self);

static PyMethodDef NameComparisonResult_methods[] = {
    { "get_order", (PyCFunction)NameComparisonResult_getOrder, METH_NOARGS,
      "Returns the order" },
    { "get_common_labels", (PyCFunction)NameComparisonResult_getCommonLabels, METH_NOARGS,
      "Returns the number of common labels" },
    { "get_relation", (PyCFunction)NameComparisonResult_getRelation, METH_NOARGS,
      "Returns the relation" },
    { NULL, NULL, 0, NULL }
};

static PyTypeObject name_comparison_result_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "libdns_python.NameComparisonResult",
    sizeof(s_NameComparisonResult),                       /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)NameComparisonResult_destroy,   /* tp_dealloc */
    NULL,                               /* tp_print */
    NULL,                               /* tp_getattr */
    NULL,                               /* tp_setattr */
    NULL,                               /* tp_reserved */
    NULL,                               /* tp_repr */
    NULL,                               /* tp_as_number */
    NULL,                               /* tp_as_sequence */
    NULL,                               /* tp_as_mapping */
    NULL,                               /* tp_hash  */
    NULL,                               /* tp_call */
    NULL,                               /* tp_str */
    NULL,                               /* tp_getattro */
    NULL,                               /* tp_setattro */
    NULL,                               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    "This is a supplemental class used only as a return value of Name.compare(). "
    "It encapsulate a tuple of the comparison: ordering, number of common labels, "
    "and relationship as follows:\n"
    "- ordering: relative ordering under the DNSSEC order relation\n"
    "- labels: the number of common significant labels of the two names being"
    "  compared\n"
    "- relationship: see NameComparisonResult.NameRelation\n",
    NULL,                               /* tp_traverse */
    NULL,                               /* tp_clear */
    NULL,                               /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    NULL,                               /* tp_iter */
    NULL,                               /* tp_iternext */
    NameComparisonResult_methods,                       /* tp_methods */
    NULL,                               /* tp_members */
    NULL,                               /* tp_getset */
    NULL,                               /* tp_base */
    NULL,                               /* tp_dict */
    NULL,                               /* tp_descr_get */
    NULL,                               /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)NameComparisonResult_init,                /* tp_init */
    NULL,                               /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    NULL,                               /* tp_free */
    NULL,                               /* tp_is_gc */
    NULL,                               /* tp_bases */
    NULL,                               /* tp_mro */
    NULL,                               /* tp_cache */
    NULL,                               /* tp_subclasses */
    NULL,                               /* tp_weaklist */
    // Note: not sure if the following are correct.  Added them just to
    // make the compiler happy.
    NULL,                               /* tp_del */
    0                                   /* tp_version_tag */
};

// TODO: is there also a way to just not define it?
static int
NameComparisonResult_init(s_NameComparisonResult* self UNUSED_PARAM, PyObject* args UNUSED_PARAM) {
    PyErr_SetString(PyExc_NotImplementedError,
                    "NameComparisonResult can't be built directly");
    return -1;
}

static void
NameComparisonResult_destroy(s_NameComparisonResult* self) {
    delete self->ncr;
    self->ncr = NULL;
    Py_TYPE(self)->tp_free(self);
}

static PyObject* 
NameComparisonResult_getOrder(s_NameComparisonResult* self) {
    return Py_BuildValue("i", self->ncr->getOrder());
}

static PyObject* 
NameComparisonResult_getCommonLabels(s_NameComparisonResult* self) {
    return Py_BuildValue("I", self->ncr->getCommonLabels());
}

static PyObject* 
NameComparisonResult_getRelation(s_NameComparisonResult* self) {
    return Py_BuildValue("I", self->ncr->getRelation());
}

// end of NameComparisonResult

// Name
typedef struct {
    PyObject_HEAD
    isc::dns::Name* name;
    size_t position;
} s_Name;

static int Name_init(s_Name* self, PyObject* args);
static void Name_destroy(s_Name* self);

static PyObject* Name_toWire(s_Name* self, PyObject* args);
static PyObject* Name_toText(s_Name* self);
static PyObject* Name_str(PyObject* self);
static PyObject* Name_getLabelCount(s_Name* self);
static PyObject* Name_at(s_Name* self, PyObject* args);
static PyObject* Name_getLength(s_Name* self);

static PyObject* Name_compare(s_Name* self, PyObject* args);
static PyObject* Name_equals(s_Name* self, PyObject* args);

static PyObject* Name_richcmp(s_Name* n1, s_Name* n2, int op);
static PyObject* Name_split(s_Name* self, PyObject* args);
static PyObject* Name_reverse(s_Name* self);
static PyObject* Name_concatenate(s_Name* self, PyObject* args);
static PyObject* Name_downcase(s_Name* self);
static PyObject* Name_isWildCard(s_Name* self);

static PyMethodDef Name_methods[] = {
    { "at", (PyCFunction)Name_at, METH_VARARGS,
      "Returns the integer value of the name data at the specified position" },
    { "get_length", (PyCFunction)Name_getLength, METH_NOARGS,
      "Returns the length" },
    { "get_labelcount", (PyCFunction)Name_getLabelCount, METH_NOARGS,
      "Returns the number of labels" },
    { "to_text", (PyCFunction)Name_toText, METH_NOARGS,
      "Returns the string representation" },
    { "to_wire", (PyCFunction)Name_toWire, METH_VARARGS,
      "Converts the Name object to wire format.\n"
      "The argument can be either a MessageRenderer or an object that "
      "implements the sequence interface. If the object is mutable "
      "(for instance a bytearray()), the wire data is added in-place.\n"
      "If it is not (for instance a bytes() object), a new object is "
      "returned" },
    { "compare", (PyCFunction)Name_compare, METH_VARARGS,
      "Returns a NameComparisonResult object. The argument must be another Name object" },
    { "equals", (PyCFunction)Name_equals, METH_VARARGS,
      "Returns true if the given Name object is equal to this one" },
    { "split", (PyCFunction)Name_split, METH_VARARGS,
      "Splits the name, takes two arguments, the first is an integer "
      "specifying the first label to place in the result. The second "
      "is an integer specifying the number of labels to put in the "
      "result. Returns a new Name object" },
    { "reverse", (PyCFunction)Name_reverse, METH_NOARGS,
      "Returns a new Name object that is the reverse of this one" },
    { "concatenate", (PyCFunction)Name_concatenate, METH_VARARGS,
      "Concatenates the given Name object to this one and returns the "
      "result as a new Name object" },
    { "downcase", (PyCFunction)Name_downcase, METH_NOARGS,
      "Downcases this name object (in-place). Returns a new reference to the Name." },
    { "is_wildcard", (PyCFunction)Name_isWildCard, METH_NOARGS,
      "Returns True if the Name object represents a wildcard name." },
    { NULL, NULL, 0, NULL }
};

static PyTypeObject name_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "libdns_python.Name",
    sizeof(s_Name),                       /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)Name_destroy,   /* tp_dealloc */
    NULL,                               /* tp_print */
    NULL,                               /* tp_getattr */
    NULL,                               /* tp_setattr */
    NULL,                               /* tp_reserved */
    NULL,                               /* tp_repr */
    NULL,                               /* tp_as_number */
    NULL,                               /* tp_as_sequence */
    NULL,                               /* tp_as_mapping */
    NULL,                               /* tp_hash  */
    NULL,                               /* tp_call */
    Name_str,                           /* tp_str */
    NULL,                               /* tp_getattro */
    NULL,                               /* tp_setattro */
    NULL,                               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    "The Name class encapsulates DNS names.\n"
    "It provides interfaces to construct a name from string or wire-format data, "
    "transform a name into a string or wire-format data, compare two names, get "
    "access to various properties of a name, etc.",
    NULL,                               /* tp_traverse */
    NULL,                               /* tp_clear */
    (richcmpfunc)Name_richcmp,                               /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    NULL,                               /* tp_iter */
    NULL,                               /* tp_iternext */
    Name_methods,                       /* tp_methods */
    NULL,                               /* tp_members */
    NULL,                               /* tp_getset */
    NULL,                               /* tp_base */
    NULL,                               /* tp_dict */
    NULL,                               /* tp_descr_get */
    NULL,                               /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)Name_init,                /* tp_init */
    NULL,                               /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    NULL,                               /* tp_free */
    NULL,                               /* tp_is_gc */
    NULL,                               /* tp_bases */
    NULL,                               /* tp_mro */
    NULL,                               /* tp_cache */
    NULL,                               /* tp_subclasses */
    NULL,                               /* tp_weaklist */
    // Note: not sure if the following are correct.  Added them just to
    // make the compiler happy.
    NULL,                               /* tp_del */
    0                                   /* tp_version_tag */
};


static int
Name_init(s_Name* self, PyObject* args)
{
    const char* s;
    PyObject* downcase = Py_False;

    /* fromText */
    if (PyArg_ParseTuple(args, "s|O!", &s, &PyBool_Type, &downcase)) {
        try {
            const std::string n(s);

            self->name = new Name(n, downcase == Py_True);
            self->position = 0;
        } catch (EmptyLabel) {
            PyErr_SetString(po_EmptyLabel, "EmptyLabel");
            return -1;
        } catch (TooLongLabel) {
            PyErr_SetString(po_TooLongLabel, "TooLongLabel");
            return -1;
        } catch (BadLabelType) {
            PyErr_SetString(po_BadLabelType, "BadLabelType");
            return -1;
        } catch (BadEscape) {
            PyErr_SetString(po_BadEscape, "BadEscape");
            return -1;
        } catch (TooLongName) {
            PyErr_SetString(po_TooLongName, "TooLongName");
            return -1;
        } catch (IncompleteName) {
            PyErr_SetString(po_IncompleteName, "IncompleteName");
            return -1;
#ifdef CATCHMEMERR
        } catch (std::bad_alloc) {
            PyErr_NoMemory();
            return -1;
#endif
        } catch (...) {
            PyErr_SetString(po_IscException, "Unexpected?!");
            return -1;
        }
        return 0;
    }
    PyErr_Clear();

    PyObject* bytes_obj;
    const char* bytes;
    Py_ssize_t len;
    unsigned int position = 0;

    /* fromWire */
    if (PyArg_ParseTuple(args, "O|IO!", &bytes_obj, &position,
                         &PyBool_Type, &downcase) &&
                         PyObject_AsCharBuffer(bytes_obj, &bytes, &len) != -1) {
        try {
            InputBuffer buffer(bytes, len);

            buffer.setPosition(position);
            self->name = new Name(buffer, downcase == Py_True);
            self->position = buffer.getPosition();
        } catch (InvalidBufferPosition) {
            PyErr_SetString(po_InvalidBufferPosition,
                            "InvalidBufferPosition");
            return -1;
        } catch (DNSMessageFORMERR) {
            PyErr_SetString(po_DNSMessageFORMERR, "DNSMessageFORMERR");
            return -1;
        } catch (...) {
            PyErr_SetString(po_IscException, "Unexpected?!");
            return -1;
        }
        return 0;
    }

    PyErr_Clear();
    PyErr_SetString(PyExc_TypeError,
                    "No valid types in Name constructor (should be string or sequence and offset");
    return -1;
}

static void
Name_destroy(s_Name* self)
{
    delete self->name;
    self->name = NULL;
    Py_TYPE(self)->tp_free(self);
}

static PyObject*
Name_at(s_Name* self, PyObject* args)
{
    size_t pos;
    if (!PyArg_ParseTuple(args, "i", &pos)) {
        return NULL;
    }
    try {
        return Py_BuildValue("i", self->name->at(pos));
    } catch (isc::OutOfRange oor) {
        PyErr_SetString(PyExc_IndexError,
                        "name index out of range");
        return NULL;
    }
}

static PyObject*
Name_getLength(s_Name* self)
{
    return Py_BuildValue("i", self->name->getLength());
}

static PyObject*
Name_getLabelCount(s_Name* self)
{
    return Py_BuildValue("i", self->name->getLabelCount());
}

static PyObject*
Name_toText(s_Name* self)
{
    return Py_BuildValue("s", self->name->toText().c_str());
}

static PyObject*
Name_str(PyObject* self)
{
    // Simply call the to_text method we already defined
    return PyObject_CallMethod(self, (char*)"to_text", (char*)"");
}

static PyObject*
Name_toWire(s_Name* self, PyObject* args)
{
    PyObject* bytes;
    s_MessageRenderer* mr;
    
    if (PyArg_ParseTuple(args, "O", &bytes) && PySequence_Check(bytes)) {
        PyObject* bytes_o = bytes;
        
        OutputBuffer buffer(255);
        self->name->toWire(buffer);
        PyObject* n = PyBytes_FromStringAndSize((const char*) buffer.getData(), buffer.getLength());
        PyObject* result = PySequence_InPlaceConcat(bytes_o, n);
        // We need to release the object we temporarily created here
        // to prevent memory leak
        Py_DECREF(n);
        return result;
    } else if (PyArg_ParseTuple(args, "O!", &messagerenderer_type, (PyObject**) &mr)) {
        self->name->toWire(*mr->messagerenderer);
        // If we return NULL it is seen as an error, so use this for
        // None returns
        Py_RETURN_NONE;
    }
    PyErr_Clear();
    PyErr_SetString(PyExc_TypeError,
                    "toWire argument must be a sequence object or a MessageRenderer");
    return NULL;
}

static PyObject*
Name_compare(s_Name* self, PyObject* args) {
    s_Name* other;

    if (!PyArg_ParseTuple(args, "O!", &name_type, (PyObject* *) &other))
        return NULL;

    s_NameComparisonResult* ret = PyObject_New(s_NameComparisonResult, &name_comparison_result_type);
    if (ret != NULL) {
        ret->ncr = new NameComparisonResult(
            self->name->compare(*other->name));
        if (ret->ncr == NULL) {
            Py_DECREF(ret);
            return NULL;
        }
    }
    return (PyObject*) ret;
}

static PyObject* 
Name_equals(s_Name* self, PyObject* args) {
    s_Name* other;

    if (!PyArg_ParseTuple(args, "O!", &name_type, (PyObject* *) &other))
        return NULL;

    if (self->name->equals(*other->name))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject* 
Name_split(s_Name* self, PyObject* args)
{
    unsigned int first, n;

    if (!PyArg_ParseTuple(args, "II", &first, &n))
        return NULL;

    s_Name* ret = PyObject_New(s_Name, &name_type);
    if (ret != NULL) {
        ret->name = NULL;
        try {
            ret->name = new Name(self->name->split(first, n));
        } catch(isc::OutOfRange oor) {
            PyErr_SetString(PyExc_IndexError, oor.what());
            ret->name = NULL;
        }
        if (ret->name == NULL) {
            Py_DECREF(ret);
            return NULL;
        }
    }
    return (PyObject*) ret;
}
#include <iostream>

static PyObject* 
Name_richcmp(s_Name* n1, s_Name* n2, int op)
{
    bool c;

    // Check for null and if the types match. If different type,
    // simply return False
    if (!n2 ||
        ((PyObject*)n1)->ob_type != ((PyObject*)n2)->ob_type
       ) {
        Py_RETURN_FALSE;
    }

    switch (op) {
    case Py_LT:
        c = n1->name->lthan(*n2->name);
        break;
    case Py_LE:
        c = n1->name->leq(*n2->name);
        break;
    case Py_EQ:
        c = n1->name->equals(*n2->name);
        break;
    case Py_NE:
        c = n1->name->nequals(*n2->name);
        break;
    case Py_GT:
        c = n1->name->gthan(*n2->name);
        break;
    case Py_GE:
        c = n1->name->geq(*n2->name);
        break;
    default:
        assert(0);              // XXX: should trigger an exception
    }
    if (c) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject*
Name_reverse(s_Name* self)
{
    s_Name* ret = PyObject_New(s_Name, &name_type);

    if (ret != NULL) {
        ret->name = new Name(self->name->reverse());
        if (ret->name == NULL) {
            Py_DECREF(ret);
            return NULL;
        }
    }
    return (PyObject*) ret;
}

static PyObject*
Name_concatenate(s_Name* self, PyObject* args)
{
    s_Name* other;

    if (!PyArg_ParseTuple(args, "O!", &name_type, (PyObject**) &other))
        return NULL;

    s_Name* ret = PyObject_New(s_Name, &name_type);
    if (ret != NULL) {
        try {
            ret->name = new Name(self->name->concatenate(*other->name));
        } catch (isc::dns::TooLongName tln) {
            PyErr_SetString(po_TooLongName, tln.what());
            ret->name = NULL;
        }
        if (ret->name == NULL) {
            Py_DECREF(ret);
            return NULL;
        }
    }
    return (PyObject*) ret;
}

static PyObject*
Name_downcase(s_Name* self)
{
    self->name->downcase();
    Py_INCREF(self);
    return (PyObject*) self;
}

static PyObject*
Name_isWildCard(s_Name* self)
{
    if (self->name->isWildcard()) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
// end of Name


// Module Initialization, all statics are initialized here
bool
initModulePart_Name(PyObject* mod)
{
    // Add the classes to the module
    // We initialize the static description object with PyType_Ready(),
    // then add it to the module

    // NameComparisonResult
    if (PyType_Ready(&name_comparison_result_type) < 0) {
        return false;
    }
    Py_INCREF(&name_comparison_result_type);

    // Add the enums to the module
    po_NameRelation = Py_BuildValue("{i:s,i:s,i:s,i:s}",
                                    0, "SUPERDOMAIN",
                                    1, "SUBDOMAIN",
                                    2, "EQUAL",
                                    3, "COMMONANCESTOR");
    addClassVariable(name_comparison_result_type, "NameRelation", po_NameRelation);

    PyModule_AddObject(mod, "NameComparisonResult",
                       (PyObject*) &name_comparison_result_type);
    
    if (PyType_Ready(&name_type) < 0) {
        return false;
    }
    Py_INCREF(&name_type);

    // Add the constants to the module
    addClassVariable(name_type, "MAX_WIRE", Py_BuildValue("I", 255U));
    addClassVariable(name_type, "MAX_LABELS", Py_BuildValue("I", 128U));
    addClassVariable(name_type, "MAX_LABELLEN", Py_BuildValue("I", 63U));
    addClassVariable(name_type, "MAX_COMPRESS_POINTER", Py_BuildValue("I", 0x3fffU));
    addClassVariable(name_type, "COMPRESS_POINTER_MARK8", Py_BuildValue("I", 0xc0U));
    addClassVariable(name_type, "COMPRESS_POINTER_MARK16", Py_BuildValue("I", 0xc000U));

    s_Name* root_name = PyObject_New(s_Name, &name_type);
    root_name->name = new Name(".");
    PyObject* po_ROOT_NAME = (PyObject*) root_name;
    Py_INCREF(po_ROOT_NAME);
    addClassVariable(name_type, "ROOT_NAME", po_ROOT_NAME);

    // Add the exceptions to the module
    po_EmptyLabel = PyErr_NewException("libdns_python.EmptyLabel", NULL, NULL);
    PyModule_AddObject(mod, "EmptyLabel", po_EmptyLabel);

    po_TooLongName = PyErr_NewException("libdns_python.TooLongName", NULL, NULL);
    PyModule_AddObject(mod, "TooLongName", po_TooLongName);

    po_TooLongLabel = PyErr_NewException("libdns_python.TooLongLabel", NULL, NULL);
    PyModule_AddObject(mod, "TooLongLabel", po_TooLongLabel);

    po_BadLabelType = PyErr_NewException("libdns_python.BadLabelType", NULL, NULL);
    PyModule_AddObject(mod, "BadLabelType", po_BadLabelType);

    po_BadEscape = PyErr_NewException("libdns_python.BadEscape", NULL, NULL);
    PyModule_AddObject(mod, "BadEscape", po_BadEscape);

    po_IncompleteName = PyErr_NewException("libdns_python.IncompleteName", NULL, NULL);
    PyModule_AddObject(mod, "IncompleteName", po_IncompleteName);

    po_InvalidBufferPosition = PyErr_NewException("libdns_python.InvalidBufferPosition", NULL, NULL);
    PyModule_AddObject(mod, "InvalidBufferPosition", po_InvalidBufferPosition);

    /* TODO; this one is a message-specific one, move to message? */
    po_DNSMessageFORMERR = PyErr_NewException("libdns_python.DNSMessageFORMERR", NULL, NULL);
    Py_INCREF(po_DNSMessageFORMERR);
    PyModule_AddObject(mod, "DNSMessageFORMERR", po_DNSMessageFORMERR);

    /* TODO: this one is module-level, move to libdns_python.cc */
    po_IscException = PyErr_NewException("libdns_python.IscException", NULL, NULL);
    Py_INCREF(po_IncompleteName);
    PyModule_AddObject(mod, "IscException", po_IscException);

    PyModule_AddObject(mod, "Name",
                       (PyObject*) &name_type);
    
    return true;
}
