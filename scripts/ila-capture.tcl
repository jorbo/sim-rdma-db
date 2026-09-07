# Capture ILA state from a live U280 xclbin (ila_stack_top in rocetest_krnl).
#
# Run via Vivado Lab (or full Vivado) on the machine hosting the card,
# with hw_server already running:
#
#   vivado_lab -mode batch -source ila-capture.tcl \
#       -tclargs <probes.ltx> <out_prefix> [snapshot|trigger] [probe_glob]
#
# Modes:
#   snapshot (default)  trigger immediately; use post-hang to read the
#                       free-running counters (tx/rx pkg, crc/psn drop).
#   trigger             arm on a rising edge of the first probe matching
#                       probe_glob (e.g. *tx_meta*valid*), then wait; run
#                       the head-node workload while this is armed.
#
# Outputs: <out_prefix>.<ila>.probes.txt (probe name list),
#          <out_prefix>.<ila>.ila / .csv  (captured data)

set ltx      [lindex $argv 0]
set out      [lindex $argv 1]
set mode     "snapshot"
set probe_pat ""
if {[llength $argv] > 2} { set mode      [lindex $argv 2] }
if {[llength $argv] > 3} { set probe_pat [lindex $argv 3] }

if {$ltx eq "" || $out eq ""} {
    puts "ERROR: usage: -tclargs <probes.ltx> <out_prefix> \[snapshot|trigger\] \[probe_glob\]"
    exit 2
}
if {![file exists $ltx]} {
    puts "ERROR: probes file not found: $ltx"
    exit 2
}

open_hw_manager
connect_hw_server -url localhost:3121

set targets [get_hw_targets -quiet]
if {[llength $targets] == 0} {
    puts "ERROR: no hardware targets — is hw_server running and the card's debug bridge visible?"
    exit 1
}
open_hw_target [lindex $targets 0]

# Pick the U280 user FPGA (skip system controller devices).
set dev ""
foreach d [get_hw_devices] {
    if {[string match -nocase "xcu280*" [get_property PART $d]]} { set dev $d }
}
if {$dev eq ""} { set dev [lindex [get_hw_devices] 0] }
current_hw_device $dev
puts "Using device: $dev (part [get_property PART $dev])"

set_property PROBES.FILE      $ltx $dev
set_property FULL_PROBES.FILE $ltx $dev
refresh_hw_device -update_hw_probes true $dev

set ilas [get_hw_ilas -quiet -of_objects $dev]
if {[llength $ilas] == 0} {
    puts "ERROR: no ILA cores found — wrong .ltx for this xclbin, or debug bridge not linked"
    exit 1
}
puts "Found [llength $ilas] ILA core(s): $ilas"

set rc 0
foreach ila $ilas {
    set tag [string map {/ _ : _} $ila]

    # Always dump the probe name list first: exact names vary with the
    # link run, and the trigger probe_glob is matched against these.
    set probes [get_hw_probes -of_objects $ila]
    set pf [open "${out}.${tag}.probes.txt" w]
    foreach p $probes {
        puts $pf "[get_property NAME $p]  width=[get_property WIDTH $p]"
    }
    close $pf
    puts "Wrote ${out}.${tag}.probes.txt ([llength $probes] probes)"

    if {$mode eq "trigger"} {
        if {$probe_pat eq ""} {
            puts "ERROR: trigger mode needs a probe_glob argument"
            set rc 2
            continue
        }
        set tp [lindex [get_hw_probes -quiet ${probe_pat} -of_objects $ila] 0]
        if {$tp eq ""} {
            puts "WARNING: no probe matches '$probe_pat' on $ila — skipping (see probes.txt)"
            set rc 3
            continue
        }
        puts "Arming $ila on rising edge of [get_property NAME $tp]"
        set_property CONTROL.TRIGGER_POSITION 512 $ila
        set_property TRIGGER_COMPARE_VALUE eq1'bR $tp
        run_hw_ila $ila
        # wait_on_hw_ila timeout is in minutes.
        if {[catch {wait_on_hw_ila -timeout 5 $ila} err]} {
            puts "WARNING: trigger did not fire within 5 min on $ila ($err); forcing capture"
            run_hw_ila -trigger_now $ila
            catch {wait_on_hw_ila -timeout 1 $ila}
        }
    } else {
        set_property CONTROL.TRIGGER_POSITION 0 $ila
        run_hw_ila -trigger_now $ila
        catch {wait_on_hw_ila -timeout 1 $ila}
    }

    set data [upload_hw_ila_data $ila]
    write_hw_ila_data -force "${out}.${tag}.ila" $data
    write_hw_ila_data -force -csv_file "${out}.${tag}.csv" $data
    puts "Wrote ${out}.${tag}.ila and .csv"
}

close_hw_manager
exit $rc
