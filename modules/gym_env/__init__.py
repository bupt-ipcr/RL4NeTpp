"""
Author       : CHEN Jiawei
Date         : 2023-10-19 10:02:20
LastEditors  : LIN Guocheng
LastEditTime : 2023-10-19 10:40:42
FilePath     : RL4Net++/modules/gym_env/__init__.py
Description  : Register OmnetEnv Environment.
"""

from gym.envs.registration import register

register(
    id="OMNET-v0",
    entry_point="env.omnet_env:OmnetEnv",
)
