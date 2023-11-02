import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.StandardHost;
import inet.node.inet.Router;
import ned.DatarateChannel;
import inet.node.ethernet.Eth1G;

network Nsfnet
{
    parameters:
        @display("p=10,10;b=712,152");
    types:
        channel C extends DatarateChannel
        {
            delay = 0.002s;
            datarate = 1Mbps;
        }
    submodules:
        H0: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H1: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H2: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H3: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H4: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H5: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H6: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H7: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H8: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H9: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H10: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H11: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        H12: StandardHost {
            parameters:
                forwarding = true;
                @display("p=250,150;i=device/laptop");
            gates:
                ethg[];
        }

        configurator: Ipv4NetworkConfigurator {
            parameters:
                addDefaultRoutes = false;
                @display("p=100,100;is=s");
        }
        R0: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R1: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R2: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R3: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R4: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R5: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R6: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R7: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R8: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R9: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R10: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R11: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        R12: Router {
            parameters:
                hasOspf = true;
                // hasRip = true;
                @display("p=250,200");
        }

        connections:
            R0.pppg++ <--> C <--> R2.pppg++;
            R0.pppg++ <--> C <--> R7.pppg++;
            R0.pppg++ <--> C <--> R11.pppg++;

            R1.pppg++ <--> C <--> R2.pppg++;
            R1.pppg++ <--> C <--> R4.pppg++;


            R3.pppg++ <--> C <--> R12.pppg++;

            R4.pppg++ <--> C <--> R12.pppg++;

            R5.pppg++ <--> C <--> R6.pppg++;
            R5.pppg++ <--> C <--> R9.pppg++;

            R6.pppg++ <--> C <--> R7.pppg++;
            R6.pppg++ <--> C <--> R12.pppg++;


            R8.pppg++ <--> C <--> R9.pppg++;

            R9.pppg++ <--> C <--> R11.pppg++;

            R10.pppg++ <--> C <--> R11.pppg++;

            R11.pppg++ <--> C <--> R12.pppg++;


            R0.ethg++ <--> Eth1G <--> H0.ethg++;
            R1.ethg++ <--> Eth1G <--> H1.ethg++;
            R2.ethg++ <--> Eth1G <--> H2.ethg++;
            R3.ethg++ <--> Eth1G <--> H3.ethg++;
            R4.ethg++ <--> Eth1G <--> H4.ethg++;
            R5.ethg++ <--> Eth1G <--> H5.ethg++;
            R6.ethg++ <--> Eth1G <--> H6.ethg++;
            R7.ethg++ <--> Eth1G <--> H7.ethg++;
            R8.ethg++ <--> Eth1G <--> H8.ethg++;
            R9.ethg++ <--> Eth1G <--> H9.ethg++;
            R10.ethg++ <--> Eth1G <--> H10.ethg++;
            R11.ethg++ <--> Eth1G <--> H11.ethg++;
            R12.ethg++ <--> Eth1G <--> H12.ethg++;
}