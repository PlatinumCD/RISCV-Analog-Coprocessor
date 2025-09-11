import os, array, sst

# --------- Basics ---------
mh_debug_level=10; mh_debug=0
dbgAddr=0; stopDbg=0
checkpointDir=""; checkpoint=""
pythonDebug=False

vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
isa="riscv64"
loader_mode = os.getenv("VANADIS_LOADER_MODE","0")

testDir="basic-io"; exe="hello-world"
physMemSize="4GiB"

tlbType="simpleTLB"; mmuType="simpleMMU"
backingType="timingDRAM"  # simpleMem | simpleDRAM | timingDRAM

sst.setProgramOption("timebase","1ps")
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()

full_exe_name = os.getenv("VANADIS_EXE", f"../tests/small/{testDir}/{exe}/{isa}/{exe}")
exe_name = full_exe_name.split("/")[-1]

verbosity=int(os.getenv("VANADIS_VERBOSE",0))
os_verbosity=int(os.getenv("VANADIS_OS_VERBOSE",verbosity))
pipe_trace_file=os.getenv("VANADIS_PIPE_TRACE","")
lsq_ld_entries=int(os.getenv("VANADIS_LSQ_LD_ENTRIES",16))
lsq_st_entries=int(os.getenv("VANADIS_LSQ_ST_ENTRIES",8))

rob_slots=int(os.getenv("VANADIS_ROB_SLOTS",96))
retires_per_cycle=int(os.getenv("VANADIS_RETIRES_PER_CYCLE",3))
issues_per_cycle=int(os.getenv("VANADIS_ISSUES_PER_CYCLE",3))
decodes_per_cycle=int(os.getenv("VANADIS_DECODES_PER_CYCLE",3))
fetches_per_cycle=int(os.getenv("VANADIS_FETCHES_PER_CYCLE",8))

integer_arith_cycles=int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES",3))
integer_arith_units=int(os.getenv("VANADIS_INTEGER_ARITH_UNITS",3))
fp_arith_cycles=int(os.getenv("VANADIS_FP_ARITH_CYCLES",4))
fp_arith_units=int(os.getenv("VANADIS_FP_ARITH_UNITS",1))
branch_arith_cycles=int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES",2))

cpu_clock=os.getenv("VANADIS_CPU_CLOCK","3.2GHz")
mem_clock=os.getenv("VANADIS_MEM_CLOCK","1.0GHz")

numCpus=int(os.getenv("VANADIS_NUM_CORES",1))
numThreads=int(os.getenv("VANADIS_NUM_HW_THREADS",1))
numMemories=int(os.getenv("VANADIS_NUM_MEMORIES",1))

rocc_type = os.getenv("GOLEM_ROCC_TYPE", "golem.RoCCAnalogFloat")
array_type = os.getenv("GOLEM_ARRAY_TYPE")
num_arrays = int(os.getenv("GOLEM_NUM_ARRAYS", 1))
array_input_size = os.getenv("ARRAY_INPUT_SIZE")
array_output_size = os.getenv("ARRAY_OUTPUT_SIZE")

vanadis_cpu_type = "vanadis."+os.getenv("VANADIS_CPU_ELEMENT_NAME","dbg_VanadisCPU")
vanadis_decoder = "vanadis.Vanadis"+vanadis_isa+"Decoder"
vanadis_os_hdlr = "vanadis.Vanadis"+vanadis_isa+"OSHandler"
protocol="MESI"

# --------- Params (matching your originals) ---------
def addPfx(p, d): return {f"{p}.{k}":v for k,v in d.items()}

osParams={"processDebugLevel":0,"dbgLevel":os_verbosity,"dbgMask":8,"cores":numCpus,
          "hardwareThreadCount":numThreads,"page_size":4096,"physMemSize":physMemSize,
          "useMMU":True,"checkpointDir":checkpointDir,"checkpoint":checkpoint}

processList=((1,{"env_count":1,"env0":f"OMP_NUM_THREADS={numCpus*numThreads}",
                 "exe":full_exe_name,"arg0":exe_name}),)
app_args=os.getenv("VANADIS_EXE_ARGS","")
if app_args:
    argv=app_args.split(); processList[0][1]["argc"]=len(argv)+1
    for i,a in enumerate(argv,1): processList[0][1][f"arg{i}"]=a
else:
    processList[0][1]["argc"]=1

osl1cacheParams={"access_latency_cycles":"2","cache_frequency":cpu_clock,"replacement_policy":"lru",
                 "coherence_protocol":protocol,"associativity":"8","cache_line_size":"64",
                 "cache_size":"32 KB","L1":"1","debug":mh_debug,"debug_level":mh_debug_level}

mmuParams={"debug_level":0,"num_cores":numCpus,"num_threads":numThreads,"page_size":4096}
memParams={"mem_size":"4GiB","access_time":"1 ns"}
simpleTimingParams={"max_requests_per_cycle":1,"mem_size":"4GiB","tCAS":8,"tRCD":8,"tRP":8,
                    "cycle_time":"1.25ns","row_size":"2KiB","row_policy":"open",
                    "debug_mask":mh_debug,"debug_level":mh_debug_level}
timingParams={"id":0,"addrMapper":"memHierarchy.roundRobinAddrMapper",
              "addrMapper.interleave_size":"64B","addrMapper.row_size":"2KiB","clock":mem_clock,
              "mem_size":"4GiB","channels":3,"channel.numRanks":4,"channel.rank.numBanks":8,
              "channel.transaction_Q_size":32,"channel.rank.bank.CL":8,"channel.rank.bank.CL_WR":6,
              "channel.rank.bank.RCD":8,"channel.rank.bank.TRP":8,"channel.rank.bank.dataCycles":2,
              "channel.rank.bank.pagePolicy":"memHierarchy.simplePagePolicy",
              "channel.rank.bank.transactionQ":"memHierarchy.reorderTransactionQ",
              "channel.rank.bank.pagePolicy.close":0,"printconfig":0,"channel.printconfig":0,
              "channel.rank.printconfig":0,"channel.rank.bank.printconfig":0}

tlbParams={"debug_level":0,"hitLatency":1,"num_hardware_threads":numThreads,
          "num_tlb_entries_per_thread":512,"tlb_set_size":1}
tlbWrapperParams={"debug_level":0}
decoderParams={"loader_mode":loader_mode,"uop_cache_entries":1536,"predecode_cache_entries":4}
osHdlrParams={}
branchPredParams={"branch_entries":32}

roccParams = {
    "clock": cpu_clock,
    "verbose": 8,
    "max_instructions": 8,
}

arrayParams = {
    "arrayLatency": "100ns",
    "clock": cpu_clock,
    "max_instructions": 8,
    "verbose": 8,
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



cpuParams={"clock":cpu_clock,"verbose":verbosity,"hardware_threads":numThreads,
           "physical_fp_registers":96,"physical_int_registers":100,
           "integer_arith_cycles":integer_arith_cycles,"integer_arith_units":integer_arith_units,
           "fp_arith_cycles":fp_arith_cycles,"fp_arith_units":fp_arith_units,
           "integer_div_units":3,"integer_div_cycles":8,"fp_div_units":1,"fp_div_cycles":48,
           "branch_units":4,"branch_unit_cycles":branch_arith_cycles,
           "print_int_reg":False,"print_fp_reg":False,"pipeline_trace_file":pipe_trace_file,
           "reorder_slots":rob_slots,"decodes_per_cycle":decodes_per_cycle,
           "issues_per_cycle":issues_per_cycle,"fetches_per_cycle":fetches_per_cycle,
           "retires_per_cycle":retires_per_cycle,
           "pause_when_retire_address":int(os.getenv("VANADIS_HALT_AT_ADDRESS",0)),
           "start_verbose_when_issue_address":dbgAddr,"stop_verbose_when_retire_address":stopDbg,
           "print_rob":False}

lsqParams={"verbose":verbosity,"address_mask":0xFFFFFFFF,"max_loads":lsq_ld_entries,"max_stores":lsq_st_entries}
l1dcacheParams={"access_latency_cycles":"2","cache_frequency":cpu_clock,"replacement_policy":"lru",
                "coherence_protocol":protocol,"associativity":"8","cache_line_size":"64",
                "cache_size":"32 KB","L1":"1","debug":mh_debug,"debug_level":mh_debug_level}
l1icacheParams={"access_latency_cycles":"2","cache_frequency":cpu_clock,"replacement_policy":"lru",
                "coherence_protocol":protocol,"associativity":"8","cache_line_size":"64",
                "cache_size":"32 KB","prefetcher":"cassini.NextBlockPrefetcher","prefetcher.reach":1,
                "L1":"1","debug":mh_debug,"debug_level":mh_debug_level}
l2cacheParams={"access_latency_cycles":"14","cache_frequency":cpu_clock,"replacement_policy":"lru",
               "coherence_protocol":protocol,"associativity":"8","cache_line_size":"64",
               "cache_size":"4 MB","mshr_latency_cycles":3,"debug":mh_debug,"debug_level":mh_debug_level}
l2_prefetch_params={"reach":16,"detect_range":1}
busParams={"bus_frequency":cpu_clock}

# NIC params for Merlin
nicVN1={"group":1,"network_bw":"50GB/s"}  # CPUs + OS
nicVN2={"group":2,"network_bw":"50GB/s"}  # Directory

# --------- OS + MMU + OS-L1 ---------
node_os = sst.Component("os","vanadis.VanadisNodeOS"); node_os.addParams(osParams)
num=0
for i,proc in processList:
    for _ in range(i):
        node_os.addParams(addPfx(f"process{num}",proc)); num+=1

node_os_mmu = node_os.setSubComponent("mmu","mmu."+mmuType); node_os_mmu.addParams(mmuParams)
node_os_mem_if = node_os.setSubComponent("mem_interface","memHierarchy.standardInterface")

os_cache = sst.Component("node_os.cache","memHierarchy.Cache"); os_cache.addParams(osl1cacheParams)
os_nic  = os_cache.setSubComponent("lowlink","memHierarchy.MemNIC"); os_nic.addParams(nicVN1)

sst.Link("os_cache_link").connect((node_os_mem_if,"lowlink","1ns"), (os_cache,"highlink","1ns"))

# --------- CPU Builder (RoCC via DTLB; L2 → MemNIC → Merlin) ---------
class CPU_Builder:
    def build(self, prefix, nodeId, cid):
        # CPU
        cpu = sst.Component(prefix, vanadis_cpu_type)
        cpu.addParams(cpuParams)
        cpu.addParam("core_id", cid)
        cpu.enableAllStatistics()

        # Decoders (+ OS handler + branch unit)
        for n in range(numThreads):
            dec = cpu.setSubComponent("decoder"+str(n), vanadis_decoder)
            dec.addParams(decoderParams); dec.enableAllStatistics()
            osh = dec.setSubComponent("os_handler", vanadis_os_hdlr)
            osh.addParams(osHdlrParams)
            br  = dec.setSubComponent("branch_unit", "vanadis.VanadisBasicBranchUnit")
            br.addParams(branchPredParams); br.enableAllStatistics()

        # LSQ + mem IFs
        lsq  = cpu.setSubComponent("lsq", "vanadis.VanadisBasicLoadStoreQueue")
        lsq.addParams(lsqParams); lsq.enableAllStatistics()
        d_if = lsq.setSubComponent("memory_interface", "memHierarchy.standardInterface")
        i_if = cpu.setSubComponent("mem_interface_inst", "memHierarchy.standardInterface")

        # RoCC + Array (uses same D-side path as CPU)
        cpu_rocc = cpu.setSubComponent("rocc", rocc_type, 0)
        cpu_rocc.addParams(roccParams); cpu_rocc.enableAllStatistics()
        roccD_if = cpu_rocc.setSubComponent("memory_interface", "memHierarchy.standardInterface")
        arr = cpu_rocc.setSubComponent("array", array_type)
        arr.addParams(arrayParams)

        # Caches
        l1d = sst.Component(prefix+".l1d", "memHierarchy.Cache")
        l1d.addParams(l1dcacheParams)
        l1i = sst.Component(prefix+".l1i", "memHierarchy.Cache")
        l1i.addParams(l1icacheParams)
        l2  = sst.Component(prefix+".l2",  "memHierarchy.Cache")
        l2.addParams(l2cacheParams)
        l2pre = l2.setSubComponent("prefetcher", "cassini.StridePrefetcher")
        l2pre.addParams(l2_prefetch_params)

        # L2 → Merlin
        nic = l2.setSubComponent("lowlink", "memHierarchy.MemNIC")
        nic.addParams(nicVN1)

        # Buses: dbus (LSQ+RoCC → DTLB), corebus (L1s → L2)
        dbus    = sst.Component(prefix+".dbus",    "memHierarchy.Bus"); dbus.addParams(busParams)
        corebus = sst.Component(prefix+".corebus", "memHierarchy.Bus"); corebus.addParams(busParams)

        # TLBs
        dtlbW = sst.Component(prefix+".dtlb", "mmu.tlb_wrapper"); dtlbW.addParams(tlbWrapperParams)
        dtlb  = dtlbW.setSubComponent("tlb", "mmu."+tlbType); dtlb.addParams(tlbParams)
        itlbW = sst.Component(prefix+".itlb", "mmu.tlb_wrapper"); itlbW.addParams(tlbWrapperParams); itlbW.addParam("exe", True)
        itlb  = itlbW.setSubComponent("tlb", "mmu."+tlbType); itlb.addParams(tlbParams)

        # ---- Wiring ----
        # D-side: LSQ + RoCC → dbus → DTLB → L1D
        sst.Link(prefix+".lsq_dbus").connect( (d_if,     "lowlink","1ns"), (dbus,    "highlink0","1ns") )
        sst.Link(prefix+".rocc_dbus").connect((roccD_if, "lowlink","1ns"), (dbus,    "highlink1","1ns") )
        sst.Link(prefix+".dbus_dtlb").connect((dbus,     "lowlink0","1ns"), (dtlbW,  "cpu_if",  "1ns") )
        sst.Link(prefix+".dtlb_l1d").connect((dtlbW,     "cache_if","1ns"), (l1d,    "highlink","1ns") )

        # I-side: I IF → ITLB → L1I
        sst.Link(prefix+".i_tlb").connect   ( (i_if,     "lowlink","1ns"), (itlbW,  "cpu_if",  "1ns") )
        sst.Link(prefix+".itlb_l1i").connect((itlbW,     "cache_if","1ns"), (l1i,    "highlink","1ns") )

        # L1s → corebus → L2
        sst.Link(prefix+".l1d_bus").connect((l1d, "lowlink","1ns"), (corebus, "highlink0","1ns"))
        sst.Link(prefix+".l1i_bus").connect((l1i, "lowlink","1ns"), (corebus, "highlink1","1ns"))
        sst.Link(prefix+".bus_l2").connect ((corebus, "lowlink0","1ns"), (l2, "highlink","1ns"))

        # Return endpoints for external wiring
        return (cpu, "os_link", "5ns"), (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns"), (nic, "port", "1ns")


## --------- CPU Builder (L2 → MemNIC → Merlin) ---------
#class CPU_Builder:
#    def build(self, prefix, nodeId, cid):
#        cpu = sst.Component(prefix, vanadis_cpu_type); cpu.addParams(cpuParams); cpu.addParam("core_id", cid); cpu.enableAllStatistics()
#
#        for n in range(numThreads):
#            dec = cpu.setSubComponent("decoder"+str(n), vanadis_decoder); dec.addParams(decoderParams); dec.enableAllStatistics()
#            osh = dec.setSubComponent("os_handler", vanadis_os_hdlr); osh.addParams(osHdlrParams)
#            br  = dec.setSubComponent("branch_unit","vanadis.VanadisBasicBranchUnit"); br.addParams(branchPredParams); br.enableAllStatistics()
#
#        lsq = cpu.setSubComponent("lsq","vanadis.VanadisBasicLoadStoreQueue"); lsq.addParams(lsqParams); lsq.enableAllStatistics()
#        d_if = lsq.setSubComponent("memory_interface","memHierarchy.standardInterface")
#        i_if = cpu.setSubComponent("mem_interface_inst","memHierarchy.standardInterface")
#
#        l1d = sst.Component(prefix+".l1d","memHierarchy.Cache"); l1d.addParams(l1dcacheParams)
#        l1i = sst.Component(prefix+".l1i","memHierarchy.Cache"); l1i.addParams(l1icacheParams)
#        l2  = sst.Component(prefix+".l2","memHierarchy.Cache"); l2.addParams(l2cacheParams)
#        l2pre = l2.setSubComponent("prefetcher","cassini.StridePrefetcher"); l2pre.addParams(l2_prefetch_params)
#        nic = l2.setSubComponent("lowlink","memHierarchy.MemNIC"); nic.addParams(nicVN1)
#
#        bus = sst.Component(prefix+".bus","memHierarchy.Bus"); bus.addParams(busParams)
#
#        dtlbW = sst.Component(prefix+".dtlb","mmu.tlb_wrapper"); dtlbW.addParams(tlbWrapperParams)
#        dtlb = dtlbW.setSubComponent("tlb","mmu."+tlbType); dtlb.addParams(tlbParams)
#        itlbW = sst.Component(prefix+".itlb","mmu.tlb_wrapper"); itlbW.addParams(tlbWrapperParams); itlbW.addParam("exe",True)
#        itlb = itlbW.setSubComponent("tlb","mmu."+tlbType); itlb.addParams(tlbParams)
#
#        sst.Link(prefix+".d_tlb").connect((d_if,"lowlink","1ns"), (dtlbW,"cpu_if","1ns"))
#        sst.Link(prefix+".d_tlb_l1").connect((dtlbW,"cache_if","1ns"), (l1d,"highlink","1ns"))
#        sst.Link(prefix+".i_tlb").connect((i_if,"lowlink","1ns"), (itlbW,"cpu_if","1ns"))
#        sst.Link(prefix+".i_tlb_l1").connect((itlbW,"cache_if","1ns"), (l1i,"highlink","1ns"))
#        sst.Link(prefix+".l1d_bus").connect((l1d,"lowlink","1ns"), (bus,"highlink0","1ns"))
#        sst.Link(prefix+".l1i_bus").connect((l1i,"lowlink","1ns"), (bus,"highlink1","1ns"))
#        sst.Link(prefix+".bus_l2").connect((bus,"lowlink0","1ns"), (l2,"highlink","1ns"))
#        return (cpu, "os_link", "5ns"), (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns"), (nic, "port", "1ns")
#
#
#class CPU_Builder:
#    def build(self, prefix, nodeId, cid):
#        # CPU
#        cpu = sst.Component(prefix, vanadis_cpu_type)
#        cpu.addParams(cpuParams)
#        cpu.addParam("core_id", cid)
#        cpu.enableAllStatistics()
#
#        # Decoders (+ OS handler + branch unit)
#        for n in range(numThreads):
#            dec = cpu.setSubComponent("decoder"+str(n), vanadis_decoder)
#            dec.addParams(decoderParams); dec.enableAllStatistics()
#            osh = dec.setSubComponent("os_handler", vanadis_os_hdlr)
#            osh.addParams(osHdlrParams)
#            br  = dec.setSubComponent("branch_unit", "vanadis.VanadisBasicBranchUnit")
#            br.addParams(branchPredParams); br.enableAllStatistics()
#
#        # LSQ + mem IFs
#        lsq  = cpu.setSubComponent("lsq", "vanadis.VanadisBasicLoadStoreQueue")
#        lsq.addParams(lsqParams); lsq.enableAllStatistics()
#        d_if = lsq.setSubComponent("memory_interface", "memHierarchy.standardInterface")
#        i_if = cpu.setSubComponent("mem_interface_inst", "memHierarchy.standardInterface")
#
#        # RoCC + Array
#        cpu_rocc = cpu.setSubComponent("rocc", rocc_type, 0)
#        cpu_rocc.addParams(roccParams); cpu_rocc.enableAllStatistics()
#        roccD_if = cpu_rocc.setSubComponent("memory_interface", "memHierarchy.standardInterface")
#        arr = cpu_rocc.setSubComponent("array", array_type)
#        arr.addParams(arrayParams)
#
#        # Caches + MemLinks
#        l1d = sst.Component(prefix+".l1d", "memHierarchy.Cache")
#        l1d.addParams(l1dcacheParams)
#        l1d_high = l1d.setSubComponent("highlink", "memHierarchy.MemLink")
#        l1d_low  = l1d.setSubComponent("lowlink",  "memHierarchy.MemLink")
#
#        l1i = sst.Component(prefix+".l1i", "memHierarchy.Cache")
#        l1i.addParams(l1icacheParams)
#        l1i_high = l1i.setSubComponent("highlink", "memHierarchy.MemLink")
#        l1i_low  = l1i.setSubComponent("lowlink",  "memHierarchy.MemLink")
#
#        l2  = sst.Component(prefix+".l2", "memHierarchy.Cache")
#        l2.addParams(l2cacheParams)
#        l2_high = l2.setSubComponent("highlink", "memHierarchy.MemLink")
#        l2_low  = l2.setSubComponent("lowlink",  "memHierarchy.MemNIC")
#        l2_low.addParams(nicVN1)
#        l2pre = l2.setSubComponent("prefetcher", "cassini.StridePrefetcher")
#        l2pre.addParams(l2_prefetch_params)
#
#        # Buses
#        dbus    = sst.Component(prefix+".dbus",    "memHierarchy.Bus"); dbus.addParams(busParams)
#        corebus = sst.Component(prefix+".corebus", "memHierarchy.Bus"); corebus.addParams(busParams)
#
#        # TLBs (no MemLinks — use fixed ports)
#        dtlbW = sst.Component(prefix+".dtlb", "mmu.tlb_wrapper")
#        dtlbW.addParams(tlbWrapperParams)
#        dtlb  = dtlbW.setSubComponent("tlb", "mmu."+tlbType)
#        dtlb.addParams(tlbParams)
#
#        itlbW = sst.Component(prefix+".itlb", "mmu.tlb_wrapper")
#        itlbW.addParams(tlbWrapperParams)
#        itlbW.addParam("exe", True)
#        itlb  = itlbW.setSubComponent("tlb", "mmu."+tlbType)
#        itlb.addParams(tlbParams)
#
#        # ---- Wiring ----
#        # D-side: LSQ + RoCC → dbus → DTLB → L1D
#        sst.Link(prefix+".lsq_dbus").connect( (d_if,     "lowlink","1ns"), (dbus,    "highlink0","1ns") )
#        sst.Link(prefix+".rocc_dbus").connect((roccD_if, "lowlink","1ns"), (dbus,    "highlink1","1ns") )
#        sst.Link(prefix+".dbus_dtlb").connect((dbus,     "lowlink0","1ns"), (dtlbW,  "cpu_if","1ns") )
#        sst.Link(prefix+".dtlb_l1d").connect((dtlbW,     "cache_if","1ns"), (l1d_high, "port","1ns") )
#
#        # I-side: I IF → ITLB → L1I
#        sst.Link(prefix+".i_tlb").connect   ( (i_if,     "lowlink","1ns"), (itlbW, "cpu_if","1ns") )
#        sst.Link(prefix+".itlb_l1i").connect((itlbW,     "cache_if","1ns"), (l1i_high,"port","1ns") )
#
#        # L1s → corebus → L2
#        sst.Link(prefix+".l1d_bus").connect((l1d_low,"port","1ns"), (corebus,"highlink0","1ns"))
#        sst.Link(prefix+".l1i_bus").connect((l1i_low,"port","1ns"), (corebus,"highlink1","1ns"))
#        sst.Link(prefix+".bus_l2").connect ((corebus,"lowlink0","1ns"), (l2_high,"port","1ns"))
#
#        # Return endpoints for external wiring
#        return (cpu, "os_link", "5ns"), (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns"), (l2_low, "port", "1ns")


cpuBuilder = CPU_Builder()

# --------- Memory + Directory (Dir on Merlin, Mem behind Dir) ---------
def build_mem_dir(idx):
    memctrl = sst.Component(f"mem{idx}","memHierarchy.MemController")
    memctrl.addParams({"clock":mem_clock,"backend.mem_size":physMemSize,"backing":"malloc","initBacking":1,
                       "debug_level":mh_debug_level,"debug":mh_debug})

    if backingType=="simpleMem":
        back = memctrl.setSubComponent("backend","memHierarchy.simpleMem"); back.addParams(memParams)
    elif backingType=="simpleDRAM":
        back = memctrl.setSubComponent("backend","memHierarchy.simpleDRAM"); back.addParams(simpleTimingParams)
    else:
        back = memctrl.setSubComponent("backend","memHierarchy.timingDRAM"); back.addParams(timingParams)

    dirctrl = sst.Component(f"dir{idx}","memHierarchy.DirectoryController")
    dirctrl.addParams({"coherence_protocol":protocol,"entry_cache_size":32768,
        "debug":mh_debug,"debug_level":mh_debug_level, "interleave_size": "64B", "interleave_step": f"{numMemories*64}B",
        "addr_range_start": idx*64, "addr_range_end": (1<<30) - ((numMemories-idx)*64)+63})
    dir_to_mem = dirctrl.setSubComponent("lowlink","memHierarchy.MemLink")
    mem_to_dir = memctrl.setSubComponent("highlink","memHierarchy.MemLink")
    sst.Link(f"dir_mem_{idx}").connect((dir_to_mem,"port","1ns"), (mem_to_dir,"port","1ns"))

    dir_nic = dirctrl.setSubComponent("highlink","memHierarchy.MemNIC"); dir_nic.addParams(nicVN2)
    return dir_nic  # connect to Merlin

# --------- Merlin Router (single high-radix) ---------
local_ports = numCpus + numMemories + 1  # CPUs + DIRs + OS
neighbor_ports = 4
total_ports = local_ports + neighbor_ports
local_base = neighbor_ports

rtr = sst.Component("chiprtr","merlin.hr_router")
rtr.addParams({
    "id":"0","xbar_bw":"100GB/s","link_bw":"100GB/s","flit_size":"256B",
    "input_buf_size":"2KB","output_buf_size":"2KB","num_vns":3,
    "num_ports": str(total_ports),
})
topo = rtr.setSubComponent("topology","merlin.mesh")
topo.addParams({"shape":"1x1","width":"1x1","local_ports":local_ports,"link_latency":"1ns"})

# --------- Build CPUs, wire OS/MMU, attach to Merlin ---------
for cid in range(numCpus):
    prefix=f"cpu{cid}"
    os_hdlr, dtlb, itlb, l2nic = cpuBuilder.build(prefix, 0, cid)

    # MMU -> core TLBs
    sst.Link(prefix+".mmu_d").connect((node_os_mmu, f"core{cid}.dtlb","1ns"), dtlb)
    sst.Link(prefix+".mmu_i").connect((node_os_mmu, f"core{cid}.itlb","1ns"), itlb)
    sst.Link(prefix+".os").connect(os_hdlr, (node_os, f"core{cid}", "5ns"))
    sst.Link(prefix+".to_rtr").connect(l2nic, (rtr, f"port{local_base+cid}", "1ns"))

# Build memories+dirs, hook dirs to Merlin
for midx in range(numMemories):
    dir_nic = build_mem_dir(midx)
    sst.Link(f"dir{midx}_to_rtr").connect((dir_nic,"port","1ns"), (rtr, f"port{local_base+numCpus+midx}", "1ns"))

# OS NIC -> Merlin
os_port = local_base + numCpus + numMemories
sst.Link("os_to_rtr").connect((os_nic,"port","1ns"), (rtr, f"port{os_port}", "1ns"))

