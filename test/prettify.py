class color:
    C_HEAD = '\033[95m'
    C_OKBLUE = '\033[94m'
    C_OKGREEN = '\033[92m'
    C_WARNING = '\033[93m'
    C_FAILED = '\033[91m'
    C_ENDC = '\033[0m'
    C_BOLD = '\033[1m'
    C_GRAY = '\033[0;37m'
    C_UNDERLINE = '\033[4m'
    C_WHITE_ON_RED = '\033[37;41m'
    C_BLACK_ON_GREEN = '\033[42;30m'

    @staticmethod
    def WARNING(string):
      return color.C_WARNING + "[ WARNING ] " + string + color.C_ENDC

    @staticmethod
    def FAIL(string):
      return "\n" + color.C_FAILED + "[ FAIL ] " + string + color.C_ENDC + "\n"

    @staticmethod
    def FAIL_INLINE():
      return '[ ' + color.C_WHITE_ON_RED + " FAIL " + color.C_ENDC + ' ]'

    @staticmethod
    def SUCCESS(string):
      return "\n" + color.C_OKGREEN + "[ SUCCESS ] " + string + color.C_ENDC + "\n"

    @staticmethod
    def PASS(string):
      return "\n" + color.C_OKGREEN + "[ PASS ] " + string + color.C_ENDC + "\n"

    @staticmethod
    def PASS_INLINE():
      return '[ ' + color.C_BLACK_ON_GREEN + " PASS " + color.C_ENDC + ' ]'
    
    @staticmethod
    def OK(string):
      return color.C_OKGREEN + "[ OK ] " + string + color.C_ENDC

    @staticmethod
    def INFO(string):
      return color.C_OKBLUE + "* " + string + ": " + color.C_ENDC

    @staticmethod
    def SUBPROC(string):
      return color.C_GRAY + "! " + string + color.C_ENDC

    @staticmethod
    def DATA(string):
      return color.C_GRAY + string + color.C_ENDC + "\n"

    @staticmethod
    def HEADER(string):
      return str("\n"+color.C_HEAD + "{:=^80}" + color.C_ENDC).format(" " + string + " ")
