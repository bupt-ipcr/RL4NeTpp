"""
Author       : CHEN Jiawei
Date         : 2023-10-19 09:51:55
LastEditors  : LIN Guocheng
LastEditTime : 2023-10-19 16:19:34
FilePath     : /home/lgc/test/RL4Net++/utils/get_ned.py
Description  : Convert a gml file to a network topology file for OMNeT++ simulation based on the URL of the gml file.
"""

import requests
import re


def get_omnetpp_ned(gml_url: str) -> None:
    """Reads gml file information from a given URL.

    Args:
        gml_url (str): URL of the gml file
    """
    if not gml_url:
        print("\033[0;31m Error! gml_url is None! \033[0m")
        return
    topo_name = re.search(r"\/(\w+)\.gml", gml_url)[1]
    gml_info = requests.get(gml_url).content.decode()
    node_info_strs = re.findall(r"node \[.*?\]", gml_info, re.S)
    node_num = len(node_info_strs)
    edge_info_str = re.findall(r"edge \[.*?\]", gml_info, re.S)
    edge_info = [
        {
            value[0]: value[1]
            for value in re.findall(r"\s+([^\s]{2,}?)\s+([^\[\]]+?)\n", e)
        }
        for e in edge_info_str
    ]
    edges = []
    bothway_edges = []
    for info in edge_info:
        edges.append((int(info["source"]), int(info["target"])))
        bothway_edges.extend(
            (
                (int(info["source"]), int(info["target"])),
                (int(info["target"]), int(info["source"])),
            )
        )
    edges = sorted(edges, key=lambda x: (x[0], x[1]))
    bothway_edges = sorted(bothway_edges, key=lambda x: (x[0], x[1]))
    edges = [
        [edge for edge in edges if edge[0] == i] for i in range(int(edges[-1][0]) + 1)
    ]
    bothway_edges = [
        [bothway_edge for bothway_edge in bothway_edges if bothway_edge[0] == i]
        for i in range(int(bothway_edges[-1][0]) + 1)
    ]
    print(
        f"\033[0;33m Topo: {topo_name} has {node_num} nodes and {len(edge_info)} edges. \033[0m"
    )
    write_ned(topo_name, node_num, edges)
    write_init_prob_file(topo_name, node_num, bothway_edges)
    print(
        f"\033[0;32m Congratulations! config/ned/{topo_name}.ned and config/ned/{topo_name}.txt were successfully generated!  \033[0m"
    )


def write_ned(
    topo_name: str,
    node_num: int,
    edges: list,
    bandwidth: float = 1,
    link_delay: float = 0.002,
) -> None:
    """Converts the read information into a .ned file.

    Args:
        topo_name (str):                name of network topology
        node_num (int):                 number of router nodes
        edges (list):                   network topology link information, where each element represents a bi-directional link
        bandwidth (float, optional):    bandwidth of the network. Defaults to 1.
        link_delay (float, optional):   delay of network link. Defaults to 0.002.
    """

    with open(f"config/ned/{topo_name}.ned", "w") as f:
        print("\033[0;33m Starting Generating config/ned/" + topo_name + ".ned \033[0m")
        f.write("import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;\n")
        f.write("import inet.node.inet.StandardHost;\n")
        f.write("import inet.node.inet.Router;\n")
        f.write("import ned.DatarateChannel;\n")
        f.write("import inet.node.ethernet.Eth1G;\n\n")
        f.write(f"network {topo_name}\n")
        f.write("{\n")
        f.write("    parameters:\n")
        f.write('        @display("p=10,10;b=712,152");\n')
        f.write("    types:\n")
        f.write("        channel C extends DatarateChannel\n")
        f.write("        {\n")
        f.write(f"            delay = {link_delay}s;\n")
        f.write(f"            datarate = {bandwidth}Mbps;\n")
        f.write("        }\n")
        f.write("    submodules:\n")
        for node_index in range(node_num):
            f.write(f"        H{node_index}" + ": StandardHost {\n")
            f.write("            parameters:\n")
            f.write("                forwarding = true;\n")
            f.write('                @display("p=250,150;i=device/laptop");\n')
            f.write("            gates:\n")
            f.write("                ethg[];\n")
            f.write("        }\n\n")
        f.write("        configurator: Ipv4NetworkConfigurator {\n")
        f.write("            parameters:\n")
        f.write("                addDefaultRoutes = false;\n")
        f.write('                @display("p=100,100;is=s");\n')
        f.write("        }\n")
        for node_index in range(node_num):
            f.write(f"        R{node_index}" + ": Router {\n")
            f.write("            parameters:\n")
            f.write("                hasOspf = true;\n")
            f.write("                // hasRip = true;\n")
            f.write('                @display("p=250,200");\n')
            f.write("        }\n\n")
        f.write("        connections:\n")
        for edge_ in edges:
            for edge in edge_:
                f.write(
                    f"            R{edge[0]}.pppg++ <--> C <--> R{edge[1]}.pppg++;\n"
                )
            f.write("\n")
        f.write("\n")
        for node_index in range(node_num):
            f.write(
                f"            R{node_index}.ethg++ <--> Eth1G <--> H{node_index}.ethg++;\n"
            )
        f.write("}")
    print(
        "\033[0;32m config/ned/"
        + topo_name
        + ".ned was successfully generated! \033[0m"
    )


def write_init_prob_file(topo_name: str, node_num: int, bothway_edges: list) -> None:
    """Write the initial probability .txt file for the network,
       at which point the forwarding probabilities for each link are equal.

    Args:
        topo_name (str):        name of network topology
        node_num (int):         number of router nodes
        bothway_edges (list):   network topology link information, where each element represents a unidirectional link
    """
    init_prob = sum(
        (
            [
                100 // len(edge) if i in [t[1] for t in edge] else 0
                for i in range(node_num)
            ]
            for edge in bothway_edges
        ),
        [],
    )
    with open(f"config/ned/{topo_name}.txt", "w") as f:
        print("\033[0;33m Starting Generating config/ned/" + topo_name + ".txt \033[0m")
        f.write(",".join(str(i) for i in init_prob))
    print(f"\033[0;32m {topo_name}.txt was successfully generated! \033[0m")


if __name__ == "__main__":
    gml_url = "http://topology-zoo.org/files/Gridnet.gml"
    get_omnetpp_ned(gml_url)
