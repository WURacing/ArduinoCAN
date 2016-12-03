#! /bin/bash
# This script formats data obtained by the sd card logger program.
# It creates files for suspension data, rpm/throttle/load/coolant data, and O2/speed/gear/voltage (each in separate folders).
#
# Usage: Put all the collected CSV files into a folder (called autox_logs) then run the script from its parent directory.

count=0;
for file in autox_logs/*;
do
	mkdir -p suspension_data;
	mkdir -p rpm_load_throttle_coolant_data;
	mkdir -p o2_speed_gear_voltage_data;
	suspensionFileName="suspension_$count.csv";
	rpmFileName="rpm_$count.csv";
	o2FileName="O2_$count.csv";
	awk -F, '$4==""' $file>suspension_data/$suspensionFileName;
	grep '^RPM_LOAD_THROTTLE_COOLANT' $file>rpm_load_throttle_coolant_data/$rpmFileName;
	grep '^O2_SPEED_GEAR_VOLTAGE' $file>o2_speed_gear_voltage_data/$o2FileName;
	let count=count+1;
done

