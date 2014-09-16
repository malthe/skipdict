from gc import collect
from unittest import TestCase
from operator import itemgetter
from random import Random, randrange, shuffle
from itertools import combinations


class BaseTestCase(TestCase):
    maxlevel = 6
    items = ()
    reverse = False
    maxDiff = None

    def setUp(self):
        items = self.items if not self.reverse else reversed(self.items)
        self.skipdict = self.make(items, self.maxlevel)

    def tearDown(self):
        collect()

    def make(self, *args, **kwargs):
        from skipdict import SkipDict
        return SkipDict(*args, **kwargs)


class PropertyTestCase(BaseTestCase):
    def test_maxlevel(self):
        self.assertEqual(self.skipdict.maxlevel, self.maxlevel)


class DictTestCase(BaseTestCase):
    items = {'Xe2W0QxllGdCW251l7U9Dg': 150.0}

    def test_length(self):
        self.assertEqual(len(self.skipdict), len(self.items))


class RandomTestCase(BaseTestCase):
    items = (("foo", 1.0), )

    def test_random_function(self):
        called = []

        def random(maxlevel):
            called.append(1)
            return maxlevel // 2

        inst = self.make(self.items, self.maxlevel, random)
        self.assertEqual(list(inst.items()), list(self.items))
        self.assertEqual(called, [1])

    def test_random_wrong_type(self):
        def random(maxlevel):
            return "foo"

        self.assertRaises(
            TypeError,
            self.make,
            self.items,
            self.maxlevel,
            random
        )

    def test_random_out_of_range(self):
        def random(maxlevel):
            return maxlevel + 1

        self.assertRaises(
            ValueError,
            self.make,
            self.items,
            self.maxlevel,
            random
        )


class IterTestCase(BaseTestCase):
    quote = "Everything popular is wrong. - Oscar Wilde"

    @property
    def items(self):
        return ((char, 1) for char in self.quote)

    def test_length(self):
        self.assertEqual(len(self.skipdict), 24)

    def test_least_common(self):
        self.assertTrue(self.quote.count(self.skipdict.keys()[0]), 1)

    def test_second_most_common(self):
        self.assertEqual(self.skipdict.keys()[-2], 'r')

    def test_most_common(self):
        self.assertEqual(self.skipdict.keys()[-1], ' ')


class FixtureTestCase(BaseTestCase):
    items = (
        ('Xe2W0QxllGdCW251l7U9Dg', 150.0),
        ('3HT/SVSdCwM+4ZjtSqHCew', 476.0),
        ('Q2BKuEOFwIojkPsnjmPNFg', 2390.0),
        ('Qjq1fGnHVc5nXvWnbEyXjQ', 3773.0),
        ('wamjIdfm+ajk81fR7gKcAA', 4729.0),
        ('uDUHFv5CtiY/Gm5LOCfGUg', 6143.0),
        ('78l934GXETN68sql2vjP5w', 6487.0),
        ('1LAwgvIO0tEikYySkaSaXw', 7449.0),
        ('Y4mDqz7LfIV4L8h2aUwAkA', 10234.0),
        ('AidPceym19y/lmIdi6JxQQ', 10779.0),
        ('hHqwNSMusq7O895yFkr+rQ', 10789.0),
        ('dg16QiUDC2rgE39FWTSOxg', 11873.0),
        ('sgAdgtQ5wRFGSOZ3xZYHyA', 12273.0),
        ('OACKY0A1ftBbyLvTzyf8lQ', 12776.0),
        ('f+dLA1jK8EFEAHxm1FKUkA', 13079.0),
        ('1uDN4mSmsEQF/o6VNiBl3g', 13147.0),
        ('nNwOvGfk9AH2tIzK8uNdzQ', 16636.0),
        ('tMUZ6A1e/1SKd3ko0FhhBQ', 17933.0),
        ('M77ZQiFlYeU4ySUtVa6XYg', 18570.0),
        ('fY7RKQu8toBxoug4CMmznA', 18711.0),
        ('UaZorA+/GnCL4nmgLs3c/g', 19968.0),
        ('VbXaOsRHqH2CAoNJiYsrqg', 20064.0),
        ('dAr84/axpItIAjjNcVPzHA', 20250.0),
        ('HjzS0QlpofFhDO2iU4mXAw', 20756.0),
        ('ipksmQaeYErYwjZ6ia46TA', 21084.0),
        ('XemDsutAYPpCZ6WY4M//ig', 21609.0),
        ('6U6fbOs8jYVfqWeArQ5HHQ', 22410.0),
        ('QFblGefWYZqFbuSK0SDPRQ', 23267.0),
        ('J13bR75czCiLfeBcIy4XTg', 26556.0),
        ('e6XlDT9h6GVPdfvBOrXW5Q', 26608.0),
        ('/eLYo+GKgAt7I2PrOrFTzQ', 28597.0),
        ('48W/xF+VIQZoyKlhktifMw', 31364.0),
        ('NTUtbi4YOHiNIV6SVrpwyg', 33709.0),
        ('364+KUYYuwlmezv1EvaE0g', 33945.0),
        ('YaD6Ktqw1iIWcFGgMEvMxg', 38248.0),
        ('cJSZfsidFuaMK9jY15g44A', 38668.0),
        ('UeP/HvscsnQXUK37Dyo8/w', 40861.0),
        ('xon2bN9ZToI4LpN4o7M2nQ', 41836.0),
        ('MQKXJCNNtWsRqoGbSaDorw', 47171.0),
        ('LCcqUwfmOFq+VXI2kGBQow', 49311.0),
        ('gMXF4DMHCWBjbgucOqWKQg', 50725.0),
        ('JKHDvGMcLQrR4G3zC2g9ug', 50875.0),
        ('Mp1feZZmnmMPJk8bGv0NaA', 51017.0),
        ('rhZyspOoakQBO9Ses3jl+A', 53781.0),
        ('JB9bMHKHoT+hMVjuBrbqlg', 56409.0),
        ('/DsgGH+7F6Fh2/81SzyXYA', 56512.0),
        ('InjjAuUMGHYUIRdRnkUw2w', 56903.0),
        ('otVFi6DLAO+v7XUAcmKttA', 57114.0),
        ('mVTvHObgjfzvZLOzl/xo2Q', 58550.0),
        ('uU1yLoXCgPtifROhCST0sA', 60267.0))

    keys = list(map(itemgetter(0), items))
    values = list(map(itemgetter(1), items))


class ProtocolTestCase(FixtureTestCase):
    def test_contains(self):
        for key in self.keys:
            self.assertTrue(key in self.skipdict)

    def test_get(self):
        for key, value in self.items:
            self.assertEqual(self.skipdict.get(key), value)

    def test_repr(self):
        self.assertEqual(
            repr(self.skipdict),
            "{" + ", ".join("%r: %r" % item for item in self.items) + "}"
        )

    def test_change(self):
        for i, key in enumerate(self.keys):
            v = 1.0 / (i + 1)
            self.skipdict.change(key, v)
            self.assertEqual(self.skipdict[key], self.values[i] + v)

    def test_change_non_existing(self):
        self.skipdict.change('foo', 5.0)
        self.assertEqual(self.skipdict['foo'], 5.0)

    def test_del_item_concrete(self):
        del self.skipdict['mVTvHObgjfzvZLOzl/xo2Q']

    def test_del_item_in_order(self):
        for key in self.keys:
            del self.skipdict[key]
            self.assertRaises(KeyError, self.skipdict.__getitem__, key)

    def test_del_item_in_reverse_order(self):
        for key in reversed(self.keys):
            del self.skipdict[key]
            self.assertRaises(KeyError, self.skipdict.__getitem__, key)

    def test_del_item_in_random_order(self):
        keys = list(self.keys)
        shuffle(keys)
        for key in keys:
            del self.skipdict[key]
            self.assertRaises(KeyError, self.skipdict.__getitem__, key)

    def test_del_item_missing(self):
        self.assertRaises(
            KeyError, self.skipdict.__delitem__,
            'zI1yPoXCgPtifROhCST0sA'
        )

    def test_set_item(self):
        self.skipdict['zI1yPoXCgPtifROhCST0sA'] = 62365.0
        self.assertEqual(self.skipdict['zI1yPoXCgPtifROhCST0sA'], 62365.0)

    def test_set_item_existing(self):
        for i in range(len(self.keys)):
            v = self.values[-i] + 1
            self.skipdict[self.keys[i]] = v
            self.assertEqual(self.skipdict[self.keys[i]], v)

    def test_set_item_existing_random(self):
        for i in (10, 20, 30, 40, 50, 60, 70):
            rnd = Random(i)
            for j in range(len(self.keys)):
                k = self.keys[rnd.randrange(0, len(self.keys), 1)]
                v = rnd.randrange(self.values[0], self.values[-1], 1)
                self.skipdict[k] = v
                self.assertEqual(self.skipdict[k], v)

    def test_set_many_items(self):
        for i in range(100000):
            key = "key%d" % i
            self.skipdict[key] = i * 2

        for i in reversed(range(10000)):
            key = "key%d" % i
            self.assertEqual(self.skipdict[key], i * 2)

    def test_setdefault(self):
        for key, value in self.items:
            self.assertEqual(self.skipdict.setdefault(key), value)

    def test_setdefault_already_exists(self):
        for key, value in self.items:
            self.assertEqual(self.skipdict.setdefault(key), value)

    def test_iteration(self):
        self.assertEqual(list(self.skipdict), self.keys)

    def test_length(self):
        self.assertEqual(len(self.skipdict), len(self.items))

    def test_index(self):
        for i in range(len(self.items)):
            key, score = self.items[i]
            self.assertEqual(self.skipdict.index(key), i)

    def test_index_missing_key(self):
        self.assertRaises(KeyError, self.skipdict.index, 'foo')

    def test_index_no_args(self):
        self.assertRaises(TypeError, self.skipdict.index)

    def test_random(self):
        # Lets test 7 random insertion orders
        for i in (10, 20, 30, 40, 50, 60, 70):
            rnd = Random(i)
            to_insert = list(self.items)
            rnd.shuffle(to_insert)

            # Let's check several sets created with different methods
            set1 = self.make()
            for k, v in to_insert:
                set1[k] = v

            # Random access
            keys = list(self.keys)
            rnd.shuffle(keys)
            for key in keys:
                self.assertEqual(set1[key], self.skipdict[key])

            # Create additional sets
            set2 = self.make(to_insert)
            set3 = self.make(set1.items())
            set4 = self.make(set2.items())

            # Check all of them
            cursets = (set1, set2, set3, set4)
            for cur in cursets:
                self.assertEqual(list(cur), self.keys)
                self.assertEqual(list(cur.keys()), self.keys)
                self.assertEqual(list(cur.values()), self.values)
                self.assertEqual(list(cur.items()), list(self.items))

            # Check equality of all combinations
            all_sets = (self.skipdict, set1, set2, set3, set4)
            for s1, s2 in combinations(all_sets, 2):
                self.assertEqual(s1, s2)

            # Let's pick up items to delete
            left = list(self.items)
            to_delete = []
            for i in range(rnd.randrange(10, 30)):
                idx = randrange(len(left))
                to_delete.append(left[idx])
                del left[idx]

            # Let's test deletion
            for cur in cursets:
                rnd.shuffle(to_delete)
                for key, value in to_delete:
                    del cur[key]

                # Test random access
                keys = list(dict(left))
                rnd.shuffle(keys)
                for key in keys:
                    self.assertNotIn(key, to_delete)
                    self.assertEqual(cur[key], self.skipdict[key])

                self.assertEqual(list(cur.items()), left)
                self.assertNotEqual(cur, self.skipdict)

            # Let's reinsert keys in random order, and check if it's
            # still ok
            for j, cur in enumerate(cursets):
                rnd.shuffle(to_delete)
                for key, value in to_delete:
                    cur[key] = value
                    self.assertEqual(cur[key], value)
                self.assertEqual(cur, self.skipdict)
                self.assertEqual(len(cur), len(self.skipdict))
                self.assertEqual(list(cur.items()), list(self.items))

                keys1 = []
                keys2 = []

                for i in range(len(self.keys)):
                    key = self.keys[i]
                    self.assertEqual(cur.index(key), i)
                    keys1.append(key)
                    keys2.append(cur.keys()[i])

                self.assertEqual(keys1, keys2)


class ReverseProtocolTestCase(ProtocolTestCase):
    reverse = True


class IteratorTestCaseMixin:
    @property
    def iterator(self):
        return self.func()

    @property
    def func(self):
        return getattr(self.skipdict, self.method)

    @property
    def expected(self):
        return getattr(self, self.method)

    def test_item(self):
        for i in range(len(self.keys)):
            self.assertEqual(
                self.iterator[i],
                self.expected[i]
            )

    def test_item_range(self):
        values = tuple(self.func(2, 3))

        for i in range(len(values)):
            self.assertEqual(values[i], self.func(2, 3)[i])

    def test_item_min_zero(self):
        self.assertEqual(
            self.func(0)[0],
            self.expected[0]
        )

    def test_call(self):
        self.assertRaises(TypeError, self.iterator)

    def test_step(self):
        self.assertRaises(
            ValueError,
            self.iterator.__getitem__,
            slice(None, None, 2),
        )

    def test_length(self):
        self.assertRaises(TypeError, len, self.iterator)

    def test_next(self):
        iterator = self.func()
        i = 0
        while True:
            try:
                item = next(iterator)
            except StopIteration:
                break

            self.assertEqual(item, self.expected[i])
            i += 1

    def test_repr(self):
        self.assertEqual(
            repr(self.iterator),
            "<SkipDictIterator "
            "type=\"%s\" "
            "forward=true "
            "min=%r "
            "max=%r "
            "at 0x%x>" % (
                self.method,
                self.values[0],
                self.values[-1],
                id(self.skipdict),
            )
        )

    def test_min(self):
        self.assertEqual(
            list(self.func(self.values[25])),
            list(self.expected[25:])
        )

    def test_max(self):
        self.assertEqual(
            list(self.func(None, self.values[25])),
            list(self.expected[:26])
        )

    def test_min_max(self):
        self.assertEqual(
            list(self.func(self.values[13], self.values[25])),
            list(self.expected[13:26])
        )

    def test_max_min(self):
        self.assertEqual(
            list(self.func(self.values[25], self.values[13])),
            list(reversed(self.expected[13:26]))
        )

    def decorate(f):
        def wrapper(self):
            s = f(self)
            self.assertEqual(
                list(self.expected[s]),
                list(self.iterator[s])
            )
        return wrapper

    @decorate
    def test_slice_min(self):
        return slice(13, None, 1)

    @decorate
    def test_slice_max(self):
        return slice(None, 13, 1)

    @decorate
    def test_slice_min_max(self):
        return slice(13, 25, 1)

    @decorate
    def test_slice_max_negative(self):
        return slice(None, -2, 1)


class IterKeysTestCase(IteratorTestCaseMixin, FixtureTestCase):
    method = "keys"


class IterValuesTestCase(IteratorTestCaseMixin, FixtureTestCase):
    method = "values"


class IterItemsTestCase(IteratorTestCaseMixin, FixtureTestCase):
    method = "items"
