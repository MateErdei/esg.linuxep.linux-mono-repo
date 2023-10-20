#!/bin/bash

# Colours to use when outputting messages
red='\033[0;031m'
green='\033[0;32m' # Unused at the time of writing this comment
clear='\033[0m'

VALID_RPATHS=(
  "\$ORIGIN"
  "\$ORIGIN/../lib64"
  "\$ORIGIN:\$ORIGIN/../lib64"
  "\$ORIGIN/../chroot/lib64:\$ORIGIN"
  "\$ORIGIN:\$ORIGIN/../lib64:\$ORIGIN/../lib"
  "\$ORIGIN/../chroot/lib64:\$ORIGIN/../lib64"
  "\$ORIGIN/../lib64:\$ORIGIN/../chroot/lib64"
  "\$ORIGIN:\$ORIGIN/../lib64:\$ORIGIN/../chroot/lib64"
  "\$ORIGIN/../base/lib64"
  "\$ORIGIN:\$ORIGIN/.."
)

DEBUG_OUTPUT_ON=0 # 0 = off, 1 = on

INSECURE_RPATH_EXISTS=0
printf "Starting search for binaries and checking rpaths\n"

for executable_path in $(find /opt/sophos-spl/ -type f ! -size 0 ! -name "*.ide" ! -name "*.vdb" -exec grep -IL . "{}" \;) # Finding binaries in /opt/sophos-spl/
do
  if [[ $DEBUG_OUTPUT_ON -eq 1 ]]
  then
    printf "[[DEBUG]]: Executable path: $executable_path\n"
  fi
  rpath_line="$(objdump -x $executable_path | grep PATH)"

  # Split rpath string by separator
  separator=' '
  tmp=${rpath_line//"$separator"/$'\2'}
  IFS=$'\2' read -a rpath_substring_array <<< "$tmp"

  if [[ $DEBUG_OUTPUT_ON -eq 1 ]]
  then
    printf "[[DEBUG]] Splitting RPATH: $rpath_line\n"
    for substring in "${rpath_substring_array[@]}"
    do
      printf "[[DEBUG]] $substring\n"
    done
  fi

  # Making sure objdump returned a non-empty value
  if [[ ${#rpath_substring_array[@]} -gt 1 ]]
  then
    # RPATH line should be of the format: "RPATH <some amount of spaces> <rpath value>"
    # Getting the last value in the rpath_string_array which corresponds to the actual <rpath value>
    rpath=${rpath_substring_array[${#rpath_substring_array[@]} - 1]}
    if [[ $DEBUG_OUTPUT_ON -eq 1 ]]
    then
      printf "[[DEBUG]] $rpath\n"
    fi

    # Just comparing against valid rpaths now
    is_rpath_valid=0
    for valid_rpath in "${VALID_RPATHS[@]}"
    do
      if [ "$valid_rpath" = "$rpath" ]
      then
        is_rpath_valid=1
      fi
    done

    if [[ $is_rpath_valid -eq 0 ]]
    then
      INSECURE_RPATH_EXISTS=1
      printf "\n${red}[[INSECURE RPATH]] Binary path: $executable_path\n"
      printf "RPATH: $rpath\n"
      printf "The RPATH of the above binary was found to be insecure${clear}\n\n"
    fi
  else
    # If objdump doesn't complain before this line is outputted then it probably means that an RPATH line is not in binary
    # Objdump would complain about not being able to recognize file
    printf "[[INFO]] Binary path: $executable_path\n"
    printf "[[INFO]] RPATH not found for above binary or objdump could not read file well\n\n"
  fi
done
printf "Finished search for binaries and checking rpaths\n"
exit $INSECURE_RPATH_EXISTS