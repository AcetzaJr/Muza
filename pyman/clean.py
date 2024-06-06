import shutil


def clean():
    shutil.rmtree("build", ignore_errors=True)
