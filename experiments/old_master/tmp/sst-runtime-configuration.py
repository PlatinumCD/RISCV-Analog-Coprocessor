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

import os, math
import sst

# -------------------- Global --------------------
mh_debug = 0
mh_debug_level = 0
dbgAddr, stopDbg = "0", "0"

checkpointDir = ""
checkpoint    = ""

pythonDebug = False

vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
isa = "riscv64"

loader_mode = os.getenv("VANADIS_LOADER_MODE", "0")

physMemSize = "8GiB"
tlbType = "simpleTLB"
mmuType = "simpleMMU"

sst.setProgramOption("timebase", "1ps")
sst.setStatisticLoadLevel(0)  # faster

# -------------------- App / Vanadis --------------------
full_exe_name = os.getenv("VANADIS_EXE", "")
exe_name = full_exe_name.split("/")[-1] if full_exe_name else ""

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = int(os.getenv("VANADIS_OS_VERBOSE", verbosity))
pipe_trace_file = ""  # off for speed
lsq_ld_entries = int(os.getenv("VANADIS_LSQ_LD_ENTRIES", 16))
lsq_st_entries = int(os.getenv("VANADIS_LSQ_ST_ENTRIES", 8))

rob_slots = int(os.getenv("VANADIS_ROB_SLOTS", 256))
retires_per_cycle = int(os.getenv("VANADIS_RETIRES_PER_CYCLE", 6))
issues_per_cycle  = int(os.getenv("VANADIS_ISSUES_PER_CYCLE", 6))
decodes_per_cycle = int(os.getenv("VANADIS_DECODES_PER_CYCLE", 6))

integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 2))
integer_arith_units  = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 4))
fp_arith_cycles      = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 6))
fp_arith_units       = int(os.getenv("VANADIS_FP_ARITH_UNITS", 4))
branch_arith_cycles  = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2))

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")
numCpus   = int(os.getenv("VANADIS_NUM_CORES", 4))
numThreads= int(os.getenv("VANADIS_NUM_HW_THREADS", 1))

vanadis_cpu_type = "vanadis.dbg_VanadisCPU"
vanadis_decoder  = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr  = "vanadis.Vanadis" + vanadis_isa + "OSHandler"

app_args = os.getenv("VANADIS_EXE_ARGS", "")
app_params = {"argc": 1}
if app_args:
    lst = app_args.split(" ")
    app_params["argc"] = len(lst) + 1
    for i, a in enumerate(lst, 1):
        app_params["arg" + str(i)] = a

# -------------------- RoCC / Array --------------------
rocc_type = os.getenv("GOLEM_ROCC_TYPE", "golem.RoCCAnalogFloat")
array_type = os.getenv("GOLEM_ARRAY_TYPE")
num_arrays = int(os.getenv("GOLEM_NUM_ARRAYS", 1))
array_input_size  = os.getenv("ARRAY_INPUT_SIZE")
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
processList = ((1, {
    "env_count": 1,
    "env0": f"OMP_NUM_THREADS={numCpus*numThreads}",
    "exe": full_exe_name,
    "arg0": exe_name,
}),)
processList[0][1].update(app_params)

# -------------------- Params: caches/MMU/NIC/router --------------------
osl1cacheParams = {
    "access_latency_cycles":"2", "cache_frequency":cpu_clock, "replacement_policy":"lru",
    "coherence_protocol":protocol, "associativity":"8", "cache_line_size":"64",
    "cache_size":"32 KB", "L1":"1", "debug":mh_debug, "debug_level":mh_debug_level,
}
mmuParams = {"debug_level":0, "num_cores":numCpus, "num_threads":numThreads, "page_size":4096}

# Router defaults (we override id/num_ports per router)
memRtrParams = {
    "xbar_bw":"200GB/s", "link_bw":"100GB/s",
    "input_buf_size":"8KB", "output_buf_size":"8KB",
    "flit_size":"256B", "num_vns":3,
}

dirCtrlParams = {
    "coherence_protocol":protocol, "entry_cache_size":"1024",
    "debug":mh_debug, "debug_level":mh_debug_level,
    "addr_range_start":"0x0", "addr_range_end":"0xFFFFFFFF",
}
dirNicParams = {"network_bw":"100GB/s", "group":2}
memCtrlParams = {
    "clock":cpu_clock, "backend.mem_size":physMemSize, "backing":"malloc", "initBacking":1,
    "addr_range_start":0, "addr_range_end":0xFFFFFFFF,
    "debug_level":mh_debug_level, "debug":mh_debug,
    "checkpointDir":checkpointDir, "checkpoint":checkpoint,
}
memParams = {"mem_size":"4GiB", "access_time":"1 ns"}

tlbParams = {"debug_level":0, "hitLatency":1, "num_hardware_threads":numThreads,
             "num_tlb_entries_per_thread":1024, "tlb_set_size":4}
tlbWrapperParams = {"debug_level":0}
decoderParams = {"loader_mode":loader_mode, "uop_cache_entries":1536, "predecode_cache_entries":4}
osHdlrParams = {}
branchPredParams = {"branch_entries":4096}

cpuParams = {
    "clock":cpu_clock, "verbose":verbosity, "hardware_threads":numThreads,
    "physical_fp_registers":168*numThreads, "physical_integer_registers":180*numThreads,
    "integer_arith_cycles":integer_arith_cycles, "integer_arith_units":integer_arith_units,
    "fp_arith_cycles":fp_arith_cycles, "fp_arith_units":fp_arith_units,
    "branch_unit_cycles":branch_arith_cycles,
    "pipeline_trace_file":pipe_trace_file, "reorder_slots":rob_slots,
    "decodes_per_cycle":decodes_per_cycle, "issues_per_cycle":issues_per_cycle,
    "retires_per_cycle":retires_per_cycle,
    "pause_when_retire_address": int(os.getenv("VANADIS_HALT_AT_ADDRESS", 0)),
    "start_verbose_when_issue_address": dbgAddr,
    "stop_verbose_when_retire_address":  stopDbg,
    "print_int_reg": False, "print_fp_reg": False,
    "print_rob": False, "checkpointDir":checkpointDir, "checkpoint":checkpoint,
}
lsqParams = {"verbose":verbosity, "address_mask":0xFFFFFFFF,
             "max_stores":lsq_st_entries, "max_loads":lsq_ld_entries}

roccParams = {"clock":cpu_clock, "verbose":verbosity, "max_instructions":8,
              "inputOperandSize":4, "outputOperandSize":4}
arrayParams= {"arrayLatency":"100ns", "clock":cpu_clock, "max_instructions":8,
              "verbose":verbosity, "mmioAddr":0, "numArrays":num_arrays,
              "arrayInputSize":array_input_size, "arrayOutputSize":array_output_size,
              "inputOperandSize":4, "outputOperandSize":4}

l1dcacheParams = {
    "access_latency_cycles":"2", "cache_frequency":cpu_clock, "replacement_policy":"lru",
    "coherence_protocol":protocol, "associativity":"8","cache_line_size":"64","cache_size":"32 KB",
    "L1":"1","debug":mh_debug,"debug_level":mh_debug_level,
}
l1icacheParams = {
    "access_latency_cycles":"2","cache_frequency":cpu_clock,"replacement_policy":"lru",
    "coherence_protocol":protocol,"associativity":"8","cache_line_size":"64","cache_size":"32 KB",
    "L1":"1","debug":mh_debug,"debug_level":mh_debug_level,
    # Prefetcher off for speed
}
l2cacheParams = {
    "access_latency_cycles":"14","cache_frequency":cpu_clock,"replacement_policy":"lru",
    "coherence_protocol":protocol,"associativity":"16","cache_line_size":"64","cache_size":"1MB",
    "mshr_latency_cycles":3,"debug":mh_debug,"debug_level":mh_debug_level,
}
busParams = {"bus_frequency":cpu_clock}
l2memLinkParams = {"group":1, "network_bw":"100GB/s"}

# -------------------- CPU builder --------------------
class CPU_Builder:
    def build(self, prefix, nodeId, cpuId):
        cpu = sst.Component(prefix, vanadis_cpu_type)
        cpu.addParams(cpuParams); cpu.addParam("core_id", cpuId); cpu.enableAllStatistics()

        for n in range(numThreads):
            decode = cpu.setSubComponent("decoder"+str(n), vanadis_decoder)
            decode.addParams(decoderParams); decode.enableAllStatistics()
            os_hdlr = decode.setSubComponent("os_handler", vanadis_os_hdlr); os_hdlr.addParams(osHdlrParams)
            branch  = decode.setSubComponent("branch_unit", "vanadis.VanadisBasicBranchUnit")
            branch.addParams(branchPredParams); branch.enableAllStatistics()

        cpu_lsq = cpu.setSubComponent("lsq","vanadis.VanadisBasicLoadStoreQueue")
        cpu_lsq.addParams(lsqParams); cpu_lsq.enableAllStatistics()

        cpu_rocc = cpu.setSubComponent("rocc", rocc_type, 0)
        cpu_rocc.addParams(roccParams); cpu_rocc.enableAllStatistics()
        computeArray = cpu_rocc.setSubComponent("array", array_type); computeArray.addParams(arrayParams)

        cpuDcacheIf = cpu_lsq.setSubComponent("memory_interface","memHierarchy.standardInterface")
        roccDcacheIf= cpu_rocc.setSubComponent("memory_interface","memHierarchy.standardInterface")
        cpuIcacheIf = cpu.setSubComponent("mem_interface_inst","memHierarchy.standardInterface")

        l1d = sst.Component(prefix+".l1dcache","memHierarchy.Cache"); l1d.addParams(l1dcacheParams); l1d.enableAllStatistics()
        l1d_hi = l1d.setSubComponent("highlink","memHierarchy.MemLink")
        l1d_lo = l1d.setSubComponent("lowlink","memHierarchy.MemLink")

        l1i = sst.Component(prefix+".l1icache","memHierarchy.Cache"); l1i.addParams(l1icacheParams)
        l1i_hi = l1i.setSubComponent("highlink","memHierarchy.MemLink")
        l1i_lo = l1i.setSubComponent("lowlink","memHierarchy.MemLink")

        l2  = sst.Component(prefix+".l2cache","memHierarchy.Cache"); l2.addParams(l2cacheParams)
        l2_hi = l2.setSubComponent("highlink","memHierarchy.MemLink")
        l2_nic= l2.setSubComponent("lowlink","memHierarchy.MemNIC"); l2_nic.addParams(l2memLinkParams)

        cache_bus = sst.Component(prefix+".bus","memHierarchy.Bus"); cache_bus.addParams(busParams)
        proc_bus  = sst.Component(prefix+".processorBus","memHierarchy.Bus"); proc_bus.addParams(busParams)

        dtlbW = sst.Component(prefix+".dtlb","mmu.tlb_wrapper"); dtlbW.addParams(tlbWrapperParams)
        dtlb  = dtlbW.setSubComponent("tlb","mmu."+tlbType); dtlb.addParams(tlbParams)
        itlbW = sst.Component(prefix+".itlb","mmu.tlb_wrapper"); itlbW.addParams(tlbWrapperParams); itlbW.addParam("exe",True)
        itlb  = itlbW.setSubComponent("tlb","mmu."+tlbType); itlb.addParams(tlbParams)

        # Wiring (keep intra-tile NoCut; interconnect remains cuttable)
        sst.Link(prefix+".cpu_dbus").connect((cpuDcacheIf,"lowlink","1ns"), (proc_bus,"high_network_0","1ns"))
        sst.Link(prefix+".rocc_dbus").connect((roccDcacheIf,"lowlink","1ns"), (proc_bus,"high_network_1","1ns"))
        sst.Link(prefix+".pbus_to_dtlb").connect((proc_bus,"low_network_0","1ns"), (dtlbW,"cpu_if","1ns"))
        sst.Link(prefix+".dtlb_to_l1d").connect((dtlbW,"cache_if","1ns"), (l1d_hi,"port","1ns"))
        sst.Link(prefix+".ic_if_to_itlb").connect((cpuIcacheIf,"lowlink","1ns"), (itlbW,"cpu_if","1ns"))
        sst.Link(prefix+".itlb_to_l1i").connect((itlbW,"cache_if","1ns"), (l1i_hi,"port","1ns"))
        sst.Link(prefix+".l1d_to_bus").connect((l1d_lo,"port","1ns"), (cache_bus,"high_network_0","1ns"))
        sst.Link(prefix+".l1i_to_bus").connect((l1i_lo,"port","1ns"), (cache_bus,"high_network_1","1ns"))
        sst.Link(prefix+".bus_to_l2").connect((cache_bus,"low_network_0","1ns"), (l2_hi,"port","1ns"))

        return (cpu, "os_link", "5ns"), (l2_nic, "port", "1ns"), (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns")

def addParamsPrefix(prefix, params):
    return { f"{prefix}.{k}": v for k,v in params.items() }

# -------------------- OS + MMU + OS L1 --------------------
node_os = sst.Component("os","vanadis.VanadisNodeOS"); node_os.addParams(osParams)
num=0
for i,process in processList:
    for _ in range(i):
        node_os.addParams(addParamsPrefix("process"+str(num), process)); num+=1

node_os_mmu = node_os.setSubComponent("mmu", "mmu."+mmuType); node_os_mmu.addParams(mmuParams)
node_os_mem_if = node_os.setSubComponent("mem_interface","memHierarchy.standardInterface")

os_cache = sst.Component("node_os.cache","memHierarchy.Cache"); os_cache.addParams(osl1cacheParams)
# Connect OS std-if directly to cache highlink (fixes 'lowlink unconfigured' error)
sst.Link("os_to_cache").connect((node_os_mem_if,"lowlink","1ns"), (os_cache,"highlink","1ns"))
os_cache_nic = os_cache.setSubComponent("lowlink","memHierarchy.MemNIC"); os_cache_nic.addParams(l2memLinkParams)

# -------------------- Directory + Memory --------------------
dirctrl = sst.Component("dirctrl","memHierarchy.DirectoryController"); dirctrl.addParams(dirCtrlParams)
dir_to_mem = dirctrl.setSubComponent("lowlink","memHierarchy.MemLink")
dir_nic    = dirctrl.setSubComponent("highlink","memHierarchy.MemNIC"); dir_nic.addParams(dirNicParams)

memctrl = sst.Component("memory","memHierarchy.MemController"); memctrl.addParams(memCtrlParams)
memToDir = memctrl.setSubComponent("highlink","memHierarchy.MemLink")
backend  = memctrl.setSubComponent("backend","memHierarchy.simpleMem"); backend.addParams(memParams)

sst.Link("dir_mem").connect((dir_to_mem,"port","1ns"), (memToDir,"port","1ns"))

# -------------------- Multi-router mesh --------------------

NX = int(os.getenv("MERLIN_NX", 2))
NY = int(os.getenv("MERLIN_NY", 2))
R  = NX * NY

NEIGHBOR_PORTS = 4              # N,S,E,W
# Spread endpoints so max locals per router is 2 (OK for 4 CPUs + OS + DIR on 4 routers)
locals_per_router = max(2, (numCpus + 2 + R - 1) // R)   # uniform across all routers

# Build routers
routers = []
for y in range(NY):
    for x in range(NX):
        rid = y*NX + x
        r = sst.Component(f"rtr{rid}", "merlin.hr_router")
        rp = dict(memRtrParams)
        rp["id"] = str(rid)
        rp["num_ports"] = str(locals_per_router + NEIGHBOR_PORTS)
        r.addParams(rp)
        topo = r.setSubComponent("topology","merlin.mesh")
        topo.addParams({
            "shape": f"{NX}x{NY}",
            "width":  "1x1",
            "local_ports": locals_per_router,
            "link_latency": "1ns",
        })
        routers.append(r)

def rtr(x,y): return routers[y*NX + x]

# ---- Neighbor wiring (neighbors come AFTER locals) ----
NB = locals_per_router           # neighbor port base

# Convention: +X(E)=NB+0, -X(W)=NB+1, +Y(S)=NB+2, -Y(N)=NB+3
for y in range(NY):
    for x in range(NX):
        rid = y*NX + x
        if x+1 < NX:   # East
            sst.Link(f"r{rid}_E").connect((rtr(x,y),   f"port{NB+0}", "1ns"),
                                          (rtr(x+1,y), f"port{NB+1}", "1ns"))
        if y+1 < NY:   # South
            sst.Link(f"r{rid}_S").connect((rtr(x,y),   f"port{NB+2}", "1ns"),
                                          (rtr(x,y+1), f"port{NB+3}", "1ns"))


# -------------------- Build CPUs and map to routers --------------------
cpuBuilder = CPU_Builder()
nodeId = 0

local_slot = [0]*R
def attach_local(rid):
    idx = local_slot[rid];  local_slot[rid] += 1
    return (routers[rid], f"port{idx}", "1ns")

# CPUs round-robin over routers
for cpu in range(numCpus):
    prefix = f"node0.cpu{cpu}"
    os_hdlr, l2nic, dtlb, itlb = cpuBuilder.build(prefix, 0, cpu)
    sst.Link(prefix+".mmu_d").connect((node_os_mmu, f"core{cpu}.dtlb", "1ns"), dtlb)
    sst.Link(prefix+".mmu_i").connect((node_os_mmu, f"core{cpu}.itlb", "1ns"), itlb)
    sst.Link(prefix+".os").connect(os_hdlr, (node_os, f"core{cpu}", "5ns"))
    rid = cpu % R
    sst.Link(prefix+".l2_to_rtr").connect(l2nic, attach_local(rid))

# Put OS cache on rtr0, Directory on rtr1 to keep <=2 locals per router
sst.Link("oscache_to_rtr").connect((os_cache_nic,"port","1ns"), attach_local(0))
sst.Link("dir_to_rtr").connect((dir_nic,"port","1ns"),        attach_local(min(1, R-1)))

