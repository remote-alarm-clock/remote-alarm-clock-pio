import subprocess

revision = (
    subprocess.check_output(["git", "describe", "--always", "--dirty"])
    .strip()
    .decode("utf-8")
)
print("'-DGIT_COMMIT_HASH=\"%s\"'" % revision)