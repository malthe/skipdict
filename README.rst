SkipDict
========

A skip dict is a Python dictionary which is permanently sorted by
value. This package provides a fast, idiomatic implementation written
in C with an extensive test suite.

The data structure uses a `skip list
<http://en.wikipedia.org/wiki/Skip_list>`_ internally.

An example use is a leaderboard where the skip dict provides
logarithmic access to the score and ranking of each user, as well as
efficient iteration in either direction from any node.

Compatible with Python 2.7+ and Python 3.3+.


Usage
-----

The skip dict works just like a normal dictionary except it maps only
to floating point values::

  from skipdict import SkipDict

  skipdict = SkipDict(maxlevel=4)

  skipdict['foo'] = 1.0
  skipdict['bar'] = 2.0

The ``SkipDict`` optionally takes a dictionary or a sequence of
``(key, value)`` pairs::

  skipdict = SkipDict({'foo': 1.0, 'bar': 2.0})
  skipdict = SkipDict(('foo', 1.0), ('bar', 1.0), ('bar', 1.0))

Note that duplicates are automatically aggregated to a sum. To
illustrate this, we can count the occurrences of letters in a text::

  skipdict = SkipDict(
      (char, 1) for char in
      "Everything popular is wrong. - Oscar Wilde"
  )

  # The most frequent letter is a space.
  skipdict.keys()[-1] == " "

The ``skipdict`` is sorted by value which means that iteration and
standard mapping protocol methods such as ``keys()``, ``values()`` and
``items()`` return items in sorted order.

Each of these methods have been extended with optional range arguments
``min`` and ``max`` which limit iteration based on value. In addition,
the iterator objects support the item and slice protocols:

>>> skipdict.keys(min=2.0)[0]
'bar'
>>> skipdict.keys(max=2.0)[1:]
['bar']

Note that the methods always return an iterator. Use ``list`` to
expand to a sequence:

>>> iterator = skipdict.keys()
>>> list(iterator)
['bar']

The ``index(value)`` method returns the first key that has exactly the
required value. If the value is not found then a ``KeyError``
exception is raised.

>>> skipdict.index(2.0)
'bar'


Alternatives
------------

Francesco Romani wrote
[pyskiplist](https://bitbucket.org/mojaves/pyskiplist) which also
provides an implementation of the skip list datastructure for CPython
written in C.

Paul Colomiets wrote
[sortedsets](https://github.com/tailhook/sortedsets) which is an
implementation in pure Python. The randomized test cases were adapted
from this package.


License
-------

Copyright (c) 204 Malthe Borch <mborch@gmail.com>

This software is provided "as-is" under the BSD License.
