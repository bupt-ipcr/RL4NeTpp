"""
Author       : CHEN Jiawei
Date         : 2023-10-19 09:51:55
LastEditors  : LIN Guocheng
LastEditTime : 2023-10-19 16:00:35
FilePath     : /home/lgc/test/RL4Net++/utils/get_rip_ned.py
Description  : Change hasOspf in the ned file to hasRip.
"""


def change_ned_to_rip(ned_file_path: str) -> None:
    """Change hasOspf in the ned file to hasRip.

    Args:
        ned_file_path (str): path of .ned file
    """
    if not ned_file_path:
        print("\033[0;31m Error! gml_url is None! \033[0m")
        return
    with open(ned_file_path, "r") as ned_file:
        context = ned_file.read()
    context = context.replace("hasOspf = true;", "hasRip = true;")
    with open(ned_file_path, "w") as ned_file:
        ned_file.write(context)


if __name__ == "__main__":
    change_ned_to_rip("config/ned/Abilene.ned")
