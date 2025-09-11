# ROB slots: 64 -> 256
# decodes/issues/retires (4/4/4) per cycle -> 6/6/6
# branch predictor entries 32 -> 4096
# tlb entries 64 -> 1024
# page size 4KB to 2MB
# mem bw 200-400 gb/s
# lsq load 16 -> 64
# lsq stores 8 -> 32
# int units 2 -> 4
# fp units 2 -> 4
# fp cycles 8 -> 6

# VVVVVVVVVVVVVVVVVVVVVV
# mesh router - kingsley
# Params: 10 iter

#v2
# keep l1
# l2 -> 256kb
# de, is, re -> 2,2,2
# int unit -> 2
# fp  unit -> 2
# ROB -> 64

#v3 - least
# intel sapphire rapid
# 

# total of 48

# ========
# 3 - cores
# 64 -> 1
# cg, bicg
# 10 iters

# =======
# 2 iters
# w/o RoCC
# 16, 32, 64 cores

import os
import sst

# -------------------- Global options --------------------
mh_debug_level = 10
mh_debug = 0
dbgAddr = "0"
stopDbg = "0"

checkpointDir = ""
checkpoint = ""

pythonDebug = False

vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
isa = "riscv64"

loader_mode = os.getenv("VANADIS_LOADER_MODE", "0")

physMemSize = "8GiB"
tlbType = "simpleTLB"
mmuType = "simpleMMU"

# Define SST core options
sst.setProgramOption("timebase", "1ps")
# NOTE: 0 ns stops instantly; comment or set a small window while debugging
# sst.setProgramOption("stop-at", "1 ms")

# Stats
sst.setStatisticLoadLevel(4)
sst.setStatisticOutput("sst.statOutputConsole")

# -------------------- App / Vanadis knobs --------------------
full_exe_name = os.getenv("VANADIS_EXE")
exe_name = full_exe_name.split("/")[-1] if full_exe_name else ""

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = int(os.getenv("VANADIS_OS_VERBOSE", verbosity))
pipe_trace_file = os.getenv("VANADIS_PIPE_TRACE", "")
lsq_ld_entries = int(os.getenv("VANADIS_LSQ_LD_ENTRIES", 16))
lsq_st_entries = int(os.getenv("VANADIS_LSQ_ST_ENTRIES", 8))

rob_slots = int(os.getenv("VANADIS_ROB_SLOTS", 256))
retires_per_cycle = int(os.getenv("VANADIS_RETIRES_PER_CYCLE", 6))
issues_per_cycle = int(os.getenv("VANADIS_ISSUES_PER_CYCLE", 6))
decodes_per_cycle = int(os.getenv("VANADIS_DECODES_PER_CYCLE", 6))

integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 2))
integer_arith_units = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 4))
fp_arith_cycles = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 6))
fp_arith_units = int(os.getenv("VANADIS_FP_ARITH_UNITS", 4))
branch_arith_cycles = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2))

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")
numCpus = int(os.getenv("VANADIS_NUM_CORES", 1))
numThreads = int(os.getenv("VANADIS_NUM_HW_THREADS", 1))

vanadis_cpu_type = "vanadis." + os.getenv("VANADIS_CPU_ELEMENT_NAME", "dbg_VanadisCPU")

app_args = os.getenv("VANADIS_EXE_ARGS", "")

app_params = {}
if app_args != "":
    app_args_list = app_args.split(" ")
    app_params["argc"] = len(app_args_list) + 1
    if verbosity > 0:
        print(f"Identified {app_params['argc']} application arguments")
    arg_start = 1
    for next_arg in app_args_list:
        app_params["arg" + str(arg_start)] = next_arg
        arg_start += 1
else:
    app_params["argc"] = 1

vanadis_decoder = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = "vanadis.Vanadis" + vanadis_isa + "OSHandler"

# -------------------- RoCC / Array --------------------
rocc_type = os.getenv("GOLEM_ROCC_TYPE", "golem.RoCCAnalogFloat")
array_type = os.getenv("GOLEM_ARRAY_TYPE")
num_arrays = int(os.getenv("GOLEM_NUM_ARRAYS", 1))
array_input_size = os.getenv("ARRAY_INPUT_SIZE")
array_output_size = os.getenv("ARRAY_OUTPUT_SIZE")

protocol = "MESI"

# -------------------- OS --------------------
osParams = {
    "processDebugLevel": 0,
    "dbgLevel": os_verbosity,
    "dbgMask": 8,
    "cores": numCpus,
    "hardwareThreadCount": numThreads,
    "page_size": 4096,
    "physMemSize": physMemSize,
    "useMMU": True,
    "checkpointDir": checkpointDir,
    "checkpoint": checkpoint,
}

processList = (
    (
        1,
        {
            "env_count": 1,
            "env0": f"OMP_NUM_THREADS={numCpus*numThreads}",
            "exe": full_exe_name,
            "arg0": exe_name,
        },
    ),
)
processList[0][1].update(app_params)

# -------------------- Caches / MMU / Router params --------------------
osl1cacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

mmuParams = {
    "debug_level": 0,
    "num_cores": numCpus,
    "num_threads": numThreads,
    "page_size": 4096,
}

# Merlin router base params (we'll override num_ports dynamically)
memRtrParams = {
    "xbar_bw": "100GB/s",
    "link_bw": "100GB/s",
    "input_buf_size": "2KB",
    "output_buf_size": "2KB",
    "flit_size": "256B",
    "id": "0",
    "num_vns": 3,  # match NIC groups (e.g., l2/group=1, dir/group=2)
}

dirCtrlParams = {
    "coherence_protocol": protocol,
    "entry_cache_size": "1024",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
    "addr_range_start": "0x0",
    "addr_range_end": "0xFFFFFFFF",
}

dirNicParams = {
    "network_bw": "50GB/s",
    "group": 2,  # VN 2
}

memCtrlParams = {
    "clock": cpu_clock,
    "backend.mem_size": physMemSize,
    "backing": "malloc",
    "initBacking": 1,
    "addr_range_start": 0,
    "addr_range_end": 0xFFFFFFFF,
    "debug_level": mh_debug_level,
    "debug": mh_debug,
    "checkpointDir": checkpointDir,
    "checkpoint": checkpoint,
}

memParams = {
    "mem_size": "4GiB",
    "access_time": "1 ns",
}

tlbParams = {
    "debug_level": 0,
    "hitLatency": 1,
    "num_hardware_threads": numThreads,
    "num_tlb_entries_per_thread": 1024,
    "tlb_set_size": 4,
}

tlbWrapperParams = {
    "debug_level": 0,
}

decoderParams = {
    "loader_mode": loader_mode,
    "uop_cache_entries": 1536,
    "predecode_cache_entries": 4,
}

osHdlrParams = {}

branchPredParams = {
    "branch_entries": 4096,
}

cpuParams = {
    "clock": cpu_clock,
    "verbose": verbosity,
    "hardware_threads": numThreads,
    "physical_fp_registers": 168 * numThreads,
    "physical_integer_registers": 180 * numThreads,
    "integer_arith_cycles": integer_arith_cycles,
    "integer_arith_units": integer_arith_units,
    "fp_arith_cycles": fp_arith_cycles,
    "fp_arith_units": fp_arith_units,
    "branch_unit_cycles": branch_arith_cycles,
    "print_int_reg": False,
    "print_fp_reg": False,
    "pipeline_trace_file": pipe_trace_file,
    "reorder_slots": rob_slots,
    "decodes_per_cycle": decodes_per_cycle,
    "issues_per_cycle": issues_per_cycle,
    "retires_per_cycle": retires_per_cycle,
    "pause_when_retire_address": int(os.getenv("VANADIS_HALT_AT_ADDRESS", 0)),
    "start_verbose_when_issue_address": dbgAddr,
    "stop_verbose_when_retire_address": stopDbg,
    "print_rob": False,
    "checkpointDir": checkpointDir,
    "checkpoint": checkpoint,
}

lsqParams = {
    "verbose": verbosity,
    "address_mask": 0xFFFFFFFF,
    "max_stores": lsq_st_entries,
    "max_loads": lsq_ld_entries,
}

roccParams = {
    "clock": cpu_clock,
    "verbose": verbosity,
    "max_instructions": 8,
}

arrayParams = {
    "arrayLatency": "100ns",
    "clock": cpu_clock,
    "max_instructions": 8,
    "verbose": verbosity,
    "mmioAddr": 0,
    "numArrays": num_arrays,
    "arrayInputSize": array_input_size,
    "arrayOutputSize": array_output_size,
}

roccarrayParams = {
    "inputOperandSize": 4,
    "outputOperandSize": 4,
}

roccParams.update(roccarrayParams)
arrayParams.update(roccarrayParams)
roccParams.update(arrayParams)

l1dcacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

l1icacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
#    "prefetcher": "cassini.NextBlockPrefetcher",
#    "prefetcher.reach": 1,
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

l2cacheParams = {
    "access_latency_cycles": "14",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "16",
    "cache_line_size": "64",
    "cache_size": "1MB",
    "mshr_latency_cycles": 3,
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

busParams = {"bus_frequency": cpu_clock}

l2memLinkParams = {
    "group": 1,             # VN 1
    "network_bw": "50GB/s",
}

# -------------------- Builder --------------------
class CPU_Builder:
    def __init__(self):
        pass

    def build(self, prefix, nodeId, cpuId):
        if pythonDebug:
            print(f"build {prefix}")

        # CPU
        cpu = sst.Component(prefix, vanadis_cpu_type)
        cpu.addParams(cpuParams)
        cpu.addParam("core_id", cpuId)
        cpu.enableAllStatistics()

        # decoders + OS handler + branch
        for n in range(numThreads):
            decode = cpu.setSubComponent("decoder" + str(n), vanadis_decoder)
            decode.addParams(decoderParams)
            decode.enableAllStatistics()

            os_hdlr = decode.setSubComponent("os_handler", vanadis_os_hdlr)
            os_hdlr.addParams(osHdlrParams)

            branch_pred = decode.setSubComponent("branch_unit", "vanadis.VanadisBasicBranchUnit")
            branch_pred.addParams(branchPredParams)
            branch_pred.enableAllStatistics()

        # LSQ
        cpu_lsq = cpu.setSubComponent("lsq", "vanadis.VanadisBasicLoadStoreQueue")
        cpu_lsq.addParams(lsqParams)
        cpu_lsq.enableAllStatistics()

        # RoCC + Array
        cpu_rocc = cpu.setSubComponent("rocc", rocc_type, 0)
        cpu_rocc.addParams(roccParams)
        cpu_rocc.enableAllStatistics()

        computeArray = cpu_rocc.setSubComponent("array", array_type)
        computeArray.addParams(arrayParams)

        # Mem IFs
        cpuDcacheIf = cpu_lsq.setSubComponent("memory_interface", "memHierarchy.standardInterface")
        roccDcacheIf = cpu_rocc.setSubComponent("memory_interface", "memHierarchy.standardInterface")
        cpuIcacheIf = cpu.setSubComponent("mem_interface_inst", "memHierarchy.standardInterface")

        # L1 D
        cpu_l1dcache = sst.Component(prefix + ".l1dcache", "memHierarchy.Cache")
        cpu_l1dcache.addParams(l1dcacheParams)
        cpu_l1dcache.enableAllStatistics()
        l1dcache_2_cpu = cpu_l1dcache.setSubComponent("highlink", "memHierarchy.MemLink")
        l1dcache_2_l2cache = cpu_l1dcache.setSubComponent("lowlink", "memHierarchy.MemLink")

        # L1 I
        cpu_l1icache = sst.Component(prefix + ".l1icache", "memHierarchy.Cache")
        cpu_l1icache.addParams(l1icacheParams)
        l1icache_2_cpu = cpu_l1icache.setSubComponent("highlink", "memHierarchy.MemLink")
        l1icache_2_l2cache = cpu_l1icache.setSubComponent("lowlink", "memHierarchy.MemLink")

        # L2
        cpu_l2cache = sst.Component(prefix + ".l2cache", "memHierarchy.Cache")
        cpu_l2cache.addParams(l2cacheParams)
        l2cache_2_l1caches = cpu_l2cache.setSubComponent("highlink", "memHierarchy.MemLink")
        l2cache_2_mem = cpu_l2cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
        l2cache_2_mem.addParams(l2memLinkParams)

        # L1 bus (D+I)
        cache_bus = sst.Component(prefix + ".bus", "memHierarchy.Bus")
        cache_bus.addParams(busParams)

        # Processor bus (CPU + RoCC -> D-side)
        processor_bus = sst.Component(prefix + ".processorBus", "memHierarchy.Bus")
        processor_bus.addParams(busParams)

        # TLBs
        dtlbWrapper = sst.Component(prefix + ".dtlb", "mmu.tlb_wrapper")
        dtlbWrapper.addParams(tlbWrapperParams)
        dtlb = dtlbWrapper.setSubComponent("tlb", "mmu." + tlbType)
        dtlb.addParams(tlbParams)

        itlbWrapper = sst.Component(prefix + ".itlb", "mmu.tlb_wrapper")
        itlbWrapper.addParams(tlbWrapperParams)
        itlbWrapper.addParam("exe", True)
        itlb = itlbWrapper.setSubComponent("tlb", "mmu." + tlbType)
        itlb.addParams(tlbParams)

        # Wiring inside the core tile
        link_lsq_l1dcache_link = sst.Link(prefix + ".link_cpu_dbus_link")
        link_lsq_l1dcache_link.connect((cpuDcacheIf, "lowlink", "1ns"), (processor_bus, "high_network_0", "1ns"))
        link_lsq_l1dcache_link.setNoCut()

        link_rocc_l1dcache_link = sst.Link(prefix + ".link_rocc_dbus_link")
        link_rocc_l1dcache_link.connect((roccDcacheIf, "lowlink", "1ns"), (processor_bus, "high_network_1", "1ns"))
        link_rocc_l1dcache_link.setNoCut()

        link_bus_l1cache_link = sst.Link(prefix + ".link_bus_l1cache_link")
        link_bus_l1cache_link.connect((processor_bus, "low_network_0", "1ns"), (dtlbWrapper, "cpu_if", "1ns"))
        link_bus_l1cache_link.setNoCut()

        link_cpu_l1dcache_link = sst.Link(prefix + ".link_cpu_l1dcache_link")
        link_cpu_l1dcache_link.connect((dtlbWrapper, "cache_if", "1ns"), (l1dcache_2_cpu, "port", "1ns"))
        link_cpu_l1dcache_link.setNoCut()

        link_cpu_itlb_link = sst.Link(prefix + ".link_cpu_itlb_link")
        link_cpu_itlb_link.connect((cpuIcacheIf, "lowlink", "1ns"), (itlbWrapper, "cpu_if", "1ns"))
        link_cpu_itlb_link.setNoCut()

        link_cpu_l1icache_link = sst.Link(prefix + ".link_cpu_l1icache_link")
        link_cpu_l1icache_link.connect((itlbWrapper, "cache_if", "1ns"), (l1icache_2_cpu, "port", "1ns"))
        link_cpu_l1icache_link.setNoCut()

        link_l1dcache_l2cache_link = sst.Link(prefix + ".link_l1dcache_l2cache_link")
        link_l1dcache_l2cache_link.connect((l1dcache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_0", "1ns"))
        link_l1dcache_l2cache_link.setNoCut()

        link_l1icache_l2cache_link = sst.Link(prefix + ".link_l1icache_l2cache_link")
        link_l1icache_l2cache_link.connect((l1icache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_1", "1ns"))
        link_l1icache_l2cache_link.setNoCut()

        link_bus_l2cache_link = sst.Link(prefix + ".link_bus_l2cache_link")
        link_bus_l2cache_link.connect((cache_bus, "low_network_0", "1ns"), (l2cache_2_l1caches, "port", "1ns"))
        link_bus_l2cache_link.setNoCut()

        # Return endpoints to wire outside
        return (cpu, "os_link", "5ns"), (l2cache_2_mem, "port", "1ns"), (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns")


def addParamsPrefix(prefix, params):
    ret = {}
    for key, value in params.items():
        ret[prefix + "." + key] = value
    return ret

# -------------------- OS + MMU + OS L1 --------------------
node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams(osParams)

num = 0
for i, process in processList:
    for _ in range(i):
        node_os.addParams(addParamsPrefix("process" + str(num), process))
        num += 1

node_os_mmu = node_os.setSubComponent("mmu", "mmu." + mmuType)
node_os_mmu.addParams(mmuParams)

node_os_mem_if = node_os.setSubComponent("mem_interface", "memHierarchy.standardInterface")

os_cache = sst.Component("node_os.cache", "memHierarchy.Cache")
os_cache.addParams(osl1cacheParams)
os_cache_2_mem = os_cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
os_cache_2_mem.addParams(l2memLinkParams)  # VN 1
link_os_cache_link = sst.Link("link_os_cache_link")
link_os_cache_link.connect( (node_os_mem_if, "lowlink", "1ns"), (os_cache, "highlink", "1ns") )

# -------------------- Merlin Router + Mesh topo --------------------
# Mesh parameters (1x1 router, but mesh reserves neighbor ports N/S/E/W)
mesh_wx, mesh_wy = 1, 1
neighbor_ports = 2 * (mesh_wx + mesh_wy)      # N,S,E,W = 4
local_ports = numCpus + 2                      # CPUs + DIR + OS = locals
local_base = neighbor_ports                    # locals start after neighbors
required_ports = neighbor_ports + local_ports  # total router ports

comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
# base router params + computed port count
rp = dict(memRtrParams)
rp["num_ports"] = str(required_ports)
comp_chiprtr.addParams(rp)

topo = comp_chiprtr.setSubComponent("topology", "merlin.mesh")
topo.addParams({
    "shape": "1x1",
    "width": f"{mesh_wx}x{mesh_wy}",
    "local_ports": local_ports,
    "link_latency": "1ns",
})

# -------------------- Directory + Memory --------------------
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams(dirCtrlParams)
dirtoM = dirctrl.setSubComponent("lowlink", "memHierarchy.MemLink")
dirNIC = dirctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
dirNIC.addParams(dirNicParams)  # VN 2

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams(memCtrlParams)
memToDir = memctrl.setSubComponent("highlink", "memHierarchy.MemLink")
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams(memParams)

# -------------------- Wire DIR <-> MEM --------------------
link_dir_2_mem = sst.Link("link_dir_2_mem")
link_dir_2_mem.connect((dirtoM, "port", "1ns"), (memToDir, "port", "1ns"))
link_dir_2_mem.setNoCut()

# -------------------- Build CPUs + connect to mesh + OS/MMU --------------------
cpuBuilder = CPU_Builder()

nodeId = 0
for cpu in range(numCpus):
    prefix = f"node{nodeId}.cpu{cpu}"
    os_hdlr, l2cache, dtlb, itlb = cpuBuilder.build(prefix, nodeId, cpu)

    # MMU -> core TLBs
    link_mmu_dtlb_link = sst.Link(prefix + ".link_mmu_dtlb_link")
    link_mmu_dtlb_link.connect((node_os_mmu, "core" + str(cpu) + ".dtlb", "1ns"), dtlb)

    link_mmu_itlb_link = sst.Link(prefix + ".link_mmu_itlb_link")
    link_mmu_itlb_link.connect((node_os_mmu, "core" + str(cpu) + ".itlb", "1ns"), itlb)

    # CPU OS handler -> Node OS
    link_core_os_link = sst.Link(prefix + ".link_core_os_link")
    link_core_os_link.connect(os_hdlr, (node_os, "core" + str(cpu), "5ns"))

    # CPU L2 MemNIC -> Router local port
    link_l2cache_2_rtr = sst.Link(prefix + ".link_l2cache_2_rtr")
    link_l2cache_2_rtr.connect(l2cache, (comp_chiprtr, f"port{local_base + cpu}", "1ns"))
    link_l2cache_2_rtr.setNoCut()

# Directory NIC -> Router local port
link_dir_2_rtr = sst.Link("link_dir_2_rtr")
link_dir_2_rtr.connect((comp_chiprtr, f"port{local_base + numCpus}", "1ns"), (dirNIC, "port", "1ns"))
link_dir_2_rtr.setNoCut()

# OS cache NIC -> Router local port
os_cache_2_rtr = sst.Link("os_cache_2_rtr")
os_cache_2_rtr.connect((os_cache_2_mem, "port", "1ns"), (comp_chiprtr, f"port{local_base + numCpus + 1}", "1ns"))
os_cache_2_rtr.setNoCut()

