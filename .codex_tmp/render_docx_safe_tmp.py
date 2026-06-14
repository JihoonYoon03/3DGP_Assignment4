import os
import runpy
import shutil
import sys
import tempfile
import uuid


class SafeTemporaryDirectory:
    def __init__(self, suffix=None, prefix=None, dir=None, ignore_cleanup_errors=False):
        self.suffix = suffix or ""
        self.prefix = prefix or "tmp"
        self.dir = os.path.abspath(dir or tempfile.gettempdir())
        self.ignore_cleanup_errors = ignore_cleanup_errors
        self.name = None

    def __enter__(self):
        for _ in range(100):
            candidate = os.path.join(self.dir, f"{self.prefix}{uuid.uuid4().hex}{self.suffix}")
            try:
                os.makedirs(candidate)
                self.name = candidate
                return candidate
            except FileExistsError:
                continue
        raise FileExistsError("could not create a unique temporary directory")

    def __exit__(self, exc_type, exc, tb):
        if self.name:
            shutil.rmtree(self.name, ignore_errors=self.ignore_cleanup_errors)
        return False


tempfile.TemporaryDirectory = SafeTemporaryDirectory

sys.argv = [
    "render_docx.py",
    r"C:\Users\wlgns\Documents\Github\3DGP_Assignment4\4차과제보고서.docx",
    "--output_dir",
    r"C:\Users\wlgns\Documents\Github\3DGP_Assignment4\.codex_tmp\report_render",
    "--emit_pdf",
]

runpy.run_path(
    r"C:\Users\wlgns\.codex\plugins\cache\openai-primary-runtime\documents\26.601.10930\skills\documents\render_docx.py",
    run_name="__main__",
)
