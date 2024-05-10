
EXE=prog_cpp.out
JSON=cross_sim_params.json

docker run -it \
    -v $(pwd):/local \
    -e VANADIS_EXE=/local/$EXE \
    -e CROSS_SIM_JSON=/local/$JSON \
    -e VANADIS_VERBOSE=0 \
    sst-crosssim-dev
