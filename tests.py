from unittest import TestCase
from operator import itemgetter
from random import Random, randrange
from itertools import combinations


class BaseTestCase(TestCase):
    maxlevel = 6
    items = ()
    reverse = False

    def setUp(self):
        items = self.items if not self.reverse else reversed(self.items)
        self.skipdict = self.make(items, self.maxlevel)

    def make(self, *args, **kwargs):
        from skipdict import SkipDict
        return SkipDict(*args, **kwargs)


class PropertyTestCase(BaseTestCase):
    def test_maxlevel(self):
        self.assertEqual(self.skipdict.maxlevel, self.maxlevel)


class ConstructorTestCase(BaseTestCase):
    items = {'Xe2W0QxllGdCW251l7U9Dg': 150.0}

    def test_with_dict(self):
        self.assertEqual(len(self.skipdict), 1)


class FuzzyTestCase(BaseTestCase):
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

    def test_del_item(self):
        del self.skipdict[self.keys[-1]]
        self.assertRaises(
            KeyError,
            self.skipdict.__getitem__,
            'zI1yPoXCgPtifROhCST0sA'
        )

    def test_del_item_missing(self):
        self.assertRaises(
            KeyError, self.skipdict.__delitem__,
            'zI1yPoXCgPtifROhCST0sA'
        )

    def test_set_item(self):
        self.skipdict['zI1yPoXCgPtifROhCST0sA'] = 62365.0
        self.assertEqual(self.skipdict['zI1yPoXCgPtifROhCST0sA'], 62365.0)

    def test_set_item_existing(self):
        self.skipdict[self.keys[-1]] = 62365.0
        self.assertEqual(self.skipdict[self.keys[-1]], 62365.0)
        self.assertEqual(len(self.skipdict), len(self.keys))

    def test_setdefault(self):
        for key, value in self.items:
            self.assertEqual(self.skipdict.setdefault(key), value)

    def test_setdefault_already_exists(self):
        for key, value in self.items:
            self.assertEqual(self.skipdict.setdefault(key), value)

    def test_iteration(self):
        self.assertEqual(list(self.skipdict), self.keys)

    def test_keys(self):
        self.assertEqual(self.skipdict.keys(), self.keys)

    def test_values(self):
        self.assertEqual(self.skipdict.values(), self.values)

    def test_items(self):
        self.assertEqual(self.skipdict.items(), list(self.items))

    def test_iteritems(self):
        self.assertEqual(list(self.skipdict.iteritems()), list(self.items))

    def test_iteritems_min(self):
        self.assertEqual(
            list(self.skipdict.iteritems(self.values[25])),
            list(self.items[25:])
        )

    def test_iteritems_max(self):
        self.assertEqual(
            list(self.skipdict.iteritems(None, self.values[25])),
            list(self.items[:26])
        )

    def test_iteritems_min_max(self):
        self.assertEqual(
            list(self.skipdict.iteritems(self.values[13], self.values[25])),
            list(self.items[13:26])
        )

    def test_iteritems_max_min(self):
        self.assertEqual(
            list(self.skipdict.iteritems(self.values[25], self.values[13])),
            list(reversed(self.items[13:26]))
        )

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

    def test_index_repr(self):
        self.assertEqual(repr(self.skipdict.index), repr(self.keys))

    def test_index_slice(self):
        self.assertEqual(
            list(self.skipdict.index[13:25]),
            self.keys[13:25]
        )

    def test_index_slice_items(self):
        self.assertEqual(
            list(self.skipdict.index[13:25].items()),
            list(self.items[13:25])
        )

    def test_index_slice_values(self):
        self.assertEqual(
            list(self.skipdict.index[13:25].values()),
            self.values[13:25]
        )

    def test_index_slice_min(self):
        self.assertEqual(
            list(self.skipdict.index[13:]),
            self.keys[13:]
        )

    def test_index_slice_max(self):
        self.assertEqual(
            list(self.skipdict.index[:13]),
            self.keys[:13]
        )

    def test_index_slice_negative(self):
        self.assertEqual(
            list(self.skipdict.index[:-2]),
            self.keys[:-2]
        )

    def test_index_subscript(self):
        length = len(self.items)
        for i in range(length):
            self.assertEqual(self.skipdict.index[i], self.keys[i])

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
                self.assertEqual(list(cur.items()), left)
                self.assertNotEqual(cur, self.skipdict)

            # Let's reinsert keys in random order, and check if it's
            # still ok
            for cur in cursets:
                rnd.shuffle(to_delete)
                for key, value in to_delete:
                    cur[key] = value
                self.assertEqual(cur, self.skipdict)
                self.assertEqual(list(cur.items()), list(self.items))
                for i in range(len(self.items)):
                    key, score = self.items[i]
                    self.assertEqual(cur.index(key), i)
                    self.assertEqual(cur.index[i], key)


class ReverseFuzzyTest(FuzzyTestCase):
    reverse = True

    # def test_slicing(self):
    #     # Slicing by index should be same as slicing list
    #     ends = (None, 0, 2, 5, 10, 49, 50, 51, -1, -2, -5, -10, -49, 50, -51)
    #     steps = (1, 2, 3)
    #     for start, stop, step in product(ends, ends, steps):
    #         self.assertEqual(list(set_n.index[start:stop:step].items()),
    #                          items[start:stop:step])
    #         self.assertEqual(list(set_r.index[start:stop:step].items()),
    #                          items[start:stop:step])
    #     # let's try to delete and reinsert slice
    #     for start, stop in product(ends, ends):
    #         slc = set_r.index[start:stop]
    #         slclist = items[start:stop]
    #         self.assertEqual(list(slc.items()), slclist)
    #         # delete a slice from set and list, then compare
    #         del set_r.index[start:stop]
    #         tmp = items[:]
    #         del tmp[start:stop]
    #         self.assertEqual(list(set_r.items()), tmp)
    #         # let's recreate set and check
    #         set_r.update(slc)
    #         self.assertEqual(list(set_r.items()), items)

    #         # try score slice on reverted set
    #         if slclist:
    #             # can do this only if set is not empty
    #             scorestart = slclist[0][1]
    #             scorestop = slclist[-1][1] + 1
    #             slc2 = set_r.by_score[scorestart:scorestop]
    #             self.assertEqual(list(slc2.items()), slclist)
    #             del set_r.by_score[scorestart:scorestop]
    #             self.assertEqual(list(set_r.items()), tmp)
    #             # and revert this change back again
    #             set_r.update(slc2)
    #             self.assertEqual(list(set_r.items()), items)


