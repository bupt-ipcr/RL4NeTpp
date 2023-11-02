"""
Author       : CHEN Jiawei
Date         : 2023-10-19 09:51:55
LastEditors  : LIN Guocheng
LastEditTime : 2023-10-19 15:59:34
FilePath     : /home/lgc/test/RL4Net++/utils/get_osfp_ned.py
Description  : Change hasRip in the ned file to hasOspf.
"""


def change_ned_to_rip(ned_file_path: str) -> None:
    """Change hasRip in the ned file to hasOspf.

    Args:
        ned_file_path (str): path of .ned file
    """
    if not ned_file_path:
        print("\033[0;31m Error! gml_url is None! \033[0m")
        return
    with open(ned_file_path, "r") as ned_file:
        context = ned_file.read()
    context = context.replace("hasRip = true;", "hasOspf = true;")
    with open(ned_file_path, "w") as ned_file:
        ned_file.write(context)


if __name__ == "__main__":
    change_ned_to_rip("config/ned/Sprint.ned")
