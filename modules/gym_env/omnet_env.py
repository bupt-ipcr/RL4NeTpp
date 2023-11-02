"""
Author       : CHEN Jiawei
Date         : 2023-10-18 20:28:25
LastEditors  : LIN Guocheng
LastEditTime : 2023-10-19 10:38:45
FilePath     : RL4Net++/modules/gym_env/omnet_env.py
Description  : A gym-based implementation, which provides the connection between python and omnetpp.
"""

import contextlib
import os
import signal

import gym
import zmq


class OmnetEnv(gym.Env):
    def __init__(self):
        self.sim_pid = None
        self.sim_proc = None
        self.port = 5555
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REP)
        self.is_multi_agent = True

    def start_zmq_socket(self):
        """start zmq socket"""
        with contextlib.suppress(Exception):
            self.socket.bind(f"tcp://*:{str(self.port)}")

    def reset(self):
        """reset omnetpp environment"""
        print("env resetting ...")
        self.close()
        self.start_zmq_socket()
        self.start_sim()

    def close(self):
        """close all inet and omnetpp process"""
        with contextlib.suppress(Exception):
            os.system("killall inet")
            os.system("killall opp_run_release")

            if self.sim_pid:
                os.killpg(self.sim_pid, signal.SIGUSR1)
                self.sim_proc = None
                self.sim_pid = None

    def start_sim(self):
        cmd = "nohup inet > out.inet &"
        os.system(cmd)

    def get_obs(self):
        """get current state or reward

        Returns:
            string: Flag for state or reward. "s" means state, while "r" means reward.
            int   : Step for the current message.
            list  : State list or reward value
        """
        request = self.socket.recv()
        req = str(request).split("@@")
        s_or_r = req[0][2:]
        step = int(req[1])
        if not self.is_multi_agent and s_or_r == "s":
            msg = [float(state) for state in req[2][:-1].split(",")]
        else:
            msg = req[2][:-1]

        return s_or_r, step, msg

    def get_states(self, state_str):
        """convert state messages to state matrix

        Args:
            state_str (string): state message

        Returns:
            list: state matrix
        """
        return [float(state) for state in state_str.split(",")]

    def get_rewards(self, reward_str):
        """convert reward messages to reward variable

        Args:
            reward_str (string): reward messages

        Returns:
            float: value of reward
        """
        return float(reward_str)

    def get_done(self):
        """get whether the environment finished

        Returns:
            bool: whether the environment finished
        """
        return False

    def get_info(self):
        """get some extra information

        Returns:
            dictionary: extra information if need
        """
        return {}

    def step(self, action):
        """make action and get state or reward from omnetpp

        Args:
            action (List): List of forwarding probability

        Returns:
            list:       next state
            list:       latency and loss rate
            bool:       whether the episode done
            dictionary: some extra information
        """
        self.make_action(list(action))
        s_, r = self.get_obs()
        done = self.get_done()
        info = self.get_info()
        return s_, r, done, info

    def make_action(self, action):
        """make action in omnetpp

        Args:
            action (string): forwarding probability of all links
        """
        self.socket.send_string(action)

    def render(self):
        pass

    def end_epsode(self):
        """inform omnetpp of the end of the current episode"""
        self.socket.send_string("end episode")

    def reward_rcvd(self):
        """inform omnetpp that python has already gotten the reward"""
        self.socket.send_string("reward received")
