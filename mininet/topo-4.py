"""
Copyright (c) 2023. ByteDance Inc. All rights reserved.
A 4 way topo:

client1-----left switch ====Bottle Link =====rightswitch----... server nodes
              |
client2-------

In this script we will change the bw, delay and queue length.

This script use sub process to start up nodes. You may also directly run with xterm or screen.
"""
from time import sleep
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import TCLink
from mininet.log import info, setLogLevel, warning
import shlex
import os

curr_dir = os.path.join(os.getcwd(), "mininet")
# curr_dir = '/home/xzc/project/mmsys23lib/cmake-build-debug/'
"""
we assume  curr_dir/ has following struct
../
  bin/servertest datanode program
   ../MPDtest download program
   ../upnode_mn0.json config file for data node 0
   ../upnode_mn1.json config file for data node 1
   ../...........more data nodes
   ../downnode_mn.json download node config file
"""


class DumbbellTopo(Topo):
    """Dumbbell: client-----left switch ====Bottle Link =====rightswitch----... server nodes"""

    def __init__(self, *args, **kwargs):
        self.right_router = None
        self.left_router = None
        self.server_nodes = None
        self.client_node = None
        self.switch_idx = 0
        super().__init__(*args, **kwargs)

    def addTCLink(self, node1, node2, **opts):
        """
        It's not a good idea to set both bw and loss rate at same link.
        We have to split the link and set bw and loss rate separately
        See https://ipmininet.readthedocs.io/en/latest/tc.html for more details

        Connect node1 with node 2. bw will be set on one interface and other paras will be set on another interface
        You may set node1-->node2 and node2-->node1 respectively with opts['params1'] and opts['params2']
        :param loss1: loss in node1--->node2
        :param loss2: loss in node2--->node1
        :param node1:
        :param node2:
        :param bw:
        :param delay:
        :param max_queue_size: in packet
        :return:
        """
        delay = opts.get("delay")
        bw = opts.get("bw")
        loss1 = opts.get("loss1")
        loss2 = opts.get("loss2")
        max_queue_size = opts.get("max_queue_size")
        if delay is None:
            delay = 2

        # break node1<====> node2 into node1<===>bridge<====>node2
        # connect left part
        n1_bridge_link_opt = {"bw": bw}
        n1_bridge_opts = {"params1": n1_bridge_link_opt}  # node1--> bridge
        bridge_n1_link_opt = {"delay": delay, "loss": loss2, "max_queue_size": max_queue_size}  # n1<-------bridge
        n1_bridge_opts["params2"] = bridge_n1_link_opt
        # connect right part
        bridge_n2_link_opt = {"delay": delay, "loss": loss1, "max_queue_size": max_queue_size}  # bridge--->n2
        bridge_n2_opts = {"params1": bridge_n2_link_opt}
        n2_bridge_link_opt = {"bw": bw}  # beidge<----n2
        bridge_n2_opts["params2"] = n2_bridge_link_opt
        bridge = self.addSwitch(f's{self.switch_idx}')
        self.switch_idx += 1
        print(f'add link {node1}<--->{node2} with opts{n1_bridge_opts} and opts{bridge_n2_opts}')
        return super().addLink(node1, bridge, cls=TCLink, **n1_bridge_opts), super().addLink(bridge, node2, cls=TCLink,
                                                                                             **bridge_n2_opts)

    def build(self, server_number, client_link_opt: dict, bot_nec_link_opt: dict, server_link_opts: [dict]):
        info(f'****Add 1 client node')

        client_node1 = self.addHost("client1", ip=f'10.100.0.1')
        client_node2 = self.addHost("client2", ip=f'10.100.0.2')
        info('*** Adding switches')
        left_router = self.addSwitch('s1')
        right_router = self.addSwitch('s2')

        server_nodes = []
        info(f'****Add {server_num} server nodes\n')
        for i in range(server_num):
            s = self.addHost(name=f'server{i}', ip=f'10.100.2.{i}')
            print(type(s))
            server_nodes.append(s)
        self.switch_idx = 3
        info('*** Creating links\n')
        # connect client and left router
        self.addTCLink(client_node1, left_router, **client_link_opt)
        self.addTCLink(client_node2, left_router, **client_link_opt)
        # connect left and right router
        self.addTCLink(left_router, right_router, **bot_nec_link_opt)
        # connect right router and server

        for i in range(server_num):
            self.addTCLink(right_router, server_nodes[i], **server_link_opts[i])
        info(" network connected ")
        self.client_node = [client_node1, client_node2]
        self.server_nodes = server_nodes
        self.left_router = left_router
        self.right_router = right_router


if __name__ == '__main__':
    setLogLevel('info')

    # config
    client_left_opts = {"bw": 1000, "delay": "10ms", "max_queue_size": 200}
    btlink_opts = {"bw": 10, "delay": "30ms", "loss1": 1, "loss2": 1, "max_queue_size": 100}
    serverlink_opts = [
        {"delay": "5ms"},
        {"delay": "10ms"},
        {"delay": "15ms"},
        {"delay": "20ms"}
    ]
    server_num = len(serverlink_opts)

    topo = DumbbellTopo(server_number=server_num,
                        client_link_opt=client_left_opts,
                        bot_nec_link_opt=btlink_opts,
                        server_link_opts=serverlink_opts)
    net = Mininet(controller=Controller, topo=topo)

    info('*** Starting network\n')
    net.start()
    # we can use "net.iperf()" to start a tcp flow
    info('*** Testing connectivity\n')
    # net.ping(server_nodes + [tracker_node] + [client_node])

    info('start servers\n')
    # Start servertest program in data node, redirect the stdout to server{i}stdout
    server_proc_ls = []
    server_std_f_ls = []
    server_nodes = []
    for node_name in topo.server_nodes:
        server_nodes.append(net.getNodeByName(node_name))
    for i in range(server_num):
        # use absolute dir to start servertest program, and the parameter is the upnode config json file
        server_start_cmd = f'{curr_dir}/bin/servertest {curr_dir}/bin/upnode_mn{i}.json '
        server_std_f = open(f'server{i}stdout', mode='w')
        server_std_f_ls.append(server_std_f)
        server_start_cmd_args = shlex.split(server_start_cmd)
        server_proc = server_nodes[i].popen(server_start_cmd_args, stdout=server_std_f)  # no output
        server_proc_ls.append(server_proc)
        # sleep(1)  # pause for one second
    sleep(3)  # wait for three seconds to finish the connecting process

    info('*** Running CLI\n')
    warning('TYPE exit or CTRL + D to exit!! DO NOT kill the CLI interface.There will be zombie process ')

    # now start two client program. Like server program, we will start two processes
    # start client 1
    client1_node = net.getNodeByName('client1')
    client1_start_cmd = f'{curr_dir}/bin/MPDtest {curr_dir}/bin/downnode_mn{1}.json'
    client1_std_f = open(f'client_{1}_stdout', mode='w')  # redirect std out
    client1_start_cmd_args = shlex.split(client1_start_cmd)
    client1_proc = client1_node.popen(client1_start_cmd_args, stdout=client1_std_f)

    # start client 2
    client2_node = net.getNodeByName('client2')
    client2_start_cmd = f'{curr_dir}/bin/MPDtest {curr_dir}/bin/downnode_mn{2}.json'
    client2_std_f = open(f'client_{2}_stdout', mode='w')  # redirect std out
    client2_start_cmd_args = shlex.split(client2_start_cmd)
    client2_proc = client2_node.popen(client2_start_cmd_args, stdout=client2_std_f)

    # wait for both client program to complete
    while True:
        if client1_proc.poll() is not None and client2_proc.poll() is not None:
            info(' client1 and client2 both exit\n')
            break
        info("client programs is still running...\n")
        sleep(2)

    CLI(net)  # start cmd line interface
    """
    To start download test, in CMD line interface,type: client {absolute dir}/MPDtest downnode_mn.json
    In my computer, it's like,
    client /home/xzc/project/mpfc/cmake-build-debug/bin/MPDtest /home/xzc/project/mpfc/cmake-build-debug/bin/downnode_mn.json
    Once the program is successfully stopped, you may find MPDTrace.txt inside the dir of this python file.
    """

    info('*** Stopping network\n')

    # clean server first
    for server_proc in server_proc_ls:
        server_proc.kill()
    for sf in server_std_f_ls:
        if not sf.closed:
            sf.close()

    # clean client now
    if not client1_std_f.closed:
        client1_std_f.close()
    if not client2_std_f.closed:
        client2_std_f.close()
    sleep(1)
    net.stop()