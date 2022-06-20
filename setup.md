**Note**: We used a internal development version of scone to run Treaty. The code is not reviewed and designed to run in a production system.


### Tested configuration

Host hardware

    Intel Core i9-10900K CPU
        8 cores (3.7 GHz)
        SGX v1
    Intel XL710 40GbE controller

Host software (please check default.nix for the dependencies)

    NixOS (kernel v5.11.21)

Interconnect

    40GbE

### Building
First build DPDK and eRPC libraries (assuming all dependencies are installed/build already).

```
cd eRPC/build
make -f Makefile_dpdk 
cmake  ..  -DPERF=OFF -DTRANSPORT=dpdk
```
For SCONE compilations use the Makefile_dpdk_scone and the flag -DSCONE respectively.

To build the application:

```
cd eRPC/2pc-eRPC
make -f Makefile_small_cluster
```
Use Makefile_small_cluster_SCONE for SCONE builds respectively.

The compilation will produce three binaries (donna, martha, rose) where you can run in the respective nodes.

You might need to adapt the IPs in ```eRPC/2pc-eRPC/apps/small-cluster/common_conf.h```.

Lastly, note that this is the code for secure Txs protocols. You can run it with or without a storage engine by setting the flag -DSTORAGE respectively in the Makefile.
Based on the storage layer you might want to use the code/scripts might require adaptations.

### Workload


## Contact
For any questions please send an email to dimitra.giantsidi@gmail.com
