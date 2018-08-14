from conans import ConanFile, CMake, tools


class PimplConan(ConanFile):
    name = "Pimpl"
    version = "1.0.0"
    license = "Boost Software License - Version 1.0"
    url = "https://github.com/microblink/pimpl"
    description = "Proving pimpls do not require heap, exceptions or runtime polymorphism."
    generators = "cmake"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }
    no_copy_source = True


    def package(self):
        self.copy("include/*", dst="")


    def package_id(self):
        self.info.header_only()

