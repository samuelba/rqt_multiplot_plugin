#!/usr/bin/env python3

import json
import os
import re
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import run_clang_tidy as rct  # noqa: E402


class FilterCompileCommandsTest(unittest.TestCase):
    def test_drops_moc_ui_qrc_and_missing_files(self):
        # Arrange
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            real_src = root / 'src' / 'PlotWidget.cpp'
            real_src.parent.mkdir()
            real_src.write_text('int x;\n')
            db = [
                {'file': str(real_src), 'directory': tmp, 'command': 'c++'},
                {'file': str(root / 'build' / 'moc_PlotWidget.cpp'), 'directory': tmp, 'command': 'c++'},
                {'file': str(root / 'build' / 'ui_PlotWidget.h'), 'directory': tmp, 'command': 'c++'},
                {'file': str(root / 'build' / 'qrc_resource.cpp'), 'directory': tmp, 'command': 'c++'},
                {'file': str(root / 'missing.cpp'), 'directory': tmp, 'command': 'c++'},
            ]

            # Act
            filtered = rct.filter_compile_commands(db, root)

            # Assert
            self.assertEqual([entry['file'] for entry in filtered], [str(real_src)])

    def test_drops_package_tests_and_gtest_sources(self):
        # Arrange
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / 'src' / 'StatusWidget.cpp'
            test = root / 'test' / 'StatusWidgetTest.cpp'
            gtest = root / 'build' / 'gtest' / 'gtest-all.cc'
            src.parent.mkdir()
            test.parent.mkdir(parents=True)
            gtest.parent.mkdir(parents=True)
            src.write_text('int a;\n')
            test.write_text('int b;\n')
            gtest.write_text('int c;\n')
            db = [
                {'file': str(src), 'directory': tmp, 'command': 'c++'},
                {'file': str(test), 'directory': tmp, 'command': 'c++'},
                {'file': str(gtest), 'directory': tmp, 'command': 'c++'},
            ]

            # Act
            filtered = rct.filter_compile_commands(db, root)

            # Assert
            self.assertEqual([entry['file'] for entry in filtered], [str(src)])


class SelectTranslationUnitsTest(unittest.TestCase):
    def test_cpp_only_changes_select_those_files(self):
        # Arrange
        tus = ['/ws/src/rqt_multiplot/src/A.cpp', '/ws/src/rqt_multiplot/src/B.cpp']
        changed = ['src/A.cpp']

        # Act
        selected = rct.select_translation_units(changed, tus, Path('/ws/src/rqt_multiplot'))

        # Assert
        self.assertEqual(selected, ['/ws/src/rqt_multiplot/src/A.cpp'])

    def test_header_change_selects_all_filtered_tus(self):
        # Arrange
        tus = ['/ws/src/rqt_multiplot/src/A.cpp', '/ws/src/rqt_multiplot/src/B.cpp']
        changed = ['include/rqt_multiplot/A.h']

        # Act
        selected = rct.select_translation_units(changed, tus, Path('/ws/src/rqt_multiplot'))

        # Assert
        self.assertEqual(selected, tus)

    def test_non_cpp_changes_select_nothing(self):
        # Arrange
        tus = ['/ws/src/rqt_multiplot/src/A.cpp']
        changed = ['package.xml', 'CMakeLists.txt']

        # Act
        selected = rct.select_translation_units(changed, tus, Path('/ws/src/rqt_multiplot'))

        # Assert
        self.assertEqual(selected, [])


class ListChangedFilesTest(unittest.TestCase):
    def test_raises_runtime_error_when_git_fails(self):
        # Arrange
        failed = Mock(returncode=1, stdout='', stderr='bad ref')

        # Act / Assert
        with patch('run_clang_tidy.subprocess.run', return_value=failed):
            with self.assertRaises(RuntimeError):
                rct.list_changed_files(Path('/tmp'), 'missing-ref')


class ChangedSinceRefTest(unittest.TestCase):
    def test_all_zeros_sha_means_full_scan(self):
        self.assertTrue(rct.is_full_scan_ref(None))
        self.assertTrue(rct.is_full_scan_ref(''))
        self.assertTrue(rct.is_full_scan_ref('0' * 40))
        self.assertFalse(rct.is_full_scan_ref('abc123'))


class JobCountTest(unittest.TestCase):
    def test_defaults_to_all_cpus(self):
        self.assertEqual(rct.job_count(cpu_count=8, override=None), 8)
        self.assertEqual(rct.job_count(cpu_count=1, override=None), 1)
        self.assertEqual(rct.job_count(cpu_count=None, override=None), 1)
        self.assertEqual(rct.job_count(cpu_count=8, override=3), 3)

    def test_ignores_invalid_jobs_env(self):
        # Arrange
        previous = os.environ.get('CLANG_TIDY_JOBS')
        os.environ['CLANG_TIDY_JOBS'] = 'nope'
        try:
            # Act
            jobs = rct.resolve_jobs(None)
        finally:
            if previous is None:
                os.environ.pop('CLANG_TIDY_JOBS', None)
            else:
                os.environ['CLANG_TIDY_JOBS'] = previous

        # Assert
        self.assertEqual(jobs, rct.job_count(os.cpu_count(), None))


class HeaderFilterTest(unittest.TestCase):
    def test_matches_package_headers_and_ignores_generated_qt_files(self):
        pattern = re.compile(rct.header_filter())

        self.assertRegex('/home/sam/projects/ros/plotting/rqt_multiplot_plugin/include/rqt_multiplot/PlotWidget.h', pattern)
        self.assertRegex('/ws/src/rqt_multiplot/src/rqt_multiplot/PlotWidget.h', pattern)
        self.assertIsNone(pattern.search('/ws/build/rqt_multiplot/ui_PlotWidget.h'))
        self.assertIsNone(pattern.search('/ws/build/rqt_multiplot/moc_PlotWidget.cpp'))
        self.assertIsNone(pattern.search('/ws/build/rqt_multiplot/include/rqt_multiplot/moc_PlotWidget.cpp'))
        self.assertIsNone(pattern.search('/usr/include/qt5/QtWidgets/qwidget.h'))
        self.assertIsNone(pattern.search('include/rqt_multiplot_plugin/PlotWidget.h'))

    def test_build_command_uses_generated_file_excluding_header_filter(self):
        cmd = rct.build_command(
            '/usr/bin/run-clang-tidy',
            Path('/tmp/build'),
            2,
            Path('/ws/src/rqt_multiplot_plugin'),
            ['/ws/src/rqt_multiplot_plugin/src/rqt_multiplot/PlotWidget.cpp'],
            None,
        )

        self.assertEqual(cmd[cmd.index('-header-filter') + 1], rct.header_filter())
        self.assertNotIn('include/rqt_multiplot_plugin/', cmd)


class WriteFilteredDatabaseTest(unittest.TestCase):
    def test_writes_compile_commands_json(self):
        # Arrange
        with tempfile.TemporaryDirectory() as tmp:
            entries = [{'file': '/a.cpp', 'directory': tmp, 'command': 'c++'}]

            # Act
            path = rct.write_compile_commands(Path(tmp), entries)

            # Assert
            self.assertEqual(path.name, 'compile_commands.json')
            self.assertEqual(json.loads(path.read_text()), entries)


if __name__ == '__main__':
    unittest.main()
