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
import array
import math

# -------------------- basics --------------------
mh_debug_level=10
mh_debug=0
dbgAddr=0
stopDbg=0
pythonDebug=False

checkpointDir = ""
checkpoint = ""

vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
isa = "riscv64" if vanadis_isa.upper().startswith("RISCV") else "mipsel"
loader_mode = os.getenv("VANADIS_LOADER_MODE", "0")

# ---- app ----
testDir="basic-io"
exe = "hello-world"
physMemSize = "4GiB"

tlbType = "simpleTLB"
mmuType = "simpleMMU"
backingType = "timingDRAM"  # simpleMem, simpleDRAM, timingDRAM

sst.setProgramOption("timebase", "1ps")
sst.setStatisticLoadLevel(100)
sst.setStatisticOutput("sst.statOutputConsole")

full_exe_name = os.getenv("VANADIS_EXE", f"../tests/small/{testDir}/{exe}/{isa}/{exe}")
exe_name = full_exe_name.split("/")[-1]

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = os.getenv("VANADIS_OS_VERBOSE", verbosity)
pipe_trace_file = os.getenv("VANADIS_PIPE_TRACE", "")
lsq_ld_entries = int(os.getenv("VANADIS_LSQ_LD_ENTRIES", 16))
lsq_st_entries = int(os.getenv("VANADIS_LSQ_ST_ENTRIES", 8))

rob_slots = int(os.getenv("VANADIS_ROB_SLOTS", 96))
retires_per_cycle = int(os.getenv("VANADIS_RETIRES_PER_CYCLE", 3))
issues_per_cycle = int(os.getenv("VANADIS_ISSUES_PER_CYCLE", 3))
decodes_per_cycle = int(os.getenv("VANADIS_DECODES_PER_CYCLE", 3))
fetches_per_cycle = int(os.getenv("VANADIS_FETCHES_PER_CYCLE", 8))

integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 3))
integer_arith_units = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 3))
fp_arith_cycles = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 4))
fp_arith_units = int(os.getenv("VANADIS_FP_ARITH_UNITS", 1))
branch_arith_cycles = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2))

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "3.2GHz")
mem_clock = os.getenv("VANADIS_MEM_CLOCK", "1.0GHz")

numThreads = int(os.getenv("VANADIS_NUM_HW_THREADS", 1))

vanadis_cpu_type = "vanadis." + os.getenv("VANADIS_CPU_ELEMENT_NAME","dbg_VanadisCPU")

# -------------------- scale by CPU count --------------------
# CPUs: choose via VANADIS_NUM_CORES or N×N with VANADIS_N
N = int(os.getenv("VANADIS_N", 0))  # if 0, ignore N
if "VANADIS_NUM_CORES" in os.environ:
    numCpus = int(os.getenv("VANADIS_NUM_CORES"))
elif N > 0:
    numCpus = N * N
else:
    numCpus = 1


CPU_PER_MEM = int(os.getenv("VANADIS_CPU_PER_MEM", 8))  # target ratio

# Memories: auto-scale with CPUs unless explicitly overridden
if "VANADIS_NUM_MEMORIES" in os.environ:
    numMemories = int(os.getenv("VANADIS_NUM_MEMORIES"))
else:
    numMemories = max(2, math.ceil(numCpus / max(1, CPU_PER_MEM)))

cpus_per_mem = max(1, math.ceil(numCpus / max(1, numMemories)))
mem_channels = int(os.getenv("VANADIS_MEM_CHANNELS", max(2, min(16, math.ceil(cpus_per_mem / 8)))))
transaction_Q_size = int(os.getenv("VANADIS_MEM_TXQ", 128))#min(256, 32 * max(1, math.ceil(cpus_per_mem / 4)))))

# Total tiles (routers) = 1 OS + CPUs + memories
total_nodes = 1 + numCpus + numMemories

# near-square mesh sized to fit all tiles
mesh_stops_x = int(math.ceil(math.sqrt(total_nodes)))
mesh_stops_y = int(math.ceil(total_nodes / mesh_stops_x))

# placement
# 0 -> OS
# [1 .. numCpus] -> CPUs
# [total_nodes-numMemories .. total_nodes-1] -> memories/directories
cpu_node_list = array.array('i', list(range(1, 1 + numCpus)))
mem_node_list = array.array('i', list(range(total_nodes - numMemories, total_nodes)))

if len(cpu_node_list) != numCpus:
    raise ValueError("cpu_node_list size mismatch")
if len(mem_node_list) != numMemories:
    raise ValueError("mem_node_list size mismatch")

# -------------------- mesh/network params (derive after mesh dims) --------------------
mesh_clock        = 6400 #3200
ctrl_mesh_flit    = 16 #8
data_mesh_flit    = 72  #36
mesh_link_latency = "100ps"
ctrl_mesh_link_bw = f"{mesh_clock*1000*1000*ctrl_mesh_flit}B/s"
data_mesh_link_bw = f"{mesh_clock*1000*1000*data_mesh_flit}B/s"

# Optionally scale input buffers a bit with log2 of CPUs (kept modest)
#buf_scale = max(1, int(math.log2(max(2, numCpus))))  # 1,2,3,...
buf_scale = max(2, int(math.ceil(numCpus/4)))  # 1,2,3,...
ctrl_network_buffers = f"{32*buf_scale}B"
data_network_buffers = f"{288*buf_scale}B"

ctrl_network_params = {
    "link_bw": ctrl_mesh_link_bw,
    "flit_size": f"{ctrl_mesh_flit}B",
    "input_buf_size": ctrl_network_buffers,
#    "port_priority_equal": 1,
}
data_network_params = {
    "link_bw": data_mesh_link_bw,
    "flit_size": f"{data_mesh_flit}B",
    "input_buf_size": data_network_buffers,
    "port_priority_equal": 1,
}

def _xy(idx):
    x = idx % mesh_stops_x
    y = idx // mesh_stops_x
    return x, y
def is_on_top_row(index):    return _xy(index)[1] == 0
def is_on_bottom_row(index): return _xy(index)[1] == (mesh_stops_y - 1)
def is_on_rightmost_column(index): return _xy(index)[0] == (mesh_stops_x - 1)
def is_on_leftmost_column(index):  return _xy(index)[0] == 0

# -------------------- app args --------------------
app_args = os.getenv("VANADIS_EXE_ARGS", "")
app_params = {"argc": 1}
if app_args:
    app_args_list = app_args.split(" ")
    app_params["argc"] = len(app_args_list) + 1
    for i,a in enumerate(app_args_list, start=1):
        app_params[f"arg{i}"] = a

vanadis_decoder = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = "vanadis.Vanadis" + vanadis_isa + "OSHandler"
protocol="MESI"

# -------------------- OS & processes (scale threads) --------------------
osParams = {
    "processDebugLevel" : 0,
    "dbgLevel" : os_verbosity,
    "dbgMask" : 8,
    "cores" : numCpus,
    "hardwareThreadCount" : numThreads,
    "page_size"  : 4096,
    "physMemSize" : physMemSize,
    "useMMU" : True,
    "checkpointDir" : checkpointDir,
    "checkpoint" : checkpoint
}

processList = (
    ( 1, {
        "env_count" : 1,
        "env0" : "OMP_NUM_THREADS={}".format(numCpus*numThreads),
        "exe" : full_exe_name,
        "arg0" : exe_name,
    } ),
)
processList[0][1].update(app_params)

# -------------------- params --------------------
osl1cacheParams = {
    "access_latency_cycles" : "2",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "L1" : "1",
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}

mmuParams = {
    "debug_level": 0,
    "num_cores": numCpus,
    "num_threads": numThreads,
    "page_size": 4096,
}

memParams = {
    "mem_size" : "4GiB",
    "access_time" : "1 ns"
}

simpleTimingParams = {
    "max_requests_per_cycle" : 1,
    "mem_size" : "4GiB",
    "tCAS" : 8, "tRCD" : 8, "tRP" : 8,
    "cycle_time" : "1.25ns",
    "row_size" : "2KiB",
    "row_policy" : "open",
    "debug_mask" : mh_debug,
    "debug_level" : mh_debug_level
}

# Slightly scale memory controller aggressiveness with size of system
#mem_channels = int(os.getenv("VANADIS_MEM_CHANNELS", max(2, min(8, math.ceil(numCpus/16)))))
timingParams = {
    "id" : 0,
    "addrMapper" : "memHierarchy.roundRobinAddrMapper",
    "addrMapper.interleave_size" : "64B",
#    "addrMapper.interleave_size" : "2KiB",
    "addrMapper.row_size" : "2KiB",
    "clock" : mem_clock,
    "mem_size" : "4GiB",
    "channels" : mem_channels,
    "channel.numRanks" : 4,
    "channel.rank.numBanks" : 8,
    "channel.transaction_Q_size" : transaction_Q_size,
    "channel.rank.bank.CL" : 8,
    "channel.rank.bank.CL_WR" : 6,
    "channel.rank.bank.RCD" : 8,
    "channel.rank.bank.TRP" : 8,
    "channel.rank.bank.dataCycles" : 2,
    "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
    "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "channel.rank.bank.pagePolicy.close" : 0,
    "printconfig" : 0,
    "channel.printconfig" : 0,
    "channel.rank.printconfig" : 0,
    "channel.rank.bank.printconfig" : 0,
}

tlbParams = {
    "debug_level": 0,
    "hitLatency": 1,
    "num_hardware_threads": numThreads,
    "num_tlb_entries_per_thread": 512,
#    "num_tlb_entries_per_thread": 2048,
    "tlb_set_size": 1,
}
tlbWrapperParams = { "debug_level": 0 }

decoderParams = { "loader_mode" : loader_mode, "uop_cache_entries" : 1536, "predecode_cache_entries" : 4 }
osHdlrParams = { }
branchPredParams = { "branch_entries" : 32 }

cpuParams = {
    "clock" : cpu_clock,
    "verbose" : verbosity,
    "hardware_threads": numThreads,
    "physical_fp_registers" : 96,
    "physical_int_registers" : 100,
    "integer_arith_cycles" : integer_arith_cycles,
    "integer_arith_units" : integer_arith_units,
    "fp_arith_cycles" : fp_arith_cycles,
    "fp_arith_units" : fp_arith_units,
    "integer_div_units" : 3,
    "integer_div_cycles" : 8,
    "fp_div_units" : 1,
    "fp_div_cycles" : 48,
    "branch_units" : 4,
    "branch_unit_cycles" : branch_arith_cycles,
    "print_int_reg" : False,
    "print_fp_reg" : False,
    "pipeline_trace_file" : pipe_trace_file,
    "reorder_slots" : rob_slots,
    "decodes_per_cycle" : decodes_per_cycle,
    "issues_per_cycle" :  issues_per_cycle,
    "fetches_per_cycle" : fetches_per_cycle,
    "retires_per_cycle" : retires_per_cycle,
    "pause_when_retire_address" : os.getenv("VANADIS_HALT_AT_ADDRESS", 0),
    "start_verbose_when_issue_address": dbgAddr,
    "stop_verbose_when_retire_address": stopDbg,
    "print_rob" : False,
}

lsqParams = {
    "verbose" : verbosity,
    "address_mask" : 0xFFFFFFFF,
    "max_loads" : lsq_ld_entries,
    "max_stores" : lsq_st_entries,
}

l1dcacheParams = {
    "access_latency_cycles" : "2",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "L1" : "1",
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
#    "mshr_entries" : 32,
}
l1icacheParams = {
    "access_latency_cycles" : "2",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
#    "mshr_entries" : 32,
}
l2cacheParams = {
    "access_latency_cycles" : "14",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "4 MB",
    "mshr_latency_cycles": 3,
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
#    "mshr_entries" : 64,
}
l2_prefetch_params = { "reach" : 16, "detect_range" : 1 }
busParams = { "bus_frequency" : cpu_clock }
l2memLinkParams = { "group" : 1, "debug" : mh_debug, "debug_level" : mh_debug_level }

# -------------------- builders --------------------
class CPU_Builder:
    def __init__(self): pass
    def build( self, prefix, nodeId, cpuId ):
        if pythonDebug: print("build {}".format(prefix))

        cpu = sst.Component(prefix, vanadis_cpu_type)
        cpu.addParams(cpuParams)
        cpu.addParam("core_id", cpuId)
        cpu.enableAllStatistics()

        for n in range(numThreads):
            decode = cpu.setSubComponent("decoder"+str(n), vanadis_decoder)
            decode.addParams(decoderParams)
            decode.enableAllStatistics()

            os_hdlr = decode.setSubComponent("os_handler", vanadis_os_hdlr)
            os_hdlr.addParams(osHdlrParams)

            branch_pred = decode.setSubComponent("branch_unit", "vanadis.VanadisBasicBranchUnit")
            branch_pred.addParams(branchPredParams)
            branch_pred.enableAllStatistics()

        cpu_lsq = cpu.setSubComponent("lsq", "vanadis.VanadisBasicLoadStoreQueue")
        cpu_lsq.addParams(lsqParams)
        cpu_lsq.enableAllStatistics()

        cpuDcacheIf = cpu_lsq.setSubComponent("memory_interface", "memHierarchy.standardInterface")
        cpuIcacheIf = cpu.setSubComponent("mem_interface_inst", "memHierarchy.standardInterface")

        cpu_l1dcache = sst.Component(prefix + ".l1dcache", "memHierarchy.Cache")
        cpu_l1dcache.addParams(l1dcacheParams)

        cpu_l1icache = sst.Component(prefix + ".l1icache", "memHierarchy.Cache")
        cpu_l1icache.addParams(l1icacheParams)

        cpu_l2cache = sst.Component(prefix + ".l2cache", "memHierarchy.Cache")
        cpu_l2cache.addParams(l2cacheParams)
#        l2pre = cpu_l2cache.setSubComponent("prefetcher", "cassini.StridePrefetcher")
#        l2pre.addParams(l2_prefetch_params)

        l2cache_2_mem = cpu_l2cache.setSubComponent("lowlink", "memHierarchy.MemNICFour")
        l2cache_2_mem.addParams(l2memLinkParams)
        l2data = l2cache_2_mem.setSubComponent("data", "kingsley.linkcontrol")
        l2req  = l2cache_2_mem.setSubComponent("req", "kingsley.linkcontrol")
        l2fwd  = l2cache_2_mem.setSubComponent("fwd", "kingsley.linkcontrol")
        l2ack  = l2cache_2_mem.setSubComponent("ack", "kingsley.linkcontrol")
        l2data.addParams(data_network_params)
        l2req.addParams(ctrl_network_params)
        l2fwd.addParams(ctrl_network_params)
        l2ack.addParams(ctrl_network_params)

        cache_bus = sst.Component(prefix + ".bus", "memHierarchy.Bus")
        cache_bus.addParams(busParams)

        dtlbWrapper = sst.Component(prefix + ".dtlb", "mmu.tlb_wrapper")
        dtlbWrapper.addParams(tlbWrapperParams)
        dtlb = dtlbWrapper.setSubComponent("tlb", "mmu." + tlbType )
        dtlb.addParams(tlbParams)

        itlbWrapper = sst.Component(prefix+".itlb", "mmu.tlb_wrapper")
        itlbWrapper.addParams(tlbWrapperParams)
        itlbWrapper.addParam("exe",True)
        itlb = itlbWrapper.setSubComponent("tlb", "mmu." + tlbType )
        itlb.addParams(tlbParams)

        link_cpu_dtlb_link = sst.Link(prefix+".link_cpu_dtlb_link")
        link_cpu_dtlb_link.connect( (cpuDcacheIf, "lowlink", "1ns"), (dtlbWrapper, "cpu_if", "1ns") )
        link_cpu_dtlb_link.setNoCut()

        link_cpu_l1dcache_link = sst.Link(prefix+".link_cpu_l1dcache_link")
        link_cpu_l1dcache_link.connect( (dtlbWrapper, "cache_if", "1ns"), (cpu_l1dcache, "highlink", "1ns") )
        link_cpu_l1dcache_link.setNoCut()

        link_cpu_itlb_link = sst.Link(prefix+".link_cpu_itlb_link")
        link_cpu_itlb_link.connect( (cpuIcacheIf, "lowlink", "1ns"), (itlbWrapper, "cpu_if", "1ns") )
        link_cpu_itlb_link.setNoCut()

        link_cpu_l1icache_link = sst.Link(prefix+".link_cpu_l1icache_link")
        link_cpu_l1icache_link.connect( (itlbWrapper, "cache_if", "1ns"), (cpu_l1icache, "highlink", "1ns") )
        link_cpu_l1icache_link.setNoCut()

        link_l1dcache_l2cache_link = sst.Link(prefix+".link_l1dcache_l2cache_link")
        link_l1dcache_l2cache_link.connect( (cpu_l1dcache, "lowlink", "1ns"), (cache_bus, "highlink0", "1ns") )
        link_l1dcache_l2cache_link.setNoCut()

        link_l1icache_l2cache_link = sst.Link(prefix+".link_l1icache_l2cache_link")
        link_l1icache_l2cache_link.connect( (cpu_l1icache, "lowlink", "1ns"), (cache_bus, "highlink1", "1ns") )
        link_l1icache_l2cache_link.setNoCut()

        link_bus_l2cache_link = sst.Link(prefix+".link_bus_l2cache_link")
        link_bus_l2cache_link.connect( (cache_bus, "lowlink0", "1ns"), (cpu_l2cache, "highlink", "1ns") )
        link_bus_l2cache_link.setNoCut()

        return (cpu, "os_link", "5ns"), (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns"), \
               (l2req, "rtr_port", mesh_link_latency), (l2ack, "rtr_port", mesh_link_latency), \
               (l2fwd, "rtr_port", mesh_link_latency), (l2data, "rtr_port", mesh_link_latency)

def addParamsPrefix(prefix,params):
    return { f"{prefix}.{k}": v for k,v in params.items() }

# -------------------- OS & caches --------------------
node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams(osParams)

num=0
for i,process in processList:
    for _ in range(i):
        node_os.addParams( addParamsPrefix( "process" + str(num), process ) )
        num+=1

node_os_mmu = node_os.setSubComponent( "mmu", "mmu." + mmuType )
node_os_mmu.addParams(mmuParams)

node_os_mem_if = node_os.setSubComponent( "mem_interface", "memHierarchy.standardInterface" )

os_cache = sst.Component("node_os.cache", "memHierarchy.Cache")
os_cache.addParams(osl1cacheParams)
os_cache_2_mem = os_cache.setSubComponent("lowlink", "memHierarchy.MemNICFour")
os_cache_2_mem.addParams( l2memLinkParams )
os_data = os_cache_2_mem.setSubComponent("data", "kingsley.linkcontrol")
os_req = os_cache_2_mem.setSubComponent("req", "kingsley.linkcontrol")
os_ack = os_cache_2_mem.setSubComponent("ack", "kingsley.linkcontrol")
os_fwd = os_cache_2_mem.setSubComponent("fwd", "kingsley.linkcontrol")
for c in (os_data, os_req, os_ack, os_fwd):
    c.addParams(data_network_params if c is os_data else ctrl_network_params)

link_os_cache_link = sst.Link("link_os_cache_link")
link_os_cache_link.connect( (node_os_mem_if, "lowlink", "1ns"), (os_cache, "highlink", "1ns") )
link_os_cache_link.setNoCut()

# -------------------- Memory & Directory builders --------------------
class MemBuilder:
    def __init__(self, capacity): self.mem_capacity = capacity
    def build(self, nodeID, memID):
        memctrl = sst.Component("memory" + str(nodeID), "memHierarchy.MemController")
        memctrl.addParams({
            "clock" : mem_clock,
            "backend.mem_size" : physMemSize,
            "backing" : "malloc",
            "initBacking": 1,
            "debug_level" : mh_debug_level,
            "debug" : mh_debug,
            "interleave_size" : "64B",
            "interleave_step" : str(numMemories * 64) + "B",
            "addr_range_start" : memID*64,
            "addr_range_end" :  1024*1024*1024 - ((numMemories - memID) * 64) + 63,
        })

        memNIC = memctrl.setSubComponent("highlink", "memHierarchy.MemNICFour")
        memNIC.addParams({ "group" : 3, "debug_level" : mh_debug_level, "debug" : mh_debug })
        memdata = memNIC.setSubComponent("data", "kingsley.linkcontrol")
        memreq  = memNIC.setSubComponent("req", "kingsley.linkcontrol")
        memack  = memNIC.setSubComponent("ack", "kingsley.linkcontrol")
        memfwd  = memNIC.setSubComponent("fwd", "kingsley.linkcontrol")
        memdata.addParams(data_network_params)
        memreq.addParams(ctrl_network_params)
        memfwd.addParams(ctrl_network_params)
        memack.addParams(ctrl_network_params)

        if backingType == "simpleMem":
            memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
            memory.addParams(memParams)
        elif backingType == "simpleDRAM":
            memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
            memory.addParams(simpleTimingParams)
        else:
            memory = memctrl.setSubComponent("backend", "memHierarchy.timingDRAM")
            memory.addParams(timingParams)

        memctrl.enableAllStatistics()
        return (memreq, "rtr_port", mesh_link_latency), (memack, "rtr_port", mesh_link_latency), \
               (memfwd, "rtr_port", mesh_link_latency), (memdata, "rtr_port", mesh_link_latency)

class DCBuilder:
    def build(self, nodeID, memID):
        # Scale directory size modestly with CPUs
        base_entries = 32768
        scale = max(1, int(math.ceil(numCpus/8)))
        dir_entries = base_entries * scale

        dirctrl = sst.Component("directory" + str(nodeID), "memHierarchy.DirectoryController")
        dirctrl.addParams({
            "clock" : mem_clock,
            "coherence_protocol" : protocol,
            "entry_cache_size" : dir_entries,
            "debug" : mh_debug,
            "debug_level" : mh_debug_level,
            "interleave_size" : "64B",
            "interleave_step" : str(numMemories * 64) + "B",
            "addr_range_start" : memID*64,
            "addr_range_end" :  1024*1024*1024 - ((numMemories - memID) * 64) + 63,
        })

        dirNIC = dirctrl.setSubComponent("highlink", "memHierarchy.MemNICFour")
        dirNIC.addParams({ "group" : 2, "debug" : mh_debug, "debug_level" : mh_debug_level })

        dcdata = dirNIC.setSubComponent("data", "kingsley.linkcontrol")
        dcreq  = dirNIC.setSubComponent("req", "kingsley.linkcontrol")
        dcfwd  = dirNIC.setSubComponent("fwd", "kingsley.linkcontrol")
        dcack  = dirNIC.setSubComponent("ack", "kingsley.linkcontrol")
        dcdata.addParams(data_network_params)
        dcreq.addParams(ctrl_network_params)
        dcfwd.addParams(ctrl_network_params)
        dcack.addParams(ctrl_network_params)

        return (dcreq, "rtr_port", mesh_link_latency), (dcack, "rtr_port", mesh_link_latency), \
               (dcfwd, "rtr_port", mesh_link_latency), (dcdata, "rtr_port", mesh_link_latency)

cpuBuilder = CPU_Builder()
memBuilder = MemBuilder(8192 * 1024 * 1024 * 1024)
dcBuilder  = DCBuilder()

# -------------------- Node builder --------------------
class NodeBuilder:
    def __init__(self):
        self.cpu_count = 0
        self.mem_count = 0

    def build_endpoint(self, nodeId, rtrreq, rtrack, rtrfwd, rtrdata):
        if nodeId == 0:
            # OS at node 0 via local2
            reqport2 = sst.Link("krtr_req_port2_" + str(nodeId))
            reqport2.connect( (rtrreq, "local2", mesh_link_latency), (os_req, "rtr_port", mesh_link_latency) )
            ackport2 = sst.Link("krtr_ack_port2_" + str(nodeId))
            ackport2.connect( (rtrack, "local2", mesh_link_latency), (os_ack, "rtr_port", mesh_link_latency) )
            fwdport2 = sst.Link("krtr_fwd_port2_" + str(nodeId))
            fwdport2.connect( (rtrfwd, "local2", mesh_link_latency), (os_fwd, "rtr_port", mesh_link_latency) )
            dataport2 = sst.Link("kRtr_data_port2_" + str(nodeId))
            dataport2.connect( (rtrdata, "local2", mesh_link_latency), (os_data, "rtr_port", mesh_link_latency) )

        if nodeId in cpu_node_list:
            prefix=f"node{nodeId}.self.cpu_count{self.cpu_count}"
            os_hdlr, dtlb, itlb, tilereq, tileack, tilefwd, tiledata = cpuBuilder.build(prefix, nodeId, self.cpu_count)

            link_mmu_dtlb_link = sst.Link(prefix + ".link_mmu_dtlb_link")
            link_mmu_dtlb_link.connect( (node_os_mmu, "core"+ str(self.cpu_count) +".dtlb", "1ns"), dtlb )

            link_mmu_itlb_link = sst.Link(prefix + ".link_mmu_itlb_link")
            link_mmu_itlb_link.connect( (node_os_mmu, "core"+ str(self.cpu_count) +".itlb", "1ns"), itlb )

            link_core_os_link = sst.Link(prefix + ".link_core_os_link")
            link_core_os_link.connect( os_hdlr, (node_os, "core" + str(self.cpu_count), "5ns") )

            reqport0 = sst.Link("krtr_req_port0_" + str(nodeId))
            reqport0.connect( (rtrreq, "local0", mesh_link_latency), tilereq )
            ackport0 = sst.Link("krtr_ack_port0_" + str(nodeId))
            ackport0.connect( (rtrack, "local0", mesh_link_latency), tileack )
            fwdport0 = sst.Link("krtr_fwd_port0_" + str(nodeId))
            fwdport0.connect( (rtrfwd, "local0", mesh_link_latency), tilefwd )
            dataport0 = sst.Link("kRtr_data_port0_" + str(nodeId))
            dataport0.connect( (rtrdata, "local0", mesh_link_latency), tiledata )

            self.cpu_count += 1

        elif nodeId in mem_node_list:
            # Memory controller via local2
            req, ack, fwd, data = memBuilder.build(nodeId, self.mem_count)
            sst.Link(f"krtr_req_mem_l2_{nodeId}").connect( (rtrreq, "local2", mesh_link_latency), req )
            sst.Link(f"krtr_ack_mem_l2_{nodeId}").connect( (rtrack, "local2", mesh_link_latency), ack )
            sst.Link(f"krtr_fwd_mem_l2_{nodeId}").connect( (rtrfwd, "local2", mesh_link_latency), fwd )
            sst.Link(f"kRtr_data_mem_l2_{nodeId}").connect( (rtrdata, "local2", mesh_link_latency), data )

            # Directory via local1
            req, ack, fwd, data = dcBuilder.build(nodeId, self.mem_count)
            sst.Link(f"krtr_req_port1_{nodeId}").connect( (rtrreq, "local1", mesh_link_latency), req )
            sst.Link(f"krtr_ack_port1_{nodeId}").connect( (rtrack, "local1", mesh_link_latency), ack )
            sst.Link(f"krtr_fwd_port1_{nodeId}").connect( (rtrfwd, "local1", mesh_link_latency), fwd )
            sst.Link(f"kRtr_data_port1_{nodeId}").connect( (rtrdata, "local1", mesh_link_latency), data )

            self.mem_count += 1
        else:
            pass

node_builder = NodeBuilder()

# -------------------- mesh --------------------
def build_mesh():
    kRtrReq=[]; kRtrAck=[]; kRtrFwd=[]; kRtrData=[]
    for _y in range(mesh_stops_y):
        for _x in range(mesh_stops_x):
            nodeNum = len(kRtrReq)
            a = sst.Component("krtr_req_"  + str(nodeNum), "kingsley.noc_mesh"); a.addParams(ctrl_network_params); kRtrReq.append(a)
            b = sst.Component("krtr_ack_"  + str(nodeNum), "kingsley.noc_mesh"); b.addParams(ctrl_network_params); kRtrAck.append(b)
            c = sst.Component("krtr_fwd_"  + str(nodeNum), "kingsley.noc_mesh"); c.addParams(ctrl_network_params); kRtrFwd.append(c)
            d = sst.Component("krtr_data_" + str(nodeNum), "kingsley.noc_mesh"); d.addParams(data_network_params); kRtrData.append(d)
            for comp in (a,b,c,d): comp.addParams({"local_ports": 3})

    i = 0
    for y in range(mesh_stops_y):
        for x in range(mesh_stops_x):
            if y != (mesh_stops_y -1):
                sst.Link("krtr_req_ns_"  + str(i)).connect( (kRtrReq[i],  "south", mesh_link_latency), (kRtrReq[i + mesh_stops_x],  "north", mesh_link_latency) )
                sst.Link("krtr_ack_ns_"  + str(i)).connect( (kRtrAck[i],  "south", mesh_link_latency), (kRtrAck[i + mesh_stops_x],  "north", mesh_link_latency) )
                sst.Link("krtr_fwd_ns_"  + str(i)).connect( (kRtrFwd[i],  "south", mesh_link_latency), (kRtrFwd[i + mesh_stops_x],  "north", mesh_link_latency) )
                sst.Link("krtr_data_ns_" + str(i)).connect( (kRtrData[i], "south", mesh_link_latency), (kRtrData[i + mesh_stops_x], "north", mesh_link_latency) )
            if x != (mesh_stops_x - 1):
                sst.Link("krtr_req_ew_"  + str(i)).connect( (kRtrReq[i],  "east", mesh_link_latency), (kRtrReq[i+1],  "west", mesh_link_latency) )
                sst.Link("krtr_ack_ew_"  + str(i)).connect( (kRtrAck[i],  "east", mesh_link_latency), (kRtrAck[i+1],  "west", mesh_link_latency) )
                sst.Link("krtr_fwd_ew_"  + str(i)).connect( (kRtrFwd[i],  "east", mesh_link_latency), (kRtrFwd[i+1],  "west", mesh_link_latency) )
                sst.Link("krtr_data_ew_" + str(i)).connect( (kRtrData[i], "east", mesh_link_latency), (kRtrData[i+1], "west", mesh_link_latency) )

            node_builder.build_endpoint(i, kRtrReq[i], kRtrAck[i], kRtrFwd[i], kRtrData[i])
            i += 1

# -------------------- main --------------------
build_mesh()
sst.setStatisticLoadLevel(7)
sst.enableAllStatisticsForAllComponents()

