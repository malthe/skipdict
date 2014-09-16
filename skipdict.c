#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "skiplist.h"

#define MAXLEVEL 32
#define P 0.25

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

#if PY_MAJOR_VERSION >= 3
inline void PyString_ConcatAndDel(PyObject** lhs, PyObject* rhs)
{
    PyObject* s = PyUnicode_Concat(*lhs, rhs);
    Py_DECREF(lhs);
    *lhs = s;
}
inline void PyString_Concat(PyObject** lhs, PyObject* rhs)
{
    PyObject* s = PyUnicode_Concat(*lhs, rhs);
    Py_DECREF(lhs);
    *lhs = s;
}
#define PySliceObject PyObject
#define PyInt_FromLong PyLong_FromLong
#define PyInt_AsLong PyLong_AsLong
#define PyString_FromString PyUnicode_FromString
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize
#define PyString_AsString PyUnicode_AsASCIIString
#define _PyString_Join PyUnicode_Join
#define INITERROR return NULL
#else
#define INITERROR return
#endif

#define double_AsString(value) \
    PyOS_double_to_string(value, 'r', 0, Py_DTSF_ADD_DOT_0, 0);

#define float_Convert(op, obj)                                  \
    if (obj == Py_None) obj = NULL;                             \
    if (obj) op = PyFloat_AsDouble(obj);                        \
    if (op == -1 && PyErr_Occurred()) {                         \
        PyObject *repr = PyObject_Repr(obj);                    \
        PyErr_Format(PyExc_TypeError,                           \
                     "not a number: %s",                        \
                     PyString_AsString(repr));                  \
        Py_DECREF(repr);                                        \
        return NULL;                                            \
    }

#define PyType_Prepare(mod, name, type)                 \
    if (PyType_Ready(type) < 0) {                       \
        INITERROR;                                      \
    }                                                   \
    Py_INCREF(type);                                    \
    PyModule_AddObject(mod, name, (PyObject *)type);

#define skipdict_Check(op) PyObject_TypeCheck(op, &SkipDictType)
#define skipdict_CheckExact(op) (Py_TYPE(op) == &SkipDictType)

static PyTypeObject SkipDictType;
static PyTypeObject SkipDictIterType;

typedef enum {KEY, VALUE, ITEM} itertype;

typedef struct {
    PyObject_HEAD
    skiplist *skiplist;
    PyObject *random;
    PyObject *mapping;
} SkipDictObject;

typedef struct {
    PyObject_HEAD
    SkipDictObject *skipdict;
    skiplistiter *iter;
    itertype type;
    unsigned long length;
} SkipDictIterObject;

typedef PyObject * (*iterfunc)(double score, PyObject*);

static PyObject * skipdictiter_next_key(double score, PyObject* value);
static PyObject * skipdictiter_next_value(double score, PyObject* value);
static PyObject * skipdictiter_next_item(double score, PyObject* value);

static iterfunc iterators[3] = { skipdictiter_next_key,
                                 skipdictiter_next_value,
                                 skipdictiter_next_item };

static const char* iterator_names[3] = { "keys", "values", "items" };
static const char* booleans[2] = { "false", "true" };

static PyObject *
skipdict_index(SkipDictObject *self, PyObject *key)
{
    PyObject* item = PyDict_GetItem(self->mapping, key);
    if (!item) {
        PyErr_SetObject(PyExc_KeyError,
                        PyObject_Repr(key));
        return NULL;
    }
    PyObject* value = PyTuple_GET_ITEM(item, 1);
    double score = PyFloat_AsDouble(value);
    if (score == -1.0 && PyErr_Occurred()) return 0;

    unsigned long rank = slGetRank(self->skiplist, score, (void*) item);
    if (!rank) {
        return NULL;
    }

    return PyLong_FromLong(rank - 1);
}

static int
skipdictiter_fetch(skiplistiter *iter, double *score, PyObject **obj)
{
    PyObject* item;
    if (slIterGet(iter, score, (void*) &item)) {
        PyErr_SetString(PyExc_StopIteration, "");
        return -1;
    }

    if (slIterNext(iter)) {
        PyErr_SetString(PyExc_TypeError,
                        "unable to advance the internal iterator");
        return -1;
    }

    *obj = PyTuple_GET_ITEM(item, 0);
    return 0;
}

static PyObject *
skipdictiter_next(SkipDictIterObject *it)
{
    if (it->length == 0) {
        PyErr_SetString(PyExc_StopIteration, "");
        return NULL;
    }

    double score;
    PyObject *key;

    int err = skipdictiter_fetch(it->iter, &score, &key);
    if (err) {
        return NULL;
    }

    it->length--;

    return iterators[it->type](score, key);
}

static PyObject *
skipdictiter_next_key(double score, PyObject* key)
{
    Py_INCREF(key);
    return key;
}

static PyObject *
skipdictiter_next_value(double score, PyObject* key)
{
    return (PyObject *) PyFloat_FromDouble(score);
}

static PyObject *
skipdictiter_next_item(double score, PyObject* key)
{
    Py_INCREF(key);
    return Py_BuildValue("(OO)",
                         key,
                         (PyObject *) PyFloat_FromDouble(score));
}

static void
skipdictiter_dealloc(SkipDictIterObject *it)
{
    PyObject_GC_UnTrack(it);
    slIterDel(it->iter);
    Py_XDECREF(it->skipdict);
    PyObject_GC_Del(it);
}

static int
skipdictiter_traverse(SkipDictIterObject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->skipdict);
    return 0;
}

static int
skipdict_delitem(SkipDictObject *self, PyObject *key, int delete)
{
    PyObject* item = PyDict_GetItem(self->mapping, key);
    if (!item) goto Fail;
    PyObject* value = PyTuple_GET_ITEM(item, 1);
    if (delete) PyDict_DelItem(self->mapping, key);
    if (!slDelete(self->skiplist, PyFloat_AsDouble(value), (void*) item, 0)) {
        goto Fail;
    }
    return 0;
 Fail:
    PyErr_SetObject(PyExc_KeyError, key);
    return -1;
}

static int
skipdict_insertobj(SkipDictObject *self, PyObject *key, PyObject *value,
                   int mode)
{
    PyObject *item, *previous;
    double score = PyFloat_AsDouble(value);
    double s;
    int level;

    if (!PyNumber_Check(value)) {
        PyObject *repr = PyObject_Repr(value);
        PyErr_Format(PyExc_TypeError,
                     "not a number: %s",
                     PyString_AsString(repr));
        Py_DECREF(repr);
        return -1;
    }

    if (score == -1.0 && PyErr_Occurred()) {
        return -1;
    }

    if (mode > 0) {
        if ((item = PyDict_GetItem(self->mapping, key))) {
            previous = PyTuple_GET_ITEM(item, 1);
            s = PyFloat_AsDouble(previous);

            if (mode == 1) {
                score -= s;
            } else {
                value = PyNumber_Add(value, previous);
                if (!value) return -1;
            }

            switch (slDelete(self->skiplist, s, (void*) item, score)) {
                case 0:
                    return -1;
                case 1:
                    break;
                case 2:
                    PyTuple_SetItem(item, 1, value);
                    Py_INCREF(value);
                    return 0;
            }

            score += s;
        }
    }

    item = PyTuple_Pack(2, key, value);
    PyDict_SetItem(self->mapping, key, item);
    Py_DECREF(item);

    if (self->random) {
        PyObject* result = PyObject_CallFunction(self->random, "i",
                                                 self->skiplist->maxlevel);
        level = (int) PyInt_AsLong(result);
        if (level == -1 && PyErr_Occurred()) {
            PyErr_Format(PyExc_TypeError,
                         "not an integer: %s",
                         PyString_AsString(PyObject_Repr(result)));
            Py_XDECREF(result);
            return -1;
        }
        Py_XDECREF(result);
        if (level > self->skiplist->maxlevel) {
            PyErr_Format(PyExc_ValueError,
                         "level not in range (0-%d): %d",
                         self->skiplist->maxlevel,
                         level);
            return -1;
        }
    } else {
        level = 1;
        while((random() & 0xffff) < (P * 0xffff))
            level += 1;
        level = (level < self->skiplist->maxlevel) \
            ? level : self->skiplist->maxlevel;
    }
    slInsert(self->skiplist, score, (void*) item, level);
    return 0;
}

static int
skipdict_insertseq(SkipDictObject *self, PyObject *seq)
{
    PyObject *it = NULL;
    Py_ssize_t i;
    PyObject *item = NULL;
    PyObject *fast = NULL;
    PyObject *ref = NULL;

    assert(seq != NULL);

    if (PyDict_Check(seq)) {
        seq = ref = PyDict_Items(seq);
    }

    if (seq != Py_None && !(PySequence_Check(seq) || PyIter_Check(seq))) {
        PyErr_SetString(PyExc_TypeError,
                        "expected dict, item sequence or iterable");
        goto Fail;
    }

    it = PyObject_GetIter(seq);
    if (it == NULL) goto Fail;

    for (i = 0; ; ++i) {
        PyObject *key, *value;
        Py_ssize_t n;

        fast = NULL;
        item = PyIter_Next(it);

        if (item == NULL) {
            if (PyErr_Occurred())
                goto Fail;
            break;
        }

        /* Convert item to sequence, and verify length 2. */
        fast = PySequence_Fast(item, "");
        if (fast == NULL) {
            if (PyErr_ExceptionMatches(PyExc_TypeError))
                PyErr_Format(PyExc_TypeError,
                    "cannot convert sequence element #%zd to a sequence",
                    i);
            goto Fail;
        }
        n = PySequence_Fast_GET_SIZE(fast);
        if (n != 2) {
            PyErr_Format(PyExc_ValueError,
                         "dictionary update sequence element #%zd "
                         "has length %zd; 2 is required",
                         i, n);
            goto Fail;
        }

        /* Update/merge with this (key, value) pair. */
        key = PySequence_Fast_GET_ITEM(fast, 0);
        value = PySequence_Fast_GET_ITEM(fast, 1);

        if (skipdict_insertobj(self, key, value, 2)) {
            goto Fail;
        }

        Py_DECREF(fast);
        Py_DECREF(item);
    }

    i = 0;
    goto Return;
Fail:
    Py_XDECREF(item);
    Py_XDECREF(fast);
    Py_XDECREF(ref);
    i = -1;
Return:
    Py_XDECREF(it);
    return Py_SAFE_DOWNCAST(i, Py_ssize_t, int);
}

static PyObject *
skipdict_change(SkipDictObject *self, PyObject *args)
{
    PyObject *key, *change;
    if (!PyArg_ParseTuple(args, "OO:change", &key, &change)) {
        return NULL;
    }

    if (skipdict_insertobj(self, key, change, 2)) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static int
skipdict_setup(SkipDictObject *self, int maxlevel,
               PyObject *rnd, PyObject *seq)
{
    int err = 0;
    self->random = rnd;
    Py_XINCREF(rnd);
    self->mapping = NULL;
    self->skiplist = NULL;
    self->skiplist = slCreate(maxlevel);
    if (!self->skiplist) {
        return -1;
    }

    self->mapping = PyDict_New();

    if (seq) {
        err = skipdict_insertseq(self, seq);
        if (err) {
            return -1;
        }
    }

    return 0;
}

static int
skipdict_init(SkipDictObject *self, PyObject *args, PyObject *kw)
{
    int maxlevel = MAXLEVEL;
    PyObject *rnd = NULL;
    PyObject *seq = NULL;
    static char *kwlist[] = {
        "sequence", "maxlevel", "random", NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|OiO:SkipDict", kwlist,
                                     &seq, &maxlevel, &rnd)) {
        return -1;
    }
    return skipdict_setup(self, maxlevel, rnd, seq);
}

static void
skipdict_dealloc(SkipDictObject* self)
{
    slFree(self->skiplist);
    if (self->mapping) {
        PyDict_Clear(self->mapping);
        Py_DECREF(self->mapping);
    }
    Py_XDECREF(self->random);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
skipdict_iterator(SkipDictObject *self, skiplistiter *iter, itertype type, Py_ssize_t length)
{
    SkipDictIterObject *it = NULL;
    if (!skipdict_Check(self)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!iter) {
        iter = slIterNewFromHead(self->skiplist);
    }

    if (!iter) {
        return NULL;
    }

    it = PyObject_GC_New(SkipDictIterObject, &SkipDictIterType);
    if (!it) {
        slIterDel(iter);
        return NULL;
    }

    Py_INCREF(self);

    it->skipdict = self;
    it->iter = iter;
    it->type = type;
    it->length = length > 1 ? (unsigned long) length : slLength(self->skiplist);
    PyObject_GC_Track(it);

    return (PyObject *)it;
}

static PyObject *
skipdict_iter(SkipDictObject *skipdict)
{
    return skipdict_iterator(skipdict, NULL, KEY, -1);
}

static PyObject *
skipdictiter_item(SkipDictIterObject *self, Py_ssize_t index)
{
    if (index < 0) {
        index = self->length + index;
    }

    index = index % slLength(self->skipdict->skiplist);

    skiplistNode* node = slGetNodeByRank(self->iter->node,
                                         self->skipdict->skiplist->level - 1,
                                         index);

    if (!node) {
        PyErr_SetObject(PyExc_IndexError,
                        PyString_FromString("index out of range"));
        return NULL;
    }

    PyObject *obj = PyTuple_GET_ITEM(node->obj, 0);
    return iterators[self->type](node->score, obj);
}

static PyObject *
skipdictiter_slice(SkipDictIterObject *self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        PyObject *i, *result;
        i = PyNumber_Index(item);
        if (!i)
            return NULL;
        long index = PyLong_AsLong(i);
        Py_DECREF(i);
        result = skipdictiter_item(self, index);
        return result;
    }

    if (!PySlice_Check(item)) {
        PyErr_Format(PyExc_TypeError,
                     "range indices must be integers or slices, not %.200s",
                     Py_TYPE(item)->tp_name);
        return NULL;
    }

    Py_ssize_t ilow;
    Py_ssize_t ihigh;
    Py_ssize_t step;
    Py_ssize_t length;

    if (PySlice_GetIndicesEx((PySliceObject *)item,
                             slLength(self->skipdict->skiplist),
                             &ilow, &ihigh, &step, &length)) {
        return NULL;
    }

    if (step != 1) {
        PyErr_Format(PyExc_ValueError,
                     "slice step %d not supported",
                     (int) step);
        return NULL;
    }


    skiplistNode* node = slGetNodeByRank(self->iter->node,
                                         self->skipdict->skiplist->level - 1,
                                         ilow);

    skiplistiter *iter = slIterNew(self->skipdict->skiplist, node);

    if (!iter) {
        return NULL;
    }

    PyObject *it = skipdict_iterator(self->skipdict, iter, self->type, ihigh - ilow);
    if (!it) {
        slIterDel(iter);
    }

    return it;
}

static Py_ssize_t
skipdict_length(SkipDictObject *self)
{
    Py_ssize_t len = slLength(self->skiplist);
    return len;
}

static int
skipdict_contains(SkipDictObject *self, PyObject *key)
{
    return PyDict_Contains(self->mapping, key);
}

static PyObject *
skipdict_getitem(SkipDictObject *self, PyObject *key)
{
    PyObject *item = PyDict_GetItem(self->mapping, key);
    if (!item) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    PyObject* value = PyTuple_GET_ITEM(item, 1);
    Py_INCREF(value);
    return value;
}

static int
skipdict_ass_sub(SkipDictObject *self, PyObject *key, PyObject *value)
{
    if (value) {
        if (skipdict_insertobj(self, key, value, 1)) return -1;
    } else {
        if (skipdict_delitem(self, key, 1)) return -1;
    }
    return 0;
}

static PyObject *
skipdict_get(SkipDictObject *self, PyObject *args)
{
    PyObject *key, *value;
    PyObject *failobj = Py_None;

    if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &failobj))
        return NULL;

    PyObject *item = PyDict_GetItem(self->mapping, key);
    if (item) {
        value = PyTuple_GET_ITEM(item, 1);
    } else {
        value = failobj;
    }

    Py_INCREF(value);
    return value;
}

PyObject *
skipdict_setdefault(SkipDictObject *self, PyObject *args)
{
    PyObject *key, *value;
    PyObject *defaultobj = Py_None;

    if (!PyArg_UnpackTuple(args, "setdefault", 1, 2, &key, &defaultobj))
        return NULL;

    PyObject *item = PyDict_GetItem(self->mapping, key);
    if (!item) {
        if (skipdict_insertobj(self, key, defaultobj, 0)) {
            return NULL;
        }
        value = defaultobj;
    } else {
        value = PyTuple_GET_ITEM(item, 1);
    }

    Py_XINCREF(value);
    return value;
}

static PyObject *
skipdict_iterator_from_range(SkipDictObject *self,
                             PyObject *args, PyObject *kw,
                             itertype type)
{
    PyObject *min = NULL;
    PyObject *max = NULL;

    static char *kwlist[] = {"min", "max", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|OO:SkipDict", kwlist,
                                     &min, &max)) {
        return NULL;
    }

    PyObject *it = NULL;
    skiplistiter *iter;

    if (!self->skiplist->tail) {
        it = PyTuple_New(0);
        return PyObject_GetIter(it);
    }

    if (!(min || max)) {
        iter = slIterNewFromHead(self->skiplist);
    } else {
        double dmin = self->skiplist->header->level[0].forward->score;
        double dmax = self->skiplist->tail->score;

        float_Convert(dmin, min);
        float_Convert(dmax, max);

        iter = slIterNewFromRange(self->skiplist, dmin, dmax);
    }

    if (!iter) {
        return NULL;
    }
    it = skipdict_iterator(self, iter, type, -1);
    if (!it) {
        slIterDel(iter);
    }
    return it;
}

static PyObject *
skipdict_keys(SkipDictObject *self, PyObject *args, PyObject *kw)
{
    return skipdict_iterator_from_range(self, args, kw, KEY);
}

static PyObject *
skipdict_values(SkipDictObject *self, PyObject *args, PyObject *kw)
{
    return skipdict_iterator_from_range(self, args, kw, VALUE);
}

static PyObject *
skipdict_items(SkipDictObject *self, PyObject *args, PyObject *kw)
{
    return skipdict_iterator_from_range(self, args, kw, ITEM);
}

static PyObject *
skipdict_repr(SkipDictObject *self)
{
    Py_ssize_t i;
    PyObject *s, *temp, *colon = NULL;
    PyObject *pieces = NULL, *result = NULL;
    PyObject *key, *value;
    SkipDictIterObject * it = NULL;
    double score;

    i = Py_ReprEnter((PyObject *)self);
    if (i != 0) {
        return i > 0 ? PyString_FromString("{...}") : NULL;
    }

    if (slLength(self->skiplist) == 0) {
        result = PyString_FromString("{}");
        goto Done;
    }

    pieces = PyList_New(0);
    if (pieces == NULL)
        goto Done;

    colon = PyString_FromString(": ");
    if (colon == NULL)
        goto Done;

    it = (SkipDictIterObject* ) skipdict_iterator(self, NULL, ITEM, -1);

    /* Do repr() on each key+value pair, and insert ": " between them.
       Note that repr may mutate the dict. */
    i = 0;
    while (!skipdictiter_fetch(it->iter, &score, &key)) {
        value = (PyObject *) PyFloat_FromDouble(score);
        int status;
        s = PyObject_Repr(key);
        PyString_Concat(&s, colon);
        PyString_ConcatAndDel(&s, PyObject_Repr(value));
        Py_DECREF(value);
        if (s == NULL)
            goto Done;
        status = PyList_Append(pieces, s);
        Py_DECREF(s);  /* append created a new ref */
        if (status < 0)
            goto Done;
    }

    PyErr_Clear();

    /* Add "{}" decorations to the first and last items. */
    assert(PyList_GET_SIZE(pieces) > 0);
    s = PyString_FromString("{");
    if (s == NULL)
        goto Done;

    temp = PyList_GET_ITEM(pieces, 0);
    PyString_ConcatAndDel(&s, temp);
    PyList_SET_ITEM(pieces, 0, s);
    if (s == NULL)
        goto Done;

    s = PyString_FromString("}");
    if (s == NULL)
        goto Done;
    temp = PyList_GET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1);
    PyString_ConcatAndDel(&temp, s);
    PyList_SET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1, temp);
    if (temp == NULL)
        goto Done;

    /* Paste them all together with ", " between. */
    s = PyString_FromString(", ");
    if (s == NULL)
        goto Done;
    result = _PyString_Join(s, pieces);
    Py_DECREF(s);
Done:
    Py_XDECREF(pieces);
    Py_XDECREF(colon);
    Py_XDECREF(it);
    Py_ReprLeave((PyObject *)self);
    return result;
}

static PyObject *
skipdict_maxlevel(SkipDictObject *self)
{
    return PyInt_FromLong(self->skiplist->maxlevel);
}

static PyObject *
skipdictiter_repr(SkipDictIterObject *self)
{
    char* min = double_AsString(self->iter->min);
    char* max = double_AsString(self->iter->max);
    PyObject *repr = PyString_FromFormat("<SkipDictIterator "
                                         "type=\"%s\" "
                                         "forward=%s "
                                         "min=%s "
                                         "max=%s "
                                         "at %p>",
                                         iterator_names[self->type],
                                         booleans[self->iter->forward],
                                         min,
                                         max,
                                         self->skipdict);
    PyMem_Free(min);
    PyMem_Free(max);
    return repr;
}

static PyObject *
skipdict_richcompare(SkipDictObject *self, PyObject* other, int op)
{
    PyObject *mapping;
    if (PyDict_Check(other)) {
        mapping = other;
    } else {
        if (!skipdict_Check(other)) {
            PyObject *repr = PyObject_Repr(other);
            PyErr_Format(PyExc_TypeError,
                         "can't compare with %s",
                         PyString_AsString(repr));
            Py_DECREF(repr);
            return NULL;
        }
        SkipDictObject* obj = (SkipDictObject*) other;
        mapping = obj->mapping;
    }
    return PyDict_Type.tp_richcompare(self->mapping, mapping, op);
}

static PyMethodDef skipdict_methods[] = {
    {"get", (PyCFunction)skipdict_get, METH_VARARGS, NULL},
    {"setdefault", (PyCFunction)skipdict_setdefault, METH_VARARGS, NULL},
    {"keys", (PyCFunction)skipdict_keys, METH_VARARGS | METH_KEYWORDS, NULL},
    {"values", (PyCFunction)skipdict_values, METH_VARARGS | METH_KEYWORDS, NULL},
    {"items", (PyCFunction)skipdict_items, METH_VARARGS | METH_KEYWORDS, NULL},
    {"index", (PyCFunction)skipdict_index, METH_O, NULL},
    {"change", (PyCFunction)skipdict_change, METH_VARARGS, NULL},
    {NULL}
};

static PyGetSetDef skipdict_getset[] = {
    {"maxlevel", (getter)skipdict_maxlevel, NULL, "maxlevel", NULL},
    {NULL}
};

static PyMappingMethods mapping = {
    (lenfunc)skipdict_length,              /*mp_length*/
    (binaryfunc)skipdict_getitem,          /*mp_subscript*/
    (objobjargproc)skipdict_ass_sub,       /*mp_ass_subscript*/
};

static PySequenceMethods sequence = {
    (lenfunc)skipdict_length,              /*sq_length*/
    0,                                     /*sq_concat*/
    0,                                     /*sq_repeat*/
    0,                                     /*sq_item*/
 	0,                                     /*sq_slice*/
    0,                                     /*sq_ass_item*/
    0,                                     /*sq_ass_slice*/
    (objobjproc)skipdict_contains,         /*sq_contains*/
    0,                                     /*sq_inplace_concat*/
    0,                                     /*sq_inplace_repeat*/
};

static PyTypeObject SkipDictType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "skipdict.SkipDict",                   /* tp_name */
    sizeof(SkipDictObject),                /* tp_basicsize */
    0,                                     /* tp_itemsize */
    (destructor)skipdict_dealloc,          /* tp_dealloc */
    0,                                     /* tp_print */
    0,                                     /* tp_getattr */
    0,                                     /* tp_setattr */
    0,                                     /* tp_compare */
    (reprfunc)skipdict_repr,               /* tp_repr */
    0,                                     /* tp_as_number */
    &sequence,                             /* tp_as_sequence*/
    &mapping,                              /* tp_as_mapping */
    (hashfunc)PyObject_HashNotImplemented, /* tp_hash */
    0,                                     /* tp_call */
    0,                                     /* tp_str */
    0,                                     /* tp_getattro */
    0,                                     /* tp_setattro */
    0,                                     /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,/* tp_flags */
    0,                                     /* tp_doc */
    0,		                               /* tp_traverse */
    0,		                               /* tp_clear */
    (richcmpfunc)skipdict_richcompare,     /* tp_richcompare */
    0,		                               /* tp_weaklistoffset */
    (getiterfunc)skipdict_iter,            /* tp_iter */
    0,		                               /* tp_iternext */
    skipdict_methods,                      /* tp_methods */
    0,                                     /* tp_members */
    skipdict_getset,                       /* tp_getset */
    0,                                     /* tp_base */
    0,                                     /* tp_dict */
    0,                                     /* tp_descr_get */
    0,                                     /* tp_descr_set */
    0,                                     /* tp_dictoffset */
    (initproc)skipdict_init,               /* tp_init */
    PyType_GenericAlloc,                   /* tp_alloc */
    PyType_GenericNew,                     /* tp_new */
    PyObject_Del,                          /* tp_free */
    0,                                     /* tp_is_gc */
};

static PyMappingMethods skipdictiter_as_mapping = {
    0,                                     /*mp_length*/
    (binaryfunc)skipdictiter_slice,        /*mp_subscript*/
    0,                                     /*mp_ass_subscript*/
};

static PyTypeObject SkipDictIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "skipdict.SkipDictIterator",            /* tp_name */
    sizeof(SkipDictIterObject),             /* tp_basicsize */
    0,                                      /* tp_itemsize */
    /* methods */
    (destructor)skipdictiter_dealloc,       /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    (reprfunc)skipdictiter_repr,            /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    &skipdictiter_as_mapping,               /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    PyObject_GenericGetAttr,                /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                      /* tp_doc */
    (traverseproc)skipdictiter_traverse,    /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    PyObject_SelfIter,                      /* tp_iter */
    (iternextfunc)skipdictiter_next,        /* tp_iternext */
    0,                                      /* tp_methods */
};

static PyMethodDef methods[] = {
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "skipdict",
        NULL,
        0,
        methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyObject *
PyInit_skipdict(void)
#else
void
initskipdict(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("skipdict", methods);
#endif

    if (module == NULL)
        INITERROR;

    PyType_Prepare(module, "SkipDict", &SkipDictType);
    PyType_Prepare(module, "SkipDictIterator", &SkipDictIterType);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
/* EOF */
