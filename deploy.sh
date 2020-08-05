obj=${1:-libquantum}
(make ${obj}) || exit 1
src="x86-tmac"
dst="arm-tmac"
scp ${obj}/final_main_x86_64 ${src}:~/workspace/sgx-driver/enclave/${obj}
scp ${obj}/final_main_x86_64 ${src}:~/workspace/sgx-driver/enclave/${obj}_x86_64
scp ${obj}/final_main_aarch64 ${src}:~/workspace/sgx-driver/enclave/${obj}_aarch64
scp ${obj}/final_main_aarch64 ${dst}:~/workspace/sgx-driver/enclave/${obj}
scp build/x86_64/linker.lds ${src}:~/workspace/sgx-driver/enclave/linker.lds
scp build/aarch64/linker.lds ${dst}:~/workspace/sgx-driver/enclave/linker.lds
# scp ${obj}/final_main_x86_64 tmac-arm:~/workspace/sgx-driver/enclave/${obj}_x86_64
# scp ${obj}/final_main_aarch64 tmac-arm:~/workspace/sgx-driver/enclave/${obj}_aarch64
