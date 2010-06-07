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

#include <dns/rrclass.h>
using namespace isc::dns;

//
// Declaration of the custom exceptions
// Initialization and addition of these go in the initModulePart
// function at the end of this file
//
static PyObject* po_InvalidRRClass;
static PyObject* po_IncompleteRRClass;

//
// Definition of the classes
//

// For each class, we need a struct, a helper functions (init, destroy,
// and static wrappers around the methods we export), a list of methods,
// and a type description

//
// RRClass
//

// The s_* Class simply coverst one instantiation of the object
typedef struct {
    PyObject_HEAD
    RRClass* rrclass;
} s_RRClass;

//
// We declare the functions here, the definitions are below
// the type definition of the object, since both can use the other
//

// General creation and destruction
static int RRClass_init(s_RRClass* self, PyObject* args);
static void RRClass_destroy(s_RRClass* self);

// These are the functions we export
static PyObject* RRClass_toText(s_RRClass* self);
// This is a second version of toText, we need one where the argument
// is a PyObject*, for the str() function in python.
static PyObject* RRClass_str(PyObject* self);
static PyObject* RRClass_toWire(s_RRClass* self, PyObject* args);
static PyObject* RRClass_getCode(s_RRClass* self);
static PyObject* RRClass_richcmp(s_RRClass* self, s_RRClass* other, int op);

// This list contains the actual set of functions we have in
// python. Each entry has
// 1. Python method name
// 2. Our static function here
// 3. Argument type
// 4. Documentation
static PyMethodDef RRClass_methods[] = {
    { "to_text", (PyCFunction)RRClass_toText, METH_NOARGS,
      "Returns the string representation" },
    { "to_wire", (PyCFunction)RRClass_toWire, METH_VARARGS,
      "Converts the RRClass object to wire format.\n"
      "The argument can be either a MessageRenderer or an object that "
      "implements the sequence interface. If the object is mutable "
      "(for instance a bytearray()), the wire data is added in-place.\n"
      "If it is not (for instance a bytes() object), a new object is "
      "returned" },
    { "get_code", (PyCFunction)RRClass_getCode, METH_NOARGS,
      "Returns the class code as an integer" },
    { NULL, NULL, 0, NULL }
};

// This defines the complete type for reflection in python and
// parsing of PyObject* to s_RRClass
// Most of the functions are not actually implemented and NULL here.
static PyTypeObject rrclass_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "libdns_python.RRClass",
    sizeof(s_RRClass),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)RRClass_destroy,        /* tp_dealloc */
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
    RRClass_str,                               /* tp_str */
    NULL,                               /* tp_getattro */
    NULL,                               /* tp_setattro */
    NULL,                               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    "The RRClass class encapsulates DNS resource record classes.\n"
    "This class manages the 16-bit integer class codes in quite a straightforward"
    "way.  The only non trivial task is to handle textual representations of"
    "RR classes, such as \"IN\", \"CH\", or \"CLASS65534\".",
    NULL,                               /* tp_traverse */
    NULL,                               /* tp_clear */
    (richcmpfunc)RRClass_richcmp,       /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    NULL,                               /* tp_iter */
    NULL,                               /* tp_iternext */
    RRClass_methods,                    /* tp_methods */
    NULL,                               /* tp_members */
    NULL,                               /* tp_getset */
    NULL,                               /* tp_base */
    NULL,                               /* tp_dict */
    NULL,                               /* tp_descr_get */
    NULL,                               /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)RRClass_init,                /* tp_init */
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
RRClass_init(s_RRClass* self, PyObject* args)
{
    const char* s;
    unsigned int i;
    PyObject* bytes = NULL;
    // The constructor argument can be a string ("IN"), an integer (1),
    // or a sequence of numbers between 0 and 255 (wire code)

    // Note that PyArg_ParseType can set PyError, and we need to clear
    // that if we try several like here. Otherwise the *next* python
    // call will suddenly appear to throw an exception.
    // (the way to do exceptions is to set PyErr and return -1)
    try {
        if (PyArg_ParseTuple(args, "s", &s)) {
            self->rrclass = new RRClass(s);
            return 0;
        } else if (PyArg_ParseTuple(args, "I", &i)) {
            PyErr_Clear();
            if (i > 65535) {
                PyErr_SetString(po_InvalidRRClass, "Class number too high");
                return -1;
            }
            self->rrclass = new RRClass(i);
            return 0;
        } else if (PyArg_ParseTuple(args, "O", &bytes) && PySequence_Check(bytes)) {
            uint8_t data[2];
            int result = readDataFromSequence(data, 2, bytes);
            if (result != 0) {
                return result;
            }
            InputBuffer ib(data, 2);
            self->rrclass = new RRClass(ib);
            PyErr_Clear();
            return 0;
        }
    /* Incomplete is never thrown, a type error would have already been raised
     * when we try to read the 2 bytes above */
    } catch (InvalidRRClass ic) {
        PyErr_Clear();
        PyErr_SetString(po_InvalidRRClass, ic.what());
        return -1;
    }
    PyErr_Clear();
    PyErr_SetString(PyExc_TypeError,
                    "no valid type in constructor argument");
    return -1;
}

static void
RRClass_destroy(s_RRClass* self)
{
    if (self->rrclass != NULL)
        delete self->rrclass;
    self->rrclass = NULL;
    Py_TYPE(self)->tp_free(self);
}

static PyObject*
RRClass_toText(s_RRClass* self)
{
    // Py_BuildValue makes python objects from native data
    return Py_BuildValue("s", self->rrclass->toText().c_str());
}

static PyObject*
RRClass_str(PyObject* self)
{
    // Simply call the to_text method we already defined
    return PyObject_CallMethod(self, (char*)"to_text", (char*)"");
}

static PyObject*
RRClass_toWire(s_RRClass* self, PyObject* args)
{
    PyObject* bytes;
    s_MessageRenderer* mr;
    
    if (PyArg_ParseTuple(args, "O", &bytes) && PySequence_Check(bytes)) {
        PyObject* bytes_o = bytes;
        
        OutputBuffer buffer(2);
        self->rrclass->toWire(buffer);
        PyObject* n = PyBytes_FromStringAndSize((const char*) buffer.getData(), buffer.getLength());
        PyObject* result = PySequence_InPlaceConcat(bytes_o, n);
        // We need to release the object we temporarily created here
        // to prevent memory leak
        Py_DECREF(n);
        return result;
    } else if (PyArg_ParseTuple(args, "O!", &messagerenderer_type, (PyObject**) &mr)) {
        self->rrclass->toWire(*mr->messagerenderer);
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
RRClass_getCode(s_RRClass* self)
{
    return Py_BuildValue("I", self->rrclass->getCode());
}

static PyObject* 
RRClass_richcmp(s_RRClass* self, s_RRClass* other, int op)
{
    bool c;

    // Check for null and if the types match. If different type,
    // simply return False
    if (!other ||
        ((PyObject*)self)->ob_type != ((PyObject*)other)->ob_type
       ) {
        Py_RETURN_FALSE;
    }

    switch (op) {
    case Py_LT:
        c = *self->rrclass < *other->rrclass;
        break;
    case Py_LE:
        c = *self->rrclass < *other->rrclass ||
            *self->rrclass == *other->rrclass;
        break;
    case Py_EQ:
        c = self->rrclass->equals(*other->rrclass);
        break;
    case Py_NE:
        c = self->rrclass->nequals(*other->rrclass);
        break;
    case Py_GT:
        c = *other->rrclass < *self->rrclass;
        break;
    case Py_GE:
        c = *other->rrclass < *self->rrclass ||
            *self->rrclass == *other->rrclass;
        break;
    default:
        assert(0);              // XXX: should trigger an exception
    }
    if (c)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}
// end of RRClass


// Module Initialization, all statics are initialized here
bool
initModulePart_RRClass(PyObject* mod)
{
    // Add the exceptions to the module
    po_InvalidRRClass = PyErr_NewException("libdns_python.InvalidRRClass", NULL, NULL);
    Py_INCREF(po_InvalidRRClass);
    PyModule_AddObject(mod, "InvalidRRClass", po_InvalidRRClass);
    po_IncompleteRRClass = PyErr_NewException("libdns_python.IncompleteRRClass", NULL, NULL);
    Py_INCREF(po_IncompleteRRClass);
    PyModule_AddObject(mod, "IncompleteRRClass", po_IncompleteRRClass);

    // We initialize the static description object with PyType_Ready(),
    // then add it to the module. This is not just a check! (leaving
    // this out results in segmentation faults)
    if (PyType_Ready(&rrclass_type) < 0) {
        return false;
    }
    Py_INCREF(&rrclass_type);
    PyModule_AddObject(mod, "RRClass",
                       (PyObject*) &rrclass_type);
    
    return true;
}
