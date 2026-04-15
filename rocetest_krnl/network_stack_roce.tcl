
# update ip catalog
# set __ip_list [get_property ip_repo_paths [current_project]]
# lappend __ip_list /home/tianlisong/bulk/thesis/fpga-network-stack/iprepo
# set_property ip_repo_paths $__ip_list [current_project]
# update_ip_catalog

# IPs used in rocetest kernel

#HLS IP cores

create_ip -name ip_handler -vendor ethz.systems.fpga -library hls -version 2.0 -module_name ip_handler_ip 
# generate_target {instantiation_template} [get_files $device_ip_dir/ip_handler_ip/ip_handler_ip.xci]
update_compile_order -fileset sources_1

create_ip -name mac_ip_encode -vendor ethz.systems.fpga -library hls -version 2.0 -module_name mac_ip_encode_ip 
# generate_target {instantiation_template} [get_files $device_ip_dir/mac_ip_encode_ip/mac_ip_encode_ip.xci]
update_compile_order -fileset sources_1

create_ip -name arp_server_subnet -vendor ethz.systems.fpga -library hls -version 1.1 -module_name arp_server_subnet_ip 
# generate_target {instantiation_template} [get_files $device_ip_dir/arp_server_subnet_ip/arp_server_subnet_ip.xci]
update_compile_order -fileset sources_1

create_ip -name rocev2 -vendor ethz.systems.fpga -library hls -version 0.82 -module_name rocev2_ip 
# generate_target {instantiation_template} [get_files $device_ip_dir/rocev2_ip/rocev2_ip.xci]
update_compile_order -fileset sources_1

# create_ip -name hash_table -vendor ethz.systems.fpga -library hls -version 1.0 -module_name hash_table_ip
# update_compile_order -fileset sources_1

#Register slices
create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name axis_register_slice_64 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_register_slice_64]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_register_slice_64/axis_register_slice_64.xci]
update_compile_order -fileset sources_1

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name axis_register_slice_128 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {16} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_register_slice_128]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_register_slice_128/axis_register_slice_128.xci]
update_compile_order -fileset sources_1

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name axis_register_slice_256 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {32} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_register_slice_256]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_register_slice_256/axis_register_slice_256.xci]
update_compile_order -fileset sources_1

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name axis_register_slice_512 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {64} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_register_slice_512]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_register_slice_512/axis_register_slice_512.xci]
update_compile_order -fileset sources_1


#FIFO / RX Buffer

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_512_cc 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {64} CONFIG.IS_ACLK_ASYNC {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_data_fifo_512_cc]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_512_cc/axis_data_fifo_512_cc.xci]
update_compile_order -fileset sources_1

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_64_d1024 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.FIFO_DEPTH {1024} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_WR_DATA_COUNT {1} CONFIG.HAS_RD_DATA_COUNT {1}] [get_ips axis_data_fifo_64_d1024]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_64_d1024/axis_data_fifo_64_d1024.xci]
update_compile_order -fileset sources_1

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_128_d1024 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {16} CONFIG.FIFO_DEPTH {1024} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_WR_DATA_COUNT {1} CONFIG.HAS_RD_DATA_COUNT {1}] [get_ips axis_data_fifo_128_d1024]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_128_d1024/axis_data_fifo_128_d1024.xci]
update_compile_order -fileset sources_1

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_256_d1024 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {32} CONFIG.FIFO_DEPTH {1024} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_WR_DATA_COUNT {1} CONFIG.HAS_RD_DATA_COUNT {1}] [get_ips axis_data_fifo_256_d1024]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_256_d1024/axis_data_fifo_256_d1024.xci]
update_compile_order -fileset sources_1

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_512_d1024 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {64} CONFIG.FIFO_DEPTH {1024} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_WR_DATA_COUNT {1} CONFIG.HAS_RD_DATA_COUNT {1}] [get_ips axis_data_fifo_512_d1024]
# generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_512_d1024/axis_data_fifo_512_d1024.xci]
update_compile_order -fileset sources_1

#Interconnects

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_merger_160 
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {20} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TKEEP {false} CONFIG.HAS_TLAST {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {false} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {20} CONFIG.S00_AXIS_TDATA_NUM_BYTES {20} CONFIG.S01_AXIS_TDATA_NUM_BYTES {20} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_merger_160]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_512_2to1 
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {64} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {64} CONFIG.S00_AXIS_TDATA_NUM_BYTES {64} CONFIG.S01_AXIS_TDATA_NUM_BYTES {64} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_512_2to1]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_512_4to1 
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {4} CONFIG.SWITCH_TDATA_NUM_BYTES {64} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_S03_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {64} CONFIG.S00_AXIS_TDATA_NUM_BYTES {64} CONFIG.S01_AXIS_TDATA_NUM_BYTES {64} CONFIG.S02_AXIS_TDATA_NUM_BYTES {64} CONFIG.S03_AXIS_TDATA_NUM_BYTES {64} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true} CONFIG.M00_S03_CONNECTIVITY {true}] [get_ips axis_interconnect_512_4to1]
update_compile_order -fileset sources_1

#Data Width Converter

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_64_to_512_converter 
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {8} CONFIG.M_TDATA_NUM_BYTES {64} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.TDEST_WIDTH {1}] [get_ips axis_64_to_512_converter]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_512_to_64_converter 
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {64} CONFIG.M_TDATA_NUM_BYTES {8} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1}] [get_ips axis_512_to_64_converter]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_128_to_512_converter 
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {16} CONFIG.M_TDATA_NUM_BYTES {64} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.TDEST_WIDTH {1}] [get_ips axis_128_to_512_converter]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_512_to_128_converter 
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {64} CONFIG.M_TDATA_NUM_BYTES {16} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1}] [get_ips axis_512_to_128_converter]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_256_to_512_converter 
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {32} CONFIG.M_TDATA_NUM_BYTES {64} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.TDEST_WIDTH {1}] [get_ips axis_256_to_512_converter]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_512_to_256_converter 
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {64} CONFIG.M_TDATA_NUM_BYTES {32} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1}] [get_ips axis_512_to_256_converter]
update_compile_order -fileset sources_1

#DDR
create_ip -name axi_datamover -vendor xilinx.com -library ip -version 5.1 -module_name axi_datamover_mem 
set_property -dict [list CONFIG.c_mm2s_stscmd_is_async {true} CONFIG.c_m_axi_mm2s_data_width {512} CONFIG.c_m_axis_mm2s_tdata_width {512} CONFIG.c_mm2s_burst_size {64} CONFIG.c_mm2s_btt_used {23} CONFIG.c_s2mm_stscmd_is_async {true} CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {512} CONFIG.c_s2mm_burst_size {64} CONFIG.c_s2mm_btt_used {23} CONFIG.c_s2mm_include_sf {false} CONFIG.c_m_axi_mm2s_id_width {1} CONFIG.c_m_axi_s2mm_id_width {1} CONFIG.c_addr_width {64}] [get_ips axi_datamover_mem]
update_compile_order -fileset sources_1

create_ip -name axi_datamover -vendor xilinx.com -library ip -version 5.1 -module_name axi_datamover_mem_unaligned 
set_property -dict [list CONFIG.c_mm2s_stscmd_is_async {true} CONFIG.c_m_axi_mm2s_data_width {512} CONFIG.c_m_axis_mm2s_tdata_width {512} CONFIG.c_include_mm2s_dre {true} CONFIG.c_mm2s_burst_size {64} CONFIG.c_mm2s_btt_used {23} CONFIG.c_s2mm_stscmd_is_async {true} CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {512} CONFIG.c_include_s2mm_dre {true} CONFIG.c_s2mm_burst_size {64} CONFIG.c_s2mm_btt_used {23} CONFIG.c_s2mm_include_sf {false} CONFIG.c_m_axi_mm2s_id_width {1} CONFIG.c_m_axi_s2mm_id_width {1} CONFIG.c_addr_width {64}] [get_ips axi_datamover_mem_unaligned]
update_compile_order -fileset sources_1

# ila
create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_mem_inf
set_property -dict [list CONFIG.C_NUM_OF_PROBES {8} CONFIG.C_EN_STRG_QUAL {1} CONFIG.C_ADV_TRIGGER {true} CONFIG.C_INPUT_PIPE_STAGES {1} ] [get_ips ila_mem_inf]
update_compile_order -fileset sources_1

create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_stack_top
set_property -dict [list CONFIG.C_NUM_OF_PROBES {26} CONFIG.C_PROBE2_WIDTH {512} \
CONFIG.C_PROBE5_WIDTH {512} CONFIG.C_PROBE8_WIDTH {160} CONFIG.C_PROBE9_WIDTH {32} \
CONFIG.C_PROBE10_WIDTH {8} CONFIG.C_PROBE13_WIDTH {87} CONFIG.C_PROBE16_WIDTH {87}\
CONFIG.C_PROBE18_WIDTH {32} CONFIG.C_PROBE19_WIDTH {32}\
CONFIG.C_PROBE22_WIDTH {512} CONFIG.C_PROBE25_WIDTH {512}\
CONFIG.C_EN_STRG_QUAL {1} CONFIG.C_ADV_TRIGGER {true} \
CONFIG.C_INPUT_PIPE_STAGES {1}] [get_ips ila_stack_top]
update_compile_order -fileset sources_1

create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_stack_top_inter
set_property -dict [list CONFIG.C_NUM_OF_PROBES {40} \
CONFIG.C_PROBE2_WIDTH {512} CONFIG.C_PROBE5_WIDTH {512} CONFIG.C_PROBE8_WIDTH {512} \
CONFIG.C_PROBE9_WIDTH {32} CONFIG.C_PROBE10_WIDTH {8} \
CONFIG.C_PROBE13_WIDTH {512} CONFIG.C_PROBE16_WIDTH {512} CONFIG.C_PROBE19_WIDTH {512} \
CONFIG.C_PROBE20_WIDTH {32} CONFIG.C_PROBE21_WIDTH {32} \
CONFIG.C_PROBE22_WIDTH {32} CONFIG.C_PROBE24_WIDTH {32} \
CONFIG.C_PROBE26_WIDTH {32} CONFIG.C_PROBE27_WIDTH {32}\
CONFIG.C_PROBE30_WIDTH {96} CONFIG.C_PROBE33_WIDTH {96}\
CONFIG.C_PROBE36_WIDTH {512} CONFIG.C_PROBE39_WIDTH {512}\
CONFIG.C_EN_STRG_QUAL {1} CONFIG.C_ADV_TRIGGER {true} \
CONFIG.C_INPUT_PIPE_STAGES {1}] [get_ips ila_stack_top_inter]
update_compile_order -fileset sources_1
