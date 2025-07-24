# Create external_projects directory if it doesn't exist
mkdir -p external_projects

# Function to clone a Git repository if it doesn't exist
clone_repo() {
  local dir=$1
  local git_url=$2
  local branch=$3

  if [ ! -d "external_projects/$dir" ]; then
    git clone --single-branch --branch "$branch" "$git_url" "external_projects/$dir"
  else
    echo "Directory external_projects/$dir already exists."
  fi
}


# Clone RISCV GNU Toolchain
clone_repo riscv-gnu-toolchain \
           https://github.com/riscv-collab/riscv-gnu-toolchain.git \
           master


# Clone LLVM Project
clone_repo llvm-project \
           https://github.com/PlatinumCD/llvm-project.git \
           llvm-riscv


# Clone Torch MLIR
clone_repo torch-mlir \
           https://github.com/PlatinumCD/torch-mlir.git \
           analog-dialect


# Clone SST Core
clone_repo sst-core \
           https://github.com/sstsimulator/sst-core.git \
           v15.0.0_Final


# Clone SST Elements
clone_repo sst-elements \
           https://github.com/sstsimulator/sst-elements.git \
           v15.0.0_Final
