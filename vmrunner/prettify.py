
class color:

    BLACK = "0"
    RED = "1"
    GREEN = "2"
    YELLOW = "3"
    BLUE = "4"
    MAGENTA = "5"
    CYAN = "6"
    WHITE = "7"

    NORMAL = "0"
    BRIGHT = "1"
    ITALIC = "3"
    UNDERLINE = "4"
    BLINK = "5"
    REVERSE = "7"
    NONE = "8"

    FG_NORMAL = "3"
    BG_NORMAL = "4"
    FG_BRIGHT = "9"
    BG_BRIGHT = "10"

    ESC = "\033["
    CLEAR = "0"

    C_GRAY = ESC + FG_NORMAL + WHITE + "m"
    C_DARK_GRAY = ESC + FG_BRIGHT + BLACK + "m"
    C_OKBLUE = ESC + FG_BRIGHT + BLUE + "m"
    C_ENDC = ESC + CLEAR + "m"
    C_OKGREEN = ESC + FG_BRIGHT + GREEN + "m"
    C_GREEN = ESC + FG_NORMAL + GREEN + "m"
    C_WARNING = ESC + FG_BRIGHT + YELLOW + "m"
    C_FAILED = ESC + FG_BRIGHT + RED + "m"

    C_HEAD = ESC + FG_BRIGHT + MAGENTA + "m"
    C_WHITE_ON_RED = ESC + FG_NORMAL + WHITE + ";" + BG_NORMAL + RED + "m"
    C_BLACK_ON_GREEN = ESC + FG_NORMAL + BLACK + ";" + BG_NORMAL + GREEN + "m"


    @staticmethod
    def code(fg = WHITE, bg = BLACK, style = NORMAL, fg_intensity = FG_NORMAL, bg_intensity = BG_NORMAL):
        return  color.ESC + style + ";" +  fg_intensity + fg + ";" + bg_intensity + bg + "m"

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
        return color.C_BLACK_ON_GREEN + "[ OK ] " + string + color.C_ENDC

    @staticmethod
    def INFO(string):
        return color.C_OKBLUE + "* " + string + ": " + color.C_ENDC

    @staticmethod
    def SUBPROC(string):
        return color.C_GREEN + "> " + color.C_DARK_GRAY + string + color.C_ENDC

    @staticmethod
    def VM(string):
        return color.C_GREEN + "<vm> " + color.C_ENDC + string

    @staticmethod
    def DATA(string):
        return color.C_GRAY + string + color.C_ENDC + "\n"

    @staticmethod
    def HEADER(string):
        return str("\n"+color.C_HEAD + "{:=^80}" + color.C_ENDC).format(" " + string + " ")


    @staticmethod
    def color_test():
        for fg in range(0,8):
            for bg in range(0,8):
                print color.code(fg = str(fg), bg = str(bg)), "Color " , str(fg), color.C_ENDC
