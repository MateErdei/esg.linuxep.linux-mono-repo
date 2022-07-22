import tap.v1 as tap
import logging
import os

BUILD_TEMPLATE_LINUX = 'JenkinsLinuxTemplate7'

RELEASE_PACKAGE = "release-package.xml"

logger = logging.getLogger(__name__)

# Extract base version from release package xml file to save hard-coding it twice.
def get_version():
    import xml.dom.minidom
    if os.path.isfile(RELEASE_PACKAGE):
        dom = xml.dom.minidom.parseString(open(RELEASE_PACKAGE).read())
        package = dom.getElementsByTagName("package")[0]
        version = package.getAttribute("version")
        if version:
            logger.info(f"Extracted version from {RELEASE_PACKAGE}: {version}")
            return version

    logger.info("CWD: %s", os.getcwd())
    logger.info("DIR CWD: %s", str(os.listdir(os.getcwd())))
    raise Exception(f"Failed to extract version from {RELEASE_PACKAGE}")


@tap.pipeline(root_sequential=False)
def versig(stage: tap.Root):
    component = tap.Component(name='versig', base_version=get_version())

    stage.artisan_build(name='versig_sspl',
                        component=component,
                        image=BUILD_TEMPLATE_LINUX,
                        release_package=RELEASE_PACKAGE
                        )
    