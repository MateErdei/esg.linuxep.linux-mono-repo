from robot.parsing import TestData
import sys


def main(path_to_suites):

    if path_to_suites in [None, ""]:
        path_to_suites = "."

    suite = TestData(parent=None, source=path_to_suites)
    traverse_tests(suite, [])


def traverse_tests(suite, inherited_tags):
    suite_tags = []
    suite_tags.extend(inherited_tags)

    if suite.setting_table.force_tags:
        suite_tags.extend(suite.setting_table.force_tags.value)

    if suite.setting_table.default_tags:
        suite_tags.extend(suite.setting_table.default_tags.value)

    for test_case in suite.testcase_table:
        test_tags = []
        if test_case.tags:
            test_tags.extend(test_case.tags.value)

        tags = test_tags + suite_tags
        unique_tags = set(tags)

        print (str(suite.name) + ", " +str(test_case.name) + ", " + str(unique_tags))
        #print (str(unique_tags) + " --- " + str(testcase.name) )

    for child in suite.children:
        traverse_tests(child, suite_tags)


if __name__ == "__main__":
    path = "."
    if len(sys.argv) == 2:
        path = sys.argv[1]
    main(path)


