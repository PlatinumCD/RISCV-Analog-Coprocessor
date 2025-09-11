// Copyright 2009-2025 NTESS.
// This file is part of the SST software package.

#ifndef _H_ANALOG_ROCC
#define _H_ANALOG_ROCC

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>

#include <sst/elements/vanadis/rocc/vroccinterface.h>
#include <sst/elements/golem/array/computeArray.h>

#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>
#include <algorithm>
#include <type_traits>

using namespace SST::Interfaces;
using namespace SST::Golem;

namespace SST {
namespace Golem {

template <typename T>
class RoCCAnalog : public SST::Vanadis::VanadisRoCCInterface {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(RoCCAnalog<T>, SST::Vanadis::VanadisRoCCInterface)

    RoCCAnalog(ComponentId_t id, Params& params)
    : VanadisRoCCInterface(id, params),
      max_instructions(params.find<size_t>("max_instructions", 8))
    {
        // Clock sanity (lets you catch bad params early)
        try {
            UnitAlgebra clk = params.find<UnitAlgebra>("clock", "1GHz");
            if (!(clk.hasUnits("Hz") || clk.hasUnits("s")) || clk.getRoundedValue() <= 0) {
                output->fatal(CALL_INFO, -1, "%s invalid 'clock'=%s\n",
                              getName().c_str(), clk.toString().c_str());
            }
        } catch (const UnitAlgebra::UnitAlgebraException& exc) {
            output->fatal(CALL_INFO, -1, "%s bad 'clock': %s\n", getName().c_str(), exc.what());
        }

        // Core params
        numArrays         = params.find<uint32_t>("numArrays", 1);
        arrayInputSize    = params.find<uint32_t>("arrayInputSize", 2);
        arrayOutputSize   = params.find<uint32_t>("arrayOutputSize", 2);
        inputOperandSize  = params.find<uint32_t>("inputOperandSize", 4);
        outputOperandSize = params.find<uint32_t>("outputOperandSize", 4);

        // Memory interface: deliver all mem responses to processIncomingRequest(...)
        memIF = loadUserSubComponent<SST::Interfaces::StandardMem>(
            "memory_interface",
            ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS,
            getTimeConverter("1ps"),
            new SST::Interfaces::StandardMem::Handler2<
                RoCCAnalog<T>,
                &RoCCAnalog<T>::processIncomingRequest>(this)
        );
        if (!memIF) {
            output->fatal(CALL_INFO, -1, "%s failed to load memory_interface\n", getName().c_str());
        }

        // Array model: completion comes via handleArrayEvent(...)
        array = loadUserSubComponent<SST::Golem::ComputeArray>(
            "array",
            ComponentInfo::SHARE_NONE,
            getTimeConverter("1ps"),
            new SST::Event::Handler2<
                RoCCAnalog<T>,
                &RoCCAnalog<T>::handleArrayEvent>(this)
        );
        if (!array) {
            output->fatal(CALL_INFO, -1, "%s failed to load array subcomponent\n", getName().c_str());
        }

        output->verbose(CALL_INFO, 1, 0,
            "%s: arrays=%u in=%u*%u out=%u*%u\n",
            getName().c_str(),
            numArrays, arrayInputSize, inputOperandSize, arrayOutputSize, outputOperandSize);
    }

    ~RoCCAnalog() override {
        for (auto* c : roccQ) delete c;
        roccQ.clear();
        if (curr_resp) { delete curr_resp; curr_resp = nullptr; }
    }

    // ---- VanadisRoCCInterface ----
    bool   RoCCFull() override      { return roccQ.size() >= max_instructions; }
    bool   isBusy() override        { return busy; }
    size_t roccQueueSize() override { return roccQ.size(); }

    void push(SST::Vanadis::RoCCCommand* c) override {
        stat_rocc_issued->addData(1);
        roccQ.push_back(c);
    }

    SST::Vanadis::RoCCResponse* respond() override {
        auto* r = curr_resp; curr_resp = nullptr; return r;
    }

    void init(unsigned int phase) override {
        memIF->init(phase);
        array->init(phase);
        unsigned L = memIF->getLineSize();
        lineSize = (L == 0 ? 64 : L);
        if (phase == 0) {
            arrayBusy.assign(numArrays, false);
            output->verbose(CALL_INFO, 2, 0, "%s: lineSize=%u\n", getName().c_str(), lineSize);
        }
    }

    void tick(uint64_t /*cycle*/) override {
        if (busy || roccQ.empty()) return;

        busy     = true;
        curr_cmd = roccQ.front();
        resetOpState();

        const uint64_t rs1 = curr_cmd->rs1; // address (phys per your usage)
        const uint64_t rs2 = curr_cmd->rs2; // array id

        switch (curr_cmd->inst->func7) {
            case 0x1: // mvm.set: set matrix from memory
                output->verbose(CALL_INFO, 9, 0, "%s: mvm.set addr=0x%" PRIx64 " aid=%" PRIu64 "\n",
                                getName().c_str(), rs1, rs2);
                startSetMatrix(rs1, static_cast<uint32_t>(rs2));
                break;

            case 0x2: // mvm.l: load input vector
                output->verbose(CALL_INFO, 9, 0, "%s: mvm.l addr=0x%" PRIx64 " aid=%" PRIu64 "\n",
                                getName().c_str(), rs1, rs2);
                startLoadVector(rs1, static_cast<uint32_t>(rs2));
                break;

            case 0x3: // mvm: compute
                output->verbose(CALL_INFO, 9, 0, "%s: mvm.compute aid=%" PRIu64 "\n",
                                getName().c_str(), rs2);
                startCompute(static_cast<uint32_t>(rs2));
                break;

            case 0x4: // mvm.s: store output vector to memory
                output->verbose(CALL_INFO, 9, 0, "%s: mvm.s addr=0x%" PRIx64 " aid=%" PRIu64 "\n",
                                getName().c_str(), rs1, rs2);
                startStoreVector(rs1, static_cast<uint32_t>(rs2));
                break;

            case 0x5: // mvm.mv: move output->input within arrays
                output->verbose(CALL_INFO, 9, 0, "%s: mvm.mv src=%" PRIu64 " dst=%" PRIu64 "\n",
                                getName().c_str(), rs1, rs2);
                array->moveOutputToInput(static_cast<uint32_t>(rs1),
                                         static_cast<uint32_t>(rs2));
                completeRoCC(0);
                break;

            default:
                output->verbose(CALL_INFO, 0, 0, "%s: unknown func7=0x%x\n",
                                getName().c_str(), curr_cmd->inst->func7);
                completeRoCC(1);
                break;
        }
    }

    // ---- Mem response pump (called via Handler2) ----
    void processIncomingRequest(Interfaces::StandardMem::Request* req) {
        using namespace Interfaces;
        if (auto* r = dynamic_cast<StandardMem::ReadResp*>(req)) {
            handleReadResp(r);
        } else if (auto* w = dynamic_cast<StandardMem::WriteResp*>(req)) {
            handleWriteResp(w);
        } else {
            output->verbose(CALL_INFO, 0, 0, "%s: unexpected mem response\n", getName().c_str());
        }
        delete req;
    }

    // ---- Array completion ----
    void handleArrayEvent(Event* ev) {
        auto* aev = static_cast<SST::Golem::ArrayEvent*>(ev);
        uint32_t aid = aev->getArrayID();
        if (aid < arrayBusy.size()) arrayBusy[aid] = false;
        if (curOp == CurOp::Compute) completeRoCC(0);
        delete ev;
    }

private:
    enum class CurOp { None, SetMatrix, LoadVec, StoreVec, Compute };

    // ---- Op lifecycle helpers ----
    void resetOpState() {
        curOp              = CurOp::None;
        arrayID            = 0;
        rdBase             = 0;
        wrBase             = 0;
        readOffset         = 0;
        readTotal          = 0;
        writeOffset        = 0;
        writeTotal         = 0;
        outputPayload.clear();
    }

    // Single-outstanding, cacheline-chunked reads
    void sendNextReadChunk() {
        if (readOffset >= readTotal) { completeRoCC(0); return; }

        const uint64_t addr  = rdBase + readOffset;
        const uint64_t align = (lineSize ? (addr % lineSize) : 0);
        const uint32_t size  = (readOffset == 0)
            ? static_cast<uint32_t>(std::min<uint64_t>(lineSize - align, readTotal - readOffset))
            : static_cast<uint32_t>(std::min<uint64_t>(lineSize,         readTotal - readOffset));

        auto* r = new Interfaces::StandardMem::Read(addr, size /*flags=0*/);
        memIF->send(r);
        // NOTE: We advance offset on response, not here (one in-flight)
    }

    // Single-outstanding, cacheline-chunked writes
    void sendNextWriteChunk() {
        if (writeOffset >= writeTotal) { completeRoCC(0); return; }

        const uint64_t addr  = wrBase + writeOffset;
        const uint64_t align = (lineSize ? (addr % lineSize) : 0);
        const uint32_t size  = (writeOffset == 0)
            ? static_cast<uint32_t>(std::min<uint64_t>(lineSize - align, writeTotal - writeOffset))
            : static_cast<uint32_t>(std::min<uint64_t>(lineSize,         writeTotal - writeOffset));

        std::vector<uint8_t> chunk(outputPayload.begin() + writeOffset,
                                   outputPayload.begin() + writeOffset + size);

        auto* w = new Interfaces::StandardMem::Write(
            addr, size, chunk,
            false /*noncacheable*/, 0 /*writeThrough*/,
            addr /*vAddr*/, 0, 0
        );
        memIF->send(w);
        // Advance on response (one in-flight)
    }

    // ---- Start ops ----
    void startSetMatrix(uint64_t base, uint32_t aid) {
        curOp      = CurOp::SetMatrix;
        arrayID    = aid;
        rdBase     = base;
        readOffset = 0;
        // matrix bytes: (rows=arrayOutputSize) x (cols=arrayInputSize) x elemSize
        readTotal  = static_cast<uint64_t>(arrayOutputSize) *
                     static_cast<uint64_t>(arrayInputSize) *
                     static_cast<uint64_t>(inputOperandSize);
        sendNextReadChunk();
    }

    void startLoadVector(uint64_t base, uint32_t aid) {
        curOp      = CurOp::LoadVec;
        arrayID    = aid;
        rdBase     = base;
        readOffset = 0;
        // vector bytes: arrayInputSize x elemSize
        readTotal  = static_cast<uint64_t>(arrayInputSize) *
                     static_cast<uint64_t>(inputOperandSize);
        sendNextReadChunk();
    }

    void startCompute(uint32_t aid) {
        curOp   = CurOp::Compute;
        arrayID = aid;
        if (arrayID >= arrayBusy.size()) arrayBusy.resize(arrayID + 1, false);
        arrayBusy[arrayID] = true;
        array->beginComputation(arrayID); // completion via handleArrayEvent
    }

    void startStoreVector(uint64_t dst, uint32_t aid) {
        curOp       = CurOp::StoreVec;
        arrayID     = aid;
        wrBase      = dst;
        writeOffset = 0;
        // bytes: arrayOutputSize x elemSize(out)
        writeTotal  = static_cast<uint64_t>(arrayOutputSize) *
                      static_cast<uint64_t>(outputOperandSize);

        outputPayload.resize(writeTotal);

        // Pack output vector into byte payload
        auto& outVec = *static_cast<std::vector<T>*>(array->getOutputVector(arrayID));
        for (size_t i = 0; i < static_cast<size_t>(arrayOutputSize); ++i) {
            T v = outVec[i];
            const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
            for (size_t j = 0; j < static_cast<size_t>(outputOperandSize); ++j) {
                outputPayload[i * outputOperandSize + j] = p[j];
            }
        }
        sendNextWriteChunk();
    }

    // ---- Mem responses ----
    void handleReadResp(Interfaces::StandardMem::ReadResp* ev) {
        if (ev->getFail()) {
            output->verbose(CALL_INFO, 0, 0, "%s: ReadResp FAIL vAddr=0x%" PRI_ADDR "\n",
                            getName().c_str(), ev->vAddr);
            completeRoCC(1);
            return;
        }

        const auto& bytes = ev->data;
        const uint64_t baseBefore = readOffset;

        if (curOp == CurOp::SetMatrix) {
            for (size_t i = 0; i < bytes.size(); i += inputOperandSize) {
                T v{};
                std::memcpy(&v, &bytes[i], inputOperandSize);
                size_t idx = (baseBefore + i) / inputOperandSize;
                array->setMatrixItem(arrayID, static_cast<int>(idx), v);
            }
        } else if (curOp == CurOp::LoadVec) {
            for (size_t i = 0; i < bytes.size(); i += inputOperandSize) {
                T v{};
                std::memcpy(&v, &bytes[i], inputOperandSize);
                size_t idx = (baseBefore + i) / inputOperandSize;
                array->setVectorItem(arrayID, static_cast<int>(idx), v);
            }
        } else {
            output->verbose(CALL_INFO, 0, 0, "%s: ReadResp w/ invalid curOp\n", getName().c_str());
            completeRoCC(1);
            return;
        }

        readOffset += bytes.size();
        if (readOffset < readTotal) sendNextReadChunk();
        else                        completeRoCC(0);
    }

    void handleWriteResp(Interfaces::StandardMem::WriteResp* ev) {
        if (ev->getFail()) {
            output->verbose(CALL_INFO, 0, 0, "%s: WriteResp FAIL vAddr=0x%" PRI_ADDR "\n",
                            getName().c_str(), ev->vAddr);
            completeRoCC(1);
            return;
        }

        if (curOp != CurOp::StoreVec) {
            output->verbose(CALL_INFO, 0, 0, "%s: WriteResp w/ invalid curOp\n", getName().c_str());
            completeRoCC(1);
            return;
        }

        // Advance by the chunk size we sent (encoded in request length; resp doesn't echo it)
        // We sent one outstanding write at a time; use lineSize step capped by remaining.
        const uint32_t last =
            (writeOffset == 0)
            ? static_cast<uint32_t>(std::min<uint64_t>(lineSize - ((wrBase + writeOffset) % lineSize),
                                                       writeTotal - writeOffset))
            : static_cast<uint32_t>(std::min<uint64_t>(lineSize, writeTotal - writeOffset));

        writeOffset += last;
        if (writeOffset < writeTotal) sendNextWriteChunk();
        else                          completeRoCC(0);
    }

    // ---- Complete and produce response back to the core ----
    void completeRoCC(uint64_t rd_val) {
        auto* finished = curr_cmd;
        curr_cmd = nullptr;

        roccQ.pop_front();
        busy = false;

        curr_resp = new SST::Vanadis::RoCCResponse(finished->inst->rd, rd_val);
        delete finished;

        resetOpState();
    }

private:
    // Queue & response
    std::deque<SST::Vanadis::RoCCCommand*> roccQ;
    SST::Vanadis::RoCCCommand*             curr_cmd {nullptr};
    SST::Vanadis::RoCCResponse*            curr_resp{nullptr};
    bool                                   busy{false};
    size_t                                 max_instructions;

    // Subcomponents
    SST::Interfaces::StandardMem* memIF {nullptr};
    SST::Golem::ComputeArray*     array {nullptr};

    // Params / sizes
    uint32_t numArrays{1};
    uint32_t arrayInputSize{0};
    uint32_t arrayOutputSize{0};
    uint32_t inputOperandSize{4};
    uint32_t outputOperandSize{4};
    unsigned lineSize{64};

    // Array bookkeeping
    std::vector<bool> arrayBusy;

    // Current operation state
    CurOp     curOp{CurOp::None};
    uint32_t  arrayID{0};

    uint64_t  rdBase{0};
    uint64_t  readOffset{0};
    uint64_t  readTotal{0};

    uint64_t  wrBase{0};
    uint64_t  writeOffset{0};
    uint64_t  writeTotal{0};
    std::vector<uint8_t> outputPayload;
};

} // namespace Golem
} // namespace SST

#endif // _H_ANALOG_ROCC

