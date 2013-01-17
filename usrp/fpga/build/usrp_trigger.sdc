###########################################################################
#
# Generated by : Version 9.0 Build 132 02/25/2009 SJ Full Version
#
# Project      : usrp_trigger
# Revision     : usrp_trigger
#
# Date         : Fri Apr 03 02:46:06 EDT 2009
#
###########################################################################
 
 

 create_clock -name master_clk -period 15.625 [get_ports {master_clk}]
 create_clock -name usb_clk -period 20.833 [get_ports {usbclk}]
 create_generated_clock -name master_pll_clk -source [get_pins {pll|altpll_component|pll|inclk[0]}] [get_pins {pll|altpll_component|pll|clk[0]}]
 create_generated_clock -name usb_pll_clk -source [get_pins {pll2|altpll_component|pll|inclk[0]}] [get_pins {pll2|altpll_component|pll|clk[0]}]
 create_clock -name gate_clk -period 500000 -waveform {0 400000} [get_ports {io_rx_a[15]}]
 create_clock -name SCLK -period 20000 [get_ports {SCLK}]
set_false_path -from [get_registers {master_cntrl:mcm|setting_reg:sr_decim|out[3]}] -to [get_registers *rd_gate*]
create_generated_clock -name strobe -source [get_pins {pll|altpll_component|pll|clk[0]}] -divide_by 4 -duty_cycle 25 [get_registers {master_cntrl:mcm|strobe_gen:ds|strobe}]
set_false_path -to [get_ports {io_rx_b[0] io_rx_b[1] io_rx_b[2] io_rx_b[3] io_rx_b[4] io_rx_b[5] io_rx_b[6] io_rx_b[7] io_rx_b[8] io_rx_b[9] io_rx_b[10] io_rx_b[11] io_rx_b[12] io_rx_b[13] io_rx_b[14] io_rx_b[15]}]
set_false_path -to [get_ports {io_tx_a[0] io_tx_a[1] io_tx_a[2] io_tx_a[3] io_tx_a[4] io_tx_a[5] io_tx_a[6] io_tx_a[7] io_tx_a[8] io_tx_a[9] io_tx_a[10] io_tx_a[11] io_tx_a[12] io_tx_a[13] io_tx_a[14] io_tx_a[15] io_tx_b[0] io_tx_b[1] io_tx_b[2] io_tx_b[3] io_tx_b[4] io_tx_b[5] io_tx_b[6] io_tx_b[7] io_tx_b[8] io_tx_b[9] io_tx_b[10] io_tx_b[11] io_tx_b[12] io_tx_b[13] io_tx_b[14] io_tx_b[15]}]