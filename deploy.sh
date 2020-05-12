working_path=${1:-helloworld}
(make ${working_path}) || exit 1
scp ${working_path}/final_main_x86_64 wxy:~/workspace/sgx-driver/enclave/enclave
scp ${working_path}/final_main_x86_64 wxy:~/workspace/sgx-driver/enclave/enclave_x86_64
scp ${working_path}/final_main_aarch64 wxy:~/workspace/sgx-driver/enclave/enclave_aarch64
scp ${working_path}/final_main_aarch64 tmac-arm:~/workspace/sgx-driver/enclave/enclave
scp build/x86_64/linker.lds wxy:~/workspace/sgx-driver/enclave/linker.lds
scp build/aarch64/linker.lds tmac-arm:~/workspace/sgx-driver/enclave/linker.lds
# scp ${working_path}/final_main_x86_64 tmac-arm:~/workspace/sgx-driver/enclave/enclave_x86_64
# scp ${working_path}/final_main_aarch64 tmac-arm:~/workspace/sgx-driver/enclave/enclave_aarch64
