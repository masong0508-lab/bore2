#!/usr/bin/env python3
"""Contract tests: a missing or malformed audit report must never pass CI."""
import unittest
from world_coherence import check_plan, check_foot


class CoherenceParserTests(unittest.TestCase):
    def test_valid_plan(self):
        self.assertEqual(check_plan(
            "281 homes: body differs 0, door differs 0, "
            "drive differs 0, walls outside the plan 0\n"), (281, 0))

    def test_plan_mismatch_is_detected(self):
        self.assertEqual(check_plan(
            "281 homes: body differs 1, door differs 2, "
            "drive differs 0, walls outside the plan 3\n"), (281, 6))

    def test_plan_missing_or_empty_is_not_a_pass(self):
        with self.assertRaises(ValueError):
            check_plan("audit passed\n")
        with self.assertRaises(ValueError):
            check_plan("0 homes: body differs 0, door differs 0, "
                       "drive differs 0, walls outside the plan 0\n")

    def test_valid_multi_kind_footprint(self):
        self.assertEqual(check_foot(
            "SHOP            112 lots,    0 differ\n"
            "HOTEL            18 lots,    0 differ\n"), (130, 0))

    def test_footprint_mismatch_is_detected(self):
        self.assertEqual(check_foot(
            "SHOP            112 lots,    2 differ\n"
            "HOTEL            18 lots,    1 differ\n"), (130, 3))

    def test_footprint_missing_or_empty_is_not_a_pass(self):
        with self.assertRaises(ValueError):
            check_foot("looks fine\n")
        with self.assertRaises(ValueError):
            check_foot("SHOP    0 lots,    0 differ\n")


if __name__ == "__main__":
    unittest.main()
