# Test Flash Manager

import unittest
from unittest.mock import MagicMock
from src.Core.flash_manager import FlashManager, FlashConfig

class TestFlashManager(unittest.TestCase):
    def setUp(self):
        self.flash_manager = FlashManager()
        self.flash_manager.device_interface_ = MagicMock()

    def test_set_and_get_config(self):
        config = FlashConfig(verify_after_write=False, erase_before_write=False, retry_count=5, timeout_ms=10000)
        self.flash_manager.set_config(config)
        self.assertEqual(self.flash_manager.get_config().verify_after_write, False)
        self.assertEqual(self.flash_manager.get_config().erase_before_write, False)
        self.assertEqual(self.flash_manager.get_config().retry_count, 5)
        self.assertEqual(self.flash_manager.get_config().timeout_ms, 10000)

    def test_flash_firmware(self):
        self.flash_manager.load_firmware_file("dummy_path")
        self.flash_manager.device_interface_.flash.return_value = True
        self.assertTrue(self.flash_manager.flash_firmware())

    def test_verify_firmware(self):
        self.flash_manager.device_interface_.verify_flash.return_value = True
        self.assertTrue(self.flash_manager.verify_firmware())

    def test_erase_device(self):
        self.flash_manager.device_interface_.erase_chip.return_value = True
        self.assertTrue(self.flash_manager.erase_device())

if __name__ == '__main__':
    unittest.main()
