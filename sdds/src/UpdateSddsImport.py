import os
import hashlib
import optparse
import shutil


def get_version_string(tagname, inifilepath):
    inifile = open(inifilepath)
    lines = inifile.readlines()
    inifile.close()

    lineprefix = tagname + '='
    for line in lines:
        if line.startswith(lineprefix):
            return line[len(lineprefix):].strip(' \n')

    raise Exception('Name %s not found in %s' % (tagname, inifilepath))


def replace_version_string(sdds_import_content, version_string):
    print 'Searching manifest content ...'
    original = sdds_import_content.split("Version>")[1]

    print 'Found: ' + original
    if original is None:
        raise Exception('Could not find element Version in the manaifest content.')

    replacement = version_string + '</'
    print 'Replacing with: ' + replacement

    return sdds_import_content.replace(original, replacement)


def remove_trailing(file_path):
    if file_path.endswith("\\"):
        file_path = file_path[:-1]
    return file_path


def update_checksum_and_size(sdds_import_content, file_name, offset, dest_file_path):
    print 'Searching manifest content ...'

    original = None
    elements = sdds_import_content.split("/><File ")
    for element in elements:
        if file_name in element:
            original = element
            break

    print 'Found: ' + original
    if original is None:
        raise Exception('Could not find the file in the manaifest content.')

    file_content = open(os.path.join(dest_file_path), "rb").read()
    md5 = hashlib.md5(file_content).hexdigest()
    size = len(file_content)
    if len(offset) == 0:
        replacement = r'MD5="%s" Name="%s" Size="%d"' % (md5.lower(), file_name, size)
    else:    
        replacement = r'MD5="%s" Name="%s" Offset="%s" Size="%d"' % (md5.lower(), file_name, offset, size)
    print 'Replacing with: ' + replacement

    return sdds_import_content.replace(original, replacement)


def generate_checksum_file(sdds_import_file_path):

    print 'Calculating checksums ...'
    file_content = open(sdds_import_file_path, "rb").read()
    md5 = hashlib.md5(file_content).hexdigest()
    sha1 = hashlib.sha1(file_content).hexdigest()

    checksum_file_path = sdds_import_file_path + ".chksum"

    print 'Writing ' + checksum_file_path + '...'
    checksum_file = open(checksum_file_path, "w")
    checksum_file.write("File: " + sdds_import_file_path + "\n")
    checksum_file.write("Size: " + str(len(file_content)) + "\n")
    checksum_file.write("MD5:  " + md5 + "\n")
    checksum_file.write("SHA1: " + sha1 + "\n")
    checksum_file.close()

    folder_path_tokens = sdds_import_file_path.split("\\")[:-1]
    folder_path_tokens.append("SDDS-Import-checksum.txt")
    old_checksum_file_path = '\\'.join(folder_path_tokens)

    if os.path.exists(old_checksum_file_path):
        print 'Deleting ' + old_checksum_file_path + '...'
        os.remove(old_checksum_file_path)


def replace_file(source_file_path, dest_file_path):

    if os.path.exists(dest_file_path):
        print 'Deleting old ' + dest_file_path + '...'
        os.remove(dest_file_path)
    else:
        raise Exception("Destination file %s does not exist" % dest_file_path)

    print("Copying %s to %s" % (source_file_path, dest_file_path))
    shutil.copyfile(source_file_path, dest_file_path)


def update_sdds_import(sdds_import_file_path, offset, file_name, dest_file_path):

    print 'Reading ' + sdds_import_file_path + '...'

    sdds_import_file = open(sdds_import_file_path)
    sdds_import_content = sdds_import_file.read()
    sdds_import_file.close()

    sdds_import_content = update_checksum_and_size(sdds_import_content, file_name, offset, dest_file_path)

    print 'Writing ' + sdds_import_file_path + '...'
    sdds_import_file = open(sdds_import_file_path, "w")
    sdds_import_file.write(sdds_import_content)
    sdds_import_file.close()


def main():

    parser = optparse.OptionParser(version="%prog version 1.0", usage="%prog filename")
    parser.add_option("-s", "--source", dest="source_file_path", type="string", help="Source file path")
    parser.add_option("-i", "--import", dest="import_file", type="string", help="SDDS-import.xml file path")
    parser.add_option("-o", "--offset", dest="offset", type="string", help="Offset: relative folder path to the package root where the target file for replacement exists")

    print "Parsing arguments"
    (arguments, args) = parser.parse_args()

    # validate parameter values
    if len(args) != 0:
        parser.error("Incorrect number of parameters")
    if not arguments.import_file.endswith(r"SDDS-Import.xml"):
        parser.error("Incorrect SDDS-import file name")
    if not os.path.isfile(arguments.source_file_path):
        print "Source file does not exist %s" % arguments.source_file_path.ToString()
        parser.error("Source file does not exist")
                
    # Ensure all paths use backslash as a separator
    sdds_import_file_path = remove_trailing(arguments.import_file.replace('/', '\\'))
    source_file_path = remove_trailing(arguments.source_file_path.replace('/', '\\'))
    offset = remove_trailing(arguments.offset.replace('/', '\\')) if arguments.offset is not None else ""
    
    # extract the file name from the source path
    file_name = source_file_path.split("\\")[-1]
    
    #construct destination file path
    dest_file_path = '\\'.join(sdds_import_file_path.split("\\")[:-1])
    if len(offset) > 0:
        dest_file_path += "\\"
        dest_file_path += offset
    dest_file_path += "\\"
    dest_file_path += file_name

    print 'Destination path ' + dest_file_path + '...'

    # copy source file to destination
    replace_file(source_file_path, dest_file_path)

    # update the manifest
    update_sdds_import(sdds_import_file_path, offset, file_name, dest_file_path)

    # update the manifest checksum file
    generate_checksum_file(sdds_import_file_path)

    print 'Done.'

main()
