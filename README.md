SkipDict
========

A skip dict is a Python dictionary which is permanently sorted by
value. This package provides a fast, idiomatic implementation written
in C – based on Redis' sorted sets.

The data structure uses a [skip
list](http://en.wikipedia.org/wiki/Skip_list) internally.

An example use is a leaderboard where the skip dict provides very fast access to the score of each user, the position and the current top users.


Usage
-----

The skip dict works just like a normal dictionary except its values
must be numbers:

```python
from skipdict import SkipDict

skipdict = SkipDict()
skipdict['foo'] = 1.0
skipdict['bar'] = 2.0
```

The ``skipdict`` is sorted by value which means that iteration and standard mapping protocol methods such as ``keys()``, ``values()`` and ``items()`` return items in sorted order.

Each of these methods have been extended with optional range arguments ``min`` and ``max`` which filter based on value:
```python
>>> skipdict.keys(min=2.0)
['bar']
```

In addition, the ``index`` attribute is a view that provides both key lookup and slicing on position:

```python
>>> skipdict.index(2.0)          # What's the first key that has value 2.0?
'bar'
>>> skipdict.index[-1]           # What key has the highest value?
'bar'
>>> skipdict.index[-1:].items()  # What key and value is highest?
[('bar', 2.0)]
```

License
-------

Copyright (c) 204 Malthe Borch <mborch@gmail.com>

This software is provided "as-is" under the BSD License.



