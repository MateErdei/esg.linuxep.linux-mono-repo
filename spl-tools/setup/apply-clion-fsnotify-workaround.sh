# CLion will watch a directory with inotify even if it is excluded:
# https://youtrack.jetbrains.com/issue/IDEA-73309/When-a-folder-is-excluded-from-the-project-is-still-watched-with-inotify
# This can result in uninterruptible excruciatingly long loading popups with the symlinks created by Bazel
# The workaround is to create a wrapper script to filter out files in that directory before passing them to fsnotifier

echo "Please provide the directory (absolute path) where CLion is installed:"
read clion_dir

echo "Please provide the directory where to install the wrapper [$clion_dir/bin]:"
read clion_fix_dir
if [ -z "$clion_fix_dir" ]
then
  clion_fix_dir="$clion_dir/bin"
fi

echo "grep -v -e '/bazel-.*' -e '/.output' | $clion_dir/bin/fsnotifier \"\$@\"" > "$clion_fix_dir/fsnotifier-wrapper"
chmod +x "$clion_fix_dir/fsnotifier-wrapper"

echo "To complete the fix, in CLion go to Help -> Edit Custom Properties and paste in:"
echo "idea.filewatcher.executable.path=$clion_fix_dir/fsnotifier-wrapper"

echo "Finally, restart CLion."
