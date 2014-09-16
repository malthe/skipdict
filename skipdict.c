#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <Python.h>
#include "skiplist.h"

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
#define PyInt_FromLong PyLong_FromLong
#define PyString_FromString PyUnicode_FromString
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize
#define PyString_AsString PyUnicode_AsASCIIString
#define _PyString_Join PyUnicode_Join
#define MOD_ERROR_VAL NULL
#define MOD_SUCCESS_VAL(val) val
#define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#define MOD_DEF(ob, name, doc, methods)                       \
    static struct PyModuleDef moduledef = {                   \
        PyModuleDef_HEAD_INIT, name, doc, -1, methods, };     \
    ob = PyModule_Create(&moduledef);
#else
#define MOD_ERROR_VAL
#define MOD_SUCCESS_VAL(val)
#define MOD_INIT(name) init##name(void)
#define MOD_DEF(ob, name, doc, methods)             \
    ob = Py_InitModule3(name, methods, doc);
#endif

#define float_Convert(op, obj)                                  \
    if (obj == Py_None) obj = NULL;                             \
    if (obj) op = PyFloat_AsDouble(obj);                        \
    if (op == -1 && PyErr_Occurred()) {                         \
    PyErr_Format(PyExc_TypeError,                               \
                 "not a number: %s",                            \
                 PyString_AsString(PyObject_Repr(obj)));        \
    return NULL;                                                \
    }

#define PyType_Prepare(mod, name, type)                 \
    if (PyType_Ready(type) < 0) {                       \
        return NULL;                                         \
    }                                                   \
    Py_INCREF(type);                                    \
    PyModule_AddObject(mod, name, (PyObject *)type);

#define skiplist_Check(op) PyObject_TypeCheck(op, &SkipDictType)
#define skiplist_CheckExact(op) (Py_TYPE(op) == &SkipDictType)

static PyTypeObject SkipDictType;
static PyTypeObject SkipDictItemIterType;
static PyTypeObject SkipDictIterType;
static PyTypeObject SkipDictIndexType;

typedef struct {
    PyObject_HEAD
    skiplist *skiplist;
    PyObject *compare;
    PyObject *random;
    PyObject *mapping;
} SkipListObject;

typedef struct {
    PyObject_HEAD
    SkipListObject *so;
    skiplistiter *iter;
} SkipListIterObject;

typedef struct {
    PyObject_HEAD
    SkipListObject *so;
} SkipListIndexObject;

static void
skiplistindex_dealloc(SkipListIndexObject *index)
{
    Py_XDECREF(index->so);
    PyObject_GC_Del(index);
}

static PyObject *
skiplistindex_call(SkipListIndexObject *self, PyObject *args)
{
    PyObject *key = NULL;
    if (!PyArg_UnpackTuple(args, "call", 1, 1, &key)) {
        return NULL;
    }

    PyObject* value = PyDict_GetItem(self->so->mapping, key);
    if (!value) {
        PyErr_SetObject(PyExc_KeyError,
                        PyObject_Repr(key));
        return NULL;
    }
    double score = PyFloat_AsDouble(value);
    if (score == -1.0 && PyErr_Occurred()) return 0;

    void *obj = (void*) key;
    unsigned long rank = slGetRank(self->so->skiplist, score, obj);
    if (!rank) {
        return NULL;
    }

    return PyLong_FromLong(rank - 1);
}

static PyObject *
skiplistindex_item(SkipListIndexObject *self, Py_ssize_t index)
{
    unsigned long rank = index + 1;
    skiplistNode* node = slGetNodeByRank(self->so->skiplist, rank);
    if (!node) {
        PyErr_SetObject(PyExc_IndexError,
                        PyString_FromString("Index out of range"));
        return NULL;
    }

    PyObject *obj = (PyObject*) node->obj;
    Py_INCREF(obj);
    return obj;
}

static int
skiplistindex_traverse(SkipListIndexObject *index, visitproc visit,
                       void *arg)
{
    Py_VISIT(index->so);
    return 0;
}

static int
skiplistiter_fetch(SkipListIterObject *it,
                   PyObject **key, PyObject **value)
{
    double score = 0;
    const void *obj = NULL;
    if (slIterGet(it->iter, &score, &obj)) {
        PyErr_SetString(PyExc_StopIteration, "");
        return -1;
    }
    if (slIterNext(it->iter)) {
        PyErr_SetString(PyExc_TypeError,
                        "Unable to advance the internal iterator");
        return -1;
    }
    *key = (PyObject *) obj;
    if (value) {
        *value = (PyObject *) PyFloat_FromDouble(score);
    }
    return 0;
}

static PyObject *
skiplistiter_next(SkipListIterObject *it)
{
    PyObject *key = NULL;
    int err = skiplistiter_fetch(it, &key, NULL);
    if (err) {
        return NULL;
    }
    Py_INCREF(key);
    return key;
}

static PyObject *
skiplistiter_next_item(SkipListIterObject *it)
{
    PyObject *key = NULL;
    PyObject *value = NULL;
    int err = skiplistiter_fetch(it, &key, &value);
    if (err) {
        return NULL;
    }
    Py_INCREF(key);
    Py_INCREF(value);
    return Py_BuildValue("(OO)", key, value);
}

static PyObject *
skiplistiter_next_value(SkipListIterObject *it)
{
    PyObject *key = NULL;
    PyObject *value = NULL;
    int err = skiplistiter_fetch(it, &key, &value);
    if (err) {
        return NULL;
    }
    Py_INCREF(value);
    return value;
}

static void
skiplistiter_dealloc(SkipListIterObject *it)
{
    PyObject_GC_UnTrack(it);
    slIterDel(it->iter);
    Py_XDECREF(it->so);
    PyObject_GC_Del(it);
}

static int
skiplistiter_traverse(SkipListIterObject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->so);
    return 0;
}

static int
skiplist_delitem(SkipListObject *self, PyObject *key, int delete)
{
    PyObject* old = PyDict_GetItem(self->mapping, key);
    if (!old) goto Fail;
    if (delete) PyDict_DelItem(self->mapping, key);
    return (!slDelete(self->skiplist, PyFloat_AsDouble(old), key));
 Fail:
    PyErr_SetObject(PyExc_KeyError,
                    PyObject_Repr(key));
    return -1;
}

static int
skiplist_insertobj(SkipListObject *self, PyObject *key, PyObject *value,
                   int check)
{
    if (!PyNumber_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "not a number: %s",
                     PyString_AsString(PyObject_Repr(value)));
        return -1;
    }

    double score = PyFloat_AsDouble(value);

    if (score == -1.0 && PyErr_Occurred()) {
        return -1;
    }

    if (check) {
        skiplist_delitem(self, key, 0);
        PyErr_Clear();
    }

    PyDict_SetItem(self->mapping, key, value);
    Py_INCREF(key);
    Py_INCREF(value);

    void *obj = (void*) key;
    slInsert(self->skiplist, score, obj);

    return 0;
}

static int
skiplist_insertseq(SkipListObject *self, PyObject *seq)
{
    PyObject *it;
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
                        "Expected dict, item sequence or iterable.");
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

        if (skiplist_insertobj(self, key, value, 0)) {
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

static int
skiplist_setup(SkipListObject *self, int maxlevel,
               PyObject *cmp, PyObject *rnd, PyObject *seq)
{
    int err = 0;
    self->compare = cmp;
    self->random = rnd;
    self->mapping = NULL;
    self->skiplist = NULL;
    Py_XINCREF(self->compare);
    Py_XINCREF(self->random);
    self->skiplist = slCreate(maxlevel);
    if (!self->skiplist) {
        err = -1;
        goto Fail;
    }

    self->mapping = PyDict_New();

    if (seq) {
        err = skiplist_insertseq(self, seq);
        if (err) {
            goto Fail;
        }
    }
    return 0;
Fail:
    Py_XDECREF(self->compare);
    Py_XDECREF(self->random);
    Py_XDECREF(self->mapping);
    return err;
}

static int
skiplist_init(SkipListObject *self, PyObject *args, PyObject *kw)
{
    int maxlevel = MAXLEVEL;
    PyObject *cmp = NULL;
    PyObject *rnd = NULL;
    PyObject *seq = NULL;
    static char *kwlist[] = {
        "sequence", "maxlevel", "compare", "random", NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|OiOO:SkipList", kwlist,
                                     &seq, &maxlevel, &cmp, &rnd)) {
        return -1;
    }
    return skiplist_setup(self, maxlevel, cmp, rnd, seq);
}

static void
skiplist_dealloc(SkipListObject* self)
{
    slFree(self->skiplist);
    Py_XDECREF(self->compare);
    Py_XDECREF(self->random);
    Py_XDECREF(self->mapping);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
skiplist_iterator(SkipListObject *self, skiplistiter *iter)
{
    SkipListIterObject *it = NULL;
    if (!skiplist_Check(self)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    it = PyObject_GC_New(SkipListIterObject, &SkipDictIterType);
    if (!it) return NULL;
    Py_INCREF(self);
    it->so = self;
    it->iter = iter;
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static PyObject *
skiplist_iter(SkipListObject *so)
{
    PyObject *it = NULL;
    skiplistiter *iter = slIterNewFromHead(so->skiplist);
    if (!iter) {
        return NULL;
    }
    it = skiplist_iterator(so, iter);
    if (!it) {
        slIterDel(iter);
    }
    return it;
}

static PyObject *
skiplistindex_iter(SkipListIndexObject *self)
{
    return skiplist_iter(self->so);
}

static PyObject *
skipdict_iterator(SkipListObject *self, skiplistiter *iter)
{
    SkipListIterObject *it = NULL;
    if (!skiplist_Check(self)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    it = PyObject_GC_New(SkipListIterObject, &SkipDictItemIterType);
    if (it == NULL) {
        return NULL;
    }
    Py_INCREF(self);
    it->so = self;
    it->iter = iter;
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static PyObject *
skipdict_iter(SkipListObject *so)
{
    PyObject *it = NULL;
    skiplistiter *iter = slIterNewFromHead(so->skiplist);
    if (!iter) {
        return NULL;
    }
    it = skipdict_iterator(so, iter);
    if (!it) {
        slIterDel(iter);
    }
    return it;
}


static PyObject *
skiplistindex_slice(SkipListIndexObject *self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        PyObject *i, *result;
        i = PyNumber_Index(item);
        if (!i)
            return NULL;
        long index = PyLong_AsLong(i);
        result = skiplistindex_item(self, index);
        Py_DECREF(i);
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

    if (PySlice_GetIndicesEx(item, self->so->skiplist->length,
                             &ilow, &ihigh, &step, &length)) {
        return NULL;
    }

    skiplistNode* low = slGetNodeByRank(self->so->skiplist, ilow + 1);
    if (!low) {
        low = self->so->skiplist->header->level[0].forward;
    }

    skiplistNode* high = slGetNodeByRank(self->so->skiplist, ihigh);
    if (!high) {
        high = self->so->skiplist->tail;
    }

    skiplistiter *iter = slIterNewFromRange(self->so->skiplist,
                                            low->score, high->score);

    if (!iter) {
        return NULL;
    }

    PyObject *it = skiplist_iterator(self->so, iter);
    if (!it) {
        slIterDel(iter);
    }
    return it;
}

static PyObject *
skipdict_iteritems(SkipListObject *self, PyObject *args, PyObject *kw)
{
    PyObject *min = NULL;
    PyObject *max = NULL;

    static char *kwlist[] = {"min", "max", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|OO:SkipList", kwlist,
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
    it = skipdict_iterator(self, iter);
    if (!it) {
        slIterDel(iter);
    }
    return it;
}

static PyObject *
skiplist_repr(SkipListObject *self)
{
    Py_ssize_t i;
    PyObject *s, *temp, *colon = NULL;
    PyObject *pieces = NULL, *result = NULL;
    PyObject *key, *value;

    i = Py_ReprEnter((PyObject *)self);
    if (i != 0) {
        return i > 0 ? PyString_FromString("{...}") : NULL;
    }

    if (self->skiplist->length == 0) {
        result = PyString_FromString("{}");
        goto Done;
    }

    pieces = PyList_New(0);
    if (pieces == NULL)
        goto Done;

    colon = PyString_FromString(": ");
    if (colon == NULL)
        goto Done;

    SkipListIterObject *iter = (SkipListIterObject* ) skipdict_iter(self);

    /* Do repr() on each key+value pair, and insert ": " between them.
       Note that repr may mutate the dict. */
    i = 0;
    while (!skiplistiter_fetch(iter, &key, &value)) {
        int status;
        /* Prevent repr from deleting value during key format. */
        Py_INCREF(value);
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
    Py_ReprLeave((PyObject *)self);
    return result;
}

static Py_ssize_t
skiplist_length(SkipListObject *self)
{
    Py_ssize_t len = slLength(self->skiplist);
    return len;
}

static Py_ssize_t
skiplistindex_length(SkipListIndexObject *self)
{
    return skiplist_length(self->so);
}

static int
skiplist_contains(SkipListObject *self, PyObject *key)
{
    return PyDict_Contains(self->mapping, key);
}

static PyObject *
skiplist_index(SkipListObject *self)
{
    SkipListIndexObject *index;

    if (!skiplist_Check(self)) {
        PyErr_Format(PyExc_TypeError,
                     "%s() requires a skipdict argument, not '%s'",
                     SkipDictIndexType.tp_name, Py_TYPE(self)->tp_name);
        return NULL;
    }

    index = PyObject_GC_New(SkipListIndexObject, &SkipDictIndexType);
    if (!index) return NULL;
    Py_INCREF(self);
    index->so = self;
    PyObject_GC_Track(index);
    return (PyObject *)index;
}

static PyObject *
skiplist_getitem(SkipListObject *self, PyObject *key)
{
    PyObject *obj = PyDict_GetItem(self->mapping, key);
    if (!obj) {
        PyErr_SetObject(PyExc_KeyError,
                        PyObject_Repr(key));
        return NULL;
    }

    Py_INCREF(obj);
    return obj;
}

static int
skiplist_ass_sub(SkipListObject *self, PyObject *key, PyObject *value)
{
    if (value) {
        if (skiplist_insertobj(self, key, value, 1)) return -1;
    } else {
        if (skiplist_delitem(self, key, 1)) return -1;
    }
    return 0;
}

static PyObject *
skiplistiter_tolist(SkipListIterObject* it,
                    PyObject* (*next)(SkipListIterObject*))
{
    register PyObject *v;

    PyObject *item;

    v = PyList_New(0);
    if (v == NULL)
        return NULL;

    for (;;) {
        item = next(it);

        if (!item) {
            if (PyErr_Occurred()) {
                if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
                    PyErr_Clear();
                } else {
                    v = NULL;
                }
                goto Return;
            }
            break;
        }

        Py_INCREF(item);
        PyList_Append(v, item);
    }
Return:
    Py_DECREF(it);
    return v;
}

static PyObject *
skiplistiter_items(SkipListIterObject *self)
{
    return skiplistiter_tolist(self, &skiplistiter_next_item);
}

static PyObject *
skiplistiter_values(SkipListIterObject *self)
{
    return skiplistiter_tolist(self, &skiplistiter_next_value);
}

static PyObject *
skiplist_get(SkipListObject *self, PyObject *args)
{
    PyObject *key;
    PyObject *failobj = Py_None;

    if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &failobj))
        return NULL;

    PyObject *value = PyDict_GetItem(self->mapping, key);
    if (!value) {
        value = failobj;
    }

    Py_INCREF(value);
    return value;
}

PyObject *
skiplist_setdefault(SkipListObject *self, PyObject *args)
{
    PyObject *key;
    PyObject *defaultobj = Py_None;

    if (!PyArg_UnpackTuple(args, "setdefault", 1, 2, &key, &defaultobj))
        return NULL;

    PyObject *value = PyDict_GetItem(self->mapping, key);
    if (!value) {
        if (skiplist_insertobj(self, key, defaultobj, 0)) {
            return NULL;
        }
        value = defaultobj;
    }

    Py_XINCREF(value);
    return value;
}


static PyObject *
skiplist_keys(SkipListObject *self)
{
    SkipListIterObject* iter = (SkipListIterObject*) skiplist_iter(self);
    return skiplistiter_tolist(iter, &skiplistiter_next);
}

static PyObject *
skiplist_values(SkipListObject *self)
{
    SkipListIterObject* iter = (SkipListIterObject*) skiplist_iter(self);
    return skiplistiter_tolist(iter, &skiplistiter_next_value);
}

static PyObject *
skiplist_items(SkipListObject *self)
{
    SkipListIterObject* iter = (SkipListIterObject*) skiplist_iter(self);
    return skiplistiter_tolist(iter, &skiplistiter_next_item);
}

static PyObject *
skiplist_maxlevel(SkipListObject *self)
{
    return PyInt_FromLong(self->skiplist->maxlevel);
}

static PyObject *
skiplistindex_repr(SkipListIndexObject *self)
{
    PyObject *keys = skiplist_keys(self->so);
    PyObject *repr = PyObject_Repr(keys);
    Py_DECREF(keys);
    return repr;
}

static PyObject *
skiplist_richcompare(SkipListObject *self, PyObject* other, int op)
{
    PyObject *mapping;
    if (PyDict_Check(other)) {
        mapping = other;
    } else {
        if (!skiplist_Check(other)) {
            PyErr_Format(PyExc_TypeError,
                         "can't compare with %s",
                         PyString_AsString(PyObject_Repr(other)));
        }
        SkipListObject* obj = (SkipListObject*) other;
        mapping = obj->mapping;
    }
    return PyDict_Type.tp_richcompare(self->mapping, mapping, op);
}

static PyMethodDef skiplist_methods[] = {
    {"get", (PyCFunction)skiplist_get, METH_VARARGS, NULL},
    {"setdefault", (PyCFunction)skiplist_setdefault, METH_VARARGS, NULL},
    {"keys", (PyCFunction)skiplist_keys,  METH_NOARGS, NULL},
    {"items", (PyCFunction)skiplist_items,  METH_NOARGS, NULL},
    {"values", (PyCFunction)skiplist_values,  METH_NOARGS, NULL},
    {"iteritems", (PyCFunction)skipdict_iteritems,  METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}
};

static PyGetSetDef skiplist_getset[] = {
    {"maxlevel", (getter)skiplist_maxlevel, NULL, "maximum level", NULL},
    {"index", (getter)skiplist_index, NULL, "index", NULL},
    {NULL}
};

static PyMappingMethods mapping = {
    (lenfunc)skiplist_length,              /*mp_length*/
    (binaryfunc)skiplist_getitem,          /*mp_subscript*/
    (objobjargproc)skiplist_ass_sub,       /*mp_ass_subscript*/
};

static PySequenceMethods sequence = {
    (lenfunc)skiplist_length,              /*sq_length*/
    0,                                     /*sq_concat*/
    0,                                     /*sq_repeat*/
    0,                                     /*sq_item*/
 	0,                                     /*sq_slice*/
    0,                                     /*sq_ass_item*/
    0,                                     /*sq_ass_slice*/
    (objobjproc)skiplist_contains,         /*sq_contains*/
    0,                                     /*sq_inplace_concat*/
    0,                                     /*sq_inplace_repeat*/
};

static PyTypeObject SkipDictType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "skipdict.SkipDict",                   /* tp_name */
    sizeof(SkipListObject),                /* tp_basicsize */
    0,                                     /* tp_itemsize */
    (destructor)skiplist_dealloc,          /* tp_dealloc */
    0,                                     /* tp_print */
    0,                                     /* tp_getattr */
    0,                                     /* tp_setattr */
    0,                                     /* tp_compare */
    (reprfunc)skiplist_repr,               /* tp_repr */
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
    (richcmpfunc)skiplist_richcompare,     /* tp_richcompare */
    0,		                               /* tp_weaklistoffset */
    (getiterfunc)skiplist_iter,            /* tp_iter */
    0,		                               /* tp_iternext */
    skiplist_methods,                      /* tp_methods */
    0,                                     /* tp_members */
    skiplist_getset,                       /* tp_getset */
    0,                                     /* tp_base */
    0,                                     /* tp_dict */
    0,                                     /* tp_descr_get */
    0,                                     /* tp_descr_set */
    0,                                     /* tp_dictoffset */
    (initproc)skiplist_init,               /* tp_init */
    PyType_GenericAlloc,                   /* tp_alloc */
    PyType_GenericNew,                     /* tp_new */
    PyObject_Del,                          /* tp_free */
    0,                                     /* tp_is_gc */
};

static PyMethodDef skiplistiter_methods[] = {
    {"items", (PyCFunction)skiplistiter_items,  METH_NOARGS, NULL},
    {"values", (PyCFunction)skiplistiter_values,  METH_NOARGS, NULL},
    {NULL}
};

static PyTypeObject SkipDictIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "skipdict.SkipDictIterator",            /* tp_name */
    sizeof(SkipListIterObject),             /* tp_basicsize */
    0,                                      /* tp_itemsize */
    /* methods */
    (destructor)skiplistiter_dealloc,       /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    PyObject_GenericGetAttr,                /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                      /* tp_doc */
    (traverseproc)skiplistiter_traverse,    /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    PyObject_SelfIter,                      /* tp_iter */
    (iternextfunc)skiplistiter_next,        /* tp_iternext */
    skiplistiter_methods,                   /* tp_methods */
};

static PyTypeObject SkipDictItemIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "skipdict.SkipDictItemIterator",        /* tp_name */
    sizeof(SkipListIterObject),             /* tp_basicsize */
    0,                                      /* tp_itemsize */
    /* methods */
    (destructor)skiplistiter_dealloc,       /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    PyObject_GenericGetAttr,                /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                      /* tp_doc */
    (traverseproc)skiplistiter_traverse,    /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    PyObject_SelfIter,                      /* tp_iter */
    (iternextfunc)skiplistiter_next_item,   /* tp_iternext */
};

static PySequenceMethods skiplistindex_as_sequence = {
    (lenfunc)skiplistindex_length,          /* sq_length*/
    0,                                      /* sq_concat*/
    0,                                      /* sq_repeat*/
    (ssizeargfunc)skiplistindex_item,       /* sq_item */
    0,                                      /* sq_slice */
    0,                                      /* sq_ass_item*/
    0,                                      /* sq_ass_slice*/
    0,                                      /* sq_contains*/
    0,                                      /* sq_inplace_concat*/
    0,                                      /* sq_inplace_repeat*/
};

static PyMappingMethods skiplistindex_as_mapping = {
    (lenfunc)skiplistindex_length,          /* mp_length */
    (binaryfunc)skiplistindex_slice,        /* mp_subscript */
    (objobjargproc)0,                       /* mp_ass_subscript */
};

static PyTypeObject SkipDictIndexType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "skipdict.SkipDictIndex",                   /* tp_name */
    sizeof(SkipListIndexObject),                /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)skiplistindex_dealloc,          /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)skiplistindex_repr,               /* tp_repr */
    0,                                          /* tp_as_number */
    &skiplistindex_as_sequence,                 /* tp_as_sequence */
    &skiplistindex_as_mapping,                  /* tp_as_mapping */
    0,                                          /* tp_hash */
    (ternaryfunc)skiplistindex_call,            /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)skiplistindex_traverse,       /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)skiplistindex_iter,            /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,
};

static PyMethodDef methods[] = {
    {NULL, NULL, 0, NULL}
};

static char doc[] = "A probabilistic data structure which provides lookups in logarithmic time.";

MOD_INIT(skipdict) {
    PyObject* mod;

    MOD_DEF(mod, "skipdict", doc, methods)

    if (mod == NULL)
        return MOD_ERROR_VAL;

    PyType_Prepare(mod, "SkipDict", &SkipDictType);
    PyType_Prepare(mod, "SkipDictIterator", &SkipDictIterType);
    PyType_Prepare(mod, "SkipDictItemIterator", &SkipDictItemIterType);
    PyType_Prepare(mod, "SkipDictIndex", &SkipDictIndexType);

    return MOD_SUCCESS_VAL(mod);
}

/* EOF */
