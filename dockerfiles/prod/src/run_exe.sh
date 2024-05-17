
export CROSS_SIM_JSON=/local/src/sst_env/cross_sim_params.json
export VANADIS_EXE=/local/src/prog_cpp.out
export VANADIS_VERBOSE=20

export VANADIS_NUM_ARRAYS=1
export VANADIS_INPUT_ARR_SIZE=6
export VANADIS_OUTPUT_ARR_SIZE=5
export VANADIS_INPUT_OP_SIZE=1
export VANADIS_OUTPUT_OP_SIZE=4


pushd sst_env
sst mvm_rocc_vanadis.py
popd
