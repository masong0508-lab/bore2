"""PPF3 patch-reader integrity regression, separate from full disc order testing."""
import pathlib
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
import packs


class PackPatchTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.path = pathlib.Path(self.tmp.name) / "test.ppf"

    def tearDown(self):
        self.tmp.cleanup()

    def write(self, data):
        self.path.write_bytes(data)
        return self.path

    def valid(self, changes=((10, b"ABC"),)):
        packs.write_ppf(self.path, "TEST", changes)
        return self.path.read_bytes()

    def rejects_without_writing(self, patch):
        self.write(patch)
        img = bytearray(b"\x00" * 128)
        with self.assertRaises(ValueError):
            packs.apply_ppf(img, self.path)
        self.assertEqual(img, bytearray(b"\x00" * 128))

    def test_valid_records_preserve_image_length(self):
        self.valid(((10, b"abc"), (127, b"Z")))
        img = bytearray(128)
        packs.apply_ppf(img, self.path)
        self.assertEqual(len(img), 128)
        self.assertEqual(img[10:13], b"abc")
        self.assertEqual(img[127], ord("Z"))

    def test_empty_patch_is_valid(self):
        self.valid(())
        img = bytearray(128)
        packs.apply_ppf(img, self.path)
        self.assertEqual(img, bytearray(128))

    def test_undo_records_are_skipped_correctly(self):
        patch = bytearray(self.valid(((3, b"XYZ"),)))
        patch[58] = 1
        patch.extend(b"OLD")  # PPF3 undo bytes follow the replacement record
        self.write(patch)
        img = bytearray(128)
        packs.apply_ppf(img, self.path)
        self.assertEqual(img[3:6], b"XYZ")

    def test_truncated_header(self):
        self.rejects_without_writing(b"PPF30")

    def test_invalid_format(self):
        self.rejects_without_writing(b"BROKE" + self.valid()[5:])

    def test_bad_flag(self):
        patch = bytearray(self.valid())
        patch[58] = 3
        self.rejects_without_writing(patch)

    def test_truncated_blockcheck(self):
        patch = bytearray(self.valid())
        patch[57] = 1
        self.rejects_without_writing(patch)

    def test_truncated_record_header(self):
        self.rejects_without_writing(self.valid(()) + b"\x00" * 8)

    def test_zero_length_record(self):
        self.rejects_without_writing(self.valid(()) + struct.pack("<QB", 10, 0))

    def test_truncated_record_payload_does_not_apply_prior_records(self):
        self.rejects_without_writing(self.valid(((1, b"FIRST"), (10, b"SECOND")))[:-2])

    def test_out_of_bounds_record_does_not_resize_image(self):
        self.rejects_without_writing(self.valid(((2, b"OK"), (127, b"AB"))))

    def test_huge_offset_is_rejected(self):
        self.rejects_without_writing(self.valid(((2**64 - 2, b"Z"),)))


if __name__ == "__main__":
    unittest.main()
