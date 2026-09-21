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

    def test_drops_fetched_dependencies_and_autogen_sources(self):
        # Arrange
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / 'src' / 'PlotWidget.cpp'
            src.parent.mkdir(parents=True)
            src.write_text('int a;\n')
            db = [
                {'file': str(src), 'directory': tmp, 'command': 'c++'},
                {
                    'file': str(root / 'build' / '_deps' / 'qwt-src' / 'src' / 'qwt_plot.cpp'),
                    'directory': tmp,
                    'command': 'c++',
                },
                {
                    'file': str(root / 'build' / 'qwt_qt6_autogen' / 'mocs_compilation.cpp'),
                    'directory': tmp,
                    'command': 'c++',
                },
            ]
            (root / 'build' / '_deps' / 'qwt-src' / 'src').mkdir(parents=True)
            (root / 'build' / '_deps' / 'qwt-src' / 'src' / 'qwt_plot.cpp').write_text('int b;\n')
            (root / 'build' / 'qwt_qt6_autogen').mkdir(parents=True)
            (root / 'build' / 'qwt_qt6_autogen' / 'mocs_compilation.cpp').write_text('int c;\n')

            # Act
            filtered = rct.filter_compile_commands(db, root)

            # Assert
            self.assertEqual([entry['file'] for entry in filtered], [str(src)])

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


class FindRunClangTidyTest(unittest.TestCase):
    def test_prefers_pinned_version(self):
        with patch.dict(os.environ, {'CLANG_TIDY_VERSION': '23'}, clear=False):
            with patch('run_clang_tidy.shutil.which', side_effect=lambda name: f'/usr/bin/{name}' if name == 'run-clang-tidy-23' else None):
                self.assertEqual(rct.find_run_clang_tidy(), '/usr/bin/run-clang-tidy-23')

    def test_defaults_to_clang_tidy_23(self):
        env = {key: value for key, value in os.environ.items() if key != 'CLANG_TIDY_VERSION'}
        with patch.dict(os.environ, env, clear=True):
            with patch('run_clang_tidy.shutil.which', side_effect=lambda name: f'/usr/bin/{name}' if name == 'run-clang-tidy-23' else None):
                self.assertEqual(rct.find_run_clang_tidy(), '/usr/bin/run-clang-tidy-23')


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
        self.assertEqual(cmd[cmd.index('-warnings-as-errors') + 1], '*')


class ExitStatusTest(unittest.TestCase):
    def test_fails_when_warning_is_printed_even_if_returncode_is_zero(self):
        output = '/tmp/src/Foo.cpp:12:3: warning: use nullptr [modernize-use-nullptr]\n'

        self.assertEqual(rct.exit_status(0, output), 1)

    def test_fails_when_error_is_printed_with_zero_returncode(self):
        output = '/tmp/src/Foo.cpp:12:3: error: use nullptr [modernize-use-nullptr]\n'

        self.assertEqual(rct.exit_status(0, output), 1)

    def test_fails_when_fatal_error_is_printed_with_zero_returncode(self):
        output = '/tmp/src/Foo.cpp:1:1: fatal error: \'missing.h\' file not found\n'

        self.assertEqual(rct.exit_status(0, output), 1)

    def test_passes_when_no_diagnostics_and_returncode_is_zero(self):
        self.assertEqual(rct.exit_status(0, 'clang-tidy: full scan (3 files)\n'), 0)

    def test_preserves_nonzero_returncode(self):
        self.assertEqual(rct.exit_status(2, ''), 2)

    def test_ignores_warning_summary_without_diagnostic_location(self):
        self.assertEqual(rct.exit_status(0, '3 warnings generated.\n'), 0)


class RunCommandTest(unittest.TestCase):
    def test_forwards_merged_output_and_returncode(self):
        returncode, output = rct.run_command(
            [
                sys.executable,
                '-u',
                '-c',
                'import sys; print("hello"); print("err", file=sys.stderr); sys.exit(3)',
            ]
        )

        self.assertEqual(returncode, 3)
        self.assertEqual(output, 'hello\nerr\n')


class MainFailOnWarningTest(unittest.TestCase):
    def test_main_returns_nonzero_when_tidy_prints_warning(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / 'src' / 'A.cpp'
            src.parent.mkdir()
            src.write_text('int x;\n')
            compile_commands = root / 'compile_commands.json'
            compile_commands.write_text(
                json.dumps([{'file': str(src), 'directory': tmp, 'command': 'c++'}])
            )
            warning = f'{src}:1:1: warning: unused [misc-unused-using-decls]\n'

            with patch('run_clang_tidy.find_run_clang_tidy', return_value='/usr/bin/run-clang-tidy'):
                with patch('run_clang_tidy.run_command', return_value=(0, warning)):
                    status = rct.main(
                        [
                            '--compile-commands',
                            str(compile_commands),
                            '--source-root',
                            str(root),
                            '--all',
                        ]
                    )

            self.assertEqual(status, 1)


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
